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


#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>
namespace nvvk {

#ifdef VK_KHR_dynamic_rendering
struct createRenderingInfo : public VkRenderingInfoKHR
{
  createRenderingInfo(VkRect2D                        renderArea,
                      const std::vector<VkImageView>& colorViews,
                      const VkImageView&              depthView,
                      VkAttachmentLoadOp              colorLoadOp     = VK_ATTACHMENT_LOAD_OP_CLEAR,
                      VkAttachmentLoadOp              depthLoadOp     = VK_ATTACHMENT_LOAD_OP_CLEAR,
                      VkClearColorValue               clearColorValue = {{0.f, 0.f, 0.f, 0.f}},
                      VkClearDepthStencilValue        clearDepthValue = {1.f, 0U},
                      VkRenderingFlagsKHR             flags           = 0);

  VkRenderingAttachmentInfoKHR              depthStencilAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR};
  std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
};
#endif

}  // namespace nvvk
