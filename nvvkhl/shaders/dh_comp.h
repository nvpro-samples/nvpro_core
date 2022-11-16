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


#ifndef DH_COMP_H
#define DH_COMP_H 1

#define WORKGROUP_SIZE 16  // Grid size used by compute shaders

#ifdef __cplusplus
inline VkExtent2D getGroupCounts(const VkExtent2D& size)
{
  return VkExtent2D{(size.width + (WORKGROUP_SIZE - 1)) / WORKGROUP_SIZE, (size.height + (WORKGROUP_SIZE - 1)) / WORKGROUP_SIZE};
}
#endif  // __cplusplus

#endif  // DH_COMP_H
