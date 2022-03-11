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

#include "dynamicrendering_vk.hpp"


namespace nvvk {
#ifdef VK_KHR_dynamic_rendering
// Helper for VK_KHR_dynamic_rendering
createRenderingInfo::createRenderingInfo(VkRect2D                        renderArea,
                                         const std::vector<VkImageView>& colorViews,
                                         const VkImageView&              depthView,
                                         VkAttachmentLoadOp              colorLoadOp /*= VK_ATTACHMENT_LOAD_OP_CLEAR*/,
                                         VkAttachmentLoadOp              depthLoadOp /*= VK_ATTACHMENT_LOAD_OP_CLEAR*/,
                                         VkClearColorValue               clearColorValue /*= {0.f, 0.f, 0.f, 0.f}*/,
                                         VkClearDepthStencilValue        clearDepthValue /*= {1.f, 0U}*/,
                                         VkRenderingFlagsKHR             flags /*= 0*/)
    : VkRenderingInfoKHR{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR}
{
  for(auto& cv : colorViews)
  {
    VkRenderingAttachmentInfoKHR colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR};
    colorAttachment.clearValue.color = clearColorValue;
    colorAttachment.imageView        = cv;
    colorAttachment.imageLayout      = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    colorAttachment.loadOp           = colorLoadOp;
    colorAttachment.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachments.emplace_back(colorAttachment);
  }

  depthStencilAttachment.imageView               = depthView;
  depthStencilAttachment.imageLayout             = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  depthStencilAttachment.loadOp                  = depthLoadOp;
  depthStencilAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
  depthStencilAttachment.clearValue.depthStencil = clearDepthValue;

  this->renderArea           = renderArea;
  this->layerCount           = 1;
  this->colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
  this->pColorAttachments    = colorAttachments.data();
  this->pDepthAttachment     = &depthStencilAttachment;
  this->pStencilAttachment   = &depthStencilAttachment;
  this->flags                = flags;
}
#endif
}  // namespace nvvk