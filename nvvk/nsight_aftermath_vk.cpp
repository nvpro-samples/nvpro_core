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

#if defined(NVVK_SUPPORTS_AFTERMATH) && defined(NVP_SUPPORTS_VULKANSDK)
#include <vulkan/vulkan.h>  // needed so GFSDK_Aftermath_SpirvCode gets declared

#include "nvh/nvprint.hpp"
#include "nvp/perproject_globals.hpp"
#include "nvp/nvpsystem.hpp"

#include "GFSDK_Aftermath.h"
#include "GFSDK_Aftermath_GpuCrashDump.h"
#include "GFSDK_Aftermath_GpuCrashDumpDecoding.h"

#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
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

inline std::string to_string(const GFSDK_Aftermath_ShaderBinaryHash& hash)
{
  return to_hex_string(hash.hash);
}
}  // namespace std

//*********************************************************
// Helper for comparing shader hashes and debug info identifier.
//

// Helper for comparing GFSDK_Aftermath_ShaderDebugInfoIdentifier.
inline bool operator<(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& lhs, const GFSDK_Aftermath_ShaderDebugInfoIdentifier& rhs)
{
  if(lhs.id[0] == rhs.id[0])
  {
    return lhs.id[1] < rhs.id[1];
  }
  return lhs.id[0] < rhs.id[0];
}

// Helper for comparing GFSDK_Aftermath_ShaderBinaryHash.
inline bool operator<(const GFSDK_Aftermath_ShaderBinaryHash& lhs, const GFSDK_Aftermath_ShaderBinaryHash& rhs)
{
  return lhs.hash < rhs.hash;
}

// Helper for comparing GFSDK_Aftermath_ShaderDebugName.
inline bool operator<(const GFSDK_Aftermath_ShaderDebugName& lhs, const GFSDK_Aftermath_ShaderDebugName& rhs)
{
  return strncmp(lhs.name, rhs.name, sizeof(lhs.name)) < 0;
}

//*********************************************************
// Helper for checking Nsight Aftermath failures.
//

inline std::string AftermathErrorMessage(GFSDK_Aftermath_Result result)
{
  switch(result)
  {
    case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
      return "Unsupported driver version - requires an NVIDIA R495 display driver or newer.";
    default:
      return "Aftermath Error 0x" + std::to_hex_string(result);
  }
}


// Helper macro for checking Nsight Aftermath results and throwing exception
// in case of a failure.
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>

#define AFTERMATH_CHECK_ERROR(FC)                                                                                      \
  [&]() {                                                                                                              \
    GFSDK_Aftermath_Result _result = FC;                                                                               \
    if(!GFSDK_Aftermath_SUCCEED(_result))                                                                              \
    {                                                                                                                  \
      MessageBoxA(0, AftermathErrorMessage(_result).c_str(), "Aftermath Error", MB_OK);                                \
      exit(1);                                                                                                         \
    }                                                                                                                  \
  }()
#else
#define AFTERMATH_CHECK_ERROR(FC)                                                                                      \
  [&]() {                                                                                                              \
    GFSDK_Aftermath_Result _result = FC;                                                                               \
    if(!GFSDK_Aftermath_SUCCEED(_result))                                                                              \
    {                                                                                                                  \
      printf("%s\n", AftermathErrorMessage(_result).c_str());                                                          \
      fflush(stdout);                                                                                                  \
      exit(1);                                                                                                         \
    }                                                                                                                  \
  }()
#endif

namespace nvvk {

  //*********************************************************
// Implements GPU crash dump tracking using the Nsight
// Aftermath API.
//
class GpuCrashTrackerImpl
{
public:
  // keep four frames worth of marker history
  const static unsigned int                                                 c_markerFrameHistory = 4;
  typedef std::array<std::map<uint64_t, std::string>, c_markerFrameHistory> MarkerMap;

  GpuCrashTrackerImpl(const MarkerMap& markerMap);
  ~GpuCrashTrackerImpl();

  // Initialize the GPU crash dump tracker.
  void initialize();

  // Track a shader compiled with -g
  void addShaderBinary(std::vector<uint32_t>& data);

  // Track an optimized shader with additional debug information
  void addShaderBinaryWithDebugInfo(std::vector<uint32_t>& data, std::vector<uint32_t>& strippedData);


private:
  //*********************************************************
  // Callback handlers for GPU crash dumps and related data.
  //

  // Handler for GPU crash dump callbacks.
  void onCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);

  // Handler for shader debug information callbacks.
  void onShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize);

  // Handler for GPU crash dump description callbacks.
  static void onDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription);

  // Handler for app-managed marker resolve callback
  void onResolveMarker(const void* pMarker, void** resolvedMarkerData, uint32_t* markerSize);

  //*********************************************************
  // Helpers for writing a GPU crash dump and debug information
  // data to files.
  //

  // Helper for writing a GPU crash dump to a file.
  void writeGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);

  // Helper for writing shader debug information to a file
  static void writeShaderDebugInformationToFile(GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier,
                                                const void*                               pShaderDebugInfo,
                                                const uint32_t                            shaderDebugInfoSize);

  //*********************************************************
  // Helpers for decoding GPU crash dump to JSON.
  //

  // Handler for shader debug info lookup callbacks.
  void onShaderDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier,
                               PFN_GFSDK_Aftermath_SetData                      setShaderDebugInfo) const;

  // Handler for shader lookup callbacks.
  void onShaderLookup(const GFSDK_Aftermath_ShaderBinaryHash& shaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary) const;

  // Handler for shader source debug info lookup callbacks.
  void onShaderSourceDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugName& shaderDebugName,
                                     PFN_GFSDK_Aftermath_SetData            setShaderBinary) const;

  //*********************************************************
  // Static callback wrappers.
  //

  // GPU crash dump callback.
  static void gpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData);

  // Shader debug information callback.
  static void shaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData);

  // GPU crash dump description callback.
  static void crashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData);

  // App-managed marker resolve callback
  static void resolveMarkerCallback(const void* pMarker, void* pUserData, void** resolvedMarkerData, uint32_t* markerSize);

  // Shader debug information lookup callback.
  static void shaderDebugInfoLookupCallback(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier,
                                            PFN_GFSDK_Aftermath_SetData                      setShaderDebugInfo,
                                            void*                                            pUserData);

  // Shader lookup callback.
  static void shaderLookupCallback(const GFSDK_Aftermath_ShaderBinaryHash* pShaderHash,
                                   PFN_GFSDK_Aftermath_SetData             setShaderBinary,
                                   void*                                   pUserData);

  // Shader source debug info lookup callback.
  static void shaderSourceDebugInfoLookupCallback(const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName,
                                                  PFN_GFSDK_Aftermath_SetData            setShaderBinary,
                                                  void*                                  pUserData);

  //*********************************************************
  // GPU crash tracker state.
  //

  // Is the GPU crash dump tracker initialized?
  bool m_initialized;

  // For thread-safe access of GPU crash tracker state.
  mutable std::mutex m_mutex;

  // List of Shader Debug Information by ShaderDebugInfoIdentifier.
  std::map<GFSDK_Aftermath_ShaderDebugInfoIdentifier, std::vector<uint8_t>> m_shaderDebugInfo;

  // App-managed marker tracking
  const MarkerMap& m_markerMap;

  //*********************************************************
  // SAhder database .
  //

  // Find a shader bytecode binary by shader hash.
  bool findShaderBinary(const GFSDK_Aftermath_ShaderBinaryHash& shaderHash, std::vector<uint32_t>& shader) const;

  // Find a source shader debug info by shader debug name generated by the DXC compiler.
  bool findShaderBinaryWithDebugData(const GFSDK_Aftermath_ShaderDebugName& shaderDebugName, std::vector<uint32_t>& shader) const;


  // List of shader binaries by ShaderBinaryHash.
  std::map<GFSDK_Aftermath_ShaderBinaryHash, std::vector<uint32_t>> m_shaderBinaries;

  // List of available shader binaries with source debug information by ShaderDebugName.
  std::map<GFSDK_Aftermath_ShaderDebugName, std::vector<uint32_t>> m_shaderBinariesWithDebugInfo;
};



//*********************************************************
// GpuCrashTrackerImpl implementation
//*********************************************************

GpuCrashTrackerImpl::GpuCrashTrackerImpl(const MarkerMap& markerMap)
    : m_initialized(false)
    , m_markerMap(markerMap)
{
}

GpuCrashTrackerImpl::~GpuCrashTrackerImpl()
{
  // If initialized, disable GPU crash dumps
  if(m_initialized)
  {
    GFSDK_Aftermath_DisableGpuCrashDumps();
  }
}

// Initialize the GPU Crash Dump Tracker
void GpuCrashTrackerImpl::initialize()
{
  // Enable GPU crash dumps and set up the callbacks for crash dump notifications,
  // shader debug information notifications, and providing additional crash
  // dump description data.Only the crash dump callback is mandatory. The other two
  // callbacks are optional and can be omitted, by passing nullptr, if the corresponding
  // functionality is not used.
  // The DeferDebugInfoCallbacks flag enables caching of shader debug information data
  // in memory. If the flag is set, ShaderDebugInfoCallback will be called only
  // in the event of a crash, right before GpuCrashDumpCallback. If the flag is not set,
  // ShaderDebugInfoCallback will be called for every shader that is compiled.
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_EnableGpuCrashDumps(
      GFSDK_Aftermath_Version_API, GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
      GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,  // Let the Nsight Aftermath library cache shader debug information.
      gpuCrashDumpCallback,                                              // Register callback for GPU crash dumps.
      shaderDebugInfoCallback,       // Register callback for shader debug information.
      crashDumpDescriptionCallback,  // Register callback for GPU crash dump description.
      resolveMarkerCallback,         // Register callback for resolving application-managed markers.
      this));                        // Set the GpuCrashTrackerImpl object as user data for the above callbacks.

  m_initialized = true;
}

// Handler for GPU crash dump callbacks from Nsight Aftermath
void GpuCrashTrackerImpl::onCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
{
  // Make sure only one thread at a time...
  std::lock_guard<std::mutex> lock(m_mutex);

  // Write to file for later in-depth analysis with Nsight Graphics.
  writeGpuCrashDumpToFile(pGpuCrashDump, gpuCrashDumpSize);
}

// Handler for shader debug information callbacks
void GpuCrashTrackerImpl::onShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize)
{
  // Make sure only one thread at a time...
  std::lock_guard<std::mutex> lock(m_mutex);

  // Get shader debug information identifier
  GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(GFSDK_Aftermath_Version_API, pShaderDebugInfo,
                                                                     shaderDebugInfoSize, &identifier));

  // Store information for decoding of GPU crash dumps with shader address mapping
  // from within the application.
  std::vector<uint8_t> data((uint8_t*)pShaderDebugInfo, (uint8_t*)pShaderDebugInfo + shaderDebugInfoSize);
  m_shaderDebugInfo[identifier].swap(data);

  // Write to file for later in-depth analysis of crash dumps with Nsight Graphics
  writeShaderDebugInformationToFile(identifier, pShaderDebugInfo, shaderDebugInfoSize);
}

// Handler for GPU crash dump description callbacks
void GpuCrashTrackerImpl::onDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription)
{
  // Add some basic description about the crash. This is called after the GPU crash happens, but before
  // the actual GPU crash dump callback. The provided data is included in the crash dump and can be
  // retrieved using GFSDK_Aftermath_GpuCrashDump_GetDescription().
  addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, getProjectName().c_str());
  addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "v1.0");
}

// Handler for app-managed marker resolve callback
void GpuCrashTrackerImpl::onResolveMarker(const void* pMarker, void** resolvedMarkerData, uint32_t* markerSize)
{
  // Important: the pointer passed back via resolvedMarkerData must remain valid after this function returns
  // using references for all of the m_markerMap accesses ensures that the pointers refer to the persistent data
  for(const auto& map : m_markerMap)
  {
    const auto& found_marker = map.find((uint64_t)pMarker);
    if(found_marker != map.end())
    {
      const std::string& marker_data = found_marker->second;
      // std::string::data() will return a valid pointer until the string is next modified
      // we don't modify the string after calling data() here, so the pointer should remain valid
      *resolvedMarkerData = (void*)marker_data.data();
      *markerSize         = static_cast<uint32_t>(marker_data.length());
      return;
    }
  }
}

// Helper for writing a GPU crash dump to a file
void GpuCrashTrackerImpl::writeGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
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

  std::vector<char> applicationName(applicationNameLength, '\0');

  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetDescription(decoder, GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
                                                                    uint32_t(applicationName.size()), applicationName.data()));

  // Create a unique file name for writing the crash dump data to a file.
  // Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
  // driver release) we may see redundant crash dumps. As a workaround,
  // attach a unique count to each generated file name.
  static int        count = 0;
  const std::string base_file_name =
      std::string(applicationName.data()) + "-" + std::to_string(baseInfo.pid) + "-" + std::to_string(++count);

  // Write the crash dump data to a file using the .nv-gpudmp extension
  // registered with Nsight Graphics.
  const std::string crash_dump_file_name = base_file_name + ".nv-gpudmp";
  const std::filesystem::path crash_dump_file_path(std::filesystem::absolute(crash_dump_file_name));
  std::cout << "\n--------------------------------------------------------------\n"
            << "Writing Aftermath dump file to:\n " << crash_dump_file_path
            << "\n--------------------------------------------------------------\n"
            << std::endl;

  std::ofstream dump_file(crash_dump_file_path, std::ios::out | std::ios::binary);
  if(dump_file)
  {
    dump_file.write(static_cast<const char*>(pGpuCrashDump), gpuCrashDumpSize);
    dump_file.close();
  }

  // Decode the crash dump to a JSON string.
  // Step 1: Generate the JSON and get the size.
  uint32_t jsonSize = 0;
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
      decoder, GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO, GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
      shaderDebugInfoLookupCallback, shaderLookupCallback, shaderSourceDebugInfoLookupCallback, this, &jsonSize));
  // Step 2: Allocate a buffer and fetch the generated JSON.
  std::vector<char> json(jsonSize);
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetJSON(decoder, uint32_t(json.size()), json.data()));

  // Write the crash dump data as JSON to a file.
  const std::string json_file_name = crash_dump_file_name + ".json";
  const std::filesystem::path json_file_path(std::filesystem::absolute(json_file_name));
  std::cout << "\n--------------------------------------------------------------\n"
            << "Writing JSON dump file to:\n " << json_file_path
            << "\n--------------------------------------------------------------\n"
            << std::endl;

  std::ofstream json_file(json_file_path, std::ios::out | std::ios::binary);
  if(json_file)
  {
    // Write the JSON to the file (excluding string termination)
    json_file.write(json.data(), json.size() - 1);
    json_file.close();
  }

  // Destroy the GPU crash dump decoder object.
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder));
}

// Helper for writing shader debug information to a file
void GpuCrashTrackerImpl::writeShaderDebugInformationToFile(GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier,
                                                        const void*                               pShaderDebugInfo,
                                                        const uint32_t                            shaderDebugInfoSize)
{
  // Create a unique file name.
  const std::string file_path = "shader-" + std::to_string(identifier) + ".nvdbg";

  std::ofstream f(file_path, std::ios::out | std::ios::binary);
  if(f)
  {
    f.write(static_cast<const char*>(pShaderDebugInfo), shaderDebugInfoSize);
  }
}

// Handler for shader debug information lookup callbacks.
// This is used by the JSON decoder for mapping shader instruction
// addresses to SPIR-V IL lines or GLSL source lines.
void GpuCrashTrackerImpl::onShaderDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier,
                                              PFN_GFSDK_Aftermath_SetData                      setShaderDebugInfo) const
{
  // Search the list of shader debug information blobs received earlier.
  auto i_debug_info = m_shaderDebugInfo.find(identifier);
  if(i_debug_info == m_shaderDebugInfo.end())
  {
    // Early exit, nothing found. No need to call setShaderDebugInfo.
    return;
  }

  // Let the GPU crash dump decoder know about the shader debug information
  // that was found.
  setShaderDebugInfo(i_debug_info->second.data(), static_cast<uint32_t>(i_debug_info->second.size()));
}

// Handler for shader lookup callbacks.
// This is used by the JSON decoder for mapping shader instruction
// addresses to SPIR-V IL lines or GLSL source lines.
// NOTE: If the application loads stripped shader binaries (ie; --strip-all in spirv-remap),
// Aftermath will require access to both the stripped and the not stripped
// shader binaries.
void GpuCrashTrackerImpl::onShaderLookup(const GFSDK_Aftermath_ShaderBinaryHash& shaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary) const
{
  // Find shader binary data for the shader hash in the shader database.
  std::vector<uint32_t> shader_binary;
  if(!findShaderBinary(shaderHash, shader_binary))
  {
    // Early exit, nothing found. No need to call setShaderBinary.
    return;
  }

  // Let the GPU crash dump decoder know about the shader data
  // that was found.
  setShaderBinary(shader_binary.data(), sizeof(uint32_t) * static_cast<uint32_t>(shader_binary.size()));
}

// Handler for shader source debug info lookup callbacks.
// This is used by the JSON decoder for mapping shader instruction addresses to
// GLSL source lines, if the shaders used by the application were compiled with
// separate debug info data files.
void GpuCrashTrackerImpl::onShaderSourceDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugName& shaderDebugName,
                                                    PFN_GFSDK_Aftermath_SetData            setShaderBinary) const
{
  // Find source debug info for the shader DebugName in the shader database.
  std::vector<uint32_t> shader_binary;
  if(!findShaderBinaryWithDebugData(shaderDebugName, shader_binary))
  {
    // Early exit, nothing found. No need to call setShaderBinary.
    return;
  }

  // Let the GPU crash dump decoder know about the shader debug data that was
  // found.
  setShaderBinary(shader_binary.data(), sizeof(uint32_t) * static_cast<uint32_t>(shader_binary.size()));
}

// Static callback wrapper for OnCrashDump
void GpuCrashTrackerImpl::gpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
{
  auto* p_gpu_crash_tracker = reinterpret_cast<GpuCrashTrackerImpl*>(pUserData);
  p_gpu_crash_tracker->onCrashDump(pGpuCrashDump, gpuCrashDumpSize);
}

// Static callback wrapper for OnShaderDebugInfo
void GpuCrashTrackerImpl::shaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData)
{
  auto* p_gpu_crash_tracker = reinterpret_cast<GpuCrashTrackerImpl*>(pUserData);
  p_gpu_crash_tracker->onShaderDebugInfo(pShaderDebugInfo, shaderDebugInfoSize);
}

// Static callback wrapper for OnDescription
void GpuCrashTrackerImpl::crashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData)
{
  auto* p_gpu_crash_tracker = reinterpret_cast<GpuCrashTrackerImpl*>(pUserData);
  p_gpu_crash_tracker->onDescription(addDescription);
}

// Static callback wrapper for OnResolveMarker
void GpuCrashTrackerImpl::resolveMarkerCallback(const void* pMarker, void* pUserData, void** resolvedMarkerData, uint32_t* markerSize)
{
  auto* p_gpu_crash_tracker = reinterpret_cast<GpuCrashTrackerImpl*>(pUserData);
  p_gpu_crash_tracker->onResolveMarker(pMarker, resolvedMarkerData, markerSize);
}

// Static callback wrapper for OnShaderDebugInfoLookup
void GpuCrashTrackerImpl::shaderDebugInfoLookupCallback(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier,
                                                    PFN_GFSDK_Aftermath_SetData                      setShaderDebugInfo,
                                                    void*                                            pUserData)
{
  auto* p_gpu_crash_tracker = reinterpret_cast<GpuCrashTrackerImpl*>(pUserData);
  p_gpu_crash_tracker->onShaderDebugInfoLookup(*pIdentifier, setShaderDebugInfo);
}

// Static callback wrapper for OnShaderLookup
void GpuCrashTrackerImpl::shaderLookupCallback(const GFSDK_Aftermath_ShaderBinaryHash* pShaderHash,
                                           PFN_GFSDK_Aftermath_SetData             setShaderBinary,
                                           void*                                   pUserData)
{
  auto* p_gpu_crash_tracker = reinterpret_cast<GpuCrashTrackerImpl*>(pUserData);
  p_gpu_crash_tracker->onShaderLookup(*pShaderHash, setShaderBinary);
}

// Static callback wrapper for OnShaderSourceDebugInfoLookup
void GpuCrashTrackerImpl::shaderSourceDebugInfoLookupCallback(const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName,
                                                          PFN_GFSDK_Aftermath_SetData            setShaderBinary,
                                                          void*                                  pUserData)
{
  auto* p_gpu_crash_tracker = reinterpret_cast<GpuCrashTrackerImpl*>(pUserData);
  p_gpu_crash_tracker->onShaderSourceDebugInfoLookup(*pShaderDebugName, setShaderBinary);
}


void GpuCrashTrackerImpl::addShaderBinary(std::vector<uint32_t>& data)
{

  // Create shader hash for the shader
  const GFSDK_Aftermath_SpirvCode  shader{data.data(), static_cast<uint32_t>(data.size())};
  GFSDK_Aftermath_ShaderBinaryHash shaderHash{};
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderHashSpirv(GFSDK_Aftermath_Version_API, &shader, &shaderHash));

  // Store the data for shader mapping when decoding GPU crash dumps.
  // cf. FindShaderBinary()
  m_shaderBinaries[shaderHash] = data;
}

void GpuCrashTrackerImpl::addShaderBinaryWithDebugInfo(std::vector<uint32_t>& data, std::vector<uint32_t>& strippedData)
{
  // Generate shader debug name.
  GFSDK_Aftermath_ShaderDebugName debugName{};
  const GFSDK_Aftermath_SpirvCode shader{data.data(), static_cast<uint32_t>(data.size())};
  const GFSDK_Aftermath_SpirvCode strippedShader{strippedData.data(), static_cast<uint32_t>(strippedData.size())};
  AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugNameSpirv(GFSDK_Aftermath_Version_API, &shader, &strippedShader, &debugName));

  // Store the data for shader instruction address mapping when decoding GPU crash dumps.
  // cf. FindShaderBinaryWithDebugData()
  m_shaderBinariesWithDebugInfo[debugName] = data;
}

// Find a shader binary by shader hash.
bool GpuCrashTrackerImpl::findShaderBinary(const GFSDK_Aftermath_ShaderBinaryHash& shaderHash, std::vector<uint32_t>& shader) const
{
  // Find shader binary data for the shader hash
  auto i_shader = m_shaderBinaries.find(shaderHash);
  if(i_shader == m_shaderBinaries.end())
  {
    // Nothing found.
    return false;
  }

  shader = i_shader->second;
  return true;
}

// Find a shader binary with debug information by shader debug name.
bool GpuCrashTrackerImpl::findShaderBinaryWithDebugData(const GFSDK_Aftermath_ShaderDebugName& shaderDebugName,
                                                    std::vector<uint32_t>&                 shader) const
{
  // Find shader binary for the shader debug name.
  auto i_shader = m_shaderBinariesWithDebugInfo.find(shaderDebugName);
  if(i_shader == m_shaderBinariesWithDebugInfo.end())
  {
    // Nothing found.
    return false;
  }

  shader = i_shader->second;
  return true;
}


  // Global marker map
static GpuCrashTrackerImpl::MarkerMap g_marker_map;

GpuCrashTracker::GpuCrashTracker()
    : m_pimpl(new GpuCrashTrackerImpl(g_marker_map))
{
}

GpuCrashTracker::~GpuCrashTracker()
{
  delete m_pimpl;
}

void GpuCrashTracker::initialize()
{
  m_pimpl->initialize();
}

}  // namespace nvvk

#else

namespace nvvk {

GpuCrashTracker::GpuCrashTracker()
    : m_pimpl(nullptr)
{
}

GpuCrashTracker::~GpuCrashTracker() {}

void GpuCrashTracker::initialize() {}

}  // namespace nvvk

#endif

