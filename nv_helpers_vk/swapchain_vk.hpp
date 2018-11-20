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
#include <vector>
#include <string>
#include <vulkan/vulkan.h>

#include "base_vk.hpp"

namespace nv_helpers_vk
{
  class SwapChain {
  private:
    VkDevice                      m_device;
    VkPhysicalDevice              m_physicalDevice;
    const VkAllocationCallbacks*  m_allocator;

    uint32_t                      m_queueFamilyIndex;

    VkSurfaceKHR                  m_surface;
    VkFormat                      m_surfaceFormat;
    VkColorSpaceKHR               m_surfaceColor;

    uint32_t                      m_swapchainImageCount;
    VkSwapchainKHR                m_swapchain;

    std::vector<VkImage>                m_swapchainImages;
    std::vector<VkImageView>            m_swapchainImageViews;
    std::vector<VkSemaphore>            m_swapchainSemaphores;
    std::vector<VkImageMemoryBarrier>   m_swapchainBarriers;

    uint32_t                      m_currentImage;
    uint32_t                      m_currentSemaphore;
    uint32_t                      m_incarnation;
    uint32_t                      m_width;
    uint32_t                      m_height;
    bool                          m_vsync;

    void  deinitResources();

  public:

    void  init(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t queueFamilyIndex, const VkAllocationCallbacks* allocator=nullptr);
    void  update(int width, int height, bool vsync);
    void  deinit();

    // returns true on success
    bool        acquire();
    void        present(VkQueue queue);

    VkFormat    getFormat() const;    
    VkSemaphore getActiveReadSemaphore() const;
    VkSemaphore getActiveWrittenSemaphore() const;
    VkImage     getActiveImage() const;
    VkImageView getActiveImageView() const;
    VkImage     getImage(uint32_t i) const;
    VkImageView getImageView(uint32_t i) const;
    int         getActiveImageIndex() const;
    int         getSwapchainImageCount() const { return m_swapchainImageCount; }

    // from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    // must apply resource transitions after update calls
    const VkImageMemoryBarrier* getImageMemoryBarriers() const;
    uint32_t    getImageCount() const;

    uint32_t    getIncarnation() const;

    VkSwapchainKHR getSwapchain() const { return m_swapchain; }
  };
}

#endif
