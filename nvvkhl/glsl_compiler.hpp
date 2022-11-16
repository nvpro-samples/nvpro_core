/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

/*

Compiling GLSL to Spir-V using shaderC

This class overloads the shaderc::Compiler to help
compiling files which have includes.


*/


#pragma once


#include <shaderc/shaderc.hpp>
#include "nvh/fileoperations.hpp"
#include "nvvk/error_vk.hpp"

namespace nvvkhl {

class GlslCompiler : public shaderc::Compiler
{
public:
  GlslCompiler() { m_compilerOptions = std::make_unique<shaderc::CompileOptions>(); };
  ~GlslCompiler() = default;

  void                     addInclude(const std::string& p) { m_includePath.push_back(p); }
  shaderc::CompileOptions* options() { return m_compilerOptions.get(); }

  shaderc::SpvCompilationResult compileFile(const std::string& filename, shaderc_shader_kind shader_kind)
  {
    std::string find_file = nvh::findFile(filename, m_includePath, true);
    if(find_file.empty())
      return {};
    std::string source_code = nvh::loadFile(find_file, false, m_includePath, true);
    m_compilerOptions->SetIncluder(std::make_unique<GlslIncluder>(m_includePath));

    return CompileGlslToSpv(source_code, shader_kind, find_file.c_str(), *m_compilerOptions);
  }

  VkShaderModule createModule(VkDevice device, const shaderc::SpvCompilationResult& compResult)
  {
    VkShaderModuleCreateInfo shaderModuleCreateInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shaderModuleCreateInfo.codeSize = (compResult.end() - compResult.begin()) * sizeof(uint32_t);
    shaderModuleCreateInfo.pCode    = reinterpret_cast<const uint32_t*>(compResult.begin());

    VkShaderModule shaderModule;
    NVVK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule));
    return shaderModule;
  }

  void resetOptions() { m_compilerOptions = std::make_unique<shaderc::CompileOptions>(); }
  void resetIncludes() { m_includePath = {}; }

private:
  std::vector<std::string>                 m_includePath;
  std::unique_ptr<shaderc::CompileOptions> m_compilerOptions;

  // Implementation of the libshaderc includer interface.
  class GlslIncluder : public shaderc::CompileOptions::IncluderInterface
  {
  public:
    GlslIncluder(std::vector<std::string> includePaths)
        : m_includePaths(includePaths)
    {
    }

    // Subtype of shaderc_include_result that holds the include data we found;
    // MUST be static_cast to this type before deleting as shaderc_include_result lacks virtual destructor.
    struct IncludeResult : public shaderc_include_result
    {
      IncludeResult(std::string content, std::string filenameFound)
          : m_content(std::move(content))
          , m_filenameFound(std::move(filenameFound))
      {
        this->source_name        = m_filenameFound.data();
        this->source_name_length = m_filenameFound.size();
        this->content            = m_content.data();
        this->content_length     = m_content.size();
        this->user_data          = nullptr;
      }
      const std::string m_content;
      const std::string m_filenameFound;
    };

    shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override
    {
      std::string find_name = nvh::findFile(requested_source, m_includePaths, true /*warn*/);
      std::string src_code  = nvh::loadFile(find_name, false /*binary*/, m_includePaths, true /*warn*/);
      return new IncludeResult(src_code, find_name);
    }

    // Handles shaderc_include_result_release_fn callbacks.
    void ReleaseInclude(shaderc_include_result* data) override { delete data; };

    std::vector<std::string> m_includePaths;
  };
};


}  // namespace nvvkhl
