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

#ifndef NV_VK_SWAPCHAIN_INCLUDED
#define NV_VK_SWAPCHAIN_INCLUDED


#include <stdio.h>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace nvvk {

/**
# class nvvk::SwapChain

Its role is to help using VkSwapchainKHR. In Vulkan we have 
to synchronize the backbuffer access ourselves, meaning we
must not write into images that the operating system uses for
presenting the image on the desktop or monitor.


*/
class SwapChain
{
private:
  VkDevice                     m_device;
  VkPhysicalDevice             m_physicalDevice;

  VkQueue  m_queue;
  uint32_t m_queueFamilyIndex;

  VkSurfaceKHR    m_surface;
  VkFormat        m_surfaceFormat;
  VkColorSpaceKHR m_surfaceColor;

  uint32_t       m_swapchainImageCount;
  VkSwapchainKHR m_swapchain;

  std::vector<VkImage>              m_swapchainImages;
  std::vector<VkImageView>          m_swapchainImageViews;
  std::vector<VkSemaphore>          m_swapchainSemaphores;
  std::vector<VkImageMemoryBarrier> m_swapchainBarriers;
#if _DEBUG
  std::vector<std::string>          m_swapchainImageNames;
  std::vector<std::string>          m_swapchainImageViewNames;
  std::vector<std::string>          m_swapchainSemaphoreNames;
#endif
  // index for current image, returned by vkAcquireNextImageKHR
  // vk spec: The order in which images are acquired is implementation-dependent, 
  // and may be different than the order the images were presented
  uint32_t m_currentImage;
  // index for current semaphore, incremented by `SwapChain::present`
  uint32_t m_currentSemaphore;
  // incremented by `SwapChain::update`, use to update other resources/track
  // changes
  uint32_t m_changeID;
  // surface width
  uint32_t m_width;
  // surface height
  uint32_t m_height;
  // if the swap operation is sync'ed with monitor
  bool     m_vsync;

  void deinitResources();

public:
  bool init(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM);
  void update(int width, int height, bool vsync);
  void update(int width, int height) {
    update(width, height, m_vsync);
  }
  void deinit();

  // returns true on success
  bool acquire();
  void present(VkQueue queue);
  void present() { present(m_queue); }

  VkFormat    getFormat() const;
  VkSemaphore getActiveReadSemaphore() const;
  VkSemaphore getActiveWrittenSemaphore() const;
  VkImage     getActiveImage() const;
  VkImageView getActiveImageView() const;
  VkImage     getImage(uint32_t i) const;
  VkImageView getImageView(uint32_t i) const;
  uint32_t    getActiveImageIndex() const;
  uint32_t    getSwapchainImageCount() const { return m_swapchainImageCount; }
  uint32_t    getWidth() const { return m_width; }
  uint32_t    getHeight() const { return m_height; }
  bool        getVsync() const { return m_vsync; }
  VkQueue     getQueue() const { return m_queue; }
  uint32_t    getQueueFamilyIndex() { return m_queueFamilyIndex; }

  // from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  // must apply resource transitions after update calls
  const VkImageMemoryBarrier* getImageMemoryBarriers() const;
  uint32_t                    getImageCount() const;
  void                        cmdUpdateBarriers(VkCommandBuffer cmd) const;

  void getActiveBarrier(VkAccessFlags         srcAccess,
                        VkAccessFlags         dstAccess,
                        VkImageLayout         oldLayout,
                        VkImageLayout         newLayout,
                        VkImageMemoryBarrier& barrier) const;

  uint32_t getChangeID() const;

  VkSwapchainKHR getSwapchain() const { return m_swapchain; }
};
}  // namespace nvvk

/**
To setup the swapchain :

* get the window handle
* create its related surface
* make sure the Queue is the one we need to render in this surface

~~~ C++
// could be arguments of a function/method :
nvvk::Context ctx;
NVPWindow     win;
...

// get the surface of the window in which to render
VkWin32SurfaceCreateInfoKHR createInfo = {};
... populate the fields of createInfo ...
createInfo.hwnd = glfwGetWin32Window(win.m_internal);
result = vkCreateWin32SurfaceKHR(ctx.m_instance, &createInfo, nullptr, &m_surface);

...
// make sure we assign the proper Queue to m_queueGCT, from what the surface tells us
ctx.setGCTQueueWithPresent(m_surface);
~~~

The initialization can happen now :

~~~ C+
m_swapChain.init(ctx.m_device, ctx.m_physicalDevice, ctx.m_queueGCT, ctx.m_queueGCT.familyIndex, 
                 m_surface, VK_FORMAT_B8G8R8A8_UNORM);
...
// after init or update you also have to setup the image layouts at some point
VkCommandBuffer cmd = ...
m_swapChain.cmdUpdateBarriers(cmd);
~~~

During a resizing of a window, you must update the swapchain as well :

~~~ C++
bool WindowSurface::resize(int w, int h)
{
...
  m_swapChain.update(w, h);
  // be cautious to also transition the image layouts
...
}
~~~


A typical renderloop would look as follows:

~~~ C++
  // handles vkAcquireNextImageKHR and setting the active image
  if(!m_swapChain.acquire())
  {
    ... handle acquire error
  }

  VkCommandBuffer cmd = ...

  if (m_swapChain.getChangeID() != lastChangeID){
    // after init or resize you have to setup the image layouts
    m_swapChain.cmdUpdateBarriers(cmd);

    lastChangeID = m_swapChain.getChangeID();
  }

  // do render operations either directly using the imageview
  VkImageView swapImageView = m_swapChain.getActiveImageView();

  // or you may always render offline int your own framebuffer
  // and then simply blit into the backbuffer
  VkImage swapImage = m_swapChain.getActiveImage();
  vkCmdBlitImage(cmd, ... swapImage ...);

  // present via a queue that supports it
  m_swapChain.present(m_queue);
~~~
*/  
#endif
