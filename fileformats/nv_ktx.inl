#include "nv_ktx.h"

#include <algorithm>
#include <array>
#include <cassert>  // Some functions produce assertion errors to assist with debugging when NDEBUG is false.
#include <fstream>
#include <GL/glcustom.h>
#include <fileformats/khr_df.h>
#include <sstream>
#include <vulkan/vulkan.h>
#ifdef NVP_SUPPORTS_ZSTD
#include <zstd.h>
#endif
#ifdef NVP_SUPPORTS_GZLIB
#include <zlib.h>
#endif

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

// Multiplies three values, returning false if the calculation could overflow,
// interpreting each value as 1 if it would be 0.
inline bool GetNumSubresources(size_t a, size_t b, size_t c, size_t& out)
{
  const size_t a_or_1 = std::max(a, size_t(1));
  const size_t b_or_1 = std::max(b, size_t(1));
  const size_t c_or_1 = std::max(c, size_t(1));
  out                 = a_or_1;
  if(b_or_1 > MAXSIZE_T / out)
  {
    return false;
  }
  out *= b_or_1;
  if(c_or_1 > MAXSIZE_T / out)
  {
    return false;
  }
  out *= c_or_1;
  return true;
}

inline ErrorWithText KTXImage::allocate(uint32_t _num_mips, uint32_t _num_layers, uint32_t _num_faces)
{
  clear();

  num_mips_possibly_0   = _num_mips;
  num_layers_possibly_0 = _num_layers;
  num_faces             = _num_faces;

  size_t num_subresources = 0;
  if(!GetNumSubresources(num_mips_possibly_0, num_layers_possibly_0, num_faces, num_subresources))
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
  const uint32_t num_mips   = std::max(num_mips_possibly_0, 1U);
  const uint32_t num_layers = std::max(num_layers_possibly_0, 1U);
  if(mip >= num_mips || layer >= num_layers || face >= num_faces)
  {
    throw std::out_of_range("KTXImage::subresource values were out of range");
  }

  // Here's the layout for data that we use. Note that we store the lowest mips
  // (mip 0) first, while the KTX format stores the highest mips first.
  return data[(size_t(mip) * size_t(num_layers) + size_t(layer)) * size_t(num_faces) + size_t(face)];
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
static_assert(sizeof(DFSample) == 16,
              "Basic data format descriptor sample type size must match Khronos Data Format spec!");

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

static_assert(sizeof(char) == sizeof(uint8_t),
              "Things will probably go wrong in nv_ktx code with istream reads if chars aren't bytes.");

inline ErrorWithText ReadKeyValueData(std::istream&                             input,
                                      uint32_t                                  kvdByteLength,
                                      bool                                      srcIsBigEndian,
                                      std::map<std::string, std::vector<char>>& outKeyValueData)
{
  {
    std::vector<char> kvBlock(kvdByteLength);
    if(!input.read(kvBlock.data(), size_t(kvdByteLength)))
    {
      return "Unable to read " + std::to_string(kvdByteLength) + " bytes of KTX2 key/value data.";
    }

    size_t byteIndex = 0;
    while(byteIndex < kvdByteLength)
    {
      // Read keyAndValueByteLength
      uint32_t keyAndValueByteLength = 0;
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
        return "Key starting at byte " + std::to_string(byteIndex)
               + " of the key/value data did not have a NUL character.";
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
  }

  return {};
}

inline size_t ASTCSize(size_t blockWidth, size_t blockHeight, size_t blockDepth, size_t width, size_t height, size_t depth)
{
  // Each ASTC block size is 128 bits = 16 bytes
  return ((width + blockWidth - 1) / blockWidth)       //
         * ((height + blockHeight - 1) / blockHeight)  //
         * ((depth + blockDepth - 1) / blockDepth)     //
         * 16;
}

// Returns the size of a width x height x depth image of the given VkFormat.
// Returns an error if the given image sizes are strictly invalid.
inline ErrorWithText ExportSize(size_t width, size_t height, size_t depth, VkFormat format, size_t& outSize)
{
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
      outSize = width * height * depth * (8 / 8);  // 8 bits per pixel
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
      outSize = width * height * depth * (16 / 8);  // 16 bits per pixel
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
      outSize = width * height * depth * (24 / 8);  // 24 bits per pixel
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
      outSize = width * height * depth * (32 / 8);  // 32 bits per pixel
      return {};
    case VK_FORMAT_R16G16B16_UNORM:
    case VK_FORMAT_R16G16B16_SNORM:
    case VK_FORMAT_R16G16B16_USCALED:
    case VK_FORMAT_R16G16B16_SSCALED:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16_SFLOAT:
      outSize = width * height * depth * (48 / 8);  // 48 bits per pixel
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
      outSize = width * height * depth * (64 / 8);  // 64 bits per pixel
      return {};
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32_SFLOAT:
      outSize = width * height * depth * (96 / 8);  // 96 bits per pixel
      return {};
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_R64G64_UINT:
    case VK_FORMAT_R64G64_SINT:
    case VK_FORMAT_R64G64_SFLOAT:
      outSize = width * height * depth * (128 / 8);  // 128 bits per pixel
      return {};
    case VK_FORMAT_R64G64B64_UINT:
    case VK_FORMAT_R64G64B64_SINT:
    case VK_FORMAT_R64G64B64_SFLOAT:
      outSize = width * height * depth * (196 / 8);  // 196 bits per pixel
      return {};
    case VK_FORMAT_R64G64B64A64_UINT:
    case VK_FORMAT_R64G64B64A64_SINT:
    case VK_FORMAT_R64G64B64A64_SFLOAT:
      outSize = width * height * depth * (256 / 8);  // 256 bits per pixel
      return {};
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
      outSize = ((width + 3) / 4) * ((height + 3) / 4) * depth * 8;  // 8 bytes per block
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
      outSize = ((width + 3) / 4) * ((height + 3) / 4) * depth * 16;  // 16 bytes per block
      return {};
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
      outSize = ASTCSize(4, 4, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
      outSize = ASTCSize(5, 4, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
      outSize = ASTCSize(5, 5, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
      outSize = ASTCSize(6, 5, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
      outSize = ASTCSize(6, 6, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
      outSize = ASTCSize(8, 5, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
      outSize = ASTCSize(8, 6, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
      outSize = ASTCSize(8, 8, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
      outSize = ASTCSize(10, 5, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
      outSize = ASTCSize(10, 6, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
      outSize = ASTCSize(10, 8, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
      outSize = ASTCSize(10, 10, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
      outSize = ASTCSize(12, 10, 1, width, height, depth);
      return {};
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
      outSize = ASTCSize(12, 12, 1, width, height, depth);
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
  GL_TO_VK_FORMAT_CASE(GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, VK_FORMAT_R4G4B4A4_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(GL_RGBA4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4, VK_FORMAT_B4G4R4A4_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, VK_FORMAT_R5G6B5_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV, VK_FORMAT_B5G6R5_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, VK_FORMAT_R5G5B5A1_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_5_5_5_1, VK_FORMAT_B5G5R5A1_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, VK_FORMAT_A1R5G5B5_UNORM_PACK16);
  GL_TO_VK_FORMAT_CASE(GL_R8, GL_RED, GL_UNSIGNED_BYTE, VK_FORMAT_R8_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_R8_SNORM, GL_RED, GL_BYTE, VK_FORMAT_R8_SNORM);
  GL_TO_VK_FORMAT_CASE(GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, VK_FORMAT_R8_UINT);
  GL_TO_VK_FORMAT_CASE(GL_R8I, GL_RED_INTEGER, GL_BYTE, VK_FORMAT_R8_SINT);
#ifdef GL_SR8_EXT
  GL_TO_VK_FORMAT_CASE(GL_SR8_EXT, GL_RED, GL_UNSIGNED_BYTE, VK_FORMAT_R8_SRGB);
#endif
  GL_TO_VK_FORMAT_CASE(GL_RG8, GL_RG, GL_UNSIGNED_BYTE, VK_FORMAT_R8G8_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_RG8_SNORM, GL_RG, GL_BYTE, VK_FORMAT_R8G8_SNORM);
  GL_TO_VK_FORMAT_CASE(GL_RG8UI, GL_RG_INTEGER, GL_UNSIGNED_BYTE, VK_FORMAT_R8G8_UINT);
  GL_TO_VK_FORMAT_CASE(GL_RG8I, GL_RG_INTEGER, GL_BYTE, VK_FORMAT_R8G8_SINT);
#ifdef GL_SRG8_EXT
  GL_TO_VK_FORMAT_CASE(GL_SRG8_EXT, GL_RG, GL_UNSIGNED_BYTE, VK_FORMAT_R8G8_SRGB);
#endif
  GL_TO_VK_FORMAT_CASE(GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGB8_SNORM, GL_RGB, GL_BYTE, VK_FORMAT_R8G8B8_SNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGB8UI, GL_RGB_INTEGER, GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8_UINT);
  GL_TO_VK_FORMAT_CASE(GL_RGB8I, GL_RGB_INTEGER, GL_BYTE, VK_FORMAT_R8G8B8_SINT);
  GL_TO_VK_FORMAT_CASE(GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8_SRGB);
  GL_TO_VK_FORMAT_CASE(GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGB8_SNORM, GL_BGR, GL_BYTE, VK_FORMAT_B8G8R8_SNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGB8UI, GL_BGR_INTEGER, GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8_UINT);
  GL_TO_VK_FORMAT_CASE(GL_RGB8I, GL_BGR_INTEGER, GL_BYTE, VK_FORMAT_B8G8R8_SINT);
  GL_TO_VK_FORMAT_CASE(GL_SRGB8, GL_BGR, GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8_SRGB);
  GL_TO_VK_FORMAT_CASE(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8A8_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGBA8_SNORM, GL_RGBA, GL_BYTE, VK_FORMAT_R8G8B8A8_SNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8A8_UINT);
  GL_TO_VK_FORMAT_CASE(GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE, VK_FORMAT_R8G8B8A8_SINT);
  GL_TO_VK_FORMAT_CASE(GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, VK_FORMAT_R8G8B8A8_SRGB);
  GL_TO_VK_FORMAT_CASE(GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8A8_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGBA8_SNORM, GL_BGRA, GL_BYTE, VK_FORMAT_B8G8R8A8_SNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGBA8UI, GL_BGRA_INTEGER, GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8A8_UINT);
  GL_TO_VK_FORMAT_CASE(GL_RGBA8I, GL_BGRA_INTEGER, GL_BYTE, VK_FORMAT_B8G8R8A8_SINT);
  GL_TO_VK_FORMAT_CASE(GL_SRGB8_ALPHA8, GL_BGRA, GL_UNSIGNED_BYTE, VK_FORMAT_B8G8R8A8_SRGB);
  GL_TO_VK_FORMAT_CASE(GL_RGB10_A2, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV, VK_FORMAT_A2R10G10B10_UNORM_PACK32);
  GL_TO_VK_FORMAT_CASE(GL_RGB10_A2UI, GL_BGRA_INTEGER, GL_UNSIGNED_INT_2_10_10_10_REV, VK_FORMAT_A2R10G10B10_UINT_PACK32);
  GL_TO_VK_FORMAT_CASE(GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, VK_FORMAT_A2B10G10R10_UNORM_PACK32);
  GL_TO_VK_FORMAT_CASE(GL_RGB10_A2UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT_2_10_10_10_REV, VK_FORMAT_A2B10G10R10_UINT_PACK32);
  GL_TO_VK_FORMAT_CASE(GL_R16, GL_RED, GL_UNSIGNED_SHORT, VK_FORMAT_R16_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_R16_SNORM, GL_RED, GL_SHORT, VK_FORMAT_R16_SNORM);
  GL_TO_VK_FORMAT_CASE(GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT, VK_FORMAT_R16_UINT);
  GL_TO_VK_FORMAT_CASE(GL_R16I, GL_RED_INTEGER, GL_SHORT, VK_FORMAT_R16_SINT);
  GL_TO_VK_FORMAT_CASE(GL_R16F, GL_RED, GL_HALF_FLOAT, VK_FORMAT_R16_SFLOAT);
  GL_TO_VK_FORMAT_CASE(GL_RG16, GL_RG, GL_UNSIGNED_SHORT, VK_FORMAT_R16G16_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_RG16_SNORM, GL_RG, GL_SHORT, VK_FORMAT_R16G16_SNORM);
  GL_TO_VK_FORMAT_CASE(GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_SHORT, VK_FORMAT_R16G16_UINT);
  GL_TO_VK_FORMAT_CASE(GL_RG16I, GL_RG_INTEGER, GL_SHORT, VK_FORMAT_R16G16_SINT);
  GL_TO_VK_FORMAT_CASE(GL_RG16F, GL_RG, GL_HALF_FLOAT, VK_FORMAT_R16G16_SFLOAT);
  GL_TO_VK_FORMAT_CASE(GL_RGB16, GL_RGB, GL_UNSIGNED_SHORT, VK_FORMAT_R16G16B16_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGB16_SNORM, GL_RGB, GL_SHORT, VK_FORMAT_R16G16B16_SNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGB16UI, GL_RGB_INTEGER, GL_UNSIGNED_SHORT, VK_FORMAT_R16G16B16_UINT);
  GL_TO_VK_FORMAT_CASE(GL_RGB16I, GL_RGB_INTEGER, GL_SHORT, VK_FORMAT_R16G16B16_SINT);
  GL_TO_VK_FORMAT_CASE(GL_RGB16F, GL_RGB, GL_HALF_FLOAT, VK_FORMAT_R16G16B16_SFLOAT);
  GL_TO_VK_FORMAT_CASE(GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT, VK_FORMAT_R16G16B16A16_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGBA16_SNORM, GL_RGBA, GL_SHORT, VK_FORMAT_R16G16B16A16_SNORM);
  GL_TO_VK_FORMAT_CASE(GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, VK_FORMAT_R16G16B16A16_UINT);
  GL_TO_VK_FORMAT_CASE(GL_RGBA16I, GL_RGBA_INTEGER, GL_SHORT, VK_FORMAT_R16G16B16A16_SINT);
  GL_TO_VK_FORMAT_CASE(GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, VK_FORMAT_R16G16B16A16_SFLOAT);
  GL_TO_VK_FORMAT_CASE(GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, VK_FORMAT_R32_UINT);
  GL_TO_VK_FORMAT_CASE(GL_R32I, GL_RED_INTEGER, GL_INT, VK_FORMAT_R32_SINT);
  GL_TO_VK_FORMAT_CASE(GL_R32F, GL_RED, GL_FLOAT, VK_FORMAT_R32_SFLOAT);
  GL_TO_VK_FORMAT_CASE(GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, VK_FORMAT_R32G32_UINT);
  GL_TO_VK_FORMAT_CASE(GL_RG32I, GL_RG_INTEGER, GL_INT, VK_FORMAT_R32G32_SINT);
  GL_TO_VK_FORMAT_CASE(GL_RG32F, GL_RG, GL_FLOAT, VK_FORMAT_R32G32_SFLOAT);
  GL_TO_VK_FORMAT_CASE(GL_RGB32UI, GL_RGB_INTEGER, GL_UNSIGNED_INT, VK_FORMAT_R32G32B32_UINT);
  GL_TO_VK_FORMAT_CASE(GL_RGB32I, GL_RGB_INTEGER, GL_INT, VK_FORMAT_R32G32B32_SINT);
  GL_TO_VK_FORMAT_CASE(GL_RGB32F, GL_RGB, GL_FLOAT, VK_FORMAT_R32G32B32_SFLOAT);
  GL_TO_VK_FORMAT_CASE(GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, VK_FORMAT_R32G32B32A32_UINT);
  GL_TO_VK_FORMAT_CASE(GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, VK_FORMAT_R32G32B32A32_SINT);
  GL_TO_VK_FORMAT_CASE(GL_RGBA32F, GL_RGBA, GL_FLOAT, VK_FORMAT_R32G32B32A32_SFLOAT);
  GL_TO_VK_FORMAT_CASE(GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV, VK_FORMAT_B10G11R11_UFLOAT_PACK32);
  GL_TO_VK_FORMAT_CASE(GL_RGB9_E5, GL_RGB, GL_UNSIGNED_INT_5_9_9_9_REV, VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
  GL_TO_VK_FORMAT_CASE(GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, VK_FORMAT_D16_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, VK_FORMAT_X8_D24_UNORM_PACK32);
  GL_TO_VK_FORMAT_CASE(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, VK_FORMAT_D32_SFLOAT);
  GL_TO_VK_FORMAT_CASE(GL_STENCIL_INDEX8, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, VK_FORMAT_S8_UINT);
  GL_TO_VK_FORMAT_CASE(GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, VK_FORMAT_D24_UNORM_S8_UINT);
  GL_TO_VK_FORMAT_CASE(GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, VK_FORMAT_D32_SFLOAT_S8_UINT);
  // Some cases where GL_TO_VK_FORMAT_CASE is too specific.
  // Note that there are some repeated glInternalFormats in this list due to UNORM/sRGB ambiguity in OpenGL!
  // I've left it in as-is in case it's useful as a translated version of KTX2 Appendix A at some point.
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGB_S3TC_DXT1_EXT, VK_FORMAT_BC1_RGB_UNORM_BLOCK);
#ifdef GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, VK_FORMAT_BC1_RGB_SRGB_BLOCK);
#endif
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
#ifdef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
#endif
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, VK_FORMAT_BC2_UNORM_BLOCK);
#ifdef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, VK_FORMAT_BC2_SRGB_BLOCK);
#endif
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, VK_FORMAT_BC3_UNORM_BLOCK);
#ifdef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, VK_FORMAT_BC3_SRGB_BLOCK);
#endif
#ifdef GL_COMPRESSED_RED_RGTC1_EXT
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RED_RGTC1_EXT, VK_FORMAT_BC4_UNORM_BLOCK);
#endif
#ifdef GL_COMPRESSED_SIGNED_RED_RGTC1_EXT
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SIGNED_RED_RGTC1_EXT, VK_FORMAT_BC4_SNORM_BLOCK);
#endif
#ifdef GL_COMPRESSED_RED_GREEN_RGTC2_EXT
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RED_GREEN_RGTC2_EXT, VK_FORMAT_BC5_UNORM_BLOCK);
#endif
#ifdef GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT, VK_FORMAT_BC5_SNORM_BLOCK);
#endif
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB, VK_FORMAT_BC6H_UFLOAT_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB, VK_FORMAT_BC6H_SFLOAT_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, VK_FORMAT_BC7_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB, VK_FORMAT_BC7_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGB8_ETC2, VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ETC2, VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA8_ETC2_EAC, VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_R11_EAC, VK_FORMAT_EAC_R11_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SIGNED_R11_EAC, VK_FORMAT_EAC_R11_SNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RG11_EAC, VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SIGNED_RG11_EAC, VK_FORMAT_EAC_R11G11_SNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_4x4_KHR, VK_FORMAT_ASTC_4x4_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR, VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_4x4_KHR, VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_5x4_KHR, VK_FORMAT_ASTC_5x4_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR, VK_FORMAT_ASTC_5x4_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_5x4_KHR, VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_5x5_KHR, VK_FORMAT_ASTC_5x5_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR, VK_FORMAT_ASTC_5x5_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_5x5_KHR, VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_6x5_KHR, VK_FORMAT_ASTC_6x5_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR, VK_FORMAT_ASTC_6x5_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_6x5_KHR, VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_6x6_KHR, VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR, VK_FORMAT_ASTC_6x6_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_6x6_KHR, VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_8x5_KHR, VK_FORMAT_ASTC_8x5_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR, VK_FORMAT_ASTC_8x5_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_8x5_KHR, VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_8x6_KHR, VK_FORMAT_ASTC_8x6_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR, VK_FORMAT_ASTC_8x6_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_8x6_KHR, VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_8x8_KHR, VK_FORMAT_ASTC_8x8_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR, VK_FORMAT_ASTC_8x8_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_8x8_KHR, VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_10x5_KHR, VK_FORMAT_ASTC_10x5_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR, VK_FORMAT_ASTC_10x5_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_10x5_KHR, VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_10x6_KHR, VK_FORMAT_ASTC_10x6_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR, VK_FORMAT_ASTC_10x6_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_10x6_KHR, VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_10x8_KHR, VK_FORMAT_ASTC_10x8_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR, VK_FORMAT_ASTC_10x8_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_10x8_KHR, VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_10x10_KHR, VK_FORMAT_ASTC_10x10_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR, VK_FORMAT_ASTC_10x10_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_10x10_KHR, VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_12x10_KHR, VK_FORMAT_ASTC_12x10_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR, VK_FORMAT_ASTC_12x10_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_12x10_KHR, VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_12x12_KHR, VK_FORMAT_ASTC_12x12_UNORM_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR, VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_12x12_KHR, VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT);
  // We don't have the GL enums for these yet:
  /*
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_3x3x3_OES, VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES, VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_3x3x3_OES, VK_FORMAT_ASTC_3x3x3_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_4x3x3_OES, VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES, VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_4x3x3_OES, VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_4x4x3_OES, VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES, VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_4x4x3_OES, VK_FORMAT_ASTC_4x4x3_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_4x4x4_OES, VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES, VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_4x4x4_OES, VK_FORMAT_ASTC_4x4x4_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_5x4x4_OES, VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES, VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_5x4x4_OES, VK_FORMAT_ASTC_5x4x4_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_5x5x4_OES, VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES, VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_5x5x4_OES, VK_FORMAT_ASTC_5x5x4_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_5x5x5_OES, VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES, VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_5x5x5_OES, VK_FORMAT_ASTC_5x5x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_6x5x5_OES, VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES, VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_6x5x5_OES, VK_FORMAT_ASTC_6x5x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_6x6x5_OES, VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES, VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_6x6x5_OES, VK_FORMAT_ASTC_6x6x5_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_6x6x6_OES, VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES, VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_ASTC_6x6x6_OES, VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG, VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG, VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG, VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT, VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT, VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG, VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG);
  GL_TO_VK_FORMAT_CASE_INTERNAL(GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG, VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG);
  */

  // Extra cases from the KTX1 tests not in Appendix A.
  GL_TO_VK_FORMAT_CASE(GL_ALPHA8, GL_ALPHA, GL_UNSIGNED_BYTE, VK_FORMAT_R8_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE, VK_FORMAT_R8_UNORM);
  GL_TO_VK_FORMAT_CASE(GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, VK_FORMAT_R8G8_UNORM);

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

  KTX1TopLevelHeader header;
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
  num_mips_possibly_0   = header.numberOfMipmapLevels;
  num_layers_possibly_0 = header.numberOfArrayElements;
  num_faces             = header.numberOfFaces;

  app_should_generate_mips = (header.numberOfMipmapLevels == 0);
  if(app_should_generate_mips)
  {
    header.numberOfMipmapLevels = 1;
  }
  // Because KTX1 files store mips from largest to smallest (as opposed to KTX2),
  // we can handle readSettings.mips by truncating num_mips_possibly_0.
  if(!readSettings.mips)
  {
    header.numberOfMipmapLevels = 1;
  }
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
    if(!GetNumSubresources(num_mips_possibly_0, num_layers_possibly_0, num_faces, validation_num_subresources))
    {
      return "Computing the number of mips times layers times faces in the file overflowed!";
    }
    if(validation_num_subresources > validation_input_size)
    {
      return "The KTX1 input had a likely invalid header - it listed " + std::to_string(num_mips_possibly_0)
             + " mips (or 0), " + std::to_string(num_layers_possibly_0) + " layers (or 0), and " + std::to_string(num_faces)
             + " faces - but the input was only " + std::to_string(validation_input_size) + " bytes long!";
    }
    if(header.bytesOfKeyValueData > validation_input_size)
    {
      return "The KTX1 input had an invalid header - it listed " + std::to_string(header.bytesOfKeyValueData)
             + " bytes of key/value data, but the input was only " + std::to_string(validation_input_size)
             + " bytes long!";
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
  UNWRAP_ERROR(allocate(num_mips_possibly_0, num_layers_possibly_0, num_faces));

  for(uint32_t mip = 0; mip < header.numberOfMipmapLevels; mip++)
  {
    // Read the image size. We use this for mip padding later on, and rely on
    // ExportSize for individual subresources.
    uint32_t imageSize;
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
          const std::streamoff cubePaddingBytes = static_cast<std::streamoff>(3 - ((faceSizeBytes + 3) % 4));
          input.seekg(cubePaddingBytes, std::ios_base::cur);
        }
      }
    }
    // Handle mip padding. The spec says that this is always 3 - ((imageSize + 3)%4) bytes,
    // and we assume bytes are the same size as chars:
    {
      const std::streamoff mipPaddingBytes = static_cast<std::streamoff>(3 - ((imageSize + 3) % 4));
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

// KTX 2 Level Index structure (Section 3.9.7 "Level Index")
struct LevelIndex
{
  uint64_t byteOffset;
  uint64_t byteLength;
  uint64_t uncompressedByteLength;
};
static_assert(sizeof(LevelIndex) == 3 * sizeof(uint64_t), "LevelIndex size must match KTX2 spec!");

enum class KTX_CUBEMAP_INCOMPLETE
{
  NONE       = 0,
  POSITIVE_X = 1 << 0,
  NEGATIVE_X = 1 << 1,
  POSITIVE_Y = 1 << 2,
  NEGATIVE_Y = 1 << 3,
  POSITIVE_Z = 1 << 4,
  NEGATIVE_Z = 1 << 5,
  ALL        = POSITIVE_X | NEGATIVE_X | POSITIVE_Y | NEGATIVE_Y | POSITIVE_Z | NEGATIVE_Z
};

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
  KTX2TopLevelHeader header;
  READ_OR_ERROR(input, header, "Failed to read KTX2 header and section 1.");

  // Copy the dimensions into the structure so that we can determine the
  // texture type later.
  format                = header.vkFormat;
  mip_0_width           = header.pixelWidth;
  mip_0_height          = header.pixelHeight;
  mip_0_depth           = header.pixelDepth;
  num_mips_possibly_0   = header.levelCount;
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
  app_should_generate_mips = (header.levelCount == 0);
  if(app_should_generate_mips)
  {
    header.levelCount = 1;
  }

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
  if(header.levelCount > 31)
  {
    std::stringstream str;
    str << "KTX2 levelCount was too large (" << header.levelCount << ") - the "
        << "maximum number of mips possible in a KTX2 file is 31.";
    return str.str();
  }
  if(readSettings.validate_input_size)
  {
    size_t validation_num_subresources = 0;
    if(!GetNumSubresources(num_mips_possibly_0, num_layers_possibly_0, num_faces, validation_num_subresources))
    {
      return "Computing the number of mips times layers times faces in the file overflowed!";
    }
    if(validation_num_subresources > validation_input_size)
    {
      return "The KTX1 input had a likely invalid header - it listed " + std::to_string(num_mips_possibly_0)
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
             + " bytes of key/value data, but the input was only " + std::to_string(validation_input_size)
             + " bytes long!";
    }
  }

  const int numMipsToRead = (readSettings.mips ? static_cast<int>(header.levelCount) : 1);

  //---------------------------------------------------------------------------
  // Load the level indices (section 2)
  std::vector<LevelIndex> levelIndices(header.levelCount);
  if(!input.read(reinterpret_cast<char*>(levelIndices.data()), sizeof(LevelIndex) * header.levelCount))
  {
    return "Unable to read Level Index from KTX2 file.";
  }

  //---------------------------------------------------------------------------
  // Load the Data Format Descriptor. We read this as a uint32_t array and then
  // interpret it later.
  // We also start counting the number of bytes read here so that we can handle
  // the align(8) sgdPadding correctly.
  std::vector<uint32_t> dfd(header.dfdByteLength / 4);
  if(!input.read(reinterpret_cast<char*>(dfd.data()), header.dfdByteLength))
  {
    return "Unable to read Data Format Descriptor from KTX2 file.";
  }

  // Get some information from the Basic Data Format Descriptor.
  uint32_t                  dfdTotalSize = 0;
  BasicDataFormatDescriptor basicDFD{};
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
  }

  VkFormat inflatedVkFormat = header.vkFormat;
  uint32_t khrDfPrimaries   = KHR_DF_PRIMARIES_SRGB;
  is_premultiplied          = false;
  is_srgb                   = true;
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
    else if(basicDFD.transferFunction = KHR_DF_TRANSFER_LINEAR)
    {
      is_srgb = false;
    }
    else
    {
      return "KTX2 Data Format Descriptor had an unhandled transferFunction ("
             + std::to_string(basicDFD.transferFunction) + ")";
    }

    if(header.vkFormat == VK_FORMAT_UNDEFINED)
    {
      if(basicDFD.colorModel == KDF_DF_MODEL_UASTC)
      {
        return "UASTC is not currently supported.";  // TODO
      }
      else if(basicDFD.colorModel == KHR_DF_MODEL_ETC1S)
      {
        return "Basis Universal ETC1S is not currently supported.";  // TODO
      }
      else
      {
        return "KTX2 VkFormat was VK_FORMAT_UNDEFINED, but the Data Format Descriptor block had unrecognized "
               "colorModel number "
               + std::to_string(basicDFD.colorModel) + ".";
      }
    }
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
      const size_t charsToRead = std::min(4ULL, value.size());
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
  if(header.sgdByteLength > 0xFFFFFFFFULL)
  {
    return "Supercompression global data length (sgdByteLength) was over 4 GB!";
  }
  std::vector<char> supercompressionGlobalData(header.sgdByteLength);
  if(header.sgdByteLength > 0)
  {
    if(!input.read(supercompressionGlobalData.data(), header.sgdByteLength))
    {
      return "Reading supercompressionGlobalData failed.";
    }
  }

// Initialize supercompression
#ifdef NVP_SUPPORTS_ZSTD
  ScopedZstdDContext zstdDCtx;
#endif
  if(header.supercompressionScheme == 1)
  {
    return "BasisLZ supercompression isn't currently supported.";  // TODO
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

  // First initialize the output:
  UNWRAP_ERROR(allocate(num_mips_possibly_0, num_layers_possibly_0, num_faces));
  // Temporary buffers used for supercompressed data.
  std::vector<char> supercompressedData;
  std::vector<char> inflatedData;
  // Traverse mips in reverse order following the spec
  for(int mip = numMipsToRead - 1; mip >= 0; mip--)
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

    size_t inflatedFaceSize = 0;
    UNWRAP_ERROR(ExportSizeExtended(mipWidth, mipHeight, mipDepth, inflatedVkFormat, inflatedFaceSize, readSettings.custom_size_callback));

    // If the data is supercompressed, we need to deflate the entire mip, then
    // separate the faces. Otherwise, we can copy the data directly.

    if(header.supercompressionScheme != 0)
    {
      // Validate sizes
      if(readSettings.validate_input_size)
      {
        if(levelIndex.byteLength > validation_input_size)
        {
          return "The KTX2 file said that level " + std::to_string(mip) + " contained "
                 + std::to_string(levelIndex.byteLength) + " bytes of supercompressed data, but the file was only "
                 + std::to_string(validation_input_size) + " bytes long!";
        }
      }

      // Is supercompressed
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

      // Check size ahead of time to ensure we don't read out of bounds.
      // This would otherwise result in an access violation on
      // invalid_face_count_and_padding.ktx2, or on an otherwise truncated file.
      const size_t inflatedDataSize           = inflatedData.size();
      const size_t expected_bytes_in_this_mip = inflatedFaceSize * size_t(header.layerCount) * size_t(header.faceCount);
      if(expected_bytes_in_this_mip > inflatedDataSize)
      {
        return "Expected " + std::to_string(expected_bytes_in_this_mip) + " bytes in mip " + std::to_string(mip)
               + ", but the inflated data was only " + std::to_string(inflatedDataSize) + " bytes long.";
      }

      // Unpack from inflatedData to the individual faces by iterating
      // over levelImages
      size_t inflatedDataPos = 0;  // Read position in inflatedData
      for(uint32_t layer = 0; layer < header.layerCount; layer++)
      {
        for(uint32_t face = 0; face < header.faceCount; face++)
        {
          std::vector<char>& subresource_data = subresource(mip, layer, face);
          UNWRAP_ERROR(ResizeVectorOrError(subresource_data, inflatedFaceSize));
          memcpy(subresource_data.data(), &inflatedData[inflatedDataPos], inflatedFaceSize);
          // Advance to next image
          inflatedDataPos += inflatedFaceSize;
        }
      }
    }
    else
    {
      // No supercompression; iterate over levelImages, reading directly from
      // the file.

      // Validate sizes
      if(readSettings.validate_input_size)
      {
        if((validation_input_size / size_t(header.layerCount)) / size_t(header.faceCount) < inflatedFaceSize)
        {
          return "The KTX2 file said it contained " + std::to_string(header.layerCount) + " array elements and "
                 + std::to_string(header.faceCount) + " faces in mip " + std::to_string(mip)
                 + ", but the input was too short (" + std::to_string(validation_input_size)
                 + " bytes) to contain that!";
        }
      }

      for(uint32_t layer = 0; layer < header.layerCount; layer++)
      {
        for(uint32_t face = 0; face < header.faceCount; face++)
        {
          std::vector<char>& subresource_data = subresource(mip, layer, face);
          UNWRAP_ERROR(ResizeVectorOrError(subresource_data, inflatedFaceSize));
          if(!input.read(subresource_data.data(), inflatedFaceSize))
          {
            return "Reading data for mip " + std::to_string(mip) + " layer " + std::to_string(layer) + " face "
                   + std::to_string(face) + " from the stream failed. Is the stream truncated?";
          }
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

  const std::streampos start_pos = output.tellp();

  // Some dimension fields can be 0 to indicate the texture type; create copies
  // of them where these 0s have been changed to 1s for indexing.
  const uint32_t num_mips_or_1   = std::max(num_mips_possibly_0, 1u);
  const uint32_t num_layers_or_1 = std::max(num_layers_possibly_0, 1u);

  // Allocate the header and Level Index.
  KTX2TopLevelHeader      header{};
  std::vector<LevelIndex> levelIndex(num_mips_or_1);  // "mip offsets"

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
      dfSamples[0].channelType = KHR_DF_CHANNEL_BC6H_COLOR | KHR_DF_SAMPLE_DATATYPE_FLOAT | KHR_DF_SAMPLE_DATATYPE_SIGNED;
      dfSamples[0].lower       = 0xBF800000u;  // -1.0f
      dfSamples[0].upper       = 0x3F800000u;  // 1.0f
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
      dfSamples[0].channelType = KHR_DF_CHANNEL_BC3_ALPHA | KHR_DF_SAMPLE_DATATYPE_LINEAR;
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
      dfSamples[0].channelType = KHR_DF_CHANNEL_BC2_ALPHA | KHR_DF_SAMPLE_DATATYPE_LINEAR;
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
      dfSamples[3].channelType = KHR_DF_CHANNEL_RGBSDA_ALPHA | KHR_DF_SAMPLE_DATATYPE_LINEAR;
      dfSamples[3].upper       = 255;
      break;
    case VK_FORMAT_R16_SFLOAT:
      // R16
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = sizeof(uint16_t);

      dfSamples[0].bitOffset   = 0;
      dfSamples[0].bitLength   = 15;  // "16"
      dfSamples[0].channelType = KHR_DF_CHANNEL_RGBSDA_RED | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
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

      dfSamples[0].channelType = KHR_DF_CHANNEL_RGBSDA_RED | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
      dfSamples[1].channelType = KHR_DF_CHANNEL_RGBSDA_GREEN | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
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

      dfSamples[0].channelType = KHR_DF_CHANNEL_RGBSDA_RED | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
      dfSamples[1].channelType = KHR_DF_CHANNEL_RGBSDA_GREEN | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
      dfSamples[2].channelType = KHR_DF_CHANNEL_RGBSDA_BLUE | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
      dfSamples[3].channelType = KHR_DF_CHANNEL_RGBSDA_ALPHA | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
      break;
    case VK_FORMAT_R32_SFLOAT:
      // R16
      dfdBlock.colorModel           = KHR_DF_MODEL_RGBSDA;
      dfdBlock.colorPrimaries       = KHR_DF_PRIMARIES_BT709;
      dfdBlock.texelBlockDimension0 = 0;
      dfdBlock.texelBlockDimension1 = 0;
      dfdBlock.bytesPlane0          = sizeof(uint32_t);

      dfSamples[0].bitOffset   = 0;
      dfSamples[0].bitLength   = 31;  // "32"
      dfSamples[0].channelType = KHR_DF_CHANNEL_RGBSDA_RED | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
      dfSamples[0].lower       = 0xBF800000u;  // -1.0f
      dfSamples[0].upper       = 0x3F800000u;  // 1.0f
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

      dfSamples[0].channelType = KHR_DF_CHANNEL_RGBSDA_RED | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
      dfSamples[1].channelType = KHR_DF_CHANNEL_RGBSDA_GREEN | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
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

      dfSamples[0].channelType = KHR_DF_CHANNEL_RGBSDA_RED | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
      dfSamples[1].channelType = KHR_DF_CHANNEL_RGBSDA_GREEN | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
      dfSamples[2].channelType = KHR_DF_CHANNEL_RGBSDA_BLUE | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
      dfSamples[3].channelType = KHR_DF_CHANNEL_RGBSDA_ALPHA | KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
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

  // Also overwrite the swizzle.
  if(false)
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
    if(supercompressLevel < zstdMinLevel)
      supercompressLevel = zstdMinLevel;
    if(supercompressLevel > zstdMaxLevel)
      supercompressLevel = zstdMaxLevel;
#else
    return "Zstandard supercompression was selected for KTX2 writing, but nv_ktx was built without Zstd!";
#endif
  }

  // Write mips from smallest to largest.
  for(int64_t mip = static_cast<int64_t>(num_mips_or_1) - 1; mip >= 0; mip--)
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
        for(size_t layer = 0; layer < num_layers_or_1; layer++)
        {
          for(size_t face = 0; face < num_faces; face++)
          {
            const std::vector<char>& this_subresource = subresource(mip, layer, face);
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
      const size_t      fullMipUncompressedLength = levelIndex[mip].uncompressedByteLength;
      const size_t      supercompressedMaxSize    = ZSTD_COMPRESSBOUND(fullMipUncompressedLength);
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
                                           rawData.data(), rawData.size(), writeSettings.supercompression_level);
      if(ZSTD_isError(errOrSize))
      {
        return "Zstandard supercompression returned error " + std::to_string(errOrSize) + ".";
      }

      if(errOrSize > supercompressedData.size())
      {
        assert(false);  // This should never happen
        return "ZSTD_compressCCtx returned a number that was larger than the size of the supercompressed data buffer.";
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
  header.levelCount  = num_mips_possibly_0;
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