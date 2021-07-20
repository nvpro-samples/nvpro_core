/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */
//-----------------------------------------------------------------------------
// A mostly self-contained reader and writer for KTX2 files and reader for KTX1
// files. Relies on Vulkan (for KTX2), GL (for KTX1), and the
// Khronos Data Format.
//
// Sample usage for reading files:
// KTXImage image;
// ErrorWithText maybe_error = image.readFromFile("data/image.ktx2");
// if(maybe_error.has_value())
// {
//   // Do something with the error message, maybe_error.value()
// }
// else
// {
//   // Access subresources using image.subresource(...), and upload them
//   // to the GPU using your graphics API of choice.
// }
//-----------------------------------------------------------------------------

#ifndef __NV_KTX_H__
#define __NV_KTX_H__

#include <array>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace nv_ktx {
// These functions return an empty std::optional if they succeeded, and a
// value with text describing the error if they failed.
using ErrorWithText = std::optional<std::string>;

// KTX files can store key/value pairs, where the key is a UTF-8
// null-terminated string and the value is an arbitrary byte array
// (but often a null-terminated ASCII string).
using KeyValueData = std::map<std::string, std::vector<char>>;

// Apps can define custom functions that return the size in bytes of new
// VkFormats. Functions of this type should take in the width, height, and
// depth of a format in the first 3 parameters, the VkFormat in the 4th, and
// return the size in bytes of an image with those dimensions in the last
// parameter. Passing in an image size of (1, 1, 1) should give the size of
// the smallest possible nonzero image. If the format is unknown, it should
// return a string; if it succeeds, it should return {}.
using CustomExportSizeFuncPtr = ErrorWithText (*)(size_t, size_t, size_t, VkFormat, size_t&);

// Configurable settings for reading files. This is a struct so that it can
// be extended in the future.
struct ReadSettings
{
  // Whether to read all mips (true), or only the base mip (false).
  bool mips = true;
  // See docs for CustomExportSizeFuncPtr
  CustomExportSizeFuncPtr custom_size_callback = nullptr;
  // If true, the reader will validate that the KTX file contains at least 1
  // byte per subresource. This will involve seeking to the end of the stream
  // to determine the length of the stream or file.
  bool validate_input_size = true;
};

enum class WriteSupercompressionType
{
  NONE,  // Apply no supercompression
  ZSTD   // ZStandard
};

// Configurable settings for writing files. This is a struct so that it can
// be extended in the future.
struct WriteSettings
{
  // Type of supercompression to apply if any
  WriteSupercompressionType supercompression = WriteSupercompressionType::NONE;
  // Supercompression type-specific quality level.
  int supercompression_level = 0;
  // See docs for CustomExportSizeFuncPtr
  CustomExportSizeFuncPtr custom_size_callback = nullptr;
};

// An enum for each of the possible elements in a ktxSwizzle value.
enum class KTX_SWIZZLE
{
  ZERO = 0,
  ONE,
  R,
  G,
  B,
  A
};

// Represents the inflated contents of a KTX or KTX2 file. This includes:
// - the VkFormat of the image data,
// - the formatted (i.e. encoded/compressed) image data for
// each element, mip level, and face,
// - and the table of key/value pairs.
// The stored data is not supercompressed, as we supercompress and inflate when
// writing and reading to and from KTX files.
struct KTXImage
{
public:
  // Clears, then sets up storage for an image with the given dimensions. These
  // can be set to 0 instead of 1 along each dimension to indicate different
  // texture types, such as 1D or 2D. See table 4.1 in the KTX 2.0
  // specification, or the comments on these variables below.
  //
  // Width, height, depth, and VkFormat should be set manually using the
  // member variables. This does not allocate the encoded subresources.
  // This can fail e.g. if the parameters are so large that the app runs out of
  // memory when allocating space.
  ErrorWithText allocate(
      // The number of mips (levels) in the image, including the base mip.
      uint32_t _num_mips = 1,
      // The number of array elements (layers) in the image. 0 for a non-array
      // texture (this has meaning in OpenGL, but not in Vulkan).
      // If representing an incomplete cube map (i.e. a cube map where not all
      // faces are stored), this is
      //   (faces per cube map) * (number of cube maps)
      // and _num_faces is 1.
      uint32_t _num_layers = 0,
      // The number of faces in the image (1 for a 2D texture, 6 for a cube map)
      uint32_t _num_faces = 1);

  // Clears all stored image and table data.
  void clear();

  // Determines the VkImageType corresponding to this KTXImage based on the
  // dimensions, according to Table 4.1 of the KTX 2.0 specification.
  // In the invalid case where mip_0_width == 0, returns VK_IMAGE_TYPE_1D.
  VkImageType getImageType() const;

  // Returns whether the loaded file was a KTX1 (1) or KTX2 (2) file.
  uint32_t getKTXVersion() const;

  // Mutably accesses the subresource at the given mip, layer, and face. If the
  // given indices are out of range, throws an std::out_of_range exception.
  std::vector<char>& subresource(uint32_t mip = 0, uint32_t layer = 0, uint32_t face = 0);

  // Reads this structure from a KTX stream, advancing the stream as well.
  // Returns an optional error message if the read failed.
  ErrorWithText readFromStream(std::istream&       input,          // The input stream, at the start of the KTX data
                               const ReadSettings& readSettings);  // Settings for the reader

  // Wrapper for readFromStream for a filename.
  ErrorWithText readFromFile(const char*         filename,       // The .ktx or .ktx2 file to read from.
                             const ReadSettings& readSettings);  // Settings for the reader

  // Writes this structure in KTX2 format to a stream.
  ErrorWithText writeKTX2Stream(std::ostream&        output,  // The output stream, at the point to start writing
                                const WriteSettings& writeSettings);  // Settings for the writer

  // Wrapper for writeKTX2Stream for a filename. Customarily, the filename ends
  // in .ktx2.
  ErrorWithText writeKTX2File(const char*          filename,        // The output stream, at the point to start writing
                              const WriteSettings& writeSettings);  // Settings for the writer

public:
  // These members can be freely modified.

  // The format of the data in this image. When reading a KTX1 file (which
  // specifies a GL format), we automatically convert to a VkFormat.
  VkFormat format = VK_FORMAT_UNDEFINED;
  // The width in pixels of the largest mip. Must be > 0.
  uint32_t mip_0_width = 1;
  // The height in pixels of the largest mip. 0 for a 1D texture.
  uint32_t mip_0_height = 0;
  // The depth in pixels of the largest mip. 0 for a 1D or 2D texture.
  uint32_t mip_0_depth = 0;
  // The number of mips (levels) in the image, including the base mip.
  // 0 indicates that an app should generate additional mips.
  uint32_t num_mips_possibly_0 = 1;
  // The number of array elements (layers) in the image. 0 for a non-array
  // texture (this has meaning in OpenGL, but not in Vulkan).
  // If representing an incomplete cube map (i.e. a cube map where not all
  // faces are stored), this is
  //   (faces per cube map) * (number of cube maps)
  // and _num_faces is 1.
  uint32_t num_layers_possibly_0 = 0;
  // The number of faces in the image (1 for a 2D texture, 6 for a cube map)
  uint32_t num_faces = 0;
  // This file's key/value table. Note that for the ktxSwizzle key, one should
  // use the swizzle element instead!
  KeyValueData key_value_data;

  // KTX files can set the number of mips to 0 to indicate that
  // the application should generate a full mip chain.
  bool app_should_generate_mips = false;

  // Whether this data represents an image with premultiplied alpha
  // (generally, storing (r*a, g*a, b*a, a) instead of (r, g, b, a)).
  // This is used when writing the Data Format Descriptor in KTX2.
  bool is_premultiplied = false;

  // Whether the Data Format Descriptor transferFunction for this data is
  // KHR_DF_TRANSFER_SRGB. (Otherwise, it is KHR_DF_TRANSFER_LINEAR.)
  // More informally, says "when a GPU accesses this texture, should it perform
  // sRGB-to-linear conversion". For instance, this is usually true for color
  // textures, and false for normal maps and depth maps. Validation requires
  // this to match the VkFormat - except in special cases such as Basis UASTC
  // and Universal.
  bool is_srgb = true;

  // Specifies how the red, green, blue, and alpha channels should be sampled
  // from the source data. For instance, {R, G, ZERO, ONE} means the red and
  // green channels should be sampled from the red and green texture components
  // respectively, the blue channel is sampled as 0, and the alpha channel is
  // sampled as 1.
  // Note that values here should be read in lieu of the key_value_data's
  // ktxSwizzle key! This is to make Basis Universal usage easier in the future.
  std::array<KTX_SWIZZLE, 4> swizzle = {KTX_SWIZZLE::R, KTX_SWIZZLE::G, KTX_SWIZZLE::B, KTX_SWIZZLE::A};

private:
  // Private functions used by readFromStream after it determines whether the
  // stream is a KTX1 or KTX2 stream.
  ErrorWithText readFromKTX1Stream(std::istream& input, const ReadSettings& readSettings);
  ErrorWithText readFromKTX2Stream(std::istream& input, const ReadSettings& readSettings);

  // Whether the loaded file was a KTX1 (1) or KTX2 (2) file.
  uint32_t read_ktx_version = 1;

private:
  // A structure containing all the image's encoded, non-supercompressed
  // image data. We store this in a buffer with an entry per subresource, and
  // provide accessors to it.
  std::vector<std::vector<char>> data;
};

}  // namespace nv_ktx

#include "nv_ktx.inl"

#endif