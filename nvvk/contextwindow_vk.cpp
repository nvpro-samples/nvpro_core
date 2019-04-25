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

// must do vulkan include first to catch win32 includes, which are otherwise omitted
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>


#include "contextwindow_vk.hpp"
#include <nvpwindow.hpp>
#include <nvpwindow_internal.hpp>

namespace nvvk
{
  //void ContextWindowVK::initWindow(VkSurfaceKHR surface, uint32_t queueFamily, VkQueueFlags queueFlags, uint32_t queueIndex)
  bool ContextWindowVK::init(const ContextInfoVK* contextInfo, NVPWindow* sourcewindow)
  {
    ContextInfoVK cflagsUsed = *contextInfo;
    m_swapVsync = 0;
    m_windowSize[0] = sourcewindow->getWidth();
    m_windowSize[1] = sourcewindow->getHeight();

    const char* exts[] = {
      VK_KHR_SURFACE_EXTENSION_NAME,
    #ifdef _WIN32
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    #endif
    };
    uint32_t numExtensions = int(sizeof(exts) / sizeof(exts[0]));
    for (uint32_t i = 0; i < numExtensions; i++) {
      cflagsUsed.addInstanceExtension(exts[i], false);
    }
    cflagsUsed.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME, false);

    if (!m_context.initContext(cflagsUsed)) {
      return false;
    }

    // Construct the surface description:
    VkSurfaceKHR surface;
    VkResult result;
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    extern HINSTANCE   g_hInstance;
    createInfo.hinstance = g_hInstance;
    createInfo.hwnd = sourcewindow->m_internal->m_hWnd;
    result = vkCreateWin32SurfaceKHR(m_context.m_instance, &createInfo, NULL, &surface);
#else  // _WIN32
    VkXcbSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.connection = info.connection;
    createInfo.window = info.window;
    result = vkCreateXcbSurfaceKHR(m_context.m_instance, &createInfo, NULL, &surface);
#endif // _WIN32
    assert(result == VK_SUCCESS);

    int queueIndex = 0;
    uint32_t queueFamily = VK_QUEUE_FAMILY_IGNORED;
    m_surface = surface;
    m_swapChain.init(m_context.m_device, m_context.m_physicalDevice, surface, queueIndex, m_context.m_allocator);
    m_presentQueueFamily = queueFamily != VK_QUEUE_FAMILY_IGNORED ? queueFamily : m_context.m_physicalInfo.getPresentQueueFamily(surface, VK_QUEUE_GRAPHICS_BIT);
    assert(m_presentQueueFamily != VK_QUEUE_FAMILY_IGNORED);
    vkGetDeviceQueue(m_context.m_device, m_presentQueueFamily, queueIndex, &m_presentQueue);

    m_swapChain.update(m_windowSize[0], m_windowSize[1], m_swapVsync);
    return result == VK_SUCCESS;
  }

  void ContextWindowVK::deinit()
  {
    vkDeviceWaitIdle(m_context.m_device);
    m_swapChain.deinit();
    vkDestroySurfaceKHR(m_context.m_instance, m_surface, m_context.m_allocator);
    m_context.deinitContext();
  }
  void ContextWindowVK::swapVsync(bool state)
  {
    if(m_swapVsync != state)
    {
      m_swapChain.update(m_windowSize[0], m_windowSize[1], state);
      m_swapVsync = state;
    }
  }

  void ContextWindowVK::swapBuffers()
  {
    m_swapChain.present(m_presentQueue);
  }

  void ContextWindowVK::swapPrepare()
  {
    if (!m_swapChain.acquire()) {
      LOGE("error: vulkan swapchain acqiure failed, try -vsync 1\n");
      exit(-1);
    }
  }

  void ContextWindowVK::swapResize(int w, int h)
  {
    if((m_windowSize[0] != w)||(m_windowSize[1] = h)) {
      VkResult err;
      err = vkDeviceWaitIdle(m_context.m_device);
      m_windowSize[0] = w;
      m_windowSize[1] = h;
      m_swapChain.update(w, h, m_swapVsync);
    }
  }
}

