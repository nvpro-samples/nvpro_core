/*
* Copyright (c) 2014-2019, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

// This is a companion utility to add debug information to an application
// See https://vulkan.lunarg.com/doc/sdk/1.1.114.0/windows/chunked_spec/chap39.html
// - User defined name to objects
// - Logically annotate region of command buffers
// - Scoped command buffer label to make thing simpler

#pragma once

#include <vulkan/vulkan_core.h>

namespace nvvk {

struct DebugUtil
{
  DebugUtil() = default;
  DebugUtil(VkDevice device)
      : m_device(device)
  {
  }

  void setup(VkDevice device) { m_device = device; }

  void setObjectName(const uint64_t object, const char* name, VkObjectType t)
  {
#ifdef _DEBUG
    VkDebugUtilsObjectNameInfoEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, t, object, name};
    vkSetDebugUtilsObjectNameEXT(m_device, &s);
#endif  // _DEBUG
  }
  // clang-format off
  void setObjectName(VkDeviceMemory object, const char* name)            { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DEVICE_MEMORY); }
  void setObjectName(VkBuffer object, const char* name)                  { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_BUFFER); }
  void setObjectName(VkBufferView object, const char* name)              { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_BUFFER_VIEW); }
  void setObjectName(VkCommandBuffer object, const char* name)           { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_COMMAND_BUFFER ); }
  void setObjectName(VkImage object, const char* name)                   { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_IMAGE); }
  void setObjectName(VkImageView object, const char* name)               { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_IMAGE_VIEW); }
  void setObjectName(VkRenderPass object, const char* name)              { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_RENDER_PASS); }
  void setObjectName(VkShaderModule object, const char* name)            { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SHADER_MODULE); }
  void setObjectName(VkPipeline object, const char* name)                { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_PIPELINE); }
  void setObjectName(VkAccelerationStructureNV object, const char* name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV); }
  void setObjectName(VkDescriptorSetLayout object, const char* name)     { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT); }
  void setObjectName(VkDescriptorSet object, const char* name)           { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET); }
  void setObjectName(VkDescriptorPool object, const char* name)          { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_POOL); }
  void setObjectName(VkSemaphore object, const char* name)               { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SEMAPHORE); }
  void setObjectName(VkSwapchainKHR object, const char* name)            { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SWAPCHAIN_KHR); }
  void setObjectName(VkQueue object, const char* name)                   { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_QUEUE); }
  void setObjectName(VkFramebuffer object, const char* name)             { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_FRAMEBUFFER); }
  // clang-format on

  //
  //---------------------------------------------------------------------------
  //
  void beginLabel(VkCommandBuffer cmdBuf, const char* label)
  {
#ifdef _DEBUG
    VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label, {1.0f, 1.0f, 1.0f, 1.0f}};
    vkCmdBeginDebugUtilsLabelEXT(cmdBuf, &s);
#endif  // _DEBUG
  }
  void endLabel(VkCommandBuffer cmdBuf)
  {
#ifdef _DEBUG
    vkCmdEndDebugUtilsLabelEXT(cmdBuf);
#endif  // _DEBUG
  }
  void insertLabel(VkCommandBuffer cmdBuf, const char* label)
  {
#ifdef _DEBUG
    VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label, {1.0f, 1.0f, 1.0f, 1.0f}};
    vkCmdInsertDebugUtilsLabelEXT(cmdBuf, &s);
#endif  // _DEBUG
  }
  //
  // Begin and End Command Label MUST be balanced, this helps as it will always close the opened label
  //
  struct ScopedCmdLabel
  {
    ScopedCmdLabel(VkCommandBuffer cmdBuf, const char* label)
        : m_cmdBuf(cmdBuf)
    {
#ifdef _DEBUG
      VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label, {1.0f, 1.0f, 1.0f, 1.0f}};
      vkCmdBeginDebugUtilsLabelEXT(cmdBuf, &s);
#endif  // _DEBUG
    }
    ~ScopedCmdLabel()
    {
#ifdef _DEBUG
      vkCmdEndDebugUtilsLabelEXT(m_cmdBuf);
#endif  // _DEBUG
    }
    void setLabel(const char* label)
    {
#ifdef _DEBUG
      VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label, {1.0f, 1.0f, 1.0f, 1.0f}};
      vkCmdInsertDebugUtilsLabelEXT(m_cmdBuf, &s);
#endif  // _DEBUG
    }

  private:
    VkCommandBuffer m_cmdBuf;
  };

  ScopedCmdLabel scopeLabel(VkCommandBuffer cmdBuf, const char* label) { return ScopedCmdLabel(cmdBuf, label); }

private:
  VkDevice m_device;
};

}  // namespace nvvk
