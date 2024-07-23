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


#pragma once
#include <glm/glm.hpp>

#include <cstdint>  // so uint32_t is available for device_host.h below

#include "shaders/dh_sky.h"
#include "imgui/imgui_helper.h"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"
#include "shaders/dh_lighting.h"
#include "vulkan/vulkan_core.h"

//////////////////////////////////////////////////////////////////////////

namespace nvvk {
class Context;
}

namespace nvvkhl {

// Imgui UI for sky parameters
bool skyParametersUI(nvvkhl_shaders::SimpleSkyParameters& params);
bool physicalSkyUI(nvvkhl_shaders::PhysicalSkyParameters& params);

class SkyBase
{
public:
  // Cannot call virtual function in constructor: setup() must be called in derived class
  SkyBase() = default;
  virtual ~SkyBase();

  virtual void setup(const VkDevice& device, nvvk::ResourceAllocator* allocator);
  virtual void setOutImage(const VkDescriptorImageInfo& outimage);
  virtual void draw(const VkCommandBuffer& cmd, const glm::mat4& view, const glm::mat4& proj, const VkExtent2D& size);
  virtual void destroy();

  virtual void           createBuffer()       = 0;
  virtual VkShaderModule createShaderModule() = 0;

  VkDescriptorSetLayout getDescriptorSetLayout() const { return m_skyDLayout; };
  VkDescriptorSet       getDescriptorSet() const { return m_skyDSet; };

protected:
  // Resources
  VkDevice                 m_device{VK_NULL_HANDLE};
  nvvk::ResourceAllocator* m_alloc{nullptr};
  nvvk::DebugUtil          m_debug;

  // To draw the Sky in image
  VkDescriptorSet       m_skyDSet{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_skyDLayout{VK_NULL_HANDLE};
  VkDescriptorPool      m_skyDPool{VK_NULL_HANDLE};
  VkPipeline            m_skyPipeline{VK_NULL_HANDLE};
  VkPipelineLayout      m_skyPipelineLayout{VK_NULL_HANDLE};

  nvvk::Buffer m_skyInfoBuf;  // Device-Host of Sky Params
};


/** @DOC_START
# class nvvkhl::SimpleSkyDome

>  This class is responsible for a basic sky dome. 

This class can render a basic sky dome with a sun, for both the rasterizer and the ray tracer. 

The `draw` method is responsible for rendering the sky dome for the rasterizer. For ray tracing, there is no need to call this method, as the sky dome is part of the ray tracing shaders (see shaders/dh_sky.h).

@DOC_END  */
class SimpleSkyDome : public SkyBase
{
public:
  SimpleSkyDome() = default;

  void createBuffer() override;

  void           updateParameterBuffer(VkCommandBuffer cmd) const;
  bool           onUI();
  VkShaderModule createShaderModule() override;

  nvvkhl_shaders::Light                getSun() const;
  nvvkhl_shaders::SimpleSkyParameters& skyParams() { return m_skyParams; }

protected:
  nvvkhl_shaders::SimpleSkyParameters m_skyParams{};
};


/** @DOC_START
# class nvvkhl::PhysicalSkyDome

>  This class is responsible for rendering a physical sky 

This class can render a physical sky dome with a sun, for both the rasterizer and the ray tracer. 

The `draw` method is responsible for rendering the sky dome for the rasterizer. For ray tracing, there is no need to call this method, as the sky dome is part of the ray tracing shaders (see shaders/dh_sky.h).

@DOC_END  */

class PhysicalSkyDome : public SkyBase
{
public:
  PhysicalSkyDome() { m_skyParams = nvvkhl_shaders::initPhysicalSkyParameters(); }

  void           createBuffer() override;
  VkShaderModule createShaderModule() override;

  void                  updateParameterBuffer(VkCommandBuffer cmd) const;
  bool                  onUI();
  nvvkhl_shaders::Light getSun() const;

  nvvkhl_shaders::PhysicalSkyParameters& skyParams() { return m_skyParams; }

protected:
  nvvkhl_shaders::PhysicalSkyParameters m_skyParams{};
};

}  // namespace nvvkhl
