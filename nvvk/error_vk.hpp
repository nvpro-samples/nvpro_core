/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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


//////////////////////////////////////////////////////////////////////////
/**
# function nvvk::checkResult
Returns true on critical error result, logs errors.
Use `NVVK_CHECK(result)` to automatically log filename/linenumber.
*/

#pragma once

#include <cassert>
#include <vulkan/vulkan_core.h>

namespace nvvk {
bool checkResult(VkResult result, const char* message = nullptr);
bool checkResult(VkResult result, const char* file, int32_t line);

#ifndef NVVK_CHECK
#define NVVK_CHECK(result) nvvk::checkResult(result, __FILE__, __LINE__)
#endif

#ifdef VULKAN_HPP
inline bool checkResult(vk::Result result, const char* message = nullptr)
{
  return checkResult((VkResult)result, message);
}
inline bool checkResult(vk::Result result, const char* file, int32_t line)
{
  return checkResult((VkResult)result, file, line);
}
#endif

}  // namespace nvvk
