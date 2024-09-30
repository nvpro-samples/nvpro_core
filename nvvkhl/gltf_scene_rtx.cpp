/*
 * Copyright (c) 2022-2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2024, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cinttypes>
#include <numeric>

#include "gltf_scene_rtx.hpp"

#include "nvvk/buffers_vk.hpp"
#include "shaders/dh_scn_desc.h"
#include "nvh/timesampler.hpp"
#include "nvh/alignment.hpp"
#include "fileformats/tinygltf_utils.hpp"

nvvkhl::SceneRtx::SceneRtx(VkDevice device, VkPhysicalDevice physicalDevice, nvvk::ResourceAllocator* alloc, uint32_t queueFamilyIndex)
    : m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_alloc(alloc)
{
  // Requesting ray tracing properties
  VkPhysicalDeviceProperties2 prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  m_rtProperties.pNext = &m_rtASProperties;
  prop2.pNext          = &m_rtProperties;
  vkGetPhysicalDeviceProperties2(m_physicalDevice, &prop2);

  m_device      = m_device;
  m_dutil       = std::make_unique<nvvk::DebugUtil>(m_device);  // Debug utility
  m_blasBuilder = std::make_unique<nvvk::BlasBuilder>(alloc, m_device);
}

nvvkhl::SceneRtx::~SceneRtx()
{
  destroy();
}

void nvvkhl::SceneRtx::create(VkCommandBuffer cmd, const nvh::gltf::Scene& scn, const SceneVk& scnVk, VkBuildAccelerationStructureFlagsKHR flags)
{
  createBottomLevelAccelerationStructure(scn, scnVk, flags);

  bool finished = false;
  do
  {
    // This won't compact the BLAS, but will create the acceleration structure
    finished = cmdBuildBottomLevelAccelerationStructure(cmd, 512'000'000);
  } while(!finished);

  cmdCreateBuildTopLevelAccelerationStructure(cmd, scn);
}

VkAccelerationStructureKHR nvvkhl::SceneRtx::tlas()
{
  return m_tlasAccel.accel;
}

void nvvkhl::SceneRtx::destroy()
{
  for(auto& blas : m_blasAccel)
    m_alloc->destroy(blas);

  m_alloc->destroy(m_instancesBuffer);
  destroyScratchBuffers();

  m_alloc->destroy(m_tlasAccel);
  m_blasAccel     = {};
  m_blasBuildData = {};
  m_tlasAccel     = {};
  m_tlasBuildData = {};
  m_blasBuilder.reset();
}


void nvvkhl::SceneRtx::destroyScratchBuffers()
{
  m_alloc->destroy(m_tlasScratchBuffer);
  m_alloc->destroy(m_blasScratchBuffer);
}

//--------------------------------------------------------------------------------------------------
// Converting a PrimitiveMesh as input for BLAS
//
nvvk::AccelerationStructureGeometryInfo nvvkhl::SceneRtx::renderPrimitiveToAsGeometry(const nvh::gltf::RenderPrimitive& prim,
                                                                                      VkDeviceAddress vertexAddress,
                                                                                      VkDeviceAddress indexAddress)
{
  nvvk::AccelerationStructureGeometryInfo result;
  uint32_t                                numTriangles = prim.indexCount / 3;

  // Describe buffer as array of VertexObj.
  VkAccelerationStructureGeometryTrianglesDataKHR triangles{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
  triangles.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;  // vec3 vertex position data.
  triangles.vertexData.deviceAddress = vertexAddress;
  triangles.vertexStride             = sizeof(glm::vec3);
  triangles.indexType                = VK_INDEX_TYPE_UINT32;
  triangles.indexData.deviceAddress  = indexAddress;
  triangles.maxVertex                = prim.vertexCount - 1;
  //triangles.transformData; // Identity

  // Identify the above data as containing opaque triangles.
  result.geometry.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  result.geometry.geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  result.geometry.flags              = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
  result.geometry.geometry.triangles = triangles;

  result.rangeInfo.firstVertex     = 0;
  result.rangeInfo.primitiveCount  = numTriangles;
  result.rangeInfo.primitiveOffset = 0;
  result.rangeInfo.transformOffset = 0;

  return result;
}

void nvvkhl::SceneRtx::createBottomLevelAccelerationStructure(const nvh::gltf::Scene&              scene,
                                                              const SceneVk&                       sceneVk,
                                                              VkBuildAccelerationStructureFlagsKHR flags)
{
  nvh::ScopedTimer st(__FUNCTION__);

  destroy();  // Make sure not to leave allocated buffers

  auto& renderPrimitives = scene.getRenderPrimitives();

  // BLAS - Storing each primitive in a geometry
  m_blasBuildData.resize(renderPrimitives.size());
  m_blasAccel.resize(m_blasBuildData.size());

  // Retrieve the array of primitive buffers (see in SceneVk)
  const auto& vertexBuffers = sceneVk.vertexBuffers();
  const auto& indices       = sceneVk.indices();

  for(uint32_t p_idx = 0; p_idx < renderPrimitives.size(); p_idx++)
  {
    auto& blasData  = m_blasBuildData[p_idx];
    blasData.asType = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    // Converting each primitive into the required format for the BLAS
    VkDeviceAddress vertexAddress = vertexBuffers[p_idx].position.address;
    VkDeviceAddress indexAddress  = indices[p_idx].address;
    // Fill the BLAS information
    auto geo = renderPrimitiveToAsGeometry(renderPrimitives[p_idx], vertexAddress, indexAddress);
    blasData.addGeometry(geo);
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo = blasData.finalizeGeometry(m_device, flags);  // Will query the size of the resulting BLAS
  }

  // Create the Blas Builder and query pool for compaction
  m_blasBuilder = std::make_unique<nvvk::BlasBuilder>(m_alloc, m_device);
}

//--------------------------------------------------------------------------------------------------
bool nvvkhl::SceneRtx::cmdBuildBottomLevelAccelerationStructure(VkCommandBuffer cmd, VkDeviceSize hintMaxBudget /*= 512'000'000*/)
{
  nvh::ScopedTimer st(__FUNCTION__);
  assert(m_blasBuilder);

  destroyScratchBuffers();

  uint32_t minAlignment = m_rtASProperties.minAccelerationStructureScratchOffsetAlignment;
  // 1) finding the largest scratch size
  VkDeviceSize scratchSize = m_blasBuilder->getScratchSize(hintMaxBudget, m_blasBuildData, minAlignment);
  // 2) allocating the scratch buffer
  m_blasScratchBuffer =
      m_alloc->createBuffer(scratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  // 3) getting the device address for the scratch buffer
  std::vector<VkDeviceAddress> scratchAddresses;
  m_blasBuilder->getScratchAddresses(hintMaxBudget, m_blasBuildData, m_blasScratchBuffer.address, scratchAddresses, minAlignment);

  // Building all BLAS in parallel, as long as there are enough budget
  return m_blasBuilder->cmdCreateParallelBlas(cmd, m_blasBuildData, m_blasAccel, scratchAddresses, hintMaxBudget);
}

//--------------------------------------------------------------------------------------------------
// Create the top-level acceleration structure from all the BLAS
//
void nvvkhl::SceneRtx::cmdCreateBuildTopLevelAccelerationStructure(VkCommandBuffer cmd, const nvh::gltf::Scene& scene)
{
  nvh::ScopedTimer st(__FUNCTION__);
  const auto&      materials   = scene.getModel().materials;
  const auto&      drawObjects = scene.getRenderNodes();

  uint32_t instanceCount = static_cast<uint32_t>(drawObjects.size());

  m_tlasInstances.clear();
  m_tlasInstances.reserve(instanceCount);
  for(const auto& object : drawObjects)
  {
    VkGeometryInstanceFlagsKHR instanceFlags{};
    const tinygltf::Material&  mat = materials[object.materialID];

    KHR_materials_transmission transmission = tinygltf::utils::getTransmission(mat);
    KHR_materials_volume       volume       = tinygltf::utils::getVolume(mat);

    // Always opaque, no need to use anyhit (faster)
    if((mat.alphaMode == "OPAQUE"
        || (mat.pbrMetallicRoughness.baseColorFactor[3] == 1.0F && mat.pbrMetallicRoughness.baseColorTexture.index == -1))
       && transmission.factor == 0)
    {
      instanceFlags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
    }

    // Need to skip the cull flag in traceray_rtx for double sided materials
    if(mat.doubleSided == 1 || volume.thicknessFactor > 0.0F)
    {
      instanceFlags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    }

    VkDeviceAddress blasAddress = m_blasAccel[object.renderPrimID].address;
    if(!object.visible)
      blasAddress = 0;  // The instance is added, but the BLAS is set to null making it invisible

    VkAccelerationStructureInstanceKHR asInstance{};
    asInstance.transform           = nvvk::toTransformMatrixKHR(object.worldMatrix);  // Position of the instance
    asInstance.instanceCustomIndex = object.renderPrimID;                             // gl_InstanceCustomIndexEXT
    asInstance.accelerationStructureReference         = blasAddress;                  // The reference to the BLAS
    asInstance.instanceShaderBindingTableRecordOffset = 0;     // We will use the same hit group for all objects
    asInstance.mask                                   = 0x01;  // Visibility mask
    asInstance.flags                                  = instanceFlags;

    // Storing the instance
    m_tlasInstances.push_back(asInstance);
  }

  VkBuildAccelerationStructureFlagsKHR buildFlags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  //if(scene.hasAnimation())
  {
    buildFlags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  }

  // Create a buffer holding the actual instance data (matrices++) for use by the AS builder
  m_instancesBuffer = m_alloc->createBuffer(cmd, m_tlasInstances,
                                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                                                | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
  m_dutil->DBG_NAME(m_instancesBuffer.buffer);

  m_tlasBuildData.asType = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  auto geo               = m_tlasBuildData.makeInstanceGeometry(m_tlasInstances.size(), m_instancesBuffer.address);
  m_tlasBuildData.addGeometry(geo);

  // Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
  nvvk::accelerationStructureBarrier(cmd, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);

  // Calculate the amount of scratch memory needed to build the TLAS
  VkAccelerationStructureBuildSizesInfoKHR sizeInfo = m_tlasBuildData.finalizeGeometry(m_device, buildFlags);

  // Create the scratch buffer needed during build of the TLAS
  m_tlasScratchBuffer = m_alloc->createBuffer(sizeInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                                                                             | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

  VkAccelerationStructureCreateInfoKHR createInfo = m_tlasBuildData.makeCreateInfo();
  m_tlasAccel                                     = m_alloc->createAcceleration(createInfo);
  m_dutil->DBG_NAME(m_tlasAccel.accel);

  // Build the TLAS
  m_tlasBuildData.cmdBuildAccelerationStructure(cmd, m_tlasAccel.accel, m_tlasScratchBuffer.address);

  // Make sure to have the TLAS ready before using it
  nvvk::accelerationStructureBarrier(cmd, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
}


// This function is called when the scene has been updated
void nvvkhl::SceneRtx::updateTopLevelAS(VkCommandBuffer cmd, const nvh::gltf::Scene& scn)
{
  //nvh::ScopedTimer st(__FUNCTION__);
  const std::vector<nvh::gltf::RenderNode>& drawObjects = scn.getRenderNodes();

  uint32_t numVisibleElement = 0;
  // Updating all matrices
  for(size_t i = 0; i < drawObjects.size(); i++)
  {
    auto& object                 = drawObjects[i];
    m_tlasInstances[i].transform = nvvk::toTransformMatrixKHR(object.worldMatrix);  // Position of the instance

    VkDeviceAddress blasAddress = m_blasAccel[object.renderPrimID].address;
    if(object.visible)
    {
      numVisibleElement++;
      m_tlasInstances[i].accelerationStructureReference = blasAddress;  // The reference to the BLAS
    }
    else
    {
      m_tlasInstances[i].accelerationStructureReference = 0;  // Making the instance invisible
    }
  }

  // Update the instance buffer
  m_alloc->getStaging()->cmdToBuffer(cmd, m_instancesBuffer.buffer, 0,
                                     m_tlasInstances.size() * sizeof(VkAccelerationStructureInstanceKHR),
                                     m_tlasInstances.data());

  // Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
  nvvk::accelerationStructureBarrier(cmd, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);

  if(m_tlasScratchBuffer.buffer == VK_NULL_HANDLE)
  {
    m_tlasScratchBuffer = m_alloc->createBuffer(m_tlasBuildData.sizeInfo.buildScratchSize,
                                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  }

  // Building or updating the top-level acceleration structure
  if(m_numVisibleElement != numVisibleElement)
  {
    m_tlasBuildData.cmdBuildAccelerationStructure(cmd, m_tlasAccel.accel, m_tlasScratchBuffer.address);
  }
  else
  {
    m_tlasBuildData.cmdUpdateAccelerationStructure(cmd, m_tlasAccel.accel, m_tlasScratchBuffer.address);
  }

  m_numVisibleElement = numVisibleElement;

  // Make sure to have the TLAS ready before using it
  nvvk::accelerationStructureBarrier(cmd, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
}


void nvvkhl::SceneRtx::cmdCompactBlas(VkCommandBuffer cmd)
{
  nvh::ScopedTimer st(__FUNCTION__ + std::string("\n"));
  m_blasBuilder->cmdCompactBlas(cmd, m_blasBuildData, m_blasAccel);

  auto stats = m_blasBuilder->getStatistics().toString();
  LOGI("%s%s\n", nvh::ScopedTimer::indent().c_str(), stats.c_str());
}

void nvvkhl::SceneRtx::destroyNonCompactedBlas()
{
  m_blasBuilder->destroyNonCompactedBlas();
}
