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

#include <vulkan/vulkan.hpp>

namespace nvvkpp {

//--------------------------------------------------------------------------------------------------
/** 
Most graphic pipeline are similar, therefor the helper `GraphicsPipelineGenerator` holds all the elements and initialize the structure with the proper default, such as `eTriangleList`, `PipelineColorBlendAttachmentState` with their mask, `DynamicState` for viewport and scissor, adjust depth test if enable, line width to 1, for example. As those are all C++, setters exist for any members and deep copy for any attachments.

Example of usage :
~~~~ c++
nvvk::GraphicsPipelineGenerator pipelineGenerator(m_device, m_pipelineLayout, m_renderPass);
pipelineGenerator.loadShader(readFile("shaders/vert_shader.vert.spv"), vk::ShaderStageFlagBits::eVertex);
pipelineGenerator.loadShader(readFile("shaders/frag_shader.frag.spv"), vk::ShaderStageFlagBits::eFragment);
pipelineGenerator.depthStencilState = {true};
pipelineGenerator.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
pipelineGenerator.vertexInputState.bindingDescriptions   = {{0, sizeof(Vertex)}};
pipelineGenerator.vertexInputState.attributeDescriptions = {
    {0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
    {1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nrm))},
    {2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, col))}};
m_pipeline = pipelineGenerator.create();
~~~~
*/

struct GraphicsPipelineGenerator
{
public:
  GraphicsPipelineGenerator(const vk::Device& device, const vk::PipelineLayout& layout, const vk::RenderPass& renderPass)
      : device(device)
  {
    pipelineCreateInfo.layout     = layout;
    pipelineCreateInfo.renderPass = renderPass;
    init();
  }

  GraphicsPipelineGenerator(const GraphicsPipelineGenerator& other)
      : GraphicsPipelineGenerator(other.device, other.layout, other.renderPass)
  {
  }

  GraphicsPipelineGenerator& operator=(const GraphicsPipelineGenerator& other) = delete;
  ~GraphicsPipelineGenerator() { destroyShaderModules(); }


  vk::PipelineShaderStageCreateInfo& addShader(const std::string&      code,
                                               vk::ShaderStageFlagBits stage,
                                               const char*             entryPoint = "main")
  {
    std::vector<char> v;
    std::copy(code.begin(), code.end(), std::back_inserter(v));
    return addShader(v, stage, entryPoint);
  }

  template <typename T>
  vk::PipelineShaderStageCreateInfo& addShader(const std::vector<T>&   code,
                                               vk::ShaderStageFlagBits stage,
                                               const char*             entryPoint = "main");

  vk::Pipeline create(const vk::PipelineCache& cache)
  {
    update();
    return device.createGraphicsPipeline(cache, pipelineCreateInfo);
  }

  vk::Pipeline create() { return create(pipelineCache); }

  void destroyShaderModules()
  {
    for(const auto& shaderStage : shaderStages)
    {
      device.destroyShaderModule(shaderStage.module);
    }
    shaderStages.clear();
  }

  // Overriding the defaults
  struct PipelineRasterizationStateCreateInfo : public vk::PipelineRasterizationStateCreateInfo
  {
    PipelineRasterizationStateCreateInfo()
    {
      lineWidth = 1.0f;
      cullMode  = vk::CullModeFlagBits::eBack;
    }
  };

  struct PipelineInputAssemblyStateCreateInfo : public vk::PipelineInputAssemblyStateCreateInfo
  {
    PipelineInputAssemblyStateCreateInfo() { topology = vk::PrimitiveTopology::eTriangleList; }
  };

  struct PipelineColorBlendAttachmentState : public vk::PipelineColorBlendAttachmentState
  {
    PipelineColorBlendAttachmentState(vk::Bool32              blendEnable_         = 0,
                                      vk::BlendFactor         srcColorBlendFactor_ = vk::BlendFactor::eZero,
                                      vk::BlendFactor         dstColorBlendFactor_ = vk::BlendFactor::eZero,
                                      vk::BlendOp             colorBlendOp_        = vk::BlendOp::eAdd,
                                      vk::BlendFactor         srcAlphaBlendFactor_ = vk::BlendFactor::eZero,
                                      vk::BlendFactor         dstAlphaBlendFactor_ = vk::BlendFactor::eZero,
                                      vk::BlendOp             alphaBlendOp_        = vk::BlendOp::eAdd,
                                      vk::ColorComponentFlags colorWriteMask_      = vk::ColorComponentFlagBits::eR
                                                                                | vk::ColorComponentFlagBits::eG
                                                                                | vk::ColorComponentFlagBits::eB
                                                                                | vk::ColorComponentFlagBits::eA)
        : vk::PipelineColorBlendAttachmentState(blendEnable_,
                                                srcColorBlendFactor_,
                                                dstColorBlendFactor_,
                                                colorBlendOp_,
                                                srcAlphaBlendFactor_,
                                                dstAlphaBlendFactor_,
                                                alphaBlendOp_,
                                                colorWriteMask_)
    {
    }
  };

  struct PipelineColorBlendStateCreateInfo : public vk::PipelineColorBlendStateCreateInfo
  {
    std::vector<PipelineColorBlendAttachmentState> blendAttachmentStates{PipelineColorBlendAttachmentState()};

    void update()
    {
      this->attachmentCount = (uint32_t)blendAttachmentStates.size();
      this->pAttachments    = blendAttachmentStates.data();
    }
  };

  struct PipelineDynamicStateCreateInfo : public vk::PipelineDynamicStateCreateInfo
  {
    std::vector<vk::DynamicState> dynamicStateEnables;
    PipelineDynamicStateCreateInfo()
    {
      dynamicStateEnables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    }
    void update()
    {
      this->dynamicStateCount = (uint32_t)dynamicStateEnables.size();
      this->pDynamicStates    = dynamicStateEnables.data();
    }
  };

  struct PipelineVertexInputStateCreateInfo : public vk::PipelineVertexInputStateCreateInfo
  {
    std::vector<vk::VertexInputBindingDescription>   bindingDescriptions;
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
    void                                             update()
    {
      vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
      vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size());
      pVertexBindingDescriptions      = bindingDescriptions.data();
      pVertexAttributeDescriptions    = attributeDescriptions.data();
    }
  };

  struct PipelineViewportStateCreateInfo : public vk::PipelineViewportStateCreateInfo
  {
    std::vector<vk::Viewport> viewports;
    std::vector<vk::Rect2D>   scissors;
    void                      update()
    {
      if(viewports.empty())
      {
        viewportCount = 1;
        pViewports    = nullptr;
      }
      else
      {
        viewportCount = (uint32_t)viewports.size();
        pViewports    = viewports.data();
      }

      if(scissors.empty())
      {
        scissorCount = 1;
        pScissors    = nullptr;
      }
      else
      {
        scissorCount = (uint32_t)scissors.size();
        pScissors    = scissors.data();
      }
    }
  };

  struct PipelineDepthStencilStateCreateInfo : public vk::PipelineDepthStencilStateCreateInfo
  {
    PipelineDepthStencilStateCreateInfo(bool depthEnable = true)
    {
      if(depthEnable)
      {
        depthTestEnable  = VK_TRUE;
        depthWriteEnable = VK_TRUE;
        depthCompareOp   = vk::CompareOp::eLessOrEqual;
      }
    }
  };

  const vk::Device&                              device;
  vk::GraphicsPipelineCreateInfo                 pipelineCreateInfo;
  vk::PipelineCache                              pipelineCache;
  vk::RenderPass&                                renderPass{pipelineCreateInfo.renderPass};
  vk::PipelineLayout&                            layout{pipelineCreateInfo.layout};
  uint32_t&                                      subpass{pipelineCreateInfo.subpass};
  PipelineInputAssemblyStateCreateInfo           inputAssemblyState;
  PipelineRasterizationStateCreateInfo           rasterizationState;
  vk::PipelineMultisampleStateCreateInfo         multisampleState;
  PipelineDepthStencilStateCreateInfo            depthStencilState;
  PipelineViewportStateCreateInfo                viewportState;
  PipelineDynamicStateCreateInfo                 dynamicState;
  PipelineColorBlendStateCreateInfo              colorBlendState;
  PipelineVertexInputStateCreateInfo             vertexInputState;
  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

  void update()
  {
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages    = shaderStages.data();
    dynamicState.update();
    colorBlendState.update();
    vertexInputState.update();
    viewportState.update();
  }

private:
  void init()
  {
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pColorBlendState    = &colorBlendState;
    pipelineCreateInfo.pMultisampleState   = &multisampleState;
    pipelineCreateInfo.pViewportState      = &viewportState;
    pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
    pipelineCreateInfo.pDynamicState       = &dynamicState;
    pipelineCreateInfo.pVertexInputState   = &vertexInputState;
  }
};

template <typename T>
vk::PipelineShaderStageCreateInfo& nvvkpp::GraphicsPipelineGenerator::addShader(const std::vector<T>&   code,
                                                                                vk::ShaderStageFlagBits stage,
                                                                                const char*             entryPoint)
{
  vk::ShaderModuleCreateInfo createInfo;
  createInfo.codeSize           = sizeof(T) * code.size();
  createInfo.pCode              = reinterpret_cast<const uint32_t*>(code.data());
  vk::ShaderModule shaderModule = device.createShaderModule(createInfo, nullptr);

  vk::PipelineShaderStageCreateInfo shaderStage;
  shaderStage.stage  = stage;
  shaderStage.module = shaderModule;
  shaderStage.pName  = entryPoint;

  shaderStages.push_back(shaderStage);
  return shaderStages.back();
}

}  // namespace nvvkpp
