/*
 * SPDX-FileCopyrightText: Copyright (c) 2021-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#pragma once

#include <string>
#include <vector>

/** 

Capture the GPU load and memory for all GPUs on the system.

Usage:
- There should be only one instance of NvmlMonitor
- call refresh() in each frame. It will not pull more measurement that the interval(ms)
- isValid() : return if it can be used
- nbGpu()   : return the number of GPU in the computer
- getMeasures() :  return the measurements for a GPU
- getGpuInfo()     : return the info about the GPU

Measurements: 
- Uses a cycle buffer. 
- Offset is the last measurement

*/

namespace nvvkhl {

class NvmlMonitor
{
public:
  NvmlMonitor(uint32_t interval = 100, uint32_t limit = 100);
  ~NvmlMonitor();

  // Per GPU memory usage and load (utilization)
  struct GpuMeasure
  {
    std::vector<unsigned long long> memory;  // Memory
    std::vector<float>              load;    // Load measurement [0, 100]
  };

  // Information per GPU
  struct GpuInfo
  {
    unsigned long long maxMem{0};  // Max memory for each GPU in bytes
#ifdef _WIN32
    uint32_t driverModel{0};  // Driver model: WDDM/TCC
#endif
    std::string name;
  };

  // Other information
  struct SysInfo
  {
    std::vector<float> cpu;  // Load measurement [0, 100]
    std::string        driverVersion;
  };


  void              refresh();  // Take measurement
  bool              isValid() { return m_valid; }
  uint32_t          getGpuCount() { return m_physicalGpuCount; }
  const GpuMeasure& getMeasures(int gpu) { return m_gpuMeasure[gpu]; }
  const GpuInfo&    getGpuInfo(int gpu) { return m_gpuInfo[gpu]; }
  const SysInfo&    getSysInfo() { return m_sysInfo; }
  int               getOffset() { return m_offset; }


private:
  std::vector<GpuMeasure> m_gpuMeasure;  // Measure pulled for each GPU
  std::vector<GpuInfo>    m_gpuInfo;     // Various info per GPU
  SysInfo                 m_sysInfo;     // CPU and driver information
  bool                    m_valid            = false;
  uint32_t                m_physicalGpuCount = 0;    // Number of NVIDIA GPU
  uint32_t                m_offset           = 0;    // Index of the most recent cpu load sample
  uint32_t                m_maxElements      = 100;  // Number of max stored measurements
  uint32_t                m_minInterval      = 100;  // Minimum interval lapse
};

}  // namespace nvvkhl
