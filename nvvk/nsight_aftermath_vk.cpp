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

#include "nsight_aftermath_vk.hpp"
#include "nvh/nvprint.hpp"
#include "nvp/perproject_globals.hpp"
#include "nvp/nvpsystem.hpp"

#ifdef NVVK_SUPPORTS_AFTERMATH
#include "GFSDK_Aftermath.h"
#include "GFSDK_Aftermath_GpuCrashDump.h"
#include "GFSDK_Aftermath_GpuCrashDumpDecoding.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include <cassert>
#include <filesystem>
#include <mutex>
#include <string>
#include <iomanip>
//--------------------------------------------------------------------------------------------------
// Some std::to_string overloads for some Nsight Aftermath API types.
//
namespace std {
template <typename T>
inline std::string to_hex_string(T n)
{
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(2 * sizeof(T)) << std::hex << n;
  return stream.str();
}

inline std::string to_string(GFSDK_Aftermath_Result result)
{
  return std::string("0x") + to_hex_string(static_cast<uint32_t>(result));
}

inline std::string to_string(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier)
{
  return to_hex_string(identifier.id[0]) + "-" + to_hex_string(identifier.id[1]);
}

inline std::string to_string(const GFSDK_Aftermath_ShaderHash& hash)
{
  return to_hex_string(hash.hash);
}

inline std::string to_string(const GFSDK_Aftermath_ShaderInstructionsHash& hash)
{
  return to_hex_string(hash.hash) + "-" + to_hex_string(hash.hash);
}
}  // namespace std

#define AFTERMATH_CHECK_ERROR(FC)                                                                                      \
  [&]() {                                                                                                              \
    GFSDK_Aftermath_Result _result = FC;                                                                               \
    assert(GFSDK_Aftermath_SUCCEED(_result));                                                                          \
  }()

namespace nvvk {

//*********************************************************
// GpuCrashTracker implementation
//*********************************************************

class GpuCrashTrackerImpl
{
public:
  GpuCrashTrackerImpl() = default;
  ~GpuCrashTrackerImpl();

  void Initialize();  // Initialize the GPU crash dump tracker.

private:
  // Callback handlers for GPU crash dumps and related data.
  void OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);
  void OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize);
  void OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription);
  void WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);
  void WriteShaderDebugInformationToFile(GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier,
                                         const void*                               pShaderDebugInfo,
                                         const uint32_t                            shaderDebugInfoSize);

  // Static callback wrappers.
  static void GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData);
  static void ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData);
  static void CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData);

  // GPU crash tracker state.
  bool               m_initialized{false};
  mutable std::mutex m_mutex{};  // For thread-safe access of GPU crash tracker state.
};


//*********************************************************
// GpuCrashTracker implementation
//*********************************************************

GpuCrashTrackerImpl::~GpuCrashTrackerImpl()
{
  if(m_initialized)
  {
    GFSDK_Aftermath_DisableGpuCrashDumps();
  }
}

// Initialize the GPU Crash Dump Tracker
void GpuCrashTrackerImpl::Initialize()
{
  // Enable GPU crash dumps and set up the callbacks for crash dump notifications,
  // shader debug information notifications, and providing additional crash
  // dump description data. Only the crash dump callback is mandatory. The other two
  // callbacks are optional and can be omitted, by passing nullptr, if the corresponding
  // functionality is not used.
  // The DeferDebugInfoCallbacks flag enables caching of shader debug information data
  // in memory. If the flag is set, ShaderDebugInfoCallback will be called only
  // in the event of a crash, right before GpuCrashDumpCallback. If the flag is not set,
  // ShaderDebugInfoCallback will be called for every shader that is compiled.
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_EnableGpuCrashDumps(
      GFSDK_Aftermath_Version_API, GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
      GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,  // Let the Nsight Aftermath library cache shader debug information.
      GpuCrashDumpCallback,                                              // Register callback for GPU crash dumps.
      ShaderDebugInfoCallback,       // Register callback for shader debug information.
      CrashDumpDescriptionCallback,  // Register callback for GPU crash dump description.
      this));                        // Set the GpuCrashTracker object as user data for the above callbacks.

  m_initialized = true;
}

// Handler for GPU crash dump callbacks from Nsight Aftermath
void GpuCrashTrackerImpl::OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
{
  // Make sure only one thread at a time...
  std::lock_guard<std::mutex> lock(m_mutex);

  // Write to file for later in-depth analysis with Nsight Graphics.
  WriteGpuCrashDumpToFile(pGpuCrashDump, gpuCrashDumpSize);
}

// Handler for shader debug information callbacks
void GpuCrashTrackerImpl::OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize)
{
  // Make sure only one thread at a time...
  std::lock_guard<std::mutex> lock(m_mutex);

  // Get shader debug information identifier
  GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(GFSDK_Aftermath_Version_API, pShaderDebugInfo,
                                                                     shaderDebugInfoSize, &identifier));

  // Write to file for later in-depth analysis of crash dumps with Nsight Graphics
  WriteShaderDebugInformationToFile(identifier, pShaderDebugInfo, shaderDebugInfoSize);
}

// Handler for GPU crash dump description callbacks
void GpuCrashTrackerImpl::OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription)
{
  // Add some basic description about the crash. This is called after the GPU crash happens, but before
  // the actual GPU crash dump callback. The provided data is included in the crash dump and can be
  // retrieved using GFSDK_Aftermath_GpuCrashDump_GetDescription().
  addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, getProjectName().c_str());
  addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "v1.0");
  addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined, "This is a GPU crash dump example.");
  addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined + 1, "Engine State: Rendering.");
  addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined + 2, "More user-defined information...");
}

// Helper for writing a GPU crash dump to a file
void GpuCrashTrackerImpl::WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
{
  // Create a GPU crash dump decoder object for the GPU crash dump.
  GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_CreateDecoder(GFSDK_Aftermath_Version_API, pGpuCrashDump,
                                                                   gpuCrashDumpSize, &decoder));

  // Use the decoder object to read basic information, like application
  // name, PID, etc. from the GPU crash dump.
  GFSDK_Aftermath_GpuCrashDump_BaseInfo baseInfo = {};
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &baseInfo));

  // Use the decoder object to query the application name that was set
  // in the GPU crash dump description.
  uint32_t applicationNameLength = 0;
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(decoder, GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
                                                                        &applicationNameLength));

  std::filesystem::path exePath(std::filesystem::path(NVPSystem::exePath()) / getProjectName());

  // Create a unique file name for writing the crash dump data to a file.
  // Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
  // driver release) we may see redundant crash dumps. As a workaround,
  // attach a unique count to each generated file name.
  static int        count = 0;
  const std::string baseFileName =
      std::string(exePath.string()) + "-" + std::to_string(baseInfo.pid) + "-" + std::to_string(++count);

  // Write the the crash dump data to a file using the .nv-gpudmp extension
  // registered with Nsight Graphics.
  const std::string crashDumpFileName = baseFileName + ".nv-gpudmp";
  std::ofstream     dumpFile(crashDumpFileName, std::ios::out | std::ios::binary);
  if(dumpFile)
  {
    dumpFile.write((const char*)pGpuCrashDump, gpuCrashDumpSize);
    dumpFile.close();
  }

  std::cerr << "\n\n**********\nDump file under: " << crashDumpFileName << "\n**********\n";

  // Decode the crash dump to a JSON string.
  // Step 1: Generate the JSON and get the size.
  uint32_t jsonSize = 0;
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
      decoder, GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO, GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
      nullptr /*ShaderDebugInfoLookupCallback*/, nullptr /*ShaderLookupCallback*/, nullptr,
      nullptr /*ShaderSourceDebugInfoLookupCallback*/, this, &jsonSize));

  // Step 2: Allocate a buffer and fetch the generated JSON.
  std::vector<char> json(jsonSize);
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetJSON(decoder, uint32_t(json.size()), json.data()));

  // Write the the crash dump data as JSON to a file.
  const std::string jsonFileName = crashDumpFileName + ".json";
  std::ofstream     jsonFile(jsonFileName, std::ios::out | std::ios::binary);
  if(jsonFile)
  {
    jsonFile.write(json.data(), json.size());
    jsonFile.close();
  }

  // Destroy the GPU crash dump decoder object.
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder));
}

// Helper for writing shader debug information to a file
void GpuCrashTrackerImpl::WriteShaderDebugInformationToFile(GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier,
                                                            const void*                               pShaderDebugInfo,
                                                            const uint32_t shaderDebugInfoSize)
{
  // Create a unique file name.
  const std::filesystem::path filePath =
      std::filesystem::path(NVPSystem::exePath()) / ("shader-" + std::to_string(identifier) + ".nvdbg");

  std::ofstream f(filePath, std::ios::out | std::ios::binary);
  if(f)
  {
    f.write((const char*)pShaderDebugInfo, shaderDebugInfoSize);
  }
}

// Static callback wrapper for OnCrashDump
void GpuCrashTrackerImpl::GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
{
  GpuCrashTrackerImpl* pGpuCrashTracker = reinterpret_cast<GpuCrashTrackerImpl*>(pUserData);
  pGpuCrashTracker->OnCrashDump(pGpuCrashDump, gpuCrashDumpSize);
}

// Static callback wrapper for OnShaderDebugInfo
void GpuCrashTrackerImpl::ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData)
{
  GpuCrashTrackerImpl* pGpuCrashTracker = reinterpret_cast<GpuCrashTrackerImpl*>(pUserData);
  pGpuCrashTracker->OnShaderDebugInfo(pShaderDebugInfo, shaderDebugInfoSize);
}

// Static callback wrapper for OnDescription
void GpuCrashTrackerImpl::CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData)
{
  GpuCrashTrackerImpl* pGpuCrashTracker = reinterpret_cast<GpuCrashTrackerImpl*>(pUserData);
  pGpuCrashTracker->OnDescription(addDescription);
}

GpuCrashTracker::GpuCrashTracker()
    : m_pimpl(new GpuCrashTrackerImpl())
{
}

GpuCrashTracker::~GpuCrashTracker()
{
  delete m_pimpl;
}

void GpuCrashTracker::Initialize()
{
  m_pimpl->Initialize();
}

}  // namespace nvvk

#else

namespace nvvk {

GpuCrashTracker::GpuCrashTracker()
    : m_pimpl(nullptr)
{
}

GpuCrashTracker::~GpuCrashTracker()
{

}

void GpuCrashTracker::Initialize()
{

}

}  // namespace nvvk

#endif

