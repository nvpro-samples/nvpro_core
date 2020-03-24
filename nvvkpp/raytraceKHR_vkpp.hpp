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

#ifndef VK_KHR_ray_tracing
static_assert(0, "KHR_ray_tracing is required for this project!");
#endif  // ! VK_KHR_ray_tracing

namespace nvvkpp {
struct RaytracingBuilderKHR
{
#if defined(ALLOC_DEDICATED)
  // Use a simplistic allocator for conciseness
  using nvvkAccel           = nvvkpp::AccelerationDedicatedKHR;
  using nvvkBuffer          = nvvkpp::BufferDedicated;
  using nvvkAllocator       = nvvkpp::AllocatorDedicated;
  using nvvkMemoryAllocator = vk::PhysicalDevice;
#elif defined(ALLOC_VMA)
  using nvvkAccel           = nvvkpp::AccelerationVma;
  using nvvkBuffer          = nvvkpp::BufferVma;
  using nvvkAllocator       = nvvkpp::AllocatorVma;
  using nvvkMemoryAllocator = VmaAllocator;
#elif defined(ALLOC_DMA)
  using nvvkAccel           = nvvkpp::AccelerationDmaKHR;
  using nvvkBuffer          = nvvkpp::BufferDma;
  using nvvkAllocator       = nvvkpp::AllocatorDma;
  using nvvkMemoryAllocator = nvvk::DeviceMemoryAllocator;
#endif

  RaytracingBuilderKHR() = default;

  // Bottom-level acceleration structure
  struct Blas
  {
    nvvkAccel                              as;     // VkAccelerationStructureKHR
    vk::BuildAccelerationStructureFlagsKHR flags;  // specifying additional parameters for acceleration structure
                                                   // builds
    std::vector<vk::AccelerationStructureCreateGeometryTypeInfoKHR> asCreateGeometryInfo;  // specifies the shape of geometries that will be
        // built into an acceleration structure
    std::vector<vk::AccelerationStructureGeometryKHR> asGeometry;  // data used to build acceleration structure geometry
    std::vector<vk::AccelerationStructureBuildOffsetInfoKHR> asBuildOffsetInfo;
  };

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
    uint32_t                     blasId{0};      // Index of the BLAS in m_blas
    uint32_t                     instanceId{0};  // Instance Index (gl_InstanceID)
    uint32_t                     hitGroupId{0};  // Hit group index in the SBT
    uint32_t                     mask{0xFF};     // Visibility mask, will be AND-ed with ray mask
    vk::GeometryInstanceFlagsKHR flags{vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable};
    nvmath::mat4f                transform{nvmath::mat4f(1)};  // Identity
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
    m_blas.clear();
    m_tlas = {};
  }

  // Returning the constructed top-level acceleration structure
  const vk::AccelerationStructureKHR& getAccelerationStructure() { return m_tlas.as.accel; }

  //--------------------------------------------------------------------------------------------------
  // Create all the BLAS from the vector of vectors of vk::GeometryNV
  // - There will be one BLAS per vector of vk::GeometryNV
  // - There will be as many BLAS there are items in the geoms vector
  // - The resulting BLAS are stored in m_blas
  //
  void buildBlas(const std::vector<RaytracingBuilderKHR::Blas>& blas_,
                 vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
  {
    m_blas = blas_;  // Keeping a copy

    vk::DeviceSize maxScratch{0};

    // Iterate over the groups of geometries, creating one BLAS for each group
    int idx{0};
    for(auto& blas : m_blas)
    {
      vk::AccelerationStructureCreateInfoKHR asCreateInfo{{}, vk::AccelerationStructureTypeKHR::eBottomLevel};
      asCreateInfo.setFlags(flags);
      asCreateInfo.setMaxGeometryCount((uint32_t)blas.asCreateGeometryInfo.size());
      asCreateInfo.setPGeometryInfos(blas.asCreateGeometryInfo.data());

      // Create an acceleration structure identifier and allocate memory to
      // store the resulting structure data
      blas.as = m_alloc.createAcceleration(asCreateInfo);
      m_debug.setObjectName(blas.as.accel, (std::string("Blas" + std::to_string(idx)).c_str()));

      // Estimate the amount of scratch memory required to build the BLAS, and
      // update the size of the scratch buffer that will be allocated to
      // sequentially build all BLASes
      vk::AccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo{
          vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch,
          vk::AccelerationStructureBuildTypeKHR::eDevice, blas.as.accel};
      vk::DeviceSize scratchSize =
          m_device.getAccelerationStructureMemoryRequirementsKHR(memoryRequirementsInfo).memoryRequirements.size;

      blas.flags = flags;
      maxScratch = std::max(maxScratch, scratchSize);
      idx++;
    }

    // Allocate the scratch buffers holding the temporary data of the
    // acceleration structure builder
    nvvkBuffer        scratchBuffer  = m_alloc.createBuffer(maxScratch, vk::BufferUsageFlagBits::eRayTracingKHR
                                                                    | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    vk::DeviceAddress scratchAddress = m_device.getBufferAddress({scratchBuffer.buffer});

    // Create a command buffer containing all the BLAS builds
    nvvkpp::SingleCommandBuffer genCmdBuf(m_device, m_queueIndex);
    vk::CommandBuffer           cmdBuf = genCmdBuf.createCommandBuffer();

    for(auto& blas : m_blas)
    {
      const vk::AccelerationStructureGeometryKHR*   pGeometry = blas.asGeometry.data();
      vk::AccelerationStructureBuildGeometryInfoKHR bottomASInfo{vk::AccelerationStructureTypeKHR::eBottomLevel};
      bottomASInfo.setFlags(flags);
      bottomASInfo.setUpdate(VK_FALSE);
      bottomASInfo.setSrcAccelerationStructure({});
      bottomASInfo.setDstAccelerationStructure(blas.as.accel);
      bottomASInfo.setGeometryArrayOfPointers(VK_FALSE);
      bottomASInfo.setGeometryCount((uint32_t)blas.asGeometry.size());
      bottomASInfo.setPpGeometries(&pGeometry);
      bottomASInfo.setScratchData(scratchAddress);

      // Pointers of offset
      std::vector<const vk::AccelerationStructureBuildOffsetInfoKHR*> pBuildOffset(blas.asBuildOffsetInfo.size());
      for(size_t i = 0; i < blas.asBuildOffsetInfo.size(); i++)
        pBuildOffset[i] = &blas.asBuildOffsetInfo[i];

      // Building the AS
      cmdBuf.buildAccelerationStructureKHR(bottomASInfo, pBuildOffset);

      // Since the scratch buffer is reused across builds, we need a barrier to
      // ensure one build is finished before starting the next one
      vk::MemoryBarrier barrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR, vk::AccessFlagBits::eAccelerationStructureWriteKHR);
      cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                             vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {}, {barrier}, {}, {});
    }

    genCmdBuf.flushCommandBuffer(cmdBuf);
    m_alloc.destroy(scratchBuffer);
    m_alloc.flushStaging();
  }

  //--------------------------------------------------------------------------------------------------
  // Convert an Instance object into a VkGeometryInstanceNV

  vk::AccelerationStructureInstanceKHR instanceToVkGeometryInstanceKHR(const Instance& instance)
  {
    Blas& blas{m_blas[instance.blasId]};

    vk::AccelerationStructureInstanceKHR gInst{};
    // The matrices for the instance transforms are row-major, instead of
    // column-major in the rest of the application
    nvmath::mat4f transp = nvmath::transpose(instance.transform);
    // The gInst.transform value only contains 12 values, corresponding to a 4x3
    // matrix, hence saving the last row that is anyway always (0,0,0,1). Since
    // the matrix is row-major, we simply copy the first 12 values of the
    // original 4x4 matrix
    memcpy(&gInst.transform, &transp, sizeof(gInst.transform));
    gInst.setInstanceCustomIndex(instance.instanceId);
    gInst.setMask(instance.mask);
    gInst.setInstanceShaderBindingTableRecordOffset(instance.hitGroupId);
    gInst.setFlags(instance.flags);
    gInst.setAccelerationStructureReference(m_device.getAccelerationStructureAddressKHR({blas.as.accel}));
    return gInst;
  }

  //--------------------------------------------------------------------------------------------------
  // Creating the top-level acceleration structure from the vector of Instance
  // - See struct of Instance
  // - The resulting TLAS will be stored in m_tlas
  //
  void buildTlas(const std::vector<Instance>&           instances,
                 vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
  {
    m_tlas.flags = flags;

    {
      vk::AccelerationStructureCreateGeometryTypeInfoKHR geometryCreate{vk::GeometryTypeKHR::eInstances};
      geometryCreate.setMaxPrimitiveCount(static_cast<uint32_t>(instances.size()));
      geometryCreate.setAllowsTransforms(VK_TRUE);

      vk::AccelerationStructureCreateInfoKHR asCreateInfo{{}, vk::AccelerationStructureTypeKHR::eTopLevel};
      asCreateInfo.setFlags(flags);
      asCreateInfo.setMaxGeometryCount(1);
      asCreateInfo.setPGeometryInfos(&geometryCreate);

      // Create the acceleration structure object and allocate the memory
      // required to hold the TLAS data
      m_tlas.as = m_alloc.createAcceleration(asCreateInfo);
      m_debug.setObjectName(m_tlas.as.accel, "Tlas");
    }

    // Compute the amount of scratch memory required by the acceleration
    // structure builder
    vk::AccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo{
        vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch,
        vk::AccelerationStructureBuildTypeKHR::eDevice, m_tlas.as.accel};
    vk::DeviceSize scratchSize =
        m_device.getAccelerationStructureMemoryRequirementsKHR(memoryRequirementsInfo).memoryRequirements.size;

    // Allocate the scratch memory
    nvvkBuffer        scratchBuffer  = m_alloc.createBuffer(scratchSize, vk::BufferUsageFlagBits::eRayTracingKHR
                                                                     | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    vk::DeviceAddress scratchAddress = m_device.getBufferAddress({scratchBuffer.buffer});

    // For each instance, build the corresponding instance descriptor
    std::vector<vk::AccelerationStructureInstanceKHR> geometryInstances;
    geometryInstances.reserve(instances.size());
    for(const auto& inst : instances)
    {
      geometryInstances.push_back(instanceToVkGeometryInstanceKHR(inst));
    }

    vk::AccelerationStructureCreateGeometryTypeInfoKHR geometryCreate{vk::GeometryTypeKHR::eInstances};
    geometryCreate.setMaxPrimitiveCount(static_cast<uint32_t>(instances.size()));
    geometryCreate.setAllowsTransforms(VK_TRUE);

    // Building the TLAS
    nvvkpp::SingleCommandBuffer genCmdBuf(m_device, m_queueIndex);
    vk::CommandBuffer           cmdBuf = genCmdBuf.createCommandBuffer();

    // Create a buffer holding the actual instance data for use by the AS
    // builder
    VkDeviceSize instanceDescsSizeInBytes = instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);

    // Allocate the instance buffer and copy its contents from host to device
    // memory
    m_instBuffer = m_alloc.createBuffer(cmdBuf, geometryInstances,
                                        vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    m_debug.setObjectName(m_instBuffer.buffer, "TLASInstances");
    vk::DeviceAddress instanceAddress = m_device.getBufferAddress(m_instBuffer.buffer);

    // Make sure the copy of the instance buffer are copied before triggering
    // the acceleration structure build
    vk::MemoryBarrier barrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eAccelerationStructureWriteKHR);
    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                           vk::DependencyFlags(), {barrier}, {}, {});

    // Build the TLAS
    vk::AccelerationStructureGeometryKHR topASGeometry{vk::GeometryTypeKHR::eInstances};
    topASGeometry.geometry.instances.setArrayOfPointers(VK_FALSE);
    topASGeometry.geometry.instances.setData(instanceAddress);

    const vk::AccelerationStructureGeometryKHR*   pGeometry = &topASGeometry;
    vk::AccelerationStructureBuildGeometryInfoKHR topASInfo;
    topASInfo.setFlags(flags);
    topASInfo.setUpdate(VK_FALSE);
    topASInfo.setSrcAccelerationStructure({});
    topASInfo.setDstAccelerationStructure(m_tlas.as.accel);
    topASInfo.setGeometryArrayOfPointers(VK_FALSE);
    topASInfo.setGeometryCount(1);
    topASInfo.setPpGeometries(&pGeometry);
    topASInfo.setScratchData(scratchAddress);

    // Build Offsets info: n instances
    vk::AccelerationStructureBuildOffsetInfoKHR buildOffsetInfo{static_cast<uint32_t>(instances.size()), 0, 0, 0};
    const vk::AccelerationStructureBuildOffsetInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

    // Build the TLAS
    cmdBuf.buildAccelerationStructureKHR(1, &topASInfo, &pBuildOffsetInfo);

    genCmdBuf.flushCommandBuffer(cmdBuf);
    m_alloc.flushStaging();
    m_alloc.destroy(scratchBuffer);
  }

  //--------------------------------------------------------------------------------------------------
  // Refit the TLAS using new instance matrices
  //
  void updateTlasMatrices(const std::vector<Instance>& instances)
  {
    VkDeviceSize bufferSize = instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
    // Create a staging buffer on the host to upload the new instance data
    nvvkBuffer stagingBuffer = m_alloc.createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
#if defined(ALLOC_VMA)
                                                    VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU
#else
                                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
#endif
    );

    // Copy the instance data into the staging buffer
    auto* gInst = reinterpret_cast<vk::AccelerationStructureInstanceKHR*>(m_alloc.map(stagingBuffer));
    for(int i = 0; i < instances.size(); i++)
    {
      gInst[i] = instanceToVkGeometryInstanceKHR(instances[i]);
    }
    m_alloc.unmap(stagingBuffer);

    // Compute the amount of scratch memory required by the AS builder to update
    // the TLAS
    vk::AccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo{
        vk::AccelerationStructureMemoryRequirementsTypeKHR::eUpdateScratch,
        vk::AccelerationStructureBuildTypeKHR::eDevice, m_tlas.as.accel};
    vk::DeviceSize scratchSize =
        m_device.getAccelerationStructureMemoryRequirementsKHR(memoryRequirementsInfo).memoryRequirements.size;
    // Allocate the scratch buffer
    nvvkBuffer        scratchBuffer  = m_alloc.createBuffer(scratchSize, vk::BufferUsageFlagBits::eRayTracingKHR
                                                                     | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    vk::DeviceAddress scratchAddress = m_device.getBufferAddress({scratchBuffer.buffer});

    // Update the instance buffer on the device side and build the TLAS
    nvvkpp::SingleCommandBuffer genCmdBuf(m_device, m_queueIndex);
    vk::CommandBuffer           cmdBuf = genCmdBuf.createCommandBuffer();

    cmdBuf.copyBuffer(stagingBuffer.buffer, m_instBuffer.buffer, vk::BufferCopy(0, 0, bufferSize));

    vk::DeviceAddress instanceAddress = m_device.getBufferAddress(m_instBuffer.buffer);

    // Make sure the copy of the instance buffer are copied before triggering
    // the acceleration structure build
    vk::MemoryBarrier barrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eAccelerationStructureWriteKHR);
    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                           vk::DependencyFlags(), {barrier}, {}, {});

    vk::AccelerationStructureGeometryKHR topASGeometry{vk::GeometryTypeKHR::eInstances};
    topASGeometry.geometry.instances.arrayOfPointers      = VK_FALSE;
    topASGeometry.geometry.instances.data                 = instanceAddress;
    const vk::AccelerationStructureGeometryKHR* pGeometry = &topASGeometry;

    vk::AccelerationStructureBuildGeometryInfoKHR topASInfo;
    topASInfo.setFlags(m_tlas.flags);
    topASInfo.setUpdate(VK_TRUE);
    topASInfo.setSrcAccelerationStructure(m_tlas.as.accel);
    topASInfo.setDstAccelerationStructure(m_tlas.as.accel);
    topASInfo.setGeometryArrayOfPointers(VK_FALSE);
    topASInfo.setGeometryCount(1);
    topASInfo.setPpGeometries(&pGeometry);
    topASInfo.setScratchData(scratchAddress);

    uint32_t                                           nbInstances      = (uint32_t)instances.size();
    vk::AccelerationStructureBuildOffsetInfoKHR        buildOffsetInfo  = {nbInstances, 0, 0, 0};
    const vk::AccelerationStructureBuildOffsetInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

    // Build the TLAS

    // Update the acceleration structure. Note the VK_TRUE parameter to trigger
    // the update, and the existing TLAS being passed and updated in place
    cmdBuf.buildAccelerationStructureKHR(1, &topASInfo, &pBuildOffsetInfo);
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

    // Compute the amount of scratch memory required by the AS builder to update
    // the TLAS
    vk::AccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo{
        vk::AccelerationStructureMemoryRequirementsTypeKHR::eUpdateScratch,
        vk::AccelerationStructureBuildTypeKHR::eDevice, blas.as.accel};
    vk::DeviceSize scratchSize =
        m_device.getAccelerationStructureMemoryRequirementsKHR(memoryRequirementsInfo).memoryRequirements.size;
    // Allocate the scratch buffer
    nvvkBuffer        scratchBuffer  = m_alloc.createBuffer(scratchSize, vk::BufferUsageFlagBits::eRayTracingKHR
                                                                     | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    vk::DeviceAddress scratchAddress = m_device.getBufferAddress({scratchBuffer.buffer});

    const vk::AccelerationStructureGeometryKHR*   pGeometry = blas.asGeometry.data();
    vk::AccelerationStructureBuildGeometryInfoKHR asInfo{vk::AccelerationStructureTypeKHR::eBottomLevel};
    asInfo.setFlags(blas.flags);
    asInfo.setUpdate(VK_TRUE);
    asInfo.setSrcAccelerationStructure(blas.as.accel);
    asInfo.setDstAccelerationStructure(blas.as.accel);
    asInfo.setGeometryArrayOfPointers(VK_FALSE);
    asInfo.setGeometryCount((uint32_t)blas.asGeometry.size());
    asInfo.setPpGeometries(&pGeometry);
    asInfo.setScratchData(scratchAddress);

    std::vector<const vk::AccelerationStructureBuildOffsetInfoKHR*> pBuildOffset(blas.asBuildOffsetInfo.size());
    for(size_t i = 0; i < blas.asBuildOffsetInfo.size(); i++)
      pBuildOffset[i] = &blas.asBuildOffsetInfo[i];

    // Update the instance buffer on the device side and build the TLAS
    nvvkpp::SingleCommandBuffer genCmdBuf(m_device, m_queueIndex);
    vk::CommandBuffer           cmdBuf = genCmdBuf.createCommandBuffer();

    // Update the acceleration structure. Note the VK_TRUE parameter to trigger
    // the update, and the existing BLAS being passed and updated in place
    cmdBuf.buildAccelerationStructureKHR(asInfo, pBuildOffset);

    genCmdBuf.flushCommandBuffer(cmdBuf);
    m_alloc.destroy(scratchBuffer);
  }

private:
  // Top-level acceleration structure
  struct Tlas
  {
    nvvkAccel                              as;
    vk::AccelerationStructureCreateInfoKHR asInfo{{}, vk::AccelerationStructureTypeKHR::eTopLevel};
    vk::BuildAccelerationStructureFlagsKHR flags;
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
