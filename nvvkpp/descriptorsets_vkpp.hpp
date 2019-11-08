/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <vulkan/vulkan.hpp>

namespace nvvkpp {

namespace util {

//--------------------------------------------------------------------------------------------------
/** 
There is a utility to create the `DescriptorSet`, the `DescriptorPool` and `DescriptorSetLayout`. 
All the information to create those structures are included in the `DescriptorSetLayoutBinding`. 
Therefore, it is only necessary to fill the `DescriptorSetLayoutBinding` and let the utilities to 
fill the information.

For assigning the information to a descriptor set, multiple `WriteDescriptorSet` need to be filled. 
Using the `nvvk::util::createWrite` with the buffer, image or acceleration structure descriptor will 
return the correct `WriteDescriptorSet` to push back.

Example of usage:
~~~~ c++
m_descSetLayoutBind[eScene].emplace_back(vk::DescriptorSetLayoutBinding(0, vkDT::eUniformBuffer, 1, vkSS::eVertex | vkSS::eFragment));
m_descSetLayoutBind[eScene].emplace_back(vk::DescriptorSetLayoutBinding(1, vkDT::eStorageBufferDynamic, 1, vkSS::eVertex ));
m_descSetLayout[eScene] = nvvk::util::createDescriptorSetLayout(m_device, m_descSetLayoutBind[eScene]);
m_descPool[eScene]      = nvvk::util::createDescriptorPool(m_device, m_descSetLayoutBind[eScene], 1);
m_descSet[eScene]       = nvvk::util::createDescriptorSet(m_device, m_descPool[eScene], m_descSetLayout[eScene]);
...
std::vector<vk::WriteDescriptorSet> writes;
writes.emplace_back(nvvk::util::createWrite(m_descSet[eScene], m_descSetLayoutBind[eScene][0], &m_uniformBuffers.scene.descriptor));
writes.emplace_back(nvvk::util::createWrite(m_descSet[eScene], m_descSetLayoutBind[eScene][1], &m_uniformBuffers.matrices.descriptor));
m_device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
~~~~
*/

inline vk::DescriptorPool createDescriptorPool(vk::Device                                         device,
                                               const std::vector<vk::DescriptorSetLayoutBinding>& bindings,
                                               uint32_t                                           maxSets = 1)
{
  // Aggregate the bindings to obtain the required size of the descriptors using that layout
  std::vector<vk::DescriptorPoolSize> counters;
  counters.reserve(bindings.size());
  for(const auto& b : bindings)
  {
    counters.emplace_back(b.descriptorType, b.descriptorCount);
  }

  // Create the pool information descriptor, that contains the number of descriptors of each type
  vk::DescriptorPoolCreateInfo poolInfo = {};
  poolInfo.setPoolSizeCount(static_cast<uint32_t>(counters.size()));
  poolInfo.setPPoolSizes(counters.data());
  poolInfo.setMaxSets(maxSets);


  vk::DescriptorPool pool = device.createDescriptorPool(poolInfo);
  if(!pool)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
  return pool;
}

inline vk::DescriptorSetLayout createDescriptorSetLayout(vk::Device device, const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
  vk::DescriptorSetLayoutCreateInfo layoutInfo(vk::DescriptorSetLayoutCreateFlags(),
                                               static_cast<uint32_t>(bindings.size()), bindings.data());
  vk::DescriptorSetLayout           layout = device.createDescriptorSetLayout(layoutInfo);
  if(!layout)
  {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
  return layout;
}

inline vk::DescriptorSet createDescriptorSet(vk::Device device, vk::DescriptorPool pool, vk::DescriptorSetLayout layout)
{
  vk::DescriptorSetAllocateInfo allocInfo(pool, 1, &layout);
  vk::DescriptorSet             set = device.allocateDescriptorSets(allocInfo)[0];
  if(!set)
  {
    throw std::runtime_error("failed to allocate descriptor set!");
  }
  return set;
}

inline vk::WriteDescriptorSet createWrite(vk::DescriptorSet                     ds,
                                          const vk::DescriptorSetLayoutBinding& binding,
                                          const vk::DescriptorBufferInfo*       info,
                                          uint32_t                              arrayElement = 0)
{
  return {ds, binding.binding, arrayElement, binding.descriptorCount, binding.descriptorType, nullptr, info};
}

inline vk::WriteDescriptorSet createWrite(vk::DescriptorSet                     ds,
                                          const vk::DescriptorSetLayoutBinding& binding,
                                          const vk::DescriptorImageInfo*        info,
                                          uint32_t                              arrayElement = 0)
{
  return {ds, binding.binding, arrayElement, binding.descriptorCount, binding.descriptorType, info};
}

inline vk::WriteDescriptorSet createWrite(vk::DescriptorSet                                    ds,
                                          const vk::DescriptorSetLayoutBinding&                binding,
                                          const vk::WriteDescriptorSetAccelerationStructureNV* info,
                                          uint32_t                                             arrayElement = 0)
{
  vk::WriteDescriptorSet res(ds, binding.binding, arrayElement, binding.descriptorCount, binding.descriptorType);
  res.setPNext(info);
  return res;
}

}  // namespace util
}  // namespace nvvk