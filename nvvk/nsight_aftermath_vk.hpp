/*
 * Copyright (c) 2021-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2021-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <vector>
#include <stdint.h>

namespace nvvk {
//// @DOC_SKIP
class GpuCrashTracker
{
public:
  void initialize();  // Initialize the GPU crash dump tracker.

  void addShaderBinary(const std::vector<uint32_t>& data);

  // Track an optimized shader with additional debug information
  void addShaderBinaryWithDebugInfo(const std::vector<uint32_t>& data, const std::vector<uint32_t>& strippedData);

  static GpuCrashTracker& getInstance();

private:
  GpuCrashTracker();
  ~GpuCrashTracker();

  class GpuCrashTrackerImpl* m_pimpl;
};

}  //namespace nvvk
