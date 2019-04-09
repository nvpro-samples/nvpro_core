--[[
/* Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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
]]

local function generate(outfilename, header, whitelist)
  
  local function extractFeatureDefs(filename, whitelist, api, apientry)
    local f = io.open(filename,"rt")
    local str = f:read("*a")
    
    -- for the pattern to work add a dummy end
    str = str.."\n\n#define VK_DUMMY_TERMINATOR_SPEC_VERSION 1\n"
    
    -- not the best pattern since we will not catch the first extension (VK_KHR_surface) but we don't need to
    local features = {}
    for feature,defstr in str:gmatch('#define VK_[%w_]+_EXTENSION_NAME%s+"([%w_]+)"\n(.-)#define VK_[%w_]+_SPEC_VERSION%s+%d+\n') do
      print(feature)
      if (not whitelist or whitelist[feature]) then
        --print(feature)
        --print(defstr)
        local defs = ""
        for def in defstr:gmatch("#ifndef VK_NO_PROTOTYPES(.-)#endif\n") do
          def = def:gsub(api, ""):gsub(apientry, "")
          defs = defs..def
        end
        
        table.insert(features, {feature=feature, defs=defs})
      end
    end
    
    f:close()
    
    return features
  end

  local function toTab(str)
    local tab = {}
    for name in str:gmatch("[%w_]+") do 
      tab[name] = true
    end
    return tab
  end
  
  local whitelist = whitelist and toTab(whitelist)

  local sdkpath = os.getenv("VULKAN_SDK")
  assert(sdkpath, "Vulkan SDK not found")
  
  
  local gl_features = extractFeatureDefs(sdkpath.."/Include/vulkan/vulkan_core.h", whitelist, "VKAPI_ATTR ", "VKAPI_CALL ")
  
  local availables_header = ""
  local availables_source = ""
  
  local loaders_header = ""
  local loaders_source = ""
  local loaders_all = ""
  
  local function process(f, api, apientry) 
    availables_header = availables_header.."extern int has_"..f.feature..";\n"
    availables_source = availables_source.."int has_"..f.feature.." = 0;\n"
    
    loaders_header = loaders_header.."#if "..f.feature.."\n"
    loaders_header = loaders_header.."int load_"..f.feature.."(VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr);\n"
    loaders_header = loaders_header.."#endif\n\n"
    
    local defs = "\n"..f.defs:gsub("%s*%*%s*","%* ")
    
    local func      = ""
    local initfunc  = ""
    local success   = ""
    local variable  = ""
    
    for const,ret,name,arg in defs:gmatch("\n([%w_]-%s*)([%w_%*]+)%s+([%w_]+)%s*(%b());") do
      
      local prot = "PFN_"..name
      local mangled = "pfn_"..name
      
      local exec = ret ~= "void" and "return " or ""
      const = const == "\n" and "" or const
      
      exec = exec..mangled.."("
      if (arg ~= "(void)") then
        for a in arg:sub(2,-2):gmatch("([^,%%]+),?") do
          local am = a:gsub("%b[]",""):match("([%w_]+)$")
          assert(am,a)
          exec = exec..am..","
        end
        exec = exec:sub(1,-2)..");"
      else
        exec = exec..");"
      end
      
      variable = variable.."static "..prot.." "..mangled.." = 0;\n"
      
      func = func..api..const..ret.." "..apientry..name..arg.."\n{\n"
      func = func.."  assert("..mangled..");\n"
      func = func.."  "..exec.."\n"
      func = func.."}\n"
      
      initfunc  = initfunc.."  "..mangled..' = ('..prot..')getDeviceProcAddr(device, "'..name..'");\n'
      success  = success.."  success = success && ("..mangled..' != 0);\n'
    end
    
    loaders_source = loaders_source.."/* /////////////////////////////////// */\n"
    loaders_source = loaders_source.."#if "..f.feature.."\n"
    loaders_source = loaders_source..variable.."\n"
    loaders_source = loaders_source..func.."\n"
    loaders_source = loaders_source.."int load_"..f.feature.."(VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr)\n{\n"
    loaders_source = loaders_source..initfunc
    loaders_source = loaders_source.."  int success = 1;\n"
    loaders_source = loaders_source..success
    loaders_source = loaders_source.."  return success;\n}\n#endif\n\n"
  end
  
  for i,f in ipairs(gl_features) do
    process(f, "VKAPI_ATTR ", "VKAPI_CALL ")
  end
    
  local fheader = io.open(outfilename..".hpp", "wt")
  local fsource = io.open(outfilename..".cpp", "wt")
  assert(fheader, "could not open "..outfilename..".hpp for writing")
  assert(fsource, "could not open "..outfilename..".cpp for writing")
    
fheader:write(header..[[

/* Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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

#include <vulkan/vulkan.h>

/* loaders */
]]..loaders_header..[[

]])

fsource:write(header..[[

/* Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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

#ifdef WIN32
  #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <assert.h>
#include "]]..outfilename..[[.hpp"

/* loaders */
]]..loaders_source..[[

]])


end

do
  generate("extensions_vk",
    "/* auto generated by extensions_vk.lua */",
    [[
    VK_KHR_push_descriptor
    VK_KHR_8bit_storage
    VK_KHR_create_renderpass2
    VK_KHR_depth_stencil_resolve
    VK_KHR_draw_indirect_count
    VK_KHR_driver_properties
    
    VK_NV_compute_shader_derivatives
    VK_NV_cooperative_matrix
    VK_NV_corner_sampled_image
    VK_NV_dedicated_allocation_image_aliasing
    VK_NV_mesh_shader
    VK_NV_ray_tracing
    VK_NV_representative_fragment_test
    VK_NV_shading_rate_image
    VK_NV_viewport_array2
    VK_NV_viewport_swizzle
    
    VK_NVX_device_generated_commands
    VK_NVX_raytracing
    
    VK_EXT_buffer_device_address
    VK_EXT_conservative_rasterization
    VK_EXT_descriptor_indexing
    VK_EXT_depth_clip_enable
    VK_EXT_memory_budget
    VK_EXT_memory_priority
    VK_EXT_pci_bus_info
    VK_EXT_sample_locations
    VK_EXT_sampler_filter_minmax
    
    
    ]])
end