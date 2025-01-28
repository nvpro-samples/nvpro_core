/*
 * Copyright (c) 2022-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "app_swapchain_vk.hpp"
#include "nvvk/error_vk.hpp"
#include "nvvk/images_vk.hpp"

nvvkhl::AppSwapchain::~AppSwapchain()
{
  assert(m_swapChain == VK_NULL_HANDLE && "Missing deinit()");
}

void nvvkhl::AppSwapchain::requestRebuild()
{
  m_needRebuild = true;
}

bool nvvkhl::AppSwapchain::needRebuilding() const
{
  return m_needRebuild;
}

VkImage nvvkhl::AppSwapchain::getNextImage() const
{
  return m_nextImages[m_nextImageIndex].image;
}

VkImageView nvvkhl::AppSwapchain::getNextImageView() const
{
  return m_nextImages[m_nextImageIndex].imageView;
}

VkFormat nvvkhl::AppSwapchain::getImageFormat() const
{
  return m_imageFormat;
}

uint32_t nvvkhl::AppSwapchain::getMaxFramesInFlight() const
{
  return m_maxFramesInFlight;
}

VkSemaphore nvvkhl::AppSwapchain::getImageAvailableSemaphore() const
{
  return m_frameResources[m_currentFrame].imageAvailableSemaphore;
}

VkSemaphore nvvkhl::AppSwapchain::getRenderFinishedSemaphore() const
{
  return m_frameResources[m_currentFrame].renderFinishedSemaphore;
}

void nvvkhl::AppSwapchain::init(VkPhysicalDevice physicalDevice, VkDevice device, const nvvkhl::QueueInfo& queue, VkSurfaceKHR surface, VkCommandPool cmdPool)
{
  m_physicalDevice = physicalDevice;
  m_device         = device;
  m_queue          = queue;
  m_surface        = surface;
  m_cmdPool        = cmdPool;
}

void nvvkhl::AppSwapchain::deinit()
{
  deinitResources();
  *this = {};
}

VkExtent2D nvvkhl::AppSwapchain::initResources(bool vSync /*= true*/)
{
  VkExtent2D outWindowSize;

  // Query the physical device's capabilities for the given surface.
  const VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo2{.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
                                                     .surface = m_surface};
  VkSurfaceCapabilities2KHR             capabilities2{.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
  vkGetPhysicalDeviceSurfaceCapabilities2KHR(m_physicalDevice, &surfaceInfo2, &capabilities2);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormats2KHR(m_physicalDevice, &surfaceInfo2, &formatCount, nullptr);
  std::vector<VkSurfaceFormat2KHR> formats(formatCount, {.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR});
  vkGetPhysicalDeviceSurfaceFormats2KHR(m_physicalDevice, &surfaceInfo2, &formatCount, formats.data());

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());

  // Choose the best available surface format and present mode
  const VkSurfaceFormat2KHR surfaceFormat2 = selectSwapSurfaceFormat(formats);
  const VkPresentModeKHR    presentMode    = selectSwapPresentMode(presentModes, vSync);
  // Set the window size according to the surface's current extent
  outWindowSize = capabilities2.surfaceCapabilities.currentExtent;
  // Set the number of images in flight, respecting GPU limitations
  m_maxFramesInFlight = std::min(m_maxFramesInFlight, capabilities2.surfaceCapabilities.maxImageCount);
  // Store the chosen image format
  m_imageFormat = surfaceFormat2.surfaceFormat.format;

  // Create the swapchain itself
  const VkSwapchainCreateInfoKHR swapchainCreateInfo{
      .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface          = m_surface,
      .minImageCount    = m_maxFramesInFlight,
      .imageFormat      = surfaceFormat2.surfaceFormat.format,
      .imageColorSpace  = surfaceFormat2.surfaceFormat.colorSpace,
      .imageExtent      = capabilities2.surfaceCapabilities.currentExtent,  // Window size set here
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform     = capabilities2.surfaceCapabilities.currentTransform,
      .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode      = presentMode,
      .clipped          = VK_TRUE,
  };
  NVVK_CHECK(vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapChain));
  //DBG_VK_NAME(m_swapChain);

  // Retrieve the swapchain images
  uint32_t imageCount;
  vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
  assert(m_maxFramesInFlight == imageCount && "Wrong swapchain setup");
  std::vector<VkImage> swapImages(imageCount);
  vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, swapImages.data());

  // Store the swapchain images and create views for them
  m_nextImages.resize(m_maxFramesInFlight);
  VkImageViewCreateInfo imageViewCreateInfo{
      .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format   = m_imageFormat,
      .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY},
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1},
  };
  for(uint32_t i = 0; i < m_maxFramesInFlight; i++)
  {
    m_nextImages[i].image = swapImages[i];
    //DBG_VK_NAME(m_nextImages[i].image);
    imageViewCreateInfo.image = m_nextImages[i].image;
    NVVK_CHECK(vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &m_nextImages[i].imageView));
    //DBG_VK_NAME(m_nextImages[i].imageView);
  }

  // Initialize frame resources for each frame
  m_frameResources.resize(m_maxFramesInFlight);
  for(size_t i = 0; i < m_maxFramesInFlight; ++i)
  {
    /*--
       * The sync objects are used to synchronize the rendering with the presentation.
       * The image available semaphore is signaled when the image is available to render.
       * The render finished semaphore is signaled when the rendering is finished.
       * The in flight fence is signaled when the frame is in flight.
      -*/
    const VkSemaphoreCreateInfo semaphoreCreateInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    NVVK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frameResources[i].imageAvailableSemaphore));
    //DBG_VK_NAME(m_frameResources[i].imageAvailableSemaphore);
    NVVK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frameResources[i].renderFinishedSemaphore));
    //DBG_VK_NAME(m_frameResources[i].renderFinishedSemaphore);
  }

  // Transition images to present layout
  {
    VkCommandBuffer cmd = nvvkhl::beginSingleTimeCommands(m_device, m_cmdPool);
    for(uint32_t i = 0; i < m_maxFramesInFlight; i++)
    {
      nvvk::cmdBarrierImageLayout(cmd, m_nextImages[i].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
    nvvkhl::endSingleTimeCommands(cmd, m_device, m_cmdPool, m_queue.queue);
  }

  return outWindowSize;
}

VkExtent2D nvvkhl::AppSwapchain::reinitResources(bool vSync /*= true*/)
{
  // Wait for all frames to finish rendering before recreating the swapchain
  vkQueueWaitIdle(m_queue.queue);

  m_currentFrame = 0;
  m_needRebuild  = false;
  deinitResources();
  return initResources(vSync);
}

void nvvkhl::AppSwapchain::deinitResources()
{
  vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
  for(FrameResources& frameRes : m_frameResources)
  {
    vkDestroySemaphore(m_device, frameRes.imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(m_device, frameRes.renderFinishedSemaphore, nullptr);
  }
  for(AppSwapchain::Image& image : m_nextImages)
  {
    vkDestroyImageView(m_device, image.imageView, nullptr);
  }
  m_frameResources = {};
  m_nextImages     = {};
}

void nvvkhl::AppSwapchain::acquireNextImage(VkDevice device)
{
  assert(m_needRebuild == false);  //, "Swapbuffer need to call reinitResources()");

  FrameResources& frame = m_frameResources[m_currentFrame];

  // Acquire the next image from the swapchain
  const VkResult result = vkAcquireNextImageKHR(device, m_swapChain, std::numeric_limits<uint64_t>::max(),
                                                frame.imageAvailableSemaphore, VK_NULL_HANDLE, &m_nextImageIndex);
  // Handle special case if the swapchain is out of date (e.g., window resize)
  if(result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    m_needRebuild = true;  // Swapchain must be rebuilt on the next frame
  }
  else
  {
    assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);  //, "Couldn't acquire swapchain image");
  }
}

void nvvkhl::AppSwapchain::presentFrame(VkQueue queue)
{
  FrameResources& frame = m_frameResources[m_currentFrame];

  // Setup the presentation info, linking the swapchain and the image index
  const VkPresentInfoKHR presentInfo{
      .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,                               // Wait for rendering to finish
      .pWaitSemaphores    = &frame.renderFinishedSemaphore,  // Synchronize presentation
      .swapchainCount     = 1,                               // Swapchain to present the image
      .pSwapchains        = &m_swapChain,                    // Pointer to the swapchain
      .pImageIndices      = &m_nextImageIndex,               // Index of the image to present
  };

  // Present the image and handle potential resizing issues
  const VkResult result = vkQueuePresentKHR(queue, &presentInfo);
  // If the swapchain is out of date (e.g., window resized), it needs to be rebuilt
  if(result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    m_needRebuild = true;
  }
  else
  {
    assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);  //, "Couldn't present swapchain image");
  }

  // Advance to the next frame in the swapchain
  m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;
}

VkSurfaceFormat2KHR nvvkhl::AppSwapchain::selectSwapSurfaceFormat(const std::vector<VkSurfaceFormat2KHR>& availableFormats) const
{
  // If there's only one available format and it's undefined, return a default format.
  if(availableFormats.size() == 1 && availableFormats[0].surfaceFormat.format == VK_FORMAT_UNDEFINED)
  {
    VkSurfaceFormat2KHR result{.sType         = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR,
                               .surfaceFormat = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}};
    return result;
  }

  const std::vector<VkSurfaceFormat2KHR> preferredFormats = {
      VkSurfaceFormat2KHR{.sType         = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR,
                          .surfaceFormat = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}},
      VkSurfaceFormat2KHR{.sType         = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR,
                          .surfaceFormat = {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}}};

  // Check available formats against the preferred formats.
  for(const VkSurfaceFormat2KHR& preferredFormat : preferredFormats)
  {
    for(const VkSurfaceFormat2KHR& availableFormat : availableFormats)
    {
      if(availableFormat.surfaceFormat.format == preferredFormat.surfaceFormat.format
         && availableFormat.surfaceFormat.colorSpace == preferredFormat.surfaceFormat.colorSpace)
      {
        return availableFormat;  // Return the first matching preferred format.
      }
    }
  }

  // If none of the preferred formats are available, return the first available format.
  return availableFormats[0];
}

VkPresentModeKHR nvvkhl::AppSwapchain::selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes,
                                                             bool                                 vSync /*= true*/)
{
  if(vSync)
  {
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  bool mailboxSupported = false, immediateSupported = false;

  for(VkPresentModeKHR mode : availablePresentModes)
  {
    if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
      mailboxSupported = true;
    if(mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
      immediateSupported = true;
  }

  if(mailboxSupported)
  {
    return VK_PRESENT_MODE_MAILBOX_KHR;
  }

  if(immediateSupported)
  {
    return VK_PRESENT_MODE_IMMEDIATE_KHR;  // Best mode for low latency
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}
