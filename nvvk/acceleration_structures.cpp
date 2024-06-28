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

#include <assert.h>

#include "nvvk/acceleration_structures.hpp"
#include "nvh/alignment.hpp"

void nvvk::AccelerationStructureBuildData::addGeometry(const VkAccelerationStructureGeometryKHR&       asGeom,
                                                       const VkAccelerationStructureBuildRangeInfoKHR& offset)
{
  asGeometry.push_back(asGeom);
  asBuildRangeInfo.push_back(offset);
}


void nvvk::AccelerationStructureBuildData::addGeometry(const AccelerationStructureGeometryInfo& asGeom)
{
  asGeometry.push_back(asGeom.geometry);
  asBuildRangeInfo.push_back(asGeom.rangeInfo);
}


VkAccelerationStructureBuildSizesInfoKHR nvvk::AccelerationStructureBuildData::finalizeGeometry(VkDevice device, VkBuildAccelerationStructureFlagsKHR flags)
{
  assert(asGeometry.size() > 0 && "No geometry added to Build Structure");
  assert(asType != VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR && "Acceleration Structure Type not set");

  buildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildInfo.type                      = asType;
  buildInfo.flags                     = flags;
  buildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
  buildInfo.dstAccelerationStructure  = VK_NULL_HANDLE;
  buildInfo.geometryCount             = static_cast<uint32_t>(asGeometry.size());
  buildInfo.pGeometries               = asGeometry.data();
  buildInfo.ppGeometries              = nullptr;
  buildInfo.scratchData.deviceAddress = 0;

  std::vector<uint32_t> maxPrimCount(asBuildRangeInfo.size());
  for(size_t i = 0; i < asBuildRangeInfo.size(); ++i)
  {
    maxPrimCount[i] = asBuildRangeInfo[i].primitiveCount;
  }

  vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
                                          maxPrimCount.data(), &sizeInfo);

  return sizeInfo;
}


VkAccelerationStructureCreateInfoKHR nvvk::AccelerationStructureBuildData::makeCreateInfo() const
{
  assert(asType != VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR && "Acceleration Structure Type not set");
  assert(sizeInfo.accelerationStructureSize > 0 && "Acceleration Structure Size not set");

  VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
  createInfo.type = asType;
  createInfo.size = sizeInfo.accelerationStructureSize;

  return createInfo;
}


nvvk::AccelerationStructureGeometryInfo nvvk::AccelerationStructureBuildData::makeInstanceGeometry(size_t numInstances,
                                                                                                   VkDeviceAddress instanceBufferAddr)
{
  assert(asType == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR && "Instance geometry can only be used with TLAS");

  // Describes instance data in the acceleration structure.
  VkAccelerationStructureGeometryInstancesDataKHR geometryInstances{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
  geometryInstances.data.deviceAddress = instanceBufferAddr;

  // Set up the geometry to use instance data.
  VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  geometry.geometryType       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  geometry.geometry.instances = geometryInstances;

  // Specifies the number of primitives (instances in this case).
  VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
  rangeInfo.primitiveCount = static_cast<uint32_t>(numInstances);

  // Prepare and return geometry information.
  AccelerationStructureGeometryInfo result;
  result.geometry  = geometry;
  result.rangeInfo = rangeInfo;

  return result;
}


void nvvk::AccelerationStructureBuildData::cmdBuildAccelerationStructure(VkCommandBuffer cmd,
                                                                         VkAccelerationStructureKHR accelerationStructure,
                                                                         VkDeviceAddress scratchAddress)
{
  assert(asGeometry.size() == asBuildRangeInfo.size() && "asGeometry.size() != asBuildRangeInfo.size()");
  assert(accelerationStructure != VK_NULL_HANDLE && "Acceleration Structure not created, first call createAccelerationStructure");

  const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo = asBuildRangeInfo.data();

  // Build the acceleration structure
  buildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
  buildInfo.dstAccelerationStructure  = accelerationStructure;
  buildInfo.scratchData.deviceAddress = scratchAddress;
  buildInfo.pGeometries = asGeometry.data();  // In case the structure was copied, we need to update the pointer

  vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &rangeInfo);

  // Since the scratch buffer is reused across builds, we need a barrier to ensure one build
  // is finished before starting the next one.
  accelerationStructureBarrier(cmd, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
}


void nvvk::AccelerationStructureBuildData::cmdUpdateAccelerationStructure(VkCommandBuffer cmd,
                                                                          VkAccelerationStructureKHR accelerationStructure,
                                                                          VkDeviceAddress scratchAddress)
{
  assert(asGeometry.size() == asBuildRangeInfo.size() && "asGeometry.size() != asBuildRangeInfo.size()");
  assert(accelerationStructure != VK_NULL_HANDLE && "Acceleration Structure not created, first call createAccelerationStructure");

  const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo = asBuildRangeInfo.data();

  // Build the acceleration structure
  buildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
  buildInfo.srcAccelerationStructure  = accelerationStructure;
  buildInfo.dstAccelerationStructure  = accelerationStructure;
  buildInfo.scratchData.deviceAddress = scratchAddress;
  buildInfo.pGeometries               = asGeometry.data();
  vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &rangeInfo);

  // Since the scratch buffer is reused across builds, we need a barrier to ensure one build
  // is finished before starting the next one.
  accelerationStructureBarrier(cmd, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
}


// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------
// Blas Builder : utility to create BLAS
// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------


nvvk::BlasBuilder::BlasBuilder(nvvk::ResourceAllocator* allocator, VkDevice device)
    : m_device(device)
    , m_alloc(allocator)
{
}

nvvk::BlasBuilder::~BlasBuilder()
{
  destroy();
}

void nvvk::BlasBuilder::createQueryPool(uint32_t maxBlasCount)
{
  VkQueryPoolCreateInfo qpci = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
  qpci.queryType             = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
  qpci.queryCount            = maxBlasCount;
  vkCreateQueryPool(m_device, &qpci, nullptr, &m_queryPool);
}

// This will build multiple BLAS serially, one after the other, ensuring that the process
// stays within the specified memory budget.
bool nvvk::BlasBuilder::cmdCreateBlas(VkCommandBuffer                              cmd,
                                      std::vector<AccelerationStructureBuildData>& blasBuildData,
                                      std::vector<nvvk::AccelKHR>&                 blasAccel,
                                      VkDeviceAddress                              scratchAddress,
                                      VkDeviceSize                                 hintMaxBudget)
{
  // It won't run in parallel, but will process all BLAS within the budget before returning
  return cmdCreateParallelBlas(cmd, blasBuildData, blasAccel, {scratchAddress}, hintMaxBudget);
}

// This function is responsible for building multiple Bottom-Level Acceleration Structures (BLAS) in parallel,
// ensuring that the process stays within the specified memory budget.
//
// Returns:
//   A boolean indicating whether all BLAS in the `blasBuildData` have been built by this function call.
//   Returns `true` if all BLAS were built, `false` otherwise.
bool nvvk::BlasBuilder::cmdCreateParallelBlas(VkCommandBuffer                              cmd,
                                              std::vector<AccelerationStructureBuildData>& blasBuildData,
                                              std::vector<nvvk::AccelKHR>&                 blasAccel,
                                              const std::vector<VkDeviceAddress>&          scratchAddress,
                                              VkDeviceSize                                 hintMaxBudget)
{
  // Initialize the query pool if necessary to handle queries for properties of built acceleration structures.
  initializeQueryPoolIfNeeded(blasBuildData);

  VkDeviceSize processBudget   = 0;                  // Tracks the total memory used in the construction process.
  uint32_t     currentQueryIdx = m_currentQueryIdx;  // Local copy of the current query index.

  // Process each BLAS in the data vector while staying under the memory budget.
  while(m_currentBlasIdx < blasBuildData.size() && processBudget < hintMaxBudget)
  {
    // Build acceleration structures and accumulate the total memory used.
    processBudget += buildAccelerationStructures(cmd, blasBuildData, blasAccel, scratchAddress, hintMaxBudget,
                                                 processBudget, currentQueryIdx);
  }

  // Check if all BLAS have been built.
  return m_currentBlasIdx >= blasBuildData.size();
}


// Initializes a query pool for recording acceleration structure properties if necessary.
// This function ensures a query pool is available if any BLAS in the build data is flagged for compaction.
void nvvk::BlasBuilder::initializeQueryPoolIfNeeded(const std::vector<AccelerationStructureBuildData>& blasBuildData)
{
  if(!m_queryPool)
  {
    // Iterate through each BLAS build data element to check if the compaction flag is set.
    for(const auto& blas : blasBuildData)
    {
      if(blas.hasCompactFlag())
      {
        createQueryPool(static_cast<uint32_t>(blasBuildData.size()));
        break;
      }
    }
  }

  // If a query pool is now available (either newly created or previously existing),
  // reset the query pool to clear any old data or states.
  if(m_queryPool)
  {
    vkResetQueryPool(m_device, m_queryPool, 0, static_cast<uint32_t>(blasBuildData.size()));
  }
}


// Builds multiple Bottom-Level Acceleration Structures (BLAS) for a Vulkan ray tracing pipeline.
// This function manages memory budgets and submits the necessary commands to the specified command buffer.
//
// Parameters:
//   cmd            - Command buffer where acceleration structure commands are recorded.
//   blasBuildData  - Vector of data structures containing the geometry and other build-related information for each BLAS.
//   blasAccel      - Vector where the function will store the created acceleration structures.
//   scratchAddress - Vector of device addresses pointing to scratch memory required for the build process.
//   hintMaxBudget  - A hint for the maximum budget allowed for building acceleration structures.
//   currentBudget  - The current usage of the budget prior to this call.
//   currentQueryIdx - Reference to the current index for queries, updated during execution.
//
// Returns:
//   The total device size used for building the acceleration structures during this function call.
VkDeviceSize nvvk::BlasBuilder::buildAccelerationStructures(VkCommandBuffer                              cmd,
                                                            std::vector<AccelerationStructureBuildData>& blasBuildData,
                                                            std::vector<nvvk::AccelKHR>&                 blasAccel,
                                                            const std::vector<VkDeviceAddress>&          scratchAddress,
                                                            VkDeviceSize                                 hintMaxBudget,
                                                            VkDeviceSize                                 currentBudget,
                                                            uint32_t& currentQueryIdx)
{
  // Temporary vectors for storing build-related data
  std::vector<VkAccelerationStructureBuildGeometryInfoKHR> collectedBuildInfo;
  std::vector<VkAccelerationStructureKHR>                  collectedAccel;
  std::vector<VkAccelerationStructureBuildRangeInfoKHR*>   collectedRangeInfo;

  // Pre-allocate memory based on the number of BLAS to be built
  collectedBuildInfo.reserve(blasBuildData.size());
  collectedAccel.reserve(blasBuildData.size());
  collectedRangeInfo.reserve(blasBuildData.size());

  // Initialize the total budget used in this function call
  VkDeviceSize budgetUsed = 0;

  // Loop through BLAS data while there is scratch address space and budget available
  while(collectedBuildInfo.size() < scratchAddress.size() && currentBudget + budgetUsed < hintMaxBudget
        && m_currentBlasIdx < blasBuildData.size())
  {
    auto&                                data       = blasBuildData[m_currentBlasIdx];
    VkAccelerationStructureCreateInfoKHR createInfo = data.makeCreateInfo();

    // Create and store acceleration structure
    blasAccel[m_currentBlasIdx] = m_alloc->createAcceleration(createInfo);
    collectedAccel.push_back(blasAccel[m_currentBlasIdx].accel);

    // Setup build information for the current BLAS
    data.buildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    data.buildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
    data.buildInfo.dstAccelerationStructure  = blasAccel[m_currentBlasIdx].accel;
    data.buildInfo.scratchData.deviceAddress = scratchAddress[m_currentBlasIdx % scratchAddress.size()];
    data.buildInfo.pGeometries               = data.asGeometry.data();
    collectedBuildInfo.push_back(data.buildInfo);
    collectedRangeInfo.push_back(data.asBuildRangeInfo.data());

    // Update the used budget with the size of the current structure
    budgetUsed += data.sizeInfo.accelerationStructureSize;
    m_currentBlasIdx++;
  }

  // Command to build the acceleration structures on the GPU
  vkCmdBuildAccelerationStructuresKHR(cmd, static_cast<uint32_t>(collectedBuildInfo.size()), collectedBuildInfo.data(),
                                      collectedRangeInfo.data());

  // Barrier to ensure proper synchronization after building
  accelerationStructureBarrier(cmd, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);

  // If a query pool is available, record the properties of the built acceleration structures
  if(m_queryPool)
  {
    vkCmdWriteAccelerationStructuresPropertiesKHR(cmd, static_cast<uint32_t>(collectedAccel.size()), collectedAccel.data(),
                                                  VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, m_queryPool,
                                                  currentQueryIdx);
    currentQueryIdx += static_cast<uint32_t>(collectedAccel.size());
  }

  // Return the total budget used in this operation
  return budgetUsed;
}


// Compacts the Bottom-Level Acceleration Structures (BLAS) that have been built, reducing their memory footprint.
// This function uses the results from previously performed queries to determine the compacted sizes and then
// creates new, smaller acceleration structures. It also handles copying from the original to the compacted structures.
//
// Notes:
//   It assumes that a query has been performed earlier to determine the possible compacted sizes of the acceleration structures.
//
void nvvk::BlasBuilder::cmdCompactBlas(VkCommandBuffer                                    cmd,
                                       std::vector<nvvk::AccelerationStructureBuildData>& blasBuildData,
                                       std::vector<nvvk::AccelKHR>&                       blasAccel)
{
  // Compute the number of queries that have been conducted between the current BLAS index and the query index.
  uint32_t queryCtn = m_currentBlasIdx - m_currentQueryIdx;
  // Ensure there is a valid query pool and BLAS to compact;
  if(m_queryPool == VK_NULL_HANDLE || queryCtn == 0)
  {
    return;
  }


  // Retrieve the compacted sizes from the query pool.
  std::vector<VkDeviceSize> compactSizes(queryCtn);
  vkGetQueryPoolResults(m_device, m_queryPool, m_currentQueryIdx, (uint32_t)compactSizes.size(),
                        compactSizes.size() * sizeof(VkDeviceSize), compactSizes.data(), sizeof(VkDeviceSize),
                        VK_QUERY_RESULT_WAIT_BIT);

  // Iterate through each BLAS index to process compaction.
  for(size_t i = m_currentQueryIdx; i < m_currentBlasIdx; i++)
  {
    size_t       idx         = i - m_currentQueryIdx;  // Calculate local index for compactSizes vector.
    VkDeviceSize compactSize = compactSizes[idx];
    if(compactSize > 0)
    {
      // Update statistical tracking of sizes before and after compaction.
      m_stats.totalCompactSize += compactSize;
      m_stats.totalOriginalSize += blasBuildData[i].sizeInfo.accelerationStructureSize;
      blasBuildData[i].sizeInfo.accelerationStructureSize = compactSize;
      m_cleanupBlasAccel.push_back(blasAccel[i]);  // Schedule old BLAS for cleanup.

      // Create a new acceleration structure for the compacted BLAS.
      VkAccelerationStructureCreateInfoKHR asCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
      asCreateInfo.size = compactSize;
      asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      blasAccel[i]      = m_alloc->createAcceleration(asCreateInfo);

      // Command to copy the original BLAS to the newly created compacted version.
      VkCopyAccelerationStructureInfoKHR copyInfo{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
      copyInfo.src  = blasBuildData[i].buildInfo.dstAccelerationStructure;
      copyInfo.dst  = blasAccel[i].accel;
      copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
      vkCmdCopyAccelerationStructureKHR(cmd, &copyInfo);

      // Update the build data to reflect the new destination of the BLAS.
      blasBuildData[i].buildInfo.dstAccelerationStructure = blasAccel[i].accel;
    }
  }

  // Update the query index to the current BLAS index, marking the end of processing for these structures.
  m_currentQueryIdx = m_currentBlasIdx;
}


void nvvk::BlasBuilder::destroyNonCompactedBlas()
{
  for(auto& blas : m_cleanupBlasAccel)
  {
    m_alloc->destroy(blas);
  }
  m_cleanupBlasAccel.clear();
}

void nvvk::BlasBuilder::destroyQueryPool()
{
  if(m_queryPool)
  {
    vkDestroyQueryPool(m_device, m_queryPool, nullptr);
    m_queryPool = VK_NULL_HANDLE;
  }
}

void nvvk::BlasBuilder::destroy()
{
  destroyQueryPool();
  destroyNonCompactedBlas();
}


struct ScratchSizeInfo
{
  VkDeviceSize maxScratch;
  VkDeviceSize totalScratch;
};

ScratchSizeInfo calculateScratchAlignedSizes(const std::vector<nvvk::AccelerationStructureBuildData>& buildData, uint32_t minAlignment)
{
  VkDeviceSize maxScratch{0};
  VkDeviceSize totalScratch{0};

  for(auto& buildInfo : buildData)
  {
    VkDeviceSize alignedSize = nvh::align_up(buildInfo.sizeInfo.buildScratchSize, minAlignment);
    // assert(alignedSize == buildInfo.sizeInfo.buildScratchSize);  // Make sure it was already aligned
    maxScratch = std::max(maxScratch, alignedSize);
    totalScratch += alignedSize;
  }

  return {maxScratch, totalScratch};
}

// Find if the total scratch size is within the budget, otherwise return n-time the max scratch size that fits in the budget
VkDeviceSize nvvk::BlasBuilder::getScratchSize(VkDeviceSize                                             hintMaxBudget,
                                               const std::vector<nvvk::AccelerationStructureBuildData>& buildData,
                                               uint32_t minAlignment /*= 128*/) const
{
  ScratchSizeInfo sizeInfo     = calculateScratchAlignedSizes(buildData, minAlignment);
  VkDeviceSize    maxScratch   = sizeInfo.maxScratch;
  VkDeviceSize    totalScratch = sizeInfo.totalScratch;

  if(totalScratch < hintMaxBudget)
  {
    return totalScratch;
  }
  else
  {
    uint64_t numScratch = std::max(uint64_t(1), hintMaxBudget / maxScratch);
    numScratch          = std::min(numScratch, buildData.size());
    return numScratch * maxScratch;
  }
}

// Return the scratch addresses fitting the scrath strategy (see above)
void nvvk::BlasBuilder::getScratchAddresses(VkDeviceSize                                             hintMaxBudget,
                                            const std::vector<nvvk::AccelerationStructureBuildData>& buildData,
                                            VkDeviceAddress               scratchBufferAddress,
                                            std::vector<VkDeviceAddress>& scratchAddresses,
                                            uint32_t                      minAlignment /*=128*/)
{
  ScratchSizeInfo sizeInfo     = calculateScratchAlignedSizes(buildData, minAlignment);
  VkDeviceSize    maxScratch   = sizeInfo.maxScratch;
  VkDeviceSize    totalScratch = sizeInfo.totalScratch;

  // Strategy 1: scratch was large enough for all BLAS, return the addresses in order
  if(totalScratch < hintMaxBudget)
  {
    VkDeviceAddress address = {};
    for(auto& buildInfo : buildData)
    {
      scratchAddresses.push_back(scratchBufferAddress + address);
      VkDeviceSize alignedSize = nvh::align_up(buildInfo.sizeInfo.buildScratchSize, minAlignment);
      address += alignedSize;
    }
  }
  // Strategy 2: there are n-times the max scratch fitting in the budget
  else
  {
    // Make sure there is at least one scratch buffer, and not more than the number of BLAS
    uint64_t numScratch = std::max(uint64_t(1), hintMaxBudget / maxScratch);
    numScratch          = std::min(numScratch, buildData.size());

    VkDeviceAddress address = {};
    for(int i = 0; i < numScratch; i++)
    {
      scratchAddresses.push_back(scratchBufferAddress + address);
      address += maxScratch;
    }
  }
}


// Generates a formatted string summarizing the statistics of BLAS compaction results.
// The output includes the original and compacted sizes in megabytes (MB), the amount of memory saved,
// and the percentage reduction in size. This method is intended to provide a quick, human-readable
// summary of the compaction efficiency.
//
// Returns:
//   A string containing the formatted summary of the BLAS compaction statistics.
std::string nvvk::BlasBuilder::Stats::toString() const
{
  // Sizes in MB
  float originalSizeMB = totalOriginalSize / (1024.0f * 1024.0f);
  float compactSizeMB  = totalCompactSize / (1024.0f * 1024.0f);
  float savedSizeMB    = (totalOriginalSize - totalCompactSize) / (1024.0f * 1024.0f);

  float fractionSmaller =
      (totalOriginalSize == 0) ? 0.0f : (totalOriginalSize - totalCompactSize) / static_cast<float>(totalOriginalSize);

  std::string output = fmt::format("BLAS Compaction: {:.1f}MB -> {:.1f}MB ({:.1f}MB saved, {:.1f}% smaller)",
                                   originalSizeMB, compactSizeMB, savedSizeMB, fractionSmaller * 100.0f);

  return output;
}

// Returns the maximum scratch buffer size needed for building all provided acceleration structures.
// This function iterates through a vector of AccelerationStructureBuildData, comparing the scratch
// size required for each structure and returns the largest value found.
//
// Returns:
//   The maximum scratch size needed as a VkDeviceSize.
VkDeviceSize nvvk::getMaxScratchSize(const std::vector<AccelerationStructureBuildData>& asBuildData)
{
  VkDeviceSize maxScratchSize = 0;
  for(const auto& blas : asBuildData)
  {
    maxScratchSize = std::max(maxScratchSize, blas.sizeInfo.buildScratchSize);
  }
  return maxScratchSize;
}
