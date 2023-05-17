/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**

\class nvvk::RaytracingBuilderKHR

\brief nvvk::RaytracingBuilderKHR is a base functionality of raytracing

This class acts as an owning container for a single top-level acceleration
structure referencing any number of bottom-level acceleration structures.
We provide functions for building (on the device) an array of BLASs and a
single TLAS from vectors of BlasInput and Instance, respectively, and
a destroy function for cleaning up the created acceleration structures.

Generally, we reference BLASs by their index in the stored BLAS array,
rather than using raw device pointers as the pure Vulkan acceleration
structure API uses.

This class does not support replacing acceleration structures once
built, but you can update the acceleration structures. For educational
purposes, this class prioritizes (relative) understandability over
performance, so vkQueueWaitIdle is implicitly used everywhere.

# Setup and Usage
\code{.cpp}
// Borrow a VkDevice and memory allocator pointer (must remain
// valid throughout our use of the ray trace builder), and
// instantiate an unspecified queue of the given family for use.
m_rtBuilder.setup(device, memoryAllocator, queueIndex);

// You create a vector of RayTracingBuilderKHR::BlasInput then
// pass it to buildBlas.
std::vector<RayTracingBuilderKHR::BlasInput> inputs = // ...
m_rtBuilder.buildBlas(inputs);

// You create a vector of RaytracingBuilder::Instance and pass to
// buildTlas. The blasId member of each instance must be below
// inputs.size() (above).
std::vector<VkAccelerationStructureInstanceKHR> instances = // ...
m_rtBuilder.buildTlas(instances);

// Retrieve the handle to the acceleration structure.
const VkAccelerationStructureKHR tlas = m.rtBuilder.getAccelerationStructure()
\endcode
*/

#include <mutex>
#include <vulkan/vulkan_core.h>

#if VK_KHR_acceleration_structure

#include "resourceallocator_vk.hpp"
#include "commands_vk.hpp"  // this is only needed here to satisfy some samples that rely on it
#include "debug_util_vk.hpp"
#include "nvh/nvprint.hpp"  // this is only needed here to satisfy some samples that rely on it
#include "nvmath/nvmath.h"
#include <type_traits>


namespace nvvk {

// Convert a Mat4x4 to the matrix required by acceleration structures
inline VkTransformMatrixKHR toTransformMatrixKHR(nvmath::mat4f matrix)
{
  // VkTransformMatrixKHR uses a row-major memory layout, while nvmath::mat4f
  // uses a column-major memory layout. We transpose the matrix so we can
  // memcpy the matrix's data directly.
  nvmath::mat4f        temp = nvmath::transpose(matrix);
  VkTransformMatrixKHR out_matrix;
  memcpy(&out_matrix, &temp, sizeof(VkTransformMatrixKHR));
  return out_matrix;
}

// Ray tracing BLAS and TLAS builder
class RaytracingBuilderKHR
{
public:
  // Inputs used to build Bottom-level acceleration structure.
  // You manage the lifetime of the buffer(s) referenced by the VkAccelerationStructureGeometryKHRs within.
  // In particular, you must make sure they are still valid and not being modified when the BLAS is built or updated.
  struct BlasInput
  {
    // Data used to build acceleration structure geometry
    std::vector<VkAccelerationStructureGeometryKHR>       asGeometry;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
    VkBuildAccelerationStructureFlagsKHR                  flags{0};
  };

  // Initializing the allocator and querying the raytracing properties
  void setup(const VkDevice& device, nvvk::ResourceAllocator* allocator, uint32_t queueIndex);

  // Destroying all allocations
  void destroy();

  // Returning the constructed top-level acceleration structure
  VkAccelerationStructureKHR getAccelerationStructure() const;

  // Return the Acceleration Structure Device Address of a BLAS Id
  VkDeviceAddress getBlasDeviceAddress(uint32_t blasId);

  // Create all the BLAS from the vector of BlasInput
  void buildBlas(const std::vector<BlasInput>&        input,
                 VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

  // Refit BLAS number blasIdx from updated buffer contents.
  void updateBlas(uint32_t blasIdx, BlasInput& blas, VkBuildAccelerationStructureFlagsKHR flags);

  // Build TLAS for static acceleration structures
  void buildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
                 VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
                 bool                                 update = false);

#ifdef VK_NV_ray_tracing_motion_blur
  // Build TLAS for mix of motion and static acceleration structures
  void buildTlas(const std::vector<VkAccelerationStructureMotionInstanceNV>& instances,
                 VkBuildAccelerationStructureFlagsKHR flags  = VK_BUILD_ACCELERATION_STRUCTURE_MOTION_BIT_NV,
                 bool                                 update = false);
#endif

  // Build TLAS from an array of VkAccelerationStructureInstanceKHR
  // - Use motion=true with VkAccelerationStructureMotionInstanceNV
  // - The resulting TLAS will be stored in m_tlas
  // - update is to rebuild the Tlas with updated matrices, flag must have the 'allow_update'
  template <class T>
  void buildTlas(const std::vector<T>&                instances,
                 VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
                 bool                                 update = false,
                 bool                                 motion = false)
  {
    // Cannot call buildTlas twice except to update.
    assert(m_tlas.accel == VK_NULL_HANDLE || update);
    uint32_t countInstance = static_cast<uint32_t>(instances.size());

    // Command buffer to create the TLAS
    nvvk::CommandPool genCmdBuf(m_device, m_queueIndex);
    VkCommandBuffer   cmdBuf = genCmdBuf.createCommandBuffer();

    // Create a buffer holding the actual instance data (matrices++) for use by the AS builder
    nvvk::Buffer instancesBuffer;  // Buffer of instances containing the matrices and BLAS ids
    instancesBuffer = m_alloc->createBuffer(cmdBuf, instances,
                                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                                                | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
    NAME_VK(instancesBuffer.buffer);
    VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, instancesBuffer.buffer};
    VkDeviceAddress           instBufferAddr = vkGetBufferDeviceAddress(m_device, &bufferInfo);

    // Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);

    // Creating the TLAS
    nvvk::Buffer scratchBuffer;
    cmdCreateTlas(cmdBuf, countInstance, instBufferAddr, scratchBuffer, flags, update, motion);

    // Finalizing and destroying temporary data
    genCmdBuf.submitAndWait(cmdBuf);  // queueWaitIdle inside.
    m_alloc->finalizeAndReleaseStaging();
    m_alloc->destroy(scratchBuffer);
    m_alloc->destroy(instancesBuffer);
  }

  // Creating the TLAS, called by buildTlas
  void cmdCreateTlas(VkCommandBuffer                      cmdBuf,          // Command buffer
                     uint32_t                             countInstance,   // number of instances
                     VkDeviceAddress                      instBufferAddr,  // Buffer address of instances
                     nvvk::Buffer&                        scratchBuffer,   // Scratch buffer for construction
                     VkBuildAccelerationStructureFlagsKHR flags,           // Build creation flag
                     bool                                 update,          // Update == animation
                     bool                                 motion           // Motion Blur
  );


protected:
  std::vector<nvvk::AccelKHR> m_blas;  // Bottom-level acceleration structure
  nvvk::AccelKHR              m_tlas;  // Top-level acceleration structure

  // Setup
  VkDevice                 m_device{VK_NULL_HANDLE};
  uint32_t                 m_queueIndex{0};
  nvvk::ResourceAllocator* m_alloc{nullptr};
  nvvk::DebugUtil          m_debug;
  nvvk::CommandPool        m_cmdPool;

  struct BuildAccelerationStructure
  {
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo;
    nvvk::AccelKHR                                  as;  // result acceleration structure
    nvvk::AccelKHR                                  cleanupAS;
  };


  void cmdCreateBlas(VkCommandBuffer                          cmdBuf,
                     std::vector<uint32_t>                    indices,
                     std::vector<BuildAccelerationStructure>& buildAs,
                     VkDeviceAddress                          scratchAddress,
                     VkQueryPool                              queryPool);
  void cmdCompactBlas(VkCommandBuffer cmdBuf, std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAs, VkQueryPool queryPool);
  void destroyNonCompacted(std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAs);
  bool hasFlag(VkFlags item, VkFlags flag) { return (item & flag) == flag; }
};

}  // namespace nvvk

#else
#error This include requires VK_KHR_acceleration_structure support in the Vulkan SDK.
#endif
