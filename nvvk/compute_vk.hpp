/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#pragma once

#include <unordered_map>
#include <memory>
#include "vulkan/vulkan_core.h"
#include "descriptorsets_vk.hpp"

#define NVVK_COMPUTE_DEFAULT_BLOCK_SIZE 256

namespace nvvk {

//////////////////////////////////////////////////////////////////////////
/**
  \class nvvk::PushComputeDispatcher

  nvvk::PushComputeDispatcher is a convenience structure for easily creating
  compute-only pipelines by defining the bindings and providing SPV code.
  The descriptor set updates are carried out using the KHR_push_descriptor 
  extension.


  Example:

  \code{.cpp}
  
  enum BindingLocation
  {
    eMyBindingLocation = 0
  };

  struct PushConstant{
   ...
  }
  pushConstant;

  nvvk::PushComputeDispatcher<PushConstant, BindingLocation> myCompute;
  VkBuffer myFirstBuffer = createMyFirstBuffer(...);
  VkBuffer mySecondBuffer = createMySecondBuffer(...);
  VkDevice device = getMyVkDevice(...);
  const uint8_t* spvCode = getMyComputeShaderCode(...);
  size_t spvCodeSize = getMyComputeShaderCodeSize(...);
  myCompute.addBufferBinding(BindingLocation::eMyBindingLocation, myFirstBuffer);
  myCompute.setCode(device, spvCode, spvCodeSize);
  myCompute.finalizePipeline(device);

  ...
  VkCommandBuffer cmd = getMyCommandBuffer(...);
  myCompute.dispatch(cmd, targetThreadCount, &pushConstant);
  ...
  myCompute.updateBufferBinding(BindingLocation::eMyBindingLocation, mySecondBuffer)
  myCompute.dispatch(cmd, targetThreadCount, &pushConstant);
  ...
  \endcode
*/


/// Barrier types usable before and after the shader dispatch
/// Those barriers apply to SHADER_READ, SHADER_WRITE and TRANSFER if needed
enum DispatcherBarrier
{
  eNone       = 0,
  eCompute    = 1,
  eTransfer   = 2,
  eGraphics   = 4,
  eRaytracing = 8
};

template <typename TPushConstants, typename TBindingEnum>
struct PushComputeDispatcher
{
  VkPipelineLayout            layout{};
  VkPipeline                  pipeline{};
  VkDescriptorSetLayout       dsetLayout{};
  nvvk::DescriptorSetBindings bindings;

  std::unordered_map<TBindingEnum, std::unique_ptr<VkDescriptorBufferInfo>>                       bufferInfos;
  std::unordered_map<TBindingEnum, std::unique_ptr<VkWriteDescriptorSetAccelerationStructureKHR>> accelInfos;
  std::unordered_map<TBindingEnum, std::unique_ptr<VkAccelerationStructureKHR>>                   accel;
  std::unordered_map<TBindingEnum, std::unique_ptr<VkDescriptorImageInfo>>                        sampledImageInfos;

  TPushConstants pushConstants{};

  std::vector<VkWriteDescriptorSet> writes;
  VkShaderModule                    shaderModule;

  bool addBufferBinding(TBindingEnum index)
  {
    if(bufferInfos.find(index) == bufferInfos.end())
    {
      bindings.addBinding(VkDescriptorSetLayoutBinding{uint32_t(index), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT});

      bufferInfos[index] = std::make_unique<VkDescriptorBufferInfo>();
      auto* info         = bufferInfos[index].get();
      *(info)            = {VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};
      writes.emplace_back(bindings.makeWrite(0, index, info));
      return true;
    }
    return false;
  }

  bool addAccelerationStructureBinding(TBindingEnum index)
  {
    if(accelInfos.find(index) == accelInfos.end())
    {
      bindings.addBinding(VkDescriptorSetLayoutBinding{uint32_t(index), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                                                       1, VK_SHADER_STAGE_COMPUTE_BIT});

      accelInfos[index] = std::make_unique<VkWriteDescriptorSetAccelerationStructureKHR>();
      auto* info        = accelInfos[index].get();

      accel[index]                     = std::make_unique<VkAccelerationStructureKHR>();
      auto* acc                        = accel[index].get();
      info->sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
      info->pNext                      = nullptr;
      info->accelerationStructureCount = 1;
      info->pAccelerationStructures = acc;

      writes.emplace_back(bindings.makeWrite(0, index, info));
      return true;
    }
    return false;
  }
  bool addSampledImageBinding(TBindingEnum  index)
  {
    if(sampledImageInfos.find(index) == sampledImageInfos.end())
    {
      bindings.addBinding(VkDescriptorSetLayoutBinding{uint32_t(index), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                                       VK_SHADER_STAGE_COMPUTE_BIT});
      sampledImageInfos[index] = std::make_unique<VkDescriptorImageInfo>();
      auto* info               = sampledImageInfos[index].get();
      writes.emplace_back(bindings.makeWrite(0, index, info));
      return true;
    }
    return false;
  }

  bool updateBufferBinding(TBindingEnum index, VkBuffer buffer)
  {
    auto it = bufferInfos.find(index);
    if(it != bufferInfos.end())
    {
      it->second->buffer = buffer;
      return true;
    }
    return false;
  }
  bool updateAccelerationStructureBinding(TBindingEnum index, VkAccelerationStructureKHR acc)
  {
    auto it = accel.find(index);
    if(it != accel.end())
    {
      *(it->second.get()) = acc;
      return true;
    }
    return false;
  }

  bool updateSampledImageBinding(TBindingEnum  index,
                                 VkSampler     sampler = VK_NULL_HANDLE,
                                 VkImageView   view    = VK_NULL_HANDLE,
                                 VkImageLayout layout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    auto it = sampledImageInfos.find(index);
    if(it != sampledImageInfos.end())
    {
      it->second->sampler     = sampler;
      it->second->imageView   = view;
      it->second->imageLayout = layout;
      return true;
    }
    return false;
  }


  bool setCode(VkDevice device, void* shaderCode, size_t codeSize)
  {
    VkShaderModuleCreateInfo moduleCreateInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    moduleCreateInfo.codeSize = codeSize;
    moduleCreateInfo.pCode    = reinterpret_cast<uint32_t*>(shaderCode);

    VkResult r = vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule);
    if(r != VK_SUCCESS || shaderModule == VK_NULL_HANDLE)
    {
      return false;
    }
    return true;
  }
  bool finalizePipeline(VkDevice device)
  {

    dsetLayout = bindings.createLayout(device, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutCreateInfo.pSetLayouts    = &dsetLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 1;

    VkPushConstantRange pushConstantRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TPushConstants)};
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstantRange;

    VkResult r = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &layout);

    if(r != VK_SUCCESS || layout == VK_NULL_HANDLE)
    {
      return false;
    }
    VkPipelineShaderStageCreateInfo stageCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stageCreateInfo.stage                           = VK_SHADER_STAGE_COMPUTE_BIT;
    stageCreateInfo.pName                           = "main";

    stageCreateInfo.module = shaderModule;

    VkComputePipelineCreateInfo createInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    createInfo.stage  = stageCreateInfo;
    createInfo.layout = layout;


    r = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline);
    if(r != VK_SUCCESS || pipeline == VK_NULL_HANDLE)
    {
      return false;
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);
    return true;
  }


  uint32_t getBlockCount(uint32_t targetThreadCount, uint32_t blockSize)
  {
    return (targetThreadCount + blockSize - 1) / blockSize;
  }


  // Bind the pipeline resources. Used internally, or if the app uses a direct call to
  // vkCmdDispatch instead of the dispatch() method
  void bind(VkCommandBuffer cmd, const TPushConstants* constants = nullptr)
  {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    if(constants != nullptr)
    {
      vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TPushConstants), constants);
    }
    vkCmdPushDescriptorSetKHR(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, static_cast<uint32_t>(writes.size()),
                              writes.data());
  }

  void dispatchThreads(VkCommandBuffer       cmd,
                       uint32_t              threadCount,
                       const TPushConstants* constants   = nullptr,
                       uint32_t              postBarrier = DispatcherBarrier::eCompute,
                       uint32_t              preBarrier  = DispatcherBarrier::eNone,
                       uint32_t              blockSize   = NVVK_COMPUTE_DEFAULT_BLOCK_SIZE)
  {
    uint32_t blockCount = getBlockCount(threadCount, blockSize);
    dispatchBlocks(cmd, blockCount, constants, postBarrier, preBarrier);
  }

  void dispatchBlocks(VkCommandBuffer       cmd,
                      uint32_t              blockCount,
                      const TPushConstants* constants   = nullptr,
                      uint32_t              postBarrier = DispatcherBarrier::eCompute,
                      uint32_t              preBarrier  = DispatcherBarrier::eNone)
  {
    if(preBarrier != eNone)
    {
      VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
      mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      VkPipelineStageFlags srcStage{};
      if((preBarrier & eCompute) || (preBarrier & eGraphics) || (preBarrier & eRaytracing))
      {
        mb.srcAccessMask |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        if(preBarrier & eCompute)
          srcStage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        if(preBarrier & eGraphics)
          srcStage |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        if(preBarrier & eRaytracing)
          srcStage |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
      }
      if(preBarrier & eTransfer)
      {
        mb.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
      }

      vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
    }


    bind(cmd, constants);
    vkCmdDispatch(cmd, blockCount, 1, 1);

    if(postBarrier != eNone)
    {
      VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
      mb.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      VkPipelineStageFlags dstStage{};
      if((postBarrier & eCompute) || (postBarrier & eGraphics) || (postBarrier & eRaytracing))
      {
        mb.dstAccessMask |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        if(postBarrier & eCompute)
          dstStage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        if(postBarrier & eGraphics)
          dstStage |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        if(postBarrier & eRaytracing)
          dstStage |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
      }
      if(postBarrier & eTransfer)
      {
        mb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        dstStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
      }

      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dstStage, 0, 1, &mb, 0, nullptr, 0, nullptr);
    }
  }

  void destroy(VkDevice device)
  {
    vkDestroyPipelineLayout(device, layout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyDescriptorSetLayout(device, dsetLayout, nullptr);

    bufferInfos.clear();
    accelInfos.clear();
    accel.clear();
    sampledImageInfos.clear();
    writes.clear();
    bindings.clear();
  }
};
}  // namespace nvvk