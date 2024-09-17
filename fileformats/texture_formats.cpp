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

// We'll use the X macro (https://en.wikipedia.org/wiki/X_macro)
// strategy here; because we have some duplicate formats -- multiple GL format
// combinations can correspond to the same DXGI or Vulkan format, for instance
// -- we'll build tables the first time they're requested.
// In cases where

#include "texture_formats.h"

#include "dxgiformat.h"  // Included in nvpro_core's dxh subproject

#include <bitset>  // For std::hash
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>

// Custom hash for OpenGLFormat
template <>
struct std::hash<texture_formats::OpenGLFormat>
{
  size_t operator()(const texture_formats::OpenGLFormat& c) const
  {
    return std::hash<size_t>()(c.internalFormat ^ std::hash<size_t>()(c.format ^ std::hash<uint32_t>()(c.type)));
  }
};

namespace texture_formats {

// GL enums, included here so that we don't have to include the OpenGL headers,
// since the include path for these may depend on the place in which this is included.
#define NV_GL_BYTE 0x1400
#define NV_GL_UNSIGNED_BYTE 0x1401
#define NV_GL_SHORT 0x1402
#define NV_GL_UNSIGNED_SHORT 0x1403
#define NV_GL_INT 0x1404
#define NV_GL_UNSIGNED_INT 0x1405
#define NV_GL_FLOAT 0x1406
#define NV_GL_HALF_FLOAT 0x140B
#define NV_GL_STENCIL_INDEX 0x1901
#define NV_GL_DEPTH_COMPONENT 0x1902
#define NV_GL_RED 0x1903
#define NV_GL_GREEN 0x1904
#define NV_GL_BLUE 0x1905
#define NV_GL_ALPHA 0x1906
#define NV_GL_RGB 0x1907
#define NV_GL_RGBA 0x1908
#define NV_GL_LUMINANCE 0x1909
#define NV_GL_LUMINANCE_ALPHA 0x190A
#define NV_GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define NV_GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define NV_GL_ALPHA8 0x803C
#define NV_GL_LUMINANCE8 0x8040
#define NV_GL_LUMINANCE8_ALPHA8 0x8045
#define NV_GL_RGB8 0x8051
#define NV_GL_RGB16 0x8054
#define NV_GL_RGBA4 0x8056
#define NV_GL_RGB5_A1 0x8057
#define NV_GL_RGBA8 0x8058
#define NV_GL_RGB10_A2 0x8059
#define NV_GL_RGBA16 0x805B
#define NV_GL_BGR 0x80E0
#define NV_GL_BGRA 0x80E1
#define NV_GL_DEPTH_COMPONENT16 0x81A5
#define NV_GL_DEPTH_COMPONENT24 0x81A6
#define NV_GL_DEPTH_COMPONENT32 0x81A7
#define NV_GL_R8 0x8229
#define NV_GL_R16 0x822A
#define NV_GL_RG8 0x822B
#define NV_GL_RG16 0x822C
#define NV_GL_R16F 0x822D
#define NV_GL_R32F 0x822E
#define NV_GL_RG16F 0x822F
#define NV_GL_RG32F 0x8230
#define NV_GL_RG 0x8227
#define NV_GL_RG_INTEGER 0x8228
#define NV_GL_R8 0x8229
#define NV_GL_R16 0x822A
#define NV_GL_RG8 0x822B
#define NV_GL_RG16 0x822C
#define NV_GL_R16F 0x822D
#define NV_GL_R32F 0x822E
#define NV_GL_RG16F 0x822F
#define NV_GL_RG32F 0x8230
#define NV_GL_R8I 0x8231
#define NV_GL_R8UI 0x8232
#define NV_GL_R16I 0x8233
#define NV_GL_R16UI 0x8234
#define NV_GL_R32I 0x8235
#define NV_GL_R32UI 0x8236
#define NV_GL_RG8I 0x8237
#define NV_GL_RG8UI 0x8238
#define NV_GL_RG16I 0x8239
#define NV_GL_RG16UI 0x823A
#define NV_GL_RG32I 0x823B
#define NV_GL_RG32UI 0x823C
#define NV_GL_UNSIGNED_SHORT_5_6_5 0x8363
#define NV_GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#define NV_GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#define NV_GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#define NV_GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define NV_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define NV_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define NV_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#define NV_GL_DEPTH_STENCIL 0x84F9
#define NV_GL_UNSIGNED_INT_24_8 0x84FA
#define NV_GL_RGBA32F 0x8814
#define NV_GL_RGB32F 0x8815
#define NV_GL_RGBA16F 0x881A
#define NV_GL_RGB16F 0x881B
#define NV_GL_DEPTH24_STENCIL8 0x88F0
#define NV_GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT 0x8A54
#define NV_GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT 0x8A55
#define NV_GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT 0x8A56
#define NV_GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT 0x8A57
#define NV_GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#define NV_GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#define NV_GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#define NV_GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#define NV_GL_R11F_G11F_B10F 0x8C3A
#define NV_GL_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define NV_GL_RGB9_E5 0x8C3D
#define NV_GL_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
#define NV_GL_SRGB8 0x8C41
#define NV_GL_SRGB_ALPHA 0x8C42
#define NV_GL_SRGB8_ALPHA8 0x8C43
#define NV_GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#define NV_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define NV_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define NV_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#define NV_GL_DEPTH_COMPONENT32F 0x8CAC
#define NV_GL_DEPTH32F_STENCIL8 0x8CAD
#define NV_GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD
#define NV_GL_STENCIL_INDEX8 0x8D48
#define NV_GL_RGB565 0x8D62
#define NV_GL_RGBA32UI 0x8D70
#define NV_GL_RGB32UI 0x8D71
#define NV_GL_RGBA16UI 0x8D76
#define NV_GL_RGB16UI 0x8D77
#define NV_GL_RGBA8UI 0x8D7C
#define NV_GL_RGB8UI 0x8D7D
#define NV_GL_RGBA32I 0x8D82
#define NV_GL_RGB32I 0x8D83
#define NV_GL_RGBA16I 0x8D88
#define NV_GL_RGB16I 0x8D89
#define NV_GL_RGBA8I 0x8D8E
#define NV_GL_RGB8I 0x8D8F
#define NV_GL_RED_INTEGER 0x8D94
#define NV_GL_RGB_INTEGER 0x8D98
#define NV_GL_RGBA_INTEGER 0x8D99
#define NV_GL_BGR_INTEGER 0x8D9A
#define NV_GL_BGRA_INTEGER 0x8D9B
#define NV_GL_COMPRESSED_RED_RGTC1_EXT 0x8DBB
#define NV_GL_COMPRESSED_SIGNED_RED_RGTC1_EXT 0x8DBC
#define NV_GL_COMPRESSED_RED_GREEN_RGTC2_EXT 0x8DBD
#define NV_GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT 0x8DBE
#define NV_GL_COMPRESSED_RGBA_BPTC_UNORM_ARB 0x8E8C
#define NV_GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB 0x8E8D
#define NV_GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB 0x8E8E
#define NV_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB 0x8E8F
#define NV_GL_R8_SNORM 0x8F94
#define NV_GL_RG8_SNORM 0x8F95
#define NV_GL_RGB8_SNORM 0x8F96
#define NV_GL_RGBA8_SNORM 0x8F97
#define NV_GL_R16_SNORM 0x8F98
#define NV_GL_RG16_SNORM 0x8F99
#define NV_GL_RGB16_SNORM 0x8F9A
#define NV_GL_RGBA16_SNORM 0x8F9B
#define NV_GL_SR8_EXT 0x8FBD
#define NV_GL_SRG8_EXT 0x8FBE
#define NV_GL_RGB10_A2UI 0x906F
#define NV_GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG 0x9137
#define NV_GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG 0x9138
#define NV_GL_COMPRESSED_R11_EAC 0x9270
#define NV_GL_COMPRESSED_SIGNED_R11_EAC 0x9271
#define NV_GL_COMPRESSED_RG11_EAC 0x9272
#define NV_GL_COMPRESSED_SIGNED_RG11_EAC 0x9273
#define NV_GL_COMPRESSED_RGB8_ETC2 0x9274
#define NV_GL_COMPRESSED_SRGB8_ETC2 0x9275
#define NV_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#define NV_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define NV_GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
#define NV_GL_COMPRESSED_RGBA_ASTC_4x4_KHR 0x93B0
#define NV_GL_COMPRESSED_RGBA_ASTC_5x4_KHR 0x93B1
#define NV_GL_COMPRESSED_RGBA_ASTC_5x5_KHR 0x93B2
#define NV_GL_COMPRESSED_RGBA_ASTC_6x5_KHR 0x93B3
#define NV_GL_COMPRESSED_RGBA_ASTC_6x6_KHR 0x93B4
#define NV_GL_COMPRESSED_RGBA_ASTC_8x5_KHR 0x93B5
#define NV_GL_COMPRESSED_RGBA_ASTC_8x6_KHR 0x93B6
#define NV_GL_COMPRESSED_RGBA_ASTC_8x8_KHR 0x93B7
#define NV_GL_COMPRESSED_RGBA_ASTC_10x5_KHR 0x93B8
#define NV_GL_COMPRESSED_RGBA_ASTC_10x6_KHR 0x93B9
#define NV_GL_COMPRESSED_RGBA_ASTC_10x8_KHR 0x93BA
#define NV_GL_COMPRESSED_RGBA_ASTC_10x10_KHR 0x93BB
#define NV_GL_COMPRESSED_RGBA_ASTC_12x10_KHR 0x93BC
#define NV_GL_COMPRESSED_RGBA_ASTC_12x12_KHR 0x93BD
#define NV_GL_COMPRESSED_RGBA_ASTC_3x3x3_OES 0x93C0
#define NV_GL_COMPRESSED_RGBA_ASTC_4x3x3_OES 0x93C1
#define NV_GL_COMPRESSED_RGBA_ASTC_4x4x3_OES 0x93C2
#define NV_GL_COMPRESSED_RGBA_ASTC_4x4x4_OES 0x93C3
#define NV_GL_COMPRESSED_RGBA_ASTC_5x4x4_OES 0x93C4
#define NV_GL_COMPRESSED_RGBA_ASTC_5x5x4_OES 0x93C5
#define NV_GL_COMPRESSED_RGBA_ASTC_5x5x5_OES 0x93C6
#define NV_GL_COMPRESSED_RGBA_ASTC_6x5x5_OES 0x93C7
#define NV_GL_COMPRESSED_RGBA_ASTC_6x6x5_OES 0x93C8
#define NV_GL_COMPRESSED_RGBA_ASTC_6x6x6_OES 0x93C9
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR 0x93D0
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR 0x93D1
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR 0x93D2
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR 0x93D3
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR 0x93D4
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR 0x93D5
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR 0x93D6
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR 0x93D7
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR 0x93D8
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR 0x93D9
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR 0x93DA
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR 0x93DB
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR 0x93DC
#define NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR 0x93DD
#define NV_GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG 0x93F0
#define NV_GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG 0x93F1

// A table of every combination we handle, in
// (DXGI, VK, glInternalFormat, glFormat, glType) order.
// Since multiple OpenGL formats can map to the same DXGI or Vulkan format:
// - REPVK will be called for repeated Vulkan formats
// - REPBOTH will be called if both repeat
#define FOR_EACH_FORMAT(DO, REPVK, REPBOTH)                                                                                                    \
  DO(DXGI_FORMAT_A8_UNORM, VK_FORMAT_S8_UINT, NV_GL_STENCIL_INDEX8, NV_GL_STENCIL_INDEX, NV_GL_UNSIGNED_BYTE)                                  \
  DO(DXGI_FORMAT_ASTC_10X10_UNORM, VK_FORMAT_ASTC_10x10_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_10x10_KHR, 0, 0)                               \
  DO(DXGI_FORMAT_ASTC_10X10_UNORM_SRGB, VK_FORMAT_ASTC_10x10_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR, 0, 0)                   \
  REPVK(DXGI_FORMAT_ASTC_10X10_TYPELESS, VK_FORMAT_ASTC_10x10_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR, 0, 0)                  \
  DO(DXGI_FORMAT_ASTC_10X5_UNORM, VK_FORMAT_ASTC_10x5_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_10x5_KHR, 0, 0)                                  \
  DO(DXGI_FORMAT_ASTC_10X5_UNORM_SRGB, VK_FORMAT_ASTC_10x5_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR, 0, 0)                      \
  REPVK(DXGI_FORMAT_ASTC_10X5_TYPELESS, VK_FORMAT_ASTC_10x5_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR, 0, 0)                     \
  DO(DXGI_FORMAT_ASTC_10X6_UNORM, VK_FORMAT_ASTC_10x6_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_10x6_KHR, 0, 0)                                  \
  DO(DXGI_FORMAT_ASTC_10X6_UNORM_SRGB, VK_FORMAT_ASTC_10x6_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR, 0, 0)                      \
  REPVK(DXGI_FORMAT_ASTC_10X6_TYPELESS, VK_FORMAT_ASTC_10x6_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR, 0, 0)                     \
  DO(DXGI_FORMAT_ASTC_10X8_UNORM, VK_FORMAT_ASTC_10x8_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_10x8_KHR, 0, 0)                                  \
  DO(DXGI_FORMAT_ASTC_10X8_UNORM_SRGB, VK_FORMAT_ASTC_10x8_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR, 0, 0)                      \
  REPVK(DXGI_FORMAT_ASTC_10X8_TYPELESS, VK_FORMAT_ASTC_10x8_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR, 0, 0)                     \
  DO(DXGI_FORMAT_ASTC_12X10_UNORM, VK_FORMAT_ASTC_12x10_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_12x10_KHR, 0, 0)                               \
  DO(DXGI_FORMAT_ASTC_12X10_UNORM_SRGB, VK_FORMAT_ASTC_12x10_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR, 0, 0)                   \
  REPVK(DXGI_FORMAT_ASTC_12X10_TYPELESS, VK_FORMAT_ASTC_12x10_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR, 0, 0)                  \
  DO(DXGI_FORMAT_ASTC_12X12_UNORM, VK_FORMAT_ASTC_12x12_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_12x12_KHR, 0, 0)                               \
  DO(DXGI_FORMAT_ASTC_12X12_UNORM_SRGB, VK_FORMAT_ASTC_12x12_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR, 0, 0)                   \
  REPVK(DXGI_FORMAT_ASTC_12X12_TYPELESS, VK_FORMAT_ASTC_12x12_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR, 0, 0)                  \
  DO(DXGI_FORMAT_ASTC_4X4_UNORM, VK_FORMAT_ASTC_4x4_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_4x4_KHR, 0, 0)                                     \
  DO(DXGI_FORMAT_ASTC_4X4_UNORM_SRGB, VK_FORMAT_ASTC_4x4_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR, 0, 0)                         \
  REPVK(DXGI_FORMAT_ASTC_4X4_TYPELESS, VK_FORMAT_ASTC_4x4_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR, 0, 0)                        \
  DO(DXGI_FORMAT_ASTC_5X4_UNORM, VK_FORMAT_ASTC_5x4_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_5x4_KHR, 0, 0)                                     \
  DO(DXGI_FORMAT_ASTC_5X4_UNORM_SRGB, VK_FORMAT_ASTC_5x4_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR, 0, 0)                         \
  REPVK(DXGI_FORMAT_ASTC_5X4_TYPELESS, VK_FORMAT_ASTC_5x4_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR, 0, 0)                        \
  DO(DXGI_FORMAT_ASTC_5X5_UNORM, VK_FORMAT_ASTC_5x5_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_5x5_KHR, 0, 0)                                     \
  DO(DXGI_FORMAT_ASTC_5X5_UNORM_SRGB, VK_FORMAT_ASTC_5x5_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR, 0, 0)                         \
  REPVK(DXGI_FORMAT_ASTC_5X5_TYPELESS, VK_FORMAT_ASTC_5x5_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR, 0, 0)                        \
  DO(DXGI_FORMAT_ASTC_6X5_UNORM, VK_FORMAT_ASTC_6x5_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_6x5_KHR, 0, 0)                                     \
  DO(DXGI_FORMAT_ASTC_6X5_UNORM_SRGB, VK_FORMAT_ASTC_6x5_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR, 0, 0)                         \
  REPVK(DXGI_FORMAT_ASTC_6X5_TYPELESS, VK_FORMAT_ASTC_6x5_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR, 0, 0)                        \
  DO(DXGI_FORMAT_ASTC_6X6_UNORM, VK_FORMAT_ASTC_6x6_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_6x6_KHR, 0, 0)                                     \
  DO(DXGI_FORMAT_ASTC_6X6_UNORM_SRGB, VK_FORMAT_ASTC_6x6_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR, 0, 0)                         \
  REPVK(DXGI_FORMAT_ASTC_6X6_TYPELESS, VK_FORMAT_ASTC_6x6_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR, 0, 0)                        \
  DO(DXGI_FORMAT_ASTC_8X5_UNORM, VK_FORMAT_ASTC_8x5_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_8x5_KHR, 0, 0)                                     \
  DO(DXGI_FORMAT_ASTC_8X5_UNORM_SRGB, VK_FORMAT_ASTC_8x5_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR, 0, 0)                         \
  REPVK(DXGI_FORMAT_ASTC_8X5_TYPELESS, VK_FORMAT_ASTC_8x5_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR, 0, 0)                        \
  DO(DXGI_FORMAT_ASTC_8X6_UNORM, VK_FORMAT_ASTC_8x6_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_8x6_KHR, 0, 0)                                     \
  DO(DXGI_FORMAT_ASTC_8X6_UNORM_SRGB, VK_FORMAT_ASTC_8x6_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR, 0, 0)                         \
  REPVK(DXGI_FORMAT_ASTC_8X6_TYPELESS, VK_FORMAT_ASTC_8x6_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR, 0, 0)                        \
  DO(DXGI_FORMAT_ASTC_8X8_UNORM, VK_FORMAT_ASTC_8x8_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_ASTC_8x8_KHR, 0, 0)                                     \
  DO(DXGI_FORMAT_ASTC_8X8_UNORM_SRGB, VK_FORMAT_ASTC_8x8_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR, 0, 0)                         \
  REPVK(DXGI_FORMAT_ASTC_8X8_TYPELESS, VK_FORMAT_ASTC_8x8_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR, 0, 0)                        \
  DO(DXGI_FORMAT_B5G5R5A1_UNORM, VK_FORMAT_R5G5B5A1_UNORM_PACK16, NV_GL_RGB5_A1, NV_GL_RGBA, NV_GL_UNSIGNED_SHORT_5_5_5_1)                     \
  DO(DXGI_FORMAT_B5G6R5_UNORM, VK_FORMAT_R5G6B5_UNORM_PACK16, NV_GL_RGB565, NV_GL_RGB, NV_GL_UNSIGNED_SHORT_5_6_5)                             \
  REPVK(DXGI_FORMAT_B8G8R8A8_TYPELESS, VK_FORMAT_B8G8R8A8_SRGB, NV_GL_SRGB8_ALPHA8, NV_GL_BGRA, NV_GL_UNSIGNED_BYTE)                           \
  DO(DXGI_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM, NV_GL_RGBA8, NV_GL_BGRA, NV_GL_UNSIGNED_BYTE)                                       \
  DO(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, VK_FORMAT_B8G8R8A8_SRGB, NV_GL_SRGB8_ALPHA8, NV_GL_BGRA, NV_GL_UNSIGNED_BYTE)                            \
  REPVK(DXGI_FORMAT_B8G8R8X8_UNORM, VK_FORMAT_B8G8R8A8_UNORM, NV_GL_RGBA8, NV_GL_BGRA, NV_GL_UNSIGNED_BYTE)                                    \
  DO(DXGI_FORMAT_BC1_UNORM, VK_FORMAT_BC1_RGBA_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0, 0)                                         \
  DO(DXGI_FORMAT_BC1_UNORM_SRGB, VK_FORMAT_BC1_RGBA_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, 0, 0)                               \
  REPVK(DXGI_FORMAT_BC1_TYPELESS, VK_FORMAT_BC1_RGBA_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, 0, 0)                              \
  DO(DXGI_FORMAT_BC2_UNORM, VK_FORMAT_BC2_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0, 0)                                              \
  DO(DXGI_FORMAT_BC2_UNORM_SRGB, VK_FORMAT_BC2_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, 0, 0)                                    \
  REPVK(DXGI_FORMAT_BC2_TYPELESS, VK_FORMAT_BC2_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, 0, 0)                                   \
  DO(DXGI_FORMAT_BC3_UNORM, VK_FORMAT_BC3_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0, 0)                                              \
  DO(DXGI_FORMAT_BC3_UNORM_SRGB, VK_FORMAT_BC3_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, 0, 0)                                    \
  REPVK(DXGI_FORMAT_BC3_TYPELESS, VK_FORMAT_BC3_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, 0, 0)                                   \
  DO(DXGI_FORMAT_BC4_SNORM, VK_FORMAT_BC4_SNORM_BLOCK, NV_GL_COMPRESSED_SIGNED_RED_RGTC1_EXT, 0, 0)                                            \
  DO(DXGI_FORMAT_BC4_UNORM, VK_FORMAT_BC4_UNORM_BLOCK, NV_GL_COMPRESSED_RED_RGTC1_EXT, 0, 0)                                                   \
  REPVK(DXGI_FORMAT_BC4_TYPELESS, VK_FORMAT_BC4_UNORM_BLOCK, NV_GL_COMPRESSED_RED_RGTC1_EXT, 0, 0)                                             \
  DO(DXGI_FORMAT_BC5_SNORM, VK_FORMAT_BC5_SNORM_BLOCK, NV_GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT, 0, 0)                                      \
  DO(DXGI_FORMAT_BC5_UNORM, VK_FORMAT_BC5_UNORM_BLOCK, NV_GL_COMPRESSED_RED_GREEN_RGTC2_EXT, 0, 0)                                             \
  REPVK(DXGI_FORMAT_BC5_TYPELESS, VK_FORMAT_BC5_UNORM_BLOCK, NV_GL_COMPRESSED_RED_GREEN_RGTC2_EXT, 0, 0)                                       \
  DO(DXGI_FORMAT_BC6H_SF16, VK_FORMAT_BC6H_SFLOAT_BLOCK, NV_GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB, 0, 0)                                     \
  DO(DXGI_FORMAT_BC6H_UF16, VK_FORMAT_BC6H_UFLOAT_BLOCK, NV_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB, 0, 0)                                   \
  REPVK(DXGI_FORMAT_BC6H_TYPELESS, VK_FORMAT_BC6H_UFLOAT_BLOCK, NV_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB, 0, 0)                            \
  DO(DXGI_FORMAT_BC7_UNORM, VK_FORMAT_BC7_UNORM_BLOCK, NV_GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, 0, 0)                                             \
  DO(DXGI_FORMAT_BC7_UNORM_SRGB, VK_FORMAT_BC7_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB, 0, 0)                                   \
  REPVK(DXGI_FORMAT_BC7_TYPELESS, VK_FORMAT_BC7_SRGB_BLOCK, NV_GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB, 0, 0)                                  \
  DO(DXGI_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM, NV_GL_DEPTH_COMPONENT16, NV_GL_DEPTH_COMPONENT, NV_GL_UNSIGNED_SHORT)                         \
  DO(DXGI_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, NV_GL_DEPTH24_STENCIL8, NV_GL_DEPTH_STENCIL, NV_GL_UNSIGNED_INT_24_8)         \
  REPVK(DXGI_FORMAT_R24G8_TYPELESS, VK_FORMAT_D24_UNORM_S8_UINT, NV_GL_DEPTH24_STENCIL8, NV_GL_DEPTH_STENCIL, NV_GL_UNSIGNED_INT_24_8)         \
  REPVK(DXGI_FORMAT_R24_UNORM_X8_TYPELESS, VK_FORMAT_D24_UNORM_S8_UINT, NV_GL_DEPTH24_STENCIL8, NV_GL_DEPTH_STENCIL, NV_GL_UNSIGNED_INT_24_8)  \
  REPVK(DXGI_FORMAT_X24_TYPELESS_G8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, NV_GL_DEPTH24_STENCIL8, NV_GL_DEPTH_STENCIL, NV_GL_UNSIGNED_INT_24_8)   \
  DO(DXGI_FORMAT_D32_FLOAT, VK_FORMAT_D32_SFLOAT, NV_GL_DEPTH_COMPONENT32F, NV_GL_DEPTH_COMPONENT, NV_GL_FLOAT)                                \
  DO(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, NV_GL_DEPTH32F_STENCIL8, NV_GL_DEPTH_STENCIL,                             \
     NV_GL_FLOAT_32_UNSIGNED_INT_24_8_REV)                                                                                                     \
  REPVK(DXGI_FORMAT_R32G8X24_TYPELESS, VK_FORMAT_D32_SFLOAT_S8_UINT, NV_GL_DEPTH32F_STENCIL8, NV_GL_DEPTH_STENCIL,                             \
        NV_GL_FLOAT_32_UNSIGNED_INT_24_8_REV)                                                                                                  \
  REPVK(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, VK_FORMAT_D32_SFLOAT_S8_UINT, NV_GL_DEPTH32F_STENCIL8,                                           \
        NV_GL_DEPTH_STENCIL, NV_GL_FLOAT_32_UNSIGNED_INT_24_8_REV)                                                                             \
  REPVK(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, NV_GL_DEPTH32F_STENCIL8,                                            \
        NV_GL_DEPTH_STENCIL, NV_GL_FLOAT_32_UNSIGNED_INT_24_8_REV)                                                                             \
  DO(DXGI_FORMAT_R10G10B10A2_UINT, VK_FORMAT_A2B10G10R10_UINT_PACK32, NV_GL_RGB10_A2UI, NV_GL_RGBA_INTEGER, NV_GL_UNSIGNED_INT_2_10_10_10_REV) \
  DO(DXGI_FORMAT_R10G10B10A2_UNORM, VK_FORMAT_A2B10G10R10_UNORM_PACK32, NV_GL_RGB10_A2, NV_GL_RGBA, NV_GL_UNSIGNED_INT_2_10_10_10_REV)         \
  REPVK(DXGI_FORMAT_R10G10B10A2_TYPELESS, VK_FORMAT_A2B10G10R10_UNORM_PACK32, NV_GL_RGB10_A2, NV_GL_RGBA, NV_GL_UNSIGNED_INT_2_10_10_10_REV)   \
  DO(DXGI_FORMAT_R11G11B10_FLOAT, VK_FORMAT_B10G11R11_UFLOAT_PACK32, NV_GL_R11F_G11F_B10F, NV_GL_RGB, NV_GL_UNSIGNED_INT_10F_11F_11F_REV)      \
  DO(DXGI_FORMAT_R16G16B16A16_FLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, NV_GL_RGBA16F, NV_GL_RGBA, NV_GL_HALF_FLOAT)                               \
  DO(DXGI_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SINT, NV_GL_RGBA16I, NV_GL_RGBA_INTEGER, NV_GL_SHORT)                               \
  DO(DXGI_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R16G16B16A16_SNORM, NV_GL_RGBA16_SNORM, NV_GL_RGBA, NV_GL_SHORT)                                \
  DO(DXGI_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_UINT, NV_GL_RGBA16UI, NV_GL_RGBA_INTEGER, NV_GL_UNSIGNED_SHORT)                     \
  DO(DXGI_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_UNORM, NV_GL_RGBA16, NV_GL_RGBA, NV_GL_UNSIGNED_SHORT)                             \
  REPVK(DXGI_FORMAT_R16G16B16A16_TYPELESS, VK_FORMAT_R16G16B16A16_SFLOAT, NV_GL_RGBA16F, NV_GL_RGBA, NV_GL_HALF_FLOAT)                         \
  DO(DXGI_FORMAT_R16G16_FLOAT, VK_FORMAT_R16G16_SFLOAT, NV_GL_RG16F, NV_GL_RG, NV_GL_HALF_FLOAT)                                               \
  DO(DXGI_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SINT, NV_GL_RG16I, NV_GL_RG_INTEGER, NV_GL_SHORT)                                               \
  DO(DXGI_FORMAT_R16G16_SNORM, VK_FORMAT_R16G16_SNORM, NV_GL_RG16_SNORM, NV_GL_RG, NV_GL_SHORT)                                                \
  REPVK(DXGI_FORMAT_R16G16_TYPELESS, VK_FORMAT_R16G16_SFLOAT, NV_GL_RG16F, NV_GL_RG, NV_GL_HALF_FLOAT)                                         \
  DO(DXGI_FORMAT_R16G16_UINT, VK_FORMAT_R16G16_UINT, NV_GL_RG16UI, NV_GL_RG_INTEGER, NV_GL_UNSIGNED_SHORT)                                     \
  DO(DXGI_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16_UNORM, NV_GL_RG16, NV_GL_RG, NV_GL_UNSIGNED_SHORT)                                             \
  DO(DXGI_FORMAT_R16_FLOAT, VK_FORMAT_R16_SFLOAT, NV_GL_R16F, NV_GL_RED, NV_GL_HALF_FLOAT)                                                     \
  DO(DXGI_FORMAT_R16_SINT, VK_FORMAT_R16_SINT, NV_GL_R16I, NV_GL_RED_INTEGER, NV_GL_SHORT)                                                     \
  DO(DXGI_FORMAT_R16_SNORM, VK_FORMAT_R16_SNORM, NV_GL_R16_SNORM, NV_GL_RED, NV_GL_SHORT)                                                      \
  REPVK(DXGI_FORMAT_R16_TYPELESS, VK_FORMAT_R16_SFLOAT, NV_GL_R16F, NV_GL_RED, NV_GL_HALF_FLOAT)                                               \
  DO(DXGI_FORMAT_R16_UINT, VK_FORMAT_R16_UINT, NV_GL_R16UI, NV_GL_RED_INTEGER, NV_GL_UNSIGNED_SHORT)                                           \
  DO(DXGI_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM, NV_GL_R16, NV_GL_RED, NV_GL_UNSIGNED_SHORT)                                                   \
  DO(DXGI_FORMAT_R32G32B32A32_FLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, NV_GL_RGBA32F, NV_GL_RGBA, NV_GL_FLOAT)                                    \
  DO(DXGI_FORMAT_R32G32B32A32_SINT, VK_FORMAT_R32G32B32A32_SINT, NV_GL_RGBA32I, NV_GL_RGBA_INTEGER, NV_GL_INT)                                 \
  DO(DXGI_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_UINT, NV_GL_RGBA32UI, NV_GL_RGBA_INTEGER, NV_GL_UNSIGNED_INT)                       \
  REPVK(DXGI_FORMAT_R32G32B32A32_TYPELESS, VK_FORMAT_R32G32B32A32_SFLOAT, NV_GL_RGBA32F, NV_GL_RGBA, NV_GL_FLOAT)                              \
  DO(DXGI_FORMAT_R32G32B32_FLOAT, VK_FORMAT_R32G32B32_SFLOAT, NV_GL_RGB32F, NV_GL_RGB, NV_GL_FLOAT)                                            \
  DO(DXGI_FORMAT_R32G32B32_SINT, VK_FORMAT_R32G32B32_SINT, NV_GL_RGB32I, NV_GL_RGB_INTEGER, NV_GL_INT)                                         \
  DO(DXGI_FORMAT_R32G32B32_UINT, VK_FORMAT_R32G32B32_UINT, NV_GL_RGB32UI, NV_GL_RGB_INTEGER, NV_GL_UNSIGNED_INT)                               \
  REPVK(DXGI_FORMAT_R32G32B32_TYPELESS, VK_FORMAT_R32G32B32_SFLOAT, NV_GL_RGB32F, NV_GL_RGB, NV_GL_FLOAT)                                      \
  DO(DXGI_FORMAT_R32G32_FLOAT, VK_FORMAT_R32G32_SFLOAT, NV_GL_RG32F, NV_GL_RG, NV_GL_FLOAT)                                                    \
  DO(DXGI_FORMAT_R32G32_SINT, VK_FORMAT_R32G32_SINT, NV_GL_RG32I, NV_GL_RG_INTEGER, NV_GL_INT)                                                 \
  DO(DXGI_FORMAT_R32G32_UINT, VK_FORMAT_R32G32_UINT, NV_GL_RG32UI, NV_GL_RG_INTEGER, NV_GL_UNSIGNED_INT)                                       \
  REPVK(DXGI_FORMAT_R32G32_TYPELESS, VK_FORMAT_R32G32_SFLOAT, NV_GL_RG32F, NV_GL_RG, NV_GL_FLOAT)                                              \
  DO(DXGI_FORMAT_R32_FLOAT, VK_FORMAT_R32_SFLOAT, NV_GL_R32F, NV_GL_RED, NV_GL_FLOAT)                                                          \
  DO(DXGI_FORMAT_R32_SINT, VK_FORMAT_R32_SINT, NV_GL_R32I, NV_GL_RED_INTEGER, NV_GL_INT)                                                       \
  DO(DXGI_FORMAT_R32_UINT, VK_FORMAT_R32_UINT, NV_GL_R32UI, NV_GL_RED_INTEGER, NV_GL_UNSIGNED_INT)                                             \
  REPVK(DXGI_FORMAT_R32_TYPELESS, VK_FORMAT_R32_SFLOAT, NV_GL_R32F, NV_GL_RED, NV_GL_FLOAT)                                                    \
  DO(DXGI_FORMAT_R8G8B8A8_SINT, VK_FORMAT_R8G8B8A8_SINT, NV_GL_RGBA8I, NV_GL_RGBA_INTEGER, NV_GL_BYTE)                                         \
  DO(DXGI_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SNORM, NV_GL_RGBA8_SNORM, NV_GL_RGBA, NV_GL_BYTE)                                          \
  DO(DXGI_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_UINT, NV_GL_RGBA8UI, NV_GL_RGBA_INTEGER, NV_GL_UNSIGNED_BYTE)                               \
  DO(DXGI_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, NV_GL_RGBA8, NV_GL_RGBA, NV_GL_UNSIGNED_BYTE)                                       \
  DO(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, VK_FORMAT_R8G8B8A8_SRGB, NV_GL_SRGB8_ALPHA8, NV_GL_RGBA, NV_GL_UNSIGNED_BYTE)                            \
  REPVK(DXGI_FORMAT_R8G8B8A8_TYPELESS, VK_FORMAT_R8G8B8A8_SRGB, NV_GL_SRGB8_ALPHA8, NV_GL_RGBA, NV_GL_UNSIGNED_BYTE)                           \
  DO(DXGI_FORMAT_R8G8_SINT, VK_FORMAT_R8G8_SINT, NV_GL_RG8I, NV_GL_RG_INTEGER, NV_GL_BYTE)                                                     \
  DO(DXGI_FORMAT_R8G8_SNORM, VK_FORMAT_R8G8_SNORM, NV_GL_RG8_SNORM, NV_GL_RG, NV_GL_BYTE)                                                      \
  DO(DXGI_FORMAT_R8G8_TYPELESS, VK_FORMAT_R8G8_SRGB, NV_GL_SRG8_EXT, NV_GL_RG, NV_GL_UNSIGNED_BYTE)                                            \
  DO(DXGI_FORMAT_R8G8_UINT, VK_FORMAT_R8G8_UINT, NV_GL_RG8UI, NV_GL_RG_INTEGER, NV_GL_UNSIGNED_BYTE)                                           \
  DO(DXGI_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8_UNORM, NV_GL_LUMINANCE8_ALPHA8, NV_GL_LUMINANCE_ALPHA, NV_GL_UNSIGNED_BYTE)                        \
  REPBOTH(DXGI_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8_UNORM, NV_GL_RG8, NV_GL_RG, NV_GL_UNSIGNED_BYTE)                                              \
  DO(DXGI_FORMAT_R8_SINT, VK_FORMAT_R8_SINT, NV_GL_R8I, NV_GL_RED_INTEGER, NV_GL_BYTE)                                                         \
  DO(DXGI_FORMAT_R8_SNORM, VK_FORMAT_R8_SNORM, NV_GL_R8_SNORM, NV_GL_RED, NV_GL_BYTE)                                                          \
  DO(DXGI_FORMAT_R8_TYPELESS, VK_FORMAT_R8_SRGB, NV_GL_SR8_EXT, NV_GL_RED, NV_GL_UNSIGNED_BYTE)                                                \
  DO(DXGI_FORMAT_R8_UINT, VK_FORMAT_R8_UINT, NV_GL_R8UI, NV_GL_RED_INTEGER, NV_GL_UNSIGNED_BYTE)                                               \
  DO(DXGI_FORMAT_R8_UNORM, VK_FORMAT_R8_UNORM, NV_GL_ALPHA8, NV_GL_ALPHA, NV_GL_UNSIGNED_BYTE)                                                 \
  REPBOTH(DXGI_FORMAT_R8_UNORM, VK_FORMAT_R8_UNORM, NV_GL_LUMINANCE8, NV_GL_LUMINANCE, NV_GL_UNSIGNED_BYTE)                                    \
  REPBOTH(DXGI_FORMAT_R8_UNORM, VK_FORMAT_R8_UNORM, NV_GL_R8, NV_GL_RED, NV_GL_UNSIGNED_BYTE)                                                  \
  DO(DXGI_FORMAT_R9G9B9E5_SHAREDEXP, VK_FORMAT_E5B9G9R9_UFLOAT_PACK32, NV_GL_RGB9_E5, NV_GL_RGB, NV_GL_UNSIGNED_INT_5_9_9_9_REV)               \
  DO(DXGI_FORMAT_R8G8_B8G8_UNORM, VK_FORMAT_G8B8G8R8_422_UNORM, 0, 0, 0)                                                                       \
  DO(DXGI_FORMAT_G8R8_G8B8_UNORM, VK_FORMAT_B8G8R8G8_422_UNORM, 0, 0, 0)                                                                       \
  REPVK(DXGI_FORMAT_YUY2, VK_FORMAT_G8B8G8R8_422_UNORM, 0, 0, 0)                                                                               \
  DO(DXGI_FORMAT_NV12, VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, 0, 0, 0)                                                                            \
  DO(DXGI_FORMAT_P010, VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16, 0, 0, 0)                                                           \
  DO(DXGI_FORMAT_P016, VK_FORMAT_G16_B16R16_2PLANE_420_UNORM, 0, 0, 0)                                                                         \
  DO(DXGI_FORMAT_Y210, VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16, 0, 0, 0)                                                              \
  DO(DXGI_FORMAT_Y216, VK_FORMAT_G16B16G16R16_422_UNORM, 0, 0, 0)                                                                              \
  DO(DXGI_FORMAT_P208, VK_FORMAT_G8_B8R8_2PLANE_422_UNORM, 0, 0, 0)

#define DO_NOTHING_ON_REPEAT(...)

// Private functions for building tables.
namespace {

std::unordered_map<OpenGLFormat, uint32_t> s_tableOpenGLToDXGI;
void addGLToDXGICombination(uint32_t dxgiFormat, uint32_t glInternalFormat, uint32_t glFormat, uint32_t glType)
{
  const OpenGLFormat glStruct = {glInternalFormat, glFormat, glType};
  if(glStruct == OpenGLFormat{})
  {
    return;  // Avoid adding unknown GL formats
  }

  if(0 == s_tableOpenGLToDXGI.count(glStruct))
  {
    s_tableOpenGLToDXGI[glStruct] = dxgiFormat;
  }
}

#ifdef NVP_SUPPORTS_VULKANSDK
std::unordered_map<OpenGLFormat, VkFormat> s_tableOpenGLToVulkan;
void addGLToVulkanCombination(VkFormat vkFormat, uint32_t glInternalFormat, uint32_t glFormat, uint32_t glType)
{
  const OpenGLFormat glStruct = {glInternalFormat, glFormat, glType};
  if(glStruct == OpenGLFormat{})
  {
    return;  // Avoid adding unknown GL formats
  }

  if(0 == s_tableOpenGLToVulkan.count(glStruct))
  {
    s_tableOpenGLToVulkan[glStruct] = vkFormat;
  }
}
#endif
// Only build the tables once, even if these functions are called from multiple
// threads.
std::once_flag s_tablesBuilt;

void buildTables()
{
#define ADD_COMBINATION(dxgi, vk, ...) addGLToDXGICombination(dxgi, __VA_ARGS__);
  FOR_EACH_FORMAT(ADD_COMBINATION, ADD_COMBINATION, ADD_COMBINATION);
#undef ADD_COMBINATION

#ifdef NVP_SUPPORTS_VULKANSDK
#define ADD_COMBINATION(dxgi, vk, ...) addGLToVulkanCombination(vk, __VA_ARGS__);
  FOR_EACH_FORMAT(ADD_COMBINATION, ADD_COMBINATION, ADD_COMBINATION);
#undef ADD_COMBINATION
#endif
}

}  // namespace

OpenGLFormat dxgiToOpenGL(uint32_t dxgiFormat)
{
  switch(dxgiFormat)
  {
#define ON_DXGI_RETURN_OPENGL(dxgi, vk, glInternalFormat, glFormat, glType)                                            \
  case dxgi:                                                                                                           \
    return {glInternalFormat, glFormat, glType};

    FOR_EACH_FORMAT(ON_DXGI_RETURN_OPENGL, ON_DXGI_RETURN_OPENGL, DO_NOTHING_ON_REPEAT)
#undef ON_DXGI_RETURN_OPENGL
    default:
      return {};
  }
}

uint32_t openGLToDXGI(const OpenGLFormat& glFormat)
{
  std::call_once(s_tablesBuilt, buildTables);
  const auto& findResult = s_tableOpenGLToDXGI.find(glFormat);
  if(findResult == s_tableOpenGLToDXGI.end())
  {
    return {};
  }
  return findResult->second;
}

#ifdef NVP_SUPPORTS_VULKANSDK
VkFormat dxgiToVulkan(uint32_t dxgiFormat)
{
  switch(dxgiFormat)
  {
#define ON_DXGI_RETURN_VK(dxgi, vk, glInternalFormat, glFormat, glType)                                                \
  case dxgi:                                                                                                           \
    return vk;

    FOR_EACH_FORMAT(ON_DXGI_RETURN_VK, ON_DXGI_RETURN_VK, DO_NOTHING_ON_REPEAT)
#undef ON_DXGI_RETURN_VK
    default:
      return VK_FORMAT_UNDEFINED;
  }
}

VkFormat openGLToVulkan(const OpenGLFormat& glFormat)
{
  std::call_once(s_tablesBuilt, buildTables);
  const auto& findResult = s_tableOpenGLToVulkan.find(glFormat);
  if(findResult == s_tableOpenGLToVulkan.end())
  {
    return {};
  }
  return findResult->second;
}

uint32_t vulkanToDXGI(VkFormat vkFormat)
{
  switch(vkFormat)
  {
#define ON_VK_RETURN_DXGI(dxgi, vk, glInternalFormat, glFormat, glType)                                                \
  case vk:                                                                                                             \
    return dxgi;

    FOR_EACH_FORMAT(ON_VK_RETURN_DXGI, DO_NOTHING_ON_REPEAT, DO_NOTHING_ON_REPEAT)
#undef ON_VK_RETURN_DXGI
    default:
      return {};
  }
}

OpenGLFormat vulkanToOpenGL(VkFormat vkFormat)
{
  switch(vkFormat)
  {
#define ON_VK_RETURN_OPENGL(dxgi, vk, glInternalFormat, glFormat, glType)                                              \
  case vk:                                                                                                             \
    return {glInternalFormat, glFormat, glType};

    FOR_EACH_FORMAT(ON_VK_RETURN_OPENGL, DO_NOTHING_ON_REPEAT, DO_NOTHING_ON_REPEAT)
#undef ON_VK_RETURN_OPENGL
    default:
      return {};
  }
}
#endif

const char* getDXGIFormatName(uint32_t dxgiFormat)
{
  switch(dxgiFormat)
  {
#define RETURN_DXGI_FORMAT_NAME(dxgi, vk, glInternalFormat, glFormat, glType)                                          \
  case dxgi:                                                                                                           \
    return #dxgi;

    FOR_EACH_FORMAT(RETURN_DXGI_FORMAT_NAME, RETURN_DXGI_FORMAT_NAME, DO_NOTHING_ON_REPEAT)
#undef RETURN_DXGI_FORMAT_NAME
    default:
      return nullptr;
  }
}

#ifdef NVP_SUPPORTS_VULKANSDK
const char* getVkFormatName(VkFormat vkFormat)
{
  switch(vkFormat)
  {
#define RETURN_VK_FORMAT_NAME(dxgi, vk, glInternalFormat, glFormat, glType)                                            \
  case vk:                                                                                                             \
    return #vk;

    FOR_EACH_FORMAT(RETURN_VK_FORMAT_NAME, DO_NOTHING_ON_REPEAT, DO_NOTHING_ON_REPEAT)
#undef RETURN_VK_FORMAT_JNAME
    default:
      return nullptr;
  }
}
#endif

// The transfer function functions work on tables of pairs of (non-sRGB, sRGB)
// formats.
#define FOR_EACH_DXGI_SRGB_PAIR(DO)                                                                                    \
  DO(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)                                                      \
  DO(DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB)                                                                \
  DO(DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB)                                                                \
  DO(DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM_SRGB)                                                                \
  DO(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)                                                      \
  DO(DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM_SRGB)                                                      \
  DO(DXGI_FORMAT_ASTC_4X4_UNORM, DXGI_FORMAT_ASTC_4X4_UNORM_SRGB)                                                      \
  DO(DXGI_FORMAT_ASTC_5X4_UNORM, DXGI_FORMAT_ASTC_5X4_UNORM_SRGB)                                                      \
  DO(DXGI_FORMAT_ASTC_5X5_UNORM, DXGI_FORMAT_ASTC_5X5_UNORM_SRGB)                                                      \
  DO(DXGI_FORMAT_ASTC_6X5_UNORM, DXGI_FORMAT_ASTC_6X5_UNORM_SRGB)                                                      \
  DO(DXGI_FORMAT_ASTC_6X6_UNORM, DXGI_FORMAT_ASTC_6X6_UNORM_SRGB)                                                      \
  DO(DXGI_FORMAT_ASTC_8X5_UNORM, DXGI_FORMAT_ASTC_8X5_UNORM_SRGB)                                                      \
  DO(DXGI_FORMAT_ASTC_8X6_UNORM, DXGI_FORMAT_ASTC_8X6_UNORM_SRGB)                                                      \
  DO(DXGI_FORMAT_ASTC_8X8_UNORM, DXGI_FORMAT_ASTC_8X8_UNORM_SRGB)                                                      \
  DO(DXGI_FORMAT_ASTC_10X5_UNORM, DXGI_FORMAT_ASTC_10X5_UNORM_SRGB)                                                    \
  DO(DXGI_FORMAT_ASTC_10X6_UNORM, DXGI_FORMAT_ASTC_10X6_UNORM_SRGB)                                                    \
  DO(DXGI_FORMAT_ASTC_10X8_UNORM, DXGI_FORMAT_ASTC_10X8_UNORM_SRGB)                                                    \
  DO(DXGI_FORMAT_ASTC_10X10_UNORM, DXGI_FORMAT_ASTC_10X10_UNORM_SRGB)                                                  \
  DO(DXGI_FORMAT_ASTC_12X10_UNORM, DXGI_FORMAT_ASTC_12X10_UNORM_SRGB)                                                  \
  DO(DXGI_FORMAT_ASTC_12X12_UNORM, DXGI_FORMAT_ASTC_12X12_UNORM_SRGB)

bool isDXGIFormatSRGB(uint32_t dxgiFormat)
{
  switch(dxgiFormat)
  {
#define SRGB_CASE(nonSRGBFormat, srgbFormat)                                                                           \
  case srgbFormat:                                                                                                     \
    return true;

    FOR_EACH_DXGI_SRGB_PAIR(SRGB_CASE)
#undef SRGB_CASE
    default:
      return false;
  }
}

uint32_t tryForceDXGIFormatTransferFunction(uint32_t dxgiFormat, bool srgb)
{
  if(srgb)
  {
    switch(dxgiFormat)
    {
#define TO_SRGB(nonSRGBFormat, srgbFormat)                                                                             \
  case nonSRGBFormat:                                                                                                  \
    return srgbFormat;

      FOR_EACH_DXGI_SRGB_PAIR(TO_SRGB)
#undef TO_SRGB
      default:
        return dxgiFormat;
    }
  }
  else
  {
    switch(dxgiFormat)
    {
#define TO_NON_SRGB(nonSRGBFormat, srgbFormat)                                                                         \
  case srgbFormat:                                                                                                     \
    return nonSRGBFormat;

      FOR_EACH_DXGI_SRGB_PAIR(TO_NON_SRGB)
#undef TO_NON_SRGB
      default:
        return dxgiFormat;
    }
  }
}

#ifdef NVP_SUPPORTS_VULKANSDK
#define FOR_EACH_VK_SRGB_PAIR(DO)                                                                                      \
  DO(VK_FORMAT_R8_UNORM, VK_FORMAT_R8_SRGB)                                                                            \
  DO(VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8_SRGB)                                                                        \
  DO(VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8_SRGB)                                                                    \
  DO(VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_B8G8R8_SRGB)                                                                    \
  DO(VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SRGB)                                                                \
  DO(VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB)                                                                \
  DO(VK_FORMAT_A8B8G8R8_UNORM_PACK32, VK_FORMAT_A8B8G8R8_SRGB_PACK32)                                                  \
  DO(VK_FORMAT_BC1_RGB_UNORM_BLOCK, VK_FORMAT_BC1_RGB_SRGB_BLOCK)                                                      \
  DO(VK_FORMAT_BC1_RGBA_UNORM_BLOCK, VK_FORMAT_BC1_RGBA_SRGB_BLOCK)                                                    \
  DO(VK_FORMAT_BC2_UNORM_BLOCK, VK_FORMAT_BC2_SRGB_BLOCK)                                                              \
  DO(VK_FORMAT_BC3_UNORM_BLOCK, VK_FORMAT_BC3_SRGB_BLOCK)                                                              \
  DO(VK_FORMAT_BC7_UNORM_BLOCK, VK_FORMAT_BC7_SRGB_BLOCK)                                                              \
  DO(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK)                                              \
  DO(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK)                                          \
  DO(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK)                                          \
  DO(VK_FORMAT_ASTC_4x4_UNORM_BLOCK, VK_FORMAT_ASTC_4x4_SRGB_BLOCK)                                                    \
  DO(VK_FORMAT_ASTC_5x4_UNORM_BLOCK, VK_FORMAT_ASTC_5x4_SRGB_BLOCK)                                                    \
  DO(VK_FORMAT_ASTC_5x5_UNORM_BLOCK, VK_FORMAT_ASTC_5x5_SRGB_BLOCK)                                                    \
  DO(VK_FORMAT_ASTC_6x5_UNORM_BLOCK, VK_FORMAT_ASTC_6x5_SRGB_BLOCK)                                                    \
  DO(VK_FORMAT_ASTC_6x6_UNORM_BLOCK, VK_FORMAT_ASTC_6x6_SRGB_BLOCK)                                                    \
  DO(VK_FORMAT_ASTC_8x5_UNORM_BLOCK, VK_FORMAT_ASTC_8x5_SRGB_BLOCK)                                                    \
  DO(VK_FORMAT_ASTC_8x6_UNORM_BLOCK, VK_FORMAT_ASTC_8x6_SRGB_BLOCK)                                                    \
  DO(VK_FORMAT_ASTC_8x8_UNORM_BLOCK, VK_FORMAT_ASTC_8x8_SRGB_BLOCK)                                                    \
  DO(VK_FORMAT_ASTC_10x5_UNORM_BLOCK, VK_FORMAT_ASTC_10x5_SRGB_BLOCK)                                                  \
  DO(VK_FORMAT_ASTC_10x6_UNORM_BLOCK, VK_FORMAT_ASTC_10x6_SRGB_BLOCK)                                                  \
  DO(VK_FORMAT_ASTC_10x8_UNORM_BLOCK, VK_FORMAT_ASTC_10x8_SRGB_BLOCK)                                                  \
  DO(VK_FORMAT_ASTC_10x10_UNORM_BLOCK, VK_FORMAT_ASTC_10x10_SRGB_BLOCK)                                                \
  DO(VK_FORMAT_ASTC_12x10_UNORM_BLOCK, VK_FORMAT_ASTC_12x10_SRGB_BLOCK)                                                \
  DO(VK_FORMAT_ASTC_12x12_UNORM_BLOCK, VK_FORMAT_ASTC_12x12_SRGB_BLOCK)                                                \
  DO(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG)                                      \
  DO(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG)                                      \
  DO(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG)                                      \
  DO(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG)

bool isVkFormatSRGB(VkFormat vkFormat)
{
  switch(vkFormat)
  {
#define SRGB_CASE(nonSRGBFormat, srgbFormat)                                                                           \
  case srgbFormat:                                                                                                     \
    return true;

    FOR_EACH_VK_SRGB_PAIR(SRGB_CASE)
#undef SRGB_CASE
    default:
      return false;
  }
}

VkFormat tryForceVkFormatTransferFunction(VkFormat vkFormat, bool srgb)
{
  if(srgb)
  {
    switch(vkFormat)
    {
#define TO_SRGB(nonSRGBFormat, srgbFormat)                                                                             \
  case nonSRGBFormat:                                                                                                  \
    return srgbFormat;

      FOR_EACH_VK_SRGB_PAIR(TO_SRGB)
#undef TO_SRGB
      default:
        return vkFormat;
    }
  }
  else
  {
    switch(vkFormat)
    {
#define TO_NON_SRGB(nonSRGBFormat, srgbFormat)                                                                         \
  case srgbFormat:                                                                                                     \
    return nonSRGBFormat;

      FOR_EACH_VK_SRGB_PAIR(TO_NON_SRGB)
#undef TO_NON_SRGB
      default:
        return vkFormat;
    }
  }
}

#endif

}  // namespace texture_formats
