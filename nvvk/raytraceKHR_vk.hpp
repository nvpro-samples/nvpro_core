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

# class nvvk::RaytracingBuilderKHR

Base functionality of raytracing

This class does not implement all what you need to do raytracing, but
helps creating the BLAS and TLAS, which then can be used by different
raytracing usage.

# Setup and Usage
~~~~ C++
m_rtBuilder.setup(device, memoryAllocator, queueIndex);
// Create array of VkGeometryNV
m_rtBuilder.buildBlas(allBlas);
// Create array of RaytracingBuilder::instance
m_rtBuilder.buildTlas(instances);
// Retrieve the acceleration structure
const VkAccelerationStructureNV& tlas = m.rtBuilder.getAccelerationStructure()
~~~~
*/


#include <mutex>
#include <vulkan/vulkan.h>

#include "allocator_vk.hpp"
#include "commands_vk.hpp"
#include "debug_util_vk.hpp"
#include "nvh/nvprint.hpp"
#include "nvmath/nvmath.h"

#include "nvh/nvprint.hpp"

namespace nvvk {
struct RaytracingBuilderKHR
{
  RaytracingBuilderKHR(RaytracingBuilderKHR const&) = delete;
  RaytracingBuilderKHR& operator=(RaytracingBuilderKHR const&) = delete;

  RaytracingBuilderKHR() = default;

  // Bottom-level acceleration structure
  struct Blas
  {
    nvvk::AccelKHR                       as;     // VkAccelerationStructureKHR
    VkBuildAccelerationStructureFlagsKHR flags;  // specifying additional parameters for acceleration structure
                                                 // builds
    std::vector<VkAccelerationStructureCreateGeometryTypeInfoKHR> asCreateGeometryInfo;  // specifies the shape of geometries that will be
        // built into an acceleration structure
    std::vector<VkAccelerationStructureGeometryKHR> asGeometry;  // data used to build acceleration structure geometry
    std::vector<VkAccelerationStructureBuildOffsetInfoKHR> asBuildOffsetInfo;
  };

  //--------------------------------------------------------------------------------------------------
  // Initializing the allocator and querying the raytracing properties
  //
  void setup(const VkDevice& device, nvvk::Allocator* allocator, uint32_t queueIndex)
  {
    m_device     = device;
    m_queueIndex = queueIndex;
    m_debug.setup(device);
    m_alloc = allocator;
  }

  // This is an instance of a BLAS
  struct Instance
  {
    uint32_t                   blasId{0};      // Index of the BLAS in m_blas
    uint32_t                   instanceId{0};  // Instance Index (gl_InstanceID)
    uint32_t                   hitGroupId{0};  // Hit group index in the SBT
    uint32_t                   mask{0xFF};     // Visibility mask, will be AND-ed with ray mask
    VkGeometryInstanceFlagsKHR flags{VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR};
    nvmath::mat4f              transform{nvmath::mat4f(1)};  // Identity
  };

  //--------------------------------------------------------------------------------------------------
  // Destroying all allocations
  //
  void destroy()
  {
    for(auto& b : m_blas)
    {
      m_alloc->destroy(b.as);
    }
    m_alloc->destroy(m_tlas.as);
    m_alloc->destroy(m_instBuffer);
    m_blas.clear();
    m_tlas = {};
  }

  // Returning the constructed top-level acceleration structure
  VkAccelerationStructureKHR getAccelerationStructure() const { return m_tlas.as.accel; }

  //--------------------------------------------------------------------------------------------------
  // Create all the BLAS from the vector of vectors of VkGeometryNV
  // - There will be one BLAS per vector of VkGeometryNV
  // - There will be as many BLAS there are items in the geoms vector
  // - The resulting BLAS are stored in m_blas
  //
  void buildBlas(const std::vector<RaytracingBuilderKHR::Blas>& blas_,
                 VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR)
  {
    m_blas = blas_;  // Keeping a copy

    VkDeviceSize maxScratch{0};  // Largest scratch buffer for our BLAS

    // Is compaction requested?
    bool doCompaction = (flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR)
                        == VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    std::vector<VkDeviceSize> originalSizes;
    originalSizes.resize(m_blas.size());

    // Iterate over the groups of geometries, creating one BLAS for each group
    int idx{0};
    for(auto& blas : m_blas)
    {
      VkAccelerationStructureCreateInfoKHR asCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
      asCreateInfo.type             = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      asCreateInfo.flags            = flags;
      asCreateInfo.maxGeometryCount = (uint32_t)blas.asCreateGeometryInfo.size();
      asCreateInfo.pGeometryInfos   = blas.asCreateGeometryInfo.data();

      // Create an acceleration structure identifier and allocate memory to
      // store the resulting structure data
      blas.as = m_alloc->createAcceleration(asCreateInfo);
      m_debug.setObjectName(blas.as.accel, (std::string("Blas" + std::to_string(idx)).c_str()));

      // Estimate the amount of scratch memory required to build the BLAS, and
      // update the size of the scratch buffer that will be allocated to
      // sequentially build all BLASes
      VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo{
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR};
      memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
      memoryRequirementsInfo.accelerationStructure = blas.as.accel;
      memoryRequirementsInfo.buildType             = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

      VkMemoryRequirements2 reqMem{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
      vkGetAccelerationStructureMemoryRequirementsKHR(m_device, &memoryRequirementsInfo, &reqMem);
      VkDeviceSize scratchSize = reqMem.memoryRequirements.size;


      blas.flags = flags;
      maxScratch = std::max(maxScratch, scratchSize);

      // Original size
      memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
      vkGetAccelerationStructureMemoryRequirementsKHR(m_device, &memoryRequirementsInfo, &reqMem);
      originalSizes[idx] = reqMem.memoryRequirements.size;

      idx++;
    }

    // Allocate the scratch buffers holding the temporary data of the
    // acceleration structure builder

    nvvk::Buffer scratchBuffer =
        m_alloc->createBuffer(maxScratch, VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    bufferInfo.buffer              = scratchBuffer.buffer;
    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(m_device, &bufferInfo);


    // Query size of compact BLAS
    VkQueryPoolCreateInfo qpci{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    qpci.queryCount = (uint32_t)m_blas.size();
    qpci.queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    VkQueryPool queryPool;
    vkCreateQueryPool(m_device, &qpci, nullptr, &queryPool);


    // Create a command buffer containing all the BLAS builds
    nvvk::CommandPool genCmdBuf(m_device, m_queueIndex);
    VkCommandBuffer   cmdBuf = genCmdBuf.createCommandBuffer();
    int               ctr{0};
    for(auto& blas : m_blas)
    {
      const VkAccelerationStructureGeometryKHR* pGeometry = blas.asGeometry.data();
      VkAccelerationStructureBuildGeometryInfoKHR bottomASInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
      bottomASInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      bottomASInfo.flags                     = flags;
      bottomASInfo.update                    = VK_FALSE;
      bottomASInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
      bottomASInfo.dstAccelerationStructure  = blas.as.accel;
      bottomASInfo.geometryArrayOfPointers   = VK_FALSE;
      bottomASInfo.geometryCount             = (uint32_t)blas.asGeometry.size();
      bottomASInfo.ppGeometries              = &pGeometry;
      bottomASInfo.scratchData.deviceAddress = scratchAddress;

      // Pointers of offset
      std::vector<const VkAccelerationStructureBuildOffsetInfoKHR*> pBuildOffset(blas.asBuildOffsetInfo.size());
      for(size_t i = 0; i < blas.asBuildOffsetInfo.size(); i++)
        pBuildOffset[i] = &blas.asBuildOffsetInfo[i];

      // Building the AS
      vkCmdBuildAccelerationStructureKHR(cmdBuf, 1, &bottomASInfo, pBuildOffset.data());

      // Since the scratch buffer is reused across builds, we need a barrier to ensure one build
      // is finished before starting the next one
      VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
      barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
      barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
      vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                           VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

      // Query the compact size
      if(doCompaction)
      {
        vkCmdWriteAccelerationStructuresPropertiesKHR(cmdBuf, 1, &blas.as.accel,
                                                      VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, ctr++);
      }
    }
    genCmdBuf.submitAndWait(cmdBuf);


    // Compacting all BLAS
    if(doCompaction)
    {
      cmdBuf = genCmdBuf.createCommandBuffer();

      // Get the size result back
      std::vector<VkDeviceSize> compactSizes(m_blas.size());
      vkGetQueryPoolResults(m_device, queryPool, 0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize),
                            compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);


      // Compacting
      std::vector<nvvk::AccelKHR> cleanupAS(m_blas.size());
      uint32_t                    totOriginalSize{0}, totCompactSize{0};
      for(int i = 0; i < m_blas.size(); i++)
      {
        LOGI("Reducing %i, from %d to %d \n", i, originalSizes[i], compactSizes[i]);
        totOriginalSize += (uint32_t)originalSizes[i];
        totCompactSize += (uint32_t)compactSizes[i];

        // Creating a compact version of the AS
        VkAccelerationStructureCreateInfoKHR asCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        asCreateInfo.compactedSize = compactSizes[i];
        asCreateInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        asCreateInfo.flags         = flags;
        auto as                    = m_alloc->createAcceleration(asCreateInfo);

        // Copy the original BLAS to a compact version
        VkCopyAccelerationStructureInfoKHR copyInfo{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
        copyInfo.src  = m_blas[i].as.accel;
        copyInfo.dst  = as.accel;
        copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
        vkCmdCopyAccelerationStructureKHR(cmdBuf, &copyInfo);
        cleanupAS[i] = m_blas[i].as;
        m_blas[i].as = as;
      }
      genCmdBuf.submitAndWait(cmdBuf);

      // Destroying the previous version
      for(auto as : cleanupAS)
        m_alloc->destroy(as);

      LOGI("------------------\n");
      LOGI("Total: %d -> %d = %d (%2.2f%s smaller) \n", totOriginalSize, totCompactSize,
           totOriginalSize - totCompactSize, (totOriginalSize - totCompactSize) / float(totOriginalSize) * 100.f, "%%");
    }

    vkDestroyQueryPool(m_device, queryPool, nullptr);
    m_alloc->destroy(scratchBuffer);
    m_alloc->finalizeAndReleaseStaging();
  }

  //--------------------------------------------------------------------------------------------------
  // Convert an Instance object into a VkGeometryInstanceNV

  VkAccelerationStructureInstanceKHR instanceToVkGeometryInstanceKHR(const Instance& instance)
  {
    Blas& blas{m_blas[instance.blasId]};

    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    addressInfo.accelerationStructure = blas.as.accel;
    VkDeviceAddress blasAddress       = vkGetAccelerationStructureDeviceAddressKHR(m_device, &addressInfo);

    VkAccelerationStructureInstanceKHR gInst{};
    // The matrices for the instance transforms are row-major, instead of
    // column-major in the rest of the application
    nvmath::mat4f transp = nvmath::transpose(instance.transform);
    // The gInst.transform value only contains 12 values, corresponding to a 4x3
    // matrix, hence saving the last row that is anyway always (0,0,0,1). Since
    // the matrix is row-major, we simply copy the first 12 values of the
    // original 4x4 matrix
    memcpy(&gInst.transform, &transp, sizeof(gInst.transform));
    gInst.instanceCustomIndex                    = instance.instanceId;
    gInst.mask                                   = instance.mask;
    gInst.instanceShaderBindingTableRecordOffset = instance.hitGroupId;
    gInst.flags                                  = instance.flags;
    gInst.accelerationStructureReference         = blasAddress;
    return gInst;
  }

  //--------------------------------------------------------------------------------------------------
  // Creating the top-level acceleration structure from the vector of Instance
  // - See struct of Instance
  // - The resulting TLAS will be stored in m_tlas
  //
  void buildTlas(const std::vector<Instance>&         instances,
                 VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR)
  {
    m_tlas.flags = flags;

    VkAccelerationStructureCreateGeometryTypeInfoKHR geometryCreate{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR};
    geometryCreate.geometryType      = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometryCreate.maxPrimitiveCount = (static_cast<uint32_t>(instances.size()));
    geometryCreate.allowsTransforms  = (VK_TRUE);

    VkAccelerationStructureCreateInfoKHR asCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    asCreateInfo.type             = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    asCreateInfo.flags            = flags;
    asCreateInfo.maxGeometryCount = 1;
    asCreateInfo.pGeometryInfos   = &geometryCreate;

    // Create the acceleration structure object and allocate the memory
    // required to hold the TLAS data
    m_tlas.as = m_alloc->createAcceleration(asCreateInfo);
    m_debug.setObjectName(m_tlas.as.accel, "Tlas");

    // Compute the amount of scratch memory required by the acceleration structure builder
    VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR};
    memoryRequirementsInfo.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
    memoryRequirementsInfo.accelerationStructure = m_tlas.as.accel;
    memoryRequirementsInfo.buildType             = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    VkMemoryRequirements2 reqMem{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    vkGetAccelerationStructureMemoryRequirementsKHR(m_device, &memoryRequirementsInfo, &reqMem);
    VkDeviceSize scratchSize = reqMem.memoryRequirements.size;


    // Allocate the scratch memory
    nvvk::Buffer scratchBuffer =
        m_alloc->createBuffer(scratchSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    bufferInfo.buffer              = scratchBuffer.buffer;
    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(m_device, &bufferInfo);


    // For each instance, build the corresponding instance descriptor
    std::vector<VkAccelerationStructureInstanceKHR> geometryInstances;
    geometryInstances.reserve(instances.size());
    for(const auto& inst : instances)
    {
      geometryInstances.push_back(instanceToVkGeometryInstanceKHR(inst));
    }

    // Building the TLAS
    nvvk::CommandPool genCmdBuf(m_device, m_queueIndex);
    VkCommandBuffer   cmdBuf = genCmdBuf.createCommandBuffer();

    // Create a buffer holding the actual instance data for use by the AS
    // builder
    VkDeviceSize instanceDescsSizeInBytes = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);

    // Allocate the instance buffer and copy its contents from host to device
    // memory
    m_instBuffer = m_alloc->createBuffer(cmdBuf, geometryInstances,
                                         VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    m_debug.setObjectName(m_instBuffer.buffer, "TLASInstances");
    //VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    bufferInfo.buffer               = m_instBuffer.buffer;
    VkDeviceAddress instanceAddress = vkGetBufferDeviceAddress(m_device, &bufferInfo);

    // Make sure the copy of the instance buffer are copied before triggering the
    // acceleration structure build
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);

    // Build the TLAS
    VkAccelerationStructureGeometryDataKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    geometry.instances.arrayOfPointers    = VK_FALSE;
    geometry.instances.data.deviceAddress = instanceAddress;
    VkAccelerationStructureGeometryKHR topASGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    topASGeometry.geometry     = geometry;


    const VkAccelerationStructureGeometryKHR* pGeometry = &topASGeometry;
    VkAccelerationStructureBuildGeometryInfoKHR topASInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    topASInfo.flags                     = flags;
    topASInfo.update                    = VK_FALSE;
    topASInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
    topASInfo.dstAccelerationStructure  = m_tlas.as.accel;
    topASInfo.geometryArrayOfPointers   = VK_FALSE;
    topASInfo.geometryCount             = 1;
    topASInfo.ppGeometries              = &pGeometry;
    topASInfo.scratchData.deviceAddress = scratchAddress;

    // Build Offsets info: n instances
    VkAccelerationStructureBuildOffsetInfoKHR        buildOffsetInfo{static_cast<uint32_t>(instances.size()), 0, 0, 0};
    const VkAccelerationStructureBuildOffsetInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

    // Build the TLAS
    vkCmdBuildAccelerationStructureKHR(cmdBuf, 1, &topASInfo, &pBuildOffsetInfo);


    genCmdBuf.submitAndWait(cmdBuf);
    m_alloc->finalizeAndReleaseStaging();
    m_alloc->destroy(scratchBuffer);
  }

  //--------------------------------------------------------------------------------------------------
  // Refit the TLAS using new instance matrices
  //
  void updateTlasMatrices(const std::vector<Instance>& instances)
  {
    VkDeviceSize bufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    // Create a staging buffer on the host to upload the new instance data
    nvvk::Buffer stagingBuffer = m_alloc->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
#if defined(NVVK_ALLOC_VMA)
                                                       VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU
#else
                                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
#endif
    );

    // Copy the instance data into the staging buffer
    auto* gInst = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(m_alloc->map(stagingBuffer));
    for(int i = 0; i < instances.size(); i++)
    {
      gInst[i] = instanceToVkGeometryInstanceKHR(instances[i]);
    }
    m_alloc->unmap(stagingBuffer);

    // Compute the amount of scratch memory required by the AS builder to update
    VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR};
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_KHR;
    memoryRequirementsInfo.accelerationStructure = m_tlas.as.accel;
    memoryRequirementsInfo.buildType             = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    VkMemoryRequirements2 reqMem{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    vkGetAccelerationStructureMemoryRequirementsKHR(m_device, &memoryRequirementsInfo, &reqMem);
    VkDeviceSize scratchSize = reqMem.memoryRequirements.size;

    // Allocate the scratch buffer
    nvvk::Buffer scratchBuffer =
        m_alloc->createBuffer(scratchSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    bufferInfo.buffer              = scratchBuffer.buffer;
    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(m_device, &bufferInfo);


    // Update the instance buffer on the device side and build the TLAS
    nvvk::CommandPool genCmdBuf(m_device, m_queueIndex);
    VkCommandBuffer   cmdBuf = genCmdBuf.createCommandBuffer();

    VkBufferCopy region{0, 0, bufferSize};
    vkCmdCopyBuffer(cmdBuf, stagingBuffer.buffer, m_instBuffer.buffer, 1, &region);

    //VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    bufferInfo.buffer               = m_instBuffer.buffer;
    VkDeviceAddress instanceAddress = vkGetBufferDeviceAddress(m_device, &bufferInfo);


    // Make sure the copy of the instance buffer are copied before triggering the
    // acceleration structure build
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);


    VkAccelerationStructureGeometryDataKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    geometry.instances.arrayOfPointers    = VK_FALSE;
    geometry.instances.data.deviceAddress = instanceAddress;
    VkAccelerationStructureGeometryKHR topASGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    topASGeometry.geometry     = geometry;

    const VkAccelerationStructureGeometryKHR* pGeometry = &topASGeometry;


    VkAccelerationStructureBuildGeometryInfoKHR topASInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    topASInfo.flags                     = m_tlas.flags;
    topASInfo.update                    = VK_TRUE;
    topASInfo.srcAccelerationStructure  = m_tlas.as.accel;
    topASInfo.dstAccelerationStructure  = m_tlas.as.accel;
    topASInfo.geometryArrayOfPointers   = VK_FALSE;
    topASInfo.geometryCount             = 1;
    topASInfo.ppGeometries              = &pGeometry;
    topASInfo.scratchData.deviceAddress = scratchAddress;

    uint32_t                                         nbInstances      = (uint32_t)instances.size();
    VkAccelerationStructureBuildOffsetInfoKHR        buildOffsetInfo  = {nbInstances, 0, 0, 0};
    const VkAccelerationStructureBuildOffsetInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

    // Build the TLAS

    // Update the acceleration structure. Note the VK_TRUE parameter to trigger the update,
    // and the existing TLAS being passed and updated in place
    vkCmdBuildAccelerationStructureKHR(cmdBuf, 1, &topASInfo, &pBuildOffsetInfo);

    genCmdBuf.submitAndWait(cmdBuf);

    m_alloc->destroy(scratchBuffer);
    m_alloc->destroy(stagingBuffer);
  }

  //--------------------------------------------------------------------------------------------------
  // Refit the BLAS from updated buffers
  //
  void updateBlas(uint32_t blasIdx)
  {
    Blas& blas = m_blas[blasIdx];

    // Compute the amount of scratch memory required by the AS builder to update    the BLAS
    VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR};
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_KHR;
    memoryRequirementsInfo.accelerationStructure = blas.as.accel;
    memoryRequirementsInfo.buildType             = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    VkMemoryRequirements2 reqMem{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    vkGetAccelerationStructureMemoryRequirementsKHR(m_device, &memoryRequirementsInfo, &reqMem);
    VkDeviceSize scratchSize = reqMem.memoryRequirements.size;

    // Allocate the scratch buffer
    nvvk::Buffer scratchBuffer =
        m_alloc->createBuffer(scratchSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    bufferInfo.buffer              = scratchBuffer.buffer;
    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(m_device, &bufferInfo);


    const VkAccelerationStructureGeometryKHR*   pGeometry = blas.asGeometry.data();
    VkAccelerationStructureBuildGeometryInfoKHR asInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    asInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    asInfo.flags                     = blas.flags;
    asInfo.update                    = VK_TRUE;
    asInfo.srcAccelerationStructure  = blas.as.accel;
    asInfo.dstAccelerationStructure  = blas.as.accel;
    asInfo.geometryArrayOfPointers   = VK_FALSE;
    asInfo.geometryCount             = (uint32_t)blas.asGeometry.size();
    asInfo.ppGeometries              = &pGeometry;
    asInfo.scratchData.deviceAddress = scratchAddress;

    std::vector<const VkAccelerationStructureBuildOffsetInfoKHR*> pBuildOffset(blas.asBuildOffsetInfo.size());
    for(size_t i = 0; i < blas.asBuildOffsetInfo.size(); i++)
      pBuildOffset[i] = &blas.asBuildOffsetInfo[i];

    // Update the instance buffer on the device side and build the TLAS
    nvvk::CommandPool genCmdBuf(m_device, m_queueIndex);
    VkCommandBuffer   cmdBuf = genCmdBuf.createCommandBuffer();


    // Update the acceleration structure. Note the VK_TRUE parameter to trigger the update,
    // and the existing BLAS being passed and updated in place
    vkCmdBuildAccelerationStructureKHR(cmdBuf, 1, &asInfo, pBuildOffset.data());

    genCmdBuf.submitAndWait(cmdBuf);
    m_alloc->destroy(scratchBuffer);
  }

private:
  // Top-level acceleration structure
  struct Tlas
  {
    nvvk::AccelKHR                       as;
    VkAccelerationStructureCreateInfoKHR asInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR, nullptr, 0,
                                                VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR};
    VkBuildAccelerationStructureFlagsKHR flags;
  };

  //--------------------------------------------------------------------------------------------------
  // Vector containing all the BLASes built and referenced by the TLAS
  std::vector<Blas> m_blas;
  // Top-level acceleration structure
  Tlas m_tlas;
  // Instance buffer containing the matrices and BLAS ids
  nvvk::Buffer m_instBuffer;

  VkDevice m_device{VK_NULL_HANDLE};
  uint32_t m_queueIndex{0};

  nvvk::Allocator* m_alloc = nullptr;
  nvvk::DebugUtil  m_debug;

#ifdef VULKAN_HPP
public:
  void buildBlas(const std::vector<RaytracingBuilderKHR::Blas>& blas_, vk::BuildAccelerationStructureFlagsKHR flags)
  {
    buildBlas(blas_, static_cast<VkBuildAccelerationStructureFlagsKHR>(flags));
  }
  void buildTlas(const std::vector<Instance>& instances, vk::BuildAccelerationStructureFlagsKHR flags)
  {
    buildTlas(instances, static_cast<VkBuildAccelerationStructureFlagsKHR>(flags));
  }

#endif
};

}  // namespace nvvk
