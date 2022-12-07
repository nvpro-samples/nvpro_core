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

#ifndef LIGHTING_H
#define LIGHTING_H 1

#include "func.glsl"              // misc functions
#include "dh_lighting.h"          // struct VisibilityContribution
#include "pbr_mat_struct.h"       // struct PbrMaterial
#include "hdr_env_sampling.glsl"  // Environment Sample
#include "light_contrib.glsl"     // light contribution


VisibilityContribution lightsContribution(in PbrMaterial pbrMat, in vec3 to_eye, in vec3 pos, in vec3 nrm, int nbLights, inout uint seed)
{
  VisibilityContribution contrib;
  contrib.radiance = vec3(0.0F);
  contrib.visible  = false;
  vec3  light_dir;
  float light_pdf = 1 / frameInfo.nbLights;
  vec3  light_contrib;
  float light_dist = 1e32F;
  bool  is_light   = false;

  {
    // randomly select one of the lights
    int   light_index = int(min(rand(seed) * frameInfo.nbLights, frameInfo.nbLights));
    Light light       = frameInfo.light[light_index];

    LightContrib contrib = singleLightContribution(light, pos, nrm, to_eye);

    light_contrib = contrib.intensity;
    light_dir     = normalize(-contrib.incidentVector);

    if(contrib.halfAngularSize > 0.0F)
    {  // <----- Sampling area lights
      vec2  rand_val     = vec2(rand(seed), rand(seed));
      float angular_size = contrib.halfAngularSize;

      // section 34  https://people.cs.kuleuven.be/~philip.dutre/GI/TotalCompendium.pdf
      vec3  dir;
      float tmp   = (1.0F - rand_val.y * (1.0F - cos(angular_size)));
      float tmp2  = tmp * tmp;
      float tetha = sqrt(1.0F - tmp2);
      dir.x       = cos(M_TWO_PI * rand_val.x) * tetha;
      dir.y       = sin(M_TWO_PI * rand_val.x) * tetha;
      dir.z       = tmp;

      vec3 tangent, binormal;
      orthonormalBasis(light_dir, tangent, binormal);
      mat3 tbn  = mat3(tangent, binormal, light_dir);
      light_dir = normalize(tbn * dir);
    }

    light_dist = (light.type == eLightTypeDirectional) ? 1e37F : length(pos - light.position);
  }

  float dotNL = dot(light_dir, nrm);
  if(dotNL > 0.0F)
  {
    float pdf      = 0.0F;
    vec3  brdf     = pbrEval(pbrMat, to_eye, light_dir, pdf);
    vec3  radiance = brdf * light_contrib / light_pdf;

    contrib.visible   = true;
    contrib.lightDir  = light_dir;
    contrib.lightDist = light_dist;
    contrib.radiance  = radiance;
  }

  return contrib;
}


VisibilityContribution environmentLightingContribution(in PbrMaterial pbrMat, in vec3 to_eye, in vec3 nrm, in vec3 envColor, in float envRotation, inout uint seed)
{
  VisibilityContribution contrib;
  contrib.radiance  = vec3(0.0F);
  contrib.visible   = false;
  contrib.lightDist = INFINITE;
  vec3  light_dir;
  vec3  light_contrib = vec3(0.0F);
  float light_pdf;

  {  // <------ Adding HDR sampling
    vec3 rand_val     = vec3(rand(seed), rand(seed), rand(seed));
    vec4 radiance_pdf = environmentSample(hdrTexture, rand_val, light_dir);
    light_dir         = rotate(light_dir, vec3(0.0F, 1.0F, 0.0F), envRotation);
    light_contrib     = radiance_pdf.xyz * envColor.xyz;
    light_pdf         = radiance_pdf.w;
  }

  float dotNL = dot(light_dir, nrm);
  if(dotNL > 0.0)
  {
    float pdf        = 0.0F;
    vec3  brdf       = pbrEval(pbrMat, to_eye, light_dir, pdf);
    float mis_weight = max(0.0F, powerHeuristic(light_pdf, pdf));
    vec3  radiance   = mis_weight * brdf * light_contrib / light_pdf;

    contrib.visible  = true;
    contrib.lightDir = light_dir;
    contrib.radiance = radiance;
  }

  return contrib;
}


#endif