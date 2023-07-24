/*
 * Copyright (c) 2023, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef BSDF_STRUCTS_H
#define BSDF_STRUCTS_H 1

#define BSDF_EVENT_ABSORB 0
#define BSDF_EVENT_DIFFUSE 1
#define BSDF_EVENT_GLOSSY (1 << 1)
#define BSDF_EVENT_SPECULAR (1 << 2)
#define BSDF_EVENT_REFLECTION (1 << 3)
#define BSDF_EVENT_TRANSMISSION (1 << 4)

#define BSDF_EVENT_DIFFUSE_REFLECTION (BSDF_EVENT_DIFFUSE | BSDF_EVENT_REFLECTION)
#define BSDF_EVENT_DIFFUSE_TRANSMISSION (BSDF_EVENT_DIFFUSE | BSDF_EVENT_TRANSMISSION)
#define BSDF_EVENT_GLOSSY_REFLECTION (BSDF_EVENT_GLOSSY | BSDF_EVENT_REFLECTION)
#define BSDF_EVENT_GLOSSY_TRANSMISSION (BSDF_EVENT_GLOSSY | BSDF_EVENT_TRANSMISSION)
#define BSDF_EVENT_SPECULAR_REFLECTION (BSDF_EVENT_SPECULAR | BSDF_EVENT_REFLECTION)
#define BSDF_EVENT_SPECULAR_TRANSMISSION (BSDF_EVENT_SPECULAR | BSDF_EVENT_TRANSMISSION)

#define BSDF_USE_MATERIAL_IOR (-1.0)

struct BsdfEvaluateData
{
  vec3  ior1;          // [in] inside ior
  vec3  ior2;          // [in] outside ior
  vec3  k1;            // [in] Toward the incoming ray
  vec3  k2;            // [in] Toward the sampled light
  vec3  bsdf_diffuse;  // [out] Diffuse contribution
  vec3  bsdf_glossy;   // [out] Specular contribution
  float pdf;           // [out] PDF
};

struct BsdfSampleData
{
  vec3  ior1;           // [in] inside ior
  vec3  ior2;           // [in] outside ior
  vec3  k1;             // [in] Toward the incoming ray
  vec3  k2;             // [in] Toward the sampled light
  vec4  xi;             // [in] 4 random [0..1]
  float pdf;            // [out] PDF
  vec3  bsdf_over_pdf;  // [out] contribution / PDF
  int   event_type;     // [out] one of the event above
};


#endif