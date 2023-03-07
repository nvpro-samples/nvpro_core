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

#include "sky.hpp"

#include "nvvkhl/shaders/dh_comp.h"
#include "_autogen/sky.comp.h"

// clang-format off
template <typename T> inline T square(T a) {return a * a;}
// clang-format on

namespace nvvkhl {
static ProceduralSkyShaderParameters fillShaderParameters(const SkyParameters& input)
{
  ProceduralSkyShaderParameters output{};

  float light_angular_size = nvmath::clamp(input.angularSize, nv_to_rad * 0.1F, nv_to_rad * 90.F);
  float light_solid_angle  = 4.0F * nv_pi * square(sinf(light_angular_size * 0.5F));
  float light_radiance     = input.intensity / light_solid_angle;

  if(input.maxLightRadiance > 0.F)
  {
    light_radiance = std::min(light_radiance, input.maxLightRadiance);
  }

  output.directionToLight   = nvmath::normalize(-input.direction);
  output.angularSizeOfLight = light_angular_size;
  output.lightColor         = light_radiance * input.color;
  output.glowSize           = nv_to_rad * nvmath::clamp(input.glowSize, 0.F, 90.F);
  output.skyColor           = input.skyColor * input.brightness;
  output.glowIntensity      = nvmath::clamp(input.glowIntensity, 0.F, 1.F);
  output.horizonColor       = input.horizonColor * input.brightness;
  output.horizonSize        = nv_to_rad * nvmath::clamp(input.horizonSize, 0.F, 90.F);
  output.groundColor        = input.groundColor * input.brightness;
  output.glowSharpness      = nvmath::clamp(input.glowSharpness, 1.F, 10.F);
  output.directionUp        = normalize(input.directionUp);

  return output;
}

SkyDome::SkyDome(nvvk::Context* ctx, nvvk::ResourceAllocator* allocator)
{
  setup(ctx->m_device, allocator);
}

void SkyDome::setup(const VkDevice& device, nvvk::ResourceAllocator* allocator)
{
  m_device = device;
  m_alloc  = allocator;
  m_debug.setup(device);

  m_skyInfoBuf = m_alloc->createBuffer(sizeof(ProceduralSkyShaderParameters),
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  NAME2_VK(m_skyInfoBuf.buffer, "SkyInfo");

  // Descriptor: the output image and parameters
  nvvk::DescriptorSetBindings bind;
  bind.addBinding(SkyBindings::eSkyOutImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT);
  bind.addBinding(SkyBindings::eSkyParam, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL);
  m_skyDLayout = bind.createLayout(m_device);
  m_skyDPool   = bind.createPool(m_device);
  m_skyDSet    = nvvk::allocateDescriptorSet(m_device, m_skyDPool, m_skyDLayout);

  // Write parameters information
  std::vector<VkWriteDescriptorSet> writes = {};
  VkDescriptorBufferInfo            buf_info{m_skyInfoBuf.buffer, 0, VK_WHOLE_SIZE};
  writes.emplace_back(bind.makeWrite(m_skyDSet, SkyBindings::eSkyParam, &buf_info));
  vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

  // Creating the pipeline layout
  VkPushConstantRange                push_constant_ranges = {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SkyPushConstant)};
  std::vector<VkDescriptorSetLayout> layouts              = {m_skyDLayout};
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

void SkyDome::draw(const VkCommandBuffer& cmd, const nvmath::mat4f& view, const nvmath::mat4f& proj, const VkExtent2D& size)
{
  LABEL_SCOPE_VK(cmd);

  // This will be to have a world direction vector pointing to the pixel
  nvmath::mat4f m = nvmath::invert(proj);
  m.a30 = m.a31 = m.a32 = m.a33 = 0.0F;

  m = nvmath::invert(view) * m;

  // Information to the compute shader
  SkyPushConstant pc{};
  pc.mvp = m;

  // Execution
  std::vector<VkDescriptorSet> dst_sets = {m_skyDSet};
  vkCmdPushConstants(cmd, m_skyPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SkyPushConstant), &pc);
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
  ProceduralSkyShaderParameters output = fillShaderParameters(m_skyParams);
  vkCmdUpdateBuffer(cmd, m_skyInfoBuf.buffer, 0, sizeof(ProceduralSkyShaderParameters), &output);

  // Make sure the buffer is available when using it
  VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  mb.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
  mb.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
}

Light SkyDome::getSun() const
{
  Light sun{};
  sun.type                  = eLightTypeDirectional;
  sun.angularSizeOrInvRange = m_skyParams.angularSize;
  sun.direction             = m_skyParams.direction;
  sun.color                 = m_skyParams.color;
  sun.intensity             = m_skyParams.intensity;
  return sun;
}

bool SkyDome::onUI()
{
  using PE = ImGuiH::PropertyEditor;

  bool changed{false};

  nvmath::vec3f dir = m_skyParams.direction;
  changed |= ImGuiH::azimuthElevationSliders(dir, true, m_skyParams.directionUp.y == 1.0F);
  m_skyParams.direction = dir;
  // clang-format off
    changed |= PE::entry("Color", [&]() { return ImGui::ColorEdit3("##1", &m_skyParams.color.x, ImGuiColorEditFlags_Float);                                   });
    changed |= PE::entry("Irradiance", [&]() { return ImGui::SliderFloat("##1", &m_skyParams.intensity, 0.F, 100.F, "%.2f", ImGuiSliderFlags_Logarithmic);    });
    changed |= PE::entry("Angular Size", [&]() { return ImGui::SliderAngle("##1", &m_skyParams.angularSize, 0.1F, 20.F);                                      });
  // clang-format on


  if(PE::treeNode("Extra"))
  {
    // clang-format off
      changed |= PE::entry("Brightness", [&]() { return ImGui::SliderFloat("Brightness", &m_skyParams.brightness, 0.F, 1.F);         });
      changed |= PE::entry("Glow Size", [&]() { return ImGui::SliderFloat("Glow Size", &m_skyParams.glowSize, 0.F, 90.F);           });
      changed |= PE::entry("Glow Sharpness", [&]() { return ImGui::SliderFloat("Glow Sharpness", &m_skyParams.glowSharpness, 1.F, 10.F); });
      changed |= PE::entry("Glow Intensity", [&]() { return ImGui::SliderFloat("Glow Intensity", &m_skyParams.glowIntensity, 0.F, 1.F);  });
      changed |= PE::entry("Horizon Size", [&]() { return ImGui::SliderFloat("Horizon Size", &m_skyParams.horizonSize, 0.F, 90.F);     });
      changed |= PE::entry("Sky Color", [&]() { return ImGui::ColorEdit3("Sky Color", &m_skyParams.skyColor.x, ImGuiColorEditFlags_Float);         });
      changed |= PE::entry("Horizon Color", [&]() { return ImGui::ColorEdit3("Horizon Color", &m_skyParams.horizonColor.x, ImGuiColorEditFlags_Float); });
      changed |= PE::entry("Ground Color", [&]() { return ImGui::ColorEdit3("Ground Color", &m_skyParams.groundColor.x, ImGuiColorEditFlags_Float);   });
    // clang-format on
    PE::treePop();
  }

  return changed;
}

}  // namespace nvvkhl
