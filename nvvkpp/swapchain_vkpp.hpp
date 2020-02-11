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

#pragma once


#include <vulkan/vulkan.hpp>

//--------------------------------------------------------------------------------------------------
/**
# nvvkpp::SwapChain

The swapchain class manage the frames to be displayed

Example :
``` C++
nvvk::SwapChain m_swapChain;
- The creation
m_swapChain.setup(m_physicalDevice, m_device, m_queue, m_graphicsQueueIndex, surface, colorFormat);
m_swapChain.create(m_size, vsync);
- On resize
m_swapChain.create(m_size, m_vsync);
m_framebuffers = m_swapChain.createFramebuffers(framebufferCreateInfo);
- Acquire the next image from the swap chain
vk::Result res = m_swapChain.acquire(m_acquireComplete, &m_curFramebuffer);
- Present the frame
vk::Result res = m_swapChain.present(m_curFramebuffer, m_renderComplete);
```
*/

namespace nvvkpp {

struct SwapChain
{
  struct SwapChainImage
  {
    vk::Image     image;
    vk::ImageView view;
  };

  vk::SurfaceKHR              surface;
  vk::SwapchainKHR            swapChain;
  vk::PhysicalDevice          physicalDevice;
  vk::Device                  device;
  vk::Queue                   queue;
  std::vector<SwapChainImage> images;
  vk::Format                  colorFormat{vk::Format::eUndefined};
  vk::ColorSpaceKHR           colorSpace{vk::ColorSpaceKHR::eSrgbNonlinear};
  uint32_t                    imageCount{0};
  uint32_t                    graphicsQueueIndex{VK_QUEUE_FAMILY_IGNORED};


  void init(const vk::PhysicalDevice& newPhysicalDevice,
            const vk::Device&         newDevice,
            const vk::Queue&          newQueue,
            uint32_t                  newGraphicsQueueIndex,
            const vk::SurfaceKHR&     newSurface,
            vk::Format                newColorFormat = vk::Format::eUndefined)
  {
    physicalDevice     = newPhysicalDevice;
    device             = newDevice;
    queue              = newQueue;
    graphicsQueueIndex = newGraphicsQueueIndex;

    surface = newSurface;

    // Get list of supported surface formats
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);

    physicalDevice.getSurfaceSupportKHR(graphicsQueueIndex, surface);

    // We alway presume RGBA8, if not a new color format need to be pass in
    colorFormat = vk::Format::eB8G8R8A8Unorm;
    colorSpace  = surfaceFormats[0].colorSpace;

    // Check if the wished format is supported
    if(newColorFormat != vk::Format::eUndefined)
    {
      for(auto& s : surfaceFormats)
      {
        if(s.format == newColorFormat)
        {
          colorFormat  = s.format;
          s.colorSpace = s.colorSpace;
        }
      }
    }
  }

  void update(vk::Extent2D& size, bool vsync = false)
  {
    if(!physicalDevice || !device || !surface)
    {
      throw std::runtime_error("Initialize the physicalDevice, device, and queue members");
    }
    const vk::SwapchainKHR oldSwapchain = swapChain;

    // Get physical device surface properties and formats
    vk::SurfaceCapabilitiesKHR surfCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    // Get available present modes
    std::vector<vk::PresentModeKHR> presentModes     = physicalDevice.getSurfacePresentModesKHR(surface);
    const auto                      presentModeCount = presentModes.size();

    vk::Extent2D swapchainExtent;
    // width and height are either both -1, or both not -1.
    if(surfCaps.currentExtent.width == -1)
    {
      // If the surface size is undefined, the size is set to
      // the size of the images requested.
      swapchainExtent = size;
    }
    else
    {
      // If the surface size is defined, the swap chain size must match
      swapchainExtent = surfCaps.currentExtent;
      size            = surfCaps.currentExtent;
    }

    // Prefer mailbox mode if present, it's the lowest latency non-tearing present  mode
    vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;

    if(!vsync)
    {
      for(size_t i = 0; i < presentModeCount; i++)
      {
        if(presentModes[i] == vk::PresentModeKHR::eMailbox)
        {
          swapchainPresentMode = vk::PresentModeKHR::eMailbox;
          break;
        }
        if((swapchainPresentMode != vk::PresentModeKHR::eMailbox) && (presentModes[i] == vk::PresentModeKHR::eImmediate))
        {
          swapchainPresentMode = vk::PresentModeKHR::eImmediate;
        }
      }
    }

    // Determine the number of images
    uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
    if((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
    {
      desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
    }

    vk::SurfaceTransformFlagBitsKHR preTransform{};
    if(surfCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
    {
      preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    }
    else
    {
      preTransform = surfCaps.currentTransform;
    }

    vk::SwapchainCreateInfoKHR swapchainCI;
    swapchainCI.setSurface(surface);
    swapchainCI.setMinImageCount(desiredNumberOfSwapchainImages);
    swapchainCI.setImageFormat(colorFormat);
    swapchainCI.setImageColorSpace(colorSpace);
    swapchainCI.setImageExtent({swapchainExtent.width, swapchainExtent.height});
    swapchainCI.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
    swapchainCI.setPreTransform(preTransform);
    swapchainCI.setImageArrayLayers(1);
    swapchainCI.setImageSharingMode(vk::SharingMode::eExclusive);
    swapchainCI.setQueueFamilyIndexCount(0);
    swapchainCI.setPQueueFamilyIndices(nullptr);
    swapchainCI.setPresentMode(swapchainPresentMode);
    swapchainCI.setOldSwapchain(oldSwapchain);
    swapchainCI.setClipped(VK_TRUE);
    swapchainCI.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);

    swapChain = device.createSwapchainKHR(swapchainCI);

    // If an existing swap chain is re-created, destroy the old swap chain
    // This also cleans up all the presentable images
    if(oldSwapchain)
    {
      for(uint32_t i = 0; i < imageCount; i++)
      {
        device.destroyImageView(images[i].view);
      }
      device.destroySwapchainKHR(oldSwapchain);
    }

    vk::ImageViewCreateInfo colorAttachmentView;
    colorAttachmentView.setFormat(colorFormat);
    colorAttachmentView.setViewType(vk::ImageViewType::e2D);
    colorAttachmentView.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    colorAttachmentView.subresourceRange.levelCount = 1;
    colorAttachmentView.subresourceRange.layerCount = 1;

    // Get the swap chain images
    auto swapChainImages = device.getSwapchainImagesKHR(swapChain);
    imageCount           = (uint32_t)swapChainImages.size();

    // Get the swap chain buffers containing the image and imageView
    images.resize(imageCount);
    for(uint32_t i = 0; i < imageCount; i++)
    {
      images[i].image           = swapChainImages[i];
      colorAttachmentView.image = swapChainImages[i];
      images[i].view            = device.createImageView(colorAttachmentView);
    }
  }

  std::vector<vk::Framebuffer> createFramebuffers(vk::FramebufferCreateInfo framebufferCreateInfo)
  {
    // Verify that the first attachment is null
    assert(framebufferCreateInfo.pAttachments[0] == vk::ImageView());

    std::vector<vk::ImageView> attachments;
    attachments.resize(framebufferCreateInfo.attachmentCount);
    for(size_t i = 0; i < framebufferCreateInfo.attachmentCount; ++i)
    {
      attachments[i] = framebufferCreateInfo.pAttachments[i];
    }
    framebufferCreateInfo.pAttachments = attachments.data();

    std::vector<vk::Framebuffer> framebuffers;
    framebuffers.resize(imageCount);
    for(uint32_t i = 0; i < imageCount; i++)
    {
      attachments[0]  = images[i].view;
      framebuffers[i] = device.createFramebuffer(framebufferCreateInfo);
    }
    return framebuffers;
  }

  vk::Result acquire(const vk::Semaphore& presentCompleteSemaphore, uint32_t* imageIndex)
  {
    const vk::Result result = device.acquireNextImageKHR(swapChain, UINT64_MAX, presentCompleteSemaphore, {}, imageIndex);
    if(result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
      throw std::error_code(result);
    }
    return result;
  }

  vk::Result present(uint32_t imageIndex, vk::Semaphore waitSemaphore)
  {
    vk::PresentInfoKHR presentInfo;
    presentInfo.setSwapchainCount(1);
    presentInfo.setPSwapchains(&swapChain);
    presentInfo.setPImageIndices(&imageIndex);
    // Check if a wait semaphore has been specified to wait for before presenting the image
    if(waitSemaphore)
    {
      presentInfo.pWaitSemaphores    = &waitSemaphore;
      presentInfo.waitSemaphoreCount = 1;
    }
    vk::Result res = vk::Result::eSuccess;
    try
    {
      res = queue.presentKHR(presentInfo);
    }
    catch(vk::OutOfDateKHRError&)
    {
      res = vk::Result::eErrorOutOfDateKHR;
    }
    return res;
  }

  void deinit()
  {
    for(uint32_t i = 0; i < imageCount; i++)
    {
      device.destroyImageView(images[i].view);
    }
    if(swapChain)
      device.destroySwapchainKHR(swapChain);
  }
};

}  // namespace nvvkpp
