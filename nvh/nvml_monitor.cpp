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


#ifdef WIN32
#include <windows.h>
#endif

#include "nvh/nvml_monitor.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#if defined(NVP_SUPPORTS_NVML)
#include <nvml.h>
#ifdef _WIN32
// The cfgmgr32 header is necessary for interrogating driver information in the registry.
#include <cfgmgr32.h>
// For convenience the library is also linked in automatically using the #pragma command.
#pragma comment(lib, "Cfgmgr32.lib")
#endif
#endif

//-------------------------------------------------------------------------------------------------
//
//
nvvkhl::NvmlMonitor::NvmlMonitor(uint32_t interval /*= 100*/, uint32_t limit /*= 100*/)
    : m_maxElements(limit)     // limit : number of measures
    , m_minInterval(interval)  // interval : ms between sampling
{
#if defined(NVP_SUPPORTS_NVML)

  nvmlReturn_t result;
  result = nvmlInit();
  if(result != NVML_SUCCESS)
    return;
  if(nvmlDeviceGetCount(&m_physicalGpuCount) != NVML_SUCCESS)
    return;
  m_gpuMeasure.resize(m_physicalGpuCount);
  m_gpuInfo.resize(m_physicalGpuCount);

  // System Info
  m_sysInfo.cpu.resize(m_maxElements);

  // Get driver version
  char driverVersion[80];
  result = nvmlSystemGetDriverVersion(driverVersion, 80);
  if(result == NVML_SUCCESS)
    m_sysInfo.driverVersion = driverVersion;

  // Loop over all GPUs
  for(int i = 0; i < (int)m_physicalGpuCount; i++)
  {
    // Sizing the data
    m_gpuMeasure[i].memory.resize(m_maxElements);
    m_gpuMeasure[i].load.resize(m_maxElements);

    // Retrieving general capabilities
    nvmlDevice_t device;
    nvmlMemory_t memory;

    // Find the memory of each cards
    result = nvmlDeviceGetHandleByIndex(i, &device);
    if(NVML_SUCCESS != result)
      return;
    nvmlDeviceGetMemoryInfo(device, &memory);
    m_gpuInfo[i].maxMem = memory.total;

    // name
    char name[96];
    result = nvmlDeviceGetName(device, name, 96);
    if(NVML_SUCCESS == result)
      m_gpuInfo[i].name = name;

#ifdef _WIN32
    // Find the model: TCC or WDDM
    nvmlDriverModel_t currentDriverModel;
    nvmlDriverModel_t pendingDriverModel;
    result = nvmlDeviceGetDriverModel(device, &currentDriverModel, &pendingDriverModel);
    if(NVML_SUCCESS != result)
      return;
    m_gpuInfo[i].driverModel = currentDriverModel;
#endif
  }
  m_valid = true;
#endif
}

//-------------------------------------------------------------------------------------------------
// Destructor: shutting down NVML
//
nvvkhl::NvmlMonitor::~NvmlMonitor()
{
#if defined(NVP_SUPPORTS_NVML)
  nvmlShutdown();
#endif
}

#if defined(NVP_SUPPORTS_NVML)

//-------------------------------------------------------------------------------------------------
// Returning the current amount of memory is used by the device
static uint64_t getMemory(nvmlDevice_t device)
{
  try
  {
    nvmlMemory_t memory{};
    nvmlDeviceGetMemoryInfo(device, &memory);
    return memory.used;
  }
  catch(std::exception ex)
  {
    return 0ULL;
  }
}

static float getLoad(nvmlDevice_t device)
{
  nvmlUtilization_t utilization{};
  nvmlReturn_t      result = nvmlDeviceGetUtilizationRates(device, &utilization);
  if(result != NVML_SUCCESS)
    return 0.0f;
  return static_cast<float>(utilization.gpu);
}


static float getCpuLoad()
{
#ifdef _WIN32
  static uint64_t s_previousTotalTicks = 0;
  static uint64_t s_previousIdleTicks  = 0;

  FILETIME idleTime, kernelTime, userTime;
  if(!GetSystemTimes(&idleTime, &kernelTime, &userTime))
    return 0.0f;

  auto fileTimeToInt64 = [](const FILETIME& ft) {
    return (((uint64_t)(ft.dwHighDateTime)) << 32) | ((uint64_t)ft.dwLowDateTime);
  };

  auto totalTicks = fileTimeToInt64(kernelTime) + fileTimeToInt64(userTime);
  auto idleTicks  = fileTimeToInt64(idleTime);

  uint64_t totalTicksSinceLastTime = totalTicks - s_previousTotalTicks;
  uint64_t idleTicksSinceLastTime  = idleTicks - s_previousIdleTicks;

  float result = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

  s_previousTotalTicks = totalTicks;
  s_previousIdleTicks  = idleTicks;

  return result * 100.f;
#else
  return 0;
#endif
}

#endif

//-------------------------------------------------------------------------------------------------
// Pulling the information from NVML and storing the data
// Note: the interval is important, as it cannot be query too quickly
//
void nvvkhl::NvmlMonitor::refresh()
{
#if defined(NVP_SUPPORTS_NVML)

  static std::chrono::high_resolution_clock::time_point s_startTime;

  if(!m_valid)
    return;

  // Pulling the information only when it is over the defined interval
  const auto now = std::chrono::high_resolution_clock::now();
  const auto t   = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_startTime).count();
  if(t < m_minInterval)
    return;
  s_startTime = now;

  // Increasing where to store the value
  m_offset = (m_offset + 1) % m_maxElements;

  // System
  m_sysInfo.cpu[m_offset] = getCpuLoad();

  // All GPUs
  for(unsigned int gpu_id = 0; gpu_id < m_physicalGpuCount; gpu_id++)
  {
    nvmlDevice_t device;
    nvmlReturn_t result = nvmlDeviceGetHandleByIndex(gpu_id, &device);
    if(result == NVML_SUCCESS)
    {
      m_gpuMeasure[gpu_id].memory[m_offset] = getMemory(device);
      m_gpuMeasure[gpu_id].load[m_offset]   = getLoad(device);
    }
  }

#endif  //  NVP_SUPPORTS_NVML
}
