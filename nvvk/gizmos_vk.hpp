/* Copyright (c) 2014-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
 # class nvvk::Axis

 Display an Axis representing the orientation of the camera in the bottom left corner of the window.
 - Initialize the Axis using `init()`
 - Add `display()` in a inline rendering pass, one of the lass command
 
 Example:  
 ~~~~~~ C++
 m_axis.display(cmdBuf, CameraManip.getMatrix(), windowSize);
 ~~~~~~ 
*/

class AxisVK
{
public:
  void init(VkDevice device, VkRenderPass renderPass, uint32_t subpass = 0, float axisSize = 40.f)
  {
    m_device   = device;
    m_axisSize = axisSize;
    createAxisObject(renderPass, subpass);
  }

  void deinit()
  {
    vkDestroyPipeline(m_device, m_pipelineTriangleFan, nullptr);
    vkDestroyPipeline(m_device, m_pipelineLines, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
  }

  void display(VkCommandBuffer cmdBuf, const nvmath::mat4f& transform, const VkExtent2D& screenSize)
  {
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineTriangleFan);

    // Setup viewport:
    VkViewport viewport{};
    viewport.width    = float(screenSize.width);
    viewport.height   = float(screenSize.height);
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    VkRect2D rect;
    rect.offset = VkOffset2D{0, 0};
    rect.extent = VkExtent2D{screenSize.width, screenSize.height};
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf, 0, 1, &rect);


    // Set the orthographic matrix in the bottom left corner
    {
      const float         pixelW   = m_axisSize / screenSize.width;
      const float         pixelH   = m_axisSize / screenSize.height;
      const nvmath::mat4f matOrtho = {pixelW * .8f,  0.0f,          0.0f,  0.0f,  //
                                      0.0f,          -pixelH * .8f, 0.0f,  0.0f,  //
                                      0.0f,          0.0f,          -0.1f, 0.0f,  //
                                      -1.f + pixelW, 1.f - pixelH,  0.5f,  1.0f};

      nvmath::mat4f modelView = transform;
      modelView.set_translate({0, 0, 0});
      modelView = matOrtho * modelView;
      // Push the matrix to the shader
      vkCmdPushConstants(cmdBuf, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(nvmath::mat4f), &modelView.a00);
    }

    // Draw 3 times the tip of the arrow, the shader is flipping the orientation and setting the color
    vkCmdDraw(cmdBuf, 6, 3, 0, 0);
    // Now draw the line of the arrow using the last 2 vertex of the buffer (offset 5)
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLines);
    vkCmdDraw(cmdBuf, 2, 3, 5, 0);
  }

private:
  void createAxisObject(VkRenderPass renderPass, uint32_t subpass);

  VkPipeline       m_pipelineTriangleFan = {};
  VkPipeline       m_pipelineLines       = {};
  VkPipelineLayout m_pipelineLayout      = {};
  float            m_axisSize            = 40.f;  // Size in pixel

  VkDevice m_device{VK_NULL_HANDLE};
};

}  // namespace nvvk
