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
//////////////////////////////////////////////////////////////////////////

#include <array>
#include <vector>

#include "nvmath/nvmath.h"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/images_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"


namespace nvvkhl {
class Context;


//--------------------------------------------------------------------------------------------------
//
// Use an environment image (HDR) and create the cubic textures for glossy reflection and diffuse illumination.
// It also has the ability to render the HDR environment, in the background of an image.
//
// Using 4 compute shaders
// - hdr_dome: to make the HDR as background
// - hdr_integrate_brdf     : generate the BRDF lookup table
// - hdr_prefilter_diffuse  : integrate the diffuse contribution in a cubemap
// - hdr_prefilter_glossy   : integrate the glossy reflection in a cubemap
//
class HdrEnvDome
{
public:
  HdrEnvDome() = default;
  HdrEnvDome(nvvk::Context* ctx, nvvk::ResourceAllocator* allocator, uint32_t queueFamilyIndex = 0U);
  ~HdrEnvDome() { destroy(); }

  void setup(const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t familyIndex, nvvk::ResourceAllocator* allocator);
  void create(VkDescriptorSet dstSet, VkDescriptorSetLayout dstSetLayout);
  void setOutImage(const VkDescriptorImageInfo& outimage);
  void draw(const VkCommandBuffer& cmdBuf,
            const nvmath::mat4f&   view,
            const nvmath::mat4f&   proj,
            const VkExtent2D&      size,
            const float*           color,
            float                  rotation = 0.F);
  void destroy();

  inline VkDescriptorSetLayout getDescLayout() const { return m_hdrLayout; }
  inline VkDescriptorSet       getDescSet() const { return m_hdrSet; }


private:
  // Resources
  VkDevice                 m_device{VK_NULL_HANDLE};
  uint32_t                 m_familyIndex{0};
  nvvk::ResourceAllocator* m_alloc{nullptr};
  nvvk::DebugUtil          m_debug;

  // From HdrEnv
  VkDescriptorSet       m_hdrEnvSet{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_hdrEnvLayout{VK_NULL_HANDLE};

  // To draw the HDR in image
  VkDescriptorSet       m_domeSet{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_domeLayout{VK_NULL_HANDLE};
  VkDescriptorPool      m_domePool{VK_NULL_HANDLE};
  VkPipeline            m_domePipeline{VK_NULL_HANDLE};
  VkPipelineLayout      m_domePipelineLayout{VK_NULL_HANDLE};


  VkDescriptorSet       m_hdrSet{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_hdrLayout{VK_NULL_HANDLE};
  VkDescriptorPool      m_hdrPool{VK_NULL_HANDLE};

  struct Textures
  {
    nvvk::Texture lutBrdf;
    nvvk::Texture diffuse;
    nvvk::Texture glossy;
  } m_textures;


  void createDescriptorSetLayout();
  void createDrawPipeline();
  void integrateBrdf(uint32_t dimension, nvvk::Texture& target);
  void prefilterHdr(uint32_t dim, nvvk::Texture& target, const VkShaderModule& module, bool doMipmap);
  void renderToCube(const VkCommandBuffer& cmdBuf,
                    nvvk::Texture&         target,
                    nvvk::Texture&         scratch,
                    VkPipelineLayout       pipelineLayout,
                    uint32_t               dim,
                    uint32_t               numMips);
};

}  // namespace nvvkhl
