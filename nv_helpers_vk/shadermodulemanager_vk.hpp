/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
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
 */

#ifndef NV_SHADERMODULEMANAGER_INCLUDED
#define NV_SHADERMODULEMANAGER_INCLUDED


#include <stdio.h>
#include <vector>
#include <string>
#include <vulkan/vulkan.h>

#if USESHADERC
#define SHADERC_SHAREDLIB
#define NV_EXTENSIONS
#include <shaderc/shaderc.h>
#undef NV_EXTENSIONS
#endif

#include <nv_helpers/shaderfilemanager.hpp>
#include <nv_helpers/assetsloader.hpp>


namespace nv_helpers_vk
{

  class ShaderModuleManager : public nv_helpers::ShaderFileManager {
  public:

    struct SetupInterface {
      virtual std::string getTypeDefine(uint32_t type) const = 0;
      virtual uint32_t    getTypeShadercKind(uint32_t type) const = 0;
      virtual void*       getShadercCompileOption(void* shadercCompiler) { return nullptr; }
    };

    struct DefaultInterface : public SetupInterface {
      std::string getTypeDefine(uint32_t type) const override;;
      uint32_t    getTypeShadercKind(uint32_t type) const override;
    };

    class ShaderModuleID {
    public:
      size_t  m_value;

      ShaderModuleID() : m_value(~0) {}

      ShaderModuleID( size_t b) : m_value(b) {}
      operator size_t() const { return m_value; }
      ShaderModuleID& operator=( size_t b) { m_value = b; return *this; }

    };

    struct ShaderModule {
      ShaderModule() : module(0), useNVextension(false) {}

      VkShaderModule            module;
      Definition                definition;
      bool                      useNVextension;
    };
    
    ShaderModuleID createShaderModule(const Definition& def);

    void destroyShaderModule( ShaderModuleID idx );
    void reloadModule(ShaderModuleID idx);

    void reloadShaderModules();
    void deleteShaderModules();
    bool areShaderModulesValid();


    bool isValid( ShaderModuleID idx ) const;
    VkShaderModule get(ShaderModuleID idx) const;
    ShaderModule& getShaderModule(ShaderModuleID idx);
    const ShaderModule& getShaderModule(ShaderModuleID idx) const;
    const char* getCode(ShaderModuleID idx, size_t *len=NULL) const;
    const size_t getCodeLen(ShaderModuleID idx) const;

    bool                  m_preprocessOnly = false;
    bool                  m_rawOnly = false;
    bool                  m_useNVextension = false;
    VkDevice              m_device = VK_NULL_HANDLE;
    DefaultInterface      m_defaultSetupIF;
    SetupInterface*       m_usedSetupIF = nullptr;
    int                   m_apiMajor = 1;
    int                   m_apiMinor = 1;

#if USESHADERC
    shaderc_compiler_t m_shadercCompiler = nullptr;
    shaderc_compile_options_t m_shadercOptions = nullptr;
#endif

    ShaderModuleManager()
    {
      m_usedSetupIF = &m_defaultSetupIF;
      m_supportsExtendedInclude = true;
#if USESHADERC
      m_shadercCompiler = shaderc_compiler_initialize();
      m_shadercOptions = shaderc_compile_options_initialize();
#endif
    }

    ~ShaderModuleManager()
    {
#if USESHADERC
      if (m_shadercCompiler) {
        shaderc_compiler_release(m_shadercCompiler);
      }
      if (m_shadercOptions) {
        shaderc_compile_options_release(m_shadercOptions);
      }
#endif
    }

  private:
    static const VkShaderModule PREPROCESS_ONLY_MODULE;

    bool setupShaderModule(ShaderModule& prog);

    std::vector<ShaderModule> m_shadermodules;
  };

}


#endif//NV_PROGRAM_INCLUDED
