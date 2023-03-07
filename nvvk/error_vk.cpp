/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#include "error_vk.hpp"

#include <nvh/nvprint.hpp>

namespace nvvk {

CheckResultCallback g_checkResultCallback;

void setCheckResultHook(const CheckResultCallback& callback)
{
  g_checkResultCallback = callback;
}

const char* getResultString(VkResult result)
{
  const char* resultString = "unknown";

#define STR(a)                                                                                                         \
  case a:                                                                                                              \
    resultString = #a;                                                                                                 \
    break;

  switch(result)
  {
    STR(VK_SUCCESS);
    STR(VK_NOT_READY);
    STR(VK_TIMEOUT);
    STR(VK_EVENT_SET);
    STR(VK_EVENT_RESET);
    STR(VK_INCOMPLETE);
    STR(VK_ERROR_OUT_OF_HOST_MEMORY);
    STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    STR(VK_ERROR_INITIALIZATION_FAILED);
    STR(VK_ERROR_DEVICE_LOST);
    STR(VK_ERROR_MEMORY_MAP_FAILED);
    STR(VK_ERROR_LAYER_NOT_PRESENT);
    STR(VK_ERROR_EXTENSION_NOT_PRESENT);
    STR(VK_ERROR_FEATURE_NOT_PRESENT);
    STR(VK_ERROR_INCOMPATIBLE_DRIVER);
    STR(VK_ERROR_TOO_MANY_OBJECTS);
    STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
    STR(VK_ERROR_FRAGMENTED_POOL);
    STR(VK_ERROR_OUT_OF_POOL_MEMORY);
    STR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
    STR(VK_ERROR_SURFACE_LOST_KHR);
    STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
    STR(VK_SUBOPTIMAL_KHR);
    STR(VK_ERROR_OUT_OF_DATE_KHR);
    STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
    STR(VK_ERROR_VALIDATION_FAILED_EXT);
    STR(VK_ERROR_INVALID_SHADER_NV);
    STR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
    STR(VK_ERROR_FRAGMENTATION_EXT);
    STR(VK_ERROR_NOT_PERMITTED_EXT);
    STR(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT);
    STR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
  }
#undef STR
  return resultString;
}

bool checkResult(VkResult result, const char* message)
{
  if(g_checkResultCallback)
    return g_checkResultCallback(result, nullptr, -1, message);

  if(result == VK_SUCCESS)
  {
    return false;
  }

  if(result < 0)
  {
    if(message)
    {
      LOGE("VkResult %d - %s - %s\n", result, getResultString(result), message);
    }
    else
    {
      LOGE("VkResult %d - %s\n", result, getResultString(result));
    }
    assert(!"Critical Vulkan Error");
    return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------
// Check the result of Vulkan and in case of error, provide a string about what happened
//
bool checkResult(VkResult result, const char* file, int32_t line)
{
  if(g_checkResultCallback)
    return g_checkResultCallback(result, file, line, nullptr);

  if(result == VK_SUCCESS)
  {
    return false;
  }

  if(result < 0)
  {
    LOGE("%s(%d): Vulkan Error : %s\n", file, line, getResultString(result));
    assert(!"Critical Vulkan Error");

    return true;
  }

  return false;
}
}  // namespace nvvk
