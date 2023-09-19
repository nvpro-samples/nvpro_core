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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <memory>
#include "shaders/dh_tonemap.h"
#include "nvvk/descriptorsets_vk.hpp"


/*

This class is meant to be use for displaying the final image rendered in linear space (sRGB).

There are two ways to use it, one which is graphic, the other is compute. 

- The graphic will render a full screen quad with the input image. It is to the 
  application to set the rendering target ( -> G-Buffer0 )

- The compute takes an image as input and write to an another one using a compute shader

- It is either one or the other, both rendering aren't needed to post-process. If both are provided
  it is for convenience. 

Note: It is important in any cases to place a barrier if there is a transition from 
      fragment to compute and compute to fragment to avoid missing results.

*/


// Forward declarations
namespace nvvk {
class Context;
class DebugUtil;
}  // namespace nvvk

namespace nvvkhl {
class AllocVma;


struct TonemapperPostProcess
{
  TonemapperPostProcess(nvvk::Context* ctx, AllocVma* alloc);
  ~TonemapperPostProcess();

  void createGraphicPipeline(VkFormat colorFormat, VkFormat depthFormat);
  void createComputePipeline();

  // It sets the in and out images used by the compute and graphic shaders.
  // The inImage, is the image you want to tonemap, the outImage is the image
  // in which the compute shader will be writing to. In graphics, you have to attach
  // a framebuffer, then the rendering will be done in the attached framebuffer image.
  void updateGraphicDescriptorSets(VkDescriptorImageInfo inImage);
  void updateComputeDescriptorSets(VkDescriptorImageInfo inImage, VkDescriptorImageInfo outImage);

  void runGraphic(VkCommandBuffer cmd);
  void runCompute(VkCommandBuffer cmd, const VkExtent2D& size);

  bool onUI();  // Display UI of the tonemapper

  void        setSettings(const Tonemapper& settings) { m_settings = settings; }
  Tonemapper& settings() { return m_settings; };  // returning access to setting values

private:
  nvvk::Context*                   m_ctx{nullptr};
  std::unique_ptr<nvvk::DebugUtil> m_dutil;


  std::unique_ptr<nvvk::DescriptorSetContainer> m_dsetGraphics;  // Holding the descriptor set information
  std::unique_ptr<nvvk::DescriptorSetContainer> m_dsetCompute;   // Holding the descriptor set information

  VkPipeline m_graphicsPipeline{VK_NULL_HANDLE};  // The graphic pipeline to render
  VkPipeline m_computePipeline{VK_NULL_HANDLE};   // The graphic pipeline to render
  Tonemapper m_settings;

  // To use VK_KHR_push_descriptor
  std::unique_ptr<VkDescriptorImageInfo> m_iimage;
  std::unique_ptr<VkDescriptorImageInfo> m_oimage;
  std::vector<VkWriteDescriptorSet>      m_writes;

  enum class TmMode
  {
    eNone,
    eGraphic,
    eCompute
  } m_mode{TmMode::eNone};
};

}  // namespace nvvkhl
