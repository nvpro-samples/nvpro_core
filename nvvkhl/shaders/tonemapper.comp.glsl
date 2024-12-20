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

#include "dh_tonemap.h"
#include "dh_comp.h"


layout(set = 0, binding = eTonemapperInput) uniform sampler2D g_image;
layout(set = 0, binding = eTonemapperOutput) writeonly uniform image2D g_out_image;

layout(push_constant) uniform shaderInformation
{
  Tonemapper tm;
};

layout(local_size_x = WORKGROUP_SIZE, local_size_y = WORKGROUP_SIZE, local_size_z = 1) in;


void main()
{
  if(gl_GlobalInvocationID.xy != clamp(gl_GlobalInvocationID.xy, vec2(0.0F), imageSize(g_out_image)))
    return;

  const vec2 pixel_center = vec2(gl_GlobalInvocationID.xy) + vec2(0.5F);
  const vec2 i_uv         = pixel_center / vec2(imageSize(g_out_image));

  vec4 R = texture(g_image, i_uv);

  if(tm.isActive == 1)
  {
    R.xyz = applyTonemap(tm, R.xyz, i_uv);
  }

  imageStore(g_out_image, ivec2(gl_GlobalInvocationID.xy), R);
}
