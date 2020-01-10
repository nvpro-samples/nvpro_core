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

/**
Base functionality of raytracing

This class does not implement all what you need to do raytracing, but
helps creating the BLAS and TLAS, which then can be used by different
raytracing usage.

# Setup and Usage
~~~~ C++
m_rtBuilder.setup(device, memoryAllocator, queueIndex);
// Create array of vk::GeometryNV 
m_rtBuilder.buildBlas(allBlas);
// Create array of RaytracingBuilder::instance
m_rtBuilder.buildTlas(instances);
// Retrieve the acceleration structure
const vk::AccelerationStructureNV& tlas = m.rtBuilder.getAccelerationStructure() 
~~~~
*/


#include <mutex>
#include <vulkan/vulkan.hpp>

#include "commands_vkpp.hpp"
#include "debug_util_vkpp.hpp"

#if defined(ALLOC_DEDICATED)
#include "allocator_dedicated_vkpp.hpp"
#elif defined(ALLOC_VMA)
#include "allocator_vma_vkpp.hpp"
#elif defined(ALLOC_DMA)
#include "allocator_dma_vkpp.hpp"
#endif  // ALLOC_DEDICATED


// See https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap33.html#acceleration-structure
struct VkGeometryInstanceNV
{
  /// Transform matrix, containing only the top 3 rows
  float transform[12];
  /// Instance index
  uint32_t instanceId : 24;
  /// Visibility mask
  uint32_t mask : 8;
  /// Index of the hit group which will be invoked when a ray hits the instance
  uint32_t hitGroupId : 24;
  /// Instance flags, such as culling
  uint32_t flags : 8;
  /// Opaque handle of the bottom-level acceleration structure
  uint64_t accelerationStructureHandle;
};

namespace nvvkpp {
struct RaytracingBuilder
{
#if defined(ALLOC_DEDICATED)
  // Use a simplistic allocator for conciseness
  using nvvkAccel           = nvvkpp::AccelerationDedicated;
  using nvvkBuffer          = nvvkpp::BufferDedicated;
  using nvvkAllocator       = nvvkpp::AllocatorDedicated;
  using nvvkMemoryAllocator = vk::PhysicalDevice;
#elif defined(ALLOC_VMA)
  using nvvkAccel           = nvvkpp::AccelerationVma;
  using nvvkBuffer          = nvvkpp::BufferVma;
  using nvvkAllocator       = nvvkpp::AllocatorVma;
  using nvvkMemoryAllocator = VmaAllocator;
#elif defined(ALLOC_DMA)
  using nvvkAccel           = nvvkpp::AccelerationDma;
  using nvvkBuffer          = nvvkpp::BufferDma;
  using nvvkAllocator       = nvvkpp::AllocatorDma;
  using nvvkMemoryAllocator = nvvk::DeviceMemoryAllocator;
#endif

  RaytracingBuilder() = default;

  //--------------------------------------------------------------------------------------------------
  // Initializing the allocator and querying the raytracing properties
  //
  void setup(const vk::Device& device, nvvkMemoryAllocator& memoryAllocator, uint32_t queueIndex)
  {
    m_device     = device;
    m_queueIndex = queueIndex;
    m_debug.setup(device);
#if defined(ALLOC_DMA)
    m_alloc.init(device, &memoryAllocator);
#else
    m_alloc.init(device, memoryAllocator);
#endif
  }

  // This is an instance of a BLAS
  struct Instance
  {
    uint32_t                    blasId{0};      // Index of the BLAS in m_blas
    uint32_t                    instanceId{0};  // Instance Index (gl_InstanceID)
    uint32_t                    hitGroupId{0};  // Hit group index in the SBT
    uint32_t                    mask{0xFF};     // Visibility mask, will be AND-ed with ray mask
    vk::GeometryInstanceFlagsNV flags{vk::GeometryInstanceFlagBitsNV::eTriangleCullDisable};
    nvmath::mat4f               transform{nvmath::mat4f(1)};  // Identity
  };


  //--------------------------------------------------------------------------------------------------
  // Destroying all allocations
  //
  void destroy()
  {
    for(auto& b : m_blas)
    {
      m_alloc.destroy(b.as);
    }
    m_alloc.destroy(m_tlas.as);
    m_alloc.destroy(m_instBuffer);
  }

  // Returning the constructed top-level acceleration structure
  const vk::AccelerationStructureNV& getAccelerationStructure() { return m_tlas.as.accel; }

  //--------------------------------------------------------------------------------------------------
  // Create all the BLAS from the vector of vectors of vk::GeometryNV
  // - There will be one BLAS per vector of vk::GeometryNV
  // - There will be as many BLAS there are items in the geoms vector
  // - The resulting BLAS are stored in m_blas
  //
  void buildBlas(const std::vector<std::vector<vk::GeometryNV>>& geoms,
                 vk::BuildAccelerationStructureFlagsNV flags = vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace)
  {
    m_blas.resize(geoms.size());

    vk::DeviceSize maxScratch{0};

    // Iterate over the groups of geometries, creating one BLAS for each group
    for(size_t i = 0; i < geoms.size(); i++)
    {
      Blas& blas{m_blas[i]};

      // Set the geometries that will be part of the BLAS
      blas.asInfo.setGeometryCount(static_cast<uint32_t>(geoms[i].size()));
      blas.asInfo.setPGeometries(geoms[i].data());
      blas.asInfo.flags = flags;
      vk::AccelerationStructureCreateInfoNV createinfo{0, blas.asInfo};

      // Create an acceleration structure identifier and allocate memory to store the
      // resulting structure data
      blas.as = m_alloc.createAcceleration(createinfo);
      m_debug.setObjectName(blas.as.accel, (std::string("Blas" + std::to_string(i)).c_str()));

      // Estimate the amount of scratch memory required to build the BLAS, and update the
      // size of the scratch buffer that will be allocated to sequentially build all BLASes
      vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{
          vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch, blas.as.accel};
      vk::DeviceSize scratchSize =
          m_device.getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo).memoryRequirements.size;

      maxScratch = std::max(maxScratch, scratchSize);
    }

    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
    nvvkBuffer scratchBuffer = m_alloc.createBuffer(maxScratch, vk::BufferUsageFlagBits::eRayTracingNV);

    // Create a command buffer containing all the BLAS builds
    nvvkpp::SingleCommandBuffer genCmdBuf(m_device, m_queueIndex);
    vk::CommandBuffer           cmdBuf = genCmdBuf.createCommandBuffer();
    for(auto& blas : m_blas)
    {
      cmdBuf.buildAccelerationStructureNV(blas.asInfo, nullptr, 0, VK_FALSE, blas.as.accel, nullptr, scratchBuffer.buffer, 0);
      // Since the scratch buffer is reused across builds, we need a barrier to ensure one build
      // is finished before starting the next one
      vk::MemoryBarrier barrier(vk::AccessFlagBits::eAccelerationStructureWriteNV, vk::AccessFlagBits::eAccelerationStructureReadNV);
      cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildNV,
                             vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, vk::DependencyFlags(), {barrier}, {}, {});
    }

    genCmdBuf.flushCommandBuffer(cmdBuf);
    m_alloc.destroy(scratchBuffer);
    m_alloc.flushStaging();
  }

  //--------------------------------------------------------------------------------------------------
  // Convert an Instance object into a VkGeometryInstanceNV

  VkGeometryInstanceNV instanceToVkGeometryInstanceNV(const Instance& instance)
  {
    Blas& blas{m_blas[instance.blasId]};
    // For each BLAS, fetch the acceleration structure handle that will allow the builder to
    // directly access it from the device
    uint64_t asHandle = 0;
    m_device.getAccelerationStructureHandleNV(blas.as.accel, sizeof(uint64_t), &asHandle);

    VkGeometryInstanceNV gInst{};
    // The matrices for the instance transforms are row-major, instead of column-major in the
    // rest of the application
    nvmath::mat4f transp = nvmath::transpose(instance.transform);
    // The gInst.transform value only contains 12 values, corresponding to a 4x3 matrix, hence
    // saving the last row that is anyway always (0,0,0,1). Since the matrix is row-major,
    // we simply copy the first 12 values of the original 4x4 matrix
    memcpy(gInst.transform, &transp, sizeof(gInst.transform));
    gInst.instanceId                  = instance.instanceId;
    gInst.mask                        = instance.mask;
    gInst.hitGroupId                  = instance.hitGroupId;
    gInst.flags                       = static_cast<uint32_t>(instance.flags);
    gInst.accelerationStructureHandle = asHandle;

    return gInst;
  }

  //--------------------------------------------------------------------------------------------------
  // Creating the top-level acceleration structure from the vector of Instance
  // - See struct of Instance
  // - The resulting TLAS will be stored in m_tlas
  //
  void buildTlas(const std::vector<Instance>&          instances,
                 vk::BuildAccelerationStructureFlagsNV flags = vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace)
  {
    // Set the instance count required to determine how much memory the TLAS will use
    m_tlas.asInfo.instanceCount = static_cast<uint32_t>(instances.size());
    m_tlas.asInfo.flags         = flags;
    vk::AccelerationStructureCreateInfoNV accelerationStructureInfo{0, m_tlas.asInfo};
    // Create the acceleration structure object and allocate the memory required to hold the TLAS data
    m_tlas.as = m_alloc.createAcceleration(accelerationStructureInfo);
    m_debug.setObjectName(m_tlas.as.accel, "Tlas");

    // Compute the amount of scratch memory required by the acceleration structure builder
    vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{
        vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch, m_tlas.as.accel};
    vk::DeviceSize scratchSize =
        m_device.getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo).memoryRequirements.size;

    // Allocate the scratch memory
    nvvkBuffer scratchBuffer = m_alloc.createBuffer(scratchSize, vk::BufferUsageFlagBits::eRayTracingNV);

    // For each instance, build the corresponding instance descriptor
    std::vector<VkGeometryInstanceNV> geometryInstances;
    geometryInstances.reserve(instances.size());
    for(const auto& inst : instances)
    {
      geometryInstances.push_back(instanceToVkGeometryInstanceNV(inst));
    }

    // Building the TLAS
    nvvkpp::SingleCommandBuffer genCmdBuf(m_device, m_queueIndex);
    vk::CommandBuffer           cmdBuf = genCmdBuf.createCommandBuffer();

    // Create a buffer holding the actual instance data for use by the AS builder
    VkDeviceSize instanceDescsSizeInBytes = instances.size() * sizeof(VkGeometryInstanceNV);

    // Allocate the instance buffer and copy its contents from host to device memory
    m_instBuffer = m_alloc.createBuffer(cmdBuf, geometryInstances, vk::BufferUsageFlagBits::eRayTracingNV);
    m_debug.setObjectName(m_instBuffer.buffer, "TLASInstances");

    // Make sure the copy of the instance buffer are copied before triggering the
    // acceleration structure build
    vk::MemoryBarrier barrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eAccelerationStructureWriteNV);
    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV,
                           vk::DependencyFlags(), {barrier}, {}, {});


    // Build the TLAS
    cmdBuf.buildAccelerationStructureNV(m_tlas.asInfo, m_instBuffer.buffer, 0, VK_FALSE, m_tlas.as.accel, nullptr,
                                        scratchBuffer.buffer, 0);

    genCmdBuf.flushCommandBuffer(cmdBuf);

    m_alloc.flushStaging();

    m_alloc.destroy(scratchBuffer);
  }

  //--------------------------------------------------------------------------------------------------
  // Refit the TLAS using new instance matrices
  //
  void updateTlasMatrices(const std::vector<Instance>& instances)
  {
    VkDeviceSize bufferSize = instances.size() * sizeof(VkGeometryInstanceNV);
    // Create a staging buffer on the host to upload the new instance data
    nvvkBuffer stagingBuffer = m_alloc.createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
#if defined(ALLOC_VMA)
                                                    VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU
#else
                                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
#endif
    );

    // Copy the instance data into the staging buffer
    auto* gInst = reinterpret_cast<VkGeometryInstanceNV*>(m_alloc.map(stagingBuffer));
    for(int i = 0; i < instances.size(); i++)
    {
      gInst[i] = instanceToVkGeometryInstanceNV(instances[i]);
    }
    m_alloc.unmap(stagingBuffer);

    // Compute the amount of scratch memory required by the AS builder to update the TLAS
    vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{
        vk::AccelerationStructureMemoryRequirementsTypeNV::eUpdateScratch, m_tlas.as.accel};
    vk::DeviceSize scratchSize =
        m_device.getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo).memoryRequirements.size;
    // Allocate the scratch buffer
    nvvkBuffer scratchBuffer = m_alloc.createBuffer(scratchSize, vk::BufferUsageFlagBits::eRayTracingNV);

    // Update the instance buffer on the device side and build the TLAS
    nvvkpp::SingleCommandBuffer genCmdBuf(m_device, m_queueIndex);
    vk::CommandBuffer           cmdBuf = genCmdBuf.createCommandBuffer();

    cmdBuf.copyBuffer(stagingBuffer.buffer, m_instBuffer.buffer, vk::BufferCopy(0, 0, bufferSize));

    // Make sure the copy of the instance buffer are copied before triggering the
    // acceleration structure build
    vk::MemoryBarrier barrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eAccelerationStructureWriteNV);
    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV,
                           vk::DependencyFlags(), {barrier}, {}, {});

    // Update the acceleration structure. Note the VK_TRUE parameter to trigger the update,
    // and the existing TLAS being passed and updated in place
    cmdBuf.buildAccelerationStructureNV(m_tlas.asInfo, m_instBuffer.buffer, 0, VK_TRUE, m_tlas.as.accel,
                                        m_tlas.as.accel, scratchBuffer.buffer, 0);
    genCmdBuf.flushCommandBuffer(cmdBuf);


    m_alloc.destroy(scratchBuffer);
    m_alloc.destroy(stagingBuffer);
  }

  //--------------------------------------------------------------------------------------------------
  // Refit the BLAS from updated buffers
  //
  void updateBlas(uint32_t blasIdx)
  {
    Blas& blas = m_blas[blasIdx];

    // Compute the amount of scratch memory required by the AS builder to update the TLAS
    vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{
        vk::AccelerationStructureMemoryRequirementsTypeNV::eUpdateScratch, blas.as.accel};
    vk::DeviceSize scratchSize =
        m_device.getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo).memoryRequirements.size;
    // Allocate the scratch buffer
    nvvkBuffer scratchBuffer = m_alloc.createBuffer(scratchSize, vk::BufferUsageFlagBits::eRayTracingNV);

    // Update the instance buffer on the device side and build the TLAS
    nvvkpp::SingleCommandBuffer genCmdBuf(m_device, m_queueIndex);
    vk::CommandBuffer           cmdBuf = genCmdBuf.createCommandBuffer();


    // Update the acceleration structure. Note the VK_TRUE parameter to trigger the update,
    // and the existing BLAS being passed and updated in place
    cmdBuf.buildAccelerationStructureNV(blas.asInfo, nullptr, 0, VK_TRUE, blas.as.accel, blas.as.accel, scratchBuffer.buffer, 0);

    genCmdBuf.flushCommandBuffer(cmdBuf);
    m_alloc.destroy(scratchBuffer);
  }

private:
  // Bottom-level acceleration structure
  struct Blas
  {
    nvvkAccel                       as;
    vk::AccelerationStructureInfoNV asInfo{vk::AccelerationStructureTypeNV::eBottomLevel};
    vk::GeometryNV                  geometry;
  };

  // Top-level acceleration structure
  struct Tlas
  {
    nvvkAccel                       as;
    vk::AccelerationStructureInfoNV asInfo{vk::AccelerationStructureTypeNV::eTopLevel};
  };

  //--------------------------------------------------------------------------------------------------
  // Vector containing all the BLASes built and referenced by the TLAS
  std::vector<Blas> m_blas;
  // Top-level acceleration structure
  Tlas m_tlas;
  // Instance buffer containing the matrices and BLAS ids
  nvvkBuffer m_instBuffer;

  vk::Device m_device;
  uint32_t   m_queueIndex{0};

  nvvkAllocator     m_alloc;
  nvvkpp::DebugUtil m_debug;
};

}  // namespace nvvkpp
