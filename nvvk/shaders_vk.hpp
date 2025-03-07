/*
 * Copyright (c) 2018-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2018-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */


#pragma once

#include <assert.h>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "nsight_aftermath_vk.hpp"
#include "error_vk.hpp"

namespace nvvk {
/** @DOC_START
# functions in nvvk

- createShaderModule : create the shader module from various binary code inputs
- createShaderStageInfo: create the shader module and setup the stage from the incoming binary code
@DOC_END */

// setting `doCheck` false means nvvk_check is not run, and therefore the function is guaranteed to make progress
inline VkShaderModule createShaderModule(VkDevice device, const uint32_t* binarycode, size_t sizeInBytes, bool doCheck = true)
{
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize                 = sizeInBytes;
  createInfo.pCode                    = binarycode;

  VkShaderModule shaderModule = VK_NULL_HANDLE;
  VkResult       result       = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);

  if(doCheck)
  {
    NVVK_CHECK(result);
  }

  if(result == VK_SUCCESS)
  {
    GpuCrashTracker::getInstance().addShaderBinary(std::vector<uint32_t>(binarycode, binarycode + sizeInBytes / 4));
  }

  return shaderModule;
}

inline VkShaderModule createShaderModule(VkDevice device, const void* binarycode, size_t sizeInBytes)
{
  return createShaderModule(device, (const uint32_t*)binarycode, sizeInBytes);
}


inline VkShaderModule createShaderModule(VkDevice device, const char* binarycode, size_t numInt32)
{
  return createShaderModule(device, (const uint32_t*)binarycode, numInt32 * 4);
}

inline VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
{
  return createShaderModule(device, (const uint32_t*)code.data(), code.size());
}

inline VkShaderModule createShaderModule(VkDevice device, const std::vector<uint8_t>& code)
{
  return createShaderModule(device, (const uint32_t*)code.data(), code.size());
}


inline VkShaderModule createShaderModule(VkDevice device, const std::vector<uint32_t>& code)
{
  return createShaderModule(device, code.data(), 4 * code.size());
}

inline VkShaderModule createShaderModule(VkDevice device, const std::string& code)
{
  return createShaderModule(device, (const uint32_t*)code.data(), code.size());
}

template <typename T>
inline VkPipelineShaderStageCreateInfo createShaderStageInfo(VkDevice              device,
                                                             const std::vector<T>& code,
                                                             VkShaderStageFlagBits stage,
                                                             const char*           entryPoint = "main")
{
  VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  shaderStage.stage  = stage;
  shaderStage.module = createShaderModule(device, code);
  shaderStage.pName  = entryPoint;
  return shaderStage;
}

inline VkPipelineShaderStageCreateInfo createShaderStageInfo(VkDevice              device,
                                                             const std::string&    code,
                                                             VkShaderStageFlagBits stage,
                                                             const char*           entryPoint = "main")
{
  VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  shaderStage.stage  = stage;
  shaderStage.module = createShaderModule(device, code);
  shaderStage.pName  = entryPoint;
  return shaderStage;
}
}  // namespace nvvk
