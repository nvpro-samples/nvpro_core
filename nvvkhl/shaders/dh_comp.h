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


#ifndef DH_COMP_H
#define DH_COMP_H 1

/* @DOC_START
# Device/Host Polyglot Overview
Files in nvvkhl named "*.h" are designed to be compiled by both C++ and GLSL
code, so that they can share structure and function definitions.
Not all functions will be available in both C++ and GLSL.
@DOC_END */

/* @DOC_START
# `WORKGROUP_SIZE` Define
> The number of threads per workgroup in X and Y used by 2D compute shaders.

Generally, all nvvkhl compute shaders use the same workgroup size. (Workgroup
sizes of 128, 256, or 512 threads are generally good choices across GPUs.)
@DOC_END */
#define WORKGROUP_SIZE 16

#ifdef __cplusplus

/** @DOC_START
# Function `getGroupCounts`
> Returns the number of workgroups needed to cover `size` threads.
@DOC_END  */
inline VkExtent2D getGroupCounts(const VkExtent2D& size, int workgroupSize = WORKGROUP_SIZE)
{
  return VkExtent2D{(size.width + (workgroupSize - 1)) / workgroupSize, (size.height + (workgroupSize - 1)) / workgroupSize};
}
#endif  // __cplusplus

#endif  // DH_COMP_H
