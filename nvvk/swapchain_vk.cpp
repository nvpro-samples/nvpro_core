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


#include "swapchain_vk.hpp"
#include "error_vk.hpp"

#include <assert.h>

#include <nvvk/debug_util_vk.hpp>
namespace nvvk {
bool SwapChain::init(VkDevice          device,
                     VkPhysicalDevice  physicalDevice,
                     VkQueue           queue,
                     uint32_t          queueFamilyIndex,
                     VkSurfaceKHR      surface,
                     VkFormat          format,
                     VkImageUsageFlags imageUsage)
{
  assert(!m_device);
  m_device           = device;
  m_physicalDevice   = physicalDevice;
  m_swapchain        = VK_NULL_HANDLE;
  m_queue            = queue;
  m_queueFamilyIndex = queueFamilyIndex;
  m_changeID         = 0;
  m_currentSemaphore = 0;
  m_surface          = surface;
  m_imageUsage       = imageUsage;

  VkResult result;

  // Get the list of VkFormat's that are supported:
  uint32_t formatCount;
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
  assert(!result);

  std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, surfFormats.data());
  assert(!result);
  // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
  // the surface has no preferred format.  Otherwise, at least one
  // supported format will be returned.

  m_surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
  m_surfaceColor  = surfFormats[0].colorSpace;

  for(uint32_t i = 0; i < formatCount; i++)
  {
    if(surfFormats[i].format == format)
    {
      m_surfaceFormat = format;
      m_surfaceColor  = surfFormats[i].colorSpace;
      return true;
    }
  }

  return false;
}

VkExtent2D SwapChain::update(int width, int height, bool vsync)
{
  m_changeID++;

  VkResult       err;
  VkSwapchainKHR oldSwapchain = m_swapchain;

  err = waitIdle();
  if(nvvk::checkResult(err, __FILE__, __LINE__))
  {
    exit(-1);
  }
  // Check the surface capabilities and formats
  VkSurfaceCapabilitiesKHR surfCapabilities;
  err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfCapabilities);
  assert(!err);

  uint32_t presentModeCount;
  err = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
  assert(!err);
  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  err = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());
  assert(!err);

  VkExtent2D swapchainExtent;
  // width and height are either both -1, or both not -1.
  if(surfCapabilities.currentExtent.width == (uint32_t)-1)
  {
    // If the surface size is undefined, the size is set to
    // the size of the images requested.
    swapchainExtent.width  = width;
    swapchainExtent.height = height;
  }
  else
  {
    // If the surface size is defined, the swap chain size must match
    swapchainExtent = surfCapabilities.currentExtent;
  }

  // test against valid size, typically hit when windows are minimized, the app must
  // prevent triggering this code accordingly
  assert(swapchainExtent.width && swapchainExtent.height);

  // everyone must support FIFO mode
  VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  // no vsync try to find a faster alternative to FIFO
  if(!vsync)
  {
    for(uint32_t i = 0; i < presentModeCount; i++)
    {
      if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      }
      if(presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
      {
        swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      }
      if (swapchainPresentMode == m_preferredVsyncOffMode)
      {
        break;
      }
    }
  }

  // Determine the number of VkImage's to use in the swap chain (we desire to
  // own only 1 image at a time, besides the images being displayed and
  // queued for display):
  uint32_t desiredNumberOfSwapchainImages = surfCapabilities.minImageCount + 1;
  if((surfCapabilities.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount))
  {
    // Application must settle for fewer images than desired:
    desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
  }

  VkSurfaceTransformFlagBitsKHR preTransform;
  if(surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
  {
    preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  }
  else
  {
    preTransform = surfCapabilities.currentTransform;
  }

  VkSwapchainCreateInfoKHR swapchain = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchain.surface                  = m_surface;
  swapchain.minImageCount            = desiredNumberOfSwapchainImages;
  swapchain.imageFormat              = m_surfaceFormat;
  swapchain.imageColorSpace          = m_surfaceColor;
  swapchain.imageExtent              = swapchainExtent;
  swapchain.imageUsage               = m_imageUsage;
  swapchain.preTransform             = preTransform;
  swapchain.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain.imageArrayLayers         = 1;
  swapchain.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
  swapchain.queueFamilyIndexCount    = 1;
  swapchain.pQueueFamilyIndices      = &m_queueFamilyIndex;
  swapchain.presentMode              = swapchainPresentMode;
  swapchain.oldSwapchain             = oldSwapchain;
  swapchain.clipped                  = true;

  err = vkCreateSwapchainKHR(m_device, &swapchain, nullptr, &m_swapchain);
  assert(!err);

  nvvk::DebugUtil debugUtil(m_device);

  debugUtil.setObjectName(m_swapchain, "SwapChain::m_swapchain");

  // If we just re-created an existing swapchain, we should destroy the old
  // swapchain at this point.
  // Note: destroying the swapchain also cleans up all its associated
  // presentable images once the platform is done with them.
  if(oldSwapchain != VK_NULL_HANDLE)
  {
    for(auto it : m_entries)
    {
      vkDestroyImageView(m_device, it.imageView, nullptr);
      vkDestroySemaphore(m_device, it.readSemaphore, nullptr);
      vkDestroySemaphore(m_device, it.writtenSemaphore, nullptr);
    }
    vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);
  }

  err = vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, nullptr);
  assert(!err);

  m_entries.resize(m_imageCount);
  m_barriers.resize(m_imageCount);

  std::vector<VkImage> images(m_imageCount);

  err = vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, images.data());
  assert(!err);
  //
  // Image views
  //
  for(uint32_t i = 0; i < m_imageCount; i++)
  {
    Entry& entry = m_entries[i];

    // image
    entry.image = images[i];

    // imageview
    VkImageViewCreateInfo viewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                            nullptr,
                                            0,
                                            entry.image,
                                            VK_IMAGE_VIEW_TYPE_2D,
                                            m_surfaceFormat,
                                            {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
                                            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    err = vkCreateImageView(m_device, &viewCreateInfo, nullptr, &entry.imageView);
    assert(!err);

    // semaphore
    VkSemaphoreCreateInfo semCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    err = vkCreateSemaphore(m_device, &semCreateInfo, nullptr, &entry.readSemaphore);
    assert(!err);
    err = vkCreateSemaphore(m_device, &semCreateInfo, nullptr, &entry.writtenSemaphore);
    assert(!err);

    // initial barriers
    VkImageSubresourceRange range = {0};
    range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel            = 0;
    range.levelCount              = VK_REMAINING_MIP_LEVELS;
    range.baseArrayLayer          = 0;
    range.layerCount              = VK_REMAINING_ARRAY_LAYERS;

    VkImageMemoryBarrier memBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    memBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memBarrier.dstAccessMask        = 0;
    memBarrier.srcAccessMask        = 0;
    memBarrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
    memBarrier.newLayout            = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    memBarrier.image                = entry.image;
    memBarrier.subresourceRange     = range;

    m_barriers[i] = memBarrier;

    debugUtil.setObjectName(entry.image, "swapchainImage:" + std::to_string(i));
    debugUtil.setObjectName(entry.imageView, "swapchainImageView:" + std::to_string(i));
    debugUtil.setObjectName(entry.readSemaphore, "swapchainReadSemaphore:" + std::to_string(i));
    debugUtil.setObjectName(entry.writtenSemaphore, "swapchainWrittenSemaphore:" + std::to_string(i));
  }


  m_updateWidth  = width;
  m_updateHeight = height;
  m_vsync        = vsync;
  m_extent       = swapchainExtent;

  m_currentSemaphore = 0;
  m_currentImage     = 0;

  return swapchainExtent;
}

void SwapChain::deinitResources()
{
  if(!m_device)
    return;

  VkResult result = waitIdle();
  if(nvvk::checkResult(result, __FILE__, __LINE__))
  {
    exit(-1);
  }

  for(auto it : m_entries)
  {
    vkDestroyImageView(m_device, it.imageView, nullptr);
    vkDestroySemaphore(m_device, it.readSemaphore, nullptr);
    vkDestroySemaphore(m_device, it.writtenSemaphore, nullptr);
  }

  if(m_swapchain)
  {
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
  }

  m_entries.clear();
  m_barriers.clear();
}

void SwapChain::deinit()
{
  deinitResources();

  m_physicalDevice = VK_NULL_HANDLE;
  m_device         = VK_NULL_HANDLE;
  m_surface        = VK_NULL_HANDLE;
  m_changeID       = 0;
}

bool SwapChain::acquire(bool* pRecreated, SwapChainAcquireState* pOut)
{
  return acquireCustom(VK_NULL_HANDLE, m_updateWidth, m_updateHeight, pRecreated, pOut);
}

bool SwapChain::acquireAutoResize(int width, int height, bool* pRecreated, SwapChainAcquireState* pOut)
{
  return acquireCustom(VK_NULL_HANDLE, width, height, pRecreated, pOut);
}

bool SwapChain::acquireCustom(VkSemaphore argSemaphore, bool* pRecreated, SwapChainAcquireState* pOut)
{
  return acquireCustom(argSemaphore, m_updateWidth, m_updateHeight, pRecreated, pOut);
}

bool SwapChain::acquireCustom(VkSemaphore argSemaphore, int width, int height, bool* pRecreated, SwapChainAcquireState* pOut)
{
  bool didRecreate = false;

  if(width != m_updateWidth || height != m_updateHeight)
  {
    deinitResources();
    update(width, height);
    m_updateWidth  = width;
    m_updateHeight = height;
    didRecreate    = true;
  }
  if(pRecreated != nullptr)
  {
    *pRecreated = didRecreate;
  }

  // try recreation a few times
  for(int i = 0; i < 2; i++)
  {
    VkSemaphore semaphore = argSemaphore ? argSemaphore : getActiveReadSemaphore();
    VkResult    result;
    result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, semaphore, (VkFence)VK_NULL_HANDLE, &m_currentImage);

    if(result == VK_SUCCESS)
    {
      if(pOut != nullptr)
      {
        pOut->image     = getActiveImage();
        pOut->view      = getActiveImageView();
        pOut->index     = getActiveImageIndex();
        pOut->waitSem   = getActiveReadSemaphore();
        pOut->signalSem = getActiveWrittenSemaphore();
      }
      return true;
    }
    else if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
      deinitResources();
      update(width, height, m_vsync);
    }
    else
    {
      return false;
    }
  }

  return false;
}

VkSemaphore SwapChain::getActiveWrittenSemaphore() const
{
  return m_entries[(m_currentSemaphore % m_imageCount)].writtenSemaphore;
}

VkSemaphore SwapChain::getActiveReadSemaphore() const
{
  return m_entries[(m_currentSemaphore % m_imageCount)].readSemaphore;
}

VkImage SwapChain::getActiveImage() const
{
  return m_entries[m_currentImage].image;
}

VkImageView SwapChain::getActiveImageView() const
{
  return m_entries[m_currentImage].imageView;
}

VkImage SwapChain::getImage(uint32_t i) const
{
  if(i >= m_imageCount)
    return nullptr;
  return m_entries[i].image;
}

void SwapChain::present(VkQueue queue)
{
  VkResult         result;
  VkPresentInfoKHR presentInfo;

  presentCustom(presentInfo);

  result = vkQueuePresentKHR(queue, &presentInfo);
  //assert(result == VK_SUCCESS); // can fail on application exit
}

void SwapChain::presentCustom(VkPresentInfoKHR& presentInfo)
{
  VkSemaphore& written = m_entries[(m_currentSemaphore % m_imageCount)].writtenSemaphore;

  presentInfo                    = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.swapchainCount     = 1;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = &written;
  presentInfo.pSwapchains        = &m_swapchain;
  presentInfo.pImageIndices      = &m_currentImage;

  m_currentSemaphore++;
}

void SwapChain::cmdUpdateBarriers(VkCommandBuffer cmd) const
{
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0,
                       nullptr, m_imageCount, m_barriers.data());
}

uint32_t SwapChain::getChangeID() const
{
  return m_changeID;
}

VkImageView SwapChain::getImageView(uint32_t i) const
{
  if(i >= m_imageCount)
    return nullptr;
  return m_entries[i].imageView;
}

}  // namespace nvvk
