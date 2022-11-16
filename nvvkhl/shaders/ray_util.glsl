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

#ifndef RAY_UTIL_H
#define RAY_UTIL_H 1

precision highp float;

//-------------------------------------------------------------------------------------------------
// Avoiding self intersections (see Ray Tracing Gems, Ch. 6)
//-----------------------------------------------------------------------
vec3 offsetRay(in vec3 p, in vec3 n)
{
  const float int_scale   = 256.0f;
  const float float_scale = 1.0f / 65536.0f;
  const float origin      = 1.0f / 32.0f;

  ivec3 of_i = ivec3(int_scale * n.x, int_scale * n.y, int_scale * n.z);

  vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0.F) ? -of_i.x : of_i.x)),
                  intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0.F) ? -of_i.y : of_i.y)),
                  intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0.F) ? -of_i.z : of_i.z)));

  return vec3(abs(p.x) < origin ? p.x + float_scale * n.x : p_i.x,  //
              abs(p.y) < origin ? p.y + float_scale * n.y : p_i.y,  //
              abs(p.z) < origin ? p.z + float_scale * n.z : p_i.z);
}

// Hacking the shadow terminator
// https://jo.dreggn.org/home/2021_terminator.pdf
// p : point of intersection
// p[a..c]: position of the triangle
// n[a..c]: normal of the triangle
// bary: barycentric coordinate of the hit position
// return the offset position
vec3 pointOffset(vec3 p, vec3 pa, vec3 pb, vec3 pc, vec3 na, vec3 nb, vec3 nc, vec3 bary)
{
  vec3 tmpu  = p - pa;
  vec3 tmpv  = p - pb;
  vec3 tmpw  = p - pc;

  float dotu = min(0.0F, dot(tmpu, na));
  float dotv = min(0.0F, dot(tmpv, nb));
  float dotw = min(0.0F, dot(tmpw, nc));

  tmpu -= dotu * na;
  tmpv -= dotv * nb;
  tmpw -= dotw * nc;

  vec3 pP = p + tmpu * bary.x + tmpv * bary.y + tmpw * bary.z;

  return pP;
}


#endif
