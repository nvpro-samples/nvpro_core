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

namespace util {
//--------------------------------------------------------------------------------------------------
/**
Simple creation of render passes
Example of usage:
~~~~~~ C++
m_renderPass   = nvvkpp::util::createRenderPass(m_device, {m_swapChain.colorFormat}, m_depthFormat, 1, true, true);
m_renderPassUI = nvvkpp::util::createRenderPass(m_device, {m_swapChain.colorFormat}, m_depthFormat, 1, false, false);
~~~~~~ 
*/
inline vk::RenderPass createRenderPass(const vk::Device&              device,
                                       const std::vector<vk::Format>& colorAttachmentFormats,
                                       vk::Format                     depthAttachmentFormat,
                                       uint32_t                       subpassCount  = 1,
                                       bool                           clearColor    = true,
                                       bool                           clearDepth    = true,
                                       vk::ImageLayout                initialLayout = vk::ImageLayout::eUndefined,
                                       vk::ImageLayout                finalLayout   = vk::ImageLayout::ePresentSrcKHR)
{
  std::vector<vk::AttachmentDescription> allAttachments;
  std::vector<vk::AttachmentReference>   colorAttachmentRefs;

  bool hasDepth = (depthAttachmentFormat != vk::Format::eUndefined);

  for(const auto& format : colorAttachmentFormats)
  {
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format        = format;
    colorAttachment.loadOp        = clearColor ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
    colorAttachment.initialLayout = initialLayout;
    colorAttachment.finalLayout   = finalLayout;

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
    colorAttachmentRef.layout     = vk::ImageLayout::eColorAttachmentOptimal;

    allAttachments.push_back(colorAttachment);
    colorAttachmentRefs.push_back(colorAttachmentRef);
  }


  vk::AttachmentReference depthAttachmentRef;
  if(hasDepth)
  {
    vk::AttachmentDescription depthAttachment;
    depthAttachment.format        = depthAttachmentFormat;
    depthAttachment.loadOp        = clearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
    depthAttachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachment.finalLayout   = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    depthAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
    depthAttachmentRef.layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    allAttachments.push_back(depthAttachment);
  }

  std::vector<vk::SubpassDescription> subpasses;
  std::vector<vk::SubpassDependency>  subpassDependencies;

  for(uint32_t i = 0; i < subpassCount; i++)
  {
    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount    = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpass.pColorAttachments       = colorAttachmentRefs.data();
    subpass.pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : VK_NULL_HANDLE;

    vk::SubpassDependency dependency = {};
    dependency.srcSubpass            = i == 0 ? (VK_SUBPASS_EXTERNAL) : (i - 1);
    dependency.dstSubpass            = i;
    dependency.srcStageMask          = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask          = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask         = vk::AccessFlags();
    dependency.dstAccessMask         = vk::AccessFlagBits::eColorAttachmentWrite;
    ;

    subpasses.push_back(subpass);
    subpassDependencies.push_back(dependency);
  }

  vk::RenderPassCreateInfo renderPassInfo;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(allAttachments.size());
  renderPassInfo.pAttachments    = allAttachments.data();
  renderPassInfo.subpassCount    = static_cast<uint32_t>(subpasses.size());
  renderPassInfo.pSubpasses      = subpasses.data();
  renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
  renderPassInfo.pDependencies   = subpassDependencies.data();
  return device.createRenderPass(renderPassInfo);
}


}  // namespace util
}  // namespace nvvk