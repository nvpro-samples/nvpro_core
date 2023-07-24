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

#ifndef FUNC_GLSL
#define FUNC_GLSL 1

#include "constants.glsl"

precision highp float;

float square(float x)
{
  return x * x;
}

float saturate(float x)
{
  return clamp(x, 0.0F, 1.0F);
}

vec3 saturate(vec3 x)
{
  return clamp(x, vec3(0.0F), vec3(1.0F));
}

vec3 slerp(vec3 a, vec3 b, float angle, float t)
{
  t            = saturate(t);
  float sin1   = sin(angle * t);
  float sin2   = sin(angle * (1.0F - t));
  float ta     = sin1 / (sin1 + sin2);
  vec3  result = mix(a, b, ta);
  return normalize(result);
}

float clampedDot(vec3 x, vec3 y)
{
  return clamp(dot(x, y), 0.0F, 1.0F);
}

// Return the tangent and binormal from the incoming normal
void createCoordinateSystem(in vec3 normal, out vec3 tangent, out vec3 bitangent)
{
  if(abs(normal.x) > abs(normal.y))
    tangent = vec3(normal.z, 0.0F, -normal.x) / sqrt(normal.x * normal.x + normal.z * normal.z);
  else
    tangent = vec3(0.0F, -normal.z, normal.y) / sqrt(normal.y * normal.y + normal.z * normal.z);
  bitangent = cross(normal, tangent);
}

//-----------------------------------------------------------------------
// Building an Orthonormal Basis, Revisited
// by Tom Duff, James Burgess, Per Christensen, Christophe Hery, Andrew Kensler, Max Liani, Ryusuke Villemin
// https://graphics.pixar.com/library/OrthonormalB/
//-----------------------------------------------------------------------
void orthonormalBasis(in vec3 normal, out vec3 tangent, out vec3 bitangent)
{
  float sgn = normal.z > 0.0F ? 1.0F : -1.0F;
  float a   = -1.0F / (sgn + normal.z);
  float b   = normal.x * normal.y * a;

  tangent   = vec3(1.0f + sgn * normal.x * normal.x * a, sgn * b, -sgn * normal.x);
  bitangent = vec3(b, sgn + normal.y * normal.y * a, -normal.y);
}

vec3 rotate(vec3 v, vec3 k, float theta)
{
  float cos_theta = cos(theta);
  float sin_theta = sin(theta);

  return (v * cos_theta) + (cross(k, v) * sin_theta) + (k * dot(k, v)) * (1.0F - cos_theta);
}


//-----------------------------------------------------------------------
// Return the UV in a lat-long HDR map
//-----------------------------------------------------------------------
vec2 getSphericalUv(vec3 v)
{
  float gamma = asin(-v.y);
  float theta = atan(v.z, v.x);

  vec2 uv = vec2(theta * M_1_OVER_PI * 0.5F, gamma * M_1_OVER_PI) + 0.5F;
  return uv;
}

//-----------------------------------------------------------------------
// Return the interpolated value between 3 values and the barycentrics
//-----------------------------------------------------------------------
vec2 mixBary(vec2 a, vec2 b, vec2 c, vec3 bary)
{
  return a * bary.x + b * bary.y + c * bary.z;
}

vec3 mixBary(vec3 a, vec3 b, vec3 c, vec3 bary)
{
  return a * bary.x + b * bary.y + c * bary.z;
}

vec4 mixBary(vec4 a, vec4 b, vec4 c, vec3 bary)
{
  return a * bary.x + b * bary.y + c * bary.z;
}

//-----------------------------------------------------------------------
// https://www.realtimerendering.com/raytracinggems/unofficial_RayTracingGems_v1.4.pdf
// 16.6.1 COSINE-WEIGHTED HEMISPHERE ORIENTED TO THE Z-AXIS
//-----------------------------------------------------------------------
vec3 cosineSampleHemisphere(float r1, float r2)
{
  float r   = sqrt(r1);
  float phi = M_TWO_PI * r2;
  vec3  dir;
  dir.x = r * cos(phi);
  dir.y = r * sin(phi);
  dir.z = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.y * dir.y));
  return dir;
}
#endif  // FUNC_GLSL