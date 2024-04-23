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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#include "imgui/imgui_helper.h"
#include "nvh/nvprint.hpp"
#include "nvvk/shaders_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/context_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"

#include <glm/gtc/constants.hpp>
using namespace glm;
#include "sky.hpp"

#include "nvvkhl/shaders/dh_comp.h"
#include "_autogen/sky.comp.h"


namespace nvvkhl {


SkyDome::SkyDome(nvvk::Context* ctx, nvvk::ResourceAllocator* allocator)
{
  setup(ctx->m_device, allocator);
}

void SkyDome::setup(const VkDevice& device, nvvk::ResourceAllocator* allocator)
{
  m_device = device;
  m_alloc  = allocator;
  m_debug.setup(device);

  m_skyInfoBuf = m_alloc->createBuffer(sizeof(nvvkhl_shaders::ProceduralSkyShaderParameters),
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  NAME2_VK(m_skyInfoBuf.buffer, "SkyInfo");

  // Descriptor: the output image and parameters
  nvvk::DescriptorSetBindings bind;
  bind.addBinding(nvvkhl_shaders::SkyBindings::eSkyOutImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT);
  bind.addBinding(nvvkhl_shaders::SkyBindings::eSkyParam, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL);
  m_skyDLayout = bind.createLayout(m_device);
  m_skyDPool   = bind.createPool(m_device);
  m_skyDSet    = nvvk::allocateDescriptorSet(m_device, m_skyDPool, m_skyDLayout);

  // Write parameters information
  std::vector<VkWriteDescriptorSet> writes = {};
  VkDescriptorBufferInfo            buf_info{m_skyInfoBuf.buffer, 0, VK_WHOLE_SIZE};
  writes.emplace_back(bind.makeWrite(m_skyDSet, nvvkhl_shaders::SkyBindings::eSkyParam, &buf_info));
  vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

  // Creating the pipeline layout
  VkPushConstantRange push_constant_ranges = {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(nvvkhl_shaders::SkyPushConstant)};
  std::vector<VkDescriptorSetLayout> layouts = {m_skyDLayout};
  VkPipelineLayoutCreateInfo         create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  create_info.setLayoutCount         = static_cast<uint32_t>(layouts.size());
  create_info.pSetLayouts            = layouts.data();
  create_info.pushConstantRangeCount = 1;
  create_info.pPushConstantRanges    = &push_constant_ranges;
  vkCreatePipelineLayout(m_device, &create_info, nullptr, &m_skyPipelineLayout);
  NAME_VK(m_skyPipelineLayout);

  // HDR Dome compute shader
  VkPipelineShaderStageCreateInfo stage_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  stage_info.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  stage_info.module = nvvk::createShaderModule(m_device, sky_comp, sizeof(sky_comp));
  stage_info.pName  = "main";

  VkComputePipelineCreateInfo comp_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  comp_info.layout = m_skyPipelineLayout;
  comp_info.stage  = stage_info;

  vkCreateComputePipelines(m_device, {}, 1, &comp_info, nullptr, &m_skyPipeline);
  NAME_VK(m_skyPipeline);

  // Clean up
  vkDestroyShaderModule(m_device, comp_info.stage.module, nullptr);
}

void SkyDome::setOutImage(const VkDescriptorImageInfo& outimage)
{
  VkWriteDescriptorSet wds{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  wds.dstSet          = m_skyDSet;
  wds.dstBinding      = 0;
  wds.descriptorCount = 1;
  wds.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  wds.pImageInfo      = &outimage;
  vkUpdateDescriptorSets(m_device, 1, &wds, 0, nullptr);
}

void SkyDome::draw(const VkCommandBuffer& cmd, const glm::mat4& view, const glm::mat4& proj, const VkExtent2D& size)
{
  LABEL_SCOPE_VK(cmd);

  // Information to the compute shader
  nvvkhl_shaders::SkyPushConstant pc{};
  pc.mvp = glm::inverse(view) * glm::inverse(proj); // This will be to have a world direction vector pointing to the pixel

  // Execution
  std::vector<VkDescriptorSet> dst_sets = {m_skyDSet};
  vkCmdPushConstants(cmd, m_skyPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(nvvkhl_shaders::SkyPushConstant), &pc);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_skyPipelineLayout, 0,
                          static_cast<uint32_t>(dst_sets.size()), dst_sets.data(), 0, nullptr);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_skyPipeline);
  VkExtent2D group_counts = getGroupCounts(size);
  vkCmdDispatch(cmd, group_counts.width, group_counts.height, 1);
}

void SkyDome::destroy()
{
  m_alloc->destroy(m_skyInfoBuf);

  vkDestroyPipeline(m_device, m_skyPipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_skyPipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_skyDLayout, nullptr);
  vkDestroyDescriptorPool(m_device, m_skyDPool, nullptr);
}

void SkyDome::updateParameterBuffer(VkCommandBuffer cmd) const
{
  nvvkhl_shaders::ProceduralSkyShaderParameters output = fillSkyShaderParameters(m_skyParams);
  vkCmdUpdateBuffer(cmd, m_skyInfoBuf.buffer, 0, sizeof(nvvkhl_shaders::ProceduralSkyShaderParameters), &output);

  // Make sure the buffer is available when using it
  VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  mb.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
  mb.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
}

nvvkhl_shaders::Light SkyDome::getSun() const
{
  nvvkhl_shaders::Light sun{};
  sun.type                  = nvvkhl_shaders::eLightTypeDirectional;
  sun.angularSizeOrInvRange = m_skyParams.angularSize;
  sun.direction             = m_skyParams.direction;
  sun.color                 = m_skyParams.color;
  sun.intensity             = m_skyParams.intensity;
  return sun;
}

bool SkyDome::onUI()
{
  return skyParametersUI(m_skyParams);
}

}  // namespace nvvkhl
