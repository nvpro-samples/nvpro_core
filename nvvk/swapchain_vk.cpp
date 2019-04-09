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

#include "swapchain_vk.hpp"
#include <assert.h>

namespace nvvk {
  void SwapChain::init(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t queueFamilyIndex, const VkAllocationCallbacks* pAllocator)
  {
    m_surface = surface;
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_swapchain = VK_NULL_HANDLE;
    m_allocator = pAllocator;
    m_queueFamilyIndex = queueFamilyIndex;
    m_incarnation = 0;

    m_currentSemaphore = 0;
    VkResult result;

    // Get the list of VkFormat's that are supported:
    uint32_t formatCount;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface,
      &formatCount, nullptr);
    assert(!result);

    std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface,
      &formatCount, surfFormats.data());
    assert(!result);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
      m_surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else {
      assert(formatCount >= 1);
      m_surfaceFormat = surfFormats[0].format;
    }
    m_surfaceColor = surfFormats[0].colorSpace;
  }

  void SwapChain::update(int width, int height, bool vsync)
  {
    m_incarnation++;

    VkResult err;
    VkSwapchainKHR oldSwapchain = m_swapchain;

    // Check the surface capabilities and formats
    VkSurfaceCapabilitiesKHR surfCapabilities;
    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      m_physicalDevice, m_surface, &surfCapabilities);
    assert(!err);

    uint32_t presentModeCount;
    err = vkGetPhysicalDeviceSurfacePresentModesKHR(
      m_physicalDevice, m_surface, &presentModeCount, nullptr);
    assert(!err);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    err = vkGetPhysicalDeviceSurfacePresentModesKHR(
      m_physicalDevice, m_surface, &presentModeCount, presentModes.data());
    assert(!err);

    VkExtent2D swapchainExtent;
    // width and height are either both -1, or both not -1.
    if (surfCapabilities.currentExtent.width == (uint32_t)-1) {
      // If the surface size is undefined, the size is set to
      // the size of the images requested.
      swapchainExtent.width = width;
      swapchainExtent.height = height;
    }
    else {
      // If the surface size is defined, the swap chain size must match
      swapchainExtent = surfCapabilities.currentExtent;

      //demo->width = surfCapabilities.currentExtent.width;
      //demo->height = surfCapabilities.currentExtent.height;
    }

    // everyone must support FIFO mode
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    // no vsync try to find a faster alternative to FIFO
    if (!vsync){
      for (uint32_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
          // prefer mailbox due to no tearing
          swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
          break;
        }
        if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
          swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
      }
    }

    // Determine the number of VkImage's to use in the swap chain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint32_t desiredNumberOfSwapchainImages =
      surfCapabilities.minImageCount + 1;
    if ((surfCapabilities.maxImageCount > 0) &&
      (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount)) {
      // Application must settle for fewer images than desired:
      desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfCapabilities.supportedTransforms &
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
      preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
      preTransform = surfCapabilities.currentTransform;
    }

    VkSwapchainCreateInfoKHR swapchain = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain.surface = m_surface;
    swapchain.minImageCount = desiredNumberOfSwapchainImages;
    swapchain.imageFormat = m_surfaceFormat;
    swapchain.imageColorSpace = m_surfaceColor;
    swapchain.imageExtent = swapchainExtent;
    swapchain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain.preTransform = preTransform;
    swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain.imageArrayLayers = 1;
    swapchain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain.queueFamilyIndexCount = 1;
    swapchain.pQueueFamilyIndices = &m_queueFamilyIndex;
    swapchain.presentMode = swapchainPresentMode;
    swapchain.oldSwapchain = oldSwapchain;
    swapchain.clipped = true;

    err = vkCreateSwapchainKHR(m_device, &swapchain, m_allocator,
      &m_swapchain);
    assert(!err);

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if (oldSwapchain != VK_NULL_HANDLE) {
      for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
        vkDestroyImageView(m_device, m_swapchainImageViews[i], m_allocator);
      }
      vkDestroySwapchainKHR(m_device, oldSwapchain, m_allocator);
    }

    err = vkGetSwapchainImagesKHR(m_device, m_swapchain,
      &m_swapchainImageCount, nullptr);
    assert(!err);

    m_swapchainImages.resize(m_swapchainImageCount);

    err = vkGetSwapchainImagesKHR(m_device, m_swapchain,
      &m_swapchainImageCount,
      m_swapchainImages.data());
    assert(!err);
    //
    // Image views
    //
    m_swapchainImageViews.resize(m_swapchainImageCount);
    for (uint32_t i = 0; i < m_swapchainImageCount; i++) {
        VkImageViewCreateInfo ci =
        {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            NULL,
            0,
            m_swapchainImages[i],
            VK_IMAGE_VIEW_TYPE_2D,
            m_surfaceFormat,
            { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
            { VK_IMAGE_ASPECT_COLOR_BIT, 0,1,0,1 }
        };
        err = vkCreateImageView(m_device, &ci, m_allocator, &m_swapchainImageViews[i]);
    }


    bool deleteOld = !m_swapchainSemaphores.empty();
    m_swapchainSemaphores.resize(m_swapchainImageCount * 2, VK_NULL_HANDLE);
    
    for (uint32_t i = 0; i < m_swapchainImageCount * 2; i++) {
      VkSemaphoreCreateInfo semCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
      if (deleteOld) {
        vkDestroySemaphore(m_device, m_swapchainSemaphores[i], m_allocator);
      }
      err = vkCreateSemaphore(m_device, &semCreateInfo, m_allocator, &m_swapchainSemaphores[i]);
      assert(!err);
    }

    m_swapchainBarriers.resize(m_swapchainImageCount);
    for (uint32_t i = 0; i < m_swapchainImageCount; i++) {
      VkImageSubresourceRange range = { 0 };
      range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      range.baseMipLevel = 0;
      range.levelCount = VK_REMAINING_MIP_LEVELS;
      range.baseArrayLayer = 0;
      range.layerCount = VK_REMAINING_ARRAY_LAYERS;

      VkImageMemoryBarrier memBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
      memBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      memBarrier.dstAccessMask = 0;
      memBarrier.srcAccessMask = 0;
      memBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      memBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      memBarrier.image = m_swapchainImages[i];
      memBarrier.subresourceRange = range;

      m_swapchainBarriers[i] = memBarrier;
    }

    m_width = width;
    m_height = height;
    m_vsync = vsync;

    m_currentSemaphore = 0;
    m_currentImage = 0;
  }

  void SwapChain::deinitResources()
  {
    for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
      vkDestroyImageView(m_device, m_swapchainImageViews[i], m_allocator);
    }

    vkDestroySwapchainKHR(m_device, m_swapchain, m_allocator);

    for (size_t i = 0; i < m_swapchainSemaphores.size(); i++) {
      vkDestroySemaphore(m_device, m_swapchainSemaphores[i], m_allocator);
    }

    m_swapchain = VK_NULL_HANDLE;
    m_swapchainSemaphores.clear();
    m_swapchainBarriers.clear();
  }

  void SwapChain::deinit()
  {
    deinitResources();
    
    m_physicalDevice = 0;
    m_device = 0;
    m_surface = 0;
    m_incarnation = 0;
  }

  bool SwapChain::acquire()
  {
    // try recreation a few times
    for (int i = 0; i < 2; i++){
      VkSemaphore semaphore = getActiveReadSemaphore();
      VkResult result;
      result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
        semaphore,
        (VkFence)0,
        &m_currentImage);

      if (result == VK_SUCCESS) {
        return true;
      }
      else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        vkDeviceWaitIdle(m_device);

        deinitResources();
        update(m_width, m_height, m_vsync);
      }
      else {
        return false;
      }
    }

    return false;
  }

  VkSemaphore SwapChain::getActiveWrittenSemaphore() const
  {
    return m_swapchainSemaphores[(m_currentSemaphore % m_swapchainImageCount) * 2 + 1];
  }

  VkSemaphore SwapChain::getActiveReadSemaphore() const
  {
    return m_swapchainSemaphores[(m_currentSemaphore % m_swapchainImageCount) * 2 + 0];
  }

  VkImage SwapChain::getActiveImage() const
  {
    return m_swapchainImages[m_currentImage];
  }

  VkImageView SwapChain::getActiveImageView() const
  {
      return m_swapchainImageViews[m_currentImage];
  }

  VkImage SwapChain::getImage(uint32_t i) const
  {
    if (i >= m_swapchainImageCount) return nullptr; return m_swapchainImages[i];
  }

  int SwapChain::getActiveImageIndex() const
  {
    return m_currentImage;
  }

  void SwapChain::present(VkQueue queue)
  {
    VkResult result;
    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    VkSemaphore written = getActiveWrittenSemaphore();
    presentInfo.swapchainCount = 1;
    if (written) {
      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores = &written;
    }
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_currentImage;

    result = vkQueuePresentKHR(queue, &presentInfo);
    //assert(result == VK_SUCCESS); // can fail on application exit

    m_currentSemaphore++;
  }

  VkFormat SwapChain::getFormat() const
  {
    return m_surfaceFormat;
  }

  uint32_t SwapChain::getImageCount() const
  {
    return m_swapchainImageCount;
  }

  void SwapChain::cmdUpdateBarriers(VkCommandBuffer cmd) const
  {
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
      0, NULL, 0, NULL, getImageCount(), getImageMemoryBarriers());
  }

  void SwapChain::getActiveBarrier(VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageMemoryBarrier& barrier) const
  {
    barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = getActiveImage();
    barrier.subresourceRange = { 0 };
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
  }

  uint32_t SwapChain::getIncarnation() const
  {
    return m_incarnation;
  }

  const VkImageMemoryBarrier* SwapChain::getImageMemoryBarriers() const
  {
    return m_swapchainBarriers.data();
  }

  VkImageView SwapChain::getImageView(uint32_t i) const
  {
    if (i >= m_swapchainImageCount) return nullptr; return m_swapchainImageViews[i];
  }

}

