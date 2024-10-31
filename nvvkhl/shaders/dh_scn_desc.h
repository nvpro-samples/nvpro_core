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

#ifndef DH_SCN_DESC_H
#define DH_SCN_DESC_H 1

/* @DOC_START
Common structures used to store glTF scenes in GPU buffers.
@DOC_END */

#ifdef __cplusplus
#define INLINE inline
namespace nvvkhl_shaders {
using mat4   = glm::mat4;
using mat3   = glm::mat3;
using mat2x3 = glm::mat2x3;
using vec4   = glm::vec4;
using vec3   = glm::vec3;
using vec2   = glm::vec2;
#else
#define INLINE
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
  uint64_t texCoord1Address;
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
  uint64_t lightAddress;
  int      numLights;  // number of punctual lights
};

// alphaMode
#define ALPHA_OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2
struct GltfTextureInfo
{
  mat2x3 uvTransform;  // 24 bytes (2x3 matrix)
  int    index;        // 4 bytes
  int    texCoord;     // 4 bytes
};                     // Total: 32 bytes


struct GltfShadeMaterial
{
  vec4  pbrBaseColorFactor;           // offset 0 - 16 bytes    - glTF Core
  vec3  emissiveFactor;               // offset 16 - 12 bytes
  float normalTextureScale;           // offset 28 - 4 bytes
  float pbrRoughnessFactor;           // offset 32 - 4 bytes
  float pbrMetallicFactor;            // offset 36 - 4 bytes
  int   alphaMode;                    // offset 40 - 4 bytes
  float alphaCutoff;                  // offset 44 - 4 bytes
  float transmissionFactor;           // offset 48 - 4 bytes    - KHR_materials_transmission
  float ior;                          // offset 52 - 4 bytes    - KHR_materials_ior
  vec3  attenuationColor;             // offset 56 - 12 bytes   - KHR_materials_volume
  float thicknessFactor;              // offset 68 - 4 bytes
  float attenuationDistance;          // offset 72 - 4 bytes
  float clearcoatFactor;              // offset 76 - 4 bytes    - KHR_materials_clearcoat
  float clearcoatRoughness;           // offset 80 - 4 bytes
  vec3  specularColorFactor;          // offset 84 - 12 bytes   - KHR_materials_specular
  float specularFactor;               // offset 96 - 4 bytes
  int   unlit;                        // offset 100 - 4 bytes   - KHR_materials_unlit
  float iridescenceFactor;            // offset 104 - 4 bytes   - KHR_materials_iridescence
  float iridescenceThicknessMaximum;  // offset 108 - 4 bytes
  float iridescenceThicknessMinimum;  // offset 112 - 4 bytes
  float iridescenceIor;               // offset 116 - 4 bytes
  float anisotropyStrength;           // offset 120 - 4 bytes   - KHR_materials_anisotropy
  vec2  anisotropyRotation;           // offset 124 - 8 bytes
  float sheenRoughnessFactor;         // offset 132 - 4 bytes   - KHR_materials_sheen
  vec3  sheenColorFactor;             // offset 136 - 12 bytes
  float occlusionStrength;            // offset 148 - 4 bytes
  float dispersion;                   // offset 152 - 4 bytes   - KHR_materials_dispersion
  int   doubleSided;                  // offset 156 - 4 bytes
  int   padding;                      // offset 160 - 4 bytes
  // Texture infos (32 bytes each) - Empirically must start at offset 164 for correct texture transforms
  GltfTextureInfo pbrBaseColorTexture;          // offset 164 - 32 bytes
  GltfTextureInfo normalTexture;                // offset 196 - 32 bytes
  GltfTextureInfo pbrMetallicRoughnessTexture;  // offset 228 - 32 bytes
  GltfTextureInfo emissiveTexture;              // offset 260 - 32 bytes
  GltfTextureInfo transmissionTexture;          // offset 292 - 32 bytes
  GltfTextureInfo thicknessTexture;             // offset 324 - 32 bytes
  GltfTextureInfo clearcoatTexture;             // offset 356 - 32 bytes
  GltfTextureInfo clearcoatRoughnessTexture;    // offset 388 - 32 bytes
  GltfTextureInfo clearcoatNormalTexture;       // offset 420 - 32 bytes
  GltfTextureInfo specularTexture;              // offset 452 - 32 bytes
  GltfTextureInfo specularColorTexture;         // offset 484 - 32 bytes
  GltfTextureInfo iridescenceTexture;           // offset 516 - 32 bytes
  GltfTextureInfo iridescenceThicknessTexture;  // offset 548 - 32 bytes
  GltfTextureInfo anisotropyTexture;            // offset 580 - 32 bytes
  GltfTextureInfo sheenColorTexture;            // offset 612 - 32 bytes
  GltfTextureInfo sheenRoughnessTexture;        // offset 644 - 32 bytes
  GltfTextureInfo occlusionTexture;             // offset 676 - 32 bytes
};                                              // Total size: 708 bytes
INLINE GltfTextureInfo defaultGltfTextureInfo()
{
  GltfTextureInfo t;
  t.uvTransform = mat2x3(1);
  t.index       = -1;
  t.texCoord    = 0;
  return t;
}

INLINE GltfShadeMaterial defaultGltfMaterial()
{
  GltfShadeMaterial m;
  m.pbrBaseColorFactor          = vec4(1, 1, 1, 1);
  m.emissiveFactor              = vec3(0, 0, 0);
  m.normalTextureScale          = 1;
  m.pbrRoughnessFactor          = 1;
  m.pbrMetallicFactor           = 1;
  m.alphaMode                   = ALPHA_OPAQUE;
  m.alphaCutoff                 = 0.5;
  m.transmissionFactor          = 0;
  m.ior                         = 1.5;
  m.attenuationColor            = vec3(1, 1, 1);
  m.thicknessFactor             = 0;
  m.attenuationDistance         = 0;
  m.clearcoatFactor             = 0;
  m.clearcoatRoughness          = 0;
  m.specularFactor              = 0;
  m.specularColorFactor         = vec3(1, 1, 1);
  m.unlit                       = 0;
  m.iridescenceFactor           = 0;
  m.iridescenceThicknessMaximum = 100;
  m.iridescenceThicknessMinimum = 400;
  m.iridescenceIor              = 1.3f;
  m.anisotropyStrength          = 0;
  m.anisotropyRotation          = vec2(0, 0);
  m.sheenRoughnessFactor        = 0;
  m.sheenColorFactor            = vec3(0, 0, 0);
  m.dispersion                  = 0;

  m.pbrBaseColorTexture         = defaultGltfTextureInfo();
  m.normalTexture               = defaultGltfTextureInfo();
  m.pbrMetallicRoughnessTexture = defaultGltfTextureInfo();
  m.emissiveTexture             = defaultGltfTextureInfo();
  m.transmissionTexture         = defaultGltfTextureInfo();
  m.thicknessTexture            = defaultGltfTextureInfo();
  m.clearcoatTexture            = defaultGltfTextureInfo();
  m.clearcoatRoughnessTexture   = defaultGltfTextureInfo();
  m.clearcoatNormalTexture      = defaultGltfTextureInfo();
  m.specularTexture             = defaultGltfTextureInfo();
  m.specularColorTexture        = defaultGltfTextureInfo();
  m.iridescenceTexture          = defaultGltfTextureInfo();
  m.iridescenceThicknessTexture = defaultGltfTextureInfo();
  m.anisotropyTexture           = defaultGltfTextureInfo();
  m.sheenColorTexture           = defaultGltfTextureInfo();
  m.sheenRoughnessTexture       = defaultGltfTextureInfo();

  return m;
}

#ifdef __cplusplus
}  // namespace nvvkhl_shaders
#endif

#endif
