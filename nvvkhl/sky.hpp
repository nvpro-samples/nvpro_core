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


#pragma once
#include "nvmath/nvmath.h"

#include <cstdint>  // so uint32_t is available for device_host.h below
#include "shaders/dh_sky.h"
#include "shaders/dh_lighting.h"
#include "vulkan/vulkan_core.h"
#include "nvvk/resourceallocator_vk.hpp"
#include "nvvk/debug_util_vk.hpp"
//////////////////////////////////////////////////////////////////////////

namespace nvvk {
class Context;
}

namespace nvvkhl {
struct SkyParameters
{
  vec3  skyColor{0.17F, 0.37F, 0.65F};
  vec3  horizonColor{0.50F, 0.70F, 0.92F};
  vec3  groundColor{0.62F, 0.59F, 0.55F};
  vec3  directionUp{0.F, 1.F, 0.F};
  float brightness       = 0.3F;   // scaler for sky brightness
  float horizonSize      = 30.F;   // +/- degrees
  float glowSize         = 5.F;    // degrees, starting from the edge of the light disk
  float glowIntensity    = 0.1F;   // [0-1] relative to light intensity
  float glowSharpness    = 4.F;    // [1-10] is the glow power exponent
  float maxLightRadiance = 100.F;  // clamp for light radiance derived from its angular size, 0 = no clamp
  // Sun
  float angularSize = nv_to_rad * 0.53F;
  float intensity   = 1.0F;
  vec3  direction   = nvmath::normalize(vec3{0.0F, -.7F, -.7F});
  vec3  color       = {1.0F, 1.0F, 1.0F};
};


class SkyDome
{
public:
  SkyDome(nvvk::Context* ctx, nvvk::ResourceAllocator* allocator);
  void setup(const VkDevice& device, nvvk::ResourceAllocator* allocator);
  void setOutImage(const VkDescriptorImageInfo& outimage);
  void draw(const VkCommandBuffer& cmd, const nvmath::mat4f& view, const nvmath::mat4f& proj, const VkExtent2D& size);
  void destroy();
  void updateParameterBuffer(VkCommandBuffer cmd) const;
  bool onUI();

  Light                 getSun() const;
  VkDescriptorSetLayout getDescriptorSetLayout() const { return m_skyDLayout; };
  VkDescriptorSet       getDescriptorSet() const { return m_skyDSet; };
  SkyParameters&        skyParams() { return m_skyParams; }

private:
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

  SkyParameters m_skyParams{};
};

}  // namespace nvvkhl
