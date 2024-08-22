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

#ifndef NVVKHL_PBR_MAT_STRUCT_H
#define NVVKHL_PBR_MAT_STRUCT_H 1

#include "func.h"

struct PbrMaterial
{
  vec3  baseColor;  // base color
  float opacity;    // 1 = opaque, 0 = fully transparent
  vec2  roughness;  // 0 = smooth, 1 = rough (anisotropic: x = U, y = V)
  float metallic;   // 0 = dielectric, 1 = metallic
  vec3  emissive;   // emissive color

  vec3 N;   // shading normal
  vec3 T;   // shading normal
  vec3 B;   // shading normal
  vec3 Ng;  // geometric normal


  float ior1;  // index of refraction : current medium (i.e. air)
  float ior2;  // index of refraction : the other side (i.e. glass)

  float specular;       // weight of the dielectric specular layer
  vec3  specularColor;  // color of the dielectric specular layer
  float transmission;   // KHR_materials_transmission

  vec3  attenuationColor;     // KHR_materials_volume
  float attenuationDistance;  //
  float thickness;            // Replace for isThinWalled?

  float clearcoat;           // KHR_materials_clearcoat
  float clearcoatRoughness;  //
  vec3  Nc;                  // clearcoat normal

  float iridescence;
  float iridescenceIor;
  float iridescenceThickness;

  vec3  sheenColor;
  float sheenRoughness;

  mat3 TBN;
};

PbrMaterial defaultPbrMaterial()
{
  PbrMaterial mat;
  mat.baseColor = vec3(1.0F);
  mat.opacity   = 1.0F;
  mat.roughness = vec2(1.0F);
  mat.metallic  = 1.0F;
  mat.emissive  = vec3(0.0F);

  mat.N  = vec3(0.0F, 0.0F, 1.0F);
  mat.Ng = vec3(0.0F, 0.0F, 1.0F);
  mat.T  = vec3(1.0F, 0.0F, 0.0F);
  mat.B  = vec3(0.0F, 1.0F, 0.0F);

  mat.ior1 = 1.0F;
  mat.ior2 = 1.5F;

  mat.specular      = 1.0F;
  mat.specularColor = vec3(1.0F);
  mat.transmission  = 0.0F;

  mat.attenuationColor    = vec3(1.0F);
  mat.attenuationDistance = 1.0F;
  mat.thickness           = 0.0F;

  mat.clearcoat          = 0.0F;
  mat.clearcoatRoughness = 0.01F;
  mat.Nc                 = vec3(0.0F, 0.0F, 1.0F);

  mat.iridescence          = 0.0F;
  mat.iridescenceIor       = 1.5F;
  mat.iridescenceThickness = 0.1F;

  mat.sheenColor     = vec3(0.0F);
  mat.sheenRoughness = 0.0F;

  return mat;
}

PbrMaterial defaultPbrMaterial(vec3 baseColor, float metallic, float roughness, vec3 N, vec3 Ng)
{
  PbrMaterial mat = defaultPbrMaterial();
  mat.baseColor   = baseColor;
  mat.metallic    = metallic;
  mat.roughness   = vec2(roughness * roughness);
  mat.Ng          = Ng;
  mat.N           = N;
  orthonormalBasis(mat.N, mat.T, mat.B);

  return mat;
}
#endif  // NVVKHL_PBR_MAT_STRUCT_H
