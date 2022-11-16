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


//-------------------------------------------------------------------------------------------------
// This fils has all the Gltf sampling and evaluation methods


#ifndef PBR_EVAL_H
#define PBR_EVAL_H 1

#include "ggx.glsl"
#include "pbr_mat_struct.h"
#include "constants.glsl"
#include "func.glsl"

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
vec3 cosineSampleHemisphere(float r1, float r2)
{
  vec3  dir;
  float r   = sqrt(r1);
  float phi = M_TWO_PI * r2;
  dir.x     = r * cos(phi);
  dir.y     = r * sin(phi);
  dir.z     = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.y * dir.y));

  return dir;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
vec3 pbrEval(in PbrMaterial state, in vec3 incomingRay, in vec3 toLight, inout float pdf)
{
  // L = Direction from surface point to light
  // V = Direction from surface point to Eye
  vec3  normal   = state.normal;
  vec3  half_vec = normalize(toLight + incomingRay);
  float n_dot_l  = clampedDot(normal, toLight);
  float n_dot_v  = clampedDot(normal, incomingRay);
  float n_dot_h  = clampedDot(normal, half_vec);
  float l_dot_h  = clampedDot(toLight, half_vec);
  float v_dot_h  = clampedDot(incomingRay, half_vec);

  vec3 f_specular = vec3(0.0F);
  vec3 f_diffuse  = vec3(0.0F);

  if(n_dot_l > 0.0F || n_dot_v > 0.0F)
  {
    float diffuse_ratio  = 0.5F * (1.0F - state.metallic);
    float specular_ratio = 1.0F - diffuse_ratio;

    vec3  f90             = vec3(1.0F);
    float specular_weight = 1.0F;
    float alpha_roughness = state.roughness;

    f_diffuse  = brdfLambertian(state.albedo.xyz, state.metallic);
    f_specular = brdfSpecularGGX(state.f0, f90, alpha_roughness, v_dot_h, n_dot_l, n_dot_v, n_dot_h);

    pdf += diffuse_ratio * (n_dot_l * M_1_OVER_PI);                                       // diffuse
    pdf += specular_ratio * dGgx(n_dot_h, alpha_roughness) * n_dot_h / (4.0F * l_dot_h);  // specular
  }

  return (f_diffuse + f_specular) * n_dot_l;
}


//-----------------------------------------------------------------------
// Return a direction for the sampling
//-----------------------------------------------------------------------
vec3 bsdfSample(in PbrMaterial state, in vec3 incomingRay, in vec3 randVal)
{
  vec3  normal      = state.normal;
  float probability = randVal.x;

  float diffuse_ratio = 0.5F * (1.0F - state.metallic);
  float r1            = randVal.y;
  float r2            = randVal.z;
  vec3  tangent, binormal;
  orthonormalBasis(normal, tangent, binormal);

  if(probability <= diffuse_ratio)
  {  // Diffuse
    vec3 to_light = cosineSampleHemisphere(r1, r2);
    return to_light.x * tangent + to_light.y * binormal + to_light.z * normal;
  }
  else
  {  // Specular
    float specular_alpha = state.roughness;
    vec3  half_vec       = ggxSampling(specular_alpha, r1, r2);
    half_vec             = tangent * half_vec.x + binormal * half_vec.y + normal * half_vec.z;
    return reflect(-incomingRay, half_vec);
  }
}


//-----------------------------------------------------------------------
// Sample and evaluate
//-----------------------------------------------------------------------
vec3 pbrSample(in PbrMaterial state, vec3 incomingRay, inout vec3 toLight, inout float pdf, in vec3 randVal)
{
  // Find bouncing ray
  toLight = bsdfSample(state, incomingRay, randVal);

  vec3  normal  = state.normal;
  float n_dot_l = dot(normal, toLight);

  // Early out
  pdf = 0.0F;
  if(n_dot_l < 0.0F)
    return vec3(0.0F);

  // Sampling with new light direction
  return pbrEval(state, incomingRay, toLight, pdf);
}

#endif  // PBR_EVAL_H
