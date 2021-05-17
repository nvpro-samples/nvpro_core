/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2018-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#include "renderpasses_vk.hpp"
#include "error_vk.hpp"
#include <assert.h>

namespace nvvk {

VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
  for(VkFormat format : candidates)
  {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

    if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
    {
      return format;
    }

    if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
    {
      return format;
    }
  }

  assert(0 && "failed to find supported format!");

  return VK_FORMAT_UNDEFINED;
}

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice)
{
  return findSupportedFormat(physicalDevice,
                             {VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT,
                              VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT},
                             VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat findDepthStencilFormat(VkPhysicalDevice physicalDevice)
{
  return findSupportedFormat(physicalDevice, {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT},
                             VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

//////////////////////////////////////////////////////////////////////////

VkRenderPass createRenderPass(VkDevice                     device,
                              const std::vector<VkFormat>& colorAttachmentFormats,
                              VkFormat                     depthAttachmentFormat,
                              uint32_t                     subpassCount /*= 1*/,
                              bool                         clearColor /*= true*/,
                              bool                         clearDepth /*= true*/,
                              VkImageLayout                initialLayout /*= VK_IMAGE_LAYOUT_UNDEFINED*/,
                              VkImageLayout                finalLayout /*= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR*/)
{

  std::vector<VkAttachmentDescription> allAttachments;
  std::vector<VkAttachmentReference>   colorAttachmentRefs;

  bool hasDepth = (depthAttachmentFormat != VK_FORMAT_UNDEFINED);

  for(const auto& format : colorAttachmentFormats)
  {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format                  = format;
    colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp                  = clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR :
                                                           ((initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) ? VK_ATTACHMENT_LOAD_OP_DONT_CARE :
                                                                                                           VK_ATTACHMENT_LOAD_OP_LOAD);
    colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout           = initialLayout;
    colorAttachment.finalLayout             = finalLayout;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment            = static_cast<uint32_t>(allAttachments.size());
    colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    allAttachments.push_back(colorAttachment);
    colorAttachmentRefs.push_back(colorAttachmentRef);
  }

  VkAttachmentReference depthAttachmentRef = {};
  if(hasDepth)
  {
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format                  = depthAttachmentFormat;
    depthAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp                  = clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;

    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    depthAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    allAttachments.push_back(depthAttachment);
  }

  std::vector<VkSubpassDescription> subpasses;
  std::vector<VkSubpassDependency>  subpassDependencies;

  for(uint32_t i = 0; i < subpassCount; i++)
  {
    VkSubpassDescription subpass    = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpass.pColorAttachments       = colorAttachmentRefs.data();
    subpass.pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : VK_NULL_HANDLE;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass          = i == 0 ? (VK_SUBPASS_EXTERNAL) : (i - 1);
    dependency.dstSubpass          = i;
    dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask       = 0;
    dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    subpasses.push_back(subpass);
    subpassDependencies.push_back(dependency);
  }

  VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassInfo.attachmentCount = static_cast<uint32_t>(allAttachments.size());
  renderPassInfo.pAttachments    = allAttachments.data();
  renderPassInfo.subpassCount    = static_cast<uint32_t>(subpasses.size());
  renderPassInfo.pSubpasses      = subpasses.data();
  renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
  renderPassInfo.pDependencies   = subpassDependencies.data();
  VkRenderPass renderPass;
  NVVK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
  return renderPass;
}

}  // namespace nvvk
