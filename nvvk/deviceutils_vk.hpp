/* Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
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

#pragma  once

#include <assert.h>
#include <platform.h>
#include <vector>
#include <vulkan/vulkan.h>

namespace nvvk
{

  //////////////////////////////////////////////////////////////////////////
  // Utilities around a Device
  // you can use this class as a local wrapper for utilities
  struct DeviceUtils
  {
    // FIXME remove Makers
    DeviceUtils()
        : m_device(VK_NULL_HANDLE)
        , m_allocator(nullptr)
    {
    }
    DeviceUtils(VkDevice device, const VkAllocationCallbacks* allocator = nullptr)
        : m_device(device)
        , m_allocator(allocator)
    {
    }

    operator VkDevice() { return m_device; }

    VkDevice                     m_device;
    const VkAllocationCallbacks* m_allocator;

    VkResult allocMemAndBindBuffer(VkBuffer                                obj,
                                   const VkPhysicalDeviceMemoryProperties& memoryProperties,
                                   VkDeviceMemory&                         gpuMem,
                                   VkMemoryPropertyFlags                   memProps);
    VkResult allocMemAndBindBuffer(VkBuffer obj, VkPhysicalDevice physicalDevice, VkDeviceMemory& gpuMem, VkMemoryPropertyFlags memProps);

    VkBuffer     createBuffer(size_t size, VkBufferUsageFlags usage, VkBufferCreateFlags flags = 0) const;
    VkBufferView createBufferView(VkBuffer buffer, VkFormat format, VkDeviceSize size, VkDeviceSize offset = 0, VkBufferViewCreateFlags flags = 0) const;
    VkBufferView createBufferView(VkDescriptorBufferInfo info, VkFormat format, VkBufferViewCreateFlags flags = 0) const;

    VkDescriptorSetLayout createDescriptorSetLayout(size_t                              numBindings,
                                                    const VkDescriptorSetLayoutBinding* bindings,
                                                    VkDescriptorSetLayoutCreateFlags    flags = 0) const;
    VkPipelineLayout      createPipelineLayout(size_t                       numLayouts,
                                               const VkDescriptorSetLayout* setLayouts,
                                               size_t                       numRanges = 0,
                                               const VkPushConstantRange*   ranges    = nullptr,
                                               VkPipelineLayoutCreateFlags  flags = 0) const;

    void           createDescriptorPoolAndSets(uint32_t                    maxSets,
                                               size_t                      poolSizeCount,
                                               const VkDescriptorPoolSize* poolSizes,
                                               VkDescriptorSetLayout       layout,
                                               VkDescriptorPool&           pool,
                                               VkDescriptorSet*            sets) const;
    VkShaderModule createShaderModule(const char* binarycode, size_t size);
    VkShaderModule createShaderModule(const std::vector<char>& code);
  };

  //////////////////////////////////////////////////////////////////////////
  // This struct is a little more demanding on the constructor.
  // So let's make it "Ex" (extended), as it may not alway be required
  // It basically needs
  // - command-pool
  // - command-Queue and related family
  // - physical device : all of this to execute commands for transitions and memory copies
  struct DeviceUtilsEx : DeviceUtils
  {
    DeviceUtilsEx() {}
    DeviceUtilsEx(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, uint32_t queueFamilyIndex, const VkAllocationCallbacks* allocator = nullptr)
    {
      init(device, physicalDevice, queue, queueFamilyIndex, allocator);
    }
    void init(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, uint32_t queueFamilyIndex, const VkAllocationCallbacks* allocator = nullptr);

    void CreateCommandPool();

    ~DeviceUtilsEx();

	  void            deInit();
    VkCommandBuffer beginSingleTimeCommands();
    void            endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void        createTextureImage(uint8_t* pixels, int texWidth, int texHeight, int texChannels, VkImage& textureImage, VkDeviceMemory& textureImageMemory);
    void        copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void        createImage(uint32_t              width,
                            uint32_t              height,
                            VkSampleCountFlagBits samples,
                            VkFormat              format,
                            VkImageTiling         tiling,
                            VkImageUsageFlags     usage,
                            VkMemoryPropertyFlags properties,
                            VkImage&              image,
                            VkDeviceMemory&       imageMemory);
    void        transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    uint32_t    findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkPhysicalDevice m_physicalDevice   = VK_NULL_HANDLE;
    VkCommandPool    m_commandPool      = VK_NULL_HANDLE;
    VkCommandBuffer  m_commandBuffer    = VK_NULL_HANDLE;
    VkQueue          m_queue            = VK_NULL_HANDLE;
    uint32_t         m_queueFamilyIndex = 0;
  };

}
