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

// This function evaluates a material at hit position

#ifndef SHADING_GLSL
#define SHADING_GLSL

#include "ray_util.glsl"  // rayOffset
#include "lighting.glsl"  // DirectLight + Visibility contribution


void stopPath()
{
  payload.hitT = INFINITE;
}

struct ShadingResult
{
  vec3 weight;
  vec3 radiance;
  vec3 rayOrigin;
  vec3 rayDirection;
};

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
ShadingResult shading(in PbrMaterial matEval, in HitState hit, in sampler2D hdrTexture, in bool useSky)
{
  ShadingResult result;

  result.radiance = matEval.emissive;  // Emissive material

  if(result.radiance.x > 0.0F || result.radiance.y > 0.0F || result.radiance.z > 0.0F)
  {  // Stop on emmissive material
    stopPath();
    return result;
  }

  // Sampling for the next ray
  vec3  ray_direction;
  float pdf      = 0.0F;
  vec3  rand_val = vec3(rand(payload.seed), rand(payload.seed), rand(payload.seed));
  vec3  brdf     = pbrSample(matEval, -gl_WorldRayDirectionEXT, ray_direction, pdf, rand_val);

  if(dot(hit.nrm, ray_direction) > 0.0F && pdf > 0.0F)
  {
    result.weight = brdf / pdf;
  }
  else
  {
    stopPath();
    return result;
  }

  // Next ray
  result.rayDirection = ray_direction;
  result.rayOrigin    = offsetRay(hit.pos, dot(ray_direction, hit.nrm) > 0.0F ? hit.nrm : -hit.nrm);


  // Light and environment contribution at hit position
  VisibilityContribution vcontrib = directLight(matEval, hit, hdrTexture, useSky);

  if(vcontrib.visible)
  {
    // Shadow ray - stop at the first intersection, don't invoke the closest hit shader (fails for transparent objects)
    uint ray_flag = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsCullBackFacingTrianglesEXT;
    payload.hitT = 0.0F;
    traceRayEXT(topLevelAS, ray_flag, 0xFF, 0, 0, 0, result.rayOrigin, 0.001, vcontrib.lightDir, vcontrib.lightDist, 0);
    // If hitting nothing, add light contribution
    if(payload.hitT == INFINITE)
      result.radiance += vcontrib.radiance;
    payload.hitT = gl_HitTEXT;
  }

  return result;
}

#endif  // SHADING_GLSL
