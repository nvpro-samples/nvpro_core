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
#define NVML_NO_UNVERSIONED_FUNC_DEFS
#include <nvml.h>
#ifdef _WIN32
// The cfgmgr32 header is necessary for interrogating driver information in the registry.
#include <cfgmgr32.h>
// For convenience the library is also linked in automatically using the #pragma command.
#pragma comment(lib, "Cfgmgr32.lib")
#endif


#define CHECK_NVML_CALL()                                                                                              \
  if(res != NVML_SUCCESS)                                                                                              \
  {                                                                                                                    \
    LOGE("NVML Error %s\n", nvmlErrorString(res));                                                                     \
  }

#define CHECK_NVML(fun)                                                                                                \
  {                                                                                                                    \
    nvmlReturn_t res = fun;                                                                                            \
    if(res != NVML_SUCCESS)                                                                                            \
    {                                                                                                                  \
      LOGE("NVML Error in %s: %s\n", #fun, nvmlErrorString(res));                                                      \
    }                                                                                                                  \
  }

#define CHECK_NVML_SUPPORT(fun, field)                                                                                 \
  {                                                                                                                    \
    nvmlReturn_t res = fun;                                                                                            \
    if(res != NVML_SUCCESS)                                                                                            \
    {                                                                                                                  \
      field.isSupported = false;                                                                                       \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      field.isSupported = true;                                                                                        \
    }                                                                                                                  \
  }


static const std::string brandToString(nvmlBrandType_t brand)
{
  switch(brand)
  {
    case NVML_BRAND_UNKNOWN:
      return "Unknown";

    case NVML_BRAND_QUADRO:
      return "Quadro";
    case NVML_BRAND_TESLA:
      return "Tesla";
    case NVML_BRAND_NVS:
      return "NVS";
    case NVML_BRAND_GRID:
      return "Grid";
    case NVML_BRAND_GEFORCE:
      return "GeForce";
    case NVML_BRAND_TITAN:
      return "Titan";
    case NVML_BRAND_NVIDIA_VAPPS:
      return "NVIDIA Virtual Applications";

    case NVML_BRAND_NVIDIA_VPC:
      return "NVIDIA Virtual PC";
    case NVML_BRAND_NVIDIA_VCS:
      return "NVIDIA Virtual Compute Server";
    case NVML_BRAND_NVIDIA_VWS:
      return "NVIDIA RTX Virtual Workstation";
    case NVML_BRAND_NVIDIA_CLOUD_GAMING:
      return "NVIDIA Cloud Gaming";
    case NVML_BRAND_QUADRO_RTX:
      return "Quadro RTX";
    case NVML_BRAND_NVIDIA_RTX:
      return "NVIDIA RTX";
    case NVML_BRAND_NVIDIA:
      return "NVIDIA";
    case NVML_BRAND_GEFORCE_RTX:
      return "GeForce RTX";
    case NVML_BRAND_TITAN_RTX:
      return "Titan RTX";
  }
  return "Unknown";
}

static const std::string computeModeToString(nvmlComputeMode_t computeMode)
{
  switch(computeMode)
  {
    case NVML_COMPUTEMODE_DEFAULT:
      return "Default";
    case NVML_COMPUTEMODE_EXCLUSIVE_THREAD:
      return "Exclusive thread";
    case NVML_COMPUTEMODE_PROHIBITED:
      return "Compute prohibited";
    case NVML_COMPUTEMODE_EXCLUSIVE_PROCESS:
      return "Exclusive process";
    default:
      return "Unknown";
  }
}
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

  m_deviceInfo.resize(m_physicalGpuCount);
  m_deviceMemory.resize(m_physicalGpuCount);
  m_deviceUtilization.resize(m_physicalGpuCount);
  m_devicePerformanceState.resize(m_physicalGpuCount);
  m_devicePowerState.resize(m_physicalGpuCount);

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
    m_deviceMemory[i].init(m_maxElements);
    m_deviceUtilization[i].init(m_maxElements);
    m_devicePerformanceState[i].init(m_maxElements);
    m_devicePowerState[i].init(m_maxElements);

    // Retrieving general capabilities
    nvmlDevice_t device;

    result = nvmlDeviceGetHandleByIndex(i, &device);
    m_deviceInfo[i].refresh(device);
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

    m_deviceMemory[gpu_id].refresh(device, m_offset);
    m_deviceUtilization[gpu_id].refresh(device, m_offset);
    m_devicePerformanceState[gpu_id].refresh(device, m_offset);
    m_devicePowerState[gpu_id].refresh(device, m_offset);
  }

#endif  //  NVP_SUPPORTS_NVML
}

void nvvkhl::NvmlMonitor::DeviceInfo::refresh(void* dev)
{
#if defined(NVP_SUPPORTS_NVML)
  nvmlDevice_t device = reinterpret_cast<nvmlDevice_t>(dev);


  CHECK_NVML_SUPPORT(nvmlDeviceGetBoardId(device, &boardId.get()), boardId);

  partNumber.get().resize(NVML_DEVICE_PART_NUMBER_BUFFER_SIZE);
  CHECK_NVML_SUPPORT(
      nvmlDeviceGetBoardPartNumber(device, partNumber.get().data(), static_cast<uint32_t>(partNumber.get().size())), partNumber);

  nvmlBrandType_t brandType;
  CHECK_NVML_SUPPORT(nvmlDeviceGetBrand(device, &brandType), brand);
  brand.get() = brandToString(brandType);

  nvmlBridgeChipHierarchy_t bridgeChipHierarchy{};
  CHECK_NVML_SUPPORT(nvmlDeviceGetBridgeChipInfo(device, &bridgeChipHierarchy), bridgeHierarchy);
  bridgeHierarchy.get().resize(bridgeChipHierarchy.bridgeCount);
  for(int i = 0; i < bridgeChipHierarchy.bridgeCount; i++)
  {
    bridgeHierarchy.get()[i].first = ((bridgeChipHierarchy.bridgeChipInfo[i].type == NVML_BRIDGE_CHIP_PLX) ? "PLX" : "BRO4");
    bridgeHierarchy.get()[i].second = fmt::format("#{}", bridgeChipHierarchy.bridgeChipInfo[i].fwVersion);
  }


  CHECK_NVML_SUPPORT(nvmlDeviceGetCpuAffinity(device, 1, (unsigned long*)&cpuAffinity.get()), cpuAffinity);

  nvmlComputeMode_t cMode;
  CHECK_NVML_SUPPORT(nvmlDeviceGetComputeMode(device, &cMode), computeMode);
  computeMode = computeModeToString(cMode);

  CHECK_NVML_SUPPORT(nvmlDeviceGetCudaComputeCapability(device, &computeCapabilityMajor.get(), &computeCapabilityMinor.get()),
                     computeCapabilityMajor);
  computeCapabilityMinor.isSupported = computeCapabilityMajor.isSupported;

  CHECK_NVML_SUPPORT(nvmlDeviceGetCurrPcieLinkGeneration(device, &pcieLinkGen.get()), pcieLinkGen);
  CHECK_NVML_SUPPORT(nvmlDeviceGetCurrPcieLinkWidth(device, &pcieLinkWidth.get()), pcieLinkWidth);

  CHECK_NVML_SUPPORT(nvmlDeviceGetDefaultApplicationsClock(device, NVML_CLOCK_GRAPHICS, &clockDefaultGraphics.get()),
                     clockDefaultGraphics);
  CHECK_NVML_SUPPORT(nvmlDeviceGetMaxClockInfo(device, NVML_CLOCK_GRAPHICS, &clockMaxGraphics.get()), clockMaxGraphics);
  CHECK_NVML_SUPPORT(nvmlDeviceGetMaxCustomerBoostClock(device, NVML_CLOCK_GRAPHICS, &clockBoostGraphics.get()), clockBoostGraphics);


  CHECK_NVML_SUPPORT(nvmlDeviceGetDefaultApplicationsClock(device, NVML_CLOCK_SM, &clockDefaultSM.get()), clockDefaultSM);
  CHECK_NVML_SUPPORT(nvmlDeviceGetMaxClockInfo(device, NVML_CLOCK_SM, &clockMaxSM.get()), clockMaxSM);
  CHECK_NVML_SUPPORT(nvmlDeviceGetMaxCustomerBoostClock(device, NVML_CLOCK_SM, &clockBoostSM.get()), clockBoostSM);

  CHECK_NVML_SUPPORT(nvmlDeviceGetDefaultApplicationsClock(device, NVML_CLOCK_MEM, &clockDefaultMem.get()), clockDefaultMem);
  CHECK_NVML_SUPPORT(nvmlDeviceGetMaxClockInfo(device, NVML_CLOCK_MEM, &clockMaxMem.get()), clockMaxMem);
  CHECK_NVML_SUPPORT(nvmlDeviceGetMaxCustomerBoostClock(device, NVML_CLOCK_MEM, &clockBoostMem.get()), clockBoostMem);

  CHECK_NVML_SUPPORT(nvmlDeviceGetDefaultApplicationsClock(device, NVML_CLOCK_VIDEO, &clockDefaultVideo.get()), clockDefaultVideo);
  CHECK_NVML_SUPPORT(nvmlDeviceGetMaxClockInfo(device, NVML_CLOCK_VIDEO, &clockMaxVideo.get()), clockMaxVideo);
  CHECK_NVML_SUPPORT(nvmlDeviceGetMaxCustomerBoostClock(device, NVML_CLOCK_VIDEO, &clockBoostVideo.get()), clockBoostVideo);

#ifdef _WIN32
  nvmlDriverModel_t currentDM, pendingDM;
  CHECK_NVML_SUPPORT(nvmlDeviceGetDriverModel(device, &currentDM, &pendingDM), currentDriverModel);
  currentDriverModel             = (currentDM == NVML_DRIVER_WDDM) ? "WDDM" : "TCC";
  pendingDriverModel             = (pendingDM == NVML_DRIVER_WDDM) ? "WDDM" : "TCC";
  pendingDriverModel.isSupported = currentDriverModel.isSupported;
#endif

  nvmlEnableState_t currentES, pendingES;
  CHECK_NVML_SUPPORT(nvmlDeviceGetEccMode(device, &currentES, &pendingES), currentEccMode);
  currentEccMode             = (currentES == NVML_FEATURE_ENABLED);
  pendingEccMode             = (pendingES == NVML_FEATURE_ENABLED);
  pendingEccMode.isSupported = currentEccMode.isSupported;

  CHECK_NVML_SUPPORT(nvmlDeviceGetEncoderCapacity(device, NVML_ENCODER_QUERY_H264, &encoderCapacityH264.get()), encoderCapacityH264);
  CHECK_NVML_SUPPORT(nvmlDeviceGetEncoderCapacity(device, NVML_ENCODER_QUERY_HEVC, &encoderCapacityHEVC.get()), encoderCapacityHEVC);

  infoROMImageVersion.get().resize(NVML_DEVICE_INFOROM_VERSION_BUFFER_SIZE);
  CHECK_NVML_SUPPORT(nvmlDeviceGetInforomImageVersion(device, infoROMImageVersion.get().data(),
                                                      static_cast<uint32_t>(infoROMImageVersion.get().size())),
                     infoROMImageVersion);

  infoROMOEMVersion.get().resize(NVML_DEVICE_INFOROM_VERSION_BUFFER_SIZE);
  infoROMECCVersion.get().resize(NVML_DEVICE_INFOROM_VERSION_BUFFER_SIZE);
  infoROMPowerVersion.get().resize(NVML_DEVICE_INFOROM_VERSION_BUFFER_SIZE);
  CHECK_NVML_SUPPORT(nvmlDeviceGetInforomVersion(device, NVML_INFOROM_OEM, infoROMOEMVersion.get().data(),
                                                 static_cast<uint32_t>(infoROMOEMVersion.get().size())),
                     infoROMOEMVersion);
  CHECK_NVML_SUPPORT(nvmlDeviceGetInforomVersion(device, NVML_INFOROM_ECC, infoROMECCVersion.get().data(),
                                                 static_cast<uint32_t>(infoROMECCVersion.get().size())),
                     infoROMECCVersion);
  CHECK_NVML_SUPPORT(nvmlDeviceGetInforomVersion(device, NVML_INFOROM_POWER, infoROMPowerVersion.get().data(),
                                                 static_cast<uint32_t>(infoROMPowerVersion.get().size())),
                     infoROMPowerVersion);

  CHECK_NVML_SUPPORT(nvmlDeviceGetMaxPcieLinkGeneration(device, &maxLinkGen.get()), maxLinkGen);
  CHECK_NVML_SUPPORT(nvmlDeviceGetMaxPcieLinkWidth(device, &maxLinkWidth.get()), maxLinkWidth);
  CHECK_NVML_SUPPORT(nvmlDeviceGetMinorNumber(device, &minorNumber.get()), minorNumber);
  CHECK_NVML_SUPPORT(nvmlDeviceGetMultiGpuBoard(device, &multiGpuBool.get()), multiGpuBool);
  deviceName.get().resize(NVML_DEVICE_NAME_V2_BUFFER_SIZE);
  CHECK_NVML_SUPPORT(nvmlDeviceGetName(device, deviceName.get().data(), static_cast<uint32_t>(deviceName.get().size())), deviceName);

  CHECK_NVML_SUPPORT(nvmlDeviceGetSupportedClocksThrottleReasons(device, reinterpret_cast<long long unsigned int*>(
                                                                             &supportedClocksThrottleReasons.get())),
                     supportedClocksThrottleReasons);

  vbiosVersion.get().resize(NVML_DEVICE_VBIOS_VERSION_BUFFER_SIZE);
  CHECK_NVML_SUPPORT(
      nvmlDeviceGetVbiosVersion(device, vbiosVersion.get().data(), static_cast<uint32_t>(vbiosVersion.get().size())), vbiosVersion);

  CHECK_NVML_SUPPORT(nvmlDeviceGetTemperatureThreshold(device, NVML_TEMPERATURE_THRESHOLD_SHUTDOWN,
                                                       &tempThresholdShutdown.get()),
                     tempThresholdShutdown);
  CHECK_NVML_SUPPORT(nvmlDeviceGetTemperatureThreshold(device, NVML_TEMPERATURE_THRESHOLD_SLOWDOWN,
                                                       &tempThresholdHWSlowdown.get()),
                     tempThresholdHWSlowdown);
  CHECK_NVML_SUPPORT(nvmlDeviceGetTemperatureThreshold(device, NVML_TEMPERATURE_THRESHOLD_MEM_MAX,
                                                       &tempThresholdSWSlowdown.get()),
                     tempThresholdSWSlowdown);
  CHECK_NVML_SUPPORT(nvmlDeviceGetTemperatureThreshold(device, NVML_TEMPERATURE_THRESHOLD_GPU_MAX,
                                                       &tempThresholdDropBelowBaseClock.get()),
                     tempThresholdDropBelowBaseClock);

  CHECK_NVML_SUPPORT(nvmlDeviceGetPowerManagementLimit(device, &powerLimit.get()), powerLimit);
  // Milliwatt to watt
  powerLimit.get() /= 1000;

  uint32_t supportedClockCount = 0;
  if(nvmlDeviceGetSupportedMemoryClocks(device, &supportedClockCount, nullptr) == NVML_ERROR_INSUFFICIENT_SIZE)
  {
    supportedMemoryClocks.isSupported = true;
    supportedMemoryClocks.get().resize(supportedClockCount);
    nvmlDeviceGetSupportedMemoryClocks(device, &supportedClockCount, supportedMemoryClocks.get().data());
  }

  for(size_t i = 0; i < supportedMemoryClocks.get().size(); i++)
  {
    supportedClockCount = 0;
    if(nvmlDeviceGetSupportedGraphicsClocks(device, supportedMemoryClocks.get()[i], &supportedClockCount, nullptr) == NVML_ERROR_INSUFFICIENT_SIZE)
    {
      supportedGraphicsClocks.isSupported = true;
      auto& graphicsClocks                = supportedGraphicsClocks.get()[supportedMemoryClocks.get()[i]];
      graphicsClocks.resize(supportedClockCount);
      nvmlDeviceGetSupportedGraphicsClocks(device, supportedMemoryClocks.get()[i], &supportedClockCount, graphicsClocks.data());
    }
  }
#endif
}

void nvvkhl::NvmlMonitor::DeviceMemory::init(uint32_t maxElements)
{
  memoryFree.get().resize(maxElements);
  memoryUsed.get().resize(maxElements);

  bar1Free.get().resize(maxElements);
  bar1Used.get().resize(maxElements);
}

void nvvkhl::NvmlMonitor::DeviceMemory::refresh(void* dev, uint32_t offset)
{
#if defined(NVP_SUPPORTS_NVML)
  nvmlDevice_t device = reinterpret_cast<nvmlDevice_t>(dev);

  nvmlBAR1Memory_t bar1Memory{};
  nvmlMemory_t     memory{};
  CHECK_NVML_SUPPORT(nvmlDeviceGetBAR1MemoryInfo(device, &bar1Memory), bar1Total);

  bar1Total              = bar1Memory.bar1Total;
  bar1Used.get()[offset] = bar1Memory.bar1Used;
  bar1Used.isSupported   = bar1Total.isSupported;

  bar1Free.get()[offset] = bar1Memory.bar1Free;
  bar1Free.isSupported   = bar1Total.isSupported;

  CHECK_NVML_SUPPORT(nvmlDeviceGetMemoryInfo(device, &memory), memoryTotal);
  memoryTotal              = memory.total;
  memoryUsed.get()[offset] = memory.used;
  memoryUsed.isSupported   = memoryTotal.isSupported;
  memoryFree.get()[offset] = memory.free;
  memoryFree.isSupported   = memoryTotal.isSupported;
#endif
}

void nvvkhl::NvmlMonitor::DeviceUtilization::init(uint32_t maxElements)
{
  gpuUtilization.get().resize(maxElements);
  memUtilization.get().resize(maxElements);
  ;
  computeProcesses.get().resize(maxElements);
  ;
  graphicsProcesses.get().resize(maxElements);
  ;
}

void nvvkhl::NvmlMonitor::DeviceUtilization::refresh(void* dev, uint32_t offset)
{
#if defined(NVP_SUPPORTS_NVML)
  nvmlDevice_t device = reinterpret_cast<nvmlDevice_t>(dev);

  nvmlUtilization_t utilization;
  CHECK_NVML_SUPPORT(nvmlDeviceGetUtilizationRates(device, &utilization), gpuUtilization);
  gpuUtilization.get()[offset] = utilization.gpu;
  memUtilization.get()[offset] = utilization.memory;
  memUtilization.isSupported   = gpuUtilization.isSupported;


  computeProcesses.get()[offset]  = 0;
  graphicsProcesses.get()[offset] = 0;
  CHECK_NVML_SUPPORT(nvmlDeviceGetComputeRunningProcesses(device, &computeProcesses.get()[offset], nullptr), computeProcesses);
  CHECK_NVML_SUPPORT(nvmlDeviceGetGraphicsRunningProcesses(device, &graphicsProcesses.get()[offset], nullptr), graphicsProcesses);
#endif
}

void nvvkhl::NvmlMonitor::DevicePerformanceState::init(uint32_t maxElements)
{
  clockGraphics.get().resize(maxElements);
  clockSM.get().resize(maxElements);
  clockMem.get().resize(maxElements);
  clockVideo.get().resize(maxElements);
  throttleReasons.get().resize(maxElements);
}

void nvvkhl::NvmlMonitor::DevicePerformanceState::refresh(void* dev, uint32_t offset)
{
#if defined(NVP_SUPPORTS_NVML)
  nvmlDevice_t device = reinterpret_cast<nvmlDevice_t>(dev);

  CHECK_NVML_SUPPORT(nvmlDeviceGetClockInfo(device, NVML_CLOCK_GRAPHICS, &clockGraphics.get()[offset]), clockGraphics);
  CHECK_NVML_SUPPORT(nvmlDeviceGetClockInfo(device, NVML_CLOCK_SM, &clockSM.get()[offset]), clockSM);
  CHECK_NVML_SUPPORT(nvmlDeviceGetClockInfo(device, NVML_CLOCK_MEM, &clockMem.get()[offset]), clockMem);
  CHECK_NVML_SUPPORT(nvmlDeviceGetClockInfo(device, NVML_CLOCK_VIDEO, &clockVideo.get()[offset]), clockVideo);

  CHECK_NVML_SUPPORT(nvmlDeviceGetCurrentClocksThrottleReasons(device, reinterpret_cast<unsigned long long*>(
                                                                           &throttleReasons.get()[offset])),
                     throttleReasons);
#endif
}

std::vector<std::string> nvvkhl::NvmlMonitor::DevicePerformanceState::getThrottleReasonStrings(uint64_t reason)
{
  std::vector<std::string> reasonStrings;
#if defined(NVP_SUPPORTS_NVML)


  if(reason & nvmlClocksThrottleReasonGpuIdle)
  {
    reasonStrings.push_back("Idle");
  }

  if(reason & nvmlClocksThrottleReasonApplicationsClocksSetting)
  {
    reasonStrings.push_back("App clock setting");
  }
  if(reason & nvmlClocksThrottleReasonSwPowerCap)
  {
    reasonStrings.push_back("SW power cap");
  }
  if(reason & nvmlClocksThrottleReasonHwSlowdown)
  {
    reasonStrings.push_back("HW slowdown");
  }
  if(reason & nvmlClocksThrottleReasonSyncBoost)
  {
    reasonStrings.push_back("Sync boost");
  }
  if(reason & nvmlClocksThrottleReasonSwThermalSlowdown)
  {
    reasonStrings.push_back("SW Thermal slowdown");
  }
  if(reason & nvmlClocksThrottleReasonHwThermalSlowdown)
  {
    reasonStrings.push_back("HW Thermal slowdown");
  }
  if(reason & nvmlClocksThrottleReasonHwPowerBrakeSlowdown)
  {
    reasonStrings.push_back("Power brake slowdown");
  }
  if(reasonStrings.empty())
  {
    reasonStrings.push_back("Full speed");
  }
#endif
  return reasonStrings;
}

const std::vector<uint64_t>& nvvkhl::NvmlMonitor::DevicePerformanceState::getAllThrottleReasonList()
{
  static std::vector<uint64_t> s_reasonList =
#if defined(NVP_SUPPORTS_NVML)
      {nvmlClocksThrottleReasonGpuIdle,
       nvmlClocksThrottleReasonApplicationsClocksSetting,
       nvmlClocksThrottleReasonSwPowerCap,
       nvmlClocksThrottleReasonHwSlowdown,
       nvmlClocksThrottleReasonSyncBoost,
       nvmlClocksThrottleReasonSwThermalSlowdown,
       nvmlClocksThrottleReasonHwThermalSlowdown,
       nvmlClocksThrottleReasonHwPowerBrakeSlowdown,
       nvmlClocksThrottleReasonNone};

#else
      {};
#endif
  return s_reasonList;
}

void nvvkhl::NvmlMonitor::DevicePowerState::init(uint32_t maxElements)
{
  power.get().resize(maxElements);
  temperature.get().resize(maxElements);
  fanSpeed.get().resize(maxElements);
}

void nvvkhl::NvmlMonitor::DevicePowerState::refresh(void* dev, uint32_t offset)
{
#if defined(NVP_SUPPORTS_NVML)
  nvmlDevice_t device = reinterpret_cast<nvmlDevice_t>(dev);

  CHECK_NVML_SUPPORT(nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temperature.get()[offset]), temperature);
  CHECK_NVML_SUPPORT(nvmlDeviceGetPowerUsage(device, &power.get()[offset]), power);
  // Milliwatt to watt
  power.get()[offset] /= 1000;
  CHECK_NVML_SUPPORT(nvmlDeviceGetFanSpeed(device, &fanSpeed.get()[offset]), fanSpeed);
#endif
}
