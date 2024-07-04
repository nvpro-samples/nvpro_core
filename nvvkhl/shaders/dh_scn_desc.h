
/*
 * Copyright (c) 2022-2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2024, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

/// @DOC_SKIP

#ifndef DH_SCN_DESC_H
#define DH_SCN_DESC_H 1

#ifdef __cplusplus
namespace nvvkhl_shaders {

using mat4 = glm::mat4;
using mat3 = glm::mat3;
using vec4 = glm::vec4;
using vec3 = glm::vec3;
using vec2 = glm::vec2;
#endif  // __cplusplus

// This is the GLTF Node structure, but flattened
struct RenderNode
{
  mat4 objectToWorld;
  mat4 worldToObject;
  int  materialID;
  int  renderPrimID;
};

// This is all the information about a vertex buffer
struct VertexBuffers
{
  uint64_t positionAddress;
  uint64_t normalAddress;
  uint64_t colorAddress;
  uint64_t tangentAddress;
  uint64_t texCoord0Address;
};

// This is the GLTF Primitive structure
struct RenderPrimitive
{
  uint64_t      indexAddress;
  VertexBuffers vertexBuffer;
};

// The scene description is a pointer to the material, render node and render primitive
// The buffers are all arrays of the above structures
struct SceneDescription
{
  uint64_t materialAddress;
  uint64_t renderNodeAddress;
  uint64_t renderPrimitiveAddress;
};

// alphaMode
#define ALPHA_OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2

struct GltfShadeMaterial
{
  // Core
  vec4  pbrBaseColorFactor;           // offset 0 - 16 bytes
  vec3  emissiveFactor;               // offset 16 - 12 bytes
  int   pbrBaseColorTexture;          // offset 28 - 4 bytes
  int   normalTexture;                // offset 32 - 4 bytes
  float normalTextureScale;           // offset 36 - 4 bytes
  int   _pad0;                        // offset 40 - 4 bytes
  float pbrRoughnessFactor;           // offset 44 - 4 bytes
  float pbrMetallicFactor;            // offset 48 - 4 bytes
  int   pbrMetallicRoughnessTexture;  // offset 52 - 4 bytes
  int   emissiveTexture;              // offset 56 - 4 bytes
  int   alphaMode;                    // offset 60 - 4 bytes
  float alphaCutoff;                  // offset 64 - 4 bytes

  // KHR_materials_transmission
  float transmissionFactor;   // offset 68 - 4 bytes
  int   transmissionTexture;  // offset 72 - 4 bytes

  // KHR_materials_ior
  float ior;  // offset 76 - 4 bytes

  // KHR_materials_volume
  vec3  attenuationColor;     // offset 80 - 12 bytes
  float thicknessFactor;      // offset 92 - 4 bytes
  int   thicknessTexture;     // offset 96 - 4 bytes
  float attenuationDistance;  // offset 100 - 4 bytes

  // KHR_materials_clearcoat
  float clearcoatFactor;            // offset 104 - 4 bytes
  float clearcoatRoughness;         // offset 108 - 4 bytes
  int   clearcoatTexture;           // offset 112 - 4 bytes
  int   clearcoatRoughnessTexture;  // offset 116 - 4 bytes
  int   clearcoatNormalTexture;     // offset 120 - 4 bytes

  // KHR_materials_specular
  vec3  specularColorFactor;   // offset 124 - 12 bytes
  float specularFactor;        // offset 136 - 4 bytes
  int   specularTexture;       // offset 140 - 4 bytes
  int   specularColorTexture;  // offset 144 - 4 bytes

  // KHR_materials_unlit
  int unlit;  // offset 148 - 4 bytes

  // KHR_materials_iridescence
  float iridescenceFactor;            // offset 152 - 4 bytes
  int   iridescenceTexture;           // offset 156 - 4 bytes
  float iridescenceThicknessMaximum;  // offset 160 - 4 bytes
  float iridescenceThicknessMinimum;  // offset 164 - 4 bytes
  int   iridescenceThicknessTexture;  // offset 168 - 4 bytes
  float iridescenceIor;               // offset 172 - 4 bytes

  // KHR_materials_anisotropy
  float anisotropyStrength;  // offset 176 - 4 bytes
  int   anisotropyTexture;   // offset 180 - 4 bytes
  float anisotropyRotation;  // offset 184 - 4 bytes

  // KHR_texture_transform
  mat3 uvTransform;  // offset 188 - 48 bytes (mat3 occupies 3 vec4, thus 3 * 16 bytes = 48 bytes)
  vec4 _pad3;        // offset 236 - 16 bytes (to cover the 36 bytes of the mat3 (C++))

  // KHR_materials_sheen
  int   sheenColorTexture;      // offset 252 - 4 bytes
  float sheenRoughnessFactor;   // offset 256 - 4 bytes
  int   sheenRoughnessTexture;  // offset 260 - 4 bytes
  vec3  sheenColorFactor;       // offset 264 - 12 bytes
  int   _pad2;                  // offset 276 - 4 bytes (padding to align to 16 bytes)

  // Total size: 280 bytes
};


#ifdef __cplusplus
inline
#endif
    GltfShadeMaterial
    defaultGltfMaterial()
{
  GltfShadeMaterial m;
  m.pbrBaseColorFactor          = vec4(1, 1, 1, 1);
  m.emissiveFactor              = vec3(0, 0, 0);
  m.pbrBaseColorTexture         = -1;
  m.normalTexture               = -1;
  m.normalTextureScale          = 1;
  m.pbrRoughnessFactor          = 1;
  m.pbrMetallicFactor           = 1;
  m.pbrMetallicRoughnessTexture = -1;
  m.emissiveTexture             = -1;
  m.alphaMode                   = ALPHA_OPAQUE;
  m.alphaCutoff                 = 0.5;
  m.transmissionFactor          = 0;
  m.transmissionTexture         = -1;
  m.ior                         = 1.5;
  m.attenuationColor            = vec3(1, 1, 1);
  m.thicknessFactor             = 0;
  m.thicknessTexture            = -1;
  m.attenuationDistance         = 0;
  m.clearcoatFactor             = 0;
  m.clearcoatRoughness          = 0;
  m.clearcoatTexture            = -1;
  m.clearcoatRoughnessTexture   = -1;
  m.clearcoatNormalTexture      = -1;
  m.specularFactor              = 0;
  m.specularTexture             = -1;
  m.specularColorFactor         = vec3(1, 1, 1);
  m.specularColorTexture        = -1;
  m.uvTransform                 = mat3(1);
  return m;
}

#ifdef __cplusplus
}  // namespace nvvkhl_shaders
#endif

#endif
