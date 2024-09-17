/*
 * Copyright (c) 2016-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2016-2024 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#define NV_DDS_UTILITY_VALUES
#include "nv_dds.h"

#include "dxgiformat.h"  // Included in third_party's dxh subproject
#include "texture_formats.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <type_traits>
#include <vector>

#ifdef _MSC_VER
#include <intrin.h>  // For BitScanReverse
#endif

namespace nv_dds {

///////////////////////////////////////////////////////////////////////////////
// Private types and functions
///////////////////////////////////////////////////////////////////////////////

namespace {
// Resizing a vector can produce an exception if the allocation fails, but
// there's no real way to validate this ahead of time. So we use
// a try/catch block.
template <class T>
ErrorWithText resizeVectorOrError(std::vector<T>& vec, size_t newSize) noexcept
{
  try
  {
    vec.resize(newSize, T{});
  }
  catch(...)
  {
    return "Allocating " + std::to_string(newSize) + " bytes of data failed.";
  }
  return {};
}

// Returns the index of the highest set bit of a mask, or 0 if mask == 0.
uint32_t highestSetBit(uint32_t mask)
{
  if(mask == 0)
    return 0;
#ifdef _MSC_VER
  unsigned long bit = 0;
  _BitScanReverse(&bit, mask);
  return bit;
#else
  return static_cast<uint32_t>(31 - __builtin_clz(mask));
#endif
}

// Returns the index of the lowest set bit of a mask, or 0 if mask == 0.
uint32_t lowestSetBit(uint32_t mask)
{
  if(mask == 0)
    return 0;
#ifdef _MSC_VER
  unsigned long bit = 0;
  _BitScanForward(&bit, mask);
  return bit;
#else
  return static_cast<uint32_t>(__builtin_ctz(mask));
#endif
}

// Returns the bit width (number of bits between and including the highest and
// lowest set bit) of a mask.
// Examples: 00000000 00000011 10000000 00000000 -> 3
//           00000000 10000000 00000001 00000000 -> 16
uint32_t maskBitWidth(uint32_t mask)
{
  if(mask == 0)
    return 0;
  return highestSetBit(mask) - lowestSetBit(mask) + 1;
}

uint32_t popcnt(uint32_t mask)
{
#ifdef _MSC_VER
  return static_cast<uint32_t>(__popcnt(mask));
#else
  return static_cast<uint32_t>(__builtin_popcount(mask));
#endif
}

struct BitmaskMultiplier
{
  uint32_t mask       = 0;
  float    multiplier = 0.0F;
  uint32_t leftshift  = 0;
};

// This function returns the constants needed to convert a bit representation
// to a UNORM or SNORM value.
// The DDS file format doesn't specify how many bits each channel takes,
// so we estimate it from the channel mask. We assume all bits in the mask
// are contiguous: i.e. channel masks like 00011100 are OK, but channel
// masks like 01000110 are not OK. (In other words, it's possible to get
// the channel value for UNORM using a single mask and multiply).
BitmaskMultiplier getMultiplierFromChannelMask(uint32_t mask, bool snorm)
{
  BitmaskMultiplier multiplier;
  if(mask == 0)
  {
    return multiplier;
  }

  // These values are the ones that make bitsToUnorm and bitsToSnorm come out correctly:
  multiplier.mask = mask;
  if(snorm)
  {
    const uint32_t hibit = highestSetBit(mask);
    // Shift so that hibit is in position 31:
    multiplier.leftshift = 31 - hibit;
    // Once you've done that, the multiplier should make the largest positive
    // value go to 1.
    const uint32_t largestPositiveValue = (mask << multiplier.leftshift) & 0x7FFFFFFF;
    if(largestPositiveValue == 0)
    {
      // This means that `mask` consisted of only 1 bit. 1-bit SNORM is
      // impossible; https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
      // requires that there's two representations for -1.0f,
      // one representation for 0.0f, and one representation for 1.0f.
      // Since this case is undefined (though the fuzzer finds a way to
      // reach it), we return a multiplier of 0.
      return multiplier;
    }
    multiplier.multiplier = 1.0F / static_cast<float>(largestPositiveValue);
    // This means that 0x7FFF... will go to 1, 0x80...1 will go to -1, and
    // 0x80...0 will go to a value slightly less than -1, which matches the
    // spec for SNORM.
    return multiplier;
  }
  else
  {
    // UNORM:
    multiplier.multiplier = 1.0F / static_cast<float>(mask);
    return multiplier;
  }
}

float bitsToUnorm(uint32_t value, const BitmaskMultiplier& mult)
{
  return static_cast<float>(value & mult.mask) * mult.multiplier;
}

float bitsToSnorm(uint32_t value, const BitmaskMultiplier& mult)
{
  // Shift the value so that its leftmost bit is at bit 31:
  uint32_t shifted = (value & mult.mask) << mult.leftshift;
  // Strictly speaking, static_cast<int32_t>(shifted) is implementation-defined.
  // So until we have bit_cast<int32_t>, use a memcpy and hope the compiler
  // determines that this is a reinterpretation of the underlying data (i.e. free):
  int32_t asint{};
  memcpy(&asint, &shifted, sizeof(uint32_t));
  const float v = static_cast<float>(asint) * mult.multiplier;
  return std::max(-1.0F, v);
}

// Macro for "read this variable from the istream; if it fails, return an error message"
#define READ_OR_ERROR(input, variable, error_message)                                                                  \
  if(!(input).read(reinterpret_cast<char*>(&(variable)), sizeof(variable)))                                            \
  {                                                                                                                    \
    return (error_message);                                                                                            \
  }

// Macro for "write this variable to the ostream; if it fails, return an error message"
#define WRITE_OR_ERROR(output, variable, error_message)                                                                \
  if(!(output).write(reinterpret_cast<const char*>(&(variable)), sizeof(variable)))                                    \
  {                                                                                                                    \
    return (error_message);                                                                                            \
  }

// Macro for "If this returned an error, propagate that error"
#define UNWRAP_ERROR(expr_returning_error_with_text)                                                                   \
  if(ErrorWithText unwrap_error_tmp = (expr_returning_error_with_text))                                                \
  {                                                                                                                    \
    return unwrap_error_tmp;                                                                                           \
  }

bool arrayIsAllPrintableChars(const char* arr, const size_t arrayLength)
{
  for(int i = 0; i < arrayLength; i++)
  {
    if(arr[i] < '!' || arr[i] > '~')
    {
      return false;
    }
  }
  return true;
}

// Returns a human-readable version of a Four-character code that can be
// printed to a terminal.
// In general, concatenating all four char values as-is is can produce issues:
// the FourCC code could contain a null character, which would cause issues
// mixing an std::string with C string functions. (For instance,
// FOURCC_BC70 does this). Or it could contain an ANSI terminal escape code.
std::string makeFourCCPrintable(const std::array<char, 4>& fourCC)
{
  if(arrayIsAllPrintableChars(fourCC.data(), fourCC.size()))
  {
    return std::string(fourCC.data(), fourCC.size());
  }

  std::string s = "(";
  for(int b = 0; b < 4; b++)
  {
    s += std::to_string(static_cast<uint8_t>(fourCC[b]));
    if(b != 3)
    {
      s += ", ";
    }
  }
  s += ")";
  return s;
}

std::string makeFourCCPrintable(uint32_t fourCC)
{
  std::array<char, 4> fourCCBytes{};
  for(int b = 0; b < 4; b++)
  {
    fourCCBytes[b] = static_cast<char>(fourCC >> (8 * b));
  }
  return makeFourCCPrintable(fourCCBytes);
}

// Suports an std::istream interface that operates on an in-memory array.
class MemoryStreamBuffer : public std::basic_streambuf<char>
{
  char*           m_data        = nullptr;
  std::streamoff  m_nextIndex   = 0;  // Always in [0, m_sizeInBytes].
  std::streamsize m_sizeInBytes = 0;

public:
  MemoryStreamBuffer(char* data, std::streamsize sizeInBytes)
      : m_data(data)
      , m_sizeInBytes(sizeInBytes)
  {
  }
  pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
  {
    const pos_type indicatesError = pos_type(static_cast<off_type>(-1));
    switch(dir)
    {
      case std::ios_base::beg:
        if(off < 0 || off > m_sizeInBytes)
          return indicatesError;
        m_nextIndex = off;
        break;
      case std::ios_base::cur:
        if(((off < 0) && m_nextIndex + off < 0) || (off >= 0 && off > m_sizeInBytes - m_nextIndex))
          return indicatesError;
        m_nextIndex += off;
        break;
      case std::ios_base::end:
        if(off > 0 || off < -m_sizeInBytes)
          return indicatesError;
        m_nextIndex = m_sizeInBytes + off;
        break;
      default:
        return indicatesError;
    }
    return m_nextIndex;
  }
  pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
  {
    return seekoff(static_cast<off_type>(pos), std::ios_base::beg, which);
  }
  // Gets the number of characters certainly available.
  std::streamsize showmanyc() override { return m_sizeInBytes - m_nextIndex; }
  // Gets the next character advancing the read pointer; EOF on error.
  int_type uflow() override
  {
    if(m_nextIndex < m_sizeInBytes)
    {
      return traits_type::to_int_type(m_data[m_nextIndex++]);
    }
    return traits_type::eof();
  }
  // Gets the next character without advancing the read pointer; EOF on error.
  int_type underflow() override
  {
    if(m_nextIndex < m_sizeInBytes)
    {
      return traits_type::to_int_type(m_data[m_nextIndex]);
    }
    return traits_type::eof();
  }
  // Reads a given number of characters and stores them into s' character array.
  // Returns the number of characters successfully read.
  std::streamsize xsgetn(char_type* s, std::streamsize count) override
  {
    if(count < 0 || s == nullptr || m_nextIndex >= m_sizeInBytes)
    {
      return 0;
    }
    const std::streamsize readableChars = std::min(count, m_sizeInBytes - m_nextIndex);
    memcpy(s, m_data + m_nextIndex, readableChars);
    m_nextIndex += readableChars;
    return readableChars;
  }
};

// An std::istream interface for a constant, in-memory array.
class MemoryStream : public std::istream
{
public:
  MemoryStream(const char* data, std::streamsize sizeInBytes)
      : m_buffer(const_cast<char*>(data), sizeInBytes)
      , std::istream(&m_buffer)
  {
    rdbuf(&m_buffer);
  }

private:
  MemoryStreamBuffer m_buffer;
};

// Computes the size of a subresource of size `width` x `height` x `depth`, encoded
// using ASTC blocks of size `blockWidth` x `blockHeight` x `blockDepth`. Returns false
// if the calculation would overflow, and returns true and stores the result in
// `out` otherwise.
bool astcSize(size_t blockWidth, size_t blockHeight, size_t blockDepth, size_t width, size_t height, size_t depth, size_t& out)
{
  return checked_math::mul4(((width + blockWidth - 1) / blockWidth),     // # of ASTC blocks along the x axis
                            ((height + blockHeight - 1) / blockHeight),  // # of ASTC blocks along the y axis
                            ((depth + blockDepth - 1) / blockDepth),     // # of ASTC blocks along the z axis
                            16,                                          // Each ASTC block size is 128 bits = 16 bytes
                            out);
}

// Computes the size on disk of a width x height x depth texture with the given
// format.
bool dxgiExportSize(size_t width, size_t height, size_t depth, uint32_t format, size_t& outSize)
{
  // You might be asking: Why not use the texture_format conversion functions
  // and merge this with nv_ktx.h's vulkanExportSize?
  // The reason is that nv_dds can't rely on Vulkan being present, since it's
  // always compiled into nvpro_core, so we can't use dxgiToVulkan. At the
  // same time, Vulkan formats are close enough to a superset of DXGI formats,
  // but not vice versa - in particular, Vulkan includes VK_FORMAT_R8G8B8_*.
  using namespace texture_formats;
  switch(format)
  {
    case DXGI_FORMAT_R1_UNORM:
      // 1 bit per pixel, rounded up to a byte
      if(!checked_math::mul3(width, height, depth, outSize))
        return false;
      outSize = (outSize + 7) / 8;
      return true;
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
      return checked_math::mul4(width, height, depth, 8 / 8, outSize);  // 8 bits per pixel
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
      return checked_math::mul4(width, height, depth, 16 / 8, outSize);  // 16 bits per pixel
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return checked_math::mul4(width, height, depth, 32 / 8, outSize);  // 32 bits per pixel
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
      return checked_math::mul4(width, height, depth, 64 / 8, outSize);  // 64 bits per pixel
    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
      return checked_math::mul4(width, height, depth, 96 / 8, outSize);  // 96 bits per pixel
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
      return checked_math::mul4(width, height, depth, 128 / 8, outSize);  // 128 bits per pixel
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
      return checked_math::mul4((width + 3) / 4, (height + 3) / 4, depth, 8, outSize);  // 8 bytes per block
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
      return checked_math::mul4((width + 3) / 4, (height + 3) / 4, depth, 16, outSize);  // 16 bytes per block
    case DXGI_FORMAT_ASTC_4X4_TYPELESS:
    case DXGI_FORMAT_ASTC_4X4_UNORM:
    case DXGI_FORMAT_ASTC_4X4_UNORM_SRGB:
      return astcSize(4, 4, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_5X4_TYPELESS:
    case DXGI_FORMAT_ASTC_5X4_UNORM:
    case DXGI_FORMAT_ASTC_5X4_UNORM_SRGB:
      return astcSize(5, 4, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_5X5_TYPELESS:
    case DXGI_FORMAT_ASTC_5X5_UNORM:
    case DXGI_FORMAT_ASTC_5X5_UNORM_SRGB:
      return astcSize(5, 5, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_6X5_TYPELESS:
    case DXGI_FORMAT_ASTC_6X5_UNORM:
    case DXGI_FORMAT_ASTC_6X5_UNORM_SRGB:
      return astcSize(6, 5, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_6X6_TYPELESS:
    case DXGI_FORMAT_ASTC_6X6_UNORM:
    case DXGI_FORMAT_ASTC_6X6_UNORM_SRGB:
      return astcSize(6, 6, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_8X5_TYPELESS:
    case DXGI_FORMAT_ASTC_8X5_UNORM:
    case DXGI_FORMAT_ASTC_8X5_UNORM_SRGB:
      return astcSize(8, 5, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_8X6_TYPELESS:
    case DXGI_FORMAT_ASTC_8X6_UNORM:
    case DXGI_FORMAT_ASTC_8X6_UNORM_SRGB:
      return astcSize(8, 6, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_8X8_TYPELESS:
    case DXGI_FORMAT_ASTC_8X8_UNORM:
    case DXGI_FORMAT_ASTC_8X8_UNORM_SRGB:
      return astcSize(8, 8, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_10X5_TYPELESS:
    case DXGI_FORMAT_ASTC_10X5_UNORM:
    case DXGI_FORMAT_ASTC_10X5_UNORM_SRGB:
      return astcSize(10, 5, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_10X6_TYPELESS:
    case DXGI_FORMAT_ASTC_10X6_UNORM:
    case DXGI_FORMAT_ASTC_10X6_UNORM_SRGB:
      return astcSize(10, 6, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_10X8_TYPELESS:
    case DXGI_FORMAT_ASTC_10X8_UNORM:
    case DXGI_FORMAT_ASTC_10X8_UNORM_SRGB:
      return astcSize(10, 8, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_10X10_TYPELESS:
    case DXGI_FORMAT_ASTC_10X10_UNORM:
    case DXGI_FORMAT_ASTC_10X10_UNORM_SRGB:
      return astcSize(10, 10, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_12X10_TYPELESS:
    case DXGI_FORMAT_ASTC_12X10_UNORM:
    case DXGI_FORMAT_ASTC_12X10_UNORM_SRGB:
      return astcSize(12, 10, 1, width, height, depth, outSize);
    case DXGI_FORMAT_ASTC_12X12_TYPELESS:
    case DXGI_FORMAT_ASTC_12X12_UNORM:
    case DXGI_FORMAT_ASTC_12X12_UNORM_SRGB:
      return astcSize(12, 12, 1, width, height, depth, outSize);
      // TODO
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_YUY2:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
    case DXGI_FORMAT_NV11:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_P208:
    case DXGI_FORMAT_V208:
    case DXGI_FORMAT_V408:
    default:
      return false;
  }
}

bool isDXGIFormatCompressed(const uint32_t dxgiFormat)
{
  return (dxgiFormat >= DXGI_FORMAT_BC1_TYPELESS && dxgiFormat <= DXGI_FORMAT_BC5_SNORM)
         || (dxgiFormat >= DXGI_FORMAT_BC6H_TYPELESS && dxgiFormat <= DXGI_FORMAT_BC7_UNORM_SRGB)
         || (dxgiFormat >= texture_formats::DXGI_FORMAT_ASTC_4X4_TYPELESS
             && dxgiFormat <= texture_formats::DXGI_FORMAT_ASTC_12X12_UNORM_SRGB);
}

bool dx9HeaderSupported(const uint32_t dxgiFormat)
{
  switch(dxgiFormat)
  {
      // We strictly only support DX9 headers for formats we're sure
      // readers can read - uncompressed formats, plus BC1-BC3, without sRGB
      // format conversion - and where the writer knows how to write the header.
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
      return true;
    default:
      return false;
  }
}

// For a format that can be written as a DX9 header, writes the pixel format
// settings into pf. Will assert failure in debug mode if the format is not
// supported, and write nothing except for pf.dwSize if so.
// isLuminance should be set in dwFlags separately.
// Writes color transform only for BC3n and OrthographicNormal.
void setDX9PixelFormat(const uint32_t format, ColorTransform colorTransform, const WriteSettings& writeSettings, DDSPixelFormat& pf)
{
  pf.dwSize = sizeof(DDSPixelFormat);
  static_assert((sizeof(DDSPixelFormat)) == 32, "DDS spec states that DDS_PIXELFORMAT size must be 32!");

  if(writeSettings.useCustomBitmask)
  {
    pf.dwRBitMask = writeSettings.bitmaskR;
    pf.dwGBitMask = writeSettings.bitmaskG;
    pf.dwBBitMask = writeSettings.bitmaskB;
    pf.dwABitMask = writeSettings.bitmaskA;

    const uint32_t rgbCombined = pf.dwRBitMask | pf.dwGBitMask | pf.dwBBitMask;
    if(rgbCombined != 0)
    {
      pf.dwFlags |= DDPF_RGB;
    }

    if(pf.dwABitMask != 0)
    {
      pf.dwFlags |= DDPF_ALPHAPIXELS;
    }

    pf.dwRGBBitCount = maskBitWidth(rgbCombined | pf.dwABitMask);
  }
  else
  {
    // Note that the DDS pixelformat starts at 0x4C and ends at 0x6B.
    switch(format)
    {
      case DXGI_FORMAT_BC3_TYPELESS:
      case DXGI_FORMAT_BC3_UNORM:
        if(colorTransform == ColorTransform::eAGBR)
        {
          pf.dwFlags  = DDPF_FOURCC;
          pf.dwFourCC = FOURCC_RXGB;
        }
        else
        {
          pf.dwFlags  = DDPF_FOURCC;
          pf.dwFourCC = FOURCC_DXT5;
        }
        break;
      case DXGI_FORMAT_BC2_TYPELESS:
      case DXGI_FORMAT_BC2_UNORM:
        pf.dwFlags  = DDPF_FOURCC;
        pf.dwFourCC = FOURCC_DXT3;
        break;
      case DXGI_FORMAT_BC1_TYPELESS:
      case DXGI_FORMAT_BC1_UNORM:
        pf.dwFlags  = DDPF_FOURCC;
        pf.dwFourCC = FOURCC_DXT1;
        break;
      case DXGI_FORMAT_A8_UNORM:
        pf.dwFlags       = DDPF_ALPHA;
        pf.dwRGBBitCount = 8;
        pf.dwRBitMask    = 0;
        pf.dwGBitMask    = 0;
        pf.dwBBitMask    = 0;
        pf.dwABitMask    = 0xFF;
        break;
      case DXGI_FORMAT_R8_UNORM:
        pf.dwFlags       = DDPF_RGB;
        pf.dwRGBBitCount = 8;
        pf.dwRBitMask    = 0xFF;
        pf.dwGBitMask    = 0;
        pf.dwBBitMask    = 0;
        pf.dwABitMask    = 0;
        break;
      case DXGI_FORMAT_R8G8_UNORM:
        if(colorTransform == ColorTransform::eOrthographicNormal)
        {
          pf.dwFlags = DDPF_FOURCC;
          pf.dwFlags = D3DFMT_CxV8U8;
        }
        else
        {
          pf.dwFlags       = DDPF_RGB;
          pf.dwRGBBitCount = 16;
          pf.dwRBitMask    = 0x00FF;
          pf.dwGBitMask    = 0xFF00;
          pf.dwBBitMask    = 0;
          pf.dwABitMask    = 0;
        }
        break;
      case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      case DXGI_FORMAT_B8G8R8A8_UNORM:
        pf.dwFlags       = DDPF_RGBA;
        pf.dwRGBBitCount = 32;
        pf.dwRBitMask    = 0x00FF0000U;
        pf.dwGBitMask    = 0x0000FF00U;
        pf.dwBBitMask    = 0x000000FFU;
        pf.dwABitMask    = 0xFF000000U;
        break;
      case DXGI_FORMAT_B8G8R8X8_TYPELESS:
      case DXGI_FORMAT_B8G8R8X8_UNORM:
        pf.dwFlags       = DDPF_RGB;
        pf.dwRGBBitCount = 32;
        pf.dwRBitMask    = 0x00FF0000U;
        pf.dwGBitMask    = 0x0000FF00U;
        pf.dwBBitMask    = 0x000000FFU;
        pf.dwABitMask    = 0;
        break;
      case DXGI_FORMAT_R16_FLOAT:
        pf.dwFlags  = DDPF_FOURCC;
        pf.dwFourCC = D3DFMT_R16F;
        break;
      case DXGI_FORMAT_R16G16_FLOAT:
        pf.dwFlags  = DDPF_FOURCC;
        pf.dwFourCC = D3DFMT_G16R16F;
        break;
      case DXGI_FORMAT_R16G16B16A16_FLOAT:
        pf.dwFlags  = DDPF_FOURCC;
        pf.dwFourCC = D3DFMT_A16B16G16R16F;
        break;
      case DXGI_FORMAT_R32_FLOAT:
        pf.dwFlags  = DDPF_FOURCC;
        pf.dwFourCC = D3DFMT_R32F;
        break;
      case DXGI_FORMAT_R32G32_FLOAT:
        pf.dwFlags  = DDPF_FOURCC;
        pf.dwFourCC = D3DFMT_G32R32F;
        break;
      case DXGI_FORMAT_R32G32B32A32_FLOAT:
        pf.dwFlags  = DDPF_FOURCC;
        pf.dwFourCC = D3DFMT_A32B32G32R32F;
        break;
      default:
        assert(!"SetDX9PixelFormat was called for an unsupported format! "
            "Please make sure that DX9HeaderSupported returns true for this "
            "format and that SetDX9PixelFormat is implemented for this format.");
    }
  }
}

void parse3ByteLibraryVersion(const uint32_t version, uint16_t& v0, uint8_t& v1, uint8_t& v2)
{
  v0 = static_cast<uint16_t>(version >> 16);
  v1 = static_cast<uint8_t>(version >> 8);
  v2 = static_cast<uint8_t>(version);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// Public functions
///////////////////////////////////////////////////////////////////////////////

const char* getColorTransformString(ColorTransform colorTransform)
{
  switch(colorTransform)
  {
    case ColorTransform::eNone:
      return "None";
    case ColorTransform::eLuminance:
      return "Luminance";
    case ColorTransform::eAGBR:
      return "AGBR (aka RXGB)";
    case ColorTransform::eYUV:
      return "YUV";
    case ColorTransform::eYCoCg:
      return "YCoCg";
    case ColorTransform::eYCoCgScaled:
      return "YCoCg Scaled";
    case ColorTransform::eAEXP:
      return "AEXP";
    case ColorTransform::eSwapRG:
      return "SwapRG";
    case ColorTransform::eOrthographicNormal:
      return "OrthographicNormal";
    default:
      return "?";
  }
}

const char* getAlphaModeString(uint32_t alphaMode)
{
  switch(alphaMode)
  {
    case DDS_ALPHA_MODE_UNKNOWN:
      return "DDS_ALPHA_MODE_UNKNOWN";
    case DDS_ALPHA_MODE_STRAIGHT:
      return "DDS_ALPHA_MODE_STRAIGHT";
    case DDS_ALPHA_MODE_PREMULTIPLIED:
      return "DDS_ALPHA_MODE_PREMULTIPLIED";
    case DDS_ALPHA_MODE_OPAQUE:
      return "DDS_ALPHA_MODE_OPAQUE";
    case DDS_ALPHA_MODE_CUSTOM:
      return "DDS_ALPHA_MODE_CUSTOM";
    default:
      return "?";
  }
}

const char* getWriterLibraryString(WriterLibrary writerLibrary)
{
  switch(writerLibrary)
  {
    case WriterLibrary::eNVTT:
      return "NVIDIA Texture Tools";
    case WriterLibrary::eNVTTExporter:
      return "NVIDIA Texture Tools Exporter";
    case WriterLibrary::eNVPS:
      return "NVIDIA DesignWorks Samples DDS Library";
    case WriterLibrary::eGIMP:
      return "GNU Image Manipulation Program";
    case WriterLibrary::eUnknown:
    default:
      return "Unknown";
  }
}

///////////////////////////////////////////////////////////////////////////////
// Subresource implementation
///////////////////////////////////////////////////////////////////////////////

ErrorWithText Subresource::create(size_t imageSizeBytes, const void* pixels)
{
  if(imageSizeBytes == 0)
    return "imageSizeBytes must be nonzero.";

  try
  {
    if(pixels != nullptr)
    {
      const char* pixelsBytes = reinterpret_cast<const char*>(pixels);
      data.assign(pixelsBytes, pixelsBytes + imageSizeBytes);
    }
    else
    {
      data.resize(imageSizeBytes, 0);
    }
  }
  catch(...)
  {
    return "Allocating " + std::to_string(imageSizeBytes) + " bytes of data failed.";
  }
  return {};
}

void Subresource::clear()
{
  *this = Subresource();
}

// Double-check some properties of Subresource.
static_assert(std::is_move_assignable_v<Subresource>);
static_assert(std::is_move_constructible_v<Subresource>);
static_assert(std::is_copy_assignable_v<Subresource>);
static_assert(std::is_copy_constructible_v<Subresource>);

///////////////////////////////////////////////////////////////////////////////
// Image implementation
///////////////////////////////////////////////////////////////////////////////

ErrorWithText Image::allocate(uint32_t _numMips, uint32_t _numLayers, uint32_t _numFaces)
{
  if(_numMips == 0)
    return "_numMips must be nonzero.";
  if(_numMips >= 32)
    return "_numMips must be less than 32.";
  if(_numLayers == 0)
    return "_numLayers must be nonzero.";
  if(_numFaces == 0)
    return "_numFaces must be nonzero.";

  m_numMips   = _numMips;
  m_numLayers = _numLayers;
  m_numFaces  = _numFaces;

  size_t totalSubresources = 0;
  if(!checked_math::mul3(m_numMips, m_numLayers, m_numFaces, totalSubresources))
  {
    return "The total number of subresources was too large to fit in a size_t.";
  }

  return resizeVectorOrError(m_data, totalSubresources);
}

void Image::clear()
{
  m_data.clear();
}

ResourceDimension Image::inferResourceDimension() const
{
  if(ResourceDimension::eUnknown != resourceDimension)
  {
    return resourceDimension;  // We know what it is!
  }
  // Otherwise, try to guess it from the dimensions of the base mip:
  if(!m_data.empty())
  {
    return ResourceDimension::eUnknown;  // We don't know the dimensions.
  }
  if(mip0Depth > 1)
  {
    return ResourceDimension::eTexture3D;
  }
  if(mip0Height > 1)
  {
    return ResourceDimension::eTexture2D;
  }
  return ResourceDimension::eTexture1D;
}

const Subresource& Image::subresource(uint32_t mip, uint32_t layer, uint32_t face) const
{
  if(mip >= m_numMips || layer >= m_numLayers || face >= m_numFaces)
  {
    throw std::out_of_range("subresource() values were out of range");
  }

  // Here's the [mip, layer, face] layout for subresources we use.
  return m_data[(static_cast<size_t>(mip) * static_cast<size_t>(m_numLayers) + static_cast<size_t>(layer)) * static_cast<size_t>(m_numFaces)
                + static_cast<size_t>(face)];
}

Subresource& Image::subresource(uint32_t mip, uint32_t layer, uint32_t face)
{
  return const_cast<Subresource&>(const_cast<const Image*>(this)->subresource(mip, layer, face));
}

ErrorWithText Image::readHeaderFromStream(std::istream& input, const ReadSettings& readSettings)
{
  // Read the file marker to ensure we have a DDS file.
  uint32_t fileCode{};
  READ_OR_ERROR(input, fileCode, "Reached the end of the input while trying to read the first four characters of the input. Is the input truncated?");
  if(fileCode != FOURCC_DDS)
  {
    return "The DDS file's first four characters were incorrect (expected \"DDS \", but the first four characters were "
           + makeFourCCPrintable(fileCode) + ".";
  }

  // Create a short alias for m_fileInfo, and clear it.
  m_fileInfo  = FileInfo{};
  FileInfo& i = m_fileInfo;

  // Read the raw header.
  READ_OR_ERROR(input, i.ddsh, "Reached the end of the input while trying to read the core portion of the DDS header. Is the input truncated?");

  // Determine the number of faces in the image.
  // Some DDS files don't have DDSCAPS2_CUBEMAP specified, but still have a
  // nonzero number of cubemap faces. So don't check for the
  // DDSCAPS2_CUBEMAP flag.
  cubemapFaceFlags = (i.ddsh.dwCaps2 & DDSCAPS2_CUBEMAP_ALL_FACES);

  // Get the number of mips in the image.
  // There's at least one DDS file that doesn't have DDSD_MIPMAPCOUNT set,
  // but still has mipmaps.
  // Also, some DDS files have a dwMipMapCount of 0, but actually have
  // at least one mip. (They disagree on whether 0 means 'a full mip chain' or
  // 'one mip, no mips outside the base layer'). We work around this by
  // requiring there to be at least one mip.
  m_numMips = std::max(1U, i.ddsh.dwMipMapCount);
  if(m_numMips >= 32)
  {
    return "The number of mips in the DDS file must be less than 32. Otherwise, the base mip would need to have a dimension of 2^32 or larger, which isn't possible";
  }

  // Determine which library wrote this file.
  if(i.ddsh.dwReserved1[9] == FOURCC_LIBRARY_EXPORTER)
  {
    i.writerLibrary        = WriterLibrary::eNVTTExporter;
    i.writerLibraryVersion = i.ddsh.dwReserved1[10];
  }
  else if(i.ddsh.dwReserved1[9] == FOURCC_LIBRARY_NVTT)
  {
    i.writerLibrary        = WriterLibrary::eNVTT;
    i.writerLibraryVersion = i.ddsh.dwReserved1[10];
  }
  else if(i.ddsh.dwReserved1[9] == FOURCC_LIBRARY_NVPS)
  {
    i.writerLibrary        = WriterLibrary::eNVPS;
    i.writerLibraryVersion = i.ddsh.dwReserved1[10];
  }
  else if(i.ddsh.dwReserved1[0] == FOURCC_LIBRARY_GIMP_WORD0 && i.ddsh.dwReserved1[1] == FOURCC_LIBRARY_GIMP_WORD1)
  {
    i.writerLibrary        = WriterLibrary::eGIMP;
    i.writerLibraryVersion = i.ddsh.dwReserved1[2];
  }

  // GIMP will also sometimes store custom format flags in dwReserved1[3].
  // Detect these, assuming that no other writers use dwReserved1[3] for a
  // different purpose and can write these values exactly:
  switch(i.ddsh.dwReserved1[3])
  {
    case FOURCC_AEXP:
      colorTransform = ColorTransform::eAEXP;
      break;
    case FOURCC_YCOCG:
      colorTransform = ColorTransform::eYCoCg;
      break;
    case FOURCC_YCOCG_SCALED:
      colorTransform = ColorTransform::eYCoCgScaled;
      break;
  }

  if(i.ddsh.dwReserved1[7] == FOURCC_UVER)
  {
    hasUserVersion = true;
    userVersion    = i.ddsh.dwReserved1[8];
  }

  // Handle DPPF_ALPHAPREMULT, in case it's there for compatibility.
  if((i.ddsh.ddspf.dwFlags & DDPF_ALPHAPREMULT) != 0)
  {
    alphaMode = DDS_ALPHA_MODE_PREMULTIPLIED;
  }

  // We won't know the number of layers in the image until we look at the DX10
  // header, if it exists. So let's examine that FourCC code now!
  const bool hasFourCC = ((i.ddsh.ddspf.dwFlags & DDPF_FOURCC) != 0);

  if(hasFourCC && (i.ddsh.ddspf.dwFourCC == FOURCC_DX10))
  {
    i.hadDx10Extension = true;
    // Read the DX10 header.
    READ_OR_ERROR(input, i.ddsh10, "DDS file header specifies a DX10 header, but the DDS reader reached the end of the input when trying to read it; is the input truncated?");

    dxgiFormat = i.ddsh10.dxgiFormat;

    // If DDS10_RESOURCE_MISC_TEXTURECUBE is set, then we have a full cubemap,
    // despite what dwCaps2 said.
    if((i.ddsh10.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE) != 0)
    {
      cubemapFaceFlags = DDSCAPS2_CUBEMAP_ALL_FACES;
    }

    m_numLayers = i.ddsh10.arraySize;

    // Look at lower 3 bits of miscFlags2 to determine alpha mode
    alphaMode = i.ddsh10.miscFlags2 & 0x7;
  }
  else
  {
    // No DX10 header.
    if(hasFourCC)
    {
      // Detect the swizzle code, which is stored in dwRGBBitCount.
      // There's no standard for this, so we just check for what old versions
      // of NVTT could output.
      switch(i.ddsh.ddspf.dwRGBBitCount)
      {
        case FOURCC_A2XY:
          colorTransform = ColorTransform::eSwapRG;
          break;
        case FOURCC_A2D5:
          colorTransform = ColorTransform::eAGBR;
          break;
      }

      // Detect the format based on the format code:
      switch(i.ddsh.ddspf.dwFourCC)
      {
        case FOURCC_DXT1:
          dxgiFormat = DXGI_FORMAT_BC1_UNORM;
          break;
        case FOURCC_DXT2:
        case FOURCC_DXT3:
          dxgiFormat = DXGI_FORMAT_BC2_UNORM;
          break;
        case FOURCC_DXT4:
        case FOURCC_DXT5:
          dxgiFormat = DXGI_FORMAT_BC3_UNORM;
          break;
          // Less standard FOURCC codes
        case FOURCC_BC4U:
        case FOURCC_ATI1:
          dxgiFormat = DXGI_FORMAT_BC4_UNORM;
          break;
        case FOURCC_BC4S:
          dxgiFormat = DXGI_FORMAT_BC4_SNORM;
          break;
        case FOURCC_BC5U:
          dxgiFormat = DXGI_FORMAT_BC5_UNORM;
          break;
        case FOURCC_ATI2:
          // ATI2 is BC5 but with the red and green channels swapped.
          // So we remove ColorTransform::eSwapRG if we have it, or add it if
          // it's missing.
          // Throw an error if the color transform is something else.
          if(colorTransform == ColorTransform::eNone)
          {
            colorTransform = ColorTransform::eSwapRG;
          }
          else if(colorTransform == ColorTransform::eSwapRG)
          {
            colorTransform = ColorTransform::eNone;
          }
          else
          {
            return "This file specified both ColorTransform " + std::to_string(static_cast<uint32_t>(colorTransform))
                   + " and a format of ATI2 (which swaps the red and green channels). nv_dds doesn't know how to combine the RG swap with the ColorTransform to get a single color transform.";
          }
          dxgiFormat = DXGI_FORMAT_BC5_UNORM;
          break;
        case FOURCC_BC5S:
          dxgiFormat = DXGI_FORMAT_BC5_SNORM;
          break;
        case FOURCC_BC6H:
          // That's right - this is UF16, not SF16.
          // These FourCC codes are highly nonstandard.
          dxgiFormat = DXGI_FORMAT_BC6H_UF16;
          break;
        case FOURCC_BC7L:
        case FOURCC_BC70:
        case FOURCC_ZOLA:
          // That's right - both 'BC7L' and 'BC7\0'
          // are BC7. It's unknown which tools
          // produce this
          // See https://github.com/microsoft/DirectXTex/pull/165.
          // Old versions of NVTT can produce 'ZOLA'.
          dxgiFormat = DXGI_FORMAT_BC7_UNORM;
          break;
        // <case FOURCC_CTX1>: There's no corresponding bitmasks or DXGI format for CTX1;
        // apps have to detect and decompress this themselves.
        case FOURCC_RGBG:
          dxgiFormat = DXGI_FORMAT_R8G8_B8G8_UNORM;
          break;
        case FOURCC_GRGB:
          dxgiFormat = DXGI_FORMAT_G8R8_G8B8_UNORM;
          break;
        case FOURCC_YUY2:
          dxgiFormat = DXGI_FORMAT_YUY2;
          break;
        case FOURCC_UYVY:
          dxgiFormat = DXGI_FORMAT_R8G8_B8G8_UNORM;
          break;
        case FOURCC_RXGB:
          dxgiFormat     = DXGI_FORMAT_BC3_UNORM;
          colorTransform = ColorTransform::eAGBR;
          break;
          // GLI and DirectXTex will write some DXGI formats without a DX10
          // header and using Direct3D format numbers by default, so we have
          // to account for that here.
          // Note that most of these are untested - R and B swaps are likely!
        case D3DFMT_R8G8B8:
          dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
          break;
        case D3DFMT_A8R8G8B8:
          dxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM;  // I think?
          break;
        case D3DFMT_X8R8G8B8:
          dxgiFormat = DXGI_FORMAT_B8G8R8X8_UNORM;
          break;
        case D3DFMT_R5G6B5:
          dxgiFormat = DXGI_FORMAT_B5G6R5_UNORM;
          break;
        case D3DFMT_X1R5G5B5:  // For now
        case D3DFMT_A1R5G5B5:
          dxgiFormat = DXGI_FORMAT_B5G5R5A1_UNORM;
          break;
        case D3DFMT_A4R4G4B4:
          dxgiFormat = DXGI_FORMAT_B4G4R4A4_UNORM;
          break;
        case D3DFMT_R3G3B2:
          // NOTE: We overwrite the header itself here to propagate information
          // to the bitmasking code!
          dxgiFormat                 = DXGI_FORMAT_UNKNOWN;
          i.ddsh.ddspf.dwRGBBitCount = 8;
          i.ddsh.ddspf.dwABitMask    = 0;
          i.ddsh.ddspf.dwRBitMask    = 0b11100000;
          i.ddsh.ddspf.dwGBitMask    = 0b00011100;
          i.ddsh.ddspf.dwBBitMask    = 0b00000011;
          i.bitmaskHasAlpha          = true;
          i.bitmaskHasRgb            = true;
          i.wasBitmasked             = true;
          break;
        case D3DFMT_A8:
          dxgiFormat = DXGI_FORMAT_A8_UNORM;
          break;
        case D3DFMT_A8R3G3B2:
          dxgiFormat                 = DXGI_FORMAT_UNKNOWN;
          i.ddsh.ddspf.dwRGBBitCount = 16;
          i.ddsh.ddspf.dwABitMask    = 0xff00;
          i.ddsh.ddspf.dwRBitMask    = 0b11100000;
          i.ddsh.ddspf.dwGBitMask    = 0b00011100;
          i.ddsh.ddspf.dwBBitMask    = 0b00000011;
          i.bitmaskHasAlpha          = true;
          i.bitmaskHasRgb            = true;
          i.wasBitmasked             = true;
          break;
        case D3DFMT_X4R4G4B4:
          dxgiFormat                 = DXGI_FORMAT_UNKNOWN;
          i.ddsh.ddspf.dwRGBBitCount = 16;
          i.ddsh.ddspf.dwABitMask    = 0x0000;
          i.ddsh.ddspf.dwRBitMask    = 0x0f00;
          i.ddsh.ddspf.dwGBitMask    = 0x00f0;
          i.ddsh.ddspf.dwBBitMask    = 0x000f;
          i.bitmaskHasRgb            = true;
          i.wasBitmasked             = true;
          break;
        case D3DFMT_A2B10G10R10:
          dxgiFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
          break;
        case D3DFMT_A8B8G8R8:
        case D3DFMT_X8B8G8R8:
          dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
          break;
        case D3DFMT_G16R16:
          dxgiFormat = DXGI_FORMAT_R16G16_UNORM;
          break;
        case D3DFMT_A2R10G10B10:
          dxgiFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
          break;
        case D3DFMT_A16B16G16R16:
          dxgiFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
          break;
        // TODO:
        // case D3DFMT_A8P8:
        // case D3DFMT_P8:
        case D3DFMT_L8:
          dxgiFormat                 = DXGI_FORMAT_UNKNOWN;
          i.ddsh.ddspf.dwRGBBitCount = 8;
          i.ddsh.ddspf.dwRBitMask    = 0xff;
          i.ddsh.ddspf.dwGBitMask    = 0xff;
          i.ddsh.ddspf.dwBBitMask    = 0xff;
          i.bitmaskHasRgb            = true;
          i.wasBitmasked             = true;
          colorTransform             = ColorTransform::eLuminance;
          break;
        case D3DFMT_A8L8:
          dxgiFormat                 = DXGI_FORMAT_UNKNOWN;
          i.ddsh.ddspf.dwRGBBitCount = 16;
          i.ddsh.ddspf.dwABitMask    = 0xff00;
          i.ddsh.ddspf.dwRBitMask    = 0x00ff;
          i.ddsh.ddspf.dwGBitMask    = 0x00ff;
          i.ddsh.ddspf.dwBBitMask    = 0x00ff;
          i.bitmaskHasAlpha          = true;
          i.bitmaskHasRgb            = true;
          i.wasBitmasked             = true;
          colorTransform             = ColorTransform::eLuminance;
          break;
        case D3DFMT_A4L4:
          dxgiFormat                 = DXGI_FORMAT_UNKNOWN;
          i.ddsh.ddspf.dwRGBBitCount = 8;
          i.ddsh.ddspf.dwABitMask    = 0xf0;
          i.ddsh.ddspf.dwRBitMask    = 0x0f;
          i.ddsh.ddspf.dwGBitMask    = 0x0f;
          i.ddsh.ddspf.dwBBitMask    = 0x0f;
          i.bitmaskHasAlpha          = true;
          i.bitmaskHasRgb            = true;
          i.wasBitmasked             = true;
          colorTransform             = ColorTransform::eLuminance;
          break;
        case D3DFMT_V8U8:
          dxgiFormat = DXGI_FORMAT_R8G8_SNORM;
          break;
          // TODO:
        // case D3DFMT_L6V5U5:
        // case D3DFMT_X8L8V8U8:
        case D3DFMT_Q8W8V8U8:
          dxgiFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
          break;
        case D3DFMT_V16U16:
          dxgiFormat = DXGI_FORMAT_R16G16_SNORM;
          break;
        case D3DFMT_A2W10V10U10:
          dxgiFormat = DXGI_FORMAT_R10G10B10A2_UINT;
          break;
        case D3DFMT_D16:
        case D3DFMT_D16_LOCKABLE:
          dxgiFormat = DXGI_FORMAT_D16_UNORM;
          break;
        case D3DFMT_D32:
        case D3DFMT_D32F_LOCKABLE:
          dxgiFormat = DXGI_FORMAT_D32_FLOAT;
          break;
        case D3DFMT_D24S8:
        case D3DFMT_D24X8:
        case D3DFMT_D24X4S4:
          dxgiFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
          break;
        // TODO: case D3DFMT_D15S1:
        // TODO: case D3DFMT_D32:
        // TODO: case D3DFMT_D32_LOCKABLE:
        // TODO: case D3DFMT_D24FS8:
        case D3DFMT_S8_LOCKABLE:
          dxgiFormat = DXGI_FORMAT_R8_UINT;
          break;
          // TODO: D3DFMT_VERTEXDATA, D3DFMT_INDEX16, D3DFMT_INDEX32
        case D3DFMT_L16:
          dxgiFormat     = DXGI_FORMAT_R16_UNORM;
          colorTransform = ColorTransform::eLuminance;
          break;
        case D3DFMT_Q16W16V16U16:
          dxgiFormat = DXGI_FORMAT_R16G16B16A16_SNORM;
          break;
        case D3DFMT_R16F:
          dxgiFormat = DXGI_FORMAT_R16_FLOAT;
          break;
        case D3DFMT_G16R16F:
          dxgiFormat = DXGI_FORMAT_R16G16_FLOAT;
          break;
        case D3DFMT_A16B16G16R16F:
          dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
          break;
        case D3DFMT_R32F:
          dxgiFormat = DXGI_FORMAT_R32_FLOAT;
          break;
        case D3DFMT_G32R32F:
          dxgiFormat = DXGI_FORMAT_R32G32_FLOAT;
          break;
        case D3DFMT_A32B32G32R32F:
          dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
          break;
        case D3DFMT_CxV8U8:
          colorTransform = ColorTransform::eOrthographicNormal;
          dxgiFormat     = DXGI_FORMAT_R8G8_SNORM;
          break;
          // TODO: D3DFMT_A1, D3DFMT_BINARYBUFFER
        case D3DFMT_A2B10G10R10_XR_BIAS:
          dxgiFormat = DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
          break;
        default: {
          // Undocumented or unsupported FourCC code.
          return "DDS file had FourCC code " + makeFourCCPrintable(i.ddsh.ddspf.dwFourCC) + ", which the DDS reader does not support.";
        }
      }
    }
    else
    {
      // Otherwise, the DDS_PIXELFORMAT structure gives us all the data
      // we need to read from an sRGB image.
      // DDPF_ALPHAPIXELS: Texture contains alpha data, dwRGBAlphaBitMask is valid.
      // DDPF_ALPHA: dwRGBBitCount = alpha channel bitcount, dwABitMask contains valid data.
      // DDPF_RGB: RGB channels exist; dwRGBBitCount and RGB masks contain valid data.
      // DDPF_YUV: Interpret RGB as YUV data
      // DDPF_LUMINANCE: Grayscale; dwRGBBitCount contains bitcount and dwRBitMask contains luminance channel mask.
      // DDPF_BUMPDUDV: All channels contain SNORM instead of UNORM data.
      if((i.ddsh.ddspf.dwFlags & DDPF_BUMPDUDV) != 0)
      {
        i.bitmaskWasBumpDuDv = true;
        i.bitmaskHasRgb      = true;
      }
      i.bitmaskHasAlpha = ((i.ddsh.ddspf.dwFlags & (DDPF_ALPHA | DDPF_ALPHAPIXELS)) != 0);
      i.bitmaskHasRgb |= ((i.ddsh.ddspf.dwFlags & (DDPF_YUV | DDPF_LUMINANCE | DDPF_RGB)) != 0);
      i.wasBitmasked = true;
    }

    // Read additional color transform info from dwFlags whether we're in DX9
    // or DX10 mode.
    if((i.ddsh.ddspf.dwFlags & DDPF_YUV) != 0)
    {
      colorTransform = ColorTransform::eYUV;
    }
    if((i.ddsh.ddspf.dwFlags & DDPF_LUMINANCE) != 0)
    {
      colorTransform = ColorTransform::eLuminance;
    }
  }

  // Were any of the cubemap fields set?
  const bool isCubemap = (cubemapFaceFlags != 0);
  // Count the number of faces; this might be a value other than 1 or 6 if we
  // have an incomplete cubemap.
  m_numFaces = popcnt(cubemapFaceFlags);
  // If no cubemap face flags were specified, then this is a 2D texture, and so
  // has 1 face.
  m_numFaces = std::max(m_numFaces, 1U);

  // Recognize and fix an issue in cubemaps created by the Texture Tools
  // Exporter that was fixed in 2023.1.1.
  // The Microsoft DDS spec and official DDS reader say arraySize is the number
  // of elements. But while the spec's cube map example sets arraySize to 6 for
  // a cube map, this is wrong, and RenderDoc will show the extra (blank)
  // elements if the reader or writer do this.
  // See NVTTE-97 and https://forums.developer.nvidia.com/t/texture-tools-exporter-cubemap-has-incorrect-arraysize/244753
  if(isCubemap && i.writerLibrary == WriterLibrary::eNVTTExporter && i.writerLibraryVersion == LIBRARY_EXPORTER_VERSION_START_THROUGH_2023_1_0)
  {
    m_numLayers = std::max(1U, m_numLayers / 6);
  }

  // Get the size of the base mip.
  mip0Width  = std::max(1U, i.ddsh.dwWidth);
  mip0Height = std::max(1U, i.ddsh.dwHeight);
  mip0Depth  = std::max(1U, i.ddsh.dwDepth);

  // If we'll read from a bitmasked format, determine what DXGI format we'll
  // decompress to. The gold standard is to decompress everything to
  // RGBA32SFLOAT, but that uses a lot of memory. Decompressing to RGBA8UNORM
  // is OK (no information loss) as long as each component's bitmask is at
  // most 8 bits wide and positive.
  if(i.wasBitmasked)
  {
    if(readSettings.bitmaskForceRgbaF32 || i.bitmaskWasBumpDuDv)
    {
      dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    }
    else
    {
      dxgiFormat             = DXGI_FORMAT_R8G8B8A8_UNORM;
      const bool alphaTooBig = i.bitmaskHasAlpha && maskBitWidth(i.ddsh.ddspf.dwABitMask) > 8;
      const bool luminanceTooBig = (colorTransform == ColorTransform::eLuminance) && maskBitWidth(i.ddsh.ddspf.dwRBitMask) > 8;
      const bool rgbTooBig = i.bitmaskHasRgb
                             && (maskBitWidth(i.ddsh.ddspf.dwRBitMask) > 8 || maskBitWidth(i.ddsh.ddspf.dwGBitMask) > 8
                                 || maskBitWidth(i.ddsh.ddspf.dwBBitMask) > 8);
      if(alphaTooBig || luminanceTooBig || rgbTooBig)
      {
        dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
      }
    }
  }

  return {};
}

ErrorWithText Image::readHeaderFromFile(const char* filename, const ReadSettings& readSettings)
{
  try
  {
    std::ifstream file(filename, std::ios::binary | std::ios::in);
    return readHeaderFromStream(file, readSettings);
  }
  catch(const std::exception& e)
  {
    return "I/O error opening " + std::string(filename) + ": " + std::string(e.what());
  }
}

ErrorWithText Image::readHeaderFromMemory(const char* buffer, size_t bufferSize, const ReadSettings& readSettings)
{
  if(bufferSize > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()))
  {
    return "The `bufferSize` parameter was too large to be stored in an std::streamsize.";
  }
  MemoryStream stream(buffer, static_cast<std::streamsize>(bufferSize));
  return readHeaderFromStream(stream, readSettings);
}

ErrorWithText Image::readFromStream(std::istream& input, const ReadSettings& readSettings)
{
  UNWRAP_ERROR(readHeaderFromStream(input, readSettings));

  size_t validationInputSize = 0;
  if(readSettings.validateInputSize)
  {
    const std::streampos initialPos = input.tellg();
    input.seekg(0, std::ios_base::end);
    const std::streampos endPos = input.tellg();
    validationInputSize         = endPos - initialPos;
    input.seekg(initialPos, std::ios_base::beg);
  }

  // Create a short alias for m_fileInfo
  const FileInfo& i = m_fileInfo;

  // Crop the output to fewer mips if readSettings.mips was specified
  const uint32_t mipsInFile = m_numMips;
  if(!readSettings.mips)
  {
    m_numMips = 1;
  }

  // Allocate space
  {
    size_t totalSubresources = 0;
    if(!checked_math::mul3(m_numFaces, m_numMips, m_numLayers, totalSubresources)
       || totalSubresources > readSettings.maxSubresourceSizeBytes / sizeof(Subresource))
    {
      return "This DDS file is too large: it had " + std::to_string(m_numFaces) + " faces, " + std::to_string(m_numMips)
             + " requested mips to read, and " + std::to_string(m_numLayers)
             + " elements. Their product, the number of subresources in the table of subresources, would require more than the reader's byte limit ("
             + std::to_string(readSettings.maxSubresourceSizeBytes) + " bytes) to store.";
    }
    if(readSettings.validateInputSize && (totalSubresources + 7) / 8 > validationInputSize)
    {
      return "This DDS file had an impossible header: it listed " + std::to_string(totalSubresources)
             + " subresources, but the input was only " + std::to_string(validationInputSize)
             + " bytes long. This would not be possible even if the input was an array of 1x1 DXGI_FORMAT_A1 textures.";
    }
  }
  UNWRAP_ERROR(allocate(m_numMips, m_numLayers, m_numFaces));

  // Precompute bitmasking weights, in case we use bitmasking.
  std::array<BitmaskMultiplier, 4> bitmaskMults;
  bitmaskMults[0] = getMultiplierFromChannelMask(i.ddsh.ddspf.dwRBitMask, i.bitmaskWasBumpDuDv);
  bitmaskMults[1] = getMultiplierFromChannelMask(i.ddsh.ddspf.dwGBitMask, i.bitmaskWasBumpDuDv);
  bitmaskMults[2] = getMultiplierFromChannelMask(i.ddsh.ddspf.dwBBitMask, i.bitmaskWasBumpDuDv);
  bitmaskMults[3] = getMultiplierFromChannelMask(i.ddsh.ddspf.dwABitMask, i.bitmaskWasBumpDuDv);

  // Iterate over images in the DDS file. Read those images we want to
  // read and skip over the rest.
  for(uint32_t layer = 0; layer < m_numLayers; layer++)
  {
    for(uint32_t face = 0; face < m_numFaces; face++)
    {
      for(uint32_t inputMip = 0; inputMip < mipsInFile; inputMip++)
      {
        // Compute the size of this image
        const uint32_t mipWidth  = getWidth(inputMip);
        const uint32_t mipHeight = getHeight(inputMip);
        const uint32_t mipDepth  = getDepth(inputMip);
        // Compute size from DDS header.
        // The DDS situation here is a bit of a mess. We prefer inferring this
        // directly from the format and size; it's hard for writers to get
        // that wrong. If there is no format, we use dwRGBBitCount here if it
        // is nonzero. If it's not there, use dwPitchOrLinearSize.
        size_t   fileTexSize           = 0;
        uint32_t bitmaskedBitsPerPixel = 0;
        if(!i.wasBitmasked && dxgiFormat != 0)
        {
          if(!dxgiExportSize(mipWidth, mipHeight, mipDepth, dxgiFormat, fileTexSize))
          {
            return "Could not determine the number of bytes used by a subresource with size " + std::to_string(mipWidth)
                   + " x " + std::to_string(mipHeight) + " x " + std::to_string(mipDepth) + " and DXGI format "
                   + std::to_string(dxgiFormat) + ".";
          }
        }
        else if(i.ddsh.ddspf.dwRGBBitCount != 0)
        {
          bitmaskedBitsPerPixel  = i.ddsh.ddspf.dwRGBBitCount;
          size_t fileTexSizeBits = 0;
          if(!checked_math::mul4(i.ddsh.ddspf.dwRGBBitCount, mipWidth, mipHeight, mipDepth, fileTexSizeBits)
             || fileTexSizeBits > std::numeric_limits<size_t>::max() - 7)
          {
            return "This file is probably not valid: mip " + std::to_string(inputMip) + " (" + std::to_string(mipWidth)
                   + " x " + std::to_string(mipHeight) + " x " + std::to_string(mipDepth) + ", dwRGBBitCount == "
                   + std::to_string(i.ddsh.ddspf.dwRGBBitCount) + ") had more bits than would fit in a size_t.";
          }
          fileTexSize = (fileTexSizeBits + 7) / 8;
        }
        else
        {
          // Since this branch is uncompressed, dwPitchOrLinearSize is
          // the number of bytes per scanline in the base mip.
          // Try to get the number of bits per pixel, assuming images are
          // densely packed.
          if((i.ddsh.dwPitchOrLinearSize % mip0Width) != 0)
          {
            return "This file is probably not valid: it didn't seem to contain DXGI format information, and its dwRGBBitCount was 0. In this situation, dwPitchOrLinearSize should be the number of bits in each scanline of mip 0 - but it wasn't evenly divisible by mip 0's width.";
          }
          bitmaskedBitsPerPixel = i.ddsh.dwPitchOrLinearSize / mip0Width;
          const uint32_t pitch  = bitmaskedBitsPerPixel * mipWidth;
          if(!checked_math::mul3(pitch, mipHeight, mipDepth, fileTexSize))
          {
            return "This file is probably not valid: mip " + std::to_string(inputMip) + " (" + std::to_string(mipWidth)
                   + " x " + std::to_string(mipHeight) + " x " + std::to_string(mipDepth)
                   + ", pitch == " + std::to_string(pitch) + " had more bytes than would fit in a size_t.";
          }
        }

        // Regardless of what we wind up with, a texture size of 0 is bad.
        // See https://github.com/nvpro-samples/nvtt_samples/issues/2
        // for how this can crash.
        if(fileTexSize == 0)
        {
          return "This file is probably not valid: mip " + std::to_string(inputMip) + " (" + std::to_string(mipWidth)
                 + " x " + std::to_string(mipHeight) + " x " + std::to_string(mipDepth)
                 + ") contained 0 bytes of data. Is a DDS format missing from the header of this file?";
        }
        // Also, make sure this isn't impossibly large.
        if(((fileTexSize / mipWidth) / mipHeight) / mipDepth > 16)
        {
          return "This file is probably not valid: mip " + std::to_string(inputMip) + " declared it contained "
                 + std::to_string(fileTexSize) + " bytes of data. However, that's larger than the number of bytes that a mip of size "
                 + std::to_string(mipWidth) + " x " + std::to_string(mipHeight) + " x " + std::to_string(mipDepth)
                 + " would contain using the largest DDS format, RGBA32F (which uses 16 bytes per pixel). Is a DDS format missing from the header of this file?";
        }
        // Or impermissibly large.
        if(fileTexSize > readSettings.maxSubresourceSizeBytes)
        {
          return "Mip " + std::to_string(inputMip) + " (" + std::to_string(mipWidth) + " x " + std::to_string(mipHeight)
                 + " x " + std::to_string(mipDepth) + ") had more bytes (" + std::to_string(fileTexSize)
                 + ") than the maximum allowed in the DDS reader's parameters ("
                 + std::to_string(readSettings.maxSubresourceSizeBytes) + ").";
        }
        // Additionally, if we're reading mip 0, double-check that the file can
        // reasonably contain at least mip 0's data.
        if(readSettings.validateInputSize && fileTexSize > validationInputSize / (static_cast<size_t>(m_numLayers) * m_numFaces))
        {
          return "This file is probably not valid: each mip 0 subresource should contain " + std::to_string(fileTexSize)
                 + " bytes of data, and there are " + std::to_string(m_numLayers) + " layers and " + std::to_string(m_numFaces)
                 + " faces, but the input is only " + std::to_string(validationInputSize) + " bytes long.";
        }

        const bool readData = (inputMip < m_numMips);
        if(!readData)
        {
          // Just go to the next image.
          if(!input.seekg(static_cast<std::streamoff>(fileTexSize), std::ios::cur))
          {
            return "Seeking to an image in a DDS input failed. Is the input truncated?";
          }
          continue;
        }

        Subresource& resource = subresource(inputMip, layer, face);
        if(i.wasBitmasked)
        {
          // Read old-style DDS format from bitmasks.
          // Note that we support bitcounts (i.e. differences in bits between
          // addresses of consecutive pixels) that are not divisible by 8!

          // Start by reading the raw data into a buffer. We always add 7 bytes
          // of padding so we don't need to perform bounds checks when reading
          // across boundaries. (This is because if we read at a bit offset of
          // 1, we'll read 31 bits from bytes 0-3, and load in 1 bit from bytes
          // 4-7.)
          if(fileTexSize > std::numeric_limits<size_t>::max() - 7)
          {
            return "This file is probably not valid: mip " + std::to_string(inputMip)
                   + " declared it contained so much data that if 7 more bytes were added, its size would overflow a size_t.";
          }
          std::vector<uint8_t> fileData;
          UNWRAP_ERROR(resizeVectorOrError(fileData, fileTexSize + 7));
          if(!input.read(reinterpret_cast<char*>(fileData.data()), static_cast<std::streamsize>(fileTexSize)))
          {
            return "Reading bitmasked data for an image in a DDS input failed. Is the input truncated?";
          }

          // Before we allocate data for the output, make sure it's also not
          // too large.
          assert(dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM || dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT);
          size_t outputTexSize = 0;
          if(!dxgiExportSize(mipWidth, mipHeight, mipDepth, dxgiFormat, outputTexSize) || outputTexSize > readSettings.maxSubresourceSizeBytes)
          {
            return "Mip " + std::to_string(inputMip) + " (" + std::to_string(mipWidth) + " x " + std::to_string(mipHeight)
                   + " x " + std::to_string(mipDepth) + ") was bitmasked and would have been decompressed to DXGI format "
                   + std::to_string(dxgiFormat) + "; that would have used more bytes than the maximum allowed in the DDS reader's parameters ("
                   + std::to_string(readSettings.maxSubresourceSizeBytes) + ").";
          }

          // Allocate the output:
          UNWRAP_ERROR(resource.create(outputTexSize, nullptr));
          char* outputData = resource.data.data();

          // Now iterate over pixels:
          size_t          bitPosition   = 0;
          size_t          pixelIdx      = 0;
          const uint32_t* fileDataBuf32 = reinterpret_cast<uint32_t*>(fileData.data());

          std::array<float, 4> pixel = {0.0F, 0.0F, 0.0F, 1.0F};

          for(size_t z = 0; z < mipDepth; ++z)
          {
            for(size_t y = 0; y < mipHeight; ++y)
            {
              for(size_t x = 0; x < mipWidth; ++x)
              {
                // Set dataBuf to 32 bits starting at bitPosition:
                const size_t wordIndex = bitPosition % 32;
                uint32_t     dataBuf   = fileDataBuf32[bitPosition / 32] >> wordIndex;
                if(wordIndex != 0)
                {
                  dataBuf |= ((fileDataBuf32[bitPosition / 32 + 1]) << (32 - wordIndex));
                }

                // Decompress the pixel:
                if(colorTransform == ColorTransform::eLuminance)
                {
                  const float v =
                      (i.bitmaskWasBumpDuDv ? bitsToSnorm(dataBuf, bitmaskMults[0]) : bitsToUnorm(dataBuf, bitmaskMults[0]));
                  pixel[0] = pixel[1] = pixel[2] = v;
                }
                else if(i.bitmaskHasRgb)
                {
                  for(int c = 0; c < 3; c++)
                  {
                    pixel[c] = (i.bitmaskWasBumpDuDv ? bitsToSnorm(dataBuf, bitmaskMults[c]) :
                                                       bitsToUnorm(dataBuf, bitmaskMults[c]));
                  }
                }

                if(i.bitmaskHasAlpha)
                {
                  pixel[3] =
                      (i.bitmaskWasBumpDuDv ? bitsToSnorm(dataBuf, bitmaskMults[3]) : bitsToUnorm(dataBuf, bitmaskMults[3]));
                }

                // Transform it to our output format:
                if(dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM)
                {
                  // RGBA8
                  uint8_t* outputPixelData = reinterpret_cast<uint8_t*>(outputData) + 4 * pixelIdx;
                  for(int c = 0; c < 4; c++)
                  {
                    // We use centered quantization here:
                    outputPixelData[c] = static_cast<uint8_t>(std::roundf(pixel[c] * 255.0F));
                  }
                }
                else
                {
                  // RGBAF32
                  float* outputPixelData = reinterpret_cast<float*>(outputData) + 4 * pixelIdx;
                  for(int c = 0; c < 4; c++)
                  {
                    outputPixelData[c] = pixel[c];
                  }
                }

                bitPosition += bitmaskedBitsPerPixel;
                pixelIdx++;
              }
            }
          }
        }
        else
        {
          // Fast path: not bitmasked; read it directly from the input into
          // the subresource.
          UNWRAP_ERROR(resource.create(fileTexSize, nullptr));
          if(!input.read(resource.data.data(), static_cast<std::streamsize>(fileTexSize)))
          {
            return "Copying data for an image in a DDS input failed. Is the input truncated?";
          }
        }
      }
    }
  }
  return {};  // Success!
}

ErrorWithText Image::readFromFile(const char* filename, const ReadSettings& readSettings)
{
  try
  {
    std::ifstream file(filename, std::ios::binary | std::ios::in);
    return readFromStream(file, readSettings);
  }
  catch(const std::exception& e)
  {
    return "I/O error opening " + std::string(filename) + ": " + std::string(e.what());
  }
}

ErrorWithText Image::readFromMemory(const char* buffer, size_t bufferSize, const ReadSettings& readSettings)
{
  if(bufferSize > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()))
  {
    return "The `bufferSize` parameter was too large to be stored in an std::streamsize.";
  }
  MemoryStream stream(buffer, bufferSize);
  return readFromStream(stream, readSettings);
}

ErrorWithText Image::writeToStream(std::ostream& output, const WriteSettings& writeSettings)
{
  //---------------------------------------------------------------------------
  // Image write: Define DDS Header and Pixel Format
  DDSHeader header = {0};
  static_assert(sizeof(DDSHeader) == 124, "DDS header size must be 124 by specification!");
  header.dwSize = sizeof(DDSHeader);

  // Specify which members contain valid data
  // Required components
  header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
  if(mip0Depth > 1)
  {
    // NVTTE-112: DDS has two flags for volumes.
    // The DX10 documentation confirms that DDSD_DEPTH indicates a volume
    // texture, since this used to be DDS_HEADER_FLAGS_VOLUME.
    header.dwFlags |= DDSD_DEPTH;
    header.dwCaps2 |= DDSCAPS2_VOLUME;
  }

  header.dwHeight = mip0Height;
  header.dwWidth  = mip0Width;
  header.dwDepth  = mip0Depth;

  // Double-check that this Image has been created correctly
  {
    size_t requiredSubresources = 0;
    if(!checked_math::mul3(m_numMips, m_numLayers, m_numFaces, requiredSubresources))
    {
      return "The number of mips, layers, and faces in this image is too large; the number of subresources they would require is greater than what would fit in a size_t.";
    }
    if(requiredSubresources != m_data.size())
    {
      return "This Image should have " + std::to_string(requiredSubresources) + " subresources, but m_data contained "
             + std::to_string(m_data.size()) + " subresources. Was this Image created correctly?";
    }
  }

  // Pitch or linear size
  if(isDXGIFormatCompressed(dxgiFormat))
  {
    // Linear size, the total number of bytes in the top level texture
    header.dwFlags |= DDSD_LINEARSIZE;
    const size_t mip0Size = getSize();
    if(mip0Size > std::numeric_limits<uint32_t>::max())
    {
      return "The number of bytes in the base mip of this texture was was greater than 2^32-1, and so wouldn't fit in the dwPitchOrLinearSize field of the DDS header.";
    }
    header.dwPitchOrLinearSize = static_cast<uint32_t>(mip0Size);
  }
  else
  {
    // Pitch, the number of bytes per scanline in an uncompressed texture
    // (we do this the simplest way possible, since currently every
    // uncompressed format has the same number of bytes per scanline)
    header.dwFlags |= DDSD_PITCH;
    const size_t mip0Pitch = getSize() / (static_cast<size_t>(mip0Height) * mip0Depth);
    if(mip0Pitch > std::numeric_limits<uint32_t>::max())
    {
      return "The pitch of the base mip of this texture was greater than 2^32-1, and so wouldn't fit in the dwPitchOrLinearSize field of the DDS header.";
    }
    header.dwPitchOrLinearSize = static_cast<uint32_t>(mip0Pitch);
  }

  header.dwMipMapCount = getNumMips();

  // Before we specify the pixel format, specify the complexity:
  header.dwCaps1 = DDSCAPS_TEXTURE;
  if(m_numMips > 1)
  {
    header.dwFlags |= DDSD_MIPMAPCOUNT;
    header.dwCaps1 |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
  }
  if(m_numFaces > 1)
  {
    header.dwCaps1 |= DDSCAPS_COMPLEX;
    header.dwCaps2 |= DDSCAPS2_CUBEMAP | cubemapFaceFlags;
  }

  if(hasUserVersion)
  {
    header.dwReserved1[7] = FOURCC_UVER;
    header.dwReserved1[8] = userVersion;
  }

  // Add a code to the reserved part of the header so that other readers
  // can tell that they're reading files written with the newest version of
  // NVDDS.
  header.dwReserved1[9]  = FOURCC_LIBRARY_NVPS;
  header.dwReserved1[10] = (2 << 16) | (1 << 8) | 0;

  //---------------------------------------------------------------------------
  // DDS Pixel Format
  // Specification: https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat
  DDSPixelFormat& pixelformat = header.ddspf;
  static_assert(sizeof(pixelformat) == 32, "DDS_PIXELFORMAT size must be 32, per specification!");
  pixelformat.dwSize = sizeof(pixelformat);

  // Note: I think DDPF_ALPHAPIXELS technically specifies that the texture
  // contains uncompressed alpha data, since the alpha bitmask (note that
  // this is really dwABitMask, not dwRGBAlphaBitMask) must then contain
  // valid data.

  // Note: We used to almost always write the DX10 header. However, it turns
  // out a lot of old readers -- which are still commonly used -- will crash
  // if given a file with this header. So now we have much finer conditions on
  // when we need to use one.
  // Specifically, we use the following set of rules, in order of precedence:
  // 1. Some formats are only specified in terms of bitmasks, so must use the
  // old spec.
  // 2. BC3n can only be specified using the DX9 header.
  // 3. Array textures must use the DX10 header.
  // 4. If useDX10HeaderIfPossible, use the DX10 header.
  // 5. Otherwise, use the DX10 hader only if DX9HeaderSupported(formatOpts) is false.
  bool       usesDXT10Header = false;
  const bool isBC3N          = (dxgiFormat == DXGI_FORMAT_BC3_UNORM || dxgiFormat == DXGI_FORMAT_BC3_TYPELESS)
                      && (colorTransform == ColorTransform::eAGBR);
  if(writeSettings.useCustomBitmask || isBC3N)
  {
    usesDXT10Header = false;
  }
  else if(m_numLayers > 1 || writeSettings.useDx10HeaderIfPossible)
  {
    usesDXT10Header = true;
  }
  else
  {
    usesDXT10Header = !dx9HeaderSupported(dxgiFormat);
  }

  if(usesDXT10Header)
  {
    pixelformat.dwFlags |= DDPF_FOURCC;
    pixelformat.dwFourCC = FOURCC_DX10;
  }
  else
  {
    setDX9PixelFormat(dxgiFormat, colorTransform, writeSettings, pixelformat);
  }


  if(isNormal)
  {
    pixelformat.dwFlags |= DDPF_NORMAL;
  }

  // If our data is premultiplied, also set DPPF_ALPHAPREMULT for compatibility:
  if(alphaMode == DDS_ALPHA_MODE_PREMULTIPLIED)
  {
    pixelformat.dwFlags |= DDPF_ALPHAPREMULT;
  }

  // Encode the color transform.
  switch(colorTransform)
  {
    case ColorTransform::eNone:
      break;
    case ColorTransform::eLuminance:
      pixelformat.dwFlags |= DDPF_LUMINANCE;
      break;
    case ColorTransform::eAGBR:
      // We can set this in dwRGBBitCount, but only if FourCC is nonzero (so
      // that there's no ambiguity).
      if(pixelformat.dwFourCC != 0)
      {
        pixelformat.dwRGBBitCount = FOURCC_A2D5;
      }
      // We always write DDPF_NORMAL when the data is in AGBR, so that
      // GIMP knows it needs to swap channels.
      pixelformat.dwFlags |= DDPF_NORMAL;
      break;
    case ColorTransform::eYUV:
      pixelformat.dwFlags |= DDPF_YUV;
      break;
    case ColorTransform::eYCoCg:
      header.dwReserved1[3] = FOURCC_YCOCG;
      break;
    case ColorTransform::eYCoCgScaled:
      header.dwReserved1[3] = FOURCC_YCOCG_SCALED;
      break;
    case ColorTransform::eAEXP:
      header.dwReserved1[3] = FOURCC_AEXP;
      break;
    case ColorTransform::eSwapRG:
      if(pixelformat.dwFourCC != 0)
      {
        pixelformat.dwRGBBitCount = FOURCC_A2XY;
      }
      break;
    case ColorTransform::eOrthographicNormal:
      // This can only be handled by SetDX9PixelFormat().
      break;
  }

  //---------------------------------------------------------------------------
  // Write the DDS magic number
  WRITE_OR_ERROR(output, FOURCC_DDS, "Could not write DDS magic number. Is writing to this file allowed?");

  // Write the DDS header
  WRITE_OR_ERROR(output, header, "Could not write DDS header.");

  //---------------------------------------------------------------------------
  // DX10 DDS extension
  // Specification: https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-header-dxt10
  if(usesDXT10Header)
  {
    DDSHeaderDX10 ddsh10     = {};
    ddsh10.dxgiFormat        = dxgiFormat;
    ddsh10.resourceDimension = resourceDimension;

    // If we have a complete cube map, it's appropriate to set this.
    // In the case of an incomplete cubemap, it's unclear. @nbickford believes
    // the spec says that if it's set, then there are 6 faces, so it would be
    // inconsistent to set it for an incomplete cubemap.
    if(m_numFaces == 6)
    {
      ddsh10.miscFlag = DDS_RESOURCE_MISC_TEXTURECUBE;
    }

    // Note: The Microsoft DDS spec and official DDS reader says arraySize is
    // the number of elements. Its cube map example sets arraySize to 6 for
    // a cubemap, but this is wrong, and RenderDoc will show the extra (blank)
    // elements if the reader or writer do this.
    // See NVTTE-97 and https://forums.developer.nvidia.com/t/texture-tools-exporter-cubemap-has-incorrect-arraysize/244753
    ddsh10.arraySize = m_numLayers;

    // Specify the transparency mode of the images.
    ddsh10.miscFlags2 = alphaMode;

    // Write DDS10 header
    static_assert(sizeof(ddsh10) == 20,
                  "DDSHeaderDX10 must be 20 bytes long (see "
                  "https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-file-layout-for-textures)!");
    WRITE_OR_ERROR(output, ddsh10, "Could not write DX10 extension.");
  }

  // Write all of the compressed data here.
  // Order:
  // for each array layer:
  //   for each cubemap face:
  //     for each mip:
  //       for each z slice:
  for(uint32_t layer = 0; layer < m_numLayers; layer++)
  {
    for(uint32_t face = 0; face < m_numFaces; face++)
    {
      for(uint32_t mip = 0; mip < m_numMips; mip++)
      {
        const std::vector<char>& data = subresource(mip, layer, face).data;
        if(!output.write(data.data(), static_cast<std::streamoff>(data.size())))
        {
          return "Could not write data for mip " + std::to_string(mip) + ", face " + std::to_string(face) + ", layer "
                 + std::to_string(layer) + ".";
        }
      }
    }
  }

  return {};
}

ErrorWithText Image::writeToFile(const char* filename, const WriteSettings& writeSettings)
{
  try
  {
    std::ofstream file(filename, std::ios::binary | std::ios::out);
    return writeToStream(file, writeSettings);
  }
  catch(const std::exception& e)
  {
    return "I/O error opening " + std::string(filename) + ": " + std::string(e.what());
  }
}

std::string Image::formatInfo() const
{
  // This initial implementation is based on NVTT 3.2.6's DDS implementation.
  const DDSHeader& header = m_fileInfo.ddsh;  // Short alias

  std::stringstream s;
  s << std::setfill('0') << std::uppercase;
  s << "Flags: 0x" << std::hex << std::setw(8) << header.dwFlags << std::dec << "\n";
  if((header.dwFlags & DDSD_CAPS) != 0)
    s << "\tDDSD_CAPS\n";
  if((header.dwFlags & DDSD_PIXELFORMAT) != 0)
    s << "\tDDSD_PIXELFORMAT\n";
  if((header.dwFlags & DDSD_WIDTH) != 0)
    s << "\tDDSD_WIDTH\n";
  if((header.dwFlags & DDSD_HEIGHT) != 0)
    s << "\tDDSD_HEIGHT\n";
  if((header.dwFlags & DDSD_DEPTH) != 0)
    s << "\tDDSD_DEPTH\n";
  if((header.dwFlags & DDSD_PITCH) != 0)
    s << "\tDDSD_PITCH\n";
  if((header.dwFlags & DDSD_LINEARSIZE) != 0)
    s << "\tDDSD_LINEARSIZE\n";
  if((header.dwFlags & DDSD_MIPMAPCOUNT) != 0)
    s << "\tDDSD_MIPMAPCOUNT\n";

  s << "Height: " << header.dwHeight << "\n";
  s << "Width: " << header.dwWidth << "\n";
  s << "Depth: " << header.dwDepth << "\n";
  if((header.dwFlags & DDSD_PITCH) != 0)
    s << "Pitch: " << header.dwPitchOrLinearSize << "\n";
  else if((header.dwFlags & DDSD_LINEARSIZE) != 0)
    s << "Linear size: " << header.dwPitchOrLinearSize << "\n";
  s << "Mipmap count: " << header.dwMipMapCount << "\n";

  s << "Pixel format:\n";
  s << "\tFlags: 0x" << std::hex << std::setw(8) << header.ddspf.dwFlags << std::dec << "\n";
  if((header.ddspf.dwFlags & DDPF_ALPHAPIXELS) != 0)
    s << "\t\tDDPF_ALPHAPIXELS\n";
  if((header.ddspf.dwFlags & DDPF_ALPHA) != 0)
    s << "\t\tDDPF_ALPHA\n";
  if((header.ddspf.dwFlags & DDPF_FOURCC) != 0)
    s << "\t\tDDPF_FOURCC\n";
  if((header.ddspf.dwFlags & DDPF_PALETTEINDEXED4) != 0)
    s << "\t\tDDPF_PALETTEINDEXED4\n";
  if((header.ddspf.dwFlags & DDPF_PALETTEINDEXEDTO8) != 0)
    s << "\t\tDDPF_PALETTEINDEXEDTO8\n";
  if((header.ddspf.dwFlags & DDPF_PALETTEINDEXED8) != 0)
    s << "\t\tDDPF_PALETTEINDEXED8\n";
  if((header.ddspf.dwFlags & DDPF_RGB) != 0)
    s << "\t\tDDPF_RGB\n";
  if((header.ddspf.dwFlags & DDPF_COMPRESSED) != 0)
    s << "\t\tDDPF_COMPRESSED\n";
  if((header.ddspf.dwFlags & DDPF_RGBTOYUV) != 0)
    s << "\t\tDDPF_RGBTOYUV\n";
  if((header.ddspf.dwFlags & DDPF_YUV) != 0)
    s << "\t\tDDPF_YUV\n";
  if((header.ddspf.dwFlags & DDPF_ZBUFFER) != 0)
    s << "\t\tDDPF_ZBUFFER\n";
  if((header.ddspf.dwFlags & DDPF_PALETTEINDEXED1) != 0)
    s << "\t\tDDPF_PALETTEINDEXED1\n";
  if((header.ddspf.dwFlags & DDPF_PALETTEINDEXED2) != 0)
    s << "\t\tDDPF_PALETTEINDEXED2\n";
  if((header.ddspf.dwFlags & DDPF_ZPIXELS) != 0)
    s << "\t\tDDPF_ZPIXELS\n";
  if((header.ddspf.dwFlags & DDPF_STENCILBUFFER) != 0)
    s << "\t\tDDPF_STENCILBUFFER\n";
  if((header.ddspf.dwFlags & DDPF_ALPHAPREMULT) != 0)
    s << "\t\tDDPF_ALPHAPREMULT\n";
  if((header.ddspf.dwFlags & DDPF_LUMINANCE) != 0)
    s << "\t\tDDPF_LUMINANCE\n";
  if((header.ddspf.dwFlags & DDPF_BUMPLUMINANCE) != 0)
    s << "\t\tDDPF_BUMPLUMINANCE\n";
  if((header.ddspf.dwFlags & DDPF_BUMPDUDV) != 0)
    s << "\t\tDDPF_BUMPDUDV\n";
  if((header.ddspf.dwFlags & DDPF_SRGB) != 0)
    s << "\t\tDDPF_SRGB\n";
  if((header.ddspf.dwFlags & DDPF_NORMAL) != 0)
    s << "\t\tDDPF_NORMAL\n";

  // Display fourcc code even when DDPF_FOURCC flag not set.
  if(header.ddspf.dwFourCC != 0)
  {
    s << "\tFourCC: " << makeFourCCPrintable(header.ddspf.dwFourCC);
    s << " (0x" << std::hex << std::setw(8) << header.ddspf.dwFourCC << std::dec << ")\n";
  }

  // If the pixel format uses a FourCC code, normally bitmasks aren't used. So
  // sometimes the dwRGBBitCount field is used to encode a 'swizzle code', like
  // 'A2D5' or 'A2XY'. NVTT sometimes does this; @nbickford isn't aware of
  // any other libraries that do.
  if(((header.ddspf.dwFlags & DDPF_FOURCC) != 0) && (header.ddspf.dwRGBBitCount != 0))
  {
    s << "\tSwizzle: " << makeFourCCPrintable(header.ddspf.dwRGBBitCount);
    s << " (0x" << std::hex << std::setw(8) << header.ddspf.dwRGBBitCount << std::dec << ")\n";
  }
  else
  {
    s << "\tBit count: " << header.ddspf.dwRGBBitCount << "\n";
  }

  s << std::hex;

  s << "\tRed mask:   0x" << std::setw(8) << header.ddspf.dwRBitMask << "\n";
  s << "\tGreen mask: 0x" << std::setw(8) << header.ddspf.dwGBitMask << "\n";
  s << "\tBlue mask:  0x" << std::setw(8) << header.ddspf.dwBBitMask << "\n";
  s << "\tAlpha mask: 0x" << std::setw(8) << header.ddspf.dwABitMask << "\n";

  s << "Caps:\n";
  s << "\tCaps 1: 0x" << std::setw(8) << header.dwCaps1 << "\n";
  if((header.dwCaps1 & DDSCAPS_COMPLEX) != 0)
    s << "\t\tDDSCAPS_COMPLEX\n";
  if((header.dwCaps1 & DDSCAPS_TEXTURE) != 0)
    s << "\t\tDDSCAPS_TEXTURE\n";
  if((header.dwCaps1 & DDSCAPS_MIPMAP) != 0)
    s << "\t\tDDSCAPS_MIPMAP\n";

  s << "\tCaps 2: 0x" << std::setw(8) << header.dwCaps2 << "\n";
  if((header.dwCaps2 & DDSCAPS2_CUBEMAP) != 0)
    s << "\t\tDDSCAPS2_CUBEMAP\n";
  if((header.dwCaps2 & DDSCAPS2_CUBEMAP_ALL_FACES) == DDSCAPS2_CUBEMAP_ALL_FACES)
  {
    s << "\t\tDDSCAPS2_CUBEMAP_ALL_FACES\n";
  }
  else
  {
    if((header.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX) != 0)
      s << "\t\tDDSCAPS2_CUBEMAP_POSITIVEX\n";
    if((header.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) != 0)
      s << "\t\tDDSCAPS2_CUBEMAP_NEGATIVEX\n";
    if((header.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY) != 0)
      s << "\t\tDDSCAPS2_CUBEMAP_POSITIVEY\n";
    if((header.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) != 0)
      s << "\t\tDDSCAPS2_CUBEMAP_NEGATIVEY\n";
    if((header.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) != 0)
      s << "\t\tDDSCAPS2_CUBEMAP_POSITIVEZ\n";
    if((header.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) != 0)
      s << "\t\tDDSCAPS2_CUBEMAP_NEGATIVEZ\n";
  }
  if((header.dwCaps2 & DDSCAPS2_VOLUME) != 0)
    s << "\t\tDDSCAPS2_VOLUME\n";

  s << std::dec;

  if(m_fileInfo.hadDx10Extension)
  {
    const nv_dds::DDSHeaderDX10& ddsh10 = m_fileInfo.ddsh10;

    s << "DX10 Header:\n";
    const char* dxgiFormatName = texture_formats::getDXGIFormatName(ddsh10.dxgiFormat);
    s << "\tDXGI Format: " << ddsh10.dxgiFormat << " ("        //
      << ((dxgiFormatName == nullptr) ? "?" : dxgiFormatName)  //
      << ")\n";
    s << "\tResource dimension: " << static_cast<uint32_t>(ddsh10.resourceDimension) << " (";
    switch(ddsh10.resourceDimension)
    {
      case ResourceDimension::eUnknown:
        s << "UNKNOWN";
        break;
      case ResourceDimension::eBuffer:
        s << "BUFFER";
        break;
      case ResourceDimension::eTexture1D:
        s << "TEXTURE1D";
        break;
      case ResourceDimension::eTexture2D:
        s << "TEXTURE2D";
        break;
      case ResourceDimension::eTexture3D:
        s << "TEXTURE3D";
        break;
      default:
        s << "?";
        break;
    }
    s << ")\n";

    s << "\tMisc flags: " << ddsh10.miscFlag << "\n";
    if((ddsh10.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE) != 0)
      s << "\t\tDDS_RESOURCE_MISC_TEXTURECUBE\n";
    s << "\tArray size flag: " << ddsh10.arraySize << "\n";
    s << "\tMisc flags 2: " << ddsh10.miscFlags2 << "\n";
    switch(alphaMode)
    {
      case DDS_ALPHA_MODE_UNKNOWN:
        s << "\t\tDDS_ALPHA_MODE_UNKNOWN\n";
        break;
      case DDS_ALPHA_MODE_STRAIGHT:
        s << "\t\tDDS_ALPHA_MODE_STRAIGHT\n";
        break;
      case DDS_ALPHA_MODE_PREMULTIPLIED:
        s << "\t\tDDS_ALPHA_MODE_PREMULTIPLIED\n";
        break;
      case DDS_ALPHA_MODE_OPAQUE:
        s << "\t\tDDS_ALPHA_MODE_OPAQUE\n";
        break;
      case DDS_ALPHA_MODE_CUSTOM:
        s << "\t\tDDS_ALPHA_MODE_CUSTOM\n";
        break;
      default:
        break;
    }
  }
  else
  {
    // No DDS10 header, but still show the format if we determined what it was
    if(0 != dxgiFormat)
    {
      if(m_fileInfo.wasBitmasked)
      {
        s << "Bitmask would be decompressed to DXGI format: ";
      }
      else
      {
        s << "Inferred DXGI format: ";
      }
      const char* dxgiFormatName = texture_formats::getDXGIFormatName(dxgiFormat);
      s << dxgiFormat << " (" << (dxgiFormatName ? dxgiFormatName : "?") << ")\n";
    }
  }

  // Library
  uint16_t v0 = 0;
  uint8_t  v1 = 0;
  uint8_t  v2 = 0;
  parse3ByteLibraryVersion(m_fileInfo.writerLibraryVersion, v0, v1, v2);
  switch(m_fileInfo.writerLibrary)
  {
    case WriterLibrary::eUnknown:
      break;
    case WriterLibrary::eNVTT:
      s << "Library: NVIDIA Texture Tools\n";
      // These casts are so that they're formatted as numbers instead of
      // characters -- this happens even with uint8_t.
      s << "\tVersion: " << static_cast<uint32_t>(v0) << "." << static_cast<uint32_t>(v1) << "."
        << static_cast<uint32_t>(v2) << "\n";
      break;
    case WriterLibrary::eNVTTExporter:
      s << "Library: NVIDIA Texture Tools Exporter\n";
      switch(m_fileInfo.writerLibraryVersion)
      {
        case LIBRARY_EXPORTER_VERSION_START_THROUGH_2023_1_0:
          s << "\tVersion: 2020.1.0 - 2023.1.0\n";
          break;
        case LIBRARY_EXPORTER_VERSION_2023_1_1_PLUS:
          s << "\tVersion: 2023.1.1+\n";
          break;
        default:
          s << "\tVersion: Unknown\n";
          break;
      }
      break;
    case WriterLibrary::eNVPS:
      s << "Library: nv_dds\n";
      s << "\tVersion: " << static_cast<uint32_t>(v0) << "." << static_cast<uint32_t>(v1) << "."
        << static_cast<uint32_t>(v2) << "\n";
      break;
    case WriterLibrary::eGIMP:
      s << "Library: GNU Image Manipulation Program's DDS plugin\n";
      s << "\tVersion: " << static_cast<uint32_t>(v0) << "." << static_cast<uint32_t>(v1) << "."
        << static_cast<uint32_t>(v2) << "\n";
      s << "\tGIMP Format FourCC: " << makeFourCCPrintable(header.dwReserved1[3]);
      s << " (0x" << std::hex << std::setw(8) << header.dwReserved1[3] << std::dec << ")\n";
      break;
    default:
      s << "Library: " << static_cast<uint32_t>(m_fileInfo.writerLibrary) << "\n";
      break;
  }

  // User version
  if(hasUserVersion)
  {
    s << "User version: " << userVersion << "\n";
  }

  return s.str();
}

}  // namespace nv_dds
