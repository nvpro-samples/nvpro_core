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


#version 450

// This shader computes a diffuse irradiance IBL map using multiple importance sampling weighted
// hemisphere sampling and environment map importance sampling.

// varying inputs
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : enable

#include "dh_hdr.h"

#include "constants.h"
#include "func.h"
#include "random.h"


// clang-format off
layout(local_size_x = WORKGROUP_SIZE, local_size_y = WORKGROUP_SIZE, local_size_z = 1) in;

layout(set = 0, binding = eHdrImage) writeonly uniform image2D gOutColor;
layout(set = 1, binding = eImpSamples,  scalar)	buffer _EnvAccel { EnvAccel envSamplingData[]; };
layout(set = 1, binding = eHdr) uniform sampler2D hdrTexture;

layout(push_constant) uniform HdrPushBlock_  { HdrPushBlock pc; };
// clang-format on


#include "hdr_env_sampling.h"


void main()
{
  // Finding the world direction
  const vec2 pixelCenter = vec2(gl_GlobalInvocationID.xy) + vec2(0.5F);
  const vec2 inUV        = pixelCenter / pc.size;
  const vec2 d           = inUV * 2.0F - 1.0F;
  vec3       direction   = vec3(pc.mvp * vec4(d.x, d.y, 1.0F, 1.0F));

  // Getting axis
  vec3 tangent, bitangent;
  vec3 normal = normalize(vec3(direction.x, -direction.y, direction.z));  // Flipping Y
  orthonormalBasis(normal, tangent, bitangent);

  // Random seed
  uint seed = xxhash32(uvec3(gl_GlobalInvocationID.xyz));


  vec3  result      = vec3(0.0f);
  uint  nsamples    = 512u;
  float inv_samples = 1.0f / float(nsamples);
  for(uint i = 0u; i < nsamples; ++i)
  {
    // Importance sample diffuse BRDF.
    {
      float xi0 = (float(i) + 0.5f) * inv_samples;
      float xi1 = rand(seed);

      float phi     = 2.0f * M_PI * xi0;
      float sin_phi = sin(phi);
      float cos_phi = cos(phi);

      float sin_theta = sqrt(1.0f - xi1);
      float cos_theta = sqrt(xi1);

      vec3 d = vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);

      vec3 direction = d.x * tangent + d.y * bitangent + d.z * normal;
      vec2 uv        = getSphericalUv(direction);

      float p_brdf_sqr = cos_theta * cos_theta * (M_1_OVER_PI * M_1_OVER_PI);

      vec4  rad_pdf = texture(hdrTexture, uv);
      float p_env   = rad_pdf.a;
      float w       = p_brdf_sqr / (p_brdf_sqr + p_env * p_env);
      result += rad_pdf.rgb * w * M_PI;
    }

    // Importance sample environment.
    {
      vec3  dir;
      vec3  rand_val = vec3(rand(seed), rand(seed), rand(seed));
      vec4  rad_pdf  = environmentSample(hdrTexture, rand_val, dir);
      float pdf      = rad_pdf.a;
      vec3  value    = rad_pdf.rgb / pdf;

      float cosine     = dot(dir, normal);
      float p_brdf_sqr = cosine * cosine * (M_1_OVER_PI * M_1_OVER_PI);
      float p_env_sqr  = pdf * pdf;
      if(cosine > 0.0f)
      {
        float w = p_env_sqr / (p_env_sqr + p_brdf_sqr);
        result += w * value * cosine;
      }
    }
  }

  vec4 frag_color = vec4(result * (1.0f / float(nsamples)) / M_PI, 1.0f);
  imageStore(gOutColor, ivec2(gl_GlobalInvocationID.xy), frag_color);
}
