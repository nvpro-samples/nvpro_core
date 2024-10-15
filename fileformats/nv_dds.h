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

//////////////////////////////////////////////////////////////////////////
/** @DOC_START
# nv_dds 2.1.0
     
> A small yet complete library for reading and writing DDS files.

Other than the C++ standard library, nv_dds only requires five files:
dxgiformat.h, nv_dds.h, nv_dds.cpp, texture_formats.h, and texture_formats.cpp.

To load a DDS file, use `Image::readFromFile()`:

```cpp
nv_dds::Image image;
nv_dds::ErrorWithText maybeError = image.readFromFile("data/image.dds", {});
if(maybeError.has_value())
{
  // Do something with the error message, maybeError.value()
}
else
{
  // Access subresources using image.subresource(...), and upload them
  // to the GPU using your graphics API of choice.
}
```

`Image`'s format field is a DXGI format. If you need to use this data with
another API, you can use the functions in texture_formats.h to look up
the corresponding API format.

To write a DDS file, use `Image::writeToFile()`:

```cpp
nv_dds::ErrorWithText maybe_error = image.writeToFile("output.dds", {});
if(maybe_error.has_value())
{
  LOGE("Failed to write to output.dds! The DDS writer reported: %s\n",
       maybe_error.value().c_str());
}
```

`Image` also provides functions to read and write streams. Each of these
read and write functions supports various settings; see `ReadSettings`
and `WriteSettings`.

Images can also be created from raw data:
```cpp
nv_dds::Image image;
image.allocate(1,  // _numMips
               1,  // _numLayers
               1); // _numFaces
image.subresource(0, 0, 0) // mip, layer, face
  .create(512, 512, 1, subresource_data); // width, height, depth, data
  = nv_dds::Subresource(512, 512, 1, subresource_data); // width, height, depth, data
```

## Limitations

Not currently supported:
* Multi-plane YUV textures with chroma subsampling
  (e.g. `DXGI_FORMAT_R8G8_B8G8_UNORM`)
* Paletted textures
* DirectDraw Surface versions before DX9

## Changes from nv_dds 1.0

nv_dds adds support for many more formats, fixes many issues, and now aims
to be secure against untrusted input, so it's worth updating. However, the API
has almost entirely changed, and now looks much more like nv_ktx's API:

* `CSurface` is now `nv_dds::Subresource`
* `CDDSImage` is now `nv_dds::Image`
* `CTexture` (representing a mip chain) has been moved to part of `Image`.
* Images can no longer be flipped. It turns out the code for this only worked
for mips whose height was a multiple of the block size; in all other cases,
flipping a block-compressed image across the y axis requires decompressing
and recompressing it, which is outside the scope of nv_dds. (We recommend
flipping your sampling y axis instead; if flipping compressed image data is
absolutely necessary, consider using a library such as NVTT.)

-- @DOC_END */

#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace nv_dds {

struct DDSPixelFormat
{
  uint32_t dwSize;
  uint32_t dwFlags;
  uint32_t dwFourCC;
  uint32_t dwRGBBitCount;
  uint32_t dwRBitMask;
  uint32_t dwGBitMask;
  uint32_t dwBBitMask;
  uint32_t dwABitMask;
};

struct DDSHeader
{
  uint32_t       dwSize;
  uint32_t       dwFlags;
  uint32_t       dwHeight;
  uint32_t       dwWidth;
  uint32_t       dwPitchOrLinearSize;
  uint32_t       dwDepth;
  uint32_t       dwMipMapCount;
  uint32_t       dwReserved1[11];
  DDSPixelFormat ddspf;
  uint32_t       dwCaps1;
  uint32_t       dwCaps2;
  uint32_t       dwReserved2[3];
};

enum class ResourceDimension : uint32_t
{
  eUnknown,
  eBuffer,
  eTexture1D,
  eTexture2D,
  eTexture3D,
  eCount
};

struct DDSHeaderDX10
{
  uint32_t          dxgiFormat;
  ResourceDimension resourceDimension;
  uint32_t          miscFlag;
  uint32_t          arraySize;
  uint32_t          miscFlags2;
};

// Some DDS writers store their writer IDs and versions in fields of the DDS
// header. WriterLibrary and WriterLibraryVersion enumerate writers that nv_dds
// can recognize:
enum class WriterLibrary : uint32_t
{
  eUnknown,
  eNVTT,          // NVIDIA Texture Tools core library
  eNVTTExporter,  // NVIDIA Texture Tools Exporter
  eNVPS,          // This library
  eGIMP           // GNU Image Manipulation Program
};

// Returns the name of a writer library.
// For WriterLibrary::eUnknown, returns "Unknown".
const char* getWriterLibraryString(WriterLibrary writerLibrary);

// Some DDS files store their data in special encodings before compression.
// The reader will try to find and identify these if it can; the app will need
// to implement its own logic to convert from the stored data to the
// true colors.
// Although it's technically possible for a file to mix these by, say,
// combining the YUV flag with the RXGB FourCC code, only one of these are
// usually present at a time in real files.
enum class ColorTransform
{
  eNone,
  // This image stores luminance values. This distinguishes between R8 and L8.
  eLuminance,
  // Also known as RXGB; the red and alpha channel are swapped. This appears
  // often when encoding normals in BC3 textures; because the two components
  // of normals are usually not very correlated, and the RGB and A channels are
  // encoded separately in BC3, this gives higher-quality results than, say,
  // using BC1.
  // Nowadays, a better alternative is to use BC5.
  eAGBR,
  // The image uses the DDPF_YUV flag. @nbickford is unsure which primaries
  // this uses.
  eYUV,
  // The image stores data in the YCoCg color model.
  // See nvtt::fromYCoCg().
  eYCoCg,
  // The image stores data in a scaled YCoCg format.
  // This isn't quite the same as nvtt::blockScaleCoCg(), which operates on
  // 4x4 blocks. Instead, when decoding, multiply the Co and Cg terms by
  // 1.f / (blue / 8.0f + 1.0f)
  // where blue ranges from 0-255, before further decoding YCoCg.
  eYCoCgScaled,
  // The image stores data where the alpha channel acts as a scaling factor.
  // Multiply the R, G, and B channels by the A channel when decoding.
  // Note that although this stands for "alpha exponent", it's different than
  // the RGBE encodings used by the .hdr file format and NVTT, and the A
  // channel is only a scaling factor.
  eAEXP,
  // Swap the red and green channels.
  eSwapRG,
  // Reconstruct the blue channel from the red and green channels using
  // sqrt(1-r^2-g^2).
  // This usually appears with an SNORM format; e.g. D3DFMT_CxV8U8 is
  // R8G8_SNORM + this reconstruction.
  // Writing is only supported when using R8G8_SNORM without the DX10 extension.
  eOrthographicNormal
};
// Returns the ColorTransform in string form. Returns "?" if out-of-bounds.
const char* getColorTransformString(ColorTransform transform);

// miscFlags2 enumeration: Alpha modes
const uint32_t DDS_ALPHA_MODE_UNKNOWN       = 0x0;
const uint32_t DDS_ALPHA_MODE_STRAIGHT      = 0x1;
const uint32_t DDS_ALPHA_MODE_PREMULTIPLIED = 0x2;
const uint32_t DDS_ALPHA_MODE_OPAQUE        = 0x3;
const uint32_t DDS_ALPHA_MODE_CUSTOM        = 0x4;
// Returns the alpha mode in string form. Returns "?" if out-of-bounds.
const char* getAlphaModeString(uint32_t alphaMode);

// These functions return an empty std::optional if they succeeded, and a
// value with text describing the error if they failed.
// Functions that return these also will do their best not to throw exceptions.
// (Constructing an std::string from a constant string could throw an exception
// if memory is extremely low.)
using ErrorWithText = std::optional<std::string>;

// Represents a single 3D image subresource.
struct Subresource
{
  // Default constructor.
  Subresource() = default;
  // Frees existing data and reallocates or copies data.
  // Specifically, if `pixels` is nullptr, `data` will be zero-initialized and
  // set to the given length in bytes. Otherwise, `data` will be copied from
  // `pixels`.
  // Returns an error message if imageSizeBytes is 0.
  ErrorWithText create(size_t imageSizeBytes, const void* pixels);
  // Frees image data and resets the size.
  void clear();

  std::vector<char> data;  // The image's raw data.
};

// Contains all the settings for reading DDS files.
struct ReadSettings
{
  // The maximum size for any subresource in bytes.
  // This prevents resource exhaustion attacks; otherwise, it's possible for
  // a stream to list an impossibly large size.
  size_t maxSubresourceSizeBytes = sizeof(float) * 4ULL * 65536ULL * 32768ULL;
  // If true, the reader will validate that the DDS file can plausibly contain
  // the data it says it contains. This will involve seeking to the end of the
  // stream to determine the length of the data.
  bool validateInputSize = true;
  // Whether mips should be read beyond the base mip.
  bool mips = true;
  // Makes the DDS reader always decompress bitmasked DDS files to
  // DXGI_FORMAT_R32G32B32A32_SFLOAT.
  //
  // By default, if a DDS file uses bitmasks and each component stores
  // <= 8 bits of data, the DDS reader decompresses to
  // DXGI_FORMAT_R8G8B8A8_UNORM; it only decompresses to
  // DXGI_FORMAT_R32G32B32A32_SFLOAT if a component has more bits.
  // However, if you know you're always converting the output to RGBAF32, then
  // you can skip a conversion by setting this to true.
  bool bitmaskForceRgbaF32 = false;
};

struct WriteSettings
{
  // If set to true, the writer will try to always use the DX10 header
  // extension, which is more precise, if the format settings allow it.
  //
  // By default (false), the writer will prefer DX9-style headers for BC1-BC3
  // and some uncompressed DXGI formats, since these are more compatible with
  // older readers (including nv_dds 1.0). However, newer formats like BC7
  // can't be expressed in DX9-style headers (although there are some obscure
  // FourCC codes for them), and will always use DX10.
  bool useDx10HeaderIfPossible = false;

  // The legacy version of the NVIDIA Texture Tools Exporter implemented
  // floating-point textures by copying certain numbers into the FourCC codes.
  // This is undocumented - but since the old plugin has been around since
  // 2004ish, the new version also needs to reproduce this behavior when
  // writing a DX9 header. (Note that these are not DXGI formats.)
  bool legacyNvtteStyleFloatCodes = false;

  // If set to `true`, the writer will ignore the Image's `format` field,
  // and will instead use a DX9 header with the bitmasks given below.
  // dwRGBBitCount, the RGB flag of dwFlags, and the ALPHA flag of dwFlags will
  // be automatically set based on the bitmask values.
  // This also overrides `useDx10HeaderIfPossible`.
  bool     useCustomBitmask = false;
  uint32_t bitmaskR         = 0;
  uint32_t bitmaskG         = 0;
  uint32_t bitmaskB         = 0;
  uint32_t bitmaskA         = 0;
};

// Represents a full image (with optional cubemap faces and mips) as contents
// of a DDS file. Includes the format in which the data is stored.
// Various flags describe how the image should be interpreted by readers,
// without fully decompressing the image.
struct Image
{
public:
  // Clears, then sets up the vector of subresources for an image with the
  // given dimensions. These must all be greater than 0, and _numMips must be
  // less than 32.
  // This can fail e.g. if the parameters are so large that the app runs out of
  // memory when allocating space.
  ErrorWithText allocate(
      // The number of mips (levels) in the image, including the base mip.
      uint32_t _numMips = 1,
      // The number of array elements (layers) in the image.
      uint32_t _numLayers = 1,
      // The number of faces in the image (1 for a 2D texture, 6 for a complete
      // cube map; if storing an incomplete cube map, don't forget to set
      // `cubemap_face_flags`.
      uint32_t _numFaces = 1);

  // Clears all stored image data. Does not clear properties.
  // allocate() must be called again before one can access subresources.
  void clear();

  // Returns the intended DirectX resource dimension of this texture.
  // If this wasn't specified in the DX10 header, it's inferred from whether
  // any of the dimensions of the base mip were 0.
  // If you want the raw resource dimension, access
  // `resource_dimension` directly.
  ResourceDimension inferResourceDimension() const;

  // Mutably accesses the subresource at the given mip, layer, and face. If the
  // given indices are out of range, throws an std::out_of_range exception.
  const Subresource& subresource(uint32_t mip = 0, uint32_t layer = 0, uint32_t face = 0) const;
  Subresource&       subresource(uint32_t mip = 0, uint32_t layer = 0, uint32_t face = 0);

  // Reads only the header of this structure from a DDS stream, advancing the
  // stream as well.
  // Note that for bitmasked data, m_fileInfo.wasBitmasked will be set,
  // and dxgiFormat will be the format the bitmasked data will be decompressed
  // to.
  ErrorWithText readHeaderFromStream(std::istream&       input,  // The input stream, at the start of the DDS data.
                                     const ReadSettings& readSettings);  // Settings for the reader.

  // Wrapper for readHeaderFromStream for a file.
  ErrorWithText readHeaderFromFile(const char*         filename,       // The .dds file to read from.
                                   const ReadSettings& readSettings);  // Settings for the reader.

  // Wrapper for readHeaderFromStream for a buffer in memory.
  ErrorWithText readHeaderFromMemory(const char*         buffer,         // The buffer in memory.
                                     size_t              bufferSize,     // Its length in bytes.
                                     const ReadSettings& readSettings);  // Settings for the reader.


  // Reads this structure from a DDS stream, advancing the stream as well.
  // Returns an optional error message if the read failed.
  // Bitmasked data will be automatically decompressed.
  ErrorWithText readFromStream(std::istream&       input,          // The input stream, at the start of the DDS data.
                               const ReadSettings& readSettings);  // Settings for the reader.

  // Wrapper for readFromStream for a file.
  ErrorWithText readFromFile(const char*         filename,       // The .dds file to read from.
                             const ReadSettings& readSettings);  // Settings for the reader.

  // Wrapper for readFromStream for a buffer in memory.
  ErrorWithText readFromMemory(const char*         buffer,         // The buffer in memory.
                               size_t              bufferSize,     // Its length in bytes.
                               const ReadSettings& readSettings);  // Settings for the reader.


  // Writes this structure in DDS format to a stream.
  ErrorWithText writeToStream(std::ostream&        output,          // The output stream, at the point to start writing
                              const WriteSettings& writeSettings);  // Settings for the writer

  // Wrapper for writeToStream for a filename. Customarily, the filename ends
  // in .dds.
  ErrorWithText writeToFile(const char*          filename,        // The output stream, at the point to start writing
                            const WriteSettings& writeSettings);  // Settings for the writer


  // Returns the width of the given mip.
  // Always greater than or equal to 1.
  inline uint32_t getWidth(uint32_t mip) const
  {
    assert(mip < 32);
    const uint32_t shiftResult = mip0Width >> mip;
    return (shiftResult == 0 ? 1 : shiftResult);
  }

  // Returns the height of the given mip.
  // Always greater than or equal to 1.
  inline uint32_t getHeight(uint32_t mip) const
  {
    assert(mip < 32);
    const uint32_t shiftResult = mip0Height >> mip;
    return (shiftResult == 0 ? 1 : shiftResult);
  }

  // Returns the depth of the given mip.
  // Always greater than or equal to 1.
  inline uint32_t getDepth(uint32_t mip) const
  {
    assert(mip < 32);
    const uint32_t shiftResult = mip0Depth >> mip;
    return (shiftResult == 0 ? 1 : shiftResult);
  }

  // Returns the size of the base mip of the image in bytes.
  // Note that the image must have been completely loaded first.
  // Always greater than or equal to 1.
  inline size_t getSize() const
  {
    assert(!m_data.empty());
    return m_data[0].data.size();
  }

  // Returns the number of mips (levels) in the image, including the base mip.
  // Always greater than or equal to 1.
  inline uint32_t getNumMips() const { return m_numMips; }

  // The number of array elements (layers) in the image.
  inline uint32_t getNumLayers() const { return m_numLayers; }

  // The number of faces in the image (1 for a 2D texture, 6 for a complete
  // cube map, some other number for an incomplete cube map).
  inline uint32_t getNumFaces() const { return m_numFaces; }

  // Read-only properties of a DDS file.
  struct FileInfo
  {
    // The original DDS headers.
    // The information below is parsed from the headers.
    DDSHeader     ddsh{};
    DDSHeaderDX10 ddsh10{};

    WriterLibrary writerLibrary = WriterLibrary::eUnknown;
    // Parsing this depends on the library; see formatInfo()'s implementation.
    uint32_t writerLibraryVersion = 0;

    // Whether the DDS file used the DX10 extension.
    bool hadDx10Extension = false;

    // Whether the DDS file was decompressed from bitmasked, non-DX10 data.
    bool wasBitmasked = false;
    // If the texture was bitmasked, whether it had an alpha component.
    bool bitmaskHasAlpha = false;
    // If the texture was bitmasked, whether it had RGB components.
    bool bitmaskHasRgb = false;
    // If the texture was bitmasked, whether it used the "bump du dv" encoding
    // for normal maps.
    bool bitmaskWasBumpDuDv = false;
  };

  // Read-only accessor for some properties of a DDS file.
  inline const FileInfo& getFileInfo() const { return m_fileInfo; }

  // Returns a printable string containing information about the Image.
  std::string formatInfo() const;

  // These members can generally be freely modified.

  // Unlike Vulkan, each of these must be nonzero for a valid Image.
  uint32_t mip0Width  = 0;
  uint32_t mip0Height = 0;
  uint32_t mip0Depth  = 0;

  // The DXGI format of the image. To use this with an API other than DirectX,
  // find the corresponding texture format using texture_formats.h.
  // To write a bitmasked image, set this to UNKNOWN (0), and use
  // WriteSettings' bitmask fields.
  uint32_t dxgiFormat = 0;

  // The type of the image: buffer, 1D texture, 2D texture, or 3D texture.
  // Set this to Unknown to automatically infer it during writing.
  ResourceDimension resourceDimension = ResourceDimension::eUnknown;

  // A bitmask of the faces included in the DDS file, if this is a cubemap.
  // Note that this starts at DDSCAPS2_CUBEMAP_POSITIVEX, not 1!
  uint32_t cubemapFaceFlags = 0;

  // The alpha mode; straight, premultiplied, opaque, or custom.
  // See DDS_ALPHA_MODE.
  // MSDN notes that the D3DX 10 and D3DX 11 libraries can't load DDS files
  // with this set to anything other than 0 (DDS10_ALPHA_MODE_UNKNOWN).
  uint32_t alphaMode = DDS_ALPHA_MODE_STRAIGHT;

  // See ColorTransform.
  ColorTransform colorTransform = ColorTransform::eNone;

  // Whether to write the DDPF_NORMAL flag.
  bool isNormal = false;

  // Some DDS files have a 'user version' field: 'UVER' in dwReserved1[7], and
  // an arbitrary 32-bit integer in dwReserved1[8]. `has_user_version` controls
  // whether to write this integer or whether it existed, and userVersion is
  // its value.
  bool     hasUserVersion = false;
  uint32_t userVersion    = 0;

private:
  uint32_t m_numMips   = 1;
  uint32_t m_numLayers = 1;
  uint32_t m_numFaces  = 1;

  FileInfo m_fileInfo = {};

  // A structure containing all the image's encoded data. We store this in a
  // buffer with an entry per subresource, and provide accessors to it.
  std::vector<Subresource> m_data;
};

//-----------------------------------------------------------------------------
// These values are included for convenience, if you need to visualize the
// contents of the DDS header.

#ifdef NV_DDS_UTILITY_VALUES
// surface description flags
// https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
const uint32_t DDSD_CAPS        = 0x00000001U;
const uint32_t DDSD_HEIGHT      = 0x00000002U;
const uint32_t DDSD_WIDTH       = 0x00000004U;
const uint32_t DDSD_PITCH       = 0x00000008U;
const uint32_t DDSD_PIXELFORMAT = 0x00001000U;
const uint32_t DDSD_MIPMAPCOUNT = 0x00020000U;
const uint32_t DDSD_LINEARSIZE  = 0x00080000U;
const uint32_t DDSD_DEPTH       = 0x00800000U;

// pixel format flags
const uint32_t DDPF_ALPHAPIXELS       = 0x00000001U;
const uint32_t DDPF_ALPHA             = 0x00000002U;
const uint32_t DDPF_FOURCC            = 0x00000004U;
const uint32_t DDPF_PALETTEINDEXED4   = 0x00000008U;
const uint32_t DDPF_PALETTEINDEXEDTO8 = 0x00000010U;
const uint32_t DDPF_PALETTEINDEXED8   = 0x00000020U;
const uint32_t DDPF_RGB               = 0x00000040U;
const uint32_t DDPF_COMPRESSED        = 0x00000080U;
const uint32_t DDPF_RGBTOYUV          = 0x00000100U;
const uint32_t DDPF_YUV               = 0x00000200U;
const uint32_t DDPF_ZBUFFER           = 0x00000400U;
const uint32_t DDPF_PALETTEINDEXED1   = 0x00000800U;
const uint32_t DDPF_PALETTEINDEXED2   = 0x00001000U;
const uint32_t DDPF_ZPIXELS           = 0x00002000U;
const uint32_t DDPF_STENCILBUFFER     = 0x00004000U;
const uint32_t DDPF_ALPHAPREMULT      = 0x00008000U;
// TODO: What was 0x00010000U?
const uint32_t DDPF_LUMINANCE     = 0x00020000U;
const uint32_t DDPF_BUMPLUMINANCE = 0x00040000U;
const uint32_t DDPF_BUMPDUDV      = 0x00080000U;
const uint32_t DDPF_RGBA          = DDPF_RGB | DDPF_ALPHAPIXELS;
// Custom NVTT flags.
const uint32_t DDPF_SRGB   = 0x40000000U;
const uint32_t DDPF_NORMAL = 0x80000000U;

// dwCaps1 flags
const uint32_t DDSCAPS_COMPLEX = 0x00000008U;
const uint32_t DDSCAPS_TEXTURE = 0x00001000U;
const uint32_t DDSCAPS_MIPMAP  = 0x00400000U;

// dwCaps2 flags
const uint32_t DDSCAPS2_CUBEMAP           = 0x00000200U;
const uint32_t DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400U;
const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800U;
const uint32_t DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000U;
const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000U;
const uint32_t DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000U;
const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000U;
const uint32_t DDSCAPS2_CUBEMAP_ALL_FACES = DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX
                                            | DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY
                                            | DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ;
const uint32_t DDSCAPS2_VOLUME = 0x00200000U;

inline constexpr uint32_t MakeFourCC(char c0, char c1, char c2, char c3)
{
  return ((uint32_t)(uint8_t)(c0) | ((uint32_t)(uint8_t)(c1) << 8) | ((uint32_t)(uint8_t)(c2) << 16)
          | ((uint32_t)(uint8_t)(c3) << 24));
}

// The DDS magic number.
const uint32_t FOURCC_DDS = MakeFourCC('D', 'D', 'S', ' ');

// Compressed texture types
const uint32_t FOURCC_DXT1 = MakeFourCC('D', 'X', 'T', '1');
const uint32_t FOURCC_DXT2 = MakeFourCC('D', 'X', 'T', '2');
const uint32_t FOURCC_DXT3 = MakeFourCC('D', 'X', 'T', '3');
const uint32_t FOURCC_DXT4 = MakeFourCC('D', 'X', 'T', '4');
const uint32_t FOURCC_DXT5 = MakeFourCC('D', 'X', 'T', '5');

// Additional FOURCC codes from DirectXTex and other sources
const uint32_t FOURCC_BC4U         = MakeFourCC('B', 'C', '4', 'U');
const uint32_t FOURCC_BC4S         = MakeFourCC('B', 'C', '4', 'S');
const uint32_t FOURCC_BC5U         = MakeFourCC('B', 'C', '5', 'U');
const uint32_t FOURCC_BC5S         = MakeFourCC('B', 'C', '5', 'S');
const uint32_t FOURCC_BC6H         = MakeFourCC('B', 'C', '6', 'H');
const uint32_t FOURCC_BC7L         = MakeFourCC('B', 'C', '7', 'L');
const uint32_t FOURCC_BC70         = MakeFourCC('B', 'C', '7', '\0');
const uint32_t FOURCC_ZOLA         = MakeFourCC('Z', 'O', 'L', 'A');  // Written by NVTT
const uint32_t FOURCC_CTX1         = MakeFourCC('C', 'T', 'X', '1');  // Written by NVTT
const uint32_t FOURCC_RGBG         = MakeFourCC('R', 'G', 'B', 'G');
const uint32_t FOURCC_GRGB         = MakeFourCC('G', 'R', 'G', 'B');
const uint32_t FOURCC_ATI1         = MakeFourCC('A', 'T', 'I', '1');  // aka BC4
const uint32_t FOURCC_ATI2         = MakeFourCC('A', 'T', 'I', '2');  // BC5 with red and green swapped
const uint32_t FOURCC_UYVY         = MakeFourCC('U', 'Y', 'V', 'Y');
const uint32_t FOURCC_YUY2         = MakeFourCC('Y', 'U', 'Y', '2');
const uint32_t FOURCC_AEXP         = MakeFourCC('A', 'E', 'X', 'P');  // Written by GIMP
const uint32_t FOURCC_RXGB         = MakeFourCC('R', 'X', 'G', 'B');  // Written by GIMP
const uint32_t FOURCC_YCOCG        = MakeFourCC('Y', 'C', 'G', '1');  // Written by GIMP
const uint32_t FOURCC_YCOCG_SCALED = MakeFourCC('Y', 'C', 'G', '2');  // Written by GIMP
const uint32_t FOURCC_A2XY         = MakeFourCC('A', '2', 'X', 'Y');  // Written by NVTT
const uint32_t FOURCC_A2D5         = MakeFourCC('A', '2', 'D', '5');  // Written by NVTT

const uint32_t FOURCC_DX10 = MakeFourCC('D', 'X', '1', '0');

// TODO (nbickford): Support FOURCC codes from
// https://docs.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering

// The various NVTT-derived DDS writers store their writer IDs in the
// dwReserved1[9] field. Here's a table of versions that exist so far:
const uint32_t FOURCC_LIBRARY_EXPORTER = MakeFourCC('N', 'V', 'T', '3');
const uint32_t FOURCC_LIBRARY_NVTT     = MakeFourCC('N', 'V', 'T', 'T');
const uint32_t FOURCC_LIBRARY_NVPS     = MakeFourCC('N', 'V', 'P', 'S');
// The Exporter stores a version number in dwReserved1[10]:
const uint32_t LIBRARY_EXPORTER_VERSION_START_THROUGH_2023_1_0 = 0;
const uint32_t LIBRARY_EXPORTER_VERSION_2023_1_1_PLUS          = 1;
// NVTT and NVPS also store version numbers in dwReserved1[10]; see
// formatInfo() for how to parse these.

// GIMP's DDS plugin stores its writer ID in dwReserved1[0:1] and its version
// number in dwReserved[2].
const uint32_t FOURCC_LIBRARY_GIMP_WORD0 = MakeFourCC('G', 'I', 'M', 'P');
const uint32_t FOURCC_LIBRARY_GIMP_WORD1 = MakeFourCC('-', 'D', 'D', 'S');
// GIMP's DDS plugin version works like NVTT and NVPS'.

// The UVER tag, which sometimes appears in dwReserved1[7].
const uint32_t FOURCC_UVER = MakeFourCC('U', 'V', 'E', 'R');

// DDS10 header miscFlag flags
const uint32_t DDS_RESOURCE_MISC_TEXTURECUBE = 0x4l;

// Some DDS writers (e.g. GLI, some modes of DirectXTex, and floating-point
// formats in old versions of NVTT) write out formats by storing their D3D9
// D3DFMT in the FourCC field.
// See https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dformat
// and https://github.com/g-truc/gli/blob/master/gli/dx.hpp
const uint32_t D3DFMT_UNKNOWN             = 0;
const uint32_t D3DFMT_R8G8B8              = 20;
const uint32_t D3DFMT_A8R8G8B8            = 21;
const uint32_t D3DFMT_X8R8G8B8            = 22;
const uint32_t D3DFMT_R5G6B5              = 23;
const uint32_t D3DFMT_X1R5G5B5            = 24;
const uint32_t D3DFMT_A1R5G5B5            = 25;
const uint32_t D3DFMT_A4R4G4B4            = 26;
const uint32_t D3DFMT_R3G3B2              = 27;
const uint32_t D3DFMT_A8                  = 28;
const uint32_t D3DFMT_A8R3G3B2            = 29;
const uint32_t D3DFMT_X4R4G4B4            = 30;
const uint32_t D3DFMT_A2B10G10R10         = 31;
const uint32_t D3DFMT_A8B8G8R8            = 32;
const uint32_t D3DFMT_X8B8G8R8            = 33;
const uint32_t D3DFMT_G16R16              = 34;
const uint32_t D3DFMT_A2R10G10B10         = 35;
const uint32_t D3DFMT_A16B16G16R16        = 36;
const uint32_t D3DFMT_A8P8                = 40;
const uint32_t D3DFMT_P8                  = 41;
const uint32_t D3DFMT_L8                  = 50;
const uint32_t D3DFMT_A8L8                = 51;
const uint32_t D3DFMT_A4L4                = 52;
const uint32_t D3DFMT_V8U8                = 60;
const uint32_t D3DFMT_L6V5U5              = 61;
const uint32_t D3DFMT_X8L8V8U8            = 62;
const uint32_t D3DFMT_Q8W8V8U8            = 63;
const uint32_t D3DFMT_V16U16              = 64;
const uint32_t D3DFMT_A2W10V10U10         = 67;
const uint32_t D3DFMT_D16_LOCKABLE        = 70;
const uint32_t D3DFMT_D32                 = 71;
const uint32_t D3DFMT_D15S1               = 73;
const uint32_t D3DFMT_D24S8               = 75;
const uint32_t D3DFMT_D24X8               = 77;
const uint32_t D3DFMT_D24X4S4             = 79;
const uint32_t D3DFMT_D16                 = 80;
const uint32_t D3DFMT_L16                 = 81;
const uint32_t D3DFMT_D32F_LOCKABLE       = 82;
const uint32_t D3DFMT_D24FS8              = 83;
const uint32_t D3DFMT_D32_LOCKABLE        = 84;
const uint32_t D3DFMT_S8_LOCKABLE         = 85;
const uint32_t D3DFMT_VERTEXDATA          = 100;
const uint32_t D3DFMT_INDEX16             = 101;
const uint32_t D3DFMT_INDEX32             = 102;
const uint32_t D3DFMT_Q16W16V16U16        = 110;
const uint32_t D3DFMT_R16F                = 111;
const uint32_t D3DFMT_G16R16F             = 112;
const uint32_t D3DFMT_A16B16G16R16F       = 113;
const uint32_t D3DFMT_R32F                = 114;
const uint32_t D3DFMT_G32R32F             = 115;
const uint32_t D3DFMT_A32B32G32R32F       = 116;
const uint32_t D3DFMT_CxV8U8              = 117;
const uint32_t D3DFMT_A1                  = 118;
const uint32_t D3DFMT_A2B10G10R10_XR_BIAS = 119;
const uint32_t D3DFMT_BINARYBUFFER        = 199;
#endif  // NV_DDS_UTILITY_VALUES

}  // namespace nv_dds
