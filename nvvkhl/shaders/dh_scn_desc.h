
/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2022 NVIDIA CORPORATION
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
  vec4 pbrBaseColorFactor;
  vec3 emissiveFactor;
  int  pbrBaseColorTexture;

  int   normalTexture;
  float normalTextureScale;
  int   _pad0;
  float pbrRoughnessFactor;

  float pbrMetallicFactor;
  int   pbrMetallicRoughnessTexture;

  int   emissiveTexture;
  int   alphaMode;
  float alphaCutoff;

  // KHR_materials_transmission
  float transmissionFactor;
  int   transmissionTexture;
  // KHR_materials_ior
  float ior;
  // KHR_materials_volume
  vec3  attenuationColor;
  float thicknessFactor;
  int   thicknessTexture;
  bool  thinWalled;
  float attenuationDistance;
  // KHR_materials_clearcoat
  float clearcoatFactor;
  float clearcoatRoughness;
  int   clearcoatTexture;
  int   clearcoatRoughnessTexture;
  int   clearcoatNormalTexture;
  // KHR_materials_specular
  float specularFactor;
  int   specularTexture;
  vec3  specularColorFactor;
  int   specularColorTexture;
  // KHR_texture_transform
  mat3 uvTransform;
};

#ifdef __cplusplus
inline 
#endif
GltfShadeMaterial defaultGltfMaterial()
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
  m.thinWalled                  = false;
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
