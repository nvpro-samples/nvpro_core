/*
 * Copyright (c) 2019-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */


#include "error_vk.hpp"

#include <nvh/nvprint.hpp>
#include <vulkan/vk_enum_string_helper.h>

namespace nvvk {

CheckResultCallback g_checkResultCallback;

void setCheckResultHook(const CheckResultCallback& callback)
{
  g_checkResultCallback = callback;
}

void checkResult(VkResult result, const char* message)
{
  if(g_checkResultCallback)
  {
    g_checkResultCallback(result, nullptr, -1, message);
    return;
  }

  if(result < 0)
  {
    if(message)
    {
      LOGE("VkResult %d - %s - %s\n", result, string_VkResult(result), message);
    }
    else
    {
      LOGE("VkResult %d - %s\n", result, string_VkResult(result));
    }
    assert(!"Critical Vulkan Error");
    exit(1);
  }
}

//--------------------------------------------------------------------------------------------------
// Check the result of Vulkan and in case of error, provide a string about what happened
//
void checkResult(VkResult result, const char* file, int32_t line)
{
  if(g_checkResultCallback)
  {
    g_checkResultCallback(result, file, line, nullptr);
    return;
  }

  if(result < 0)
  {
    LOGE("%s(%d): Vulkan Error : %s\n", file, line, string_VkResult(result));
    assert(!"Critical Vulkan Error");
    exit(1);
  }
}
}  // namespace nvvk
