/*
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef CONSTANTS_GLSL
#define CONSTANTS_GLSL

/* @DOC_START
Useful mathematical constants:
* `M_PI` (pi)
* `M_TWO_PI` (2*pi)
* `M_PI_2` (pi/2)
* `M_PI_4` (pi/4)
* `M_1_OVER_PI` (1/pi)
* `M_2_OVER_PI` (2/pi)
* `M_1_PI` (also 1/pi)
* `INFINITE` (infinity)
@DOC_END */

#ifndef __cplusplus
precision highp float;
#endif


#ifndef M_PI
const float M_PI = 3.1415926535897F;  // PI
#endif

#ifndef M_TWO_PI
const float M_TWO_PI = 6.2831853071795F;  // 2*PI
#endif

#ifndef M_PI_2
const float M_PI_2 = 1.5707963267948F;  // PI/2
#endif

#ifndef M_PI_4
const float M_PI_4 = 0.7853981633974F;  // PI/4
#endif

#ifndef M_1_OVER_PI
const float M_1_OVER_PI = 0.3183098861837F;  // 1/PI
#endif

#ifndef M_2_OVER_PI
const float M_2_OVER_PI = 0.6366197723675F;  // 2/PI
#endif

#ifndef M_1_PI
const float M_1_PI = 0.3183098861837F;  // 1/PI
#endif

#ifndef INFINITE
const float INFINITE = 1e32F;
#endif

#endif  // CONSTANTS_GLSL