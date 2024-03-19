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
#include <map>


/** @DOC_START

Capture the GPU load and memory for all GPUs on the system.

Usage:
- There should be only one instance of NvmlMonitor
- call refresh() in each frame. It will not pull more measurement that the interval(ms)
- isValid() : return if it can be used
- nbGpu()   : return the number of GPU in the computer
- getGpuInfo()     : static info about the GPU
- getDeviceMemory() : memory consumption info
- getDeviceUtilization() : GPU and memory utilization
- getDevicePerformanceState() : clock speeds and throttle reasons
- getDevicePowerState() : power, temperature and fan speed

Measurements: 
- Uses a cycle buffer. 
- Offset is the last measurement

@DOC_END */

namespace nvvkhl {

class NvmlMonitor
{
public:
  NvmlMonitor(uint32_t interval = 100, uint32_t limit = 100);
  ~NvmlMonitor();

  template <typename T>
  struct NVMLField
  {
    T    data;
    bool isSupported;

    operator T&() { return data; }
    T&       get() { return data; }
    const T& get() const { return data; }

    T& operator=(const T& rhs)
    {
      data = rhs;
      return data;
    }
  };

  // Static device information
  struct DeviceInfo
  {
    NVMLField<std::string> currentDriverModel;
    NVMLField<std::string> pendingDriverModel;

    NVMLField<uint32_t>    boardId;
    NVMLField<std::string> partNumber;
    NVMLField<std::string> brand;
    // Ordered list of bridge chips, each with a type and firmware version strings
    NVMLField<std::vector<std::pair<std::string, std::string>>> bridgeHierarchy;
    NVMLField<uint64_t>                                         cpuAffinity;
    NVMLField<std::string>                                      computeMode;
    NVMLField<int32_t>                                          computeCapabilityMajor;
    NVMLField<int32_t>                                          computeCapabilityMinor;
    NVMLField<uint32_t>                                         pcieLinkGen;
    NVMLField<uint32_t>                                         pcieLinkWidth;

    NVMLField<uint32_t> clockDefaultGraphics;
    NVMLField<uint32_t> clockDefaultSM;
    NVMLField<uint32_t> clockDefaultMem;
    NVMLField<uint32_t> clockDefaultVideo;

    NVMLField<uint32_t> clockMaxGraphics;
    NVMLField<uint32_t> clockMaxSM;
    NVMLField<uint32_t> clockMaxMem;
    NVMLField<uint32_t> clockMaxVideo;

    NVMLField<uint32_t> clockBoostGraphics;
    NVMLField<uint32_t> clockBoostSM;
    NVMLField<uint32_t> clockBoostMem;
    NVMLField<uint32_t> clockBoostVideo;


    NVMLField<bool> currentEccMode;
    NVMLField<bool> pendingEccMode;

    NVMLField<uint32_t>    encoderCapacityH264;
    NVMLField<uint32_t>    encoderCapacityHEVC;
    NVMLField<std::string> infoROMImageVersion;
    NVMLField<std::string> infoROMOEMVersion;
    NVMLField<std::string> infoROMECCVersion;
    NVMLField<std::string> infoROMPowerVersion;
    NVMLField<uint64_t>    supportedClocksThrottleReasons;
    NVMLField<std::string> vbiosVersion;
    NVMLField<uint32_t>    maxLinkGen;
    NVMLField<uint32_t>    maxLinkWidth;
    NVMLField<uint32_t>    minorNumber;
    NVMLField<uint32_t>    multiGpuBool;
    NVMLField<std::string> deviceName;


    NVMLField<uint32_t> tempThresholdShutdown;
    NVMLField<uint32_t> tempThresholdHWSlowdown;
    NVMLField<uint32_t> tempThresholdSWSlowdown;
    NVMLField<uint32_t> tempThresholdDropBelowBaseClock;

    NVMLField<uint32_t> powerLimit;

    NVMLField<std::vector<uint32_t>>                     supportedMemoryClocks;
    NVMLField<std::map<uint32_t, std::vector<uint32_t>>> supportedGraphicsClocks;


    void refresh(void* device);
  };

  // Device memory usage
  struct DeviceMemory
  {
    NVMLField<uint64_t>              bar1Total;
    NVMLField<std::vector<uint64_t>> bar1Used;
    NVMLField<std::vector<uint64_t>> bar1Free;

    NVMLField<uint64_t>              memoryTotal;
    NVMLField<std::vector<uint64_t>> memoryUsed;
    NVMLField<std::vector<uint64_t>> memoryFree;

    void init(uint32_t maxElements);
    void refresh(void* device, uint32_t offset);
  };

  // Device utilization ratios
  struct DeviceUtilization
  {
    NVMLField<std::vector<uint32_t>> gpuUtilization;
    NVMLField<std::vector<uint32_t>> memUtilization;
    NVMLField<std::vector<uint32_t>> computeProcesses;
    NVMLField<std::vector<uint32_t>> graphicsProcesses;

    void init(uint32_t maxElements);
    void refresh(void* device, uint32_t offset);
  };

  // Device performance state: clocks and throttling
  struct DevicePerformanceState
  {
    NVMLField<std::vector<uint32_t>> clockGraphics;
    NVMLField<std::vector<uint32_t>> clockSM;
    NVMLField<std::vector<uint32_t>> clockMem;
    NVMLField<std::vector<uint32_t>> clockVideo;
    NVMLField<std::vector<uint64_t>> throttleReasons;

    void                            init(uint32_t maxElements);
    void                            refresh(void* device, uint32_t offset);
    static std::vector<std::string> getThrottleReasonStrings(uint64_t reason);

    static const std::vector<uint64_t>& getAllThrottleReasonList();
  };

  // Device power and temperature
  struct DevicePowerState
  {
    NVMLField<std::vector<uint32_t>> power;
    NVMLField<std::vector<uint32_t>> temperature;
    NVMLField<std::vector<uint32_t>> fanSpeed;

    void init(uint32_t maxElements);
    void refresh(void* device, uint32_t offset);
  };

  // Other information
  struct SysInfo
  {
    std::vector<float> cpu;  // Load measurement [0, 100]
    std::string        driverVersion;
  };


  void                          refresh();  // Take measurement
  bool                          isValid() { return m_valid; }
  uint32_t                      getGpuCount() { return m_physicalGpuCount; }
  const DeviceInfo&             getDeviceInfo(int gpu) { return m_deviceInfo[gpu]; }
  const DeviceMemory&           getDeviceMemory(int gpu) { return m_deviceMemory[gpu]; }
  const DeviceUtilization&      getDeviceUtilization(int gpu) { return m_deviceUtilization[gpu]; }
  const DevicePerformanceState& getDevicePerformanceState(int gpu) { return m_devicePerformanceState[gpu]; }
  const DevicePowerState&       getDevicePowerState(int gpu) { return m_devicePowerState[gpu]; }
  const SysInfo&                getSysInfo() { return m_sysInfo; }
  int                           getOffset() { return m_offset; }


private:
  std::vector<DeviceInfo>             m_deviceInfo;
  std::vector<DeviceMemory>           m_deviceMemory;
  std::vector<DeviceUtilization>      m_deviceUtilization;
  std::vector<DevicePerformanceState> m_devicePerformanceState;
  std::vector<DevicePowerState>       m_devicePowerState;
  SysInfo                             m_sysInfo;  // CPU and driver information
  bool                                m_valid            = false;
  uint32_t                            m_physicalGpuCount = 0;    // Number of NVIDIA GPU
  uint32_t                            m_offset           = 0;    // Index of the most recent cpu load sample
  uint32_t                            m_maxElements      = 100;  // Number of max stored measurements
  uint32_t                            m_minInterval      = 100;  // Minimum interval lapse
};

}  // namespace nvvkhl
