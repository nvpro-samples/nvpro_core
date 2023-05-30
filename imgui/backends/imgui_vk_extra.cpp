/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#include "imgui_vk_extra.h"
#include "../imgui_helper.h"

static ImGui_ImplVulkan_InitInfo g_VulkanInitInfo = {};

static void check_vk_result(VkResult err)
{
  assert(err == VK_SUCCESS);
}

void ImGui::InitVK(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, uint32_t queueFamilyIndex, VkRenderPass pass, int subPassIndex)
{
  VkResult err = VK_RESULT_MAX_ENUM;

  std::vector<VkDescriptorPoolSize> poolSize{{VK_DESCRIPTOR_TYPE_SAMPLER, 1}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
  VkDescriptorPoolCreateInfo        poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.maxSets       = 2;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
  poolInfo.pPoolSizes    = poolSize.data();
  vkCreateDescriptorPool(device, &poolInfo, nullptr, &g_VulkanInitInfo.DescriptorPool);

  // Setup Platform/Renderer backends
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance                  = VkInstance(666);  // <--- WRONG need argument
  init_info.PhysicalDevice            = physicalDevice;
  init_info.Device                    = device;
  init_info.QueueFamily               = queueFamilyIndex;
  init_info.Queue                     = queue;
  init_info.PipelineCache             = VK_NULL_HANDLE;
  init_info.DescriptorPool            = g_VulkanInitInfo.DescriptorPool;
  init_info.Subpass                   = subPassIndex;
  init_info.MinImageCount             = 2;
  init_info.ImageCount                = 3;                      // <--- WRONG need argument
  init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;  // <--- need argument?
  init_info.Allocator                 = nullptr;
  init_info.CheckVkResultFn           = nullptr;
  ImGui_ImplVulkan_Init(&init_info, pass);
  g_VulkanInitInfo = init_info;

  // Upload Fonts
  VkCommandPool           pool = VK_NULL_HANDLE;
  VkCommandPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  createInfo.queueFamilyIndex = queueFamilyIndex;
  err                         = vkCreateCommandPool(device, &createInfo, nullptr, &pool);
  check_vk_result(err);

  VkCommandBuffer             cmd = VK_NULL_HANDLE;
  VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool        = pool;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;
  err                          = vkAllocateCommandBuffers(device, &allocInfo, &cmd);
  check_vk_result(err);

  VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  err             = vkBeginCommandBuffer(cmd, &beginInfo);
  check_vk_result(err);

  // Creation of the font
  ImGui_ImplVulkan_CreateFontsTexture(cmd);

  err = vkEndCommandBuffer(cmd);
  check_vk_result(err);

  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount = 1;
  submit.pCommandBuffers    = &cmd;
  err                       = vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
  check_vk_result(err);

  err = vkDeviceWaitIdle(device);
  check_vk_result(err);
  vkFreeCommandBuffers(device, pool, 1, &cmd);
  vkDestroyCommandPool(device, pool, nullptr);
}


void ImGui::ShutdownVK()
{
  ImGui_ImplVulkan_InitInfo* v = &g_VulkanInitInfo;
  vkDestroyDescriptorPool(v->Device, v->DescriptorPool, v->Allocator);
  ImGui_ImplVulkan_Shutdown();
}
