/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

\class nvvk::SBTWrapper

nvvk::SBTWrapper is a generic SBT builder from the ray tracing pipeline

The builder will iterate through the pipeline create info `VkRayTracingPipelineCreateInfoKHR`
to find the number of raygen, miss, hit and callable shader groups were created. 
The handles for those group will be retrieved from the pipeline and written in the right order in
separated buffer.

Convenient functions exist to retrieve all information to be used in TraceRayKHR.

## Usage
- Setup the builder (`setup()`)
- After the pipeline creation, call `create()` with the same info used for the creation of the pipeline.
- Use `getRegions()` to get all the vk::StridedDeviceAddressRegionKHR needed by TraceRayKHR()


### Example
\code{.cpp}
m_sbtWrapper.setup(m_device, m_graphicsQueueIndex, &m_alloc, m_rtProperties);
// ...
m_sbtWrapper.create(m_rtPipeline, rayPipelineInfo);
// ...
auto& regions = m_stbWrapper.getRegions();
vkCmdTraceRaysKHR(cmdBuf, &regions[0], &regions[1], &regions[2], &regions[3], size.width, size.height, 1);
\endcode


## Extra

If data are attached to a shader group (see shaderRecord), it need to be provided independently.
In this case, the user must know the group index for the group type. 

Here the Hit group 1 and 2 has data, but not the group 0. 
Those functions must be called before create.

\code{.cpp}
m_sbtWrapper.addData(SBTWrapper::eHit, 1, m_hitShaderRecord[0]);
m_sbtWrapper.addData(SBTWrapper::eHit, 2, m_hitShaderRecord[1]);
\endcode


## Special case

It is also possible to create a pipeline with only a few groups but having a SBT representing many more groups. 

The following example shows a more complex setup. 
There are: 1 x raygen, 2 x miss, 2 x hit.
BUT the SBT will have 3 hit by duplicating the second hit in its table.
So, the same hit shader defined in the pipeline, can be called with different data.

In this case, the use must provide manually the information to the SBT. 
All extra group must be explicitly added. 

The following show how to get handle indices provided in the pipeline, and we are adding another hit group, re-using the 4th pipeline entry.
Note: we are not providing the pipelineCreateInfo, because we are manually defining it.

\code{.cpp}
// Manually defining group indices
m_sbtWrapper.addIndices(rayPipelineInfo); // Add raygen(0), miss(1), miss(2), hit(3), hit(4) from the pipeline info
m_sbtWrapper.addIndex(SBTWrapper::eHit, 4);  // Adding a 3rd hit, duplicate from the hit:1, which make hit:2 available.
m_sbtWrapper.addHitData(SBTWrapper::eHit, 2, m_hitShaderRecord[1]); // Adding data to this hit shader
m_sbtWrapper.create(m_rtPipeline);
\endcode

*/

#include <array>

#include "nvvk/resourceallocator_vk.hpp"
#include "nvvk/debug_util_vk.hpp"

namespace nvvk {
class SBTWrapper
{
public:
  enum GroupType
  {
    eRaygen,
    eMiss,
    eHit,
    eCallable
  };

  void setup(VkDevice                                               device,
             uint32_t                                               familyIndex,
             nvvk::ResourceAllocator*                               allocator,
             const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rtProperties);
  void destroy();

  // To call after the ray tracer pipeline creation
  // The rayPipelineInfo parameter is the structure used to define the pipeline,
  // while librariesInfo describe the potential input pipeline libraries
  void create(VkPipeline                                            rtPipeline,
              VkRayTracingPipelineCreateInfoKHR                     rayPipelineInfo = {},
              const std::vector<VkRayTracingPipelineCreateInfoKHR>& librariesInfo   = {});

  // Optional, to be used in combination with addIndex. Leave create() `rayPipelineInfo`
  // and 'librariesInfo' empty.  The rayPipelineInfo parameter is the structure used to
  // define the pipeline, while librariesInfo describe the potential input pipeline libraries
  void addIndices(VkRayTracingPipelineCreateInfoKHR                     rayPipelineInfo,
                  const std::vector<VkRayTracingPipelineCreateInfoKHR>& libraries = {});

  // Pushing back a GroupType and the handle pipeline index to use
  // i.e addIndex(eHit, 3) is pushing a Hit shader group using the 3rd entry in the pipeline
  void addIndex(GroupType t, uint32_t index) { m_index[t].push_back(index); }

  // Adding 'Shader Record' data to the group index.
  // i.e. addData(eHit, 0, myValue) is adding 'myValue' to the HIT group 0.
  template <typename T>
  void addData(GroupType t, uint32_t groupIndex, T& data)
  {
    addData(t, groupIndex, (uint8_t*)&data, sizeof(T));
  }

  void addData(GroupType t, uint32_t groupIndex, uint8_t* data, size_t dataSize)
  {
    std::vector<uint8_t> dst(data, data + dataSize);
    m_data[t][groupIndex] = dst;
  }

  // Getters
  uint32_t        indexCount(GroupType t) { return static_cast<uint32_t>(m_index[t].size()); }
  uint32_t        getStride(GroupType t) { return m_stride[t]; }
  VkDeviceAddress getAddress(GroupType t);

  // returns the entire size of a group. Raygen Stride and Size must be equal, even if the buffer contains many of them.
  uint32_t getSize(GroupType t) { return t == eRaygen ? getStride(eRaygen) : getStride(t) * indexCount(t); }

  // Return the address region of a group. indexOffset allow to offset the starting shader of the group.
  const VkStridedDeviceAddressRegionKHR getRegion(GroupType t, uint32_t indexOffset = 0);

  // Return the address regions of all groups. The offset allows to select which RayGen to use.
  const std::array<VkStridedDeviceAddressRegionKHR, 4> getRegions(uint32_t rayGenIndexOffset = 0);


private:
  using entry = std::unordered_map<uint32_t, std::vector<uint8_t>>;

  std::array<std::vector<uint32_t>, 4> m_index;               // Offset index in pipeline
  std::array<nvvk::Buffer, 4>          m_buffer;              // Buffer of handles + data
  std::array<uint32_t, 4>              m_stride{0, 0, 0, 0};  // Stride of each group
  std::array<entry, 4>                 m_data;                // Local data to groups (Shader Record)

  uint32_t m_handleSize{0};
  uint32_t m_handleAlignment{0};
  uint32_t m_shaderGroupBaseAlignment{0};

  VkDevice                 m_device{VK_NULL_HANDLE};
  nvvk::ResourceAllocator* m_pAlloc{nullptr};  // Allocator for buffer, images, acceleration structures
  nvvk::DebugUtil          m_debug;            // Utility to name objects
  uint32_t                 m_queueIndex{0};
};

}  // namespace nvvk
