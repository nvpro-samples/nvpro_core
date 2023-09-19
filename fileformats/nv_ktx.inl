#include "nv_ktx.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>  // Some functions produce assertion errors to assist with debugging when NDEBUG is false.
#include <fstream>
#include <mutex>
#include <sstream>
#include <string.h>  // memcpy
#include <vulkan/vulkan_core.h>
#ifdef NVP_SUPPORTS_ZSTD
#include <zstd.h>
#endif
#ifdef NVP_SUPPORTS_GZLIB
#include <zlib.h>
#endif
#ifdef NVP_SUPPORTS_BASISU
#include <basisu_comp.h>
#include <basisu_transcoder.h>
#endif

#include "khr_df.h"

namespace nv_ktx {

// Some sources for this code:
// KTX 1 specification at https://github.com/KhronosGroup/KTX-Specification/releases/tag/1.0-final
// KTX 2 specification at https://github.khronos.org/KTX-Specification/
// Khronos Data Format Specification 1.3.1 at https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html
//   (especially chapters 3-5)

//-----------------------------------------------------------------------------
// SHARED KTX1 + KTX2 FUNCTIONS
//-----------------------------------------------------------------------------

const size_t  IDENTIFIER_LEN                 = 12;
const uint8_t ktx1Identifier[IDENTIFIER_LEN] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
const uint8_t ktx2Identifier[IDENTIFIER_LEN] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};

// Resizing a vector can produce an exception if the allocation fails, but
// there's no real way to validate this ahead of time. So we use
// a try/catch block.
template <class T>
inline ErrorWithText ResizeVectorOrError(std::vector<T>& vec, size_t newSize)
{
  try
  {
    vec.resize(newSize);
  }
  catch(...)
  {
    return "Allocating " + std::to_string(newSize) + " bytes of data failed.";
  }
  return {};
}

// Multiplies two values, returning false if the calculation would overflow
inline bool CheckedMul2(size_t a, size_t b, size_t& out)
{
  if((a != 0) && (b > SIZE_MAX / a))  // Safe way of checking a * b > SIZE_MAX
  {
    return false;
  }
  out = a * b;
  return true;
}

inline bool CheckedMul3(size_t a, size_t b, size_t c, size_t& out)
{
  if(!CheckedMul2(a, b, out))
    return false;
  if(!CheckedMul2(c, out, out))
    return false;
  return true;
}

inline bool CheckedMul4(size_t a, size_t b, size_t c, size_t d, size_t& out)
{
  if(!CheckedMul3(a, b, c, out))
    return false;
  if(!CheckedMul2(d, out, out))
    return false;
  return true;
}

inline bool CheckedMul5(size_t a, size_t b, size_t c, size_t d, size_t e, size_t& out)
{
  if(!CheckedMul4(a, b, c, d, out))
    return false;
  if(!CheckedMul2(e, out, out))
    return false;
  return true;
}

// Multiplies three values, returning false if the calculation would overflow,
// interpreting each value as 1 if it would be 0.
inline bool GetNumSubresources(size_t a, size_t b, size_t c, size_t& out)
{
  return CheckedMul3(std::max(a, size_t(1)), std::max(b, size_t(1)), std::max(c, size_t(1)), out);
}

inline ErrorWithText KTXImage::allocate(uint32_t _num_mips, uint32_t _num_layers, uint32_t _num_faces)
{
  clear();

  num_mips              = _num_mips;
  num_layers_possibly_0 = _num_layers;
  num_faces             = _num_faces;

  size_t num_subresources = 0;
  if(!GetNumSubresources(num_mips, num_layers_possibly_0, num_faces, num_subresources))
  {
    return "Computing the required number of subresources overflowed a size_t!";
  }
  return ResizeVectorOrError(data, num_subresources);
}

inline void KTXImage::clear()
{
  data.clear();
}

inline std::vector<char>& KTXImage::subresource(uint32_t mip, uint32_t layer, uint32_t face)
{
  const uint32_t num_mips_clamped   = std::max(num_mips, 1U);
  const uint32_t num_layers_clamped = std::max(num_layers_possibly_0, 1U);
  if(mip >= num_mips_clamped || layer >= num_layers_clamped || face >= num_faces)
  {
    throw std::out_of_range("KTXImage::subresource values were out of range");
  }

  // Here's the layout for data that we use. Note that we store the lowest mips
  // (mip 0) first, while the KTX format stores the highest mips first.
  return data[(size_t(mip) * size_t(num_layers_clamped) + size_t(layer)) * size_t(num_faces) + size_t(face)];
}

inline VkImageType KTXImage::getImageType() const
{
  if(mip_0_width == 0)
  {
    return VK_IMAGE_TYPE_1D;
  }
  else if(mip_0_depth == 0)
  {
    return VK_IMAGE_TYPE_2D;
  }
  else
  {
    return VK_IMAGE_TYPE_3D;
  }
}

inline uint32_t KTXImage::getKTXVersion() const
{
  return read_ktx_version;
}

// Macro for "read this variable from the istream; if it fails, return an error message"
#define READ_OR_ERROR(input, variable, error_message)                                                                  \
  if(!(input).read(reinterpret_cast<char*>(&(variable)), sizeof(variable)))                                            \
  {                                                                                                                    \
    return (error_message);                                                                                            \
  }

// Macro for "If this returned an error, propagate that error"
#define UNWRAP_ERROR(expr_returning_error_with_text)                                                                   \
  if(ErrorWithText unwrap_error_tmp = (expr_returning_error_with_text))                                                \
  {                                                                                                                    \
    return unwrap_error_tmp;                                                                                           \
  }

inline size_t RoundUp(size_t value, size_t multiplier)
{
  const size_t mod = value % multiplier;
  if(mod == 0)
    return value;
  return value + (multiplier - mod);
}

// Basic Data Format Descriptor from the Khronos Data Format,
// without sample information.
struct BasicDataFormatDescriptor
{
  uint32_t vendorId : 17;
  uint32_t descriptorType : 15;
  uint16_t versionNumber : 16;
  uint16_t descriptorBlockSize : 16;
  uint8_t  colorModel : 8;
  uint8_t  colorPrimaries : 8;
  uint8_t  transferFunction : 8;
  uint8_t  flags : 8;
  uint8_t  texelBlockDimension0 : 8;
  uint8_t  texelBlockDimension1 : 8;
  uint8_t  texelBlockDimension2 : 8;
  uint8_t  texelBlockDimension3 : 8;
  uint8_t  bytesPlane0 : 8;
  uint8_t  bytesPlane1 : 8;
  uint8_t  bytesPlane2 : 8;
  uint8_t  bytesPlane3 : 8;
  uint8_t  bytesPlane4 : 8;
  uint8_t  bytesPlane5 : 8;
  uint8_t  bytesPlane6 : 8;
  uint8_t  bytesPlane7 : 8;
};
static_assert(sizeof(BasicDataFormatDescriptor) == 24, "Basic data format descriptor size must match the KDF spec!");

struct DFSample
{
  uint16_t bitOffset;
  uint8_t  bitLength;
  uint8_t  channelType;
  uint8_t  samplePosition0;
  uint8_t  samplePosition1;
  uint8_t  samplePosition2;
  uint8_t  samplePosition3;
  uint32_t lower;
  uint32_t upper;
};
static_assert(sizeof(DFSample) == 16, "Basic data format descriptor sample type size must match Khronos Data Format spec!");

// Interprets T as an array of uint32_ts and swaps the endianness of each element.
template <class T>
inline void SwapEndian32(T& data)
{
  static_assert((sizeof(T) % 4) == 0, "T must be interpretable as an array of 32-bit words.");
  size_t    numInts           = sizeof(T) / 4;
  uint32_t* reinterpretedData = reinterpret_cast<uint32_t*>(&data);
  for(size_t i = 0; i < numInts; i++)
  {
    uint32_t value = reinterpretedData[i];
    value          = ((value & 0x000000FFu) << 24)  //
            | ((value & 0x0000FF00u) << 8)          //
            | ((value & 0x00FF0000u) >> 8)          //
            | ((value & 0xFF000000u) >> 24);
    reinterpretedData[i] = value;
  }
}

// Interprets data, an array dataSizeBytes long, as an array of elements of size
// typeSizeBytes. Then swaps the endianness of each element.
inline void SwapEndianGeneral(size_t dataSizeBytes, void* data, uint32_t typeSizeBytes)
{
  if(typeSizeBytes == 0 || typeSizeBytes == 1)
  {
    return;  // Nothing to do
  }
  assert(dataSizeBytes % typeSizeBytes == 0);  // Otherwise we'll swap all but the last element
  uint8_t*     dataAsBytes = reinterpret_cast<uint8_t*>(data);
  const size_t typeSize64  = size_t(typeSizeBytes);
  const size_t numElements = dataSizeBytes / typeSizeBytes;  // e.g. 5/3 -> 1 element
  const size_t numSwaps    = typeSize64 / 2;                 // e.g. 3 bytes -> 1 swap

  for(size_t eltIdx = 0; eltIdx < numElements; eltIdx++)
  {
    for(size_t swapIdx = 0; swapIdx < numSwaps; swapIdx++)
    {
      std::swap(dataAsBytes[eltIdx * typeSize64 + swapIdx], dataAsBytes[eltIdx * typeSize64 + typeSize64 - 1 - swapIdx]);
    }
  }
}

static_assert(sizeof(char) == sizeof(uint8_t), "Things will probably go wrong in nv_ktx code with istream reads if chars aren't bytes.");

inline ErrorWithText ReadKeyValueData(std::istream&                             input,
                                      uint32_t                                  kvdByteLength,
                                      bool                                      srcIsBigEndian,
                                      std::map<std::string, std::vector<char>>& outKeyValueData)
{
  std::vector<char> kvBlock;
  UNWRAP_ERROR(ResizeVectorOrError(kvBlock, kvdByteLength));
  if(!input.read(kvBlock.data(), size_t(kvdByteLength)))
  {
    return "Unable to read " + std::to_string(kvdByteLength) + " bytes of KTX2 key/value data.";
  }

  size_t byteIndex = 0;
  while(byteIndex < kvdByteLength)
  {
    // Read keyAndValueByteLength
    uint32_t keyAndValueByteLength = 0;
    // Check to make sure we don't read out of bounds
    if(byteIndex + sizeof(keyAndValueByteLength) >= kvBlock.size())
    {
      return "Key/value data starting at byte " + std::to_string(byteIndex)
             + "of the key/value data block did not have enough space to contain the 32-bit key/value size. Is the key/value data truncated?";
    }
    memcpy(&keyAndValueByteLength, &kvBlock[byteIndex], sizeof(keyAndValueByteLength));
    if(srcIsBigEndian)
    {
      SwapEndian32<uint32_t>(keyAndValueByteLength);
    }
    byteIndex += sizeof(keyAndValueByteLength);

    // If byteIndex + keyAndValueByteLength > the length of kvBlock, we read
    // byteIndex incorrectly; we'll treat this as a non-fatal error.
    if(byteIndex + keyAndValueByteLength > kvBlock.size())
    {
      assert("Key/value data had byte length that was too long!");
      return {};
    }

    // Parse the key and value according to 3.11.2 "keyAndValue".
    // First, find the index of the NUL character terminating the key.
    size_t keyLength = 0;
    while(keyLength < size_t(keyAndValueByteLength))
    {
      if(kvBlock[byteIndex + keyLength] == '\0')
      {
        break;  // Found the NUL character!
      }
      keyLength++;  // Next character
    }

    // If we somehow reached the end without finding the NUL character, this
    // key is invalid - we could read it correctly, but others might not,
    // and we have to copy the keys to the output anyways.
    if(keyLength == size_t(keyAndValueByteLength))
    {
      return "Key starting at byte " + std::to_string(byteIndex) + " of the key/value data did not have a NUL character.";
    }

    // Construct the key and value from ranges.
    std::string key(&kvBlock[byteIndex], keyLength);  // Don't include null character
    std::vector<char> value(kvBlock.begin() + (byteIndex + keyLength + 1), kvBlock.begin() + (byteIndex + keyAndValueByteLength));
    // Handle duplicate keys gracefully by using later keys. Note that KTX
    // requires that keys not be duplicated.
    outKeyValueData.insert_or_assign(key, value);

    // Skip directly to the next key, including padding.
    byteIndex += RoundUp(size_t(keyAndValueByteLength), 4);
  }

  return {};
}

// Computes the size of a subresource of size `width` x `height` x `depth`, encoded
// using ASTC blocks of size `blockWidth` x `blockHeight` x `blockDepth`. Returns false
// if the calculation would overflow, and returns true and stores the result in
// `out` otherwise.
inline bool ASTCSize(size_t blockWidth, size_t blockHeight, size_t blockDepth, size_t width, size_t height, size_t depth, size_t& out)
{
  return CheckedMul4(((width + blockWidth - 1) / blockWidth),     // # of ASTC blocks along the x axis
                     ((height + blockHeight - 1) / blockHeight),  // # of ASTC blocks along the y axis
                     ((depth + blockDepth - 1) / blockDepth),     // # of ASTC blocks along the z axis
                     16,                                          // Each ASTC block size is 128 bits = 16 bytes
                     out);
}

// Returns the size of a width x height x depth image of the given VkFormat.
// Returns an error if the given image sizes are strictly invalid.
inline ErrorWithText ExportSize(size_t width, size_t height, size_t depth, VkFormat format, size_t& outSize)
{
  static const char* overflow_error_message =
      "Invalid file: One of the subresources had a size that would require more than 2^64-1 bytes of data!";
  switch(format)
  {
    case VK_FORMAT_R4G4_UNORM_PACK8:
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8_USCALED:
    case VK_FORMAT_R8_SSCALED:
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8_SRGB:
    case VK_FORMAT_S8_UINT:
      if(!CheckedMul4(width, height, depth, 8 / 8, outSize))  // 8 bits per pixel
        return overflow_error_message;
      return {};
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8_USCALED:
    case VK_FORMAT_R8G8_SSCALED:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8_SRGB:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16_USCALED:
    case VK_FORMAT_R16_SSCALED:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D16_UNORM_S8_UINT:
      if(!CheckedMul4(width, height, depth, 16 / 8, outSize))  // 16 bits per pixel
        return overflow_error_message;
      return {};
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_R8G8B8_USCALED:
    case VK_FORMAT_R8G8B8_SSCALED:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_B8G8R8_UNORM:
    case VK_FORMAT_B8G8R8_SNORM:
    case VK_FORMAT_B8G8R8_USCALED:
    case VK_FORMAT_B8G8R8_SSCALED:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8_SRGB:
      if(!CheckedMul4(width, height, depth, 24 / 8, outSize))  // 24 bits per pixel
        return overflow_error_message;
      return {};
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_R8G8B8A8_USCALED:
    case VK_FORMAT_R8G8B8A8_SSCALED:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_USCALED:
    case VK_FORMAT_B8G8R8A8_SSCALED:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16_USCALED:
    case VK_FORMAT_R16G16_SSCALED:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
      if(!CheckedMul4(width, height, depth, 32 / 8, outSize))  // 32 bits per pixel
        return overflow_error_message;
      return {};
    case VK_FORMAT_R16G16B16_UNORM:
    case VK_FORMAT_R16G16B16_SNORM:
    case VK_FORMAT_R16G16B16_USCALED:
    case VK_FORMAT_R16G16B16_SSCALED:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16_SFLOAT:
      if(!CheckedMul4(width, height, depth, 48 / 8, outSize))  // 48 bits per pixel
        return overflow_error_message;
      return {};
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_R16G16B16A16_USCALED:
    case VK_FORMAT_R16G16B16A16_SSCALED:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R64_UINT:
    case VK_FORMAT_R64_SINT:
    case VK_FORMAT_R64_SFLOAT:
    // Technically 40 or 64, but we choose the latter to make earlier special cases work:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
      if(!CheckedMul4(width, height, depth, 64 / 8, outSize))  // 64 bits per pixel
        return overflow_error_message;
      return {};
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32_SFLOAT:
      if(!CheckedMul4(width, height, depth, 96 / 8, outSize))  // 96 bits per pixel
        return overflow_error_message;
      return {};
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_R64G64_UINT:
    case VK_FORMAT_R64G64_SINT:
    case VK_FORMAT_R64G64_SFLOAT:
      if(!CheckedMul4(width, height, depth, 128 / 8, outSize))  // 128 bits per pixel
        return overflow_error_message;
      return {};
    case VK_FORMAT_R64G64B64_UINT:
    case VK_FORMAT_R64G64B64_SINT:
    case VK_FORMAT_R64G64B64_SFLOAT:
      if(!CheckedMul4(width, height, depth, 196 / 8, outSize))  // 196 bits per pixel
        return overflow_error_message;
      return {};
    case VK_FORMAT_R64G64B64A64_UINT:
    case VK_FORMAT_R64G64B64A64_SINT:
    case VK_FORMAT_R64G64B64A64_SFLOAT:
      if(!CheckedMul4(width, height, depth, 256 / 8, outSize))  // 256 bits per pixel
        return overflow_error_message;
      return {};
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
      if(!CheckedMul4((width + 3) / 4, (height + 3) / 4, depth, 8, outSize))  // 8 bytes per block
        return overflow_error_message;
      return {};
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
      if(!CheckedMul4((width + 3) / 4, (height + 3) / 4, depth, 16, outSize))  // 16 bytes per block
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
      if(!ASTCSize(4, 4, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
      if(!ASTCSize(5, 4, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
      if(!ASTCSize(5, 5, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
      if(!ASTCSize(6, 5, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
      if(!ASTCSize(6, 6, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
      if(!ASTCSize(8, 5, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
      if(!ASTCSize(8, 6, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
      if(!ASTCSize(8, 8, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
      if(!ASTCSize(10, 5, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
      if(!ASTCSize(10, 6, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
      if(!ASTCSize(10, 8, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
      if(!ASTCSize(10, 10, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
      if(!ASTCSize(12, 10, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
      if(!ASTCSize(12, 12, 1, width, height, depth, outSize))
        return overflow_error_message;
      return {};
    default:
      return "Tried to find size of unrecognized VkFormat " + std::to_string(format) + ".";
  }
}

inline ErrorWithText ExportSizeExtended(size_t width, size_t height, size_t depth, VkFormat format, size_t& outSize, CustomExportSizeFuncPtr extra_callback)
{
  const ErrorWithText builtin_result = ExportSize(width, height, depth, format, outSize);
  if(!builtin_result.has_value() || extra_callback == nullptr)
  {
    return builtin_result;
  }

  // We didn't recognize the format using the built-in ExportSize, so try the
  // provided reader.
  return extra_callback(width, height, depth, format, outSize);
}

//-----------------------------------------------------------------------------
// KTX1 READER
//-----------------------------------------------------------------------------

#define GL_TO_VK_FORMAT_CASE(internalFormat, format, type, vkFormatResult)                                             \
  if((glInternalFormat == internalFormat) && (glFormat == format) && (glType == type))                                 \
  {                                                                                                                    \
    vkFormat = vkFormatResult;                                                                                         \
    return {};                                                                                                         \
  }

#define GL_TO_VK_FORMAT_CASE_INTERNAL(internalFormat, vkFormatResult)                                                  \
  if(glInternalFormat == internalFormat)                                                                               \
  {                                                                                                                    \
    vkFormat = vkFormatResult;                                                                                         \
    return {};                                                                                                         \
  }

// GL enums, included here so that we don't have to include the OpenGL headers,
// since the include path for these may depend on the place in which this is included.
#define NVKTX_GL_BYTE 0x1400
#define NVKTX_GL_UNSIGNED_BYTE 0x1401
#define NVKTX_GL_SHORT 0x1402
#define NVKTX_GL_UNSIGNED_SHORT 0x1403
#define NVKTX_GL_INT 0x1404
#define NVKTX_GL_UNSIGNED_INT 0x1405
#define NVKTX_GL_FLOAT 0x1406
#define NVKTX_GL_HALF_FLOAT 0x140B
#define NVKTX_GL_STENCIL_INDEX 0x1901
#define NVKTX_GL_DEPTH_COMPONENT 0x1902
#define NVKTX_GL_RED 0x1903
#define NVKTX_GL_GREEN 0x1904
#define NVKTX_GL_BLUE 0x1905
#define NVKTX_GL_ALPHA 0x1906
#define NVKTX_GL_RGB 0x1907
#define NVKTX_GL_RGBA 0x1908
#define NVKTX_GL_LUMINANCE 0x1909
#define NVKTX_GL_LUMINANCE_ALPHA 0x190A
#define NVKTX_GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define NVKTX_GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define NVKTX_GL_ALPHA8 0x803C
#define NVKTX_GL_LUMINANCE8 0x8040
#define NVKTX_GL_LUMINANCE8_ALPHA8 0x8045
#define NVKTX_GL_RGB8 0x8051
#define NVKTX_GL_RGB16 0x8054
#define NVKTX_GL_RGBA4 0x8056
#define NVKTX_GL_RGB5_A1 0x8057
#define NVKTX_GL_RGBA8 0x8058
#define NVKTX_GL_RGB10_A2 0x8059
#define NVKTX_GL_RGBA16 0x805B
#define NVKTX_GL_BGR 0x80E0
#define NVKTX_GL_BGRA 0x80E1
#define NVKTX_GL_DEPTH_COMPONENT16 0x81A5
#define NVKTX_GL_DEPTH_COMPONENT24 0x81A6
#define NVKTX_GL_DEPTH_COMPONENT32 0x81A7
#define NVKTX_GL_R8 0x8229
#define NVKTX_GL_R16 0x822A
#define NVKTX_GL_RG8 0x822B
#define NVKTX_GL_RG16 0x822C
#define NVKTX_GL_R16F 0x822D
#define NVKTX_GL_R32F 0x822E
#define NVKTX_GL_RG16F 0x822F
#define NVKTX_GL_RG32F 0x8230
#define NVKTX_GL_RG 0x8227
#define NVKTX_GL_RG_INTEGER 0x8228
#define NVKTX_GL_R8 0x8229
#define NVKTX_GL_R16 0x822A
#define NVKTX_GL_RG8 0x822B
#define NVKTX_GL_RG16 0x822C
#define NVKTX_GL_R16F 0x822D
#define NVKTX_GL_R32F 0x822E
#define NVKTX_GL_RG16F 0x822F
#define NVKTX_GL_RG32F 0x8230
#define NVKTX_GL_R8I 0x8231
#define NVKTX_GL_R8UI 0x8232
#define NVKTX_GL_R16I 0x8233
#define NVKTX_GL_R16UI 0x8234
#define NVKTX_GL_R32I 0x8235
#define NVKTX_GL_R32UI 0x8236
#define NVKTX_GL_RG8I 0x8237
#define NVKTX_GL_RG8UI 0x8238
#define NVKTX_GL_RG16I 0x8239
#define NVKTX_GL_RG16UI 0x823A
#define NVKTX_GL_RG32I 0x823B
#define NVKTX_GL_RG32UI 0x823C
#define NVKTX_GL_UNSIGNED_SHORT_5_6_5 0x8363
#define NVKTX_GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#define NVKTX_GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#define NVKTX_GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#define NVKTX_GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define NVKTX_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define NVKTX_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define NVKTX_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#define NVKTX_GL_DEPTH_STENCIL 0x84F9
#define NVKTX_GL_UNSIGNED_INT_24_8 0x84FA
#define NVKTX_GL_RGBA32F 0x8814
#define NVKTX_GL_RGB32F 0x8815
#define NVKTX_GL_RGBA16F 0x881A
#define NVKTX_GL_RGB16F 0x881B
#define NVKTX_GL_DEPTH24_STENCIL8 0x88F0
#define NVKTX_GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT 0x8A54
#define NVKTX_GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT 0x8A55
#define NVKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT 0x8A56
#define NVKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT 0x8A57
#define NVKTX_GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#define NVKTX_GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#define NVKTX_GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#define NVKTX_GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#define NVKTX_GL_R11F_G11F_B10F 0x8C3A
#define NVKTX_GL_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define NVKTX_GL_RGB9_E5 0x8C3D
#define NVKTX_GL_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
#define NVKTX_GL_SRGB8 0x8C41
#define NVKTX_GL_SRGB_ALPHA 0x8C42
#define NVKTX_GL_SRGB8_ALPHA8 0x8C43
#define NVKTX_GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#define NVKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define NVKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define NVKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#define NVKTX_GL_DEPTH_COMPONENT32F 0x8CAC
#define NVKTX_GL_DEPTH32F_STENCIL8 0x8CAD
#define NVKTX_GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD
#define NVKTX_GL_STENCIL_INDEX8 0x8D48
#define NVKTX_GL_RGB565 0x8D62
#define NVKTX_GL_RGBA32UI 0x8D70
#define NVKTX_GL_RGB32UI 0x8D71
#define NVKTX_GL_RGBA16UI 0x8D76
#define NVKTX_GL_RGB16UI 0x8D77
#define NVKTX_GL_RGBA8UI 0x8D7C
#define NVKTX_GL_RGB8UI 0x8D7D
#define NVKTX_GL_RGBA32I 0x8D82
#define NVKTX_GL_RGB32I 0x8D83
#define NVKTX_GL_RGBA16I 0x8D88
#define NVKTX_GL_RGB16I 0x8D89
#define NVKTX_GL_RGBA8I 0x8D8E
#define NVKTX_GL_RGB8I 0x8D8F
#define NVKTX_GL_RED_INTEGER 0x8D94
#define NVKTX_GL_RGB_INTEGER 0x8D98
#define NVKTX_GL_RGBA_INTEGER 0x8D99
#define NVKTX_GL_BGR_INTEGER 0x8D9A
#define NVKTX_GL_BGRA_INTEGER 0x8D9B
#define NVKTX_GL_COMPRESSED_RED_RGTC1_EXT 0x8DBB
#define NVKTX_GL_COMPRESSED_SIGNED_RED_RGTC1_EXT 0x8DBC
#define NVKTX_GL_COMPRESSED_RED_GREEN_RGTC2_EXT 0x8DBD
#define NVKTX_GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT 0x8DBE
#define NVKTX_GL_COMPRESSED_RGBA_BPTC_UNORM_ARB 0x8E8C
#define NVKTX_GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB 0x8E8D
#define NVKTX_GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB 0x8E8E
#define NVKTX_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB 0x8E8F
#define NVKTX_GL_R8_SNORM 0x8F94
#define NVKTX_GL_RG8_SNORM 0x8F95
#define NVKTX_GL_RGB8_SNORM 0x8F96
#define NVKTX_GL_RGBA8_SNORM 0x8F97
#define NVKTX_GL_R16_SNORM 0x8F98
#define NVKTX_GL_RG16_SNORM 0x8F99
#define NVKTX_GL_RGB16_SNORM 0x8F9A
#define NVKTX_GL_RGBA16_SNORM 0x8F9B
#define NVKTX_GL_SR8_EXT 0x8FBD
#define NVKTX_GL_SRG8_EXT 0x8FBE
#define NVKTX_GL_RGB10_A2UI 0x906F
#define NVKTX_GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG 0x9137
#define NVKTX_GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG 0x9138
#define NVKTX_GL_COMPRESSED_R11_EAC 0x9270
#define NVKTX_GL_COMPRESSED_SIGNED_R11_EAC 0x9271
#define NVKTX_GL_COMPRESSED_RG11_EAC 0x9272
#define NVKTX_GL_COMPRESSED_SIGNED_RG11_EAC 0x9273
#define NVKTX_GL_COMPRESSED_RGB8_ETC2 0x9274
#define NVKTX_GL_COMPRESSED_SRGB8_ETC2 0x9275
#define NVKTX_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#define NVKTX_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define NVKTX_GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_4x4_KHR 0x93B0
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_5x4_KHR 0x93B1
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_5x5_KHR 0x93B2
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_6x5_KHR 0x93B3
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_6x6_KHR 0x93B4
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_8x5_KHR 0x93B5
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_8x6_KHR 0x93B6
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_8x8_KHR 0x93B7
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_10x5_KHR 0x93B8
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_10x6_KHR 0x93B9
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_10x8_KHR 0x93BA
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_10x10_KHR 0x93BB
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_12x10_KHR 0x93BC
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_12x12_KHR 0x93BD
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_3x3x3_OES 0x93C0
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_4x3x3_OES 0x93C1
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_4x4x3_OES 0x93C2
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_4x4x4_OES 0x93C3
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_5x4x4_OES 0x93C4
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_5x5x4_OES 0x93C5
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_5x5x5_OES 0x93C6
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_6x5x5_OES 0x93C7
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_6x6x5_OES 0x93C8
#define NVKTX_GL_COMPRESSED_RGBA_ASTC_6x6x6_OES 0x93C9
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR 0x93D0
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR 0x93D1
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR 0x93D2
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR 0x93D3
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR 0x93D4
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR 0x93D5
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR 0x93D6
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR 0x93D7
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR 0x93D8
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR 0x93D9
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR 0x93DA
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR 0x93DB
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR 0x93DC
#define NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR 0x93DD
#define NVKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG 0x93F0
#define NVKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG 0x93F1

// Used in the KTX1 reader to convert a KTX1 GL format to a corresponding
// Vulkan format, or to return an error if no match could be found. Based on
// Appendix A in the KTX2 Specification.
// Each of the input fields comes from the KTX1 file specification:
//   glType is the type of the values in each group, or 0 for compressed textures.
//   glFormat lists the channels per pixel, or 0 for compressed textures.
//   glInternalFormat is a compressed internal format if compressed, or a sized
// internal format if not.
//   glBaseInternalFormat lists the channels per pixel, and matches glFormat
// for uncompressed textures.
inline ErrorWithText GLToVkFormat(const uint32_t glType, const uint32_t glFormat, const uint32_t glInternalFormat, VkFormat& vkFormat)
{
  // We don't have the GL enums for some formats in glsubset.h yet - those have been commented out.
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA4, NVKTX_GL_RGBA, NVKTX_GL_UNSIGNED_SHORT_4_4_4_4, VK_FORMAT_R4G4B4A4_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA4, NVKTX_GL_BGRA, NVKTX_GL_UNSIGNED_SHORT_4_4_4_4, VK_FORMAT_B4G4R4A4_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB565, NVKTX_GL_RGB, NVKTX_GL_UNSIGNED_SHORT_5_6_5, VK_FORMAT_R5G6B5_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB565, NVKTX_GL_RGB, NVKTX_GL_UNSIGNED_SHORT_5_6_5_REV, VK_FORMAT_B5G6R5_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB5_A1, NVKTX_GL_RGBA, NVKTX_GL_UNSIGNED_SHORT_5_5_5_1, VK_FORMAT_R5G5B5A1_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB5_A1, NVKTX_GL_BGRA, NVKTX_GL_UNSIGNED_SHORT_5_5_5_1, VK_FORMAT_B5G5R5A1_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB5_A1, NVKTX_GL_BGRA, NVKTX_GL_UNSIGNED_SHORT_1_5_5_5_REV, VK_FORMAT_A1R5G5B5_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R8, NVKTX_GL_RED, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R8_SNORM, NVKTX_GL_RED, NVKTX_GL_BYTE, VK_FORMAT_R8_SNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R8UI, NVKTX_GL_RED_INTEGER, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R8I, NVKTX_GL_RED_INTEGER, NVKTX_GL_BYTE, VK_FORMAT_R8_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_SR8_EXT, NVKTX_GL_RED, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8_SRGB);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG8, NVKTX_GL_RG, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8G8_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG8_SNORM, NVKTX_GL_RG, NVKTX_GL_BYTE, VK_FORMAT_R8G8_SNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG8UI, NVKTX_GL_RG_INTEGER, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8G8_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG8I, NVKTX_GL_RG_INTEGER, NVKTX_GL_BYTE, VK_FORMAT_R8G8_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_SRG8_EXT, NVKTX_GL_RG, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8G8_SRGB);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB8, NVKTX_GL_RGB, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB8_SNORM, NVKTX_GL_RGB, NVKTX_GL_BYTE, VK_FORMAT_R8G8B8_SNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB8UI, NVKTX_GL_RGB_INTEGER, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB8I, NVKTX_GL_RGB_INTEGER, NVKTX_GL_BYTE, VK_FORMAT_R8G8B8_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_SRGB8, NVKTX_GL_RGB, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8_SRGB);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB8, NVKTX_GL_BGR, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB8_SNORM, NVKTX_GL_BGR, NVKTX_GL_BYTE, VK_FORMAT_B8G8R8_SNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB8UI, NVKTX_GL_BGR_INTEGER, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB8I, NVKTX_GL_BGR_INTEGER, NVKTX_GL_BYTE, VK_FORMAT_B8G8R8_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_SRGB8, NVKTX_GL_BGR, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8_SRGB);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA8, NVKTX_GL_RGBA, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8A8_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA8_SNORM, NVKTX_GL_RGBA, NVKTX_GL_BYTE, VK_FORMAT_R8G8B8A8_SNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA8UI, NVKTX_GL_RGBA_INTEGER, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8A8_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA8I, NVKTX_GL_RGBA_INTEGER, NVKTX_GL_BYTE, VK_FORMAT_R8G8B8A8_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_SRGB8_ALPHA8, NVKTX_GL_RGBA, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8A8_SRGB);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA8, NVKTX_GL_BGRA, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8A8_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA8_SNORM, NVKTX_GL_BGRA, NVKTX_GL_BYTE, VK_FORMAT_B8G8R8A8_SNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA8UI, NVKTX_GL_BGRA_INTEGER, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8A8_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA8I, NVKTX_GL_BGRA_INTEGER, NVKTX_GL_BYTE, VK_FORMAT_B8G8R8A8_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_SRGB8_ALPHA8, NVKTX_GL_BGRA, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8A8_SRGB);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB10_A2, NVKTX_GL_BGRA, NVKTX_GL_UNSIGNED_INT_2_10_10_10_REV, VK_FORMAT_A2R10G10B10_UNORM_PACK32);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB10_A2UI, NVKTX_GL_BGRA_INTEGER, NVKTX_GL_UNSIGNED_INT_2_10_10_10_REV,
                       VK_FORMAT_A2R10G10B10_UINT_PACK32);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB10_A2, NVKTX_GL_RGBA, NVKTX_GL_UNSIGNED_INT_2_10_10_10_REV, VK_FORMAT_A2B10G10R10_UNORM_PACK32);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB10_A2UI, NVKTX_GL_RGBA_INTEGER, NVKTX_GL_UNSIGNED_INT_2_10_10_10_REV,
                       VK_FORMAT_A2B10G10R10_UINT_PACK32);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R16, NVKTX_GL_RED, NVKTX_GL_UNSIGNED_SHORT, VK_FORMAT_R16_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R16_SNORM, NVKTX_GL_RED, NVKTX_GL_SHORT, VK_FORMAT_R16_SNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R16UI, NVKTX_GL_RED_INTEGER, NVKTX_GL_UNSIGNED_SHORT, VK_FORMAT_R16_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R16I, NVKTX_GL_RED_INTEGER, NVKTX_GL_SHORT, VK_FORMAT_R16_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R16F, NVKTX_GL_RED, NVKTX_GL_HALF_FLOAT, VK_FORMAT_R16_SFLOAT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG16, NVKTX_GL_RG, NVKTX_GL_UNSIGNED_SHORT, VK_FORMAT_R16G16_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG16_SNORM, NVKTX_GL_RG, NVKTX_GL_SHORT, VK_FORMAT_R16G16_SNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG16UI, NVKTX_GL_RG_INTEGER, NVKTX_GL_UNSIGNED_SHORT, VK_FORMAT_R16G16_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG16I, NVKTX_GL_RG_INTEGER, NVKTX_GL_SHORT, VK_FORMAT_R16G16_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG16F, NVKTX_GL_RG, NVKTX_GL_HALF_FLOAT, VK_FORMAT_R16G16_SFLOAT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB16, NVKTX_GL_RGB, NVKTX_GL_UNSIGNED_SHORT, VK_FORMAT_R16G16B16_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB16_SNORM, NVKTX_GL_RGB, NVKTX_GL_SHORT, VK_FORMAT_R16G16B16_SNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB16UI, NVKTX_GL_RGB_INTEGER, NVKTX_GL_UNSIGNED_SHORT, VK_FORMAT_R16G16B16_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB16I, NVKTX_GL_RGB_INTEGER, NVKTX_GL_SHORT, VK_FORMAT_R16G16B16_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB16F, NVKTX_GL_RGB, NVKTX_GL_HALF_FLOAT, VK_FORMAT_R16G16B16_SFLOAT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA16, NVKTX_GL_RGBA, NVKTX_GL_UNSIGNED_SHORT, VK_FORMAT_R16G16B16A16_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA16_SNORM, NVKTX_GL_RGBA, NVKTX_GL_SHORT, VK_FORMAT_R16G16B16A16_SNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA16UI, NVKTX_GL_RGBA_INTEGER, NVKTX_GL_UNSIGNED_SHORT, VK_FORMAT_R16G16B16A16_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA16I, NVKTX_GL_RGBA_INTEGER, NVKTX_GL_SHORT, VK_FORMAT_R16G16B16A16_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA16F, NVKTX_GL_RGBA, NVKTX_GL_HALF_FLOAT, VK_FORMAT_R16G16B16A16_SFLOAT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R32UI, NVKTX_GL_RED_INTEGER, NVKTX_GL_UNSIGNED_INT, VK_FORMAT_R32_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R32I, NVKTX_GL_RED_INTEGER, NVKTX_GL_INT, VK_FORMAT_R32_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R32F, NVKTX_GL_RED, NVKTX_GL_FLOAT, VK_FORMAT_R32_SFLOAT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG32UI, NVKTX_GL_RG_INTEGER, NVKTX_GL_UNSIGNED_INT, VK_FORMAT_R32G32_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG32I, NVKTX_GL_RG_INTEGER, NVKTX_GL_INT, VK_FORMAT_R32G32_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RG32F, NVKTX_GL_RG, NVKTX_GL_FLOAT, VK_FORMAT_R32G32_SFLOAT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB32UI, NVKTX_GL_RGB_INTEGER, NVKTX_GL_UNSIGNED_INT, VK_FORMAT_R32G32B32_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB32I, NVKTX_GL_RGB_INTEGER, NVKTX_GL_INT, VK_FORMAT_R32G32B32_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB32F, NVKTX_GL_RGB, NVKTX_GL_FLOAT, VK_FORMAT_R32G32B32_SFLOAT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA32UI, NVKTX_GL_RGBA_INTEGER, NVKTX_GL_UNSIGNED_INT, VK_FORMAT_R32G32B32A32_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA32I, NVKTX_GL_RGBA_INTEGER, NVKTX_GL_INT, VK_FORMAT_R32G32B32A32_SINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGBA32F, NVKTX_GL_RGBA, NVKTX_GL_FLOAT, VK_FORMAT_R32G32B32A32_SFLOAT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_R11F_G11F_B10F, NVKTX_GL_RGB, NVKTX_GL_UNSIGNED_INT_10F_11F_11F_REV, VK_FORMAT_B10G11R11_UFLOAT_PACK32);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_RGB9_E5, NVKTX_GL_RGB, NVKTX_GL_UNSIGNED_INT_5_9_9_9_REV, VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_DEPTH_COMPONENT16, NVKTX_GL_DEPTH_COMPONENT, NVKTX_GL_UNSIGNED_SHORT, VK_FORMAT_D16_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_DEPTH_COMPONENT24, NVKTX_GL_DEPTH_COMPONENT, NVKTX_GL_UNSIGNED_INT, VK_FORMAT_X8_D24_UNORM_PACK32);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_DEPTH_COMPONENT32F, NVKTX_GL_DEPTH_COMPONENT, NVKTX_GL_FLOAT, VK_FORMAT_D32_SFLOAT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_STENCIL_INDEX8, NVKTX_GL_STENCIL_INDEX, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_S8_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_DEPTH24_STENCIL8, NVKTX_GL_DEPTH_STENCIL, NVKTX_GL_UNSIGNED_INT_24_8, VK_FORMAT_D24_UNORM_S8_UINT);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_DEPTH32F_STENCIL8, NVKTX_GL_DEPTH_STENCIL, NVKTX_GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
                       VK_FORMAT_D32_SFLOAT_S8_UINT);
  // Some cases where GL_TO_VK_FORMAT_CASE is too specific.
  // Note that there are some repeated glInternalFormats in this list due to UNORM/sRGB ambiguity in OpenGL!
  // I've left it in as-is in case it's useful as a translated version of KTX2 Appendix A at some point.
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGB_S3TC_DXT1_EXT, VK_FORMAT_BC1_RGB_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, VK_FORMAT_BC1_RGB_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, VK_FORMAT_BC2_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, VK_FORMAT_BC2_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, VK_FORMAT_BC3_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, VK_FORMAT_BC3_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RED_RGTC1_EXT, VK_FORMAT_BC4_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SIGNED_RED_RGTC1_EXT, VK_FORMAT_BC4_SNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RED_GREEN_RGTC2_EXT, VK_FORMAT_BC5_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT, VK_FORMAT_BC5_SNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB, VK_FORMAT_BC6H_UFLOAT_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB, VK_FORMAT_BC6H_SFLOAT_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, VK_FORMAT_BC7_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB, VK_FORMAT_BC7_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGB8_ETC2, VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ETC2, VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA8_ETC2_EAC, VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_R11_EAC, VK_FORMAT_EAC_R11_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SIGNED_R11_EAC, VK_FORMAT_EAC_R11_SNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RG11_EAC, VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SIGNED_RG11_EAC, VK_FORMAT_EAC_R11G11_SNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_4x4_KHR, VK_FORMAT_ASTC_4x4_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR, VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_4x4_KHR, VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_5x4_KHR, VK_FORMAT_ASTC_5x4_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR, VK_FORMAT_ASTC_5x4_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_5x4_KHR, VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_5x5_KHR, VK_FORMAT_ASTC_5x5_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR, VK_FORMAT_ASTC_5x5_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_5x5_KHR, VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_6x5_KHR, VK_FORMAT_ASTC_6x5_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR, VK_FORMAT_ASTC_6x5_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_6x5_KHR, VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_6x6_KHR, VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR, VK_FORMAT_ASTC_6x6_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_6x6_KHR, VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_8x5_KHR, VK_FORMAT_ASTC_8x5_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR, VK_FORMAT_ASTC_8x5_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_8x5_KHR, VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_8x6_KHR, VK_FORMAT_ASTC_8x6_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR, VK_FORMAT_ASTC_8x6_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_8x6_KHR, VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_8x8_KHR, VK_FORMAT_ASTC_8x8_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR, VK_FORMAT_ASTC_8x8_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_8x8_KHR, VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_10x5_KHR, VK_FORMAT_ASTC_10x5_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR, VK_FORMAT_ASTC_10x5_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_10x5_KHR, VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_10x6_KHR, VK_FORMAT_ASTC_10x6_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR, VK_FORMAT_ASTC_10x6_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_10x6_KHR, VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_10x8_KHR, VK_FORMAT_ASTC_10x8_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR, VK_FORMAT_ASTC_10x8_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_10x8_KHR, VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_10x10_KHR, VK_FORMAT_ASTC_10x10_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR, VK_FORMAT_ASTC_10x10_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_10x10_KHR, VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_12x10_KHR, VK_FORMAT_ASTC_12x10_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR, VK_FORMAT_ASTC_12x10_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_12x10_KHR, VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_12x12_KHR, VK_FORMAT_ASTC_12x12_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR, VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_12x12_KHR, VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT);
  // The 3D ASTC formats in VK_EXT_extension_289 are intentionally not enabled
  // in the Vulkan XML, so they do not appear in vulkan.h. The following is
  // kept in case of future expansion.
  /*
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_3x3x3_OES, VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES, VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_3x3x3_OES, VK_FORMAT_ASTC_3x3x3_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_4x3x3_OES, VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES, VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_4x3x3_OES, VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_4x4x3_OES, VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES, VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_4x4x3_OES, VK_FORMAT_ASTC_4x4x3_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_4x4x4_OES, VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES, VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_4x4x4_OES, VK_FORMAT_ASTC_4x4x4_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_5x4x4_OES, VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES, VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_5x4x4_OES, VK_FORMAT_ASTC_5x4x4_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_5x5x4_OES, VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES, VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_5x5x4_OES, VK_FORMAT_ASTC_5x5x4_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_5x5x5_OES, VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES, VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_5x5x5_OES, VK_FORMAT_ASTC_5x5x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_6x5x5_OES, VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES, VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_6x5x5_OES, VK_FORMAT_ASTC_6x5x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_6x6x5_OES, VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES, VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_6x6x5_OES, VK_FORMAT_ASTC_6x6x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_6x6x6_OES, VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES, VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_ASTC_6x6x6_OES, VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT);
  */
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG, VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG, VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG, VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT, VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT, VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG, VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(NVKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG, VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG);

  // Extra cases from the KTX1 tests not in Appendix A.
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_ALPHA8, NVKTX_GL_ALPHA, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_LUMINANCE8, NVKTX_GL_LUMINANCE, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8_UNORM);
  GL_TO_VK_FORMAT_CASE(NVKTX_GL_LUMINANCE8_ALPHA8, NVKTX_GL_LUMINANCE_ALPHA, NVKTX_GL_UNSIGNED_BYTE, VK_FORMAT_R8G8_UNORM);

  std::stringstream str;
  str << "Unable to find corresponding VkFormat for glType " << glType << ", glFormat " << glFormat
      << ", and glInternalFormat " << glInternalFormat << ".";
  return str.str();
}

// Used in the KTX1 reader to determine the default transfer function for a
// VkFormat, since KTX1 doesn't have the Data Format Descriptor.
inline bool IsKTX1FormatSRGBByDefault(VkFormat format)
{
  switch(format)
  {
      // Formats that are definitely always sRGB
    case VK_FORMAT_R8_SRGB:
    case VK_FORMAT_R8G8_SRGB:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_B8G8R8_SRGB:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
    case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
    case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
    case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
    case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
      return true;
      // Formats that are definitely always linear
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_R32G32B32_SFLOAT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
      // According to toktx, otherwise the correct choice is to interpret it as linear:
    default:
      return false;
  }
}

// The values from UInt32 endianness through UInt32 bytesOfKeyValueData in the KTX1 header.
struct KTX1TopLevelHeader
{
  uint32_t endianness;
  uint32_t glType;
  uint32_t glTypeSize;
  uint32_t glFormat;
  uint32_t glInternalFormat;
  uint32_t glBaseInternalFormat;
  uint32_t pixelWidth;
  uint32_t pixelHeight;
  uint32_t pixelDepth;
  uint32_t numberOfArrayElements;
  uint32_t numberOfFaces;
  uint32_t numberOfMipmapLevels;
  uint32_t bytesOfKeyValueData;
};

// Reads a KTX 1.0 file, *starting after the 12-byte identifier*.
inline ErrorWithText KTXImage::readFromKTX1Stream(std::istream& input, const ReadSettings& readSettings)
{
  size_t validation_input_size = 0;
  if(readSettings.validate_input_size)
  {
    const std::streampos initial_pos = input.tellg();
    input.seekg(0, std::ios_base::end);
    const std::streampos end_pos = input.tellg();
    validation_input_size        = end_pos - initial_pos + 12;
    input.seekg(initial_pos, std::ios_base::beg);
  }

  KTX1TopLevelHeader header{};
  READ_OR_ERROR(input, header, "Failed to read KTX1 header.");
  // Determine if the file needs to be swapped from big-endian to little-endian.
  // (We assume the machine is little-endian as we use CUDA.)
  // If this is true, we need to swap every UInt32 in the KTX1 File Structure
  // as well as each element in uncompressed texture data.
  bool needsSwapEndian = false;
  if(header.endianness == 0x01020304u)
  {
    needsSwapEndian = true;
  }
  else if(header.endianness != 0x04030201u)
  {
    std::stringstream str;
    str << "KXT1 endianness (0x" << std::hex << header.endianness << ") did not match either big-endian or little-endian formats.";
    return str.str();
  }

  if(needsSwapEndian)
  {
    SwapEndian32<KTX1TopLevelHeader>(header);
  }

  // Set the dimensions in the structure to indicate the size and type of the
  // texture. Then replace some fields with 1 if they were 0 to make the rest
  // of the importer less complex.
  mip_0_width           = header.pixelWidth;
  mip_0_height          = header.pixelHeight;
  mip_0_depth           = header.pixelDepth;
  num_layers_possibly_0 = header.numberOfArrayElements;
  num_faces             = header.numberOfFaces;

  app_should_generate_mips = (header.numberOfMipmapLevels == 0);
  if(app_should_generate_mips)
  {
    header.numberOfMipmapLevels = 1;
  }
  // Because KTX1 files store mips from largest to smallest (as opposed to KTX2),
  // we can handle readSettings.mips by truncating num_mips.
  if(!readSettings.mips)
  {
    header.numberOfMipmapLevels = 1;
  }
  num_mips = (readSettings.mips ? header.numberOfMipmapLevels : 1);
  // Keep track of the special case where we have padding with non-array cubemap textures:
  const bool isArray = (header.numberOfArrayElements != 0);
  if(!isArray)
    header.numberOfArrayElements = 1;
  if(header.pixelDepth == 0)
    header.pixelDepth = 1;
  if(header.pixelHeight == 0)
    header.pixelHeight = 1;

  // Validate pixel width
  if(header.pixelWidth == 0)
  {
    return "KTX1 image had a width of 0 pixels!";
  }
  // Validate number of faces
  if(header.numberOfFaces == 0)
  {
    return "KTX1 image had no faces!";
  }
  if(header.numberOfFaces > 6)
  {
    return "KTX1 image had too many faces!";
  }
  // The maximum image dimensions are 2^32-1 by 2^32-1, so there can be at most
  // 31 mips.
  if(header.numberOfMipmapLevels > 31)
  {
    return "KTX1 image had more than 31 mips!";
  }
  if(readSettings.validate_input_size)
  {
    size_t validation_num_subresources = 0;
    if(!GetNumSubresources(num_mips, num_layers_possibly_0, num_faces, validation_num_subresources))
    {
      return "Computing the number of mips times layers times faces in the file overflowed!";
    }
    if(validation_num_subresources > validation_input_size)
    {
      return "The KTX1 input had a likely invalid header - it listed " + std::to_string(num_mips) + " mips (or 0), "
             + std::to_string(num_layers_possibly_0) + " layers (or 0), and " + std::to_string(num_faces)
             + " faces - but the input was only " + std::to_string(validation_input_size) + " bytes long!";
    }
    if(header.bytesOfKeyValueData > validation_input_size)
    {
      return "The KTX1 input had an invalid header - it listed " + std::to_string(header.bytesOfKeyValueData)
             + " bytes of key/value data, but the input was only " + std::to_string(validation_input_size) + " bytes long!";
    }
  }

  //---------------------------------------------------------------------------
  // Read key-value data.
  UNWRAP_ERROR(ReadKeyValueData(input, header.bytesOfKeyValueData, needsSwapEndian, key_value_data));

  // KTX1 doesn't have ktxSwizzle, so:
  swizzle = {KTX_SWIZZLE::R, KTX_SWIZZLE::G, KTX_SWIZZLE::B, KTX_SWIZZLE::A};

  //---------------------------------------------------------------------------
  // Calculate formats and fields used for decompression.

  // Determine a corresponding VkFormat for the given GL format.
  format = VK_FORMAT_UNDEFINED;
  UNWRAP_ERROR(GLToVkFormat(header.glType, header.glFormat, header.glInternalFormat, format));

  // Guess if this format is sRGB.
  is_srgb = IsKTX1FormatSRGBByDefault(format);

  // Always set this to false, since DXT2 and DXT4 aren't supported in
  // EXT_texture_compression_s3tc.
  is_premultiplied = false;

  // Allocate table of subresources.
  UNWRAP_ERROR(allocate(num_mips, num_layers_possibly_0, num_faces));

  for(uint32_t mip = 0; mip < header.numberOfMipmapLevels; mip++)
  {
    // Read the image size. We use this for mip padding later on, and rely on
    // ExportSize for individual subresources.
    uint32_t imageSize = 0;
    READ_OR_ERROR(input, imageSize, "Failed to read KTX1 imageSize for mip 0.");
    if(needsSwapEndian)
    {
      SwapEndian32(imageSize);
    }

    const size_t mipWidth  = std::max(1u, header.pixelWidth >> mip);
    const size_t mipHeight = std::max(1u, header.pixelHeight >> mip);
    const size_t mipDepth  = std::max(1u, header.pixelDepth >> mip);

    // Compute the size of a face in bytes
    size_t faceSizeBytes = 0;
    UNWRAP_ERROR(ExportSizeExtended(mipWidth, mipHeight, mipDepth, format, faceSizeBytes, readSettings.custom_size_callback));
    // Validate it
    if(readSettings.validate_input_size)
    {
      if(((validation_input_size / size_t(header.numberOfArrayElements)) / size_t(header.numberOfFaces)) < faceSizeBytes)
      {
        return "The KTX1 file said it contained " + std::to_string(header.numberOfArrayElements)
               + " array elements and " + std::to_string(header.numberOfFaces) + " faces in mip " + std::to_string(mip)
               + ", but the input was too short (" + std::to_string(validation_input_size) + " bytes) to contain that!";
      }
    }

    for(uint32_t array_element = 0; array_element < header.numberOfArrayElements; array_element++)
    {
      for(uint32_t face = 0; face < header.numberOfFaces; face++)
      {
        // Allocate storage for the encoded face
        std::vector<char>& faceBuffer = subresource(mip, array_element, face);
        ErrorWithText      maybeError = ResizeVectorOrError(faceBuffer, faceSizeBytes);
        if(maybeError.has_value())
        {
          return "Allocating encoded data for mip " + std::to_string(mip) + " layer " + std::to_string(array_element)
                 + " face " + std::to_string(face) + " failed (probably out of memory).";
        }

        if(!input.read(faceBuffer.data(), faceSizeBytes))
        {
          return "Reading mip " + std::to_string(mip) + " layer " + std::to_string(array_element) + " face "
                 + std::to_string(face) + " failed (is the file truncated)?";
        }

        // Apply endianness swapping
        if(needsSwapEndian)
        {
          SwapEndianGeneral(faceBuffer.size(), faceBuffer.data(), header.glTypeSize);
        }

        // Handle cubePadding
        if((!isArray) && (header.numberOfFaces == 6))
        {
          // faceSizeBytes mod 4: 0 1 2 3
          // cubePadding        : 0 3 2 1
          const std::streamoff cubePaddingBytes = 3 - ((static_cast<std::streamoff>(faceSizeBytes) + 3) % 4);
          input.seekg(cubePaddingBytes, std::ios_base::cur);
        }
      }
    }
    // Handle mip padding. The spec says that this is always 3 - ((imageSize + 3)%4) bytes,
    // and we assume bytes are the same size as chars.
    {
      const std::streamoff mipPaddingBytes = 3 - ((static_cast<std::streamoff>(imageSize) + 3) % 4);
      input.seekg(mipPaddingBytes, std::ios_base::cur);
    }
  }

  return {};
}


//-----------------------------------------------------------------------------
// KTX2 READER + WRITER
//-----------------------------------------------------------------------------

// KDF_DF_MODEL_UASTC (not KHR_DF) is only defined in the KTX2 spec at the
// moment per https://github.com/KhronosGroup/KTX-Specification/issues/119,
// and has value 166.
const uint32_t KDF_DF_MODEL_UASTC = 166;
// Similarly, these channel types are only described in the KTX2 spec at the moment.
// We include an enum for quick combination lookup.
const uint32_t KHR_DF_CHANNEL_ETC1S_RGB = 0;
const uint32_t KHR_DF_CHANNEL_ETC1S_RRR = 3;
const uint32_t KHR_DF_CHANNEL_ETC1S_GGG = 4;
const uint32_t KHR_DF_CHANNEL_ETC1S_AAA = 15;
// Describes the four valid combinations of ETC1S slices in the KTX2
// specification. These are listed in the order they appear there.
enum class ETC1SCombination
{
  RGB,   // One slice, RGB
  RGBA,  // Two slices, RGB + AAA
  R,     // One slice, RRR
  RG     // Two slices, RRR + GGG
};
// Also specific to Basis ETC1S+BasisLZ - Basis includes support for texture
// array animation encoding using P-frames and I-frames, and it marks which
// frames are which using flags in its table of image descriptions.
// This is the "isPFrame" flag in the KTX2 specification.
const uint32_t KTX2_IMAGE_IS_P_FRAME = 2;

// KTX 2 Level Index structure (Section 3.9.7 "Level Index")
struct LevelIndex
{
  uint64_t byteOffset;
  uint64_t byteLength;
  uint64_t uncompressedByteLength;
};
static_assert(sizeof(LevelIndex) == 3 * sizeof(uint64_t), "LevelIndex size must match KTX2 spec!");

#ifdef NVP_SUPPORTS_ZSTD
// A Zstandard decompression context that is automatically freed when it goes out of scope.
struct ScopedZstdDContext
{
  ScopedZstdDContext() {}
  ~ScopedZstdDContext() { Free(); }
  inline void Init()
  {
    Free();
    pCtx = ZSTD_createDCtx();
  }
  inline void Free()
  {
    if(pCtx != nullptr)
    {
      ZSTD_freeDCtx(pCtx);
      pCtx = nullptr;
    }
  }
  ZSTD_DCtx* pCtx = nullptr;
};

// A Zstandard compression context that is automatically freed when it goes out of scope.
struct ScopedZstdCContext
{
  ScopedZstdCContext() {}
  ~ScopedZstdCContext() { Free(); }
  inline void Init()
  {
    Free();
    pCtx = ZSTD_createCCtx();
  }
  inline void Free()
  {
    if(pCtx != nullptr)
    {
      ZSTD_freeCCtx(pCtx);
      pCtx = nullptr;
    }
  }
  ZSTD_CCtx* pCtx = nullptr;
};
#endif

#ifdef NVP_SUPPORTS_GZLIB
// A Zlib inflation stream that is automatically deinitialized when it goes out of scope.
struct ScopedZlibDStream
{
  ScopedZlibDStream() {}
  ~ScopedZlibDStream() { Free(); }
  inline int Init()
  {
    Free();
    stream.zalloc   = Z_NULL;
    stream.zfree    = Z_NULL;
    stream.opaque   = Z_NULL;
    stream.avail_in = 0;
    stream.next_in  = Z_NULL;
    return inflateInit(&stream);
  }
  inline void Free() { inflateEnd(&stream); }
  z_stream    stream{};
};
#endif

#ifdef NVP_SUPPORTS_BASISU

// Stores data per-image that is required to transcode a BasisLZ+ETC1S image.
struct BasisLZDecompressionObjects
{
  // Stores and knows how to decode the BasisLZ + ETC1S stream
  basist::basisu_lowlevel_etc1s_transcoder* etc1sTranscoder = nullptr;
  // Additional information from the global data block not included in the transcoder
  std::vector<basist::ktx2_etc1s_image_desc> etc1sImageDescs;
  basist::ktx2_transcoder_state              ktx2TranscoderState;

  ~BasisLZDecompressionObjects()
  {
    if(etc1sTranscoder)
    {
      delete etc1sTranscoder;
      etc1sTranscoder = nullptr;
    }
  }
};

// Basis Universal makes use of some global data, and prints developer error
// messages if we attempt to initialize this more than once.
// This Meyers singleton keeps track of said global data.
struct BasisUSingleton
{
  static BasisUSingleton& GetInstance()
  {
    static BasisUSingleton s;
    return s;
  }

  // No copying
  BasisUSingleton(const BasisUSingleton&) = delete;
  BasisUSingleton& operator=(const BasisUSingleton&) = delete;

  void TranscodeUASTCToBC7OrASTC44(char* output, const char* inData, size_t width, size_t height, size_t depth, bool to_astc)
  {
    if(!Initialize())
      return;

    const basist::uastc_block* buf        = reinterpret_cast<const basist::uastc_block*>(inData);
    const size_t               numBlocksX = (width + 3) / 4;
    const size_t               numBlocksY = (height + 3) / 4;
    const size_t               numBlocksZ = (depth + 3) / 4;
    const size_t               numBlocks  = numBlocksX * numBlocksY * numBlocksZ;
    if(numBlocks > INT64_MAX)
      return;  // Won't fit in an OpenMP range

    const int64_t numBlocksI = static_cast<int64_t>(numBlocks);
    if(to_astc)
    {
#if defined(_OPENMP)
#pragma omp parallel for
#endif
      for(int64_t blockIdx = 0; blockIdx < numBlocksI; blockIdx++)
      {
        const basist::uastc_block& block = buf[blockIdx];
        char*                      dst   = output + size_t(blockIdx) * 16;  // 16 bytes per ASTC block
        basist::transcode_uastc_to_astc(block, dst);
      }
    }
    else
    {
      // To BC7
#if defined(_OPENMP)
#pragma omp parallel for
#endif
      for(int64_t blockIdx = 0; blockIdx < numBlocksI; blockIdx++)
      {
        const basist::uastc_block& block = buf[blockIdx];
        char*                      dst   = output + size_t(blockIdx) * 16;  // 16 bytes per BC7 block
        basist::transcode_uastc_to_bc7(block, dst);
      }
    }
  }

  ErrorWithText PrepareBasisLZObjects(BasisLZDecompressionObjects& outObjects,
                                      const std::vector<uint8_t>&  ktxSGD,
                                      const uint32_t               numMips,
                                      const uint32_t               numLayers,
                                      const uint32_t               numFaces)
  {
    if(!Initialize())
    {
      return "Initializing BasisU failed!";
    }
    // For reference, see Basis Universal's ktx2_transcoder::decompress_etc1s_global_data().

    // Compute the image count, erroring if we would overflow:
    uint32_t imageCount = numMips;
    {
      if(numMips == 0 || numLayers == 0 || numFaces == 0)
      {
        return "The number of mips, layers, or faces was 0!";
      }
      if(imageCount >= (UINT_MAX / numLayers))
      {
        return "The number of images was over 2^32-1!";
      }
      imageCount *= numLayers;
      if(imageCount >= (UINT_MAX / numFaces))
      {
        return "The number of images was over 2^32-1!";
      }
      imageCount *= numFaces;
    }

    if(ktxSGD.size() < sizeof(basist::ktx2_etc1s_global_data_header))
    {
      return "The data included ETC1S+BasisLZ compression, but the length of the supercompression global data was too "
             "short to contain the required information!";
    }

    size_t offsetInSGD = 0;

    basist::ktx2_etc1s_global_data_header etc1sHeader;
    memcpy(&etc1sHeader, ktxSGD.data() + offsetInSGD, sizeof(etc1sHeader));
    offsetInSGD += sizeof(etc1sHeader);

    // Check the ETC1S header.
    if((!etc1sHeader.m_endpoints_byte_length) || (!etc1sHeader.m_selectors_byte_length) || (!etc1sHeader.m_tables_byte_length))
    {
      return "The data included ETC1S+BasisLZ compression, but the supercompression global data had invalid byte "
             "length "
             "properties!";
    }

    if((!etc1sHeader.m_endpoint_count) || (!etc1sHeader.m_selector_count))
    {
      return "The data included ETC1S+BasisLZ compression, but the endpoint or selector count is 0, which is invalid!";
    }

    if((sizeof(basist::ktx2_etc1s_global_data_header) + sizeof(basist::ktx2_etc1s_image_desc) * imageCount + etc1sHeader.m_endpoints_byte_length
        + etc1sHeader.m_selectors_byte_length + etc1sHeader.m_tables_byte_length + etc1sHeader.m_extended_byte_length)
       > ktxSGD.size())
    {
      return "The data included ETC1S+BasisLZ compression, but the supercompression global data was invalid: it was "
             "too "
             "small to contain the data its header said it contains!";
    }

    // Read the image descriptions
    UNWRAP_ERROR(ResizeVectorOrError(outObjects.etc1sImageDescs, imageCount));
    memcpy(outObjects.etc1sImageDescs.data(), ktxSGD.data() + offsetInSGD, sizeof(basist::ktx2_etc1s_image_desc) * imageCount);
    offsetInSGD += sizeof(basist::ktx2_etc1s_image_desc) * imageCount;

    // We'll verify that slice byte lengths are nonzero later.

    // Initialize the ETC1S transcoder.
    if(outObjects.etc1sTranscoder)
      delete outObjects.etc1sTranscoder;
    outObjects.etc1sTranscoder = new basist::basisu_lowlevel_etc1s_transcoder();
    outObjects.etc1sTranscoder->clear();

    // Decode tables and palettes.
    const uint8_t* endpointData = ktxSGD.data() + offsetInSGD;
    const uint8_t* selectorData = endpointData + uint32_t(etc1sHeader.m_endpoints_byte_length);
    const uint8_t* tablesData   = selectorData + uint32_t(etc1sHeader.m_selectors_byte_length);

    if(!outObjects.etc1sTranscoder->decode_tables(tablesData, etc1sHeader.m_tables_byte_length))
    {
      return "Failed to decode tables from the BasisLZ supercompression data";
    }

    if(!outObjects.etc1sTranscoder->decode_palettes(etc1sHeader.m_endpoint_count, endpointData, etc1sHeader.m_endpoints_byte_length,  //
                                                    etc1sHeader.m_selector_count, selectorData, etc1sHeader.m_selectors_byte_length))
    {
      return "Failed to decode palettes from the BasisLZ supercompression data";
    }

    outObjects.ktx2TranscoderState.clear();

    return {};
  }

  // Attempts to initialize the Basis encoder and decoder, and returns true if that
  // succeeded or if it was already initialized (this can be called
  // multiple times)
  bool Initialize()
  {
    if(!m_initialized.load())
    {
      std::lock_guard<std::mutex> lock(m_modificationMutex);
      basist::basisu_transcoder_init();
      basisu::basisu_encoder_init();
      m_initialized.store(true);
    }
    return true;
  }

private:
  BasisUSingleton(){};
  // Frees objects if loaded
  ~BasisUSingleton()
  {
    std::lock_guard<std::mutex> lock(m_modificationMutex);
    m_initialized.store(false);
  }

private:
  std::mutex        m_modificationMutex;
  std::atomic<bool> m_initialized = false;
};
#endif

#pragma pack(push, 1)
struct KTX2TopLevelHeader
{
  VkFormat vkFormat;
  uint32_t typeSize;
  uint32_t pixelWidth;
  uint32_t pixelHeight;
  uint32_t pixelDepth;
  uint32_t layerCount;  // Num array elements
  uint32_t faceCount;
  uint32_t levelCount;  // Num mips
  uint32_t supercompressionScheme;

  // Index (1)
  uint32_t dfdByteOffset;
  uint32_t dfdByteLength;
  uint32_t kvdByteOffset;
  uint32_t kvdByteLength;
  uint64_t sgdByteOffset;
  uint64_t sgdByteLength;
};
#pragma pack(pop)

static_assert(sizeof(VkFormat) == sizeof(uint32_t), "VkFormat size must match KTX2 spec!");
static_assert(sizeof(KTX2TopLevelHeader) == 68, "KTX2 top-level header size must match spec! Padding issue?");

// Reads a KTX 2.0 file, *starting after the 12-byte identifier*.
inline ErrorWithText KTXImage::readFromKTX2Stream(std::istream& input, const ReadSettings& readSettings)
{
  // Get the position of the start of the file in the stream so that we can add
  // padding correctly later.
  const std::streampos start_pos = input.tellg() - std::streampos(12);  // Since we start after the identifier
  size_t               validation_input_size = 0;
  if(readSettings.validate_input_size)
  {
    const std::streampos initial_pos = input.tellg();
    input.seekg(0, std::ios_base::end);
    const std::streampos end_pos = input.tellg();
    validation_input_size        = end_pos - initial_pos + 12;
    input.seekg(initial_pos, std::ios_base::beg);
  }

  //---------------------------------------------------------------------------
  // Read sections 0 and 1 of the file structure.
  KTX2TopLevelHeader header{};
  READ_OR_ERROR(input, header, "Failed to read KTX2 header and section 1.");

  // Copy the dimensions into the structure so that we can determine the
  // texture type later.
  format = header.vkFormat;  // This is the inflated VkFormat; we may swap this out during supercompression due to transcoding.
  mip_0_width           = header.pixelWidth;
  mip_0_height          = header.pixelHeight;
  mip_0_depth           = header.pixelDepth;
  num_layers_possibly_0 = header.layerCount;
  // num_faces cannot be 0, on the other hand, so we set it below!

  // If the image width is 0, we can't read it (and the file is invalid).
  if(header.pixelWidth == 0)
  {
    return "KTX2 image width was 0 (i.e. the file contains no pixels).";
  }

  // These dimensions are 0 only to indicate the type of the texture
  // (see section 4.1). To make the rest of the reader simpler and to avoid
  // errors later on, we can change each of these to 1 if they're 0.
  if(header.pixelHeight == 0)
    header.pixelHeight = 1;
  if(header.pixelDepth == 0)
    header.pixelDepth = 1;
  if(header.layerCount == 0)
    header.layerCount = 1;
  if(header.faceCount == 0)
    header.faceCount = 1;
  // KTX files also only use levelCount == 0 to indicate that loaders should
  // generate other levels if needed (section 3.7 "levelCount"). Since the user
  // ultimately controls this and we always only load the first mip, we change
  // a 0 to a 1 here as well.
  // We have a special case where we need max(1, the original number of levels)
  // for Basis ETC1S unpacking.
  const uint32_t original_num_mips_max_1 = std::max(1u, header.levelCount);
  app_should_generate_mips               = (header.levelCount == 0);
  if(app_should_generate_mips)
  {
    header.levelCount = 1;
  }
  num_mips = (readSettings.mips ? static_cast<int>(header.levelCount) : 1);

  num_faces = header.faceCount;

  // Validate the data format descriptor byte length. KDF 1.3 assumes the Data
  // Format Descriptor works as a series of 32-bit words, and there's language
  // in the KTX2 spec that reinforces this (although it may not quite match
  // the Basic Structure).
  if((header.dfdByteLength % 4) != 0)
  {
    return "KTX2 Data Format Descriptor byte length was not a multiple of 4, so is not valid.";
  }

  // Validate the level count because we allocate memory based off it. It can't
  // be larger than 31 - if it were, then pixelWidth and pixelHeight wouldn't
  // fit in UInt32 types.
  if(original_num_mips_max_1 > 31)
  {
    std::stringstream str;
    str << "KTX2 levelCount was too large (" << original_num_mips_max_1 << ") - the "
        << "maximum number of mips possible in a KTX2 file is 31.";
    return str.str();
  }
  if(readSettings.validate_input_size)
  {
    size_t validation_num_subresources = 0;
    if(!GetNumSubresources(original_num_mips_max_1, num_layers_possibly_0, num_faces, validation_num_subresources))
    {
      return "Computing the number of mips times layers times faces in the file overflowed!";
    }
    if(validation_num_subresources > validation_input_size)
    {
      return "The KTX1 input had a likely invalid header - it listed " + std::to_string(original_num_mips_max_1)
             + " mips (or 0), " + std::to_string(num_layers_possibly_0) + " layers (or 0), and " + std::to_string(num_faces)
             + " faces - but the input was only " + std::to_string(validation_input_size) + " bytes long!";
    }
    if(header.dfdByteLength > validation_input_size)
    {
      return "The KTX2 input had an invalid header - it said its Data Format Descriptor was " + std::to_string(header.dfdByteLength)
             + " bytes long, but the input was only " + std::to_string(validation_input_size) + " bytes long!";
    }
    if(header.kvdByteLength > validation_input_size)
    {
      return "The KTX2 input had an invalid header - it listed " + std::to_string(header.kvdByteLength)
             + " bytes of key/value data, but the input was only " + std::to_string(validation_input_size) + " bytes long!";
    }
  }

  //---------------------------------------------------------------------------
  // Load the level indices (section 2)
  std::vector<LevelIndex> levelIndices(original_num_mips_max_1);
  if(!input.read(reinterpret_cast<char*>(levelIndices.data()), sizeof(LevelIndex) * original_num_mips_max_1))
  {
    return "Unable to read Level Index from KTX2 file.";
  }

  // Check the size of the level indices against the developer-supplied limits
  for(size_t i = 0; i < levelIndices.size(); i++)
  {
    if(levelIndices[i].uncompressedByteLength > readSettings.max_resource_size_in_bytes)
    {
      return "Mip level " + std::to_string(i) + "'s uncompressedByteLength ("
        + std::to_string(levelIndices[i].uncompressedByteLength)
        + ") was larger than the max image size in bytes per mip specified in "
        "the KTX reader's ReadSettings ("
        + std::to_string(readSettings.max_resource_size_in_bytes)
        + "). This file is either invalid, or if this size is intended, "
        "ReadSettings::max_resource_size_in_bytes should be set to a larger value.";
    }
  }

  //---------------------------------------------------------------------------
  // Load the Data Format Descriptor. We read this as a uint32_t array and then
  // interpret it later.
  // We also start counting the number of bytes read here so that we can handle
  // the align(8) sgdPadding correctly.
  std::vector<uint32_t> dfd;
  UNWRAP_ERROR(ResizeVectorOrError(dfd, header.dfdByteLength / 4));
  if(!input.read(reinterpret_cast<char*>(dfd.data()), header.dfdByteLength))
  {
    return "Unable to read Data Format Descriptor from KTX2 file.";
  }

  // Get some information from the Basic Data Format Descriptor.
  uint32_t                  dfdTotalSize = 0;
  BasicDataFormatDescriptor basicDFD{};
  std::vector<DFSample>     dfdSamples;
  bool                      basicDFDExists = false;
  if(header.dfdByteLength >= sizeof(dfdTotalSize) + sizeof(basicDFD))
  {
    dfdTotalSize = dfd[0];
    if(dfdTotalSize != header.dfdByteLength)
    {
      return "KTX2 Data Format Descriptor was invalid (data format descriptor's dfdTotalSize didn't match the index's "
             "dfdByteLength)";
    }

    memcpy(&basicDFD, &dfd[1], sizeof(BasicDataFormatDescriptor));
    basicDFDExists = true;

    // Attempt to read the sample data from the Data Format Descriptor.
    if(size_t(basicDFD.descriptorBlockSize) + sizeof(dfdTotalSize) > header.dfdByteLength)
    {
      return "KTX2 Data Format Descriptor was invalid (the basic data format descriptor descriptorBlockSize was "
             + std::to_string(basicDFD.descriptorBlockSize)
             + " - that plus 4 bytes for totalSize was larger than the length of the whole data format descriptor, "
             + std::to_string(dfdTotalSize) + ")";
    }
    if(size_t(basicDFD.descriptorBlockSize) < sizeof(BasicDataFormatDescriptor))
    {
      return "KTX2 Data Format Descriptor was invalid (the basic data format descriptor descriptorBlockSize was "
             + std::to_string(basicDFD.descriptorBlockSize) + ", which was shorter than the size of the basic DFD itself, "
             + std::to_string(sizeof(BasicDataFormatDescriptor)) + ")";
    }

    const size_t numDFDSamples = (size_t(basicDFD.descriptorBlockSize) - sizeof(BasicDataFormatDescriptor)) / sizeof(DFSample);
    UNWRAP_ERROR(ResizeVectorOrError(dfdSamples, numDFDSamples));
    // If numDFDSamples is 0, dfdSamples.data() can be nullptr, and passing nullptr to memcpy() is undefined behavior.
    if(numDFDSamples != 0)
    {
      memcpy(dfdSamples.data(), reinterpret_cast<char*>(dfd.data()) + sizeof(dfdTotalSize) + sizeof(BasicDataFormatDescriptor),
             numDFDSamples * sizeof(DFSample));
    }
  }


  uint32_t khrDfPrimaries              = KHR_DF_PRIMARIES_SRGB;
  is_premultiplied                     = false;
  is_srgb                              = true;
  size_t           basisETC1SNumSlices = 1;  // Basis ETC1S can have 1 or two slices (which occurs in RGBA and R+G)
  ETC1SCombination basisETC1SCombo{};
#ifdef NVP_SUPPORTS_BASISU
  basist::transcoder_texture_format basisDstFmt = basist::transcoder_texture_format::cTFBC7_RGBA;  // Same as the inflated VkFormat but in an enum Basis uses
#endif
  if(basicDFDExists)
  {
    if((basicDFD.flags & KHR_DF_FLAG_ALPHA_PREMULTIPLIED) != 0)
    {
      is_premultiplied = true;
    }

    if(basicDFD.transferFunction == KHR_DF_TRANSFER_SRGB)
    {
      is_srgb = true;
    }
    else if(basicDFD.transferFunction == KHR_DF_TRANSFER_LINEAR)
    {
      is_srgb = false;
    }
    else
    {
      return "KTX2 Data Format Descriptor had an unhandled transferFunction (" + std::to_string(basicDFD.transferFunction) + ")";
    }

    if(header.vkFormat == VK_FORMAT_UNDEFINED)
    {
      if(basicDFD.colorModel == KDF_DF_MODEL_UASTC)
      {
#ifdef NVP_SUPPORTS_BASISU
        input_supercompression = InputSupercompression::eBasisUASTC;
        if(readSettings.device_supports_astc)
        {
          // Prefer ASTC, since then transcoding is lossless:
          format = is_srgb ? VK_FORMAT_ASTC_4x4_SRGB_BLOCK : VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        }
        else
        {
          // Otherwise, BC7 is preferred:
          format = is_srgb ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;
        }
#else
        return "KTX2 color model was Basis UASTC, but NVP_SUPPORTS_BASISU was not defined.";
#endif
      }
      else if(basicDFD.colorModel == KHR_DF_MODEL_ETC1S)
      {
#ifdef NVP_SUPPORTS_BASISU
        input_supercompression = InputSupercompression::eBasisETC1S;
        // There are four ETC1S channel possibilities. The final format is
        // BC4 for RRR, BC5 for RRR+GGG, and BC7 for RGB and RGB+AAA.
        basisETC1SNumSlices = dfdSamples.size();
        if(dfdSamples.size() == 1)
        {
          // Must be RRR or RGB
          if(dfdSamples[0].channelType == KHR_DF_CHANNEL_ETC1S_RRR)
          {
            basisETC1SCombo = ETC1SCombination::R;
            basisDstFmt     = basist::transcoder_texture_format::cTFBC4;
            format          = VK_FORMAT_BC4_UNORM_BLOCK;
          }
          else if(dfdSamples[0].channelType == KHR_DF_CHANNEL_ETC1S_RGB)
          {
            basisETC1SCombo = ETC1SCombination::RGB;
            basisDstFmt     = basist::transcoder_texture_format::cTFBC7_RGBA;
            format          = is_srgb ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;
          }
          else
          {
            return "KTX2 color model was Basis ETC1S, but there was one slice of an unknown channel type ("
                   + std::to_string(dfdSamples[0].channelType) + ")";
          }
        }
        else if(dfdSamples.size() == 2)
        {
          // Must be RRR+GGG or RGB+AAA; we disallow listing these channels
          // in a different order.
          if(dfdSamples[0].channelType == KHR_DF_CHANNEL_ETC1S_RRR && dfdSamples[1].channelType == KHR_DF_CHANNEL_ETC1S_GGG)
          {
            basisETC1SCombo = ETC1SCombination::RG;
            basisDstFmt     = basist::transcoder_texture_format::cTFBC5;
            format          = VK_FORMAT_BC5_UNORM_BLOCK;
          }
          else if(dfdSamples[0].channelType == KHR_DF_CHANNEL_ETC1S_RGB && dfdSamples[1].channelType == KHR_DF_CHANNEL_ETC1S_AAA)
          {
            basisETC1SCombo = ETC1SCombination::RGBA;
            basisDstFmt     = basist::transcoder_texture_format::cTFBC7_RGBA;
            format          = is_srgb ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;
          }
          else
          {
            return "KTX2 color model was Basis ETC1S and there were two slices, but the channel types ("
                   + std::to_string(dfdSamples[0].channelType) + " and " + std::to_string(dfdSamples[1].channelType) + " were unknown";
          }
        }
        else
        {
          return "KTX2 color model was Basis ETC1S, but there were an unusual number of slices ("
                 + std::to_string(dfdSamples.size()) + ", should be 1 or 2)";
        }
#else
        return "KTX2 color model was Basis ETC1S, but NVP_SUPPORTS_BASISU was not defined.";
#endif
      }
      else
      {
        return "KTX2 VkFormat was VK_FORMAT_UNDEFINED, but the Data Format Descriptor block had unrecognized "
               "colorModel number "
               + std::to_string(basicDFD.colorModel) + ".";
      }
    }
  }

  // Perform additional validation to rule out invalid Basis+format+supercompression
  // combinations. Not doing these checks can lead to surprising behavior!
  // Basis ETC1S must only appear with supercompression mode 1, and vice versa.
  if((input_supercompression == InputSupercompression::eBasisETC1S) != (header.supercompressionScheme == 1))
  {
    return "KTX2 file was invalid - the Basis ETC1S flag didn't match whether supercompression scheme 1 (BasisLZ) was used.";
  }

  //---------------------------------------------------------------------------
  // Read section 4, key/value data. To do this, we can read kvdByteLength
  // bytes, then extract keys and values from that.
  UNWRAP_ERROR(ReadKeyValueData(input, header.kvdByteLength, false, key_value_data));

  // Parse the ktxSwizzle value if it exists.
  {
    swizzle          = {KTX_SWIZZLE::R, KTX_SWIZZLE::G, KTX_SWIZZLE::B, KTX_SWIZZLE::A};
    const auto kvpIt = key_value_data.find("KTXswizzle");
    if(kvpIt != key_value_data.end())
    {
      // Read up to 4 characters (slightly less constrained than the spec)
      const std::vector<char> value = kvpIt->second;
      // Value should end with a NULL character, but if it doesn't that's OK
      const size_t charsToRead = std::min(size_t(4), value.size());
      for(size_t i = 0; i < charsToRead; i++)
      {
        switch(value[i])
        {
          case 'r':
            swizzle[i] = KTX_SWIZZLE::R;
            break;
          case 'g':
            swizzle[i] = KTX_SWIZZLE::G;
            break;
          case 'b':
            swizzle[i] = KTX_SWIZZLE::B;
            break;
          case 'a':
            swizzle[i] = KTX_SWIZZLE::A;
            break;
          case '0':
            swizzle[i] = KTX_SWIZZLE::ZERO;
            break;
          case '1':
            swizzle[i] = KTX_SWIZZLE::ONE;
            break;
          default:
            break;
        }
      }
    }
  }

  //---------------------------------------------------------------------------
  // The align(8) sgdPadding.
  if(header.sgdByteLength > 0)
  {
    std::streampos bytesReadMod8 = ((input.tellg() - start_pos) % 8);
    if(bytesReadMod8 > 0)
    {
      input.seekg(bytesReadMod8, std::ios::cur);
    }
  }

  // Section 6, supercompression global data.
  // First check the sgdByteLength because we allocate memory based off it.
  // Values that are really large are allowed by the KTX2 spec, but someone could
  // use this to cause an out-of-memory error.
  if(header.sgdByteLength > readSettings.max_resource_size_in_bytes)
  {
    return "Supercompression global data length (sgdByteLength) was over the maximum size specified in the "
           "ReadSettings object! The file is either invalid, or if this is intentional, "
           "ReadSettings::max_resource_size_in_bytes should be set to a larger value.";
  }
  std::vector<uint8_t> supercompressionGlobalData;
  UNWRAP_ERROR(ResizeVectorOrError(supercompressionGlobalData, header.sgdByteLength));
  if(header.sgdByteLength > 0)
  {
    if(!input.read(reinterpret_cast<char*>(supercompressionGlobalData.data()), header.sgdByteLength))
    {
      return "Reading supercompressionGlobalData failed.";
    }
  }

// Initialize supercompression
#ifdef NVP_SUPPORTS_ZSTD
  ScopedZstdDContext zstdDCtx;
#endif
#ifdef NVP_SUPPORTS_BASISU
  BasisLZDecompressionObjects basisLZDCtx;
  // Basis ETC1S supports a sort of video format, where there are I-frames
  // and P-frames and frames correspond to array elements. The Basis code
  // currently allows this if there's a KTXanimData key, or if there are
  // P-frames indicated in the supercompression image descriptions.
  // We diverge slightly from Basis here and require videos to be 2D; Basis
  // technically allows cubemap arrays with KTXanimData set to be interpreted
  // as videos, I think.
  bool isVideo = false;
#endif
  if(header.supercompressionScheme == 1)
  {
#ifdef NVP_SUPPORTS_BASISU
    // Initialize supercompression global data
    UNWRAP_ERROR(BasisUSingleton::GetInstance().PrepareBasisLZObjects(basisLZDCtx, supercompressionGlobalData, original_num_mips_max_1,
                                                                      std::max(1u, num_layers_possibly_0), num_faces));
    // Video criterion; don't permit 1-frame videos following Basis here
    if(num_faces == 1 && num_layers_possibly_0 > 1)
    {
      isVideo = (key_value_data.find("KTXanimData") != key_value_data.end());
      if(!isVideo)
      {
        for(const basist::ktx2_etc1s_image_desc& id : basisLZDCtx.etc1sImageDescs)
        {
          if(id.m_image_flags & KTX2_IMAGE_IS_P_FRAME)
          {
            isVideo = true;
            break;
          }
        }
      }
    }

    // Check validity of slice sizes
    for(const basist::ktx2_etc1s_image_desc& id : basisLZDCtx.etc1sImageDescs)
    {
      if(id.m_rgb_slice_byte_length == 0)
      {
        return "KTX2 stream was incorrectly formatted: a BasisLZ+ETC1S RGB slice had byte length 0.";
      }
      if((basisETC1SNumSlices == 2) && (id.m_alpha_slice_byte_length == 0))
      {
        return "KTX2 stream was incorrectly formatted: a BasisLZ+ETC1S alpha slice had byte length 0.";
      }
    }
#else
    return "KTX2 header specified BasisLZ supercompression, but NVP_SUPPORTS_BASISU was not defined.";
#endif
  }
  else if(header.supercompressionScheme == 2)
  {
    // Set up Zstandard
#ifdef NVP_SUPPORTS_ZSTD
    zstdDCtx.Init();
    if(zstdDCtx.pCtx == nullptr)
    {
      return "Initializing Zstandard context failed.";
    }
#else
    return "KTX2 stream uses Zstandard supercompression, but nv_ktx was built without Zstd.";
#endif
  }
  else if(header.supercompressionScheme == 3)
  {
// Nothing to do for Zlib, but check to ensure it's supported
#ifndef NVP_SUPPORTS_GZLIB
    return "KTX2 stream uses Zlib supercompression, but nv_ktx was built without Zlib.";
#endif
  }
  else if(header.supercompressionScheme > 3)
  {
    return "Does not know about supercompression scheme " + std::to_string(header.supercompressionScheme) + ".";
  }

  //---------------------------------------------------------------------------
  // Section 7, Mip Level Array.
  // Read, inflate, and decompress each image in turn.
  //
  // For each mip:
  //   If uncompressed:
  //     Copy each subresource
  //   Else if Zstd:
  //     Zstd decompress the mip data
  //     Copy each subresource
  //   Else if Zlib:
  //     Zlib decompress the mip data
  //     Copy each subresource
  //   Else if Basis ETC1S+BasisLZ:
  //     BasisLZ decompress the mip data to 1 or 2 ETC1S slices.
  //     For each subresource
  //       Transcode it from ETC1S to the inflated VkFormat
  //
  //   Then: if UASTC:
  //     For each subresource
  //       Transcode it from UASTC to the inflated VkFormat

  // First initialize the output:
  UNWRAP_ERROR(allocate(num_mips, num_layers_possibly_0, num_faces));
  // Temporary buffers used for supercompressed data.
  std::vector<char> supercompressedData;
  std::vector<char> inflatedData;
  // Traverse mips in reverse order following the spec
  for(int mip = num_mips - 1; mip >= 0; mip--)
  {
    // Seek to the start of that mip's data and read it. Note that this skips
    // over mipPadding.
    const LevelIndex& levelIndex = levelIndices[mip];
    if(!input.seekg(levelIndex.byteOffset + start_pos, std::ios::beg))
    {
      return "Failed to seek to KTX2 mip " + std::to_string(mip) + " data!";
    }

    const size_t mipWidth  = std::max(1u, header.pixelWidth >> mip);
    const size_t mipHeight = std::max(1u, header.pixelHeight >> mip);
    const size_t mipDepth  = std::max(1u, header.pixelDepth >> mip);

    // The size of each face in the inflated vkFormat, after inflation.
    size_t inflatedFaceSize = 0;
    if(header.vkFormat == VK_FORMAT_UNDEFINED)
    {
      // Check for the Basis UASTC and Universal cases. I don't know if ETC1S
      // or UASTC supports depth.
      if(input_supercompression == InputSupercompression::eBasisUASTC)
      {
        if(!CheckedMul4((mipWidth + 3) / 4, (mipHeight + 3) / 4, mipDepth, 16, inflatedFaceSize))  // 16 bytes per 4x4 block
          return "Invalid KTX2 file: A subresource had size " + std::to_string(mipWidth) + " x " + std::to_string(mipHeight)
                 + " x " + std::to_string(mipDepth) + ", which would require more than 2^64 - 1 bytes to store decompressed.";
      }
      else if(input_supercompression == InputSupercompression::eBasisETC1S)
      {
        if(!CheckedMul5((mipWidth + 3) / 4, (mipHeight + 3) / 4, mipDepth, basisETC1SNumSlices, 8, inflatedFaceSize))  // 8 bytes per 4x4 block per slice
          return "Invalid KTX2 file: A subresource had size " + std::to_string(mipWidth) + " x " + std::to_string(mipHeight)
                 + " x " + std::to_string(mipDepth) + " with " + std::to_string(basisETC1SNumSlices)
                 + " slices, which would require more than 2^64 - 1 bytes to store decompressed.";
      }
      else
      {
        return "Tried to find the size of an undefined VkFormat, with an unknown Khronos Data Format color model. Is "
               "this a new texture format?";
      }
    }
    else
    {
      UNWRAP_ERROR(ExportSizeExtended(mipWidth, mipHeight, mipDepth, header.vkFormat, inflatedFaceSize, readSettings.custom_size_callback));
    }

    // The size of each face in the final vkFormat, after inflation and transcoding.
    size_t finalFaceSize = 0;
    UNWRAP_ERROR(ExportSizeExtended(mipWidth, mipHeight, mipDepth, format, finalFaceSize, readSettings.custom_size_callback));

    if(finalFaceSize > readSettings.max_resource_size_in_bytes)
    {
      return "Subresource was too large! The KTX2 file said that each mip 0 face had dimensions "
             + std::to_string(mipWidth) + " x " + std::to_string(mipHeight) + " x " + std::to_string(mipDepth)
             + " with format " + std::to_string(format) + ". This has a computed size of "
             + std::to_string(finalFaceSize) + " bytes, which is larger than the limit of max_resource_size_in_bytes = "
             + std::to_string(readSettings.max_resource_size_in_bytes) + " bytes.";
    }

    // Validate sizes
    if(readSettings.validate_input_size)
    {
      // Level-wide constraint on read data
      if(levelIndex.byteLength > validation_input_size)
      {
        return "The KTX2 file said that level " + std::to_string(mip) + " contained " + std::to_string(levelIndex.byteLength)
               + " bytes of supercompressed data, but the file was only " + std::to_string(validation_input_size) + " bytes long!";
      }

      if(header.supercompressionScheme == 0)
      {
        // Level-wide constraint on read data
        if(levelIndex.uncompressedByteLength > validation_input_size)
        {
          return "The KTX2 file said no supercompression was used and that level " + std::to_string(mip) + " contained "
                 + std::to_string(levelIndex.uncompressedByteLength) + " bytes of data, but the file was only "
                 + std::to_string(validation_input_size) + " bytes long!";
        }

        // Per-face more specific constraint, making use of how non-supercompressed
        // UASTC and ASTC (the transcoded-to format) are both 128 bits/block.
        if((validation_input_size / size_t(header.layerCount)) / size_t(header.faceCount) < finalFaceSize)
        {
          return "The KTX2 file said it contained " + std::to_string(header.layerCount) + " array elements and "
                 + std::to_string(header.faceCount) + " faces in mip " + std::to_string(mip)
                 + ", but the input was too short (" + std::to_string(validation_input_size) + " bytes) to contain that!";
        }
      }
    }

    // FAST PATH - if no supercompression and no UASTC, we can read directly:
    if((header.supercompressionScheme == 0) && (input_supercompression != InputSupercompression::eBasisUASTC))
    {
      for(uint32_t layer = 0; layer < header.layerCount; layer++)
      {
        for(uint32_t face = 0; face < header.faceCount; face++)
        {
          std::vector<char>& subresource_data = subresource(mip, layer, face);
          UNWRAP_ERROR(ResizeVectorOrError(subresource_data, finalFaceSize));
          if(!input.read(subresource_data.data(), finalFaceSize))
          {
            return "Reading data for mip " + std::to_string(mip) + " layer " + std::to_string(layer) + " face "
                   + std::to_string(face) + " from the stream failed. Is the stream truncated?";
          }
        }
      }
    }
    else
    {
      //               decompression     transcoding
      // supercompressedData -> inflatedData -> subresource
      //                             ^
      //                             |
      //                             in the ETC1S + UASTC case, we load file data into here directly
      //                             (it turns out ETC1S doesn't do anything per-level)
      if(header.supercompressionScheme == 0)
      {
        // UASTC, ETC1S: Load file data into inflatedData directly
        UNWRAP_ERROR(ResizeVectorOrError(inflatedData, levelIndex.uncompressedByteLength));
        if(!input.read(inflatedData.data(), levelIndex.uncompressedByteLength))
        {
          return "Reading mip " + std::to_string(mip) + "'s data failed.";
        }
      }
      else if(header.supercompressionScheme == 1)
      {
        // ETC1S files often have uncompressedByteLength set to 0 for some reason.
        // In any case, we want to read the compressed byte length.
        UNWRAP_ERROR(ResizeVectorOrError(inflatedData, levelIndex.byteLength));
        if(!input.read(inflatedData.data(), levelIndex.byteLength))
        {
          return "Reading mip " + std::to_string(mip) + "'s data failed.";
        }
      }
      else
      {
        // Read into supercompressedData
        UNWRAP_ERROR(ResizeVectorOrError(supercompressedData, levelIndex.byteLength));
        if(!input.read(supercompressedData.data(), levelIndex.byteLength))
        {
          return "Reading mip " + std::to_string(mip) + "'s supercompressed data failed.";
        }

        // Inflate the supercompressed data. We must use another buffer for this.
        UNWRAP_ERROR(ResizeVectorOrError(inflatedData, levelIndex.uncompressedByteLength));

        if(header.supercompressionScheme == 2)
        {
          // Zstandard
#ifdef NVP_SUPPORTS_ZSTD
          size_t zstdError = ZSTD_decompress(inflatedData.data(), inflatedData.size(),  //
                                             supercompressedData.data(), supercompressedData.size());
          if(ZSTD_isError(zstdError))
          {
            const char* zstdErrorName = ZSTD_getErrorName(zstdError);
            return "Mip " + std::to_string(mip) + " Zstandard inflation failed with the message '"
                   + std::string(zstdErrorName) + "' (code " + std::to_string(zstdError) + ").";
          }
#else
          assert(!"nv_ktx was compiled without Zstandard support, but the KTX stream was not rejected! This should never happen.");
#endif
        }
        else if(header.supercompressionScheme == 3)
        {
          // Zlib
#ifdef NVP_SUPPORTS_GZLIB
          ScopedZlibDStream zlibStream;
          int               zlibError = zlibStream.Init();
          if(zlibError != Z_OK)
          {
            return "Zlib initialization failed (error code " + std::to_string(zlibError) + ").";
          }
          if(supercompressedData.size() > UINT_MAX || inflatedData.size() > UINT_MAX)
          {
            return "Zlib compressed or decompressed data for mip " + std::to_string(mip) + " was larger than 4 GB.";
          }
          zlibStream.stream.next_in   = reinterpret_cast<Bytef*>(supercompressedData.data());
          zlibStream.stream.avail_in  = static_cast<uInt>(supercompressedData.size());
          zlibStream.stream.next_out  = reinterpret_cast<Bytef*>(inflatedData.data());
          zlibStream.stream.avail_out = static_cast<uInt>(inflatedData.size());
          zlibError                   = inflate(&zlibStream.stream, Z_NO_FLUSH);
          if(zlibError != Z_OK)
          {
            return "Zlib inflation failed (error code " + std::to_string(zlibError) + ").";
          }
          zlibStream.Free();
#else
          assert(!"nv_ktx was compiled without Zlib support, but the KTX stream was not rejected! This should never happen.");
#endif
        }
      }

      // Check size ahead of time to ensure we don't read out of bounds.
      // This would otherwise result in an access violation on
      // invalid_face_count_and_padding.ktx2, or on an otherwise truncated file.
      // This doesn't apply to ETC1S, because it does inflation and transcoding
      // all at once.
      if(header.supercompressionScheme != 1)
      {
        const size_t inflatedDataSize           = inflatedData.size();
        const size_t expected_bytes_in_this_mip = inflatedFaceSize * size_t(header.layerCount) * size_t(header.faceCount);
        if(expected_bytes_in_this_mip > inflatedDataSize)
        {
          return "Expected " + std::to_string(expected_bytes_in_this_mip) + " bytes in mip " + std::to_string(mip)
                 + ", but the inflated data was only " + std::to_string(inflatedDataSize) + " bytes long.";
        }
      }

      // Write into each subresource, possibly transcoding from the source
      // format to this->format (for UASTC and ETC1S)
      size_t inflatedDataPos = 0;  // Read position in inflatedData
      for(uint32_t layer = 0; layer < header.layerCount; layer++)
      {
        for(uint32_t face = 0; face < header.faceCount; face++)
        {
          std::vector<char>& subresource_data = subresource(mip, layer, face);
          UNWRAP_ERROR(ResizeVectorOrError(subresource_data, finalFaceSize));

          if(input_supercompression == InputSupercompression::eBasisUASTC)
          {
#ifdef NVP_SUPPORTS_BASISU
            BasisUSingleton::GetInstance().TranscodeUASTCToBC7OrASTC44(subresource_data.data(),
                                                                       &inflatedData[inflatedDataPos], mipWidth, mipHeight,
                                                                       mipDepth, readSettings.device_supports_astc);
#else
            assert(!"nv_ktx was compiled without Basis support, but the KTX stream was not rejected! This should never happen.");
#endif
          }
          else if(input_supercompression == InputSupercompression::eBasisETC1S)
          {
#ifdef NVP_SUPPORTS_BASISU
            // Get the ETC1S image description
            const size_t etc1sImageIdx =
                (std::max(1u, num_layers_possibly_0) * size_t(mip) + size_t(layer)) * size_t(num_faces) + size_t(face);
            const basist::ktx2_etc1s_image_desc imageDesc  = basisLZDCtx.etc1sImageDescs[etc1sImageIdx];
            const size_t                        numBlocksX = (mipWidth + 3) / 4;
            const size_t                        numBlocksY = (mipHeight + 3) / 4;

            if(!basisLZDCtx.etc1sTranscoder->transcode_image(
                   basisDstFmt,                                      // Basis destination format
                   subresource_data.data(),                          // Output data
                   uint32_t(numBlocksX * numBlocksY),                // Number of blocks in the output
                   reinterpret_cast<uint8_t*>(inflatedData.data()),  // Compressed data for this level
                   uint32_t(inflatedData.size()),                    // Compressed data length
                   uint32_t(numBlocksX), uint32_t(numBlocksY),       // Block dimensions
                   uint32_t(mipWidth), uint32_t(mipHeight),          // Pixel dimensions
                   uint32_t(mip),                                    // Mip number
                   imageDesc.m_rgb_slice_byte_offset, imageDesc.m_rgb_slice_byte_length,  // Range of first slice from the start of the compressed data
                   imageDesc.m_alpha_slice_byte_offset, imageDesc.m_alpha_slice_byte_length,  // Range of second slice from the start of the compressed data
                   0,                                                    // No need for nonstandard decoder flags here
                   (basisETC1SNumSlices == 2),                           // Whether it has 2 slices or only 1
                   isVideo,                                              // Whether this is ETC1S video
                   0,                                                    // Output row pitch in blocks, or 0
                   &basisLZDCtx.ktx2TranscoderState.m_transcoder_state,  // Persistent transcoder state
                   false))                                               // Output in blocks, not pixels
            {
              return "Failed to decompress BasisLZ+ETC1S mip " + std::to_string(mip) + ", layer "
                     + std::to_string(layer) + ", face " + std::to_string(face) + "!";
            }
#else
            assert(!"nv_ktx was compiled without Basis support, but the KTX stream was not rejected! This should never happen.");
#endif
          }
          else
          {
            // Not UASTC or ETC1S, no transcoding needed
            // We've checked to make sure this is okay above, but double-check
            // here in case the behavior above changes in future versions of
            // the code.
            if(header.supercompressionScheme == 1)
            {
              return "Failed to read KTX2 file: BasisLZ supercompression was enabled, but control reached the non-BasisLZ copy. This should never happen.";
            }
            if(inflatedDataPos + inflatedFaceSize > inflatedData.size())
            {
              return "Failed to read KTX2 file: the size of the inflated data didn't match the expected size.";
            }
            memcpy(subresource_data.data(), &inflatedData[inflatedDataPos], inflatedFaceSize);
          }

          // Advance to next image
          inflatedDataPos += inflatedFaceSize;
        }
      }
    }
  }

  return {};
}

//-----------------------------------------------------------------------------
// Writing functions

// Sets the Data Format Descriptor information for ASTC LDR formats given the
// size of the block. Doesn't change the transfer function and flags.
inline void SetASTCFlags(uint8_t xsize, uint8_t ysize, BasicDataFormatDescriptor& descriptor, std::vector<DFSample>& samples)
{
  descriptor.colorModel     = KHR_DF_MODEL_ASTC;
  descriptor.colorPrimaries = KHR_DF_PRIMARIES_BT709;
  // Don't override transferFunction and flags.
  assert(xsize > 0 && ysize > 0);
  descriptor.texelBlockDimension0 = xsize - 1;
  descriptor.texelBlockDimension1 = ysize - 1;
  descriptor.bytesPlane0          = 16;

  samples.resize(1);
  samples[0].bitLength   = 127;
  samples[0].channelType = KHR_DF_CHANNEL_ASTC_DATA;
  samples[0].lower       = 0;
  samples[0].upper       = UINT32_MAX;
}

template <class T>
inline size_t SizeofVector(const std::vector<T>& vec)
{
  return vec.size() * sizeof(T);
}

inline std::vector<char> StringToCharVector(const std::string& str)
{
  const char*  cString        = str.c_str();
  const size_t strlenWithZero = str.size() + 1;
  return std::vector<char>(cString, cString + strlenWithZero);
}

// Returns the least common multiple of n and 4.
inline size_t LCM4(size_t n)
{
  if(n % 4 == 0)
  {
    return n;
  }
  else if(n % 2 == 0)
  {
    return n * 2;
  }
  else
  {
    return n * 4;
  }
}


inline ErrorWithText KTXImage::writeKTX2Stream(std::ostream& output, const WriteSettings& writeSettings)
{
  // This function is more difficult than DDS writing, since the header
  // contains offsets into the rest of the file.
  // The way this works is that we'll initially write a blank identifier and
  // top-level header, then write the rest of the file keeping track of the
  // different offsets. Then we'll rewind back to the start and write the
  // header and offsets.

  // For Basis formats, we actually use the Basis Universal KTX2 writer entirely.
  // The reason is that basisu::basis_compressor doesn't have a level of
  // abstraction between "compress a block of data" and
  // "write a .basis or .ktx2 file" - the functions to do so are declared as
  // private. KTX-Software gets around this by writing to a .basis stream
  // in-memory, then reading the .basis stream and getting the needed data,
  // which takes a few hundred lines of code. It feels safest and least cursed
  // to me to rely on the built-in Basis implementation here.

  //---------------------------------------------------------------------------
  // Common things for both writers.
  // Some dimension fields can be 0 to indicate the texture type; create copies
  // of them where these 0s have been changed to 1s for indexing.
  if(num_mips < 1)
  {
    return "Failed to write KTX2 file: num_mips (" + std::to_string(num_mips) + ") was less than 1.";
  }
  const uint32_t num_layers_or_1 = std::max(num_layers_possibly_0, 1u);

  // First, apply modifications to the KTXswizzle information early so that
  // they're handled by both writers.
  {
    std::stringstream ktxSwizzle;
    for(int c = 0; c < 4; c++)
    {
      switch(swizzle[c])
      {
        case KTX_SWIZZLE::R:
          ktxSwizzle << "r";
          break;
        case KTX_SWIZZLE::G:
          ktxSwizzle << "g";
          break;
        case KTX_SWIZZLE::B:
          ktxSwizzle << "b";
          break;
        case KTX_SWIZZLE::A:
          ktxSwizzle << "a";
          break;
        case KTX_SWIZZLE::ZERO:
          ktxSwizzle << "0";
          break;
        case KTX_SWIZZLE::ONE:
          ktxSwizzle << "1";
          break;
        default:
          assert(!"Unknown KTX_SWIZZLE!");
          return "Internal error: Unknown KTX_SWIZZLE.";
          break;
      }
    }
    key_value_data["KTXswizzle"] = StringToCharVector(ktxSwizzle.str());
  }

  if(writeSettings.encode_rgba8_to_format != EncodeRGBA8ToFormat::NO)
  {
#ifndef NVP_SUPPORTS_BASISU
    return "Failed to write KTX2 file: encoding to a Basis format was specified, but NVP_SUPPORTS_BASISU was "
           "not defined.";
#else
    // Validate format
    if((format != VK_FORMAT_B8G8R8A8_SRGB) && (format != VK_FORMAT_B8G8R8A8_UNORM))
    {
      return "Failed to write KTX2 file: encoding RGBA8 data to a Basis format was specified, but the Vulkan format of "
             "the input data, "
             + std::to_string(format) + " wasn't VK_FORMAT_B8G8R8A8_UNORM or VK_FORMAT_B8G8R8A8_SRGB.";
    }

    // Basisu doesn't support volume textures fully yet, I think
    if(mip_0_depth > 1)
    {
      return "Failed to write KTX2 file: Volumes with Basis compression aren't supported yet.";
    }

    BasisUSingleton::GetInstance().Initialize();
    basisu::basis_compressor_params params;
    params.m_perceptual = is_srgb;
    params.m_ktx2_srgb_transfer_func = is_srgb;
    params.m_mip_gen = false;
    params.m_read_source_images = false;
    params.m_write_output_basis_files = false;
    params.m_status_output = false;
    params.m_debug = false;
    params.m_validate_etc1s = false;
    params.m_compression_level = writeSettings.etc1s_encoding_level;
    params.m_check_for_alpha = false;
    params.m_multithreading = true;
    params.m_create_ktx2_file = true;
    params.m_quality_level = 128;

    // Determine the texture type
    if((mip_0_depth == 0) || (mip_0_depth == 1))  // Avoid volumes for now
    {
      // [2D or cubemap] *
      if(num_faces == 6)
      {
        // Cubemap
        params.m_tex_type = basist::cBASISTexTypeCubemapArray;
      }
      else
      {
        if(num_layers_possibly_0 == 0)
        {
          // 2D non-array
          params.m_tex_type = basist::cBASISTexType2D;
        }
        else
        {
          // 2D array
          params.m_tex_type = basist::cBASISTexType2DArray;
        }
      }
    }
    else
    {
      // Volumes; reject cubemaps
      if(num_faces == 6)
      {
        return "Cubemaps where each face is a volume are not supported in KTX2 according to section 4.1, Texture Type.";
      }
      else
      {
        params.m_tex_type = basist::cBASISTexTypeVolume;
      }
    }

    // UASTC vs. ETC1S
    if(writeSettings.encode_rgba8_to_format == EncodeRGBA8ToFormat::UASTC)
    {
      params.m_uastc = true;
      params.m_pack_uastc_flags = static_cast<uint32_t>(writeSettings.uastc_encoding_quality);
      params.m_force_alpha = true;
      if(writeSettings.supercompression == WriteSupercompressionType::ZSTD)
      {
        params.m_rdo_uastc = true;
        params.m_rdo_uastc_quality_scalar = writeSettings.rdo_lambda;
        params.m_rdo_uastc_multithreading = true;
        params.m_ktx2_uastc_supercompression = basist::ktx2_supercompression::KTX2_SS_ZSTANDARD;
        params.m_ktx2_zstd_supercompression_level = writeSettings.supercompression_level;
      }
    }
    else
    {
      params.m_force_alpha = (writeSettings.encode_rgba8_to_format == EncodeRGBA8ToFormat::ETC1S_RGBA);
      params.m_quality_level = std::max(0, std::min((writeSettings.etc1s_encoding_level * 255) / 6, 255));
      //params.m_global_sel_pal = true; // Enabling this seems to make things very slow
      params.m_uastc = false;
      // KTX-Software currently sets these to true if the input is a normal map.
      params.m_no_endpoint_rdo = !writeSettings.rdo_etc1s;
      params.m_no_selector_rdo = !writeSettings.rdo_etc1s;
      if(!writeSettings.rdo_etc1s)
      {
        params.m_compression_level = 0;
      }
    }

    // Create a job pool for multithreading
    basisu::job_pool job_pool(std::thread::hardware_concurrency());
    params.m_pJob_pool = &job_pool;

    // Copy key/value data, except for KTXwriter, since basisu will make its
    // own key for that.
    for(const auto& kvp : key_value_data)
    {
      if(kvp.first == "KTXwriter")
        continue;
      params.m_ktx2_key_values.push_back(basist::ktx2_transcoder::key_value());
      basist::ktx2_transcoder::key_value& outKVP = params.m_ktx2_key_values.back();
      if((kvp.first.size() > (UINT32_MAX - 1)) || (kvp.second.size() > UINT32_MAX))
      {
        return "A key/value pair was too large for Basis' KTX2 writer.";
      }
      outKVP.m_key.append(reinterpret_cast<const uint8_t*>(kvp.first.data()), static_cast<uint32_t>(kvp.first.size()));
      outKVP.m_key.push_back(0);
      outKVP.m_value.append(reinterpret_cast<const uint8_t*>(kvp.second.data()), static_cast<uint32_t>(kvp.second.size()));
    }

    // Copy image data. I believe these are in KTX2 order.
    params.m_source_images.reserve(size_t(num_layers_or_1) * size_t(num_faces));
    params.m_source_mipmap_images.reserve(size_t(num_layers_or_1) * size_t(num_faces));
    for(uint32_t layer = 0; layer < num_layers_or_1; layer++)
    {
      for(uint32_t face = 0; face < num_faces; face++)
      {
        if(num_mips > 1)
        {
          params.m_source_mipmap_images.push_back(basisu::vector<basisu::image>());
          params.m_source_mipmap_images.back().reserve(size_t(num_mips) - 1);
        }
        for(uint32_t mip = 0; mip < num_mips; mip++)
        {
          std::vector<char>& this_subresource = subresource(mip, layer, face);
          const uint32_t width = std::max(1u, mip_0_width >> mip);
          const uint32_t height = std::max(1u, mip_0_height >> mip);
          const size_t widthS = static_cast<size_t>(width);
          const size_t heightS = static_cast<size_t>(height);
          // Mip 0 images go in m_source_images, while higher mips go in m_source_mipmap_images.
          if(mip == 0)
          {
            params.m_source_images.push_back(basisu::image(width, height));
          }
          else
          {
            params.m_source_mipmap_images.back().push_back(basisu::image(width, height));
          }
          basisu::image& out_image = (mip == 0 ? params.m_source_images.back() : params.m_source_mipmap_images.back().back());

          for(size_t y = 0; y < heightS; y++)
          {
            for(size_t x = 0; x < widthS; x++)
            {
              // Workaround for an issue where basisu ignores m_check_for_alpha set to false if user-supplied mips are provided
              const uint8_t a = params.m_force_alpha ? static_cast<uint8_t>(this_subresource[(widthS * y + x) * 4 + 3]) : 255;
              out_image(uint32_t(x), uint32_t(y))
                  .set(static_cast<uint8_t>(this_subresource[(widthS * y + x) * 4 + 2]),  // R from B
                       static_cast<uint8_t>(this_subresource[(widthS * y + x) * 4 + 1]),  // G from G
                       static_cast<uint8_t>(this_subresource[(widthS * y + x) * 4 + 0]),  // B from R
                       a);                                                                // A from A
            }
          }
        }
      }
    }

    // Create the KTX2 data!
    basisu::basis_compressor basis_compressor;
    // basisu::enable_debug_printf(true); // Uncomment this to print out status messages

    if(!basis_compressor.init(params))
    {
      return "Failed to initialize Basis Universal compressor.";
    }

    const basisu::basis_compressor::error_code basis_result = basis_compressor.process();
    switch(basis_result)
    {
      case basisu::basis_compressor::cECSuccess:
        break;
      case basisu::basis_compressor::cECFailedReadingSourceImages:
        return "Basis Universal compressor failed to read source images.";
      case basisu::basis_compressor::cECFailedValidating:
        return "Basis Universal compressor input failed validation (most likely an error in the Texture Tools "
               "Exporter).";
      case basisu::basis_compressor::cECFailedEncodeUASTC:
        return "Basis Universal compressor failed to encode to UASTC.";
      case basisu::basis_compressor::cECFailedFrontEnd:
        return "Basis Universal compressor failed in the ETC1S frontend.";
      case basisu::basis_compressor::cECFailedFontendExtract:
        return "Basis Universal compressor failed to extract data from the ETC1S frontend.";
      case basisu::basis_compressor::cECFailedBackend:
        return "Basis Universal compressor failed during BasisLZ compression.";
      case basisu::basis_compressor::cECFailedCreateBasisFile:
        return "Basis Universal compressor failed when creating Basis-formatted output.";
      case basisu::basis_compressor::cECFailedWritingOutput:
        return "Basis Universal compressor failed when writing the output file.";
      case basisu::basis_compressor::cECFailedUASTCRDOPostProcess:
        return "Basis Universal compressor failed when performing the UASTC Rate-Distortion Optimization post-process.";
      case basisu::basis_compressor::cECFailedCreateKTX2File:
        return "Basis Universal compressor to create a KTX2 file.";
    }

    // Write it out to the stream!
    if(!output.write(reinterpret_cast<const char*>(basis_compressor.get_output_ktx2_file().data()),
                     basis_compressor.get_output_ktx2_file().size()))
    {
      return "Basis Universal compressor succeeded, but the I/O operation of writing the compressed data to a stream "
             "failed! Is the file in use or the location requires administrator permissions?";
    }

    return {};
#endif
  }

  //---------------------------------------------------------------------------
  // Normal KTX2 writing
  const std::streampos start_pos = output.tellp();

  // Allocate the header and Level Index.
  KTX2TopLevelHeader      header{};
  std::vector<LevelIndex> levelIndex(num_mips);  // "mip offsets"

  // Write the header (we'll write it again), then zeros up to the Data Format Descriptor.
  if(!output.write(reinterpret_cast<const char*>(ktx2Identifier), IDENTIFIER_LEN))
  {
    return "Failed to write identifier the first time. Is the stream writable?";
  }
  if(!output.write(reinterpret_cast<char*>(&header), sizeof(header)))
  {
    return "Failed to write zeros for header.";
  }
  if(!output.write(reinterpret_cast<char*>(levelIndex.data()), SizeofVector(levelIndex)))
  {
    return "Failed to write zeros for level index.";
  }

  //---------------------------------------------------------------------------
  // Write the Data Format Descriptor.
  // First, we know header.dfdByteOffset now.
  header.dfdByteOffset = static_cast<uint32_t>(output.tellp() - start_pos);
  // Since the Khronos Data Format specifies
  // data format descriptors for most of the formats we support, we base
  // things off that.
  // Unfortunately, BC2, BC3, and BC5 use two samples, so we need to use a vector here:
  std::vector<DFSample>     dfSamples(1);  // Can be resized
  BasicDataFormatDescriptor dfdBlock{};
  // Set some initial fields
  dfdBlock.descriptorType = 0;
  dfdBlock.vendorId       = 0;  // Khronos
  dfdBlock.versionNumber  = 2;
  dfdBlock.colorPrimaries = KHR_DF_PRIMARIES_BT709;
  // Note that we don't use the transferFunctions in the examples, since we
  // specify a transfer function for each format.
  dfdBlock.transferFunction = is_srgb ? KHR_DF_TRANSFER_SRGB : KHR_DF_TRANSFER_LINEAR;
  // Similarly with premultiplied alpha:
  if(is_premultiplied)
  {
    dfdBlock.flags |= KHR_DF_FLAG_ALPHA_PREMULTIPLIED;
  }
  else
  {
    dfdBlock.flags |= KHR_DF_FLAG_ALPHA_STRAIGHT;
  }

  // Switch over formats
  // The only case where VkFormat isn't enough is BGRX vs. BGRA, where we have
  // to do something special for the BGRX case.
  switch(format)
  {
    case VK_FORMAT_BC7_SRGB_BLOCK:
      // BC7
      dfdBlock.colorModel           = KHR_DF_MODEL_BC7;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 3;
      dfdBlock.texelBlockDimension1 = 3;
      dfdBlock.bytesPlane0          = 16;
      dfSamples[0].bitLength        = 127;
      dfSamples[0].channelType      = KHR_DF_CHANNEL_BC7_COLOR;
      dfSamples[0].upper            = UINT32_MAX;
      break;
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
      // BC6H signed
      dfdBlock.colorModel           = KHR_DF_MODEL_BC6H;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 3;
      dfdBlock.texelBlockDimension1 = 3;
      dfdBlock.bytesPlane0          = 16;
      dfSamples[0].bitLength        = 127;
      dfSamples[0].channelType =
          uint8_t(KHR_DF_CHANNEL_BC6H_COLOR) | uint8_t(KHR_DF_SAMPLE_DATATYPE_FLOAT | KHR_DF_SAMPLE_DATATYPE_SIGNED);
      dfSamples[0].lower = 0xBF800000u;  // -1.0f
      dfSamples[0].upper = 0x3F800000u;  // 1.0f
      break;
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
      // BC6H unsigned
      dfdBlock.colorModel           = KHR_DF_MODEL_BC6H;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 3;
      dfdBlock.texelBlockDimension1 = 3;
      dfdBlock.bytesPlane0          = 16;
      dfSamples[0].bitLength        = 127;
      dfSamples[0].channelType      = uint8_t(KHR_DF_CHANNEL_BC6H_COLOR) | uint8_t(KHR_DF_SAMPLE_DATATYPE_FLOAT);
      dfSamples[0].lower            = 0;            // 0.0f
      dfSamples[0].upper            = 0x3F800000u;  // 1.0f
      break;
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
      SetASTCFlags(4, 4, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
      SetASTCFlags(5, 4, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
      SetASTCFlags(5, 5, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
      SetASTCFlags(6, 5, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
      SetASTCFlags(6, 6, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
      SetASTCFlags(8, 5, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
      SetASTCFlags(8, 6, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
      SetASTCFlags(8, 8, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
      SetASTCFlags(10, 5, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
      SetASTCFlags(10, 6, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
      SetASTCFlags(10, 8, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
      SetASTCFlags(10, 10, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
      SetASTCFlags(12, 10, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
      SetASTCFlags(12, 12, dfdBlock, dfSamples);
      break;
    case VK_FORMAT_BC5_UNORM_BLOCK:
      dfdBlock.colorModel           = KHR_DF_MODEL_BC5;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 3;
      dfdBlock.texelBlockDimension1 = 3;
      dfdBlock.bytesPlane0          = 16;
      dfSamples.resize(2);
      dfSamples[0].bitLength   = 63;
      dfSamples[0].channelType = KHR_DF_CHANNEL_BC5_RED;
      dfSamples[0].upper       = UINT32_MAX;
      dfSamples[1].bitLength   = 63;
      dfSamples[1].bitOffset   = 64;
      dfSamples[1].channelType = KHR_DF_CHANNEL_BC5_GREEN;
      dfSamples[1].upper       = UINT32_MAX;
      break;
    case VK_FORMAT_BC4_UNORM_BLOCK:
      dfdBlock.colorModel           = KHR_DF_MODEL_BC4;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 3;
      dfdBlock.texelBlockDimension1 = 3;
      dfdBlock.bytesPlane0          = 8;
      dfSamples[0].bitLength        = 63;
      dfSamples[0].channelType      = KHR_DF_CHANNEL_BC4_DATA;
      dfSamples[0].upper            = UINT32_MAX;
      break;
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
      // We actually switch between DXT4 and DXT5 here based on
      // premultiplication, following the examples in the spec.
      if(is_premultiplied)
      {
        dfdBlock.colorModel = KHR_DF_MODEL_DXT4;
      }
      else
      {
        dfdBlock.colorModel = KHR_DF_MODEL_DXT5;
      }
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 3;
      dfdBlock.texelBlockDimension1 = 3;
      dfdBlock.bytesPlane0          = 16;
      dfSamples.resize(2);
      dfSamples[0].bitLength   = 63;
      dfSamples[0].channelType = uint8_t(KHR_DF_CHANNEL_BC3_ALPHA) | uint8_t(KHR_DF_SAMPLE_DATATYPE_LINEAR);
      dfSamples[0].upper       = UINT32_MAX;
      dfSamples[1].bitOffset   = 64;
      dfSamples[1].bitLength   = 63;
      dfSamples[1].channelType = KHR_DF_CHANNEL_BC3_COLOR;
      dfSamples[1].upper       = UINT32_MAX;
      break;
    case VK_FORMAT_BC2_SRGB_BLOCK:
      // Same premultiplication situation here as BC3
      if(is_premultiplied)
      {
        dfdBlock.colorModel = KHR_DF_MODEL_DXT2;
      }
      else
      {
        dfdBlock.colorModel = KHR_DF_MODEL_DXT3;
      }
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 3;
      dfdBlock.texelBlockDimension1 = 3;
      dfdBlock.bytesPlane0          = 16;
      dfSamples.resize(2);
      dfSamples[0].bitLength = 63;
      // The alpha channel must always be linear!
      dfSamples[0].channelType = uint8_t(KHR_DF_CHANNEL_BC2_ALPHA) | uint8_t(KHR_DF_SAMPLE_DATATYPE_LINEAR);
      dfSamples[0].upper       = UINT32_MAX;
      dfSamples[1].bitOffset   = 64;
      dfSamples[1].bitLength   = 63;
      dfSamples[1].channelType = KHR_DF_CHANNEL_BC2_COLOR;
      dfSamples[1].upper       = UINT32_MAX;
      break;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
      // BC1a
      dfdBlock.colorModel           = KHR_DF_MODEL_DXT1A;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 3;
      dfdBlock.texelBlockDimension1 = 3;
      dfdBlock.bytesPlane0          = 8;
      dfSamples[0].bitLength        = 63;
      dfSamples[0].channelType      = KHR_DF_CHANNEL_BC1A_ALPHAPRESENT;
      dfSamples[0].upper            = UINT32_MAX;
      break;
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
      // BC1
      dfdBlock.colorModel           = KHR_DF_MODEL_DXT1A;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 3;
      dfdBlock.texelBlockDimension1 = 3;
      dfdBlock.bytesPlane0          = 8;
      dfSamples[0].bitLength        = 63;
      dfSamples[0].channelType      = KHR_DF_CHANNEL_BC1A_COLOR;
      dfSamples[0].upper            = UINT32_MAX;
      break;
    case VK_FORMAT_R8_UNORM:
      // R8 UNORM
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = 1;

      dfSamples[0].bitOffset   = 0;
      dfSamples[0].bitLength   = 7;  // "8"
      dfSamples[0].channelType = KHR_DF_CHANNEL_RGBSDA_RED;
      dfSamples[0].upper       = 255;
      break;
    case VK_FORMAT_R8_SRGB:
      // R8 SRGB
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = 1;

      dfSamples[0].bitOffset   = 0;
      dfSamples[0].bitLength   = 7;  // "8"
      dfSamples[0].channelType = KHR_DF_CHANNEL_RGBSDA_RED;
      dfSamples[0].upper       = 255;
      break;
    case VK_FORMAT_B8G8R8_SRGB:
      // B in byte 0, G in byte 1, R in byte 2
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = 3;

      dfSamples.resize(3);

      dfSamples[0].bitOffset   = 0;
      dfSamples[0].bitLength   = 7;  // "8"
      dfSamples[0].channelType = KHR_DF_CHANNEL_RGBSDA_BLUE;
      dfSamples[0].upper       = 255;

      dfSamples[1].bitOffset   = 8;
      dfSamples[1].bitLength   = 7;  // "8"
      dfSamples[1].channelType = KHR_DF_CHANNEL_RGBSDA_GREEN;
      dfSamples[1].upper       = 255;

      dfSamples[2].bitOffset   = 16;
      dfSamples[2].bitLength   = 7;  // "8"
      dfSamples[2].channelType = KHR_DF_CHANNEL_RGBSDA_RED;
      dfSamples[2].upper       = 255;
      break;
    case VK_FORMAT_B8G8R8A8_SRGB:
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = 4;

      // B in byte 0, G in byte 1, R in byte 2, A in byte 3
      dfSamples.resize(4);

      dfSamples[0].bitOffset   = 0;
      dfSamples[0].bitLength   = 7;  // "8"
      dfSamples[0].channelType = KHR_DF_CHANNEL_RGBSDA_BLUE;
      dfSamples[0].upper       = 255;

      dfSamples[1].bitOffset   = 8;
      dfSamples[1].bitLength   = 7;  // "8"
      dfSamples[1].channelType = KHR_DF_CHANNEL_RGBSDA_GREEN;
      dfSamples[1].upper       = 255;

      dfSamples[2].bitOffset   = 16;
      dfSamples[2].bitLength   = 7;  // "8"
      dfSamples[2].channelType = KHR_DF_CHANNEL_RGBSDA_RED;
      dfSamples[2].upper       = 255;

      dfSamples[3].bitOffset   = 24;
      dfSamples[3].bitLength   = 7;  // "8"
      dfSamples[3].channelType = uint8_t(KHR_DF_CHANNEL_RGBSDA_ALPHA) | uint8_t(KHR_DF_SAMPLE_DATATYPE_LINEAR);
      dfSamples[3].upper       = 255;
      break;
    case VK_FORMAT_R16_SFLOAT:
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = sizeof(uint16_t);

      dfSamples[0].bitOffset = 0;
      dfSamples[0].bitLength = 15;  // "16"
      dfSamples[0].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_RED) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      // Yes, these are 32-bit floats, not 16-bit floats! From the spec
      dfSamples[0].lower = 0xBF800000u;  // -1.0f
      dfSamples[0].upper = 0x3F800000u;  // 1.0f
      break;
    case VK_FORMAT_R16G16_SFLOAT:
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = sizeof(uint16_t) * 2;

      dfSamples.resize(2);
      for(uint32_t c = 0; c < 2; c++)
      {
        dfSamples[c].bitOffset = 16 * c;
        dfSamples[c].bitLength = 15;           // "16"
        dfSamples[c].lower     = 0xBF800000u;  // -1.0f
        dfSamples[c].upper     = 0x3F800000u;  // 1.0f
      }

      dfSamples[0].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_RED) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      dfSamples[1].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_GREEN) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      break;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = sizeof(uint16_t) * 4;

      dfSamples.resize(4);
      for(uint32_t c = 0; c < 4; c++)
      {
        dfSamples[c].bitOffset = 16 * c;
        dfSamples[c].bitLength = 15;           // "16"
        dfSamples[c].lower     = 0xBF800000u;  // -1.0f
        dfSamples[c].upper     = 0x3F800000u;  // 1.0f
      }

      dfSamples[0].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_RED) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      dfSamples[1].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_GREEN) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      dfSamples[2].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_BLUE) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      dfSamples[3].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_ALPHA) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      break;
    case VK_FORMAT_R32_SFLOAT:
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = sizeof(uint32_t);

      dfSamples[0].bitOffset = 0;
      dfSamples[0].bitLength = 31;  // "32"
      dfSamples[0].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_RED) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      dfSamples[0].lower = 0xBF800000u;  // -1.0f
      dfSamples[0].upper = 0x3F800000u;  // 1.0f
      break;
    case VK_FORMAT_R32G32_SFLOAT:
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = sizeof(uint32_t) * 2;

      dfSamples.resize(2);
      for(uint32_t c = 0; c < 2; c++)
      {
        dfSamples[c].bitOffset = 32 * c;
        dfSamples[c].bitLength = 31;           // "32"
        dfSamples[c].lower     = 0xBF800000u;  // -1.0f
        dfSamples[c].upper     = 0x3F800000u;  // 1.0f
      }

      dfSamples[0].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_RED) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      dfSamples[1].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_GREEN) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      break;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = sizeof(uint32_t) * 4;

      dfSamples.resize(4);
      for(uint32_t c = 0; c < 4; c++)
      {
        dfSamples[c].bitOffset = 32 * c;
        dfSamples[c].bitLength = 31;           // "32"
        dfSamples[c].lower     = 0xBF800000u;  // -1.0f
        dfSamples[c].upper     = 0x3F800000u;  // 1.0f
      }

      dfSamples[0].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_RED) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      dfSamples[1].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_GREEN) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      dfSamples[2].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_BLUE) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      dfSamples[3].channelType =
          uint8_t(KHR_DF_CHANNEL_RGBSDA_ALPHA) | uint8_t(KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT);
      break;
    default:
      return "The writer has no method to write the Data Format Descriptor for VkFormat " + std::to_string(format) + ".";
  }

  // In the case of supercompressed formats, we have to set all bytesPlane
  // fields to 0 per the "DFD for Supercompressed Data" section.
  if(writeSettings.supercompression != WriteSupercompressionType::NONE)
  {
    dfdBlock.bytesPlane0 = 0;
    dfdBlock.bytesPlane1 = 0;
    dfdBlock.bytesPlane2 = 0;
    dfdBlock.bytesPlane3 = 0;
    dfdBlock.bytesPlane4 = 0;
    dfdBlock.bytesPlane5 = 0;
    dfdBlock.bytesPlane6 = 0;
    dfdBlock.bytesPlane7 = 0;
  }

  // Compute sizes
  dfdBlock.descriptorBlockSize = sizeof(dfdBlock) + SizeofVector(dfSamples);
  const uint32_t dfdTotalSize  = dfdBlock.descriptorBlockSize + sizeof(uint32_t);

  // Write the Data Format Descriptor
  if(!output.write(reinterpret_cast<const char*>(&dfdTotalSize), sizeof(dfdTotalSize)))
  {
    return "Writing dfdTotalSize failed.";
  }

  if(!output.write(reinterpret_cast<char*>(&dfdBlock), sizeof(dfdBlock)))
  {
    return "Writing the Basic Data Format Descriptor failed.";
  }

  if(!output.write(reinterpret_cast<char*>(dfSamples.data()), SizeofVector(dfSamples)))
  {
    return "Writing the samples of the Basic Data Format Descriptor failed.";
  }

  // Also set dfdByteLength now.
  header.dfdByteLength = dfdTotalSize;

  //---------------------------------------------------------------------------
  // Key/Value Data

  // Fill in the KTXwriter field if it's not already assigned.
  if(key_value_data.find("KTXwriter") == key_value_data.end())
  {
    key_value_data["KTXwriter"] = StringToCharVector("nvpro-samples' nv_ktx version 1");
  }

  // We now know the offset of the key/value data.
  header.kvdByteOffset = static_cast<uint32_t>(output.tellp() - start_pos);
  header.kvdByteLength = 0;
  // Write the key/value data.
  for(const auto& kvp : key_value_data)
  {
    // Include the null character on the key, but note that the values already
    // include it (and may not be strings!)
    const uint32_t keySizeWithNull = static_cast<uint32_t>(kvp.first.size() + 1);
    const uint32_t valueSize       = static_cast<uint32_t>(kvp.second.size());
    // Write key and value byte length. Does not include padding!
    const uint32_t keyAndValueByteLength = keySizeWithNull + valueSize;
    if(!output.write(reinterpret_cast<const char*>(&keyAndValueByteLength), sizeof(uint32_t)))
    {
      return "Writing keyAndValueByteLength failed.";
    }
    if(!output.write(kvp.first.c_str(), keySizeWithNull))
    {
      return "Writing a key-value pair failed.";
    }
    if(!output.write(reinterpret_cast<const char*>(kvp.second.data()), valueSize))
    {
      return "Writing a key-value pair failed.";
    }

    // Write up to 4 null characters
    std::array<char, 4> nulls            = {'\0', '\0', '\0', '\0'};
    const size_t        valuePaddingSize = RoundUp(keyAndValueByteLength, 4) - keyAndValueByteLength;
    assert(valuePaddingSize < 4);
    if(!output.write(nulls.data(), valuePaddingSize))
    {
      return "Writing value padding failed.";
    }

    header.kvdByteLength +=
        static_cast<uint32_t>(sizeof(uint32_t)) + keyAndValueByteLength + static_cast<uint32_t>(valuePaddingSize);
  }

  //---------------------------------------------------------------------------
  // Section 6
  // Currently no Basis supercompression, so no need for alignment here.
  assert(header.sgdByteLength == 0);

  //---------------------------------------------------------------------------
  // Section 7, the Mip Level Array
  // This is also where we perform Zstd supercompression!

  // First get the texel block size.
  size_t        texel_block_size;
  ErrorWithText maybeError = ExportSizeExtended(1, 1, 1, format, texel_block_size, writeSettings.custom_size_callback);
  if(maybeError.has_value())
  {
    return "Error getting the texel block size for VkFormat " + std::to_string(format) + "!";
  }

// Zstandard supercompression context
#ifdef NVP_SUPPORTS_ZSTD
  ScopedZstdCContext zstdContext;
  int                zstd_clamped_supercompression_level = writeSettings.supercompression_level;
#endif
  if(writeSettings.supercompression == WriteSupercompressionType::ZSTD)
  {
#ifdef NVP_SUPPORTS_ZSTD
    zstdContext.Init();
    if(zstdContext.pCtx == nullptr)
    {
      return "Initializing the Zstandard context for supercompression failed!";
    }

    // Clamp the compression level to Zstandard's min and max
    const int zstdMinLevel = ZSTD_minCLevel();
    const int zstdMaxLevel = ZSTD_maxCLevel();
    assert(zstdMaxLevel >= zstdMinLevel);
    if(zstd_clamped_supercompression_level < zstdMinLevel)
      zstd_clamped_supercompression_level = zstdMinLevel;
    if(zstd_clamped_supercompression_level > zstdMaxLevel)
      zstd_clamped_supercompression_level = zstdMaxLevel;
#else
    return "Zstandard supercompression was selected for KTX2 writing, but nv_ktx was built without Zstd!";
#endif
  }

  // Write mips from smallest to largest.
  for(int64_t mip = static_cast<int64_t>(num_mips) - 1; mip >= 0; mip--)
  {
    // First the mip padding if not supercompressed:
    if(writeSettings.supercompression == WriteSupercompressionType::NONE)
    {
      const size_t pos_from_start = output.tellp() - start_pos;
      const size_t mipPaddingSize = RoundUp(pos_from_start, LCM4(texel_block_size)) - pos_from_start;
      if(mipPaddingSize > 0)
      {
        // NOTE: Could be better
        std::vector<char> nulls(mipPaddingSize, 0);
        if(!output.write(nulls.data(), nulls.size()))
        {
          return "Writing mip padding failed.";
        }
      }
    }
    // We now know levels[mip].byteOffset, which comes after mip padding.
    levelIndex[mip].byteOffset = output.tellp() - start_pos;

    const size_t mipWidth  = std::max(1u, mip_0_width >> mip);
    const size_t mipHeight = std::max(1u, mip_0_height >> mip);
    const size_t mipDepth  = std::max(1u, mip_0_depth >> mip);

    // The size of each subresource of this mip in bytes.
    size_t subresource_size_bytes = 0;
    UNWRAP_ERROR(ExportSizeExtended(mipWidth, mipHeight, mipDepth, format, subresource_size_bytes, writeSettings.custom_size_callback));

    // Compute the size of this mip in bytes.
    levelIndex[mip].uncompressedByteLength = size_t(num_layers_or_1) * size_t(num_faces) * subresource_size_bytes;

    // If not supercompressing, write each face to the file.
    if(writeSettings.supercompression == WriteSupercompressionType::NONE)
    {
      for(uint32_t layer = 0; layer < num_layers_or_1; layer++)
      {
        for(uint32_t face = 0; face < num_faces; face++)
        {
          const std::vector<char>& this_subresource = subresource(static_cast<uint32_t>(mip), layer, face);
          assert(this_subresource.size() == subresource_size_bytes);
          if(!output.write(this_subresource.data(), this_subresource.size()))
          {
            return "Writing mip " + std::to_string(mip) + " layer " + std::to_string(layer) + " face "
                   + std::to_string(face) + " failed.";
          }
        }
      }
      levelIndex[mip].byteLength = levelIndex[mip].uncompressedByteLength;
    }
    else
    {
// We currently only support Zstd for writing.
#ifndef NVP_SUPPORTS_ZSTD
      return "Zstandard supercompression was selected, but nv_ktx was built without Zstd and execution reached the "
             "levelImages loop. This should never happen.";
#else
      if(writeSettings.supercompression != WriteSupercompressionType::ZSTD)
      {
        return "Only Zstandard supercompression is currently supported.";
      }
      // Concatenate all face data into a single buffer.
      // (Note: could potentially have lower peak memory usage but be more
      // complex using the Zstandard streaming API.)
      std::vector<char> rawData;
      {
        UNWRAP_ERROR(ResizeVectorOrError(rawData, levelIndex[mip].uncompressedByteLength));
        size_t pos_in_raw_data = 0;
        for(uint32_t layer = 0; layer < num_layers_or_1; layer++)
        {
          for(uint32_t face = 0; face < num_faces; face++)
          {
            const std::vector<char>& this_subresource = subresource(uint32_t(mip), layer, face);
            assert(this_subresource.size() == subresource_size_bytes);
            memcpy(&rawData[pos_in_raw_data], this_subresource.data(), this_subresource.size());
            pos_in_raw_data += this_subresource.size();
          }
        }
      }

      // Also allocate a buffer with the maximum possible compressed size needed.
      // (Note that this is always larger than the source!)
      // Also note that we'll always write the supercompressed data even when
      // it's larger, as the client controls whether supercompression is used.
      const size_t fullMipUncompressedLength = levelIndex[mip].uncompressedByteLength;
      const size_t supercompressedMaxSize = ZSTD_COMPRESSBOUND(fullMipUncompressedLength);
      std::vector<char> supercompressedData;
      try
      {
        supercompressedData.resize(supercompressedMaxSize);
      }
      catch(...)
      {
        return "Allocating memory for Zstandard supercompressed output failed!";
      }

      // Compress!
      size_t errOrSize = ZSTD_compressCCtx(zstdContext.pCtx, supercompressedData.data(), supercompressedData.size(),
                                           rawData.data(), rawData.size(), zstd_clamped_supercompression_level);
      if(ZSTD_isError(errOrSize))
      {
        return "Zstandard supercompression returned error " + std::to_string(errOrSize) + ".";
      }

      if(errOrSize > supercompressedData.size())
      {
        assert(false);  // This should never happen
        return "ZSTD_compressCCtx returned a number that was larger than the size of the supercompressed data "
               "buffer.";
      }

      // Write the supercompressed data to the file.
      if(!output.write(supercompressedData.data(), errOrSize))
      {
        return "Writing mip " + std::to_string(mip) + "'s supercompressed data to the file failed!";
      }
      levelIndex[mip].byteLength = errOrSize;
#endif
    }
  }

  // Now, fill in the header, then go back and write the identifier, header, and level index.
  header.vkFormat = format;
  switch(format)
  {
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
      header.typeSize = 2;
      break;
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
      header.typeSize = 4;
      break;
    default:
      header.typeSize = 1;
      break;
  }
  header.pixelWidth  = mip_0_width;
  header.pixelHeight = mip_0_height;
  header.pixelDepth  = mip_0_depth;
  header.layerCount  = num_layers_possibly_0;
  header.faceCount   = num_faces;
  header.levelCount  = app_should_generate_mips ? 0 : num_mips;
  switch(writeSettings.supercompression)
  {
    case WriteSupercompressionType::NONE:
      header.supercompressionScheme = 0;
      break;
    case WriteSupercompressionType::ZSTD:
      header.supercompressionScheme = 2;
      break;
    default:
      return "Unsupported WriteSupercompression type while writing KTX2 header.";
  }

  output.seekp(start_pos, std::ios::beg);
  output.write(reinterpret_cast<const char*>(ktx2Identifier), IDENTIFIER_LEN);
  output.write(reinterpret_cast<const char*>(&header), sizeof(header));
  output.write(reinterpret_cast<const char*>(levelIndex.data()), SizeofVector(levelIndex));

  // And we're done!
  return {};
}

inline ErrorWithText KTXImage::writeKTX2File(const char* filename, const WriteSettings& writeSettings)
{
  std::ofstream output(filename, std::ofstream::binary | std::ofstream::out | std::ofstream::trunc);
  return writeKTX2Stream(output, writeSettings);
}

//-----------------------------------------------------------------------------
// KTX1/KTX2 READING BRANCH
//-----------------------------------------------------------------------------

inline ErrorWithText KTXImage::readFromStream(std::istream& input, const ReadSettings& readSettings)
{
  // Read the identifier.
  uint8_t identifier[IDENTIFIER_LEN]{};
  if(!input.read(reinterpret_cast<char*>(identifier), IDENTIFIER_LEN))
  {
    return "Reading the identifier failed!";
  }

  // Check if the identifier matches either the KTX 1 identifier or the KTX 2 identifier.
  if(memcmp(identifier, ktx1Identifier, IDENTIFIER_LEN) == 0)
  {
    read_ktx_version = 1;
    return readFromKTX1Stream(input, readSettings);
  }
  else if(memcmp(identifier, ktx2Identifier, IDENTIFIER_LEN) == 0)
  {
    read_ktx_version = 2;
    return readFromKTX2Stream(input, readSettings);
  }

  // Otherwise,
  return "Not a KTX1 or KTX2 file (first 12 bytes weren't a valid identifier).";
}

inline ErrorWithText KTXImage::readFromFile(const char* filename, const ReadSettings& readSettings)
{
  std::ifstream input_stream(filename, std::ifstream::in | std::ifstream::binary);
  return readFromStream(input_stream, readSettings);
}

}  // namespace nv_ktx
