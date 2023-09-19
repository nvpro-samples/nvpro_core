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


#include <memory>
#include <filesystem>
#include <shaderc/shaderc.hpp>
#include "nvh/fileoperations.hpp"
#include "nvvk/error_vk.hpp"

namespace nvvkhl {


// Implementation of the libshaderc includer interface.
class GlslIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
  GlslIncluder(const std::vector<std::string>& includePaths)
      : m_includePaths(includePaths)
  {
  }

  // Subtype of shaderc_include_result that holds the include data we found;
  // MUST be static_cast to this type before deleting as shaderc_include_result lacks virtual destructor.
  struct IncludeResult : public shaderc_include_result
  {
    IncludeResult(const std::string& content, const std::string& filenameFound)
        : m_content(content)
        , m_filenameFound(filenameFound)
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
    // Check the relative path for #include "quotes"
    std::string find_name;
    if(type == shaderc_include_type_relative)
    {
      std::filesystem::path relative_path = std::filesystem::path(requesting_source).parent_path() / requested_source;
      if(std::filesystem::exists(relative_path))
        find_name = relative_path.string();
    }

    // If nothing found yet, search include directories
    // TODO: skip nvh::findFile searching the current working directory
    if(find_name.empty())
    {
      find_name = nvh::findFile(requested_source, m_includePaths, true /*warn*/);
    }

    std::string src_code;
    if(find_name.empty())
    {
      // [shaderc.h] For a failed inclusion, this contains the error message.
      src_code = "Could not find include file in any include path.";
    }
    else
    {
      src_code = nvh::loadFile(find_name, false /*binary*/);
    }
    return new IncludeResult(src_code, find_name);
  }

  // Handles shaderc_include_result_release_fn callbacks.
  void ReleaseInclude(shaderc_include_result* data) override { delete static_cast<IncludeResult*>(data); };

  const std::vector<std::string>& m_includePaths;
};

class GlslCompiler : public shaderc::Compiler
{
public:
  GlslCompiler() { m_compilerOptions = makeOptions(); };
  ~GlslCompiler() = default;

  void                     addInclude(const std::string& p) { m_includePaths.push_back(p); }
  shaderc::CompileOptions* options() { return m_compilerOptions.get(); }

  // Returns a blank CompileOptions object initialized with this GlslCompiler's
  // GlslIncluder. CompileOptions must not outlive the GlslCompiler as it holds
  // a reference to the includes.
  std::unique_ptr<shaderc::CompileOptions> makeOptions()
  {
    auto options = std::make_unique<shaderc::CompileOptions>();
    options->SetIncluder(std::make_unique<GlslIncluder>(m_includePaths));
    return options;
  }

  shaderc::SpvCompilationResult compileFile(const std::string& filename, shaderc_shader_kind shader_kind)
  {
    std::string find_file = nvh::findFile(filename, m_includePaths, true);
    if(find_file.empty())
      return {};
    return compileFile(find_file, shader_kind, *m_compilerOptions);
  }

  // Overload to provide a separate compile options object.
  shaderc::SpvCompilationResult compileFile(const std::string& filename, shaderc_shader_kind shader_kind, const shaderc::CompileOptions& options)
  {
    if(!std::filesystem::exists(filename))
      return {};
    std::string source_code = nvh::loadFile(filename, false);
    return CompileGlslToSpv(source_code, shader_kind, filename.c_str(), options);
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

  void resetOptions() { m_compilerOptions = makeOptions(); }
  void resetIncludes() { m_includePaths = {}; }

private:
  std::vector<std::string>                 m_includePaths;
  std::unique_ptr<shaderc::CompileOptions> m_compilerOptions;
};


}  // namespace nvvkhl
