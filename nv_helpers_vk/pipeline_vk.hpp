/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
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

#include <platform.h>
#include <assert.h>
#include <algorithm>
#include <vulkan/vulkan.h>

namespace nv_helpers_vk {

  class GraphicsPipelineState
  {
  public:
    explicit GraphicsPipelineState(VkPipelineLayout layout, VkPipelineCreateFlags flags = 0)
    {
      createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
      createInfo.flags = flags;
      createInfo.pStages = stages;
      createInfo.pVertexInputState = &viState;
      createInfo.pInputAssemblyState = &iaState;
      createInfo.pViewportState = &vpState;
      createInfo.pRasterizationState = &rsState;
      createInfo.pMultisampleState = &msState;
      createInfo.pDepthStencilState = &dsState;
      createInfo.pColorBlendState = &cbState;
      createInfo.layout = layout;

      viState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

      iaState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
      iaState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

      vpState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
      vpState.viewportCount = 1;
      vpState.pViewports = viewports;
      vpState.scissorCount = 1;
      vpState.pScissors = scissors;

      rsState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

      msState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
      msState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

      dsState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
      dsState.depthCompareOp = VK_COMPARE_OP_ALWAYS;

      cbState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
      cbState.attachmentCount = 1;
      cbState.pAttachments = attachments;

      dyState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };

      for (size_t i = 0; i < NV_ARRAY_SIZE(stages); ++i)
        stages[i] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

      for (size_t i = 0; i < NV_ARRAY_SIZE(attachments); ++i)
        attachments[i] = { VK_FALSE,
        VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };

      for (size_t i = 0; i < NV_ARRAY_SIZE(scissors); ++i)
        scissors[i] = { { 0, 0 }, { 800, 600 } };

      for (size_t i = 0; i < NV_ARRAY_SIZE(viewports); ++i)
        viewports[i] = { 0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f };

      for (size_t i = 0; i < NV_ARRAY_SIZE(dynamicStates); i++)
        dynamicStates[i] = VK_DYNAMIC_STATE_MAX_ENUM;
    }

    GraphicsPipelineState(GraphicsPipelineState &&) = delete;
    GraphicsPipelineState(const GraphicsPipelineState &) = delete;
    void operator=(GraphicsPipelineState &&) = delete;
    void operator=(const GraphicsPipelineState &) = delete;

    void setRenderPass(VkRenderPass pass) { createInfo.renderPass = pass; }

    void addShaderStage(VkShaderStageFlagBits stage, VkShaderModule module, const char *entrypoint = "main")
    {
      assert(createInfo.stageCount < NV_ARRAY_SIZE(stages));
      stages[createInfo.stageCount].stage = stage;
      stages[createInfo.stageCount].module = module;
      stages[createInfo.stageCount].pName = entrypoint;
      createInfo.stageCount++;
    }
    void addDynamicState(VkDynamicState state)
    {
      dynamicStates[dyState.dynamicStateCount] = state;
      dyState.dynamicStateCount++;
      dyState.pDynamicStates = dynamicStates;
      createInfo.pDynamicState = &dyState;

      switch (state)
      {
      case VK_DYNAMIC_STATE_VIEWPORT:
        vpState.pViewports = nullptr;
        break;
      case VK_DYNAMIC_STATE_SCISSOR:
        vpState.pScissors = nullptr;
        break;
      }
    }

    void setVertexInputBindings(uint32_t num_bindings, const VkVertexInputBindingDescription *bindings)
    {
      viState.vertexBindingDescriptionCount = num_bindings;
      viState.pVertexBindingDescriptions = bindings;
    }
    void setVertexInputAttributes(uint32_t num_attributes, const VkVertexInputAttributeDescription *attributes)
    {
      viState.vertexAttributeDescriptionCount = num_attributes;
      viState.pVertexAttributeDescriptions = attributes;
    }
    void setPrimitiveTopology(VkPrimitiveTopology topology) { iaState.topology = topology; }
    void setCullMode(VkCullModeFlags mode, VkFrontFace front)
    {
      rsState.cullMode = mode;
      rsState.frontFace = front;
    }
    void setPolygonMode(VkPolygonMode mode) { rsState.polygonMode = mode; }
    void setAttachmentColorMask(uint32_t attachment, VkColorComponentFlags mask)
    {
      assert(attachment < NV_ARRAY_SIZE(attachments));
      attachments[attachment].colorWriteMask = mask;
      cbState.attachmentCount = std::max(cbState.attachmentCount, attachment + 1);
    }
    void setDepthTest(bool enable, bool write, VkCompareOp op = VK_COMPARE_OP_ALWAYS)
    {
      dsState.depthTestEnable = enable;
      dsState.depthWriteEnable = write;
      dsState.depthCompareOp = op;
    }
    void setDepthBounds(bool enable, float min = 0.0f, float max = 1.0f)
    {
      dsState.depthBoundsTestEnable = enable;
      dsState.minDepthBounds = min;
      dsState.maxDepthBounds = max;
    }
    void setStencilTest(bool enable, VkCompareOp op = VK_COMPARE_OP_ALWAYS)
    {
      dsState.stencilTestEnable = enable;
      dsState.back.compareOp = op;
      dsState.front.compareOp = op;
    }

    void setScissorRect(uint32_t index, const VkRect2D &rect)
    {
      assert(index < NV_ARRAY_SIZE(scissors));
      scissors[index] = rect;
      vpState.scissorCount = std::max(vpState.scissorCount, index + 1);
    }
    void setViewportRect(uint32_t index, const VkViewport &viewport)
    {
      assert(index < NV_ARRAY_SIZE(viewports));
      viewports[index] = viewport;
      vpState.viewportCount = std::max(vpState.viewportCount, index + 1);
    }

    VkGraphicsPipelineCreateInfo createInfo;

  public:
    VkPipelineVertexInputStateCreateInfo viState;
    VkPipelineInputAssemblyStateCreateInfo iaState;
    VkPipelineViewportStateCreateInfo vpState;
    VkPipelineRasterizationStateCreateInfo rsState;
    VkPipelineMultisampleStateCreateInfo msState;
    VkPipelineDepthStencilStateCreateInfo dsState;
    VkPipelineColorBlendStateCreateInfo cbState;
    VkPipelineDynamicStateCreateInfo dyState;

    VkPipelineShaderStageCreateInfo stages[5];
    VkPipelineColorBlendAttachmentState attachments[5];
    VkRect2D scissors[5];
    VkViewport viewports[5];
    VkDynamicState dynamicStates[5];
  };

}
