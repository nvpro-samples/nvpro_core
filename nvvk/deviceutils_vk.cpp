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

#include "makers_vk.hpp"
#include "deviceutils_vk.hpp"
#include "physical_vk.hpp"


namespace nvvk 
{

  VkShaderModule DeviceUtils::createShaderModule(const char* binarycode, size_t size)
  {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize                 = size;
    createInfo.pCode                    = (uint32_t*)binarycode;

    VkShaderModule shaderModule;
    if(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
  }

  VkShaderModule DeviceUtils::createShaderModule(const std::vector<char>& code)
  {
    return createShaderModule(code.data(), code.size());
  }


  void DeviceUtils::createDescriptorPoolAndSets(uint32_t                    maxSets,
                                                size_t                      poolSizeCount,
                                                const VkDescriptorPoolSize* poolSizes,
                                                VkDescriptorSetLayout       layout,
                                                VkDescriptorPool&           pool,
                                                VkDescriptorSet*            sets) const
  {
    VkResult result;

    VkDescriptorPool           descrPool;
    VkDescriptorPoolCreateInfo descrPoolInfo = {};
    descrPoolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descrPoolInfo.pNext                      = nullptr;
    descrPoolInfo.maxSets                    = maxSets;
    descrPoolInfo.poolSizeCount              = uint32_t(poolSizeCount);
    descrPoolInfo.pPoolSizes                 = poolSizes;

    // scene pool
    result = vkCreateDescriptorPool(m_device, &descrPoolInfo, m_allocator, &descrPool);
    assert(result == VK_SUCCESS);
    pool = descrPool;

    for(uint32_t n = 0; n < maxSets; n++)
    {
      VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
      allocInfo.descriptorPool              = descrPool;
      allocInfo.descriptorSetCount          = 1;
      allocInfo.pSetLayouts                 = &layout;

      result = vkAllocateDescriptorSets(m_device, &allocInfo, sets + n);
      assert(result == VK_SUCCESS);
    }
  }

  VkResult DeviceUtils::allocMemAndBindBuffer(VkBuffer                                obj,
                                              const VkPhysicalDeviceMemoryProperties& memoryProperties,
                                              VkDeviceMemory&                         gpuMem,
                                              VkMemoryPropertyFlags                   memProps)
  {
    VkResult result;

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device, obj, &memReqs);

    VkMemoryAllocateInfo memInfo;
    if(!PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(memoryProperties, memReqs, memProps, memInfo))
    {
      return VK_ERROR_INITIALIZATION_FAILED;
    }

    result = vkAllocateMemory(m_device, &memInfo, nullptr, &gpuMem);
    if(result != VK_SUCCESS)
    {
      return result;
    }

    result = vkBindBufferMemory(m_device, obj, gpuMem, 0);
    if(result != VK_SUCCESS)
    {
      return result;
    }

    return VK_SUCCESS;
  }

  VkResult DeviceUtils::allocMemAndBindBuffer(VkBuffer obj, VkPhysicalDevice physicalDevice, VkDeviceMemory& gpuMem, VkMemoryPropertyFlags memProps)
  {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    return allocMemAndBindBuffer(obj, memProperties, gpuMem, memProps);
  }

  VkBuffer DeviceUtils::createBuffer(size_t size, VkBufferUsageFlags usage, VkBufferCreateFlags flags) const
  {
    VkResult           result;
    VkBuffer           buffer;
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size               = size;
    bufferInfo.usage              = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.flags              = flags;

    result = vkCreateBuffer(m_device, &bufferInfo, m_allocator, &buffer);
    assert(result == VK_SUCCESS);

    return buffer;
  }

  VkBufferView DeviceUtils::createBufferView(VkBuffer buffer, VkFormat format, VkDeviceSize size, VkDeviceSize offset, VkBufferViewCreateFlags flags) const
  {
    VkResult result;

    VkBufferViewCreateInfo info = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    info.buffer                 = buffer;
    info.flags                  = flags;
    info.offset                 = offset;
    info.range                  = size;
    info.format                 = format;

    assert(size);

    VkBufferView view;
    result = vkCreateBufferView(m_device, &info, m_allocator, &view);
    assert(result == VK_SUCCESS);
    return view;
  }

  VkBufferView DeviceUtils::createBufferView(VkDescriptorBufferInfo dinfo, VkFormat format, VkBufferViewCreateFlags flags) const
  {
    VkResult result;

    VkBufferViewCreateInfo info = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    info.buffer                 = dinfo.buffer;
    info.flags                  = flags;
    info.offset                 = dinfo.offset;
    info.range                  = dinfo.range;
    info.format                 = format;

    VkBufferView view;
    result = vkCreateBufferView(m_device, &info, m_allocator, &view);
    assert(result == VK_SUCCESS);
    return view;
  }

  VkDescriptorSetLayout DeviceUtils::createDescriptorSetLayout(size_t                              numBindings,
                                                               const VkDescriptorSetLayoutBinding* bindings,
                                                               VkDescriptorSetLayoutCreateFlags    flags) const
  {
    VkResult                        result;
    VkDescriptorSetLayoutCreateInfo descriptorSetEntry = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptorSetEntry.bindingCount                    = uint32_t(numBindings);
    descriptorSetEntry.pBindings                       = bindings;
    descriptorSetEntry.flags                           = flags;

    VkDescriptorSetLayout descriptorSetLayout;
    result = vkCreateDescriptorSetLayout(m_device, &descriptorSetEntry, m_allocator, &descriptorSetLayout);
    assert(result == VK_SUCCESS);

    return descriptorSetLayout;
  }

  VkPipelineLayout DeviceUtils::createPipelineLayout(size_t                       numLayouts,
                                                     const VkDescriptorSetLayout* setLayouts,
                                                     size_t                       numRanges,
                                                     const VkPushConstantRange*   ranges,
                                                     VkPipelineLayoutCreateFlags flags) const
  {
    VkResult                   result;
    VkPipelineLayoutCreateInfo layoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutCreateInfo.setLayoutCount             = uint32_t(numLayouts);
    layoutCreateInfo.pSetLayouts                = setLayouts;
    layoutCreateInfo.pushConstantRangeCount     = uint32_t(numRanges);
    layoutCreateInfo.pPushConstantRanges        = ranges;
    layoutCreateInfo.flags = flags;

    VkPipelineLayout pipelineLayout;
    result = vkCreatePipelineLayout(m_device, &layoutCreateInfo, m_allocator, &pipelineLayout);
    assert(result == VK_SUCCESS);

    return pipelineLayout;
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  void DeviceUtilsEx::init(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, uint32_t queueFamilyIndex, const VkAllocationCallbacks* allocator)
  {
    DeviceUtils::m_device    = device;
    DeviceUtils::m_allocator = allocator;
    m_queue                  = queue;
    m_physicalDevice         = physicalDevice;
    m_queueFamilyIndex       = queueFamilyIndex;
    CreateCommandPool();
  }

  void DeviceUtilsEx::CreateCommandPool()
  {

    VkCommandPoolCreateInfo info = {};
    info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex        = m_queueFamilyIndex;
    VkResult result              = vkCreateCommandPool(m_device, &info, m_allocator, &m_commandPool);
    assert(result == VK_SUCCESS);
  }

  DeviceUtilsEx::~DeviceUtilsEx()
  {
    deInit();
  }

  void DeviceUtilsEx::deInit()
  {
    if(m_commandPool)
      vkDestroyCommandPool(m_device, m_commandPool, m_allocator);
    m_commandPool = VK_NULL_HANDLE;
  }


  VkCommandBuffer DeviceUtilsEx::beginSingleTimeCommands()
  {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool                 = m_commandPool;
    allocInfo.commandBufferCount          = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  void DeviceUtilsEx::endSingleTimeCommands(VkCommandBuffer commandBuffer)
  {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo       = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_queue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  void DeviceUtilsEx::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
  {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size               = size;
    bufferInfo.usage              = usage;
    bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize       = memRequirements.size;
    allocInfo.memoryTypeIndex      = findMemoryType(memRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
  }
  //--------------------------------------------------------------------------------------------------
  //
  //
  void DeviceUtilsEx::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
  {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion = {};
    copyRegion.size         = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  void DeviceUtilsEx::createTextureImage(uint8_t* pixels, int texWidth, int texHeight, int texChannels, VkImage& textureImage, VkDeviceMemory& textureImageMemory)
  {
    DeviceUtils    deviceUtils(m_device);
    VkDeviceSize   imageSize = texWidth * texHeight * 4;
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    stagingBuffer = deviceUtils.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    deviceUtils.allocMemAndBindBuffer(stagingBuffer, memProperties, stagingBufferMemory,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    //createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    //  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //  stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingBufferMemory);

    createImage(texWidth, texHeight, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                textureImage, textureImageMemory);

    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  void DeviceUtilsEx::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
  {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region               = {};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = {0, 0, 0};
    region.imageExtent                     = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
  }
  //--------------------------------------------------------------------------------------------------
  //
  //
  void DeviceUtilsEx::createImage(uint32_t              width,
                                  uint32_t              height,
                                  VkSampleCountFlagBits samples,
                                  VkFormat              format,
                                  VkImageTiling         tiling,
                                  VkImageUsageFlags     usage,
                                  VkMemoryPropertyFlags properties,
                                  VkImage&              image,
                                  VkDeviceMemory&       imageMemory)
  {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width      = width;
    imageInfo.extent.height     = height;
    imageInfo.extent.depth      = 1;
    imageInfo.mipLevels         = 1;
    imageInfo.arrayLayers       = 1;
    imageInfo.format            = format;
    imageInfo.tiling            = tiling;
    imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage             = usage;
    imageInfo.samples           = samples;
    imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize       = memRequirements.size;
    allocInfo.memoryTypeIndex      = findMemoryType(memRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(m_device, image, imageMemory, 0);
  }
  //--------------------------------------------------------------------------------------------------
  //
  //
  VkImageView DeviceUtilsEx::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
  {
    VkImageViewCreateInfo viewInfo           = {};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = format;
    viewInfo.subresourceRange.aspectMask     = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    VkImageView imageView;
    if(vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  uint32_t DeviceUtilsEx::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
  {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
      if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
      {
        return i;
      }
    }

    throw std::runtime_error("failed to find suitable memory type!");
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  void DeviceUtilsEx::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
  {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout            = oldLayout;
    barrier.newLayout            = newLayout;
    barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;

    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    barrier.srcAccessMask = 0;  //
    barrier.dstAccessMask = 0;  //

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

      if(format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
      {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      }
    }
    else
    {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = 0;

      sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
      throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleTimeCommands(commandBuffer);
  }
}

