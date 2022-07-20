/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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
#include <array>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "nvmath/nvmath.h"
#include "nvvk/pipeline_vk.hpp"  // Using the Pipeline Generator Utility


namespace nvvk {

//--------------------------------------------------------------------------------------------------
/**
 \class nvvk::Axis

 nvvk::Axis displays an Axis representing the orientation of the camera in the bottom left corner of the window.
 - Initialize the Axis using `init()`
 - Add `display()` in a inline rendering pass, one of the lass command
 
 Example:  
 \code{.cpp}
 m_axis.display(cmdBuf, CameraManip.getMatrix(), windowSize);
 \endcode 
*/

class AxisVK
{
public:
  struct CreateAxisInfo
  {
    VkRenderPass          renderPass{VK_NULL_HANDLE};
    uint32_t              subpass{0};
    std::vector<VkFormat> colorFormat;
    VkFormat              depthFormat{};
    VkFormat              stencilFormat{};
    float                 axisSize{50.f};
  };


  void init(VkDevice device, VkRenderPass renderPass, uint32_t subpass = 0, float axisSize = 50.f)
  {
    m_device   = device;
    m_axisSize = axisSize;

    CreateAxisInfo info;
    info.renderPass = renderPass;
    info.subpass    = subpass;
    createAxisObject(info);
  }

  void init(VkDevice device, CreateAxisInfo info)
  {
    m_device   = device;
    m_axisSize = info.axisSize;
    createAxisObject(info);
  }

  void deinit()
  {
    vkDestroyPipeline(m_device, m_pipelineTriangleFan, nullptr);
    vkDestroyPipeline(m_device, m_pipelineLines, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
  }

  void display(VkCommandBuffer cmdBuf, const nvmath::mat4f& transform, const VkExtent2D& screenSize);

  void setAxisSize(float s) { m_axisSize = s; }

private:
  void createAxisObject(CreateAxisInfo& info);

  VkPipeline       m_pipelineTriangleFan = {};
  VkPipeline       m_pipelineLines       = {};
  VkPipelineLayout m_pipelineLayout      = {};
  float            m_axisSize            = 50.f;  // Size in pixel

  VkDevice m_device{VK_NULL_HANDLE};
};

}  // namespace nvvk
