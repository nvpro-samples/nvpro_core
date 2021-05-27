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

# class nvvk::RaytracingBuilderKHR

Base functionality of raytracing

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
~~~~ C++
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
std::vector<RayTracingBuilderKHR::Instance> instances = // ...
m_rtBuilder.buildTlas(instances);

// Retrieve the handle to the acceleration structure.
const VkAccelerationStructureKHR tlas = m.rtBuilder.getAccelerationStructure()
~~~~
*/

#include <mutex>
#include <vulkan/vulkan_core.h>

#if VK_KHR_acceleration_structure

#include "resourceallocator_vk.hpp"
#include "commands_vk.hpp" // this is only needed here to satisfy some samples that rely on it
#include "debug_util_vk.hpp"
#include "nvh/nvprint.hpp" // this is only needed here to satisfy some samples that rely on it
#include "nvmath/nvmath.h"


namespace nvvk {
class RaytracingBuilderKHR
{
public:
  RaytracingBuilderKHR(RaytracingBuilderKHR const&) = delete;
  RaytracingBuilderKHR& operator=(RaytracingBuilderKHR const&) = delete;

  RaytracingBuilderKHR() = default;

  // Inputs used to build Bottom-level acceleration structure.
  // You manage the lifetime of the buffer(s) referenced by the
  // VkAccelerationStructureGeometryKHRs within. In particular, you must
  // make sure they are still valid and not being modified when the BLAS
  // is built or updated.
  struct BlasInput
  {
    // Data used to build acceleration structure geometry
    std::vector<VkAccelerationStructureGeometryKHR>       asGeometry;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
  };

  //--------------------------------------------------------------------------------------------------
  // Initializing the allocator and querying the raytracing properties
  //

  void setup(const VkDevice& device, nvvk::ResourceAllocator* allocator, uint32_t queueIndex);

  // This is an instance of a BLAS
  struct Instance
  {
    uint32_t                   blasId{0};            // Index of the BLAS in m_blas
    uint32_t                   instanceCustomId{0};  // Instance Index (gl_InstanceCustomIndexEXT)
    uint32_t                   hitGroupId{0};        // Hit group index in the SBT
    uint32_t                   mask{0xFF};           // Visibility mask, will be AND-ed with ray mask
    VkGeometryInstanceFlagsKHR flags{VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR};
    nvmath::mat4f              transform{nvmath::mat4f(1)};  // Identity
  };

  //--------------------------------------------------------------------------------------------------
  // Destroying all allocations
  //

  void destroy();

  // Returning the constructed top-level acceleration structure
  VkAccelerationStructureKHR getAccelerationStructure() const;

  //--------------------------------------------------------------------------------------------------
  // Create all the BLAS from the vector of BlasInput
  // - There will be one BLAS per input-vector entry
  // - There will be as many BLAS as input.size()
  // - The resulting BLAS (along with the inputs used to build) are stored in m_blas,
  //   and can be referenced by index.

  void buildBlas(const std::vector<RaytracingBuilderKHR::BlasInput>& input,
                 VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);


  //--------------------------------------------------------------------------------------------------
  // Convert an Instance object into a VkAccelerationStructureInstanceKHR

  VkAccelerationStructureInstanceKHR instanceToVkGeometryInstanceKHR(const Instance& instance);

  //--------------------------------------------------------------------------------------------------
  // Creating the top-level acceleration structure from the vector of Instance
  // - See struct of Instance
  // - The resulting TLAS will be stored in m_tlas
  // - update is to rebuild the Tlas with updated matrices
  void buildTlas(const std::vector<Instance>&         instances,
                 VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
                 bool                                 update = false);

  //--------------------------------------------------------------------------------------------------
  // Refit BLAS number blasIdx from updated buffer contents.
  //
  void updateBlas(uint32_t blasIdx);

#ifdef VULKAN_HPP
public:
  void buildBlas(const std::vector<RaytracingBuilderKHR::BlasInput>& blas_, vk::BuildAccelerationStructureFlagsKHR flags)
  {
    buildBlas(blas_, static_cast<VkBuildAccelerationStructureFlagsKHR>(flags));
  }

  void buildTlas(const std::vector<Instance>& instances, vk::BuildAccelerationStructureFlagsKHR flags, bool update = false)
  {
    buildTlas(instances, static_cast<VkBuildAccelerationStructureFlagsKHR>(flags), update);
  }

#endif

private:
  // Top-level acceleration structure
  struct Tlas
  {
    nvvk::AccelKHR                       as;
    VkBuildAccelerationStructureFlagsKHR flags = 0;
  };

  // Bottom-level acceleration structure, along with the information needed to re-build it.
  struct BlasEntry
  {
    // User-provided input.
    BlasInput input;

    // VkAccelerationStructureKHR plus extra info needed for our memory allocator.
    // The RaytracingBuilderKHR that created this DOES destroy it when destroyed.
    nvvk::AccelKHR as;

    // Additional parameters for acceleration structure builds
    VkBuildAccelerationStructureFlagsKHR flags = 0;

    BlasEntry() = default;
    BlasEntry(BlasInput input_)
        : input(std::move(input_))
        , as()
    {
    }
  };


  //--------------------------------------------------------------------------------------------------
  // Vector containing all the BLASes built in buildBlas (and referenced by the TLAS)
  std::vector<BlasEntry> m_blas;
  // Top-level acceleration structure
  Tlas m_tlas;
  // Instance buffer containing the matrices and BLAS ids
  nvvk::Buffer m_instBuffer;

  VkDevice m_device{VK_NULL_HANDLE};
  uint32_t m_queueIndex{0};

  nvvk::ResourceAllocator* m_alloc = nullptr;
  nvvk::DebugUtil          m_debug;
};

}  // namespace nvvk

#else
#error This include requires VK_KHR_ray_tracing support in the Vulkan SDK.
#endif
