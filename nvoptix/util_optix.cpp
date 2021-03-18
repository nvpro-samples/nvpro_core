/*-----------------------------------------------------------------------
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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
*/ //--------------------------------------------------------------------

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <nvrtc.h>

#pragma message("TODO TODO TODO TODO TODO TODO TODO : assetsloader DELETED... Replace this by another helper if required. ")
//#include "nvh/assetsloader.hpp"
#include "util_optix.h"

namespace fs = std::experimental::filesystem;

// Error check/report helper for users of the C API
#define NVRTC_CHECK_ERROR(func)                                                                                        \
  do                                                                                                                   \
  {                                                                                                                    \
    nvrtcResult code = func;                                                                                           \
    if(code != NVRTC_SUCCESS)                                                                                          \
      throw optix::Exception("ERROR: " __FILE__ "( ): " + std::string(nvrtcGetErrorString(code)));                     \
  } while(0)

// NVRTC compiler options
#define CUDA_NVRTC_OPTIONS                                                                                             \
  "-arch", "compute_30", "-use_fast_math", "-lineinfo", "-default-device", "-rdc", "true", "-D__x86_64", 0,

//--------------------------------------------------------------------------------------------------
// Return the path to the executable
//
std::string getExecutablePath()
{
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

  // Windows
  static char buf[MAX_PATH];
  GetModuleFileNameA(nullptr, buf, MAX_PATH);
  std::string exeDirectory(buf);
  // Here, the path is raw Windows path, you can not change \\ to /
  exeDirectory.erase(exeDirectory.find_last_of('\\') + 1, std::string::npos);
  return exeDirectory;
}

//--------------------------------------------------------------------------------------------------
//
//
void getPtxFromCuString(std::string& ptx, const std::string& cu_source, const std::string& name, std::string& log_string)
{
  // Create program
  nvrtcProgram prog = nullptr;
  NVRTC_CHECK_ERROR(nvrtcCreateProgram(&prog, cu_source.c_str(), name.c_str(), 0, nullptr, nullptr));

  // Gather NVRTC options
  std::vector<std::string> options;

  // Set sample dir as the primary include path
  options.emplace_back("-I.");
  options.push_back("-I" + name);
  options.push_back("-I" + getExecutablePath());
  options.push_back("-I" + getExecutablePath() + "/cuda");


  // Collect include dirs for the Optix and Cuda paths (see the macros and preprocessor definitions
  // optix.props)
  // this case is when the sample gets compiled from the same machine : we know where to find CUDA and OptiX from
  // what cmake told the sample. It's convenient for testing the sampel (and Teamcity)
  options.push_back(std::string("-I") + std::string(OPTIX_PATH) + "/include");
  options.push_back(std::string("-I") + std::string(OPTIX_PATH) + "/include/optixu");
  options.push_back(std::string("-I") + std::string(OPTIX_PATH) + "/SDK/support/mdl-sdk/include");
  options.push_back(std::string("-I") + std::string(CUDA_PATH) + "/include");
  // Now let's add more: the local installed versions.
  // it is possible that the machine running it only get some officially installed versions
  const char* env_CUDA = NULL;
  if(env_CUDA = std::getenv("CUDA_PATH"))
  {
    options.push_back(std::string("-I") + std::string(env_CUDA) + "/include");
  }
  const char* env_OPTIX = NULL;
  if((env_OPTIX = std::getenv("OPTIX_PATH")) == NULL)
  {
    // try to locate it by ourselves
#ifdef WIN32
    static char* str_hardcoded_path = "C:\\ProgramData\\NVIDIA Corporation\\OptiX SDK " OPTIX_VERSION_STR;
#else
    static char* str_hardcoded_path = "TODO: /usr.../OptiX SDK " OPTIX_VERSION_STR;
#endif
    if(fs::exists(str_hardcoded_path))
      env_OPTIX = str_hardcoded_path;
  }
  if(env_OPTIX)
  {
    if(strstr(env_OPTIX, OPTIX_VERSION_STR) == NULL)
    {
      char str[300];
      sprintf(str, "Wrong version: needed %s and found %s", OPTIX_VERSION_STR, env_OPTIX);
      MessageBoxA(nullptr, str, "OptiX Warning", MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);
    }
    options.push_back(std::string("-I") + std::string(env_OPTIX) + "/include");
    options.push_back(std::string("-I") + std::string(env_OPTIX) + "/include/optixu");
    options.push_back(std::string("-I") + std::string(env_OPTIX) + "/SDK/support/mdl-sdk/include");
  }

  // Collect NVRTC options
  const char*  compiler_options[] = {CUDA_NVRTC_OPTIONS};
  const size_t n_compiler_options = sizeof(compiler_options) / sizeof(compiler_options[0]);
  for(size_t i = 0; i < n_compiler_options - 1; i++)
    options.emplace_back(compiler_options[i]);

  // JIT compile CU to PTX
  std::vector<const char*> coptions;
  coptions.reserve(options.size());
  for(auto& o : options)
    coptions.push_back(o.c_str());

  const nvrtcResult compileRes = nvrtcCompileProgram(prog, (int)options.size(), coptions.data());

  // Retrieve log output
  size_t      log_size = 0;
  std::string nvrtcLog;
  NVRTC_CHECK_ERROR(nvrtcGetProgramLogSize(prog, &log_size));
  nvrtcLog.resize(log_size);
  if(log_size > 1)
  {
    NVRTC_CHECK_ERROR(nvrtcGetProgramLog(prog, &nvrtcLog[0]));
    log_string = nvrtcLog;
  }
  if(compileRes != NVRTC_SUCCESS)
    throw optix::Exception("NVRTC Compilation failed.\n" + nvrtcLog);

  // Retrieve PTX code
  size_t ptx_size = 0;
  NVRTC_CHECK_ERROR(nvrtcGetPTXSize(prog, &ptx_size));
  ptx.resize(ptx_size);
  NVRTC_CHECK_ERROR(nvrtcGetPTX(prog, &ptx[0]));

  // Cleanup
  NVRTC_CHECK_ERROR(nvrtcDestroyProgram(&prog));
}

//--------------------------------------------------------------------------------------------------
// Get the PTX from the Cuda file.
// It first look if a "cuda_file.ptx" exist and newer, and return it
// Otherwise compile the Cuda to PTX and save a copy to disk
//
void OptixUtil::getPtxString(const std::string& sampleName, const std::string& cuda_file, std::string& ptx_out)
{
  // Will save the PTX next to the executable under the `sampleName` folder
  std::string ptxPath = getExecutablePath() + sampleName + "/";
  fs::create_directory(ptxPath);
  std::string ptxfilename = fs::path(cuda_file).filename().string();
  ptxfilename             = ptxfilename.substr(0, ptxfilename.find(".cu")) + ".ptx";
  std::string ptxName     = ptxPath + ptxfilename;


  std::string cudaFilename = AssetLoaderFindFile(cuda_file);

  // The Cuda file or the PTX must exist
  if(!fs::exists(ptxName) && cudaFilename.empty())
  {
    std::stringstream o;
    o << "Cannot find the Cuda file " << cuda_file << " under " << fs::current_path();
    o << "\n neither the PTX " << ptxName;
    throw optix::Exception(o.str());
  }

  // Check if there is a PTX already compiled and newer than the CU source
  std::error_code err_c;
  auto            tptx = fs::last_write_time(ptxPath, err_c);
  auto            tcu  = fs::last_write_time(cudaFilename, err_c);
  if(tptx >= tcu)
  {
    std::ifstream file(ptxName.c_str());
    if(file.good())
    {
      std::stringstream source_buffer;
      source_buffer << file.rdbuf();
      ptx_out = source_buffer.str();
      file.close();
      return;
    }
  }

  // Load the source of the Cuda file
  std::string cu, filenameFound;
  cu = AssetLoadTextFile(cuda_file, filenameFound);
  if(cu.empty())
  {
    throw optix::Exception("Cannot find Cuda file " + cuda_file);
  }

  // Grab the path to the Cuda file location to add it as include path
  std::string location;
  std::size_t found = filenameFound.find_last_of("/\\");
  location          = filenameFound.substr(0, found);

  // Compile the Cuda to PTX
  std::string log;
  getPtxFromCuString(ptx_out, cu, location, log);

  // Export the PTX file to avoid recompiling it
  std::ofstream fileout(ptxName.c_str());
  if(fileout.good())
  {
    fileout << ptx_out;
    fileout.close();
  }
}

//--------------------------------------------------------------------------------------------------
// Nice way to extract most of the information regarding the error
//
void OptixUtil::handleException(const optix::Exception& e, optix::Context ctx)
{
  std::stringstream o;
  std::string       error_string = ctx->getErrorString(e.getErrorCode());

  o << "Err(" << e.getErrorCode() << "): \n\n" << e.getErrorString() << "\n\n";

  if(e.getErrorString() != error_string)
    o << error_string;

  o << std::endl;
  std::cerr << o.str();

#if defined(_WIN32) && defined(_DEBUG)
  {
    std::vector<char> s(o.str().length() + 1);
    sprintf(s.data(), "OptiX Error: %s", o.str().c_str());
    MessageBoxA(nullptr, s.data(), "OptiX Error", MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);
    exit(1);
  }
#endif
}
