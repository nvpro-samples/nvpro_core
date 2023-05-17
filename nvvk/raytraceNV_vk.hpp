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

\class nvvk::RaytracingBuilderNV

\brief nvvk::RaytracingBuilderNV is a base functionality of raytracing

This class does not implement all what you need to do raytracing, but
helps creating the BLAS and TLAS, which then can be used by different
raytracing usage.

# Setup and Usage
\code{.cpp}
m_rtBuilder.setup(device, memoryAllocator, queueIndex);
// Create array of VkGeometryNV
m_rtBuilder.buildBlas(allBlas);
// Create array of RaytracingBuilder::instance
m_rtBuilder.buildTlas(instances);
// Retrieve the acceleration structure
const VkAccelerationStructureNV& tlas = m.rtBuilder.getAccelerationStructure()
\endcode
*/


#include <mutex>
#include <vulkan/vulkan_core.h>

#if VK_NV_ray_tracing

#include "resourceallocator_vk.hpp"
#include "commands_vk.hpp" // this is only needed here to satisfy some samples that rely on it
#include "debug_util_vk.hpp"
#include "nvh/nvprint.hpp" // this is only needed here to satisfy some samples that rely on it
#include "nvmath/nvmath.h"

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

namespace nvvk {
class RaytracingBuilderNV
{
public:
  RaytracingBuilderNV(RaytracingBuilderNV const&) = delete;
  RaytracingBuilderNV& operator=(RaytracingBuilderNV const&) = delete;

  RaytracingBuilderNV() = default;

  //--------------------------------------------------------------------------------------------------
  // Initializing the allocator and querying the raytracing properties
  //
  void setup(VkDevice device, nvvk::ResourceAllocator* allocator, uint32_t queueIndex);

  // This is an instance of a BLAS
  struct Instance
  {
    uint32_t                  blasId{0};      // Index of the BLAS in m_blas
    uint32_t                  instanceId{0};  // Instance Index (gl_InstanceID)
    uint32_t                  hitGroupId{0};  // Hit group index in the SBT
    uint32_t                  mask{0xFF};     // Visibility mask, will be AND-ed with ray mask
    VkGeometryInstanceFlagsNV flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
    nvmath::mat4f             transform{nvmath::mat4f(1)};  // Identity
  };


  //--------------------------------------------------------------------------------------------------
  // Destroying all allocations
  //
  void destroy();

  // Returning the constructed top-level acceleration structure
  VkAccelerationStructureNV getAccelerationStructure() const;

  //--------------------------------------------------------------------------------------------------
  // Create all the BLAS from the vector of vectors of VkGeometryNV
  // - There will be one BLAS per vector of VkGeometryNV
  // - There will be as many BLAS there are items in the geoms vector
  // - The resulting BLAS are stored in m_blas
  //
  void buildBlas(const std::vector<std::vector<VkGeometryNV>>& geoms,
                 VkBuildAccelerationStructureFlagsNV flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV);

  //--------------------------------------------------------------------------------------------------
  // Convert an Instance object into a VkGeometryInstanceNV

  VkGeometryInstanceNV instanceToVkGeometryInstanceNV(const Instance& instance);

  //--------------------------------------------------------------------------------------------------
  // Creating the top-level acceleration structure from the vector of Instance
  // - See struct of Instance
  // - The resulting TLAS will be stored in m_tlas
  //
  void buildTlas(const std::vector<Instance>&        instances,
                 VkBuildAccelerationStructureFlagsNV flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV);

  //--------------------------------------------------------------------------------------------------
  // Refit the TLAS using new instance matrices
  //
  void updateTlasMatrices(const std::vector<Instance>& instances);

  //--------------------------------------------------------------------------------------------------
  // Refit the BLAS from updated buffers
  //
  void updateBlas(uint32_t blasIdx);

private:
  // Bottom-level acceleration structure
  struct Blas
  {
    nvvk::AccelNV                 as;
    VkAccelerationStructureInfoNV asInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV, nullptr,
                                         VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV};
    VkGeometryNV                  geometry;
  };

  // Top-level acceleration structure
  struct Tlas
  {
    nvvk::AccelNV                 as;
    VkAccelerationStructureInfoNV asInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV, nullptr,
                                         VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV};
  };

  //--------------------------------------------------------------------------------------------------
  // Vector containing all the BLASes built and referenced by the TLAS
  std::vector<Blas> m_blas;
  // Top-level acceleration structure
  Tlas m_tlas;
  // Instance buffer containing the matrices and BLAS ids
  nvvk::Buffer m_instBuffer;

  VkDevice m_device;
  uint32_t m_queueIndex{0};

  nvvk::ResourceAllocator* m_alloc = nullptr;
  nvvk::DebugUtil          m_debug;
};

}  // namespace nvvk

#else
#error This include requires VK_NV_ray_tracing support in the Vulkan SDK.
#endif
