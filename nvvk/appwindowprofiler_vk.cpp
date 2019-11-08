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


#include "appwindowprofiler_vk.hpp"
#include "context_vk.hpp"
#include "error_vk.hpp"

#include <nvh/misc.hpp>
#include <nvh/nvprint.hpp>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan_win32.h>
#endif

namespace nvvk {

void AppWindowProfilerVK::contextInit()
{
  //m_contextWindow.init(&m_deviceInfo, this);
  ContextCreateInfo contextInfo = m_contextInfo;
  m_swapVsync                   = false;

  contextInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME, false);
#ifdef _WIN32
  contextInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, false);
#else
  contextInfo.addInstanceExtension(VK_KHR_XCB_SURFACE_EXTENSION_NAME, false);
#endif
  contextInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME, false);

  if(!m_context.init(contextInfo))
  {
    return;
  }

  // Construct the surface description:
  VkResult result;
#ifdef _WIN32
  HWND      hWnd      = glfwGetWin32Window(m_internal);
  HINSTANCE hInstance = GetModuleHandle(NULL);

  VkWin32SurfaceCreateInfoKHR createInfo = {};
  createInfo.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  createInfo.pNext                       = NULL;
  createInfo.hinstance                   = hInstance;
  createInfo.hwnd                        = hWnd;
  result = vkCreateWin32SurfaceKHR(m_context.m_instance, &createInfo, nullptr, &m_surface);
#else   // _WIN32
  VkXcbSurfaceCreateInfoKHR createInfo = {};
  createInfo.sType                     = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
  createInfo.pNext                     = NULL;
  createInfo.connection                = info.connection;
  createInfo.window                    = info.window;
  result = vkCreateXcbSurfaceKHR(m_context.m_instance, &createInfo, nullptr, &m_surface);
#endif  // _WIN32
  assert(result == VK_SUCCESS);

  m_context.setGCTQueueWithPresent(m_surface);

  m_swapChain.init(m_context.m_device, m_context.m_physicalDevice, m_context.m_queueGCT,
                   m_context.m_queueGCT.familyIndex, m_surface);
  m_swapChain.update(getWidth(), getHeight(), m_swapVsync);

  m_profilerVK.init(m_context.m_device, m_context.m_physicalDevice);
  m_profilerVK.setMarkerUsage(m_context.hasDeviceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));
}

void AppWindowProfilerVK::contextDeinit()
{
  m_profilerVK.deinit();

  VkResult result = vkDeviceWaitIdle(m_context.m_device);
  if (nvvk::checkResult(result, __FILE__, __LINE__)) {
    exit(-1);
  }
  m_swapChain.deinit();
  vkDestroySurfaceKHR(m_context.m_instance, m_surface, nullptr);
  m_context.deinit();
}

void AppWindowProfilerVK::swapResize(int width, int height)
{
  if((m_swapChain.getWidth() != width) || (m_swapChain.getHeight() != height))
  {
    VkResult result = vkDeviceWaitIdle(m_context.m_device);
    if (nvvk::checkResult(result, __FILE__, __LINE__)) {
      exit(-1);
    }
    m_swapChain.update(width, height, m_swapVsync);
  }
}
void AppWindowProfilerVK::swapPrepare()
{
  if(!m_swapChain.acquire())
  {
    LOGE("error: vulkan swapchain acqiure failed, try -vsync 1\n");
    exit(-1);
  }
}

void AppWindowProfilerVK::swapBuffers()
{
  m_swapChain.present(m_context.m_queueGCT);
}

void AppWindowProfilerVK::swapVsync(bool swapVsync)
{
  if(m_swapVsync != swapVsync)
  {
    VkResult result = vkDeviceWaitIdle(m_context.m_device);
    if (nvvk::checkResult(result, __FILE__, __LINE__)) {
      exit(-1);
    }
    m_swapChain.update(getWidth(), getHeight(), swapVsync);
    m_swapVsync = swapVsync;
  }
}

const char* AppWindowProfilerVK::contextGetDeviceName()
{
  return m_context.m_physicalInfo.properties.deviceName;
}

}  // namespace nvvk
