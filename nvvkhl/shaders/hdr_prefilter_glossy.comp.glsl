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

// This shader computes a glossy IBL map to be used with the Unreal 4 PBR shading model as
// described in
//
// "Real Shading in Unreal Engine 4" by Brian Karis
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
//
// As an extension to the original it uses multiple importance sampling weighted BRDF importance
// sampling and environment map importance sampling to yield good results for high dynamic range
// lighting.

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : enable

#include "dh_hdr.h"


// clang-format off
layout(local_size_x = WORKGROUP_SIZE, local_size_y = WORKGROUP_SIZE, local_size_z = 1) in;


layout(set = 0, binding = eHdrImage) writeonly uniform image2D g_outColor;
layout(set = 1, binding = eImpSamples,  scalar)	buffer _EnvAccel { EnvAccel envSamplingData[]; };
layout(set = 1, binding = eHdr) uniform sampler2D hdrTexture;

layout(push_constant) uniform HdrPushBlock_  { HdrPushBlock pc; };
// clang-format on

#include "constants.h"
#include "func.h"
#include "random.h"
#include "hdr_env_sampling.h"


// Importance sample a GGX microfacet distribution.
vec3 ggxSample(vec2 xi, float alpha)
{
  float phi       = 2.0F * M_PI * xi.x;
  float cos_theta = sqrt((1.0F - xi.y) / (1.0F + (alpha * alpha - 1.0F) * xi.y));
  float sin_theta = sqrt(1.0F - cos_theta * cos_theta);

  return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

// Evaluate a GGX microfacet distribution.
float ggxEval(float alpha, float nh)
{
  float a2   = alpha * alpha;
  float nh2  = nh * nh;
  float tan2 = (1.0f - nh2) / nh2;
  float f    = a2 + tan2;
  return a2 / (f * f * M_PI * nh2 * nh);
}


struct EnvmapSampleValue
{
  vec3  dir;
  vec3  value;
  float pdf;
};


void main()
{
  const vec2 pixel_center = vec2(gl_GlobalInvocationID.xy) + vec2(0.5F);
  const vec2 in_uv        = pixel_center / vec2(pc.size);
  const vec2 d            = in_uv * 2.0F - 1.0F;
  vec3       direction    = vec3(pc.mvp * vec4(d.x, d.y, 1.0F, 1.0F));

  vec3 tangent, bitangent;
  vec3 normal = normalize(vec3(direction.x, -direction.y, direction.z));  // Flipping Y
  orthonormalBasis(normal, tangent, bitangent);

  float alpha    = pc.roughness;
  uint  nsamples = alpha > 0.0F ? 512u : 1u;


  uint state = xxhash32(uvec3(gl_GlobalInvocationID.xy, pc.roughness * 10.0F));

  // The integrals are additionally weighted by the cosine and normalized using the average cosine of
  // the importance sampled BRDF directions (as in the Unreal publication).
  float weight_sum = 0.0f;

  vec3  result       = vec3(0.0F);
  float inv_nsamples = 1.0F / float(nsamples);
  for(uint i = 0u; i < nsamples; ++i)
  {
    // Importance sample BRDF.
    {
      float xi0 = (float(i) + 0.5F) * inv_nsamples;
      float xi1 = rand(state);

      vec3 h0 = alpha > 0.0f ? ggxSample(vec2(xi0, xi1), alpha) : vec3(0.0F, 0.0F, 1.0F);
      vec3 h  = tangent * h0.x + bitangent * h0.y + normal * h0.z;

      vec3 direction = normalize(2.0 * dot(normal, h) * h - normal);

      float cos_theta = dot(normal, direction);
      if(cos_theta > 0.0F)
      {
        vec2  uv = getSphericalUv(direction);
        float w  = 1.0F;
        if(alpha > 0.0F)
        {
          float pdf_brdf_sqr = ggxEval(alpha, h0.z) * 0.25F / dot(direction, h);
          pdf_brdf_sqr *= pdf_brdf_sqr;
          float pdf_env = texture(hdrTexture, uv).a;
          w             = pdf_brdf_sqr / (pdf_brdf_sqr + pdf_env * pdf_env);
        }
        result += w * texture(hdrTexture, uv).rgb * cos_theta;
        weight_sum += cos_theta;
      }
    }

    // Importance sample environment.
    if(alpha > 0.0f)
    {
      vec3 randVal = vec3(rand(state), rand(state), rand(state));

      EnvmapSampleValue val;
      vec4              radPdf = environmentSample(hdrTexture, randVal, val.dir);
      val.pdf                  = radPdf.a;
      val.value                = radPdf.rgb / val.pdf;


      vec3  h  = normalize(normal + val.dir);
      float nh = dot(h, normal);
      float kh = dot(val.dir, h);
      float nk = dot(val.dir, normal);
      if(kh > 0.0F && nh > 0.0F && nk > 0.0F)
      {
        float pdf_env_sqr = val.pdf * val.pdf;
        float pdf_brdf    = ggxEval(alpha, nh) * 0.25F / kh;

        float w = pdf_env_sqr / (pdf_env_sqr + pdf_brdf * pdf_brdf);
        result += w * val.value * pdf_brdf * nk * nk;
      }
    }
  }

  vec4 result_color = vec4(result / float(weight_sum), 1.0F);
  imageStore(g_outColor, ivec2(gl_GlobalInvocationID.xy), result_color);
}
