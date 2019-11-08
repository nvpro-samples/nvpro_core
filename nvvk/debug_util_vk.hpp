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
  DebugUtil(const VkDevice& device) : m_device(device) {}

  void setup(const VkDevice& device)
  {
    m_device   = device;
  }

  void setObjectName(const uint64_t object, const char* name, VkObjectType t)
  {
#ifdef _DEBUG
    VkDebugUtilsObjectNameInfoEXT s{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT , nullptr, t, object, name };
    vkSetDebugUtilsObjectNameEXT(m_device, &s);
#endif  // _DEBUG
  }

  void setObjectName(const VkBuffer& object, const char* name)                  { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_BUFFER); }
  void setObjectName(const VkCommandBuffer& object, const char* name)           { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_COMMAND_BUFFER ); }
  void setObjectName(const VkImage& object, const char* name)                   { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_IMAGE); }
  void setObjectName(const VkImageView& object, const char* name)               { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_IMAGE_VIEW); }
  void setObjectName(const VkRenderPass& object, const char* name)              { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_RENDER_PASS); }
  void setObjectName(const VkShaderModule& object, const char* name)            { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SHADER_MODULE); }
  void setObjectName(const VkPipeline& object, const char* name)                { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_PIPELINE); }
  void setObjectName(const VkAccelerationStructureNV& object, const char* name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV); }
  void setObjectName(const VkDescriptorSetLayout& object, const char* name)     { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT); }
  void setObjectName(const VkDescriptorSet& object, const char* name)           { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET); }
  void setObjectName(const VkSemaphore& object, const char* name)               { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SEMAPHORE); }
  void setObjectName(const VkSwapchainKHR& object, const char* name)            { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SWAPCHAIN_KHR); }
  void setObjectName(const VkQueue& object, const char* name)                   { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_QUEUE); }
  //
  //---------------------------------------------------------------------------
  //
  void beginLabel(const VkCommandBuffer& cmdBuf, const char* label)
  {
#ifdef _DEBUG
    VkDebugUtilsLabelEXT s{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label, {1.0f, 1.0f, 1.0f, 1.0f} };
    vkCmdBeginDebugUtilsLabelEXT(cmdBuf, &s);
#endif  // _DEBUG
  }
  void endLabel(const VkCommandBuffer& cmdBuf)
  {
#ifdef _DEBUG
    vkCmdEndDebugUtilsLabelEXT(cmdBuf);
#endif  // _DEBUG
  }
  void insertLabel(const VkCommandBuffer& cmdBuf, const char* label)
  {
#ifdef _DEBUG
    VkDebugUtilsLabelEXT s{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label, {1.0f, 1.0f, 1.0f, 1.0f} };
    vkCmdInsertDebugUtilsLabelEXT(cmdBuf, &s);
#endif  // _DEBUG
  }
  //
  // Begin and End Command Label MUST be balanced, this helps as it will always close the opened label
  //
  struct ScopedCmdLabel
  {
    ScopedCmdLabel(const VkCommandBuffer& cmdBuf, const char* label)
        : m_cmdBuf(cmdBuf)
    {
#ifdef _DEBUG
      VkDebugUtilsLabelEXT s{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label, {1.0f, 1.0f, 1.0f, 1.0f} };
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
      VkDebugUtilsLabelEXT s{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label, {1.0f, 1.0f, 1.0f, 1.0f} };
      vkCmdInsertDebugUtilsLabelEXT(m_cmdBuf, &s);
#endif  // _DEBUG
    }

  private:
    const VkCommandBuffer&  m_cmdBuf;
  };

  ScopedCmdLabel scopeLabel(const VkCommandBuffer& cmdBuf, const char* label)
  {
    return ScopedCmdLabel(cmdBuf, label);
  }

private:
  VkDevice                m_device;
};

}  // namespace nvvk
