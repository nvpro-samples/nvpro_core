#include "raytraceNV_vk.hpp"
#include <cinttypes>

void nvvk::RaytracingBuilderNV::setup(VkDevice device, nvvk::ResourceAllocator* allocator, uint32_t queueIndex)
{
  m_device     = device;
  m_queueIndex = queueIndex;
  m_debug.setup(device);
  m_alloc = allocator;
}

void nvvk::RaytracingBuilderNV::destroy()
{
  for(auto& b : m_blas)
  {
    m_alloc->destroy(b.as);
  }
  m_alloc->destroy(m_tlas.as);
  m_alloc->destroy(m_instBuffer);
}

VkAccelerationStructureNV nvvk::RaytracingBuilderNV::getAccelerationStructure() const
{
  return m_tlas.as.accel;
}

void nvvk::RaytracingBuilderNV::buildBlas(const std::vector<std::vector<VkGeometryNV>>& geoms, VkBuildAccelerationStructureFlagsNV flags)
{
  m_blas.resize(geoms.size());

  VkDeviceSize maxScratch{0};

  // Is compaction requested?
  bool doCompaction = (flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_NV)
                      == VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_NV;
  std::vector<VkDeviceSize> originalSizes;
  originalSizes.resize(m_blas.size());


  // Iterate over the groups of geometries, creating one BLAS for each group
  for(size_t i = 0; i < geoms.size(); i++)
  {
    Blas& blas{m_blas[i]};

    // Set the geometries that will be part of the BLAS
    blas.asInfo.geometryCount = static_cast<uint32_t>(geoms[i].size());
    blas.asInfo.pGeometries   = geoms[i].data();
    blas.asInfo.flags         = flags;
    VkAccelerationStructureCreateInfoNV createinfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV};
    createinfo.info = blas.asInfo;

    // Create an acceleration structure identifier and allocate memory to store the
    // resulting structure data
    blas.as = m_alloc->createAcceleration(createinfo);
    m_debug.setObjectName(blas.as.accel, (std::string("Blas" + std::to_string(i)).c_str()));

    // Estimate the amount of scratch memory required to build the BLAS, and update the
    // size of the scratch buffer that will be allocated to sequentially build all BLASes
    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV};
    memoryRequirementsInfo.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
    memoryRequirementsInfo.accelerationStructure = blas.as.accel;


    VkMemoryRequirements2 reqMem{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memoryRequirementsInfo, &reqMem);
    VkDeviceSize scratchSize = reqMem.memoryRequirements.size;


    // Original size
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memoryRequirementsInfo, &reqMem);
    originalSizes[i] = reqMem.memoryRequirements.size;

    maxScratch = std::max(maxScratch, scratchSize);
  }

  // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
  nvvk::Buffer scratchBuffer = m_alloc->createBuffer(maxScratch, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV);


  // Query size of compact BLAS
  VkQueryPoolCreateInfo qpci{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
  qpci.queryCount = (uint32_t)m_blas.size();
  qpci.queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_NV;
  VkQueryPool queryPool;
  vkCreateQueryPool(m_device, &qpci, nullptr, &queryPool);


  // Create a command buffer containing all the BLAS builds
  nvvk::CommandPool            genCmdBuf(m_device, m_queueIndex);
  int                          ctr{0};
  std::vector<VkCommandBuffer> allCmdBufs;
  allCmdBufs.reserve(m_blas.size());
  for(auto& blas : m_blas)
  {
    VkCommandBuffer cmdBuf = genCmdBuf.createCommandBuffer();
    allCmdBufs.push_back(cmdBuf);

    vkCmdBuildAccelerationStructureNV(cmdBuf, &blas.asInfo, nullptr, 0, VK_FALSE, blas.as.accel, nullptr, scratchBuffer.buffer, 0);

    // Since the scratch buffer is reused across builds, we need a barrier to ensure one build
    // is finished before starting the next one
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    // Query the compact size
    if(doCompaction)
    {
      vkCmdWriteAccelerationStructuresPropertiesNV(cmdBuf, 1, &blas.as.accel,
                                                   VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_NV, queryPool, ctr++);
    }
  }
  genCmdBuf.submitAndWait(allCmdBufs);
  allCmdBufs.clear();


  // Compacting all BLAS
  if(doCompaction)
  {
    VkCommandBuffer cmdBuf = genCmdBuf.createCommandBuffer();

    // Get the size result back
    std::vector<VkDeviceSize> compactSizes(m_blas.size());
    vkGetQueryPoolResults(m_device, queryPool, 0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize),
                          compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);


    // Compacting
    std::vector<nvvk::AccelNV> cleanupAS(m_blas.size());
    uint32_t                   totOriginalSize{0}, totCompactSize{0};
    for(int i = 0; i < m_blas.size(); i++)
    {
      LOGI("Reducing %i, from %" PRIu64 " to %" PRIu64 " \n", i, originalSizes[i], compactSizes[i]);
      totOriginalSize += (uint32_t)originalSizes[i];
      totCompactSize += (uint32_t)compactSizes[i];

      // Creating a compact version of the AS
      VkAccelerationStructureInfoNV asInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV};
      asInfo.type  = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
      asInfo.flags = flags;
      VkAccelerationStructureCreateInfoNV asCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV};
      asCreateInfo.compactedSize = compactSizes[i];
      asCreateInfo.info          = asInfo;
      auto as                    = m_alloc->createAcceleration(asCreateInfo);

      // Copy the original BLAS to a compact version
      vkCmdCopyAccelerationStructureNV(cmdBuf, as.accel, m_blas[i].as.accel, VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_NV);

      cleanupAS[i] = m_blas[i].as;
      m_blas[i].as = as;
    }
    genCmdBuf.submitAndWait(cmdBuf);

    // Destroying the previous version
    for(auto as : cleanupAS)
      m_alloc->destroy(as);

    LOGI("------------------\n");
    const float fractionSmaller = (totOriginalSize == 0) ? 0 : (totOriginalSize - totCompactSize) / float(totOriginalSize);
    LOGI("Total: %d -> %d = %d (%2.2f%s smaller) \n", totOriginalSize, totCompactSize, totOriginalSize - totCompactSize,
         fractionSmaller * 100.f, "%%");
  }

  vkDestroyQueryPool(m_device, queryPool, nullptr);
  m_alloc->destroy(scratchBuffer);
  m_alloc->finalizeAndReleaseStaging();
}

VkGeometryInstanceNV nvvk::RaytracingBuilderNV::instanceToVkGeometryInstanceNV(const nvvk::RaytracingBuilderNV::Instance& instance)
{
  Blas& blas{m_blas[instance.blasId]};
  // For each BLAS, fetch the acceleration structure handle that will allow the builder to
  // directly access it from the device
  uint64_t asHandle = 0;
  vkGetAccelerationStructureHandleNV(m_device, blas.as.accel, sizeof(uint64_t), &asHandle);

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

void nvvk::RaytracingBuilderNV::buildTlas(const std::vector<nvvk::RaytracingBuilderNV::Instance>& instances,
                                          VkBuildAccelerationStructureFlagsNV                     flags)
{
  // Set the instance count required to determine how much memory the TLAS will use
  m_tlas.asInfo.instanceCount = static_cast<uint32_t>(instances.size());
  m_tlas.asInfo.flags         = flags;
  VkAccelerationStructureCreateInfoNV accelerationStructureInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV};
  accelerationStructureInfo.info = m_tlas.asInfo;
  // Create the acceleration structure object and allocate the memory required to hold the TLAS data
  m_tlas.as = m_alloc->createAcceleration(accelerationStructureInfo);
  m_debug.setObjectName(m_tlas.as.accel, "Tlas");

  // Compute the amount of scratch memory required by the acceleration structure builder
  VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV};
  memoryRequirementsInfo.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
  memoryRequirementsInfo.accelerationStructure = m_tlas.as.accel;

  VkMemoryRequirements2 reqMem{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memoryRequirementsInfo, &reqMem);
  VkDeviceSize scratchSize = reqMem.memoryRequirements.size;


  // Allocate the scratch memory
  nvvk::Buffer scratchBuffer = m_alloc->createBuffer(scratchSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV);

  // For each instance, build the corresponding instance descriptor
  std::vector<VkGeometryInstanceNV> geometryInstances;
  geometryInstances.reserve(instances.size());
  for(const auto& inst : instances)
  {
    geometryInstances.push_back(instanceToVkGeometryInstanceNV(inst));
  }

  // Building the TLAS
  nvvk::CommandPool genCmdBuf(m_device, m_queueIndex);
  VkCommandBuffer   cmdBuf = genCmdBuf.createCommandBuffer();

  // Allocate the instance buffer and copy its contents from host to device memory
  m_instBuffer = m_alloc->createBuffer(cmdBuf, geometryInstances, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV);
  m_debug.setObjectName(m_instBuffer.buffer, "TLASInstances");

  // Make sure the copy of the instance buffer are copied before triggering the
  // acceleration structure build
  VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
  vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0,
                       1, &barrier, 0, nullptr, 0, nullptr);


  // Build the TLAS
  vkCmdBuildAccelerationStructureNV(cmdBuf, &m_tlas.asInfo, m_instBuffer.buffer, 0, VK_FALSE, m_tlas.as.accel, nullptr,
                                    scratchBuffer.buffer, 0);


  genCmdBuf.submitAndWait(cmdBuf);

  m_alloc->finalizeAndReleaseStaging();
  m_alloc->destroy(scratchBuffer);
}

void nvvk::RaytracingBuilderNV::updateTlasMatrices(const std::vector<nvvk::RaytracingBuilderNV::Instance>& instances)
{
  VkDeviceSize bufferSize = instances.size() * sizeof(VkGeometryInstanceNV);
  // Create a staging buffer on the host to upload the new instance data
  nvvk::Buffer stagingBuffer = m_alloc->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
#if defined(NVVK_ALLOC_VMA)
                                                     VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU
#else
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
#endif
  );

  // Copy the instance data into the staging buffer
  auto* gInst = reinterpret_cast<VkGeometryInstanceNV*>(m_alloc->map(stagingBuffer));
  for(int i = 0; i < instances.size(); i++)
  {
    gInst[i] = instanceToVkGeometryInstanceNV(instances[i]);
  }
  m_alloc->unmap(stagingBuffer);

  // Compute the amount of scratch memory required by the AS builder to update the TLAS
  VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV};
  memoryRequirementsInfo.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
  memoryRequirementsInfo.accelerationStructure = m_tlas.as.accel;

  VkMemoryRequirements2 reqMem{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memoryRequirementsInfo, &reqMem);
  VkDeviceSize scratchSize = reqMem.memoryRequirements.size;


  // Allocate the scratch buffer
  nvvk::Buffer scratchBuffer = m_alloc->createBuffer(scratchSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV);

  // Update the instance buffer on the device side and build the TLAS
  nvvk::CommandPool genCmdBuf(m_device, m_queueIndex);
  VkCommandBuffer   cmdBuf = genCmdBuf.createCommandBuffer();

  VkBufferCopy region{0, 0, bufferSize};
  vkCmdCopyBuffer(cmdBuf, stagingBuffer.buffer, m_instBuffer.buffer, 1, &region);

  // Make sure the copy of the instance buffer are copied before triggering the
  // acceleration structure build
  VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
  vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0,
                       1, &barrier, 0, nullptr, 0, nullptr);


  // Update the acceleration structure. Note the VK_TRUE parameter to trigger the update,
  // and the existing TLAS being passed and updated in place
  vkCmdBuildAccelerationStructureNV(cmdBuf, &m_tlas.asInfo, m_instBuffer.buffer, 0, VK_TRUE, m_tlas.as.accel,
                                    m_tlas.as.accel, scratchBuffer.buffer, 0);


  genCmdBuf.submitAndWait(cmdBuf);

  m_alloc->destroy(scratchBuffer);
  m_alloc->destroy(stagingBuffer);
}

void nvvk::RaytracingBuilderNV::updateBlas(uint32_t blasIdx)
{
  Blas& blas = m_blas[blasIdx];

  // Compute the amount of scratch memory required by the AS builder to update the TLAS
  VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV};
  memoryRequirementsInfo.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
  memoryRequirementsInfo.accelerationStructure = blas.as.accel;

  VkMemoryRequirements2 reqMem{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memoryRequirementsInfo, &reqMem);
  VkDeviceSize scratchSize = reqMem.memoryRequirements.size;

  // Allocate the scratch buffer
  nvvk::Buffer scratchBuffer = m_alloc->createBuffer(scratchSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV);

  // Update the instance buffer on the device side and build the TLAS
  nvvk::CommandPool genCmdBuf(m_device, m_queueIndex);
  VkCommandBuffer   cmdBuf = genCmdBuf.createCommandBuffer();


  // Update the acceleration structure. Note the VK_TRUE parameter to trigger the update,
  // and the existing BLAS being passed and updated in place
  vkCmdBuildAccelerationStructureNV(cmdBuf, &blas.asInfo, nullptr, 0, VK_TRUE, blas.as.accel, blas.as.accel,
                                    scratchBuffer.buffer, 0);

  genCmdBuf.submitAndWait(cmdBuf);
  m_alloc->destroy(scratchBuffer);
}
