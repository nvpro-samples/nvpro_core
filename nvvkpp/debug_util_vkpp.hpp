/*
* Copyright (c) 2014-2019, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

/**
 This is a companion utility to add debug information to an application. 
 See [Chapter 39](https://vulkan.lunarg.com/doc/sdk/1.1.114.0/windows/chunked_spec/chap39.html).

 - User defined name to objects
 - Logically annotate region of command buffers
 - Scoped command buffer label to make thing simpler
 
 
Example
~~~~~~ C++
nvvkpp::DebugUtil    m_debug;
m_debug.setup(device);
...
m_debug.setObjectName(m_vertices.buffer, "sceneVertex");
m_debug.setObjectName(m_pipeline, "scenePipeline");
~~~~~~ 
 
*/
#pragma once

#include <vulkan/vulkan.hpp>


namespace nvvkpp {

struct DebugUtil
{
  DebugUtil() = default;
  DebugUtil(const vk::Device& device)
      : m_device(device)
  {
  }

  void setup(const vk::Device& device) { m_device = device; }

  template <typename T>
  void setObjectName(const T& object, const char* name, vk::ObjectType t)
  {
#ifdef _DEBUG
    m_device.setDebugUtilsObjectNameEXT({t, reinterpret_cast<const uint64_t&>(object), name});
#endif  // _DEBUG
  }


  // clang-format off
  void setObjectName(const vk::Buffer& object, const char* name)                  { setObjectName(object, name, vk::ObjectType::eBuffer); }
  void setObjectName(const vk::CommandBuffer& object, const char* name)           { setObjectName(object, name, vk::ObjectType::eCommandBuffer ); }
  void setObjectName(const vk::Image& object, const char* name)                   { setObjectName(object, name, vk::ObjectType::eImage); }
  void setObjectName(const vk::ImageView& object, const char* name)               { setObjectName(object, name, vk::ObjectType::eImageView); }
  void setObjectName(const vk::RenderPass& object, const char* name)              { setObjectName(object, name, vk::ObjectType::eRenderPass); }
  void setObjectName(const vk::ShaderModule& object, const char* name)            { setObjectName(object, name, vk::ObjectType::eShaderModule); }
  void setObjectName(const vk::Pipeline& object, const char* name)                { setObjectName(object, name, vk::ObjectType::ePipeline); }
  void setObjectName(const vk::AccelerationStructureNV& object, const char* name) { setObjectName(object, name, vk::ObjectType::eAccelerationStructureNV); }
  void setObjectName(const vk::DescriptorSetLayout& object, const char* name)     { setObjectName(object, name, vk::ObjectType::eDescriptorSetLayout); }
  void setObjectName(const vk::DescriptorSet& object, const char* name)           { setObjectName(object, name, vk::ObjectType::eDescriptorSet); }
  void setObjectName(const vk::Semaphore& object, const char* name)               { setObjectName(object, name, vk::ObjectType::eSemaphore); }
  void setObjectName(const vk::SwapchainKHR& object, const char* name)            { setObjectName(object, name, vk::ObjectType::eSwapchainKHR); }
  void setObjectName(const vk::Queue& object, const char* name)                   { setObjectName(object, name, vk::ObjectType::eQueue); }
  // clang-format on


  //
  //---------------------------------------------------------------------------
  //
  void beginLabel(const vk::CommandBuffer& cmdBuf, const char* label)
  {
#ifdef _DEBUG
    cmdBuf.beginDebugUtilsLabelEXT({label});
#endif  // _DEBUG
  }
  void endLabel(const vk::CommandBuffer& cmdBuf)
  {
#ifdef _DEBUG
    cmdBuf.endDebugUtilsLabelEXT();
#endif  // _DEBUG
  }
  void insertLabel(const vk::CommandBuffer& cmdBuf, const char* label)
  {
#ifdef _DEBUG
    cmdBuf.insertDebugUtilsLabelEXT({label});
#endif  // _DEBUG
  }
  //
  // Begin and End Command Label MUST be balanced, this helps as it will always close the opened label
  //
  struct ScopedCmdLabel
  {
    ScopedCmdLabel(const vk::CommandBuffer& cmdBuf, const char* label)
        : m_cmdBuf(cmdBuf)
    {
#ifdef _DEBUG
      cmdBuf.beginDebugUtilsLabelEXT({label});
#endif  // _DEBUG
    }
    ~ScopedCmdLabel()
    {
#ifdef _DEBUG
      m_cmdBuf.endDebugUtilsLabelEXT();
#endif  // _DEBUG
    }
    void setLabel(const char* label)
    {
#ifdef _DEBUG
      m_cmdBuf.insertDebugUtilsLabelEXT({label});
#endif  // _DEBUG
    }

  private:
    const vk::CommandBuffer& m_cmdBuf;
  };

  ScopedCmdLabel scopeLabel(const vk::CommandBuffer& cmdBuf, const char* label)
  {
    return ScopedCmdLabel(cmdBuf, label);
  }

private:
  vk::Device m_device;
};

}  // namespace nvvkpp
