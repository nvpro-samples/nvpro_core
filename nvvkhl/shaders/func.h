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

#ifndef NVVKHL_FUNC_H
#define NVVKHL_FUNC_H 1

/* @DOC_START
Useful utility functions for shaders.
@DOC_END */

#include "constants.h"

#ifdef __cplusplus
#ifndef OUT_TYPE
#define OUT_TYPE(T) T&
#endif
using glm::atan;
using glm::clamp;
using glm::cross;
using glm::dot;
using glm::mat4;
using glm::max;
using glm::mix;
using glm::normalize;
using glm::sqrt;
using glm::vec2;
using glm::vec3;
using glm::vec4;
#else
#define OUT_TYPE(T) out T
precision highp float;
#define static
#define inline
#endif


inline float square(float x)
{
  return x * x;
}

/* @DOC_START
# Function `saturate`
> Clamps a value to the range [0,1].
@DOC_END */
inline float saturate(float x)
{
  return clamp(x, 0.0F, 1.0F);
}

inline vec3 saturate(vec3 x)
{
  return clamp(x, vec3(0.0F), vec3(1.0F));
}

/* @DOC_START
# Function `luminance`
> Returns the luminance of a linear RGB color, using Rec. 709 coefficients.
@DOC_END */
inline float luminance(vec3 color)
{
  return color.x * 0.2126F + color.y * 0.7152F + color.z * 0.0722F;
}

/* @DOC_START
# Function `clampedDot`
> Takes the dot product of two values and clamps the result to [0,1].
@DOC_END */
inline float clampedDot(vec3 x, vec3 y)
{
  return clamp(dot(x, y), 0.0F, 1.0F);
}

/* @DOC_START
# Function `orthonormalBasis`
> Builds an orthonormal basis: given only a normal vector, returns a
tangent and bitangent.

This uses the technique from "Improved accuracy when building an orthonormal
basis" by Nelson Max, https://jcgt.org/published/0006/01/02.

Any tangent-generating algorithm must produce at least one discontinuity
when operating on a sphere (due to the hairy ball theorem); this has a
small ring-shaped discontinuity at `normal.z == -0.99998796`.
@DOC_END */
inline void orthonormalBasis(vec3 normal, OUT_TYPE(vec3) tangent, OUT_TYPE(vec3) bitangent)
{
  if(normal.z < -0.99998796F)  // Handle the singularity
  {
    tangent   = vec3(0.0F, -1.0F, 0.0F);
    bitangent = vec3(-1.0F, 0.0F, 0.0F);
    return;
  }
  float a   = 1.0F / (1.0F + normal.z);
  float b   = -normal.x * normal.y * a;
  tangent   = vec3(1.0F - normal.x * normal.x * a, b, -normal.x);
  bitangent = vec3(b, 1.0f - normal.y * normal.y * a, -normal.y);
}

/* @DOC_START
# Function `makeFastTangent`
> Like `orthonormalBasis()`, but returns a tangent and tangent sign that matches
> the glTF convention.
@DOC_END */
inline vec4 makeFastTangent(vec3 normal)
{
  vec3 tangent, unused;
  orthonormalBasis(normal, tangent, unused);
  // The glTF bitangent sign here is 1.f since for
  // normal == vec3(0.0F, 0.0F, 1.0F), we get
  // tangent == vec3(1.0F, 0.0F, 0.0F) and bitangent == vec3(0.0F, 1.0F, 0.0F),
  // so bitangent = cross(normal, tangent).
  return vec4(tangent, 1.f);
}

/* @DOC_START
# Function `rotate`
> Rotates the vector `v` around the unit direction `k` by an angle of `theta`.

At `theta == pi/2`, returns `cross(k, v) + k * dot(k, v)`. This means that
rotations are clockwise in right-handed coordinate systems and
counter-clockwise in left-handed coordinate systems.
@DOC_END */
inline vec3 rotate(vec3 v, vec3 k, float theta)
{
  float cos_theta = cos(theta);
  float sin_theta = sin(theta);

  return (v * cos_theta) + (cross(k, v) * sin_theta) + (k * dot(k, v)) * (1.0F - cos_theta);
}


/* @DOC_START
# Function `getSphericalUv`
> Given a direction, returns the UV coordinate of an environment map for that
> direction using a spherical projection.
@DOC_END */
inline vec2 getSphericalUv(vec3 v)
{
  float gamma = asin(-v.y);
  float theta = atan(v.z, v.x);

  vec2 uv = vec2(theta * M_1_OVER_PI * 0.5F, gamma * M_1_OVER_PI) + 0.5F;
  return uv;
}

/* @DOC_START
# Function `mixBary`
> Interpolates between 3 values, using the barycentric coordinates of a triangle.
@DOC_END */
inline vec2 mixBary(vec2 a, vec2 b, vec2 c, vec3 bary)
{
  return a * bary.x + b * bary.y + c * bary.z;
}

inline vec3 mixBary(vec3 a, vec3 b, vec3 c, vec3 bary)
{
  return a * bary.x + b * bary.y + c * bary.z;
}

inline vec4 mixBary(vec4 a, vec4 b, vec4 c, vec3 bary)
{
  return a * bary.x + b * bary.y + c * bary.z;
}

/* @DOC_START
# Function `cosineSampleHemisphere
> Samples a hemisphere using a cosine-weighted distribution.

See https://www.realtimerendering.com/raytracinggems/unofficial_RayTracingGems_v1.4.pdf,
section 16.6.1, "COSINE-WEIGHTED HEMISPHERE ORIENTED TO THE Z-AXIS".
@DOC_END */
inline vec3 cosineSampleHemisphere(float r1, float r2)
{
  float r   = sqrt(r1);
  float phi = M_TWO_PI * r2;
  vec3  dir;
  dir.x = r * cos(phi);
  dir.y = r * sin(phi);
  dir.z = sqrt(max(0.F, 1.F - dir.x * dir.x - dir.y * dir.y));
  return dir;
}

/* @DOC_START
# Function `powerHeuristic`
> The power heuristic for multiple importance sampling, with `beta = 2`.

See equation 9.13 of https://graphics.stanford.edu/papers/veach_thesis/thesis.pdf.
@DOC_END */
float powerHeuristic(float a, float b)
{
  const float t = a * a;
  return t / (b * b + t);
}

#undef OUT_TYPE
#endif  // NVVKHL_FUNC_H