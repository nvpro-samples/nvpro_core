/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "nvvk/context_vk.hpp"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/raytraceKHR_vk.hpp"
#include "nvvkhl/alloc_vma.hpp"
#include "nvvkhl/pipeline_container.hpp"

#include "gltf_scene_vk.hpp"

namespace nvvkhl {

class SceneRtx
{
public:
  SceneRtx(nvvk::Context* ctx, AllocVma* alloc, uint32_t queueFamilyIndex = 0U);
  ~SceneRtx();

  // Create both bottom and top level acceleration structures
  void create(const Scene&                         scn,
              const SceneVk&                       scnVk,
              VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
                                                           | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
  // Create all bottom level acceleration structures (BLAS)
  virtual void createBottomLevelAS(const nvh::GltfScene& scn,
                                   const SceneVk&        scnVk,
                                   VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
                                                                                | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);

  // Create the top level acceleration structures, referencing all BLAS
  virtual void createTopLevelAS(const nvh::GltfScene& scn,
                                VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
                                                                             | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);

  // Return the constructed acceleration structure
  VkAccelerationStructureKHR tlas();

 
  void destroy();

protected:
  nvvk::RaytracingBuilderKHR::BlasInput primitiveToGeometry(const nvh::GltfPrimMesh& prim,
                                                            VkDeviceAddress          vertexAddress,
                                                            VkDeviceAddress          indexAddress);

  nvvk::Context* m_ctx;
  AllocVma*      m_alloc;

  VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
  nvvk::RaytracingBuilderKHR m_rtBuilder;
};

}  // namespace nvvkhl
