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

#pragma once

#include "nvvk/context_vk.hpp"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"
#include "nvvkhl/pipeline_container.hpp"

#include "gltf_scene_vk.hpp"
#include "nvvk/acceleration_structures.hpp"

/** @DOC_START
# class nvvkhl::SceneRtx

>  This class is responsible for the ray tracing acceleration structure. 

It is using the `nvh::gltf::Scene` and `nvvkhl::SceneVk` information to create the acceleration structure.

 @DOC_END */
namespace nvvkhl {

class SceneRtx
{
public:
  SceneRtx(VkDevice device, VkPhysicalDevice physicalDevice, nvvk::ResourceAllocator* alloc, uint32_t queueFamilyIndex = 0U);
  ~SceneRtx();

  // Create both bottom and top level acceleration structures (cannot compact)
  void create(VkCommandBuffer cmd, const nvh::gltf::Scene& scn, const SceneVk& scnVk, VkBuildAccelerationStructureFlagsKHR flags);
  // Create the bottom level acceleration structure
  void createBottomLevelAccelerationStructure(const nvh::gltf::Scene& scene, const SceneVk& sceneVk, VkBuildAccelerationStructureFlagsKHR flags);
  // Build the bottom level acceleration structure
  bool cmdBuildBottomLevelAccelerationStructure(VkCommandBuffer cmd, VkDeviceSize hintMaxBudget = 512'000'000);

  // Create the top level acceleration structure
  void cmdCreateBuildTopLevelAccelerationStructure(VkCommandBuffer cmd, const nvh::gltf::Scene& scene);
  // Compact the bottom level acceleration structure
  void cmdCompactBlas(VkCommandBuffer cmd);
  // Destroy the original acceleration structures that was compacted
  void destroyNonCompactedBlas();
  // Update the instance buffer and build the TLAS (animation)
  void updateTopLevelAS(VkCommandBuffer cmd, const nvh::gltf::Scene& scene);

  void updateBottomLevelAS(VkCommandBuffer cmd, const nvh::gltf::Scene& scene);

  // Return the constructed acceleration structure
  VkAccelerationStructureKHR tlas();

  // Destroy all acceleration structures
  void destroy();
  void destroyScratchBuffers();

protected:
  VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
  VkPhysicalDeviceAccelerationStructurePropertiesKHR m_rtASProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};

  nvvk::AccelerationStructureGeometryInfo renderPrimitiveToAsGeometry(const nvh::gltf::RenderPrimitive& prim,
                                                                      VkDeviceAddress                   vertexAddress,
                                                                      VkDeviceAddress                   indexAddress);

  VkDevice         m_device         = VK_NULL_HANDLE;
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

  nvvk::ResourceAllocator*           m_alloc = nullptr;
  std::unique_ptr<nvvk::DebugUtil>   m_dutil;
  std::unique_ptr<nvvk::BlasBuilder> m_blasBuilder;

  std::vector<nvvk::AccelerationStructureBuildData> m_blasBuildData;
  std::vector<nvvk::AccelKHR>                       m_blasAccel;

  nvvk::AccelerationStructureBuildData            m_tlasBuildData;
  nvvk::AccelKHR                                  m_tlasAccel;
  std::vector<VkAccelerationStructureInstanceKHR> m_tlasInstances;

  nvvk::Buffer m_blasScratchBuffer;
  nvvk::Buffer m_tlasScratchBuffer;
  nvvk::Buffer m_instancesBuffer;

  uint32_t m_numVisibleElement = 0;
};

}  // namespace nvvkhl
