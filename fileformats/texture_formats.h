/*
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

/** @DOC_START

Provides:
* Methods for translating texture format names between DirectX, Vulkan, and
OpenGL.
* A method for getting the size of a Vulkan subresource using linear tiling.
* The extended ASTC values for DXGI_FORMAT.

-- @DOC_END */

#pragma once
#include <cstddef>
#include <cstdint>

#ifdef NVP_SUPPORTS_VULKANSDK
#include <vulkan/vulkan_core.h>  // For VkFormat
#endif

namespace texture_formats {

struct OpenGLFormat
{
  uint32_t internalFormat = 0;
  // Unused if using a compressed format; otherwise, specifies the format of
  // client pixel data.
  uint32_t format = 0;
  // Unused (0) if using a compressed format; otherwise, specifies the OpenGL
  // data type of client pixel data.
  uint32_t type = 0;

  bool operator==(const OpenGLFormat& b) const
  {
    return (internalFormat == b.internalFormat) && (format == b.format) && (type == b.type);
  }
};

// API texture format name translation functions.
// Each of these returns an "unknown" format -- 0 -- if it couldn't figure out
// what the corresponding format was.
OpenGLFormat dxgiToOpenGL(uint32_t dxgiFormat);
uint32_t     openGLToDXGI(const OpenGLFormat& glFormat);
#ifdef NVP_SUPPORTS_VULKANSDK
VkFormat     dxgiToVulkan(uint32_t dxgiFormat);
VkFormat     openGLToVulkan(const OpenGLFormat& glFormat);
uint32_t     vulkanToDXGI(VkFormat vkFormat);
OpenGLFormat vulkanToOpenGL(VkFormat vkFormat);
#endif

// Returns the enum name of a DXGI format. If the name isn't contained in
// texture_format's tables, returns nullptr.
const char* getDXGIFormatName(uint32_t dxgiFormat);

#ifdef NVP_SUPPORTS_VULKANSDK
// Returns the enum name of a VkFormat. If the name isn't contained in
// texture_format's tables, returns nullptr.
const char* getVkFormatName(VkFormat vkFormat);
#endif

// Returns whether the given DXGI format ends in _SRGB, i.e. whether the GPU
// automatically performs sRGB-to-linear conversion when sampling it.
bool isDXGIFormatSRGB(uint32_t dxgiFormat);
// Tries to change the given DXGI format to another one that uses the given
// transfer function, if it exists. Otherwise, returns the input.
// For instance, tryForceDXGITransferFunction(DXGI_FORMAT_BC2_UNORM, true)
// returns DXGI_FORMAT_BC2_UNORM_SRGB, while
// tryForceDXGITransferFunction(DXGI_FORMAT_BC4_UNORM, true) returns
// DXGI_FORMAT_BC4_UNORM, since there is no DXGI_FORMAT_BC4_UNORM_SRGB.
// This is useful because by convention, both UNORM and UNORM_SRGB DDS files
// typically contain sRGB data, but the engine usually knows whether it wants
// the GPU to perform automatic sRGB-to-linear conversion.
uint32_t tryForceDXGIFormatTransferFunction(uint32_t dxgiFormat, bool srgb);

#ifdef NVP_SUPPORTS_VULKANSDK
// Returns whether the given VkFormat includes _SRGB, i.e. whether the GPU
// automatically performs sRGB-to-linear conversion when sampling it.
bool isVkFormatSRGB(VkFormat vkFormat);
// Tries to change the given VkFormat to another one that uses the given
// transfer function, if it exists. Otherwise, returns the input.
VkFormat tryForceVkFormatTransferFunction(VkFormat vkFormat, bool srgb);
#endif


// DXGI ASTC extension
// According to Fei Yang, these once appeared in an MS document, then disappeared.
// We filled in DXGI_FORMAT_ASTC_4X4_TYPELESS, which was missing,
// using https://gli.g-truc.net/0.6.1/api/a00001.html.
const uint32_t DXGI_FORMAT_ASTC_4X4_TYPELESS     = 133;
const uint32_t DXGI_FORMAT_ASTC_4X4_UNORM        = 134;
const uint32_t DXGI_FORMAT_ASTC_4X4_UNORM_SRGB   = 135;
const uint32_t DXGI_FORMAT_ASTC_5X4_TYPELESS     = 137;
const uint32_t DXGI_FORMAT_ASTC_5X4_UNORM        = 138;
const uint32_t DXGI_FORMAT_ASTC_5X4_UNORM_SRGB   = 139;
const uint32_t DXGI_FORMAT_ASTC_5X5_TYPELESS     = 141;
const uint32_t DXGI_FORMAT_ASTC_5X5_UNORM        = 142;
const uint32_t DXGI_FORMAT_ASTC_5X5_UNORM_SRGB   = 143;
const uint32_t DXGI_FORMAT_ASTC_6X5_TYPELESS     = 145;
const uint32_t DXGI_FORMAT_ASTC_6X5_UNORM        = 146;
const uint32_t DXGI_FORMAT_ASTC_6X5_UNORM_SRGB   = 147;
const uint32_t DXGI_FORMAT_ASTC_6X6_TYPELESS     = 149;
const uint32_t DXGI_FORMAT_ASTC_6X6_UNORM        = 150;
const uint32_t DXGI_FORMAT_ASTC_6X6_UNORM_SRGB   = 151;
const uint32_t DXGI_FORMAT_ASTC_8X5_TYPELESS     = 153;
const uint32_t DXGI_FORMAT_ASTC_8X5_UNORM        = 154;
const uint32_t DXGI_FORMAT_ASTC_8X5_UNORM_SRGB   = 155;
const uint32_t DXGI_FORMAT_ASTC_8X6_TYPELESS     = 157;
const uint32_t DXGI_FORMAT_ASTC_8X6_UNORM        = 158;
const uint32_t DXGI_FORMAT_ASTC_8X6_UNORM_SRGB   = 159;
const uint32_t DXGI_FORMAT_ASTC_8X8_TYPELESS     = 161;
const uint32_t DXGI_FORMAT_ASTC_8X8_UNORM        = 162;
const uint32_t DXGI_FORMAT_ASTC_8X8_UNORM_SRGB   = 163;
const uint32_t DXGI_FORMAT_ASTC_10X5_TYPELESS    = 165;
const uint32_t DXGI_FORMAT_ASTC_10X5_UNORM       = 166;
const uint32_t DXGI_FORMAT_ASTC_10X5_UNORM_SRGB  = 167;
const uint32_t DXGI_FORMAT_ASTC_10X6_TYPELESS    = 169;
const uint32_t DXGI_FORMAT_ASTC_10X6_UNORM       = 170;
const uint32_t DXGI_FORMAT_ASTC_10X6_UNORM_SRGB  = 171;
const uint32_t DXGI_FORMAT_ASTC_10X8_TYPELESS    = 173;
const uint32_t DXGI_FORMAT_ASTC_10X8_UNORM       = 174;
const uint32_t DXGI_FORMAT_ASTC_10X8_UNORM_SRGB  = 175;
const uint32_t DXGI_FORMAT_ASTC_10X10_TYPELESS   = 177;
const uint32_t DXGI_FORMAT_ASTC_10X10_UNORM      = 178;
const uint32_t DXGI_FORMAT_ASTC_10X10_UNORM_SRGB = 179;
const uint32_t DXGI_FORMAT_ASTC_12X10_TYPELESS   = 181;
const uint32_t DXGI_FORMAT_ASTC_12X10_UNORM      = 182;
const uint32_t DXGI_FORMAT_ASTC_12X10_UNORM_SRGB = 183;
const uint32_t DXGI_FORMAT_ASTC_12X12_TYPELESS   = 185;
const uint32_t DXGI_FORMAT_ASTC_12X12_UNORM      = 186;
const uint32_t DXGI_FORMAT_ASTC_12X12_UNORM_SRGB = 187;

};  // namespace texture_formats

namespace checked_math {

// Multiplies two values, returning false if the calculation would overflow.
inline bool mul2(size_t a, size_t b, size_t& out)
{
  if((a != 0) && (b > SIZE_MAX / a))  // Safe way of checking a * b > SIZE_MAX
  {
    return false;
  }
  out = a * b;
  return true;
}

// Multiplies three values, returning false if the calculation would overflow.
inline bool mul3(size_t a, size_t b, size_t c, size_t& out)
{
  // a * b * 0 is always OK, even if a * b overflows.
  if(c == 0)
  {
    out = 0;
    return true;
  }
  if(!mul2(a, b, out))
    return false;
  if(!mul2(c, out, out))
    return false;
  return true;
}

// Multiplies four values, returning false if the calculation would overflow.
inline bool mul4(size_t a, size_t b, size_t c, size_t d, size_t& out)
{
  // a * b * c * 0 is always OK, even if a * b * c overflows.
  if(d == 0)
  {
    out = 0;
    return true;
  }
  if(!mul3(a, b, c, out))
    return false;
  if(!mul2(d, out, out))
    return false;
  return true;
}

// Multiplies five values, returning false if the calculation would overflow.
inline bool mul5(size_t a, size_t b, size_t c, size_t d, size_t e, size_t& out)
{
  // a * b * c * d * 0 is always OK, even if a * b * c * d overflows.
  if(e == 0)
  {
    out = 0;
    return true;
  }
  if(!mul4(a, b, c, d, out))
    return false;
  if(!mul2(e, out, out))
    return false;
  return true;
}

};  // namespace checked_math
