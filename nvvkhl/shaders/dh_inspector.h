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


// Shader header for inspection shader variables
// Prior to including this header the following macros need to be defined
// Either INSPECTOR_MODE_COMPUTE or INSPECTOR_MODE_FRAGMENT
// If INSPECTOR_MODE_COMPUTE is defined the shader must expose invocation information (e.g. gl_LocalInvocationID).
// This typically applies to compute, task and mesh shaders
// If INSPECTOR_MODE_FRAGMENT is defined the shader must expose fragment information (e.g. gl_FragCoord).
// This applies to fragment shaders
//
// INSPECTOR_DESCRIPTOR_SET: the index of the descriptor set containing the Inspector buffers
// INSPECTOR_INSPECTION_DATA_BINDING: the binding index of the buffer containing the inspection information, as provided by ElementInspector::getComputeInspectionBuffer()
// INSPECTOR_METADATA_BINDING: the binding index of the buffer containing the inspection metadata, as provided by ElementInspector::getComputeMetadataBuffer()

#ifndef DH_INSPECTOR_H
#define DH_INSPECTOR_H

#ifdef __cplusplus
#include <cstdint>
namespace nvvkhl_shaders {
using uvec3 = glm::uvec3;
using uvec2 = glm::uvec2;
#else
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_shader_atomic_int64 : require
#extension GL_KHR_shader_subgroup_basic : require
#endif

#define WARP_SIZE 32

#define WARP_2D_SIZE_X 8
#define WARP_2D_SIZE_Y 4
#define WARP_2D_SIZE_Z 1

struct InspectorComputeMetadata
{
  uvec3    minBlock;
  uint32_t u32PerThread;
  uvec3    maxBlock;
  uint32_t minWarpInBlock;
  uint32_t maxWarpInBlock;
};

struct InspectorFragmentMetadata
{
  uvec2    minFragment;
  uvec2    maxFragment;
  uvec2    renderSize;
  uint32_t u32PerThread;
};

struct InspectorCustomMetadata
{
  uvec3    minCoord;
  uint32_t pad0;
  uvec3    maxCoord;
  uint32_t pad1;
  uvec3    extent;
  uint32_t u32PerThread;
};


#ifndef __cplusplus

#if !(defined INSPECTOR_MODE_COMPUTE) && !(defined INSPECTOR_MODE_FRAGMENT) && !(defined INSPECTOR_MODE_CUSTOM)
#error At least one inspector mode (INSPECTOR_MODE_COMPUTE, INSPECTOR_MODE_FRAGMENT, INSPECTOR_MODE_CUSTOM) must be defined before including this file
#endif

#if(defined INSPECTOR_MODE_COMPUTE) && (defined INSPECTOR_MODE_FRAGMENT)
#error Only one of inspector modes INSPECTOR_MODE_COMPUTE, INSPECTOR_MODE_FRAGMENT can be chosen
#endif

#ifndef INSPECTOR_DESCRIPTOR_SET
#error The descriptor set containing thread inspection data must be provided before including this file
#endif


#ifdef INSPECTOR_MODE_CUSTOM
#ifndef INSPECTOR_CUSTOM_INSPECTION_DATA_BINDING
#error The descriptor binding containing custom thread inspection data must be provided before including this file
#endif
#ifndef INSPECTOR_CUSTOM_METADATA_BINDING
#error The descriptor binding containing custom thread inspection metadata must be provided before including this file
#endif
#endif

#if(defined INSPECTOR_MODE_COMPUTE) || (defined INSPECTOR_MODE_FRAGMENT)
#ifndef INSPECTOR_INSPECTION_DATA_BINDING
#error The descriptor binding containing thread inspection data must be provided before including this file
#endif

#ifndef INSPECTOR_METADATA_BINDING
#error The descriptor binding containing thread inspection metadata must be provided before including this file
#endif
#endif


#ifdef INSPECTOR_MODE_COMPUTE
layout(set = INSPECTOR_DESCRIPTOR_SET, binding = INSPECTOR_INSPECTION_DATA_BINDING) buffer InspectorInspectionData
{
  uint32_t inspectorInspectionData[];
};

layout(set = INSPECTOR_DESCRIPTOR_SET, binding = INSPECTOR_METADATA_BINDING) readonly buffer InspectorComputeInspectionMetadata
{
  InspectorComputeMetadata inspectorMetadata;
};

/** @DOC_START
# Function inspect32BitValue
>  Inspect a 32-bit value at a given index
@DOC_END  */
void inspect32BitValue(uint32_t index, uint32_t v)
{

  if(clamp(gl_WorkGroupID, inspectorMetadata.minBlock, inspectorMetadata.maxBlock) != gl_WorkGroupID)
  {
    return;
  }

  uint32_t warpIndex = gl_SubgroupID;

  if(warpIndex < inspectorMetadata.minWarpInBlock || warpIndex > inspectorMetadata.maxWarpInBlock)
    return;
  uint32_t inspectedThreadsPerBlock = (inspectorMetadata.maxWarpInBlock - inspectorMetadata.minWarpInBlock + 1) * gl_SubgroupSize;
  ;

  uint32_t blockIndex = gl_WorkGroupID.x + gl_NumWorkGroups.x * (gl_WorkGroupID.y + gl_NumWorkGroups.y * gl_WorkGroupID.z);
  uint32_t minBlockIndex =
      inspectorMetadata.minBlock.x
      + gl_NumWorkGroups.x * (inspectorMetadata.minBlock.y + gl_NumWorkGroups.y * inspectorMetadata.minBlock.z);

  uint32_t blockStart = inspectedThreadsPerBlock * (blockIndex - minBlockIndex) * inspectorMetadata.u32PerThread;
  uint32_t warpStart = (warpIndex - inspectorMetadata.minWarpInBlock) * inspectorMetadata.u32PerThread * gl_SubgroupSize;
  uint32_t threadInWarpStart = gl_SubgroupInvocationID * inspectorMetadata.u32PerThread;

  inspectorInspectionData[blockStart + warpStart + threadInWarpStart + index] = v;
}

#endif

#ifdef INSPECTOR_MODE_FRAGMENT

layout(set = INSPECTOR_DESCRIPTOR_SET, binding = INSPECTOR_INSPECTION_DATA_BINDING) buffer InspectorInspectionData
{
  uint64_t inspectorInspectionData[];
};

layout(set = INSPECTOR_DESCRIPTOR_SET, binding = INSPECTOR_METADATA_BINDING) readonly buffer InspectorFragmentInspectionMetadata
{
  InspectorFragmentMetadata inspectorMetadata;
};

void inspect32BitValue(uint32_t index, uint32_t v)
{

  uvec2 fragment = uvec2(floor(gl_FragCoord.xy));


  if(clamp(fragment, inspectorMetadata.minFragment, inspectorMetadata.maxFragment) != fragment)
  {
    return;
  }


  uvec2    localFragment   = fragment - inspectorMetadata.minFragment;
  uint32_t inspectionWidth = inspectorMetadata.maxFragment.x - inspectorMetadata.minFragment.x + 1;
  uint32_t fragmentIndex   = localFragment.x + inspectionWidth * localFragment.y;

  // Atomically store the fragment depth along with the value so we always keep the fragment value
  // with the lowest depth
  float    z     = 1.f - clamp(gl_FragCoord.z, 0.f, 1.f);
  uint64_t zUint = floatBitsToUint(z);
  uint64_t value = (zUint << 32) | uint64_t(v);
  atomicMax(inspectorInspectionData[fragmentIndex * inspectorMetadata.u32PerThread / 2 + index], value);
}

#endif

#ifdef INSPECTOR_MODE_CUSTOM

layout(set = INSPECTOR_DESCRIPTOR_SET, binding = INSPECTOR_CUSTOM_INSPECTION_DATA_BINDING) buffer InspectorCustomInspections
{
  uint32_t inspectorCustomInspection[];
};

layout(set = INSPECTOR_DESCRIPTOR_SET, binding = INSPECTOR_CUSTOM_METADATA_BINDING) readonly buffer InspectorCustomInspectionMetadata
{
  InspectorCustomMetadata inspectorCustomMetadata;
};

/** @DOC_START
# Function inspectCustom32BitValue
>  Inspect a 32-bit value at a given index
@DOC_END  */
void inspectCustom32BitValue(uint32_t index, uvec3 location, uint32_t v)
{
  if(clamp(location, inspectorCustomMetadata.minCoord, inspectorCustomMetadata.maxCoord) != location)
  {
    return;
  }


  uvec3    localCoord       = location - inspectorCustomMetadata.minCoord;
  uint32_t inspectionWidth  = inspectorCustomMetadata.maxCoord.x - inspectorCustomMetadata.minCoord.x + 1;
  uint32_t inspectionHeight = inspectorCustomMetadata.maxCoord.y - inspectorCustomMetadata.minCoord.y + 1;
  uint32_t coordIndex       = localCoord.x + inspectionWidth * (localCoord.y + inspectionHeight * localCoord.z);

  inspectorCustomInspection[coordIndex * inspectorCustomMetadata.u32PerThread + index] = v;
}


#endif


#endif

#ifdef __cplusplus
}  // namespace nvvkhl_shaders
#endif

#endif  // DH_INSPECTOR_H