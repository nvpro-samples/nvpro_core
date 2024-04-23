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
struct SkyParameters
{
  glm::vec3 skyColor{0.17F, 0.37F, 0.65F};
  glm::vec3 horizonColor{0.50F, 0.70F, 0.92F};
  glm::vec3 groundColor{0.62F, 0.59F, 0.55F};
  glm::vec3 directionUp{0.F, 1.F, 0.F};
  float     brightness       = 0.3F;   // scaler for sky brightness
  float     horizonSize      = 30.F;   // +/- degrees
  float     glowSize         = 5.F;    // degrees, starting from the edge of the light disk
  float     glowIntensity    = 0.1F;   // [0-1] relative to light intensity
  float     glowSharpness    = 4.F;    // [1-10] is the glow power exponent
  float     maxLightRadiance = 100.F;  // clamp for light radiance derived from its angular size, 0 = no clamp
  // Sun
  float     angularSize = glm::radians(0.53F);
  float     intensity   = 1.0F;
  glm::vec3 direction   = glm::normalize(glm::vec3{0.0F, -.7F, -.7F});
  glm::vec3 color       = {1.0F, 1.0F, 1.0F};
};

inline nvvkhl_shaders::ProceduralSkyShaderParameters fillSkyShaderParameters(const SkyParameters& input)
{
  nvvkhl_shaders::ProceduralSkyShaderParameters output{};

  auto  square             = [](auto a) { return a * a; };
  float light_angular_size = glm::clamp(input.angularSize, glm::radians(0.1F), glm::radians(90.F));
  float light_solid_angle  = 4.0F * glm::pi<float>() * square(sinf(light_angular_size * 0.5F));
  float light_radiance     = input.intensity / light_solid_angle;

  if(input.maxLightRadiance > 0.F)
  {
    light_radiance = std::min(light_radiance, input.maxLightRadiance);
  }

  output.directionToLight   = glm::normalize(-input.direction);
  output.angularSizeOfLight = light_angular_size;
  output.lightColor         = light_radiance * input.color;
  output.glowSize           = glm::radians(glm::clamp(input.glowSize, 0.F, 90.F));
  output.skyColor           = input.skyColor * input.brightness;
  output.glowIntensity      = glm::clamp(input.glowIntensity, 0.F, 1.F);
  output.horizonColor       = input.horizonColor * input.brightness;
  output.horizonSize        = glm::radians(glm::clamp(input.horizonSize, 0.F, 90.F));
  output.groundColor        = input.groundColor * input.brightness;
  output.glowSharpness      = glm::clamp(input.glowSharpness, 1.F, 10.F);
  output.directionUp        = normalize(input.directionUp);

  return output;
}


inline bool skyParametersUI(SkyParameters& skyParams)
{
  using PE = ImGuiH::PropertyEditor;

  bool changed{false};

  glm::vec3 dir = skyParams.direction;
  changed |= ImGuiH::azimuthElevationSliders(dir, true, skyParams.directionUp.y == 1.0F);
  skyParams.direction = dir;
  // clang-format off
    changed |= PE::entry("Color", [&]() { return ImGui::ColorEdit3("##1", &skyParams.color.x, ImGuiColorEditFlags_Float);                                   });
    changed |= PE::entry("Irradiance", [&]() { return ImGui::SliderFloat("##1", &skyParams.intensity, 0.F, 100.F, "%.2f", ImGuiSliderFlags_Logarithmic);    });
    changed |= PE::entry("Angular Size", [&]() { return ImGui::SliderAngle("##1", &skyParams.angularSize, 0.1F, 20.F);                                      });
  // clang-format on


  if(PE::treeNode("Extra"))
  {
    // clang-format off
      changed |= PE::entry("Brightness", [&]() { return ImGui::SliderFloat("Brightness", &skyParams.brightness, 0.F, 1.F);         });
      changed |= PE::entry("Glow Size", [&]() { return ImGui::SliderFloat("Glow Size", &skyParams.glowSize, 0.F, 90.F);           });
      changed |= PE::entry("Glow Sharpness", [&]() { return ImGui::SliderFloat("Glow Sharpness", &skyParams.glowSharpness, 1.F, 10.F); });
      changed |= PE::entry("Glow Intensity", [&]() { return ImGui::SliderFloat("Glow Intensity", &skyParams.glowIntensity, 0.F, 1.F);  });
      changed |= PE::entry("Horizon Size", [&]() { return ImGui::SliderFloat("Horizon Size", &skyParams.horizonSize, 0.F, 90.F);     });
      changed |= PE::entry("Sky Color", [&]() { return ImGui::ColorEdit3("Sky Color", &skyParams.skyColor.x, ImGuiColorEditFlags_Float);         });
      changed |= PE::entry("Horizon Color", [&]() { return ImGui::ColorEdit3("Horizon Color", &skyParams.horizonColor.x, ImGuiColorEditFlags_Float); });
      changed |= PE::entry("Ground Color", [&]() { return ImGui::ColorEdit3("Ground Color", &skyParams.groundColor.x, ImGuiColorEditFlags_Float);   });
    // clang-format on
    PE::treePop();
  }

  return changed;
}


/** @DOC_START
# class nvvkhl::SkyDome

>  This class is responsible for the sky dome. 

This class can render a sky dome with a sun, for both the rasterizer and the ray tracer. 

The `draw` method is responsible for rendering the sky dome for the rasterizer. For ray tracing, there is no need to call this method, as the sky dome is part of the ray tracing shaders (see shaders/dh_sky.h).

@DOC_END  */
class SkyDome
{
public:
  SkyDome(nvvk::Context* ctx, nvvk::ResourceAllocator* allocator);
  void setup(const VkDevice& device, nvvk::ResourceAllocator* allocator);
  void setOutImage(const VkDescriptorImageInfo& outimage);
  void draw(const VkCommandBuffer& cmd, const glm::mat4& view, const glm::mat4& proj, const VkExtent2D& size);
  void destroy();
  void updateParameterBuffer(VkCommandBuffer cmd) const;
  bool onUI();

  nvvkhl_shaders::Light getSun() const;
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
