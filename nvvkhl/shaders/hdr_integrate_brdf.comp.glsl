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

// This shader computes a glossy BRDF map to be used with the Unreal 4 PBR shading model as
// described in
//
// "Real Shading in Unreal Engine 4" by Brian Karis
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
//


#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "dh_hdr.h"

layout(set = 0, binding = eHdrImage) writeonly uniform image2D gOutColor;
layout(local_size_x = WORKGROUP_SIZE, local_size_y = WORKGROUP_SIZE, local_size_z = 1) in;


const float M_PI = 3.14159265359F;

// See http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float radinv(uint bits)
{
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10;  // / 0x100000000
}

vec2 hammersley2D(uint i, uint N)
{
  return vec2(float(i) / float(N), radinv(i));
}

vec3 ggxSample(vec2 xi, vec3 normal, float alpha)
{
  // compute half-vector in spherical coordinates
  float phi       = 2.0F * M_PI * xi.x;
  float cos_theta = sqrt((1.0F - xi.y) / (1.0F + (alpha * alpha - 1.0F) * xi.y));
  float sin_theta = sqrt(1.0F - cos_theta * cos_theta);

  return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

float geometrySchlickGgx(float ndotv, float roughness)
{
  // note that we use a different k for IBL
  float a = roughness;
  float k = (a * a) / 2.0F;

  float nom   = ndotv;
  float denom = ndotv * (1.0F - k) + k;

  return nom / denom;
}

float geometrySmith(vec3 normal, vec3 view, vec3 light, float roughness)
{
  float ndotv = max(dot(normal, view), 0.0F);
  float ndotl = max(dot(normal, light), 0.0F);
  float g1    = geometrySchlickGgx(ndotv, roughness);
  float g2    = geometrySchlickGgx(ndotl, roughness);

  return g1 * g2;
}

vec2 integrateBrdf(float ndotv, float roughness)
{
  vec3 view;
  view.x = sqrt(1.0F - ndotv * ndotv);  // sin
  view.y = 0.0;
  view.z = ndotv;

  float A = 0.0F;
  float B = 0.0F;

  const vec3 normal = vec3(0.0F, 0.0F, 1.0F);

  const uint nsamples = 1024u;
  float      alpha    = roughness * roughness;

  for(uint i = 0u; i < nsamples; ++i)
  {
    vec2 xi = hammersley2D(i, nsamples);
    vec3 h0 = ggxSample(xi, normal, alpha);
    vec3 h  = vec3(h0.y, -h0.x, h0.z);

    vec3 light = normalize(2.0F * dot(view, h) * h - view);

    float ndotl = max(light.z, 0.0F);
    float ndoth = max(h.z, 0.0F);
    float vdoth = max(dot(view, h), 0.0F);

    if(ndotl > 0.0)
    {
      float G     = geometrySmith(normal, view, light, roughness);
      float G_Vis = (G * vdoth) / (ndoth * ndotv);
      float Fc    = pow(1.0 - vdoth, 5.0F);

      A += (1.0 - Fc) * G_Vis;
      B += Fc * G_Vis;
    }
  }
  A /= float(nsamples);
  B /= float(nsamples);
  return vec2(A, B);
}

void main()
{
  const vec2 pixel_center = vec2(gl_GlobalInvocationID.xy) + vec2(0.5F);
  const vec2 in_uv        = pixel_center / vec2(imageSize(gOutColor));
  vec2       brdf         = integrateBrdf(in_uv.s, 1.0F - in_uv.t);
  imageStore(gOutColor, ivec2(gl_GlobalInvocationID.xy), vec4(brdf, 0.0F, 0.0F));
}
