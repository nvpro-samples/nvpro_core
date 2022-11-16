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


#ifndef CONSTANTS_GLSL
#define CONSTANTS_GLSL


precision highp float;

const float M_PI        = 3.1415926535897F;  // PI
const float M_TWO_PI    = 6.2831853071795F;  // 2*PI
const float M_PI_2      = 1.5707963267948F;  // PI/2
const float M_PI_4      = 0.7853981633974F;  // PI/4
const float M_1_OVER_PI = 0.3183098861837F;  // 1/PI
const float M_2_OVER_PI = 0.6366197723675F;  // 2/PI

const float INFINITE = 1e32F;

#endif  // CONSTANTS_GLSL