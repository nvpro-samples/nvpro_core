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

#include "window_vk.hpp"

namespace nv_helpers_vk 
{
  void BasicWindow::initWindow(VkSurfaceKHR surface, uint32_t queueFamily, VkQueueFlags queueFlags, uint32_t queueIndex)
  {
    m_surface = surface;
    m_swapChain.init(m_context.m_device, m_context.m_physicalDevice, surface, queueIndex, m_context.m_allocator);
    m_presentQueueFamily = queueFamily != VK_QUEUE_FAMILY_IGNORED ? queueFamily : m_context.m_physicalInfo.getPresentQueueFamily(surface, VK_QUEUE_GRAPHICS_BIT);
    assert(m_presentQueueFamily != VK_QUEUE_FAMILY_IGNORED);
    vkGetDeviceQueue(m_context.m_device, m_presentQueueFamily, queueIndex, &m_presentQueue);
  }
  
  void BasicWindow::deinitWindow()
  {
    vkDeviceWaitIdle(m_context.m_device);
    m_swapChain.deinit();
    vkDestroySurfaceKHR(m_context.m_instance, m_surface, m_context.m_allocator);
    m_context.deinitContext();
  }
}

