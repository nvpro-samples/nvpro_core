/*
 * Copyright (c) 2022-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vulkan/vulkan_core.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3.h>


#include "nvvk/error_vk.hpp"

namespace nvvkhl {

struct QueueInfo
{
  uint32_t familyIndex = ~0U;
  uint32_t queueIndex  = ~0U;
  VkQueue  queue       = VK_NULL_HANDLE;
};

inline VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool cmdPool)
{
  const VkCommandBufferAllocateInfo allocInfo{.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                              .commandPool        = cmdPool,
                                              .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                              .commandBufferCount = 1};
  VkCommandBuffer                   cmd{};
  NVVK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &cmd));
  const VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                           .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  NVVK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));
  return cmd;
}

inline void endSingleTimeCommands(VkCommandBuffer cmd, VkDevice device, VkCommandPool cmdPool, VkQueue queue)
{
  // Submit and clean up
  NVVK_CHECK(vkEndCommandBuffer(cmd));

  // Create fence for synchronization
  const VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  VkFence                 fence{};
  NVVK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &fence));

  const VkCommandBufferSubmitInfo cmdBufferInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = cmd};
  VkSubmitInfo2 submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .commandBufferInfoCount = 1, .pCommandBufferInfos = &cmdBufferInfo};
  NVVK_CHECK(vkQueueSubmit2(queue, 1, &submitInfo, fence));
  NVVK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

  // Cleanup
  vkDestroyFence(device, fence, nullptr);
  vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
}

//-------------------------------------------------------------------------------------------------
// Helper to add required extension for Window / Surface creation
//
template <typename T>
inline void addSurfaceExtensions(std::vector<T>& instanceExtensions)
{
  int result = glfwInit();
  assert(result == GLFW_TRUE);
  uint32_t     count;
  const char** extensions = glfwGetRequiredInstanceExtensions(&count);
  for(uint32_t i = 0; i < count; i++)
  {
    instanceExtensions.emplace_back(extensions[i]);
  }
  instanceExtensions.emplace_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
}


}  // namespace nvvkhl
