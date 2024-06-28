/*
 * Copyright (c) 2014-2024, NVIDIA CORPORATION.  All rights reserved.
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
#include <sstream>
#include "vulkan/vulkan_core.h"
#include "resourceallocator_vk.hpp"


namespace nvvk {

/** @DOC_START

# `nvvk::accelerationStructureBarrier` Function
This function sets up a memory barrier specifically for acceleration structure operations in Vulkan, ensuring proper data synchronization during the build or update phases. It operates between pipeline stages that deal with acceleration structure building.

# `nvvk::toTransformMatrixKHR` Function
This function converts a `glm::mat4` matrix to the matrix format required by acceleration structures in Vulkan.

# `nvvk::AccelerationStructureGeometryInfo` Structure
- **Purpose**: Holds information about acceleration structure geometry, including the geometry structure and build range information.

# `nvvk::AccelerationStructureBuildData` Structure
- **Purpose**: Manages the building of Vulkan acceleration structures of a specified type.
- **Key Functions**:
  - `addGeometry`: Adds a geometry with build range information to the acceleration structure.
  - `finalizeGeometry`: Configures the build information and calculates the necessary size information.
  - `makeCreateInfo`: Creates an acceleration structure based on the current build and size information.
  - `cmdBuildAccelerationStructure`: Builds the acceleration structure in a Vulkan command buffer.
  - `cmdUpdateAccelerationStructure`: Updates the acceleration structure in a Vulkan command buffer.
  - `hasCompactFlag`: Checks if the compact flag is set for the build.
- **Usage**:
  - For each BLAS, 
    - Add geometry using `addGeometry`.
    - Finalize the geometry and get the size requirements using `finalizeGeometry`.
    - Keep the max scratch buffer size in mind when creating the scratch buffer.
  - Create Scratch Buffer using the information returned by finalizeGeometry.
  - For each BLAS, 
    - Create the acceleration structure using `makeCreateInfo`.

* @DOC_END */


// Helper function to insert a memory barrier for acceleration structures
inline void accelerationStructureBarrier(VkCommandBuffer cmd, VkAccessFlags src, VkAccessFlags dst)
{
  VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  barrier.srcAccessMask = src;
  barrier.dstAccessMask = dst;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                       VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

// Convert a Mat4x4 to the matrix required by acceleration structures
inline VkTransformMatrixKHR toTransformMatrixKHR(glm::mat4 matrix)
{
  // VkTransformMatrixKHR uses a row-major memory layout, while glm::mat4
  // uses a column-major memory layout. We transpose the matrix so we can
  // memcpy the matrix's data directly.
  glm::mat4            temp = glm::transpose(matrix);
  VkTransformMatrixKHR out_matrix;
  memcpy(&out_matrix, &temp, sizeof(VkTransformMatrixKHR));
  return out_matrix;
}

// Single Geometry information, multiple can be used in a single BLAS
struct AccelerationStructureGeometryInfo
{
  VkAccelerationStructureGeometryKHR       geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
};

// Template for building Vulkan Acceleration Structures of a specified type.
struct AccelerationStructureBuildData
{
  VkAccelerationStructureTypeKHR asType = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;  // Mandatory to set

  // Collection of geometries for the acceleration structure.
  std::vector<VkAccelerationStructureGeometryKHR> asGeometry;
  // Build range information corresponding to each geometry.
  std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildRangeInfo;
  // Build information required for acceleration structure.
  VkAccelerationStructureBuildGeometryInfoKHR buildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
  // Size information for acceleration structure build resources.
  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

  // Adds a geometry with its build range information to the acceleration structure.
  void addGeometry(const VkAccelerationStructureGeometryKHR& asGeom, const VkAccelerationStructureBuildRangeInfoKHR& offset);
  void addGeometry(const AccelerationStructureGeometryInfo& asGeom);

  AccelerationStructureGeometryInfo makeInstanceGeometry(size_t numInstances, VkDeviceAddress instanceBufferAddr);

  // Configures the build information and calculates the necessary size information.
  VkAccelerationStructureBuildSizesInfoKHR finalizeGeometry(VkDevice device, VkBuildAccelerationStructureFlagsKHR flags);

  // Creates an acceleration structure based on the current build and size info.
  VkAccelerationStructureCreateInfoKHR makeCreateInfo() const;

  // Commands to build the acceleration structure in a Vulkan command buffer.
  void cmdBuildAccelerationStructure(VkCommandBuffer cmd, VkAccelerationStructureKHR accelerationStructure, VkDeviceAddress scratchAddress);

  // Commands to update the acceleration structure in a Vulkan command buffer.
  void cmdUpdateAccelerationStructure(VkCommandBuffer cmd, VkAccelerationStructureKHR accelerationStructure, VkDeviceAddress scratchAddress);

  // Checks if the compact flag is set for the build.
  bool hasCompactFlag() const { return buildInfo.flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR; }
};

// Get the maximum scratch buffer size required for the acceleration structure build
VkDeviceSize getMaxScratchSize(const std::vector<AccelerationStructureBuildData>& asBuildData);

/**
 * @brief Manages the construction and optimization of Bottom-Level Acceleration Structures (BLAS) for Vulkan Ray Tracing.
 *
 * This class is designed to  facilitates the  construction of BLAS based on provided build information, queries for 
 * compaction potential, compacts BLAS for efficient memory usage, and cleans up resources. 
 * It ensures that operations are performed within a specified memory budget, if possible,
 * and provides statistical data on the compaction results. This class is crucial for optimizing ray tracing performance
 * by managing BLAS efficiently in Vulkan-enabled applications.
 * 
 * Usage:
 * - Create a BlasBuilder object with a resource allocator and device.
 * - Call cmdCreateBlas or cmdCreateParallelBlas in a loop to create the BLAS from the provided BlasBuildData, until it returns true.
 * - Call cmdCompactBlas to compact the BLAS that have been built.
 * - Call destroyNonCompactedBlas to destroy the original BLAS that were compacted.
 * - Call destroy to clean up all resources. 
 * - Call getStatistics to get statistics about the compacted BLAS.
 * 
 * For parallel BLAS creation, the scratch buffer size strategy can be used to find the best size needed for the scratch buffer.
 * - Call getScratchSize to get the maximum size needed for the scratch buffer.
 * - User allocate the scratch buffer with the size returned by getScratchSize.
 * - Call getScratchAddresses to get the address for each BLAS.
 * - Use the scratch buffer addresses to build the BLAS in parallel.
 * 
 */

class BlasBuilder
{
public:
  BlasBuilder(nvvk::ResourceAllocator* allocator, VkDevice device);
  ~BlasBuilder();

  struct Stats
  {
    VkDeviceSize totalOriginalSize = 0;
    VkDeviceSize totalCompactSize  = 0;

    std::string toString() const;
  };


  // Create the BLAS from the vector of BlasBuildData
  // Each BLAS will be created in sequence and share the same scratch buffer
  // Return true if ALL the BLAS were created within the budget
  // if not, this function needs to be called again until it returns true
  bool cmdCreateBlas(VkCommandBuffer                              cmd,
                     std::vector<AccelerationStructureBuildData>& blasBuildData,   // List of the BLAS to build */
                     std::vector<nvvk::AccelKHR>&                 blasAccel,       // List of the acceleration structure
                     VkDeviceAddress                              scratchAddress,  //  Address of the scratch buffer
                     VkDeviceSize                                 hintMaxBudget = 512'000'000);

  // Create the BLAS from the vector of BlasBuildData in parallel
  // The advantage of this function is that it will try to build as many BLAS as possible in parallel
  // but it requires a scratch buffer per BLAS, or less but then each of them must large enough to hold the largest BLAS
  // This function needs to be called until it returns true
  bool cmdCreateParallelBlas(VkCommandBuffer                              cmd,
                             std::vector<AccelerationStructureBuildData>& blasBuildData,
                             std::vector<nvvk::AccelKHR>&                 blasAccel,
                             const std::vector<VkDeviceAddress>&          scratchAddress,
                             VkDeviceSize                                 hintMaxBudget = 512'000'000);

  // Compact the BLAS that have been built
  // Synchronization must be done by the application between the build and the compact
  void cmdCompactBlas(VkCommandBuffer cmd, std::vector<AccelerationStructureBuildData>& blasBuildData, std::vector<nvvk::AccelKHR>& blasAccel);

  // Destroy the original BLAS that was compacted
  void destroyNonCompactedBlas();

  // Destroy all information
  void destroy();

  // Return the statistics about the compacted BLAS
  Stats getStatistics() const { return m_stats; };

  // Scratch size strategy:
  // Find the maximum size of the scratch buffer needed for the BLAS build
  //
  // Strategy:
  // - Loop over all BLAS to find the maximum size
  // - If the max size is within the budget, return it. This will return as many addresses as there are BLAS
  // - Otherwise, return n*maxBlasSize, where n is the number of BLAS and maxBlasSize is the maximum size found for a single BLAS.
  //   In this case, fewer addresses will be returned than the number of BLAS, but each can be used to build any BLAS
  //
  // Usage
  // - Call this function to get the maximum size needed for the scratch buffer
  // - User allocate the scratch buffer with the size returned by this function
  // - Call getScratchAddresses to get the address for each BLAS
  //
  // Note: 128 is the default alignment for the scratch buffer
  //       (VkPhysicalDeviceAccelerationStructurePropertiesKHR::minAccelerationStructureScratchOffsetAlignment)
  VkDeviceSize getScratchSize(VkDeviceSize                                             hintMaxBudget,
                              const std::vector<nvvk::AccelerationStructureBuildData>& buildData,
                              uint32_t                                                 minAlignment = 128) const;

  void getScratchAddresses(VkDeviceSize                                             hintMaxBudget,
                           const std::vector<nvvk::AccelerationStructureBuildData>& buildData,
                           VkDeviceAddress                                          scratchBufferAddress,
                           std::vector<VkDeviceAddress>&                            scratchAddresses,
                           uint32_t                                                 minAlignment = 128);


private:
  void         destroyQueryPool();
  void         createQueryPool(uint32_t maxBlasCount);
  void         initializeQueryPoolIfNeeded(const std::vector<AccelerationStructureBuildData>& blasBuildData);
  VkDeviceSize buildAccelerationStructures(VkCommandBuffer                              cmd,
                                           std::vector<AccelerationStructureBuildData>& blasBuildData,
                                           std::vector<nvvk::AccelKHR>&                 blasAccel,
                                           const std::vector<VkDeviceAddress>&          scratchAddress,
                                           VkDeviceSize                                 hintMaxBudget,
                                           VkDeviceSize                                 currentBudget,
                                           uint32_t&                                    currentQueryIdx);

  VkDevice                 m_device;
  nvvk::ResourceAllocator* m_alloc     = nullptr;
  VkQueryPool              m_queryPool = VK_NULL_HANDLE;
  uint32_t                 m_currentBlasIdx{0};
  uint32_t                 m_currentQueryIdx{0};

  std::vector<nvvk::AccelKHR> m_cleanupBlasAccel;

  // Stats
  Stats m_stats;
};


}  // namespace nvvk