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


#ifndef NV_VK_SWAPCHAIN_INCLUDED
#define NV_VK_SWAPCHAIN_INCLUDED


#include <stdio.h>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace nvvk {

/**
\class nvvk::SwapChain

\brief nvvk::SwapChain is a helper to handle swapchain setup and use

In Vulkan, we have to use `VkSwapchainKHR` to request a swap chain
(front and back buffers) from the operating system and manually
synchronize our and OS's access to the images within the swap chain.
This helper abstracts that process.

For each swap chain image there is an ImageView, and one read and write
semaphore synchronizing it (see `SwapChainAcquireState`).

To start, you need to call `init`, then `update` with the window's
initial framebuffer size (for example, use `glfwGetFramebufferSize`).
Then, in your render loop, you need to call `acquire()` to get the
swap chain image to draw to, draw your frame (waiting and signalling
the appropriate semaphores), and call `present()`.

Sometimes, the swap chain needs to be re-created (usually due to
window resizes). `nvvk::SwapChain` detects this automatically and
re-creates the swap chain for you. Every new swap chain is assigned a
unique ID (`getChangeID()`), allowing you to detect swap chain
re-creations. This usually triggers a `VkDeviceWaitIdle`; however, if
this is not appropriate, see `setWaitQueue()`.

Finally, there is a utility function to setup the image transitions
from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
which is the format an image must be in before it is presented.

Example in combination with nvvk::Context :

* get the window handle
* create its related surface
* make sure the Queue is the one we need to render in this surface

\code{.cpp}
// could {.cpp}be arguments of a function/method :
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
\endcode

The initialization can happen now :

\code{.cpp}
m_swapChain.init(ctx.m_device, ctx.m_physicalDevice, ctx.m_queueGCT, ctx.m_queueGCT.familyIndex,
                 m_surface, VK_FORMAT_B8G8R8A8_UNORM);
...
// after init or update you also have to setup the image layouts at some point
VkCommandBuffer cmd = ...
m_swapChain.cmdUpdateBarriers(cmd);
\endcode

During a resizing of a window, you can update the swapchain as well :

\code{.cpp}
bool WindowSurface::resize(int w, int h)
{
...
  m_swapChain.update(w, h);
  // be cautious to also transition the image layouts
...
}
\endcode


A typical renderloop would look as follows:

\code{.cpp}
  // handles vkAcquireNextImageKHR and setting the active image
  // w,h only needed if update(w,h) not called reliably.
  int w, h;
  bool recreated;
  glfwGetFramebufferSize(window, &w, &h);
  if(!m_swapChain.acquire(w, h, &recreated, [, optional SwapChainAcquireState ptr]))
  {
    ... handle acquire error (shouldn't happen)
  }

  VkCommandBuffer cmd = ...

  // acquire might have recreated the swap chain: respond if needed here.
  // NOTE: you can also check the recreated variable above, but this
  // only works if the swap chain was recreated this frame.
  if (m_swapChain.getChangeID() != lastChangeID){
    // after init or resize you have to setup the image layouts
    m_swapChain.cmdUpdateBarriers(cmd);

    lastChangeID = m_swapChain.getChangeID();
  }

  // do render operations either directly using the imageview
  VkImageView swapImageView = m_swapChain.getActiveImageView();

  // or you may always render offline int your own framebuffer
  // and then simply blit into the backbuffer. NOTE: use
  // m_swapChain.getWidth() / getHeight() to get blit dimensions,
  // actual swap chain image size may differ from requested width/height.
  VkImage swapImage = m_swapChain.getActiveImage();
  vkCmdBlitImage(cmd, ... swapImage ...);

  // setup submit
  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &cmd;

  // we need to ensure to wait for the swapchain image to have been read already
  // so we can safely blit into it

  VkSemaphore swapchainReadSemaphore      = m_swapChain->getActiveReadSemaphore();
  VkPipelineStageFlags swapchainReadFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores    = &swapchainReadSemaphore;
  submitInfo.pWaitDstStageMask  = &swapchainReadFlags);

  // once this submit completed, it means we have written the swapchain image
  VkSemaphore swapchainWrittenSemaphore = m_swapChain->getActiveWrittenSemaphore();
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = &swapchainWrittenSemaphore;

  // submit it
  vkQueueSubmit(m_queue, 1, &submitInfo, fence);

  // present via a queue that supports it
  // this will also setup the dependency for the appropriate written semaphore
  // and bump the semaphore cycle
  m_swapChain.present(m_queue);
\endcode

*/

// What SwapChain::acquire produces: a swap chain image plus
// semaphores protecting it.
struct SwapChainAcquireState
{
  // The image and its view and index in the swap chain.
  VkImage     image;
  VkImageView view;
  uint32_t    index;
  // MUST wait on this semaphore before writing to the image. ("The
  // system" signals this semaphore when it's done presenting the
  // image and can safely be reused).
  VkSemaphore waitSem;
  // MUST signal this semaphore when done writing to the image, and
  // before presenting it. (The system waits for this before presenting).
  VkSemaphore signalSem;
};


class SwapChain
{
private:
  struct Entry
  {
    VkImage     image{};
    VkImageView imageView{};
    // be aware semaphore index may not match active image index
    VkSemaphore readSemaphore{};
    VkSemaphore writtenSemaphore{};
  };

  VkDevice         m_device         = VK_NULL_HANDLE;
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

  VkQueue  m_queue{};
  VkQueue  m_waitQueue{};  // See waitIdle and setWaitQueue.
  uint32_t m_queueFamilyIndex{0};

  VkSurfaceKHR    m_surface{};
  VkFormat        m_surfaceFormat{};
  VkColorSpaceKHR m_surfaceColor{};

  uint32_t       m_imageCount{0};
  VkSwapchainKHR m_swapchain{};

  std::vector<Entry>                m_entries;
  std::vector<VkImageMemoryBarrier> m_barriers;

  // index for current image, returned by vkAcquireNextImageKHR
  // vk spec: The order in which images are acquired is implementation-dependent,
  // and may be different than the order the images were presented
  uint32_t m_currentImage{0};
  // index for current semaphore, incremented by `SwapChain::present`
  uint32_t m_currentSemaphore{0};
  // incremented by `SwapChain::update`, use to update other resources or track changes
  uint32_t m_changeID{0};
  // surface
  VkExtent2D m_extent{0, 0};
  // requested on update
  uint32_t m_updateWidth{0};
  uint32_t m_updateHeight{0};
  // if the swap operation is sync'ed with monitor
  bool m_vsync = false;
  // if vsync is off which mode to prefer
  VkPresentModeKHR m_preferredVsyncOffMode = VK_PRESENT_MODE_MAILBOX_KHR;
  // usage flags for swapchain images
  VkImageUsageFlags m_imageUsage{};

  VkResult waitIdle()
  {
    if(m_waitQueue)
      return vkQueueWaitIdle(m_waitQueue);
    else
      return vkDeviceWaitIdle(m_device);
  }

  // triggers device/queue wait idle
  void deinitResources();

public:
  SwapChain(SwapChain const&) = delete;
  SwapChain& operator=(SwapChain const&) = delete;

  SwapChain() {}

  static constexpr VkImageUsageFlags s_defaultImageUsage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  SwapChain(VkDevice          device,
            VkPhysicalDevice  physicalDevice,
            VkQueue           queue,
            uint32_t          queueFamilyIndex,
            VkSurfaceKHR      surface,
            VkFormat          format     = VK_FORMAT_B8G8R8A8_UNORM,
            VkImageUsageFlags imageUsage = s_defaultImageUsage)
  {
    init(device, physicalDevice, queue, queueFamilyIndex, surface, format, imageUsage);
  }
  ~SwapChain() { deinit(); }

  bool init(VkDevice          device,
            VkPhysicalDevice  physicalDevice,
            VkQueue           queue,
            uint32_t          queueFamilyIndex,
            VkSurfaceKHR      surface,
            VkFormat          format     = VK_FORMAT_B8G8R8A8_UNORM,
            VkImageUsageFlags imageUsage = s_defaultImageUsage);

  // triggers queue/device wait idle
  void deinit();

  // update the swapchain configuration
  // (must be called at least once after init)
  // triggers queue/device wait idle
  // returns actual swapchain dimensions, which may differ from requested
  VkExtent2D update(int width, int height, bool vsync);
  VkExtent2D update(int width, int height) { return update(width, height, m_vsync); }

  // Returns true on success.
  //
  // Sets active index to the next swap chain image to draw to.
  // The handles and semaphores for this image are optionally written to *pOut.
  //
  // `acquire` and `acquireAutoResize` use getActiveReadSemaphore();
  // `acquireCustom` allows you to provide your own semaphore.
  //
  // If the swap chain was invalidated (window resized, etc.), the
  // swap chain will be recreated, which triggers queue/device wait
  // idle.  If you are not calling `update` manually on window resize,
  // you must pass the new swap image size explicitly.
  //
  // WARNING: The actual swap image size might not match what is
  // requested; use getWidth/getHeight to check actual swap image
  // size.
  //
  // If the swap chain was recreated, *pRecreated is set to true (if
  // pRecreated != nullptr); otherwise, set to false.
  //
  // WARNING the swap chain could be spontaneously recreated, even if
  // you are calling `update` whenever the window is resized.
  bool acquire(bool* pRecreated = nullptr, SwapChainAcquireState* pOut = nullptr);
  bool acquireAutoResize(int width, int height, bool* pRecreated, SwapChainAcquireState* pOut = nullptr);

  // Can be made public if this functionality is needed again.
private:
  bool acquireCustom(VkSemaphore semaphore, bool* pRecreated = nullptr, SwapChainAcquireState* pOut = nullptr);
  bool acquireCustom(VkSemaphore semaphore, int width, int height, bool* pRecreated, SwapChainAcquireState* pOut = nullptr);

public:
  // all present functions bump semaphore cycle

  // present on provided queue
  void present(VkQueue queue);
  // present using a default queue from init time
  void present() { present(m_queue); }
  // present via a custom function
  // (e.g. when extending via VkDeviceGroupPresentInfoKHR)
  // fills in defaults for provided presentInfo
  // with getActiveImageIndex()
  // and getActiveWrittenSemaphore()
  void presentCustom(VkPresentInfoKHR& outPresentInfo);

  VkSemaphore getActiveReadSemaphore() const;
  VkSemaphore getActiveWrittenSemaphore() const;
  VkImage     getActiveImage() const;
  VkImageView getActiveImageView() const;
  uint32_t    getActiveImageIndex() const { return m_currentImage; }

  uint32_t    getImageCount() const { return m_imageCount; }
  VkImage     getImage(uint32_t i) const;
  VkImageView getImageView(uint32_t i) const;
  VkFormat    getFormat() const { return m_surfaceFormat; }

  // Get the actual size of the swap chain images.
  uint32_t   getWidth() const { return m_extent.width; }
  uint32_t   getHeight() const { return m_extent.height; }
  VkExtent2D getExtent() const { return m_extent; }

  // Get the requested size of the swap chain images. THIS IS RARELY USEFUL.
  uint32_t getUpdateWidth() const { return m_updateWidth; }
  uint32_t getUpdateHeight() const { return m_updateHeight; }

  bool           getVsync() const { return m_vsync; }
  VkSwapchainKHR getSwapchain() const { return m_swapchain; }

  // does a vkCmdPipelineBarrier for VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  // must apply resource transitions after update calls
  void cmdUpdateBarriers(VkCommandBuffer cmd) const;

  uint32_t getChangeID() const;

  // Ordinarily, `SwapChain` calls vkDeviceWaitIdle before recreating
  // the swap chain. However, if setWaitQueue is called with a
  // non-null queue, we only wait for that queue instead of the whole
  // device.  This may be needed if you are using queues in other CPU
  // threads that are not synchronized to the render loop.
  void setWaitQueue(VkQueue waitQueue = VK_NULL_HANDLE) { m_waitQueue = waitQueue; }

  // typically either VK_PRESENT_MODE_MAILBOX_KHR or VK_PRESENT_MODE_IMMEDIATE_KHR
  void setPreferredVsyncOffMode(VkPresentModeKHR mode) { m_preferredVsyncOffMode = mode; }
};
}  // namespace nvvk
#endif
