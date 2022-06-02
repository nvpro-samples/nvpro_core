/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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


#ifndef NV_SHADERMODULEMANAGER_INCLUDED
#define NV_SHADERMODULEMANAGER_INCLUDED

#include <mutex>
#include <stdio.h>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#if NVP_SUPPORTS_SHADERC
#define NV_EXTENSIONS
#include <shaderc/shaderc.h>
#undef NV_EXTENSIONS
#endif

#include <nvh/shaderfilemanager.hpp>


namespace nvvk {

//////////////////////////////////////////////////////////////////////////
/**
  \class nvvk::ShaderModuleManager

  The nvvk::ShaderModuleManager manages VkShaderModules stored in files (SPIR-V or GLSL)

  Using ShaderFileManager it will find the files and resolve #include for GLSL.
  You must add include directories to the base-class for this.

  It also comes with some convenience functions to reload shaders etc.
  That is why we pass out the ShaderModuleID rather than a VkShaderModule directly.

  To change the compilation behavior manipulate the public member variables
  prior createShaderModule.

  m_filetype is crucial for this. You can pass raw spir-v files or GLSL.
  If GLSL is used, shaderc must be used as well (which must be added via
  _add_package_ShaderC() in CMake of the project)

  Example:

  \code{.cpp}
  ShaderModuleManager mgr(myDevice);

  // derived from ShaderFileManager
  mgr.addDirectory("spv/");

  // all shaders get this injected after #version statement
  mgr.m_prepend = "#define USE_NOISE 1\n";

  vid = mgr.createShaderModule(VK_SHADER_STAGE_VERTEX_BIT,   "object.vert.glsl");
  fid = mgr.createShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "object.frag.glsl");

  // ... later use module
  info.module = mgr.get(vid);
  \endcode
*/

class ShaderModuleID
{
public:
  size_t m_value;

  ShaderModuleID()
      : m_value(size_t(~0))
  {
  }

  ShaderModuleID(size_t b)
      : m_value(b)
  {
  }
  ShaderModuleID& operator=(size_t b)
  {
    m_value = b;
    return *this;
  }

  bool isValid() const { return m_value != size_t(~0); }

  operator bool() const { return isValid(); }
  operator size_t() const { return m_value; }

  friend bool operator==(const ShaderModuleID& lhs, const ShaderModuleID& rhs) { return rhs.m_value == lhs.m_value; }
};

class ShaderModuleManager : public nvh::ShaderFileManager
{
public:
  struct ShaderModule
  {
    ShaderModule()
        : module(0)
    {
    }

    VkShaderModule module;
    std::string    moduleSPIRV;
    Definition     definition;
  };

  void init(VkDevice device, int apiMajor = 1, int apiMinor = 1);

  // also calls deleteShaderModules
  void deinit();

  ShaderModuleID createShaderModule(uint32_t           type,
                                    std::string const& filename,
                                    std::string const& prepend   = "",
                                    FileType           fileType  = FILETYPE_DEFAULT,
                                    std::string const& entryname = "main");

  void destroyShaderModule(ShaderModuleID idx);
  void reloadModule(ShaderModuleID idx);

  void reloadShaderModules();
  void deleteShaderModules();
  bool areShaderModulesValid();

#if NVP_SUPPORTS_SHADERC
  void setOptimizationLevel(shaderc_optimization_level level) { m_shadercOptimizationLevel = level; }
#endif


  bool                isValid(ShaderModuleID idx) const;
  VkShaderModule      get(ShaderModuleID idx) const;
  ShaderModule&       getShaderModule(ShaderModuleID idx);
  const ShaderModule& getShaderModule(ShaderModuleID idx) const;
  const char*         getCode(ShaderModuleID idx, size_t* len = NULL) const;
  const size_t        getCodeLen(ShaderModuleID idx) const;
  bool                dumpSPIRV(ShaderModuleID idx, const char* filename) const;
  bool                getSPIRV(ShaderModuleID idx, size_t* pLen, const uint32_t** pCode) const;


  // state will affect the next created shader module
  // also keep m_filetype in mind!
  bool m_preprocessOnly  = false;
  bool m_keepModuleSPIRV = false;

  //////////////////////////////////////////////////////////////////////////
  //
  // for internal development, useful when we have new shader types that
  // are not covered by public VulkanSDK

  struct SetupInterface
  {
    // This class is to aid using a shaderc library version that is not
    // provided by the Vulkan SDK, but custom. Therefore it allows custom settings etc.
    // Useful for driver development of new shader stages, otherwise can be pretty much ignored.

    virtual std::string getTypeDefine(uint32_t type) const      = 0;
    virtual uint32_t    getTypeShadercKind(uint32_t type) const = 0;
    virtual void*       getShadercCompileOption(void* shadercCompiler) { return nullptr; }
  };

  void setSetupIF(SetupInterface* setupIF);


  ShaderModuleManager(ShaderModuleManager const&) = delete;
  ShaderModuleManager& operator=(ShaderModuleManager const&) = delete;

  // Constructors reference-count the shared shaderc compiler, and
  // disable ShaderFileManager's homemade #include mechanism iff we're
  // using shaderc.
#if NVP_SUPPORTS_SHADERC
  static constexpr bool s_handleIncludePasting = false;
#else
  static constexpr bool s_handleIncludePasting = true;
#endif

  ShaderModuleManager(VkDevice device = nullptr)
      : ShaderFileManager(s_handleIncludePasting)
  {
    m_usedSetupIF             = &m_defaultSetupIF;
    m_supportsExtendedInclude = true;

    if(device)
      init(device);
  }

  ~ShaderModuleManager()
  {
    deinit();
  }

  // Shaderc has its own interface for handling include files that I
  // have to subclass; this needs access to protected
  // ShaderFileManager functions.
  friend class ShadercIncludeBridge;

private:
  ShaderModuleID createShaderModule(const Definition& def);
  bool           setupShaderModule(ShaderModule& prog);


  struct DefaultInterface : public SetupInterface
  {
    std::string getTypeDefine(uint32_t type) const override;
    uint32_t    getTypeShadercKind(uint32_t type) const override;
  };


  static const VkShaderModule PREPROCESS_ONLY_MODULE;

  VkDevice         m_device = nullptr;
  DefaultInterface m_defaultSetupIF;
  SetupInterface*  m_usedSetupIF = nullptr;

  int m_apiMajor = 1;
  int m_apiMinor = 1;

#if NVP_SUPPORTS_SHADERC
  static uint32_t            s_shadercCompilerUsers;
  static shaderc_compiler_t  s_shadercCompiler;  // Lock mutex below while using.
  static std::mutex          s_shadercCompilerMutex;
  shaderc_compile_options_t  m_shadercOptions           = nullptr;
  shaderc_optimization_level m_shadercOptimizationLevel = shaderc_optimization_level_performance;
#endif

  std::vector<ShaderModule> m_shadermodules;
};

}  // namespace nvvk


#endif  //NV_PROGRAM_INCLUDED
