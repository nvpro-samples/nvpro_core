/*
 * Copyright (c) 2022-2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2024, NVIDIA CORPORATION.
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
#include "nvvkhl/shaders/dh_sky.h"

#include "imgui/imgui_helper.h"

#include "_autogen/sky.comp.glsl.h"
#include "_autogen/sky_physical.comp.glsl.h"


namespace nvvkhl {


SkyBase::~SkyBase()
{
  destroy();
}

void SimpleSkyDome::createBuffer()
{
  VkDeviceSize bufferSize = sizeof(nvvkhl_shaders::SimpleSkyParameters);
  m_skyInfoBuf = m_alloc->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void PhysicalSkyDome::createBuffer()
{
  VkDeviceSize bufferSize = sizeof(nvvkhl_shaders::PhysicalSkyParameters);
  m_skyInfoBuf = m_alloc->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

VkShaderModule SimpleSkyDome::createShaderModule()
{
  return nvvk::createShaderModule(m_device, sky_comp_glsl, sizeof(sky_comp_glsl));
}

VkShaderModule PhysicalSkyDome::createShaderModule()
{
  return nvvk::createShaderModule(m_device, sky_physical_comp_glsl, sizeof(sky_physical_comp_glsl));
}


void SkyBase::setup(const VkDevice& device, nvvk::ResourceAllocator* allocator)
{
  m_device = device;
  m_alloc  = allocator;
  m_debug.setup(device);

  createBuffer();  // Virtual
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
  stage_info.module = createShaderModule();
  stage_info.pName  = "main";

  VkComputePipelineCreateInfo comp_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  comp_info.layout = m_skyPipelineLayout;
  comp_info.stage  = stage_info;

  vkCreateComputePipelines(m_device, {}, 1, &comp_info, nullptr, &m_skyPipeline);
  NAME_VK(m_skyPipeline);

  // Clean up
  vkDestroyShaderModule(m_device, comp_info.stage.module, nullptr);
}

void SkyBase::setOutImage(const VkDescriptorImageInfo& outimage)
{
  VkWriteDescriptorSet wds{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  wds.dstSet          = m_skyDSet;
  wds.dstBinding      = 0;
  wds.descriptorCount = 1;
  wds.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  wds.pImageInfo      = &outimage;
  vkUpdateDescriptorSets(m_device, 1, &wds, 0, nullptr);
}

void SkyBase::draw(const VkCommandBuffer& cmd, const glm::mat4& view, const glm::mat4& proj, const VkExtent2D& size)
{
  LABEL_SCOPE_VK(cmd);

  // Information to the compute shader
  nvvkhl_shaders::SkyPushConstant pc{};

  // Remove translation from view
  glm::mat4 viewNoTrans = view;
  viewNoTrans[3]        = {0.0f, 0.0f, 0.0f, 1.0f};
  pc.mvp = glm::inverse(proj * viewNoTrans);  // This will be to have a world direction vector pointing to the pixel

  // Execution
  std::vector<VkDescriptorSet> dst_sets = {m_skyDSet};
  vkCmdPushConstants(cmd, m_skyPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(nvvkhl_shaders::SkyPushConstant), &pc);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_skyPipelineLayout, 0,
                          static_cast<uint32_t>(dst_sets.size()), dst_sets.data(), 0, nullptr);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_skyPipeline);
  VkExtent2D group_counts = getGroupCounts(size);
  vkCmdDispatch(cmd, group_counts.width, group_counts.height, 1);
}

void SkyBase::destroy()
{
  m_alloc->destroy(m_skyInfoBuf);

  vkDestroyPipeline(m_device, m_skyPipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_skyPipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_skyDLayout, nullptr);
  vkDestroyDescriptorPool(m_device, m_skyDPool, nullptr);
  m_skyPipeline       = VK_NULL_HANDLE;
  m_skyPipelineLayout = VK_NULL_HANDLE;
  m_skyDLayout        = VK_NULL_HANDLE;
  m_skyDPool          = VK_NULL_HANDLE;
}


void memoryBarrier(VkCommandBuffer cmd)
{
  // Make sure the buffer is available when using it
  VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  mb.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
  mb.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                       VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                       0, 1, &mb, 0, nullptr, 0, nullptr);
}


SimpleSkyDome::SimpleSkyDome()
    : m_skyParams(nvvkhl_shaders::initSimpleSkyParameters())
{
}

void SimpleSkyDome::updateParameterBuffer(VkCommandBuffer cmd) const
{
  vkCmdUpdateBuffer(cmd, m_skyInfoBuf.buffer, 0, sizeof(nvvkhl_shaders::SimpleSkyParameters), &m_skyParams);
  memoryBarrier(cmd);  // Make sure the buffer is available when using it
}


PhysicalSkyDome::PhysicalSkyDome()
    : m_skyParams(nvvkhl_shaders::initPhysicalSkyParameters())
{
}


void PhysicalSkyDome::updateParameterBuffer(VkCommandBuffer cmd) const
{
  vkCmdUpdateBuffer(cmd, m_skyInfoBuf.buffer, 0, sizeof(nvvkhl_shaders::PhysicalSkyParameters), &m_skyParams);
  memoryBarrier(cmd);  // Make sure the buffer is available when using it
}

nvvkhl_shaders::Light SimpleSkyDome::getSun() const
{
  nvvkhl_shaders::Light sun{};
  sun.type                  = nvvkhl_shaders::eLightTypeDirectional;
  sun.angularSizeOrInvRange = m_skyParams.angularSizeOfLight;
  sun.direction             = -m_skyParams.directionToLight;
  sun.color                 = m_skyParams.sunColor;
  sun.intensity             = m_skyParams.sunIntensity;

  return sun;
}

nvvkhl_shaders::Light PhysicalSkyDome::getSun() const
{
  nvvkhl_shaders::Light sun{};
  sun.type                  = nvvkhl_shaders::eLightTypeDirectional;
  sun.angularSizeOrInvRange = 0.00465f * m_skyParams.sunDiskScale;
  sun.direction             = -m_skyParams.sunDirection;
  sun.color                 = glm::vec3(1, 1, 1);
  sun.intensity             = m_skyParams.sunDiskIntensity;

  return sun;
}


bool SimpleSkyDome::onUI()
{
  return skyParametersUI(m_skyParams);
}

bool PhysicalSkyDome::onUI()
{
  return physicalSkyUI(m_skyParams);
}

bool skyParametersUI(nvvkhl_shaders::SimpleSkyParameters& params)
{
  namespace PE = ImGuiH::PropertyEditor;

  bool changed{false};

  changed |= ImGuiH::azimuthElevationSliders(params.directionToLight, false, params.directionUp.y >= params.directionUp.z);
  changed |= PE::ColorEdit3("Color", &params.sunColor.x, ImGuiColorEditFlags_Float);
  changed |= PE::SliderFloat("Irradiance", &params.sunIntensity, 0.F, 100.F, "%.2f", ImGuiSliderFlags_Logarithmic);
  changed |= PE::SliderAngle("Angular Size", &params.angularSizeOfLight, 0.1F, 20.F);
  params.angularSizeOfLight = glm::clamp(params.angularSizeOfLight, glm::radians(0.1F), glm::radians(90.F));

  auto  square           = [](auto a) { return a * a; };
  float lightAngularSize = glm::clamp(params.angularSizeOfLight, glm::radians(0.1F), glm::radians(90.F));
  float lightSolidAngle  = 4.0F * glm::pi<float>() * square(sinf(lightAngularSize * 0.5F));
  float lightRadiance    = params.sunIntensity / lightSolidAngle;
  params.lightRadiance   = params.sunColor * lightRadiance;

  if(PE::treeNode("Extra"))
  {
    changed |= PE::SliderFloat("Brightness", &params.brightness, 0.F, 1.F);
    changed |= PE::SliderAngle("Glow Size", &params.glowSize, 0.F, 20.F);
    changed |= PE::SliderFloat("Glow Sharpness", &params.glowSharpness, 1.F, 10.F);
    changed |= PE::SliderFloat("Glow Intensity", &params.glowIntensity, 0.F, 1.F);
    changed |= PE::SliderAngle("Horizon Size", &params.horizonSize, 0.F, 90.F);
    changed |= PE::ColorEdit3("Sky Color", &params.skyColor.x, ImGuiColorEditFlags_Float);
    changed |= PE::ColorEdit3("Horizon Color", &params.horizonColor.x, ImGuiColorEditFlags_Float);
    changed |= PE::ColorEdit3("Ground Color", &params.groundColor.x, ImGuiColorEditFlags_Float);
    PE::treePop();
  }

  return changed;
}

bool physicalSkyUI(nvvkhl_shaders::PhysicalSkyParameters& params)
{
  namespace PE = ImGuiH::PropertyEditor;


  bool changed{false};
  if(PE::entry(
         "", [&] { return ImGui::SmallButton("reset"); }, "Default values"))
  {
    params  = nvvkhl_shaders::initPhysicalSkyParameters();
    changed = true;
  }
  changed |= ImGuiH::azimuthElevationSliders(params.sunDirection, false, params.yIsUp == 1);
  changed |= PE::SliderFloat("Sun Disk Scale", &params.sunDiskScale, 0.F, 10.F);
  changed |= PE::SliderFloat("Sun Disk Intensity", &params.sunDiskIntensity, 0.F, 5.F);
  changed |= PE::SliderFloat("Sun Glow Intensity", &params.sunGlowIntensity, 0.F, 5.F);

  if(PE::treeNode("Extra"))
  {
    changed |= PE::SliderFloat("Haze", &params.haze, 0.F, 15.F);
    changed |= PE::SliderFloat("Red Blue Shift", &params.redblueshift, -1.F, 1.F);
    changed |= PE::SliderFloat("Saturation", &params.saturation, 0.F, 1.F);
    changed |= PE::SliderFloat("Horizon Height", &params.horizonHeight, -1.F, 1.F);
    changed |= PE::ColorEdit3("Ground Color", &params.groundColor.x, ImGuiColorEditFlags_Float);
    changed |= PE::SliderFloat("Horizon Blur", &params.horizonBlur, 0.F, 5.F);
    changed |= PE::ColorEdit3("Night Color", &params.nightColor.x, ImGuiColorEditFlags_Float);
    PE::treePop();
  }

  return changed;
}

}  // namespace nvvkhl
