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
#include <vector>
#include "vulkan/vulkan_core.h"

namespace nvvk {

//////////////////////////////////////////////////////////////////////////
// Helper to generate specialization info
//
//  Examples:
//    nvvk::Specialization specialization;
//    specialization.add(0, 5); // Adding value 5 to constant_id=0
//    VkPipelineShaderStageCreateInfo info;
//    ...
//    info.pSpecializationInfo = specialization.getSpecialization();
//    createPipeline();
//
// Note: this is adding information in a vector, therefor add all values
//       before calling getSpecialization(). Construct the pipeline before
//       specialization get out of scope, or pointer getting invalidated
//       by adding new values or clearing the vector of data.
//
class Specialization
{
public:
  void add(uint32_t constantID, int32_t value)
  {
    m_specValues.push_back(value);
    VkSpecializationMapEntry entry;
    entry.constantID = constantID;
    entry.size       = sizeof(int32_t);
    entry.offset     = static_cast<uint32_t>(m_specEntries.size() * sizeof(int32_t));
    m_specEntries.emplace_back(entry);
  }

  void add(const std::vector<std::pair<uint32_t, int32_t>>& const_values)
  {
    for(const auto& v : const_values)
    {
      add(v.first, v.second);
    }
  }

  VkSpecializationInfo* getSpecialization()
  {
    m_specInfo.dataSize      = static_cast<uint32_t>(m_specValues.size() * sizeof(int32_t));
    m_specInfo.pData         = m_specValues.data();
    m_specInfo.mapEntryCount = static_cast<uint32_t>(m_specEntries.size());
    m_specInfo.pMapEntries   = m_specEntries.data();
    return &m_specInfo;
  }

  void clear()
  {
    m_specValues.clear();
    m_specEntries.clear();
    m_specInfo = {};
  }

private:
  std::vector<int32_t>                  m_specValues;
  std::vector<VkSpecializationMapEntry> m_specEntries;
  VkSpecializationInfo                  m_specInfo{};
};
}  // namespace nvvk
