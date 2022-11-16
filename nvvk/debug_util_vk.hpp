/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


/// \class DebugUtil
/// This is a companion utility to add debug information to an application
/// See https://vulkan.lunarg.com/doc/sdk/1.1.114.0/windows/chunked_spec/chap39.html
/// - User defined name to objects
/// - Logically annotate region of command buffers
/// - Scoped command buffer label to make thing simpler

#pragma once

#include <algorithm>
#include <string.h>
#include <string>
#include <vulkan/vulkan_core.h>
#include "nvh/nvprint.hpp"

namespace nvvk {

class DebugUtil
{
public:
  DebugUtil() = default;
  DebugUtil(VkDevice device)
      : m_device(device)
  {
  }

  static void setEnabled(bool state) { s_enabled = state; }

  void setup(VkDevice device) { m_device = device; }

  void setObjectName(const uint64_t object, const std::string& name, VkObjectType t)
  {
    if(s_enabled)
    {
      VkDebugUtilsObjectNameInfoEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, t, object, name.c_str()};
      vkSetDebugUtilsObjectNameEXT(m_device, &s);
    }
  }

  // clang-format off
  void setObjectName(VkBuffer object, const std::string& name)                  { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_BUFFER); }
  void setObjectName(VkBufferView object, const std::string& name)              { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_BUFFER_VIEW); }
  void setObjectName(VkCommandBuffer object, const std::string& name)           { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_COMMAND_BUFFER ); }
  void setObjectName(VkCommandPool object, const std::string& name)             { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_COMMAND_POOL ); }
  void setObjectName(VkDescriptorPool object, const std::string& name)          { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_POOL); }
  void setObjectName(VkDescriptorSet object, const std::string& name)           { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET); }
  void setObjectName(VkDescriptorSetLayout object, const std::string& name)     { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT); }
  void setObjectName(VkDevice object, const std::string& name)                  { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DEVICE); }
  void setObjectName(VkDeviceMemory object, const std::string& name)            { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DEVICE_MEMORY); }
  void setObjectName(VkFramebuffer object, const std::string& name)             { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_FRAMEBUFFER); }
  void setObjectName(VkImage object, const std::string& name)                   { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_IMAGE); }
  void setObjectName(VkImageView object, const std::string& name)               { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_IMAGE_VIEW); }
  void setObjectName(VkPipeline object, const std::string& name)                { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_PIPELINE); }
  void setObjectName(VkPipelineLayout object, const std::string& name)          { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_PIPELINE_LAYOUT); }
  void setObjectName(VkQueryPool object, const std::string& name)               { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_QUERY_POOL); }
  void setObjectName(VkQueue object, const std::string& name)                   { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_QUEUE); }
  void setObjectName(VkRenderPass object, const std::string& name)              { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_RENDER_PASS); }
  void setObjectName(VkSampler object, const std::string& name)                 { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SAMPLER); }
  void setObjectName(VkSemaphore object, const std::string& name)               { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SEMAPHORE); }
  void setObjectName(VkShaderModule object, const std::string& name)            { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SHADER_MODULE); }
  void setObjectName(VkSwapchainKHR object, const std::string& name)            { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SWAPCHAIN_KHR); }

#if VK_NV_ray_tracing
  void setObjectName(VkAccelerationStructureNV object, const std::string& name)  { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV); }
#endif
#if VK_KHR_acceleration_structure
  void setObjectName(VkAccelerationStructureKHR object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR); }
#endif
  // clang-format on

  //
  //---------------------------------------------------------------------------
  //
  void beginLabel(VkCommandBuffer cmdBuf, const std::string& label)
  {
    if(s_enabled)
    {
      VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f}};
      vkCmdBeginDebugUtilsLabelEXT(cmdBuf, &s);
    }
  }
  void endLabel(VkCommandBuffer cmdBuf)
  {
    if(s_enabled)
    {
      vkCmdEndDebugUtilsLabelEXT(cmdBuf);
    }
  }
  void insertLabel(VkCommandBuffer cmdBuf, const std::string& label)
  {
    if(s_enabled)
    {
      VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f}};
      vkCmdInsertDebugUtilsLabelEXT(cmdBuf, &s);
    }
  }
  //
  // Begin and End Command Label MUST be balanced, this helps as it will always close the opened label
  //
  struct ScopedCmdLabel
  {
    ScopedCmdLabel(VkCommandBuffer cmdBuf, const std::string& label)
        : m_cmdBuf(cmdBuf)
    {
      if(s_enabled)
      {
        VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f}};
        vkCmdBeginDebugUtilsLabelEXT(cmdBuf, &s);
      }
    }
    ~ScopedCmdLabel()
    {
      if(s_enabled)
      {
        vkCmdEndDebugUtilsLabelEXT(m_cmdBuf);
      }
    }
    void setLabel(const std::string& label)
    {
      if(s_enabled)
      {
        VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f}};
        vkCmdInsertDebugUtilsLabelEXT(m_cmdBuf, &s);
      }
    }

  private:
    VkCommandBuffer m_cmdBuf;
  };

  ScopedCmdLabel scopeLabel(VkCommandBuffer cmdBuf, const std::string& label) { return ScopedCmdLabel(cmdBuf, label); }

private:
  VkDevice    m_device{VK_NULL_HANDLE};
  static bool s_enabled;
};

//////////////////////////////////////////////////////////////////////////
/// Macros to help automatically naming variables.
/// Names will be in the form of MyClass::m_myBuffer (in example.cpp:123)
///
/// To use:
/// - Debug member class MUST be named 'm_debug'
/// - Individual name: NAME_VK(m_myBuffer.buffer) or with and index NAME_IDX_VK(m_texture.image, i)
/// - Create/associate and name, instead of
///     pipeline = createPipeline();
///     NAME_VK(pipeline)
///   call
///     CREATE_NAMED_VK(pipeline , createPipeline());
/// - Scope functions can also be automatically named, at the beginning of a function
///   call LABEL_SCOPE_VK( commandBuffer )
///
///
// clang-format off
inline const char* fileNameSplitter(const char* n) { return std::max<const char*>(n, std::max(strrchr(n, '\\') + 1, strrchr(n, '/') + 1)); }
inline const char* upToLastSpace(const char* n) { return std::max<const char*>(n, strrchr(n, ' ') + 1); }
#define CLASS_NAME nvvk::upToLastSpace(typeid(*this).name())
#define NAME_FILE_LOCATION  std::string(" in ") + std::string(nvvk::fileNameSplitter(__FILE__)) + std::string(":" S__LINE__ ")")

// Individual naming
#define NAME_VK(_x) m_debug.setObjectName(_x, (std::string(CLASS_NAME) + std::string("::") + std::string(#_x " (") + NAME_FILE_LOCATION).c_str())
#define NAME2_VK(_x, _s) m_debug.setObjectName(_x, (std::string(_s) + std::string(" (" #_x) + NAME_FILE_LOCATION).c_str())
#define NAME_IDX_VK(_x, _i) m_debug.setObjectName(_x, \
                            (std::string(CLASS_NAME) + std::string("::") + std::string(#_x " (" #_i "=") + std::to_string(_i) + std::string(", ") + NAME_FILE_LOCATION).c_str())

// Name in creation
#define CREATE_NAMED_VK(_x, _c)              \
  _x = _c;                                   \
  NAME_VK(_x);
#define CREATE_NAMED_IDX_VK(_x, _i, _c)      \
  _x = _c;                                   \
  NAME_IDX_VK(_x, _i);

// Running scope
#define LABEL_SCOPE_VK(_cmd)                                                                                                                \
  auto _scopeLabel =  m_debug.scopeLabel(_cmd, std::string(CLASS_NAME) + std::string("::") + std::string(__func__) + std::string(", in ")   \
                                   + std::string(nvvk::fileNameSplitter(__FILE__)) + std::string(":" S__LINE__ ")"))


// Non-defined named variable of the above macros (Ex: m_myDbg->DBG_NAME(vulan_obj); )
#define DBG_NAME(_x)                                                                                                   \
  setObjectName(_x, (std::string(CLASS_NAME) + std::string("::") + std::string(#_x " (") + NAME_FILE_LOCATION).c_str())
#define DBG_NAME_IDX(_x, _i)                                                                                           \
  setObjectName(_x, (std::string(CLASS_NAME) + std::string("::") + std::string(#_x " (" #_i "=") + std::to_string(_i)  \
                     + std::string(", ") + NAME_FILE_LOCATION)                                                         \
                        .c_str())
#define DBG_SCOPE(_cmd)                                                                                                \
  scopeLabel(_cmd, std::string(CLASS_NAME) + std::string("::") + std::string(__func__) + std::string(", in ")          \
                       + std::string(nvvk::fileNameSplitter(__FILE__)) + std::string(":" S__LINE__ ")"))


// clang-format on
}  // namespace nvvk
