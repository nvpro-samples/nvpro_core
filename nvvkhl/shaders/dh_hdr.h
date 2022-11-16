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

#ifndef DH_HDR_H
#define DH_HDR_H 1

#ifdef __cplusplus
using namespace nvmath;
using mat4 = nvmath::mat4f;
using vec4 = nvmath::vec4f;
using vec3 = nvmath::vec3f;
using vec2 = nvmath::vec2f;
#endif  // __cplusplus

#ifndef WORKGROUP_SIZE
#define WORKGROUP_SIZE 16  // Grid size used by compute shaders
#endif

// Environment acceleration structure - computed in hdr_env
struct EnvAccel
{
  uint  alias;
  float q;
};

struct HdrPushBlock
{
  mat4  mvp;
  vec2  size;
  float roughness;
  uint  numSamples;
};


struct HdrDomePushConstant
{
  mat4  mvp;
  vec4  multColor;
  float rotation;
};


// clang-format off
#ifdef __cplusplus // Descriptor binding helper for C++ and GLSL
#define START_BINDING(a) enum a {
#define END_BINDING() }
#define INLINE inline
#else
#define START_BINDING(a)  const uint
#define END_BINDING()
#define INLINE
#endif

START_BINDING(EnvBindings)
eHdr = 0, 
eImpSamples = 1 
END_BINDING();

START_BINDING(EnvDomeBindings)
eHdrBrdf = 0, 
eHdrDiffuse = 1, 
eHdrSpecular = 2 
END_BINDING();

START_BINDING(EnvDomeDraw)
eHdrImage = 0
END_BINDING();

#endif  // DH_HDR_H
