/*
 * Copyright (c) 2019-2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2024, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */


#include "debug_util_vk.hpp"

namespace nvvk {

bool DebugUtil::s_enabled = false;

// Local extension functions point
PFN_vkCmdBeginDebugUtilsLabelEXT  DebugUtil::s_vkCmdBeginDebugUtilsLabelEXT  = 0;
PFN_vkCmdEndDebugUtilsLabelEXT    DebugUtil::s_vkCmdEndDebugUtilsLabelEXT    = 0;
PFN_vkCmdInsertDebugUtilsLabelEXT DebugUtil::s_vkCmdInsertDebugUtilsLabelEXT = 0;
PFN_vkSetDebugUtilsObjectNameEXT  DebugUtil::s_vkSetDebugUtilsObjectNameEXT  = 0;


void DebugUtil::setup(VkDevice device)
{
  m_device = device;
  // Get the function pointers
  if(s_enabled == false)
  {
    s_vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
    s_vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
    s_vkCmdInsertDebugUtilsLabelEXT =
        (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdInsertDebugUtilsLabelEXT");
    s_vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");

    s_enabled = s_vkCmdBeginDebugUtilsLabelEXT != nullptr && s_vkCmdEndDebugUtilsLabelEXT != nullptr
                && s_vkCmdInsertDebugUtilsLabelEXT != nullptr && s_vkSetDebugUtilsObjectNameEXT != nullptr;
  }
}

void DebugUtil::setObjectName(const uint64_t object, const std::string& name, VkObjectType t) const
{
  if(s_enabled)
  {
    VkDebugUtilsObjectNameInfoEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, t, object, name.c_str()};
    s_vkSetDebugUtilsObjectNameEXT(m_device, &s);
  }
}

void DebugUtil::beginLabel(VkCommandBuffer cmdBuf, const std::string& label)
{
  if(s_enabled)
  {
    VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f}};
    s_vkCmdBeginDebugUtilsLabelEXT(cmdBuf, &s);
  }
}

void DebugUtil::endLabel(VkCommandBuffer cmdBuf)
{
  if(s_enabled)
  {
    s_vkCmdEndDebugUtilsLabelEXT(cmdBuf);
  }
}

void DebugUtil::insertLabel(VkCommandBuffer cmdBuf, const std::string& label)
{
  if(s_enabled)
  {
    VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f}};
    s_vkCmdInsertDebugUtilsLabelEXT(cmdBuf, &s);
  }
}

}  // namespace nvvk
