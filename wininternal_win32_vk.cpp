/*-----------------------------------------------------------------------
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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
*/ //--------------------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include "resources.h"

#define DECL_WININTERNAL
#include "main.h"

#include <vulkan/vulkan.h>
#ifdef USEVULKANSDK
#include <vulkan/vk_sdk_platform.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

#include "wininternal_win32_vk.hpp"

extern LRESULT CALLBACK WindowProc(HWND   m_hWnd,
  UINT   msg,
  WPARAM wParam,
  LPARAM lParam);
extern HINSTANCE   g_hInstance;
extern std::vector<NVPWindow *> g_windows;

template <typename T, size_t sz> inline uint32_t getArraySize(T(&t)[sz]) { return sz; }


VkResult NVPWindow::createSurfaceVK(VkInstance instance, const VkAllocationCallbacks *allocator, VkSurfaceKHR* surface)
{
  VkWin32SurfaceCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  createInfo.pNext = NULL;
  createInfo.hinstance = g_hInstance;
  createInfo.hwnd = m_internal->m_hWnd;
  VkResult res = vkCreateWin32SurfaceKHR(instance, &createInfo, allocator, surface);

  return res;
}

const nv_helpers_vk::BasicWindow* NVPWindow::getBasicWindowVK()
{
  assert(m_api == WINDOW_API_VULKAN);
  return &static_cast<WINinternalVK*>(m_internal)->m_basicWindow;
}


const char** NVPWindow::sysGetRequiredSurfaceExtensionsVK(uint32_t &numExtensions) {
  static const char* extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
  };

  numExtensions = int(sizeof(extensions) / sizeof(extensions[0]));
  return extensions;
}

//------------------------------------------------------------------------------
// allocator for this
//------------------------------------------------------------------------------
WINinternal* newWINinternalVK(NVPWindow *win)
{
  return new WINinternalVK(win);
}

//------------------------------------------------------------------------------
// TODO:
// - cflags could contain the layers we want to activate
// - choice to make on queue: shall we leave it to the sample ?
//------------------------------------------------------------------------------
bool WINinternalVK::initBase(const NVPWindow::ContextFlagsBase* baseFlags, NVPWindow* sourcewindow)
{
  const NVPWindow::ContextFlagsVK* cflags = (const NVPWindow::ContextFlagsVK*)baseFlags;
  DestroyWindow(m_hWndDummy);
  m_hWndDummy = NULL;

  NVPWindow::ContextFlagsVK cflagsUsed = *cflags;

  uint32_t numExtensions;
  const char** exts = NVPWindow::sysGetRequiredSurfaceExtensionsVK(numExtensions);
  for (uint32_t i = 0; i < numExtensions; i++) {
    cflagsUsed.addInstanceExtension(exts[i], false);
  }
  cflagsUsed.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME, false);

  if (!cflagsUsed.initDeviceContext(m_basicWindow.m_context)) {
    return false;
  }

  m_deviceName = std::string(m_basicWindow.m_context.m_physicalInfo.properties.deviceName);

  // Construct the surface description:
  VkSurfaceKHR surface;
  VkResult result;
#ifdef _WIN32
  VkWin32SurfaceCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  createInfo.pNext = NULL;
  createInfo.hinstance = g_hInstance;
  createInfo.hwnd = m_hWnd;
  result = vkCreateWin32SurfaceKHR(m_basicWindow.m_context.m_instance, &createInfo, NULL, &surface);
#else  // _WIN32
  VkXcbSurfaceCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
  createInfo.pNext = NULL;
  createInfo.connection = info.connection;
  createInfo.window = info.window;
  result = vkCreateXcbSurfaceKHR(m_basicWindow.m_context.m_instance, &createInfo, NULL, &surface);
#endif // _WIN32
  assert(result == VK_SUCCESS);

  m_basicWindow.initWindow(surface, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_GRAPHICS_BIT, 0);

  // m_basicWindow.m_swapChain.update(16, 16, m_swapInterval != 0);

  return true;
}


void WINinternalVK::swapInterval(int i)
{
  if (m_swapInterval != i) {
    m_basicWindow.m_swapChain.update(m_win->getWidth(), m_win->getHeight(), i != 0);
    m_swapInterval = i;
  }
}

void WINinternalVK::swapBuffers()
{
  m_basicWindow.m_swapChain.present(m_basicWindow.m_presentQueue);
}

void WINinternalVK::swapPrepare()
{
  if (!m_basicWindow.m_swapChain.acquire()) {
    LOGE("error: vulkan swapchain acqiure failed, try -vsync 1\n");
    exit(-1);
  }
}

void WINinternalVK::terminate()
{
  m_basicWindow.deinitWindow();
  WINinternal::terminate();
}

void WINinternalVK::reshape(int w, int h)
{
  m_basicWindow.m_swapChain.update(w, h, m_swapInterval != 0);
}
