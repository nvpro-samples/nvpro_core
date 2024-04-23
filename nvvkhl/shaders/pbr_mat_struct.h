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

#ifndef PBR_MAT_STRUCT_H
#define PBR_MAT_STRUCT_H 1

struct PbrMaterial
{
  vec4  albedo;               // base color
  float roughness;            // 0 = smooth, 1 = rough
  float metallic;             // 0 = dielectric, 1 = metallic
  vec3  normal;               // shading normal
  vec3  emissive;             // emissive color
  vec3  f0;                   // full reflectance color (n incidence angle)
  vec3  f90;                  // reflectance color at grazing angle
  float eta;                  // index of refraction
  float specularWeight;       // product of specularFactor and specularTexture.a
  float transmissionFactor;   // KHR_materials_transmission
  float ior;                  // KHR_materials_ior
  vec3  attenuationColor;     // KHR_materials_volume
  float attenuationDistance;  // KHR_materials_volume
  float thicknessFactor;      // KHR_materials_volume
  float clearcoatFactor;      // KHR_materials_clearcoat
  float clearcoatRoughness;   // KHR_materials_clearcoat
  vec3  clearcoatNormal;      // KHR_materials_clearcoat
};

#endif
