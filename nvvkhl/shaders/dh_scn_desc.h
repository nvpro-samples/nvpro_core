
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

#ifndef DH_SCN_DESC_H
#define DH_SCN_DESC_H 1

#ifdef __cplusplus
using mat4 = nvmath::mat4f;
using vec4 = nvmath::vec4f;
using vec3 = nvmath::vec3f;
#endif  // __cplusplus


struct InstanceInfo
{
  mat4 objectToWorld;
  mat4 worldToObject;
  int  materialID;
};

struct Vertex
{
  vec4 position;  // POS.xyz + UV.x
  vec4 normal;    // NRM.xyz + UV.y
  vec4 tangent;   // TNG.xyz + sign: 1, -1
};

struct PrimMeshInfo
{
  uint64_t vertexAddress;
  uint64_t indexAddress;
  int      materialIndex;
};

struct SceneDescription
{
  uint64_t materialAddress;
  uint64_t instInfoAddress;
  uint64_t primInfoAddress;
};

// shadingModel
#define MATERIAL_METALLICROUGHNESS 0
#define MATERIAL_SPECULARGLOSSINESS 1
// alphaMode
#define ALPHA_OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2

struct GltfShadeMaterial
{
  vec4 pbrBaseColorFactor;
  vec3 emissiveFactor;
  int  pbrBaseColorTexture;

  int   normalTexture;
  float normalTextureScale;
  int   shadingModel;
  float pbrRoughnessFactor;

  float pbrMetallicFactor;
  int   pbrMetallicRoughnessTexture;
  int   khrSpecularGlossinessTexture;
  int   khrDiffuseTexture;

  vec4  khrDiffuseFactor;
  vec3  khrSpecularFactor;
  float khrGlossinessFactor;

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
  float attenuationDistance;
  // KHR_materials_clearcoat
  float clearcoatFactor;
  float clearcoatRoughness;
  int   clearcoatTexture;
  int   clearcoatRoughnessTexture;
};

#endif
