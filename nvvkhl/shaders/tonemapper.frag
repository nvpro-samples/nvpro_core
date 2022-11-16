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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#version 450

#extension GL_GOOGLE_include_directive : enable

#include "dh_tonemap.h"

layout(location = 0) in vec2 i_uv;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = eTonemapperInput) uniform sampler2D g_image;


layout(push_constant) uniform shaderInformation
{
  Tonemapper tm;
};


void main()
{
  vec4 R = texture(g_image, i_uv);

  if(tm.isActive == 1)
    R.xyz = applyTonemap(tm, R.xyz, i_uv);

  o_color = R;
}
