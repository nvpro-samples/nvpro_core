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

#pragma once

#include <fstream>
#include <nvmath/nvmath.h>
#include <vulkan/vulkan.hpp>

/** 
 Various utility functions
*/


namespace nvvkpp {
namespace util {


/** 
**clearColor**: easy clear color contructor.
*/
inline vk::ClearColorValue clearColor(const nvmath::vec4f& v = nvmath::vec4f(0.f, 0.f, 0.f, 0.f))
{
  vk::ClearColorValue result;
  memcpy(&result.float32, &v.x, sizeof(result.float32));
  return result;
}

/** 
**createShaderModule**: create a shader module from SPIR-V code.
*/
inline vk::ShaderModule createShaderModule(const vk::Device& device, size_t size, const void* code)
{
  return device.createShaderModule(vk::ShaderModuleCreateInfo({}, size, static_cast<const uint32_t*>(code)));
}

template <typename T>
inline vk::ShaderModule createShaderModule(const vk::Device& device, const std::vector<T>& code)
{
  return createShaderModule(device, code.size() * sizeof(T), code.data());
}

inline vk::ShaderModule createShaderModule(const vk::Device& device, const std::string& code)
{
  return createShaderModule(device, code.size(), code.data());
}

template <typename T>
inline vk::PipelineShaderStageCreateInfo loadShader(const vk::Device&       device,
                                                    const std::vector<T>&   code,
                                                    vk::ShaderStageFlagBits stage,
                                                    const char*             entryPoint = "main")
{
  vk::PipelineShaderStageCreateInfo shaderStage;
  shaderStage.stage  = stage;
  shaderStage.module = createShaderModule(device, code);
  shaderStage.pName  = entryPoint;
  return shaderStage;
}

inline vk::PipelineShaderStageCreateInfo loadShader(const vk::Device&       device,
                                                    const std::string&      code,
                                                    vk::ShaderStageFlagBits stage,
                                                    const char*             entryPoint = "main")
{
  vk::PipelineShaderStageCreateInfo shaderStage;
  shaderStage.stage  = stage;
  shaderStage.module = createShaderModule(device, code);
  shaderStage.pName  = entryPoint;
  return shaderStage;
}


/**
**linker**: link VK structures through their pNext

Example of Usage:
~~~~~~ C++
vk::Something a;
vk::SomethingElse b;
vk::Other c;
nvvk::util::linker(a,b,c); // will get a.pNext -> b.pNext -> c.pNext -> nullptr
~~~~~~
*/
template <typename T>
void linker(T& first)
{
  first.setPNext(nullptr);
}

template <typename T, typename S, typename... Args>
void linker(T& first, S& second, Args... args)
{
  first.setPNext(&second);
  linker(second, args...);
}


}  // namespace util
}  // namespace nvvkpp
