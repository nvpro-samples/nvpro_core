/*
 * Copyright (c) 2021-2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2021-2024, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "imgui/backends/imgui_vk_extra.h"
#include "imgui/imgui_helper.h"
#include <backends/imgui_impl_vulkan.h>

static ImGui_ImplVulkan_InitInfo s_VulkanInitInfo = {};

static void check_vk_result(VkResult err)
{
  assert(err == VK_SUCCESS);
}

void ImGui::InitVK(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, uint32_t queueFamilyIndex, VkRenderPass pass, uint32_t subPassIndex)
{
  VkResult err = VK_RESULT_MAX_ENUM;

  assert(s_VulkanInitInfo.Device == nullptr);

  // Setup Platform/Renderer backends
  ImGui_ImplVulkan_InitInfo& init_info = s_VulkanInitInfo;

  init_info = {};

  std::vector<VkDescriptorPoolSize> poolSize{{VK_DESCRIPTOR_TYPE_SAMPLER, 1}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
  VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets       = 2;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
  poolInfo.pPoolSizes    = poolSize.data();
  vkCreateDescriptorPool(device, &poolInfo, nullptr, &init_info.DescriptorPool);

  init_info.Instance        = VkInstance(666);  // <--- WRONG need argument
  init_info.PhysicalDevice  = physicalDevice;
  init_info.Device          = device;
  init_info.QueueFamily     = queueFamilyIndex;
  init_info.Queue           = queue;
  init_info.PipelineCache   = VK_NULL_HANDLE;
  init_info.RenderPass      = pass;
  init_info.Subpass         = subPassIndex;
  init_info.MinImageCount   = 2;
  init_info.ImageCount      = 3;  // required for cyclic usage
  init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator       = nullptr;
  init_info.CheckVkResultFn = nullptr;

  ImGui_ImplVulkan_Init(&init_info);

  ImGui_ImplVulkan_CreateFontsTexture();
}

void ImGui::InitVK(VkInstance                           instance,
                   VkDevice                             device,
                   VkPhysicalDevice                     physicalDevice,
                   VkQueue                              queue,
                   uint32_t                             queueFamilyIndex,
                   const VkPipelineRenderingCreateInfo& dynamicRendering)
{
  VkResult err = VK_RESULT_MAX_ENUM;

  assert(s_VulkanInitInfo.Device == nullptr);

  // Setup Platform/Renderer backends
  ImGui_ImplVulkan_InitInfo& init_info = s_VulkanInitInfo;

  init_info = {};

  std::vector<VkDescriptorPoolSize> poolSize{{VK_DESCRIPTOR_TYPE_SAMPLER, 1}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
  VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets       = 2;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
  poolInfo.pPoolSizes    = poolSize.data();
  vkCreateDescriptorPool(device, &poolInfo, nullptr, &init_info.DescriptorPool);


  init_info.Instance        = instance;
  init_info.PhysicalDevice  = physicalDevice;
  init_info.Device          = device;
  init_info.QueueFamily     = queueFamilyIndex;
  init_info.Queue           = queue;
  init_info.PipelineCache   = VK_NULL_HANDLE;
  init_info.RenderPass      = VK_NULL_HANDLE;
  init_info.Subpass         = 0;
  init_info.MinImageCount   = 2;
  init_info.ImageCount      = 3;  // required for cyclic usage
  init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator       = nullptr;
  init_info.CheckVkResultFn = nullptr;

  // ImGui pulls dynamic rendering functions from instance
  assert(instance);
  assert(dynamicRendering.pNext == nullptr);
  init_info.UseDynamicRendering         = VK_TRUE;
  init_info.PipelineRenderingCreateInfo = dynamicRendering;

  ImGui_ImplVulkan_Init(&init_info);

  ImGui_ImplVulkan_CreateFontsTexture();
}

void ImGui::ShutdownVK()
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplVulkan_InitInfo* v = &s_VulkanInitInfo;
  vkDestroyDescriptorPool(v->Device, v->DescriptorPool, v->Allocator);

  s_VulkanInitInfo = {};
}