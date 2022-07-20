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

#include <platform.h>

#include <vector>
#include <vulkan/vulkan_core.h>

namespace nvvk {

//////////////////////////////////////////////////////////////////////////
/**
  The utilities in this file provide a more direct approach, we encourage to use
  higher-level mechanisms also provided in the allocator / memorymanagement classes.

  # functions in nvvk

  - makeBufferCreateInfo : wraps setup of VkBufferCreateInfo (implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT)
  - makeBufferViewCreateInfo : wraps setup of VkBufferViewCreateInfo
  - createBuffer : wraps vkCreateBuffer
  - createBufferView : wraps vkCreateBufferView
  - getBufferDeviceAddressKHR : wraps vkGetBufferDeviceAddressKHR
  - getBufferDeviceAddress : wraps vkGetBufferDeviceAddress

  \code{.cpp}
  VkBufferCreateInfo bufferCreate = makeBufferCreateInfo (size, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
  VkBuffer buffer                 = createBuffer(device, bufferCreate);
  VkBufferView bufferView         = createBufferView(device, makeBufferViewCreateInfo(buffer, VK_FORMAT_R8G8B8A8_UNORM, size));
  \endcode
*/

// implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
inline VkBufferCreateInfo makeBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkBufferCreateFlags flags = 0)
{
  VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  createInfo.size               = size;
  createInfo.usage              = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  createInfo.flags              = flags;

  return createInfo;
}

inline VkBufferViewCreateInfo makeBufferViewCreateInfo(VkBuffer                buffer,
                                                       VkFormat                format,
                                                       VkDeviceSize            range,
                                                       VkDeviceSize            offset = 0,
                                                       VkBufferViewCreateFlags flags  = 0)
{
  VkBufferViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
  createInfo.buffer                 = buffer;
  createInfo.offset                 = offset;
  createInfo.range                  = range;
  createInfo.flags                  = flags;
  createInfo.format                 = format;

  return createInfo;
}

inline VkBufferViewCreateInfo makeBufferViewCreateInfo(const VkDescriptorBufferInfo& descrInfo,
                                                       VkFormat                      fmt,
                                                       VkBufferViewCreateFlags       flags = 0)
{
  VkBufferViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
  createInfo.buffer                 = descrInfo.buffer;
  createInfo.offset                 = descrInfo.offset;
  createInfo.range                  = descrInfo.range;
  createInfo.flags                  = flags;
  createInfo.format                 = fmt;

  return createInfo;
}


inline VkDeviceAddress getBufferDeviceAddressKHR(VkDevice device, VkBuffer buffer)
{
  if(buffer == VK_NULL_HANDLE)
    return 0ULL;

  VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
  info.buffer                    = buffer;
  return vkGetBufferDeviceAddressKHR(device, &info);
}

inline VkDeviceAddress getBufferDeviceAddress(VkDevice device, VkBuffer buffer)
{
  if(buffer == VK_NULL_HANDLE)
    return 0ULL;

  VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  info.buffer                    = buffer;
  return vkGetBufferDeviceAddress(device, &info);
}

//////////////////////////////////////////////////////////////////////////
// these use pass by value so one can easily chain createBuffer(device, makeBufferCreateInfo(...));

inline VkBuffer createBuffer(VkDevice device, VkBufferCreateInfo info)
{
  VkBuffer buffer;
  VkResult result = vkCreateBuffer(device, &info, nullptr, &buffer);
  assert(result == VK_SUCCESS);
  return buffer;
}

inline VkBufferView createBufferView(VkDevice device, VkBufferViewCreateInfo info)
{
  VkBufferView bufferView;
  VkResult     result = vkCreateBufferView(device, &info, nullptr, &bufferView);
  assert(result == VK_SUCCESS);
  return bufferView;
}

}  // namespace nvvk
