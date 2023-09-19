/*
 * Copyright (c) 2022-2023, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2023 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#include "gltf_scene_rtx.hpp"

#include "nvvk/buffers_vk.hpp"
#include "shaders/dh_scn_desc.h"
#include "nvh/timesampler.hpp"

nvvkhl::SceneRtx::SceneRtx(nvvk::Context* ctx, AllocVma* alloc, uint32_t queueFamilyIndex)
    : m_ctx(ctx)
    , m_alloc(alloc)
{
  // Requesting ray tracing properties
  VkPhysicalDeviceProperties2 prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  prop2.pNext = &m_rtProperties;
  vkGetPhysicalDeviceProperties2(m_ctx->m_physicalDevice, &prop2);

  // Create utilities to create BLAS/TLAS and the Shading Binding Table (SBT)
  m_rtBuilder.setup(m_ctx->m_device, m_alloc, queueFamilyIndex);
}

nvvkhl::SceneRtx::~SceneRtx()
{
  destroy();
}

void nvvkhl::SceneRtx::create(const Scene& scn, const SceneVk& scnVk, VkBuildAccelerationStructureFlagsKHR flags /*=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR */)
{
  destroy();  // Make sure not to leave allocated buffers

  createBottomLevelAS(scn.scene(), scnVk, flags);
  createTopLevelAS(scn.scene(), flags);
}

VkAccelerationStructureKHR nvvkhl::SceneRtx::tlas()
{
  return m_rtBuilder.getAccelerationStructure();
}

void nvvkhl::SceneRtx::destroy()
{
  m_rtBuilder.destroy();
}

//--------------------------------------------------------------------------------------------------
// Converting a PrimitiveMesh as input for BLAS
//
nvvk::RaytracingBuilderKHR::BlasInput nvvkhl::SceneRtx::primitiveToGeometry(const nvh::GltfPrimMesh& prim,
                                                                            VkDeviceAddress          vertexAddress,
                                                                            VkDeviceAddress          indexAddress)
{
  uint32_t max_prim_count = prim.indexCount / 3;

  // Describe buffer as array of VertexObj.
  VkAccelerationStructureGeometryTrianglesDataKHR triangles{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
  triangles.vertexFormat             = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec3 vertex position data.
  triangles.vertexData.deviceAddress = vertexAddress;
  triangles.vertexStride             = sizeof(Vertex);
  triangles.indexType                = VK_INDEX_TYPE_UINT32;
  triangles.indexData.deviceAddress  = indexAddress;
  triangles.maxVertex                = prim.vertexCount - 1;
  //triangles.transformData; // Identity

  // Identify the above data as containing opaque triangles.
  VkAccelerationStructureGeometryKHR as_geom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  as_geom.geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  as_geom.flags              = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
  as_geom.geometry.triangles = triangles;

  VkAccelerationStructureBuildRangeInfoKHR offset{};
  offset.firstVertex     = 0;
  offset.primitiveCount  = max_prim_count;
  offset.primitiveOffset = 0;
  offset.transformOffset = 0;

  // Our blas is made from only one geometry, but could be made of many geometries
  nvvk::RaytracingBuilderKHR::BlasInput input;
  input.asGeometry.emplace_back(as_geom);
  input.asBuildOffsetInfo.emplace_back(offset);

  return input;
}

void nvvkhl::SceneRtx::createBottomLevelAS(const nvh::GltfScene& scn, const SceneVk& scnVk, VkBuildAccelerationStructureFlagsKHR flags)
{
  nvh::ScopedTimer st(std::string(__FUNCTION__) + "\n");

  // BLAS - Storing each primitive in a geometry
  std::vector<nvvk::RaytracingBuilderKHR::BlasInput> all_blas;
  all_blas.reserve(scn.m_primMeshes.size());

  // Retrieve the array of primitive buffers (see in SceneVk)
  const auto& vertices = scnVk.vertices();
  const auto& indices  = scnVk.indices();

  for(uint32_t p_idx = 0; p_idx < scn.m_primMeshes.size(); p_idx++)
  {
    auto vertex_address = nvvk::getBufferDeviceAddress(m_ctx->m_device, vertices[p_idx].buffer);
    auto index_address  = nvvk::getBufferDeviceAddress(m_ctx->m_device, indices[p_idx].buffer);

    auto geo = primitiveToGeometry(scn.m_primMeshes[p_idx], vertex_address, index_address);
    all_blas.push_back({geo});
  }
  m_rtBuilder.buildBlas(all_blas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
                                      | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
}

void nvvkhl::SceneRtx::createTopLevelAS(const nvh::GltfScene& scn, VkBuildAccelerationStructureFlagsKHR flags)
{
  nvh::ScopedTimer st(__FUNCTION__);

  std::vector<VkAccelerationStructureInstanceKHR> tlas;
  tlas.reserve(scn.m_nodes.size());
  for(const auto& node : scn.m_nodes)
  {
    VkGeometryInstanceFlagsKHR flags{};
    const nvh::GltfPrimMesh&   prim_mesh = scn.m_primMeshes[node.primMesh];
    const nvh::GltfMaterial&   mat       = scn.m_materials[prim_mesh.materialIndex];

    // Always opaque, no need to use anyhit (faster)
    if(mat.alphaMode == 0 || (mat.baseColorFactor.w == 1.0F && mat.baseColorTexture == -1))
    {
      flags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
    }

    // Need to skip the cull flag in traceray_rtx for double sided materials
    if(mat.doubleSided == 1)
    {
      flags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    }

    VkAccelerationStructureInstanceKHR ray_inst{};
    ray_inst.transform                      = nvvk::toTransformMatrixKHR(node.worldMatrix);  // Position of the instance
    ray_inst.instanceCustomIndex            = node.primMesh;  // gl_InstanceCustomIndexEXT
    ray_inst.accelerationStructureReference = m_rtBuilder.getBlasDeviceAddress(node.primMesh);
    ray_inst.instanceShaderBindingTableRecordOffset = 0;  // We will use the same hit group for all objects
    ray_inst.flags                                  = flags;
    ray_inst.mask                                   = 0xFF;
    tlas.emplace_back(ray_inst);
  }
  m_rtBuilder.buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}
