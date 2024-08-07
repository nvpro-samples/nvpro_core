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
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "dh_sky.h"
// clang-format off
layout(local_size_x = WORKGROUP_SIZE, local_size_y = WORKGROUP_SIZE, local_size_z = 1) in;
layout(set = 0, binding = eSkyOutImage) writeonly uniform image2D g_out_hdr;
layout(set = 0, binding = eSkyParam) uniform SkyInfo_ {  PhysicalSkyParameters skyInfo; };
layout(push_constant) uniform SkyDomePushConstant_ { SkyPushConstant pc; };
// clang-format on

void main()
{
  const vec2 pixelCenter = vec2(gl_GlobalInvocationID.xy) + vec2(0.5F);
  const vec2 texCoord    = pixelCenter / vec2(imageSize(g_out_hdr));
  const vec2 d           = texCoord * 2.0 - 1.0;
  vec3       direction   = normalize(vec3(pc.mvp * vec4(d.x, d.y, 1.0F, 1.0F)));

  vec3 color = evalPhysicalSky(skyInfo, direction);
  imageStore(g_out_hdr, ivec2(gl_GlobalInvocationID.xy), vec4(color, 1.0F));
}
