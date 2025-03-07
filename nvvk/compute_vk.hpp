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


#pragma once

#include <nvh/nvprint.hpp>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include "vulkan/vulkan_core.h"
#include "descriptorsets_vk.hpp"
#include "nvvk/shaders_vk.hpp"

#define NVVK_COMPUTE_DEFAULT_BLOCK_SIZE_1D 256

namespace nvvk {

//////////////////////////////////////////////////////////////////////////
/** @DOC_START
  # class nvvk::PushComputeDispatcher

  nvvk::PushComputeDispatcher is a convenience structure for easily creating
  compute-only pipelines by defining the bindings and providing SPV code.
  The descriptor set updates are carried out using the KHR_push_descriptor 
  extension.


  Example:

  ```cpp
  
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
  myCompute.init(device);
  const uint8_t* spvCode = getMyComputeShaderCode(...);
  size_t spvCodeSize = getMyComputeShaderCodeSize(...);
  myCompute.getBindings().addBinding(BindingLocation::eMyBindingLocation, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL););
  myCompute.setCode(spvCode, spvCodeSize);
  myCompute.finalizePipeline();

  ...
  myCompute.updateBufferBinding(BindingLocation::eMyBindingLocation, myFirstBuffer)
  VkCommandBuffer cmd = getMyCommandBuffer(...);

  myCompute.dispatch(cmd, targetThreadCount, &pushConstant);
  ...
  myCompute.updateBufferBinding(BindingLocation::eMyBindingLocation, mySecondBuffer)
  myCompute.dispatch(cmd, targetThreadCount, &pushConstant);
  ...
  ```
@DOC_END */


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

template <typename TPushConstants = void, typename TBindingEnum = uint32_t, int32_t TPipelineCount = 1, uint32_t TCustomWriteDescriptorSetPNextMaxSize = 0>
class PushComputeDispatcher
{
public:
  PushComputeDispatcher() = default;
  PushComputeDispatcher(VkDevice device) { init(device); }

  virtual ~PushComputeDispatcher() { deinit(); }

  void init(VkDevice device) { m_device = device; }

  // Set the shader code for the pipeline at index pipelineIndex. The code will be compiled into a shader module that will be destroyed when the PushComputeDispatcher is destroyed.
  bool setCode(const void* shaderCode, size_t codeSize, int32_t pipelineIndex = 0)
  {
    if(m_device == VK_NULL_HANDLE)
    {
      LOGE("%s: VK_NULL_HANDLE device. Call PushComputeDispatcher::init() beforehand.\n", __FUNCTION__);
      assert(false);
      return false;
    }

    if(m_isFinalized)
    {
      LOGE("%s: Cannot set shader code after finalizing the pipeline\n", __FUNCTION__);
      assert(false);
      return false;
    }


    m_shaderModules[pipelineIndex].module = nvvk::createShaderModule(m_device, shaderCode, codeSize);

    if(m_shaderModules[pipelineIndex].module == VK_NULL_HANDLE)
    {
      return false;
    }
    m_shaderModules[pipelineIndex].isLocal = true;

    return true;
  }

  // Set the shader module for the pipeline at index pipelineIndex. The module will not be destroyed when the PushComputeDispatcher is destroyed.
  bool setCode(VkShaderModule shaderModule, uint32_t pipelineIndex = 0u)
  {
    if(m_device == VK_NULL_HANDLE)
    {
      LOGE("%s: VK_NULL_HANDLE device. Call PushComputeDispatcher::init() beforehand.\n", __FUNCTION__);
      assert(false);
      return false;
    }

    if(m_isFinalized)
    {
      LOGE("%s: Cannot set shader code after finalizing the pipeline\n", __FUNCTION__);
      assert(false);
      return false;
    }
    m_shaderModules[pipelineIndex].module  = shaderModule;
    m_shaderModules[pipelineIndex].isLocal = false;
    return shaderModule != VK_NULL_HANDLE;
  }

  // This is the main method to add bindings to the pipeline. To add a binding call getBindings().addBinding(...).
  inline nvvk::DescriptorSetBindings&       getBindings() { return m_bindings; }
  inline const nvvk::DescriptorSetBindings& getBindings() const { return m_bindings; }

  // Once the code for all pipelines has been provided and all bindings have been added by calling getBindings().addBinding(...), this method creates the pipeline layout and the pipelines.
  bool finalizePipeline(const VkSpecializationInfo* specialization = nullptr)
  {
    if(m_isFinalized)
    {
      LOGE("%s: Cannot finalize a PushComputeDispatcher that has already been finalized\n", __FUNCTION__);
      assert(false);
      return false;
    }

    if(m_device == VK_NULL_HANDLE)
    {
      LOGE("%s: VK_NULL_HANDLE device. Call PushComputeDispatcher::init() beforehand.\n", __FUNCTION__);
      assert(false);
      return false;
    }

    m_descriptorSetLayout = m_bindings.createLayout(m_device, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutCreateInfo.pSetLayouts    = &m_descriptorSetLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 1;

    VkPushConstantRange pushConstantRange{};

    if constexpr(!std::is_void<TPushConstants>::value)
    {
      pushConstantRange                               = {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TPushConstants)};
      pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
      pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstantRange;
    }

    VkResult r = vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_layout);

    if(r != VK_SUCCESS || m_layout == VK_NULL_HANDLE)
    {
      LOGE("%s: Pipeline layout creation failed\n", __FUNCTION__);
      return false;
    }
    VkPipelineShaderStageCreateInfo stageCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stageCreateInfo.stage                           = VK_SHADER_STAGE_COMPUTE_BIT;
    stageCreateInfo.pName                           = "main";
    stageCreateInfo.pSpecializationInfo             = specialization;

    for(int32_t i = 0; i < TPipelineCount; i++)
    {
      stageCreateInfo.module = m_shaderModules[i].module;

      if(stageCreateInfo.module == VK_NULL_HANDLE)
      {
        LOGE("%s: Shader module not set for pipeline %d\n", __FUNCTION__, i);
        return false;
      }
      VkComputePipelineCreateInfo createInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
      createInfo.stage  = stageCreateInfo;
      createInfo.layout = m_layout;
      r                 = vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipelines[i]);
      if(r != VK_SUCCESS || m_pipelines[i] == VK_NULL_HANDLE)
      {
        LOGE("%s: Creation failed for pipeline %d\n", __FUNCTION__, i);
        return false;
      }
      // If the module has been created by the PushComputeDispatcher, destroy it. Otherwise it is the app's responsibility to destroy it.
      if(m_shaderModules[i].isLocal)
      {
        vkDestroyShaderModule(m_device, m_shaderModules[i].module, nullptr);
      }
    }

    uint32_t currentOffset = 0;

    for(size_t i = 0; i < m_bindings.size(); i++)
    {
      m_bindingOffsets[m_bindings.data()[i].binding] = currentOffset;
      currentOffset += m_bindings.data()[i].descriptorCount;
    }
    m_bindingData.resize(currentOffset);

    m_isFinalized = true;

    return true;
  }

  bool isValid() const { return m_isFinalized; }

  // Update the buffer binding at location index with the provided buffer. If the binding is an array, the arrayElement parameter specifies the index of the array element to update.
  bool updateBinding(TBindingEnum index, VkBuffer buffer, uint32_t arrayElement = 0)
  {
    // Sanity check, verify that the binding location has previously been added by the app using getBindings().addBinding(...)
    if(m_bindings.getCount(index) <= arrayElement)
    {
      LOGE("%s: Invalid arrayElement %d for binding %d\n", __FUNCTION__, arrayElement, index);
      assert(false);
      return false;
    }

    if(!m_isFinalized)
    {
      LOGE("%s: Cannot update shader bindings before finalizing the pipeline\n", __FUNCTION__);
      assert(false);
      return false;
    }

    size_t offset = m_bindingOffsets[index] + arrayElement;

    if(m_bindingData[offset].type == eNone)
    {
      m_bindingData[offset].type             = eBuffer;
      m_bindingData[offset].bufferInfo       = {};
      m_bindingData[offset].bufferInfo.range = VK_WHOLE_SIZE;
      m_writes.emplace_back(m_bindings.makeWrite(0, index, &m_bindingData[offset].bufferInfo, arrayElement));
    }

    if(m_bindingData[offset].type == eBuffer)
    {
      m_bindingData[offset].bufferInfo.buffer = buffer;
    }
    else
    {
      LOGE("%s: Inconsistent type at arrayElement %d for binding %d: Buffer type expected\n", __FUNCTION__, arrayElement, index);
      assert(false);
      return false;
    }
    return true;
  }
  bool updateBinding(TBindingEnum index, VkAccelerationStructureKHR acc, uint32_t arrayElement = 0)
  {
    // Sanity check, verify that the binding location has previously been added by the app using bindings.addBinding(...)
    if(m_bindings.getCount(index) <= arrayElement)
    {
      LOGE("%s: Invalid arrayElement %d for binding %d\n", __FUNCTION__, arrayElement, index);
      assert(false);
      return false;
    }
    if(!m_isFinalized)
    {
      LOGE("%s: Cannot update shader bindings before finalizing the pipeline\n", __FUNCTION__);
      assert(false);

      return false;
    }
    size_t offset = m_bindingOffsets[index] + arrayElement;

    if(m_bindingData[offset].type == eNone)
    {
      m_bindingData[offset].accelInfo = {};
      auto* info                      = &m_bindingData[offset].accelInfo;

      m_bindingData[offset].accel = {};

      info->sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
      info->pNext                      = nullptr;
      info->accelerationStructureCount = 1;
      info->pAccelerationStructures    = &m_bindingData[offset].accel;

      m_writes.emplace_back(m_bindings.makeWrite(0, index, info, arrayElement));
    }


    if(m_bindingData[offset].type == eAccelerationStructure)
    {
      auto* info                    = &m_bindingData[offset].accelInfo;
      info->pAccelerationStructures = acc;
    }
    else
    {
      LOGE("%s: Inconsistent type at arrayElement %d for binding %d: Buffer type expected\n", __FUNCTION__, arrayElement, index);
      assert(false);

      return false;
    }

    return true;
  }

  bool updateBinding(TBindingEnum  index,
                     VkImageView   view         = VK_NULL_HANDLE,
                     VkImageLayout layout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VkSampler     sampler      = VK_NULL_HANDLE,
                     uint32_t      arrayElement = 0)
  {
    // Sanity check, verify that the binding location has previously been added by the app using getBindings().addBinding(...)
    if(m_bindings.getCount(index) <= arrayElement)
    {
      LOGE("%s: Invalid arrayElement %d for binding %d\n", __FUNCTION__, arrayElement, index);
      assert(false);
      return false;
    }
    if(!m_isFinalized)
    {
      LOGE("%s: Cannot update shader bindings before finalizing the pipeline\n", __FUNCTION__);
      assert(false);

      return false;
    }
    size_t offset = m_bindingOffsets[index] + arrayElement;

    if(m_bindingData[offset].type == eNone)
    {
      m_bindingData[offset].type      = eImage;
      m_bindingData[offset].imageInfo = {};

      m_writes.emplace_back(m_bindings.makeWrite(0, index, &m_bindingData[offset].imageInfo, arrayElement));
    }


    if(m_bindingData[offset].type == eImage)
    {
      VkDescriptorImageInfo* info = &m_bindingData[offset].imageInfo;

      info->sampler     = sampler;
      info->imageView   = view;
      info->imageLayout = layout;
    }
    else
    {
      LOGE("%s: Inconsistent type at arrayElement %d for binding %d: Buffer type expected\n", __FUNCTION__, arrayElement, index);
      assert(false);

      return false;
    }
    return true;
  }

  // Generic method to update the binding at location index with the provided data in the pNext pointer of the VkWriteDescriptorSet structure.
  bool updateBinding(TBindingEnum index, void* writeDescriptorSetPNextData, uint32_t writeDescriptorSetPNextDataSize, uint32_t arrayElement = 0)
  {
    // Sanity check, verify that the binding location has previously been added by the app using getBindings().addBinding(...)
    if(m_bindings.getCount(index) <= arrayElement)
    {
      LOGE("%s: Invalid arrayElement %d for binding %d\n", __FUNCTION__, arrayElement, index);
      assert(false);
      return false;
    }
    if(!m_isFinalized)
    {
      LOGE("%s: Cannot update shader bindings before finalizing the pipeline\n", __FUNCTION__);
      assert(false);

      return false;
    }

    if(writeDescriptorSetPNextDataSize > TCustomWriteDescriptorSetPNextMaxSize)
    {
      LOGE("%s: writeDescriptorSetPNextDataSize %d exceeds TCustomWriteDescriptorSetPNextMaxSize %d\n", __FUNCTION__,
           writeDescriptorSetPNextDataSize, TCustomWriteDescriptorSetPNextMaxSize);
      assert(false);
      return false;
    }

    size_t offset = m_bindingOffsets[index] + arrayElement;

    if(m_bindingData[offset].type == eNone)
    {
      m_bindingData[offset].type = eCustom;
      VkWriteDescriptorSet writeDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
      writeDescriptorSet.descriptorCount = 1;
      writeDescriptorSet.dstArrayElement = arrayElement;
      writeDescriptorSet.dstBinding      = index;
      writeDescriptorSet.pNext           = m_bindingData[offset].customInfo;
      m_writes.emplace_back(writeDescriptorSet);
    }


    if(m_bindingData[offset].type == eCustom)
    {
      memcpy(m_bindingData[offset].customInfo, writeDescriptorSetPNextData, writeDescriptorSetPNextDataSize);
    }
    else
    {
      LOGE("%s: Inconsistent type at arrayElement %d for binding %d: Buffer type expected\n", __FUNCTION__, arrayElement, index);
      assert(false);

      return false;
    }
    return true;
  }


  // Utility function to compute the number of blocks needed to dispatch the requested number of threads
  uint32_t getBlockCount(uint32_t targetThreadCount, uint32_t blockSize)
  {
    return (targetThreadCount + blockSize - 1) / blockSize;
  }


  // Bind the pipeline resources. Used internally, or if the app uses a direct call to
  // vkCmdDispatch instead of the dispatch() method
  bool bind(VkCommandBuffer cmd, const TPushConstants* constants = nullptr, int32_t pipelineIndex = 0)
  {
    if(!m_isFinalized)
    {
      LOGE("%s: Cannot bind the pipeline before finalizing it\n", __FUNCTION__);
      assert(false);

      return false;
    }
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines[pipelineIndex]);
    if(constants != nullptr)
    {
      if constexpr(!std::is_void<TPushConstants>::value)
      {
        vkCmdPushConstants(cmd, m_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TPushConstants), constants);
      }
      else
      {
        LOGW("%s: PushConstants are not supported for a PushComputeDispatcher<void, ...>\n", __FUNCTION__);
      }
    }
    if(!m_writes.empty())
    {
      vkCmdPushDescriptorSetKHR(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_layout, 0,
                                static_cast<uint32_t>(m_writes.size()), m_writes.data());
    }
    return true;
  }

  // Dispatch the requested number of threads in a 1D grid, based on blockSize. If pipelineIndex is negative all pipelines will be executed sequentially.
  // Otherwise the pipeline at index pipelineIndex will be executed.
  bool dispatchThreads(VkCommandBuffer       cmd,
                       uint32_t              threadCount,
                       const TPushConstants* constants     = nullptr,
                       uint32_t              postBarrier   = DispatcherBarrier::eCompute,
                       uint32_t              preBarrier    = DispatcherBarrier::eNone,
                       uint32_t              blockSize     = NVVK_COMPUTE_DEFAULT_BLOCK_SIZE_1D,
                       int32_t               pipelineIndex = -1)
  {
    uint32_t blockCount = getBlockCount(threadCount, blockSize);
    return dispatchBlocks(cmd, blockCount, constants, postBarrier, preBarrier, pipelineIndex);
  }

  // Dispatch the requested number of blocks in a 1D grid. If pipelineIndex is negative all pipelines will be executed sequentially.
  // Otherwise the pipeline at index pipelineIndex will be executed.
  bool dispatchBlocks(VkCommandBuffer       cmd,
                      uint32_t              blockCount,
                      const TPushConstants* constants     = nullptr,
                      uint32_t              postBarrier   = DispatcherBarrier::eCompute,
                      uint32_t              preBarrier    = DispatcherBarrier::eNone,
                      int32_t               pipelineIndex = -1)
  {

    return dispatchBlocks(cmd, {blockCount, 1, 1}, constants, postBarrier, preBarrier, pipelineIndex);
  }

  // Dispatch the requested number of blocks in a 1D/2D/3D grid (use 1 for the unused dimensions). If pipelineIndex is negative all pipelines will be executed sequentially.
  // Otherwise the pipeline at index pipelineIndex will be executed.
  bool dispatchBlocks(VkCommandBuffer       cmd,
                      const glm::uvec3&     blockCount,
                      const TPushConstants* constants     = nullptr,
                      uint32_t              postBarrier   = DispatcherBarrier::eCompute,
                      uint32_t              preBarrier    = DispatcherBarrier::eNone,
                      int32_t               pipelineIndex = -1)
  {
    InternalLaunchParams launchParams{};
    launchParams.isIndirect       = false;
    launchParams.directBlockCount = blockCount;
    return dispatchBlocksInternal(cmd, launchParams, constants, postBarrier, preBarrier, pipelineIndex);
  }

  // Indirect dispatch the requested number of blocks in a 1D/2D/3D grid (use 1 for the unused dimensions). The number of blocks is stored in device memory in the indirectBlockCount buffer, at the offset indirectOffset.
  //  If pipelineIndex is negative all pipelines will be executed sequentially. Otherwise the pipeline at index pipelineIndex will be executed.
  bool dispatchBlocksIndirect(VkCommandBuffer       cmd,
                              VkBuffer              indirectBlockCount,
                              VkDeviceSize          indirectOffset,
                              const TPushConstants* constants     = nullptr,
                              uint32_t              postBarrier   = DispatcherBarrier::eCompute,
                              uint32_t              preBarrier    = DispatcherBarrier::eNone,
                              int32_t               pipelineIndex = -1)
  {
    InternalLaunchParams launchParams{};
    launchParams.isIndirect                = true;
    launchParams.indirectBlockCount.buffer = indirectBlockCount;
    launchParams.indirectBlockCount.offset = indirectOffset;
    return dispatchBlocksInternal(cmd, launchParams, constants, postBarrier, preBarrier, pipelineIndex);
  }

  // Destroy the pipeline layout and the pipelines, and clear the binding data
  void deinit()
  {
    if(m_device == VK_NULL_HANDLE)
    {
      return;
    }

    vkDestroyPipelineLayout(m_device, m_layout, nullptr);
    m_layout = VK_NULL_HANDLE;

    for(int32_t i = 0; i < TPipelineCount; i++)
    {
      vkDestroyPipeline(m_device, m_pipelines[i], nullptr);
      m_pipelines[i] = VK_NULL_HANDLE;
    }
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
    m_descriptorSetLayout = VK_NULL_HANDLE;

    m_bindingOffsets.clear();
    m_bindingData.clear();

    m_writes.clear();
    m_bindings.clear();
    m_device      = VK_NULL_HANDLE;
    m_isFinalized = false;
  }

  VkPipeline getPipeline(int32_t index = 0) const { return m_pipelines[index]; }

private:
  VkPipelineLayout                       m_layout{};
  std::array<VkPipeline, TPipelineCount> m_pipelines{};
  VkDescriptorSetLayout                  m_descriptorSetLayout{};
  nvvk::DescriptorSetBindings            m_bindings;


  // Binding data which will be pushed before dispatching the compute shader(s)
  std::vector<VkWriteDescriptorSet> m_writes;

  // Types of resources that can be bound to the shader
  enum BindingType
  {
    eNone,
    eBuffer,
    eImage,
    eAccelerationStructure,
    eCustom
  };

  // Container for the binding information. The type field specifies the type of resource that is bound to the shader.
  // The union allows all the binding data to be stored in a single vector
  struct Binding
  {
    BindingType type{eNone};
    union
    {
      VkDescriptorBufferInfo bufferInfo;
      VkDescriptorImageInfo  imageInfo;

      struct
      {
        VkWriteDescriptorSetAccelerationStructureKHR accelInfo;
        VkAccelerationStructureKHR                   accel;
      };
      // Min size of 1 to avoid compile error with empty array if custom data is not used
      uint8_t customInfo[TCustomWriteDescriptorSetPNextMaxSize == 0 ? 1 : TCustomWriteDescriptorSetPNextMaxSize];
    };
  };

  // Keep the data referenced by the VkWriteDescriptorSets above so the app developer does not have to worry about pointer scopes
  std::vector<Binding> m_bindingData;
  // For each binding point , store the offset in m_bindingData where the data for that binding is stored
  std::unordered_map<uint32_t, size_t> m_bindingOffsets;

  // True if the pipeline has been finalized, used for sanity checking
  bool m_isFinalized{false};

  // Shader module information
  struct ShaderModule
  {
    VkShaderModule module{VK_NULL_HANDLE};
    // If true the shader module has been created by the PushComputeDispatcher and will be destroyed when the PushComputeDispatcher is destroyed.
    // Otherwise the module has been provided by the app, and it is the app's responsibility to destroy it.
    bool isLocal{false};
  };

  // Array of shader modules that will use the same bindings. Each pipeline will use a different shader module.
  std::array<ShaderModule, TPipelineCount> m_shaderModules;

  // Logical device for which the pipelines are created
  VkDevice m_device{VK_NULL_HANDLE};

  // Launch parameters, used internally only
  struct InternalLaunchParams
  {
    // If true the launch uses indirect parameters stored in device memory
    bool isIndirect{false};
    union
    {
      // Direct launch parameters defining the number of blocks in each dimension
      glm::uvec3 directBlockCount{0, 0, 0};
      // Indirect launch parameters
      struct
      {
        // Buffer containing the number of blocks in each dimension
        VkBuffer buffer{VK_NULL_HANDLE};
        // Offset in the buffer where the indirect parameters are stored
        VkDeviceSize offset{0ull};
      } indirectBlockCount;
    };
  };

  enum BarrierType
  {
    ePreBarrier,
    ePostBarrier
  };


  inline void barrier(VkCommandBuffer cmd, uint32_t b, BarrierType t)
  {
    if(b != eNone)
    {
      VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};

      VkAccessFlags& computeMask  = (t == ePreBarrier) ? mb.dstAccessMask : mb.srcAccessMask;
      VkAccessFlags& externalMask = (t == ePreBarrier) ? mb.srcAccessMask : mb.dstAccessMask;

      VkPipelineStageFlags computeStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

      computeMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      VkPipelineStageFlags externalStage{};

      if((b & eCompute) || (b & eGraphics) || (b & eRaytracing))
      {
        externalMask |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        if(b & eCompute)
          externalStage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        if(b & eGraphics)
          externalStage |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        if(b & eRaytracing)
          externalStage |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
      }
      if(b & eTransfer)
      {
        externalMask |= VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        externalStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
      }

      VkPipelineStageFlags srcStage = (t == ePreBarrier) ? externalStage : computeStage;
      VkPipelineStageFlags dstStage = (t == ePreBarrier) ? computeStage : externalStage;

      vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 1, &mb, 0, nullptr, 0, nullptr);
    }
  }

  // Internal dispatch method used by the public dispatch methods above
  bool dispatchBlocksInternal(VkCommandBuffer             cmd,
                              const InternalLaunchParams& launchParams,
                              const TPushConstants*       constants   = nullptr,
                              uint32_t                    postBarrier = DispatcherBarrier::eCompute,
                              uint32_t                    preBarrier  = DispatcherBarrier::eNone,
                              // If pipelineIndex < 0 all pipelines will be executed sequentially. Otherwise, only dispatch the requested pipeline
                              int32_t pipelineIndex = -1)
  {
    if(!m_isFinalized)
    {
      LOGE("%s: Cannot dispatch the pipeline before finalizing it\n", __FUNCTION__);
      assert(false);

      return false;
    }
    // Apply pipeline barriers before the dispatch if requested
    barrier(cmd, preBarrier, ePreBarrier);

    // If the pipelineIndex was -1, all pipelines will be executed sequentially. Otherwise we only execute one pipeline
    // at the requested index.
    uint32_t currentPipeline = (pipelineIndex < 0) ? 0 : pipelineIndex;
    uint32_t dispatchCount   = (pipelineIndex < 0) ? TPipelineCount : 1;

    for(uint32_t i = 0; i < dispatchCount; i++)
    {
      // Bind the current pipeline, push constants and push the descriptors
      bind(cmd, constants, currentPipeline + i);

      // Launch the compute shader
      if(launchParams.isIndirect)
      {
        vkCmdDispatchIndirect(cmd, launchParams.indirectBlockCount.buffer, launchParams.indirectBlockCount.offset);
      }
      else
      {
        vkCmdDispatch(cmd, launchParams.directBlockCount.x, launchParams.directBlockCount.y,
                      launchParams.directBlockCount.z);
      }
      // Apply pipeline barriers after the dispatch if requested
      barrier(cmd, postBarrier, ePostBarrier);
    }
    return true;
  }
};
}  // namespace nvvk
