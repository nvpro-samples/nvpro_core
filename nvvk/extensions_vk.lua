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

-- HOW TO USE
--
-- Modify the whitelist of extensions you may want to use.
-- Only those extensions that have functions will get a loader
-- (load_VK_BLAH_whatever) so not all extensions in this list
-- may get a function in the end.
--
-- check out this and the other extensions_vk files for write access
-- run with any lua5.1 compatible lua runtime (there is one shared_internal):
--
--    lua extensions_vk.lua
--
-- within this directory. It will look for the VULKAN_SDK environment variable
-- which the VulkanSDK installer sets.

local extensionSubset = [[
    VK_KHR_push_descriptor
    VK_KHR_8bit_storage
    VK_KHR_create_renderpass2
    VK_KHR_depth_stencil_resolve
    VK_KHR_draw_indirect_count
    VK_KHR_driver_properties
    VK_KHR_pipeline_executable_properties
    
    VK_NV_compute_shader_derivatives
    VK_NV_cooperative_matrix
    VK_NV_corner_sampled_image
    VK_NV_coverage_reduction_mode
    VK_NV_dedicated_allocation_image_aliasing
    VK_NV_mesh_shader
    VK_NV_ray_tracing
    VK_NV_representative_fragment_test
    VK_NV_shading_rate_image
    VK_NV_viewport_array2
    VK_NV_viewport_swizzle
    VK_NV_scissor_exclusive
    
    VK_NVX_device_generated_commands
    
    VK_EXT_buffer_device_address
    VK_EXT_debug_marker
    VK_EXT_calibrated_timestamps
    VK_EXT_conservative_rasterization
    VK_EXT_descriptor_indexing
    VK_EXT_depth_clip_enable
    VK_EXT_memory_budget
    VK_EXT_memory_priority
    VK_EXT_pci_bus_info
    VK_EXT_sample_locations
    VK_EXT_sampler_filter_minmax
    VK_EXT_texel_buffer_alignment
    VK_EXT_debug_utils
    VK_EXT_host_query_reset
    
    VK_KHR_external_memory_win32
    VK_KHR_external_semaphore_win32
    VK_KHR_external_fence_win32
    ]]

local function generate(outfilename, header, whitelist)
  
  local function extractFeatureDefs(features, filename, whitelist, api, apientry, platform)
    local f = io.open(filename,"rt")
    local str = f:read("*a")
    
    print("looking for whitelisted extensions:")
    
    local version = str:match("VK_HEADER_VERSION%s+(%d+)")
    
    -- for the pattern to work add a dummy end
    str = str.."\n\n#define VK_DUMMY_TERMINATOR_SPEC_VERSION 1\n"
    
    -- not the best pattern since we will not catch the first extension (VK_KHR_surface) but we don't need to
    for feature,defstr in str:gmatch('#define VK_[%w_]+_EXTENSION_NAME%s+"([%w_]+)"\n(.-)#define VK_[%w_]+_SPEC_VERSION%s+%d+\n') do
      if (not whitelist or whitelist[feature]) then
        --print(defstr)
        local defs = ""
        for def in defstr:gmatch("#ifndef VK_NO_PROTOTYPES(.-)#endif\n") do
          def = def:gsub(api, ""):gsub(apientry, "")
          defs = defs..def
        end
        print(feature, "loader:", defs ~= "")
        
        table.insert(features, {feature=feature, defs=defs, platform=platform})
      end
    end
    
    f:close()
    
    return version
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
  
  local vk_features = {}
  local vk_version = extractFeatureDefs(vk_features, sdkpath.."/Include/vulkan/vulkan_core.h", whitelist, "VKAPI_ATTR ", "VKAPI_CALL ")
                     extractFeatureDefs(vk_features, sdkpath.."/Include/vulkan/vulkan_win32.h", whitelist, "VKAPI_ATTR ", "VKAPI_CALL ", "VK_USE_PLATFORM_WIN32_KHR")
  
  local availables_header = ""
  local availables_source = ""
  
  local loaders_header = ""
  local loaders_source = ""
  local loaders_all = ""
  local reset_all = ""
  
  local function process(f, api, apientry) 
    local defs = "\n"..f.defs:gsub("%s*%*%s*","%* ")
    
    local func      = ""
    local initfunc  = ""
    local success   = ""
    local variable  = ""
    local reset     = ""
    
    for const,ret,name,arg in defs:gmatch("\n([%w_]-%s*)([%w_%*]+)%s+([%w_]+)%s*(%b());") do
      
      local prot = "PFN_"..name
      local mangled = "pfn_"..name
      
      local exec = ret ~= "void" and "return " or ""
      const = const == "\n" and "" or const
      
      local isInstanceFunc = arg:match("%(%s*VkInstance%s+")
      
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
      reset    = reset.."  "..prot.." "..mangled.." = 0;\n"
      
      func = func..api..const..ret.." "..apientry..name..arg.."\n{\n"
      func = func.."  assert("..mangled..");\n"
      func = func.."  "..exec.."\n"
      func = func.."}\n"
      
      if (isInstanceFunc) then
        initfunc  = initfunc.."  "..mangled..' = ('..prot..')getInstanceProcAddr(instance, "'..name..'");\n'
      else
        initfunc  = initfunc.."  "..mangled..' = ('..prot..')getDeviceProcAddr(device, "'..name..'");\n'
      end
      success  = success.."  success = success && ("..mangled..' != 0);\n'
    end
    
    if (initfunc ~= "")then
      local preproc_condition = "#if "..f.feature..(f.platform and " && defined("..f.platform..")" or "").."\n"
      
      loaders_header = loaders_header..preproc_condition
      loaders_header = loaders_header.."int load_"..f.feature.."(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr);\n"
      loaders_header = loaders_header.."extern int has_"..f.feature..";\n"
      loaders_header = loaders_header.."#endif\n\n"
      
      loaders_source = loaders_source.."/* /////////////////////////////////// */\n"
      loaders_source = loaders_source..preproc_condition
      loaders_source = loaders_source..variable.."\n"
      loaders_source = loaders_source..func.."\n"
      loaders_source = loaders_source.."int has_"..f.feature.." = 0;\n"
      loaders_source = loaders_source.."int load_"..f.feature.."(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr)\n{\n"
      loaders_source = loaders_source..initfunc
      loaders_source = loaders_source.."  int success = 1;\n"
      loaders_source = loaders_source..success
      loaders_source = loaders_source.."  has_"..f.feature.." = success;\n"
      loaders_source = loaders_source.."  return success;\n}\n"
      loaders_source = loaders_source.."#endif\n\n"
      
      loaders_all = loaders_all.."  "..preproc_condition
      loaders_all = loaders_all.."  load_"..f.feature.."(instance, getInstanceProcAddr, device, getDeviceProcAddr);\n"
      loaders_all = loaders_all.."  #endif\n"
      
      reset_all = reset_all.."  "..preproc_condition
      reset_all = reset_all.."  has_"..f.feature.." = 0;\n"
      reset_all = reset_all..reset
      reset_all = reset_all.."  #endif\n\n"
    end
  end
  
  for i,f in ipairs(vk_features) do
    process(f, "VKAPI_ATTR ", "VKAPI_CALL ")
  end
    
  local fheader = io.open(outfilename..".hpp", "wt")
  local fsource = io.open(outfilename..".cpp", "wt")
  assert(fheader, "could not open "..outfilename..".hpp for writing")
  assert(fsource, "could not open "..outfilename..".cpp for writing")
    
fheader:write("/* based on VK_HEADER_VERSION "..vk_version.." */\n"..header..[[

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

//////////////////////////////////////////////////////////////////////////
/**
  # Vulkan Extension Loader

  The extensions_vk files takes care of loading and providing the symbols of
  Vulkan C Api extensions.
  It is generated by `extensions_vk.lua` which contains a whitelist of
  extensions to be made available.

  The framework triggers this implicitly in the `nvvk::Context` class.
  
  If you want to use it in your own code, see the instructions in the 
  lua file how to generate it.

  ~~~ c++
    // loads all known extensions
    load_VK_EXTENSION_SUBSET(instance, vkGetInstanceProcAddr, device, vkGetDeviceProcAddr);

    // load individual extension
    load_VK_KHR_push_descriptor(instance, vkGetInstanceProcAddr, device, vkGetDeviceProcAddr);
  ~~~

*/

#pragma once

#include <vulkan/vulkan.h>

/* super load/reset */
void load_VK_EXTENSION_SUBSET(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr);
void reset_VK_EXTENSION_SUBSET();

/* loaders */
]]..loaders_header..[[

]])

fsource:write("/* based on VK_HEADER_VERSION "..vk_version.." */\n"..header..[[


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

#include <assert.h>
#include "]]..outfilename..[[.hpp"

/* loaders */
]]..loaders_source..[[

/* super load/reset */
void load_VK_EXTENSION_SUBSET(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr) {
]]..loaders_all..[[
}
void reset_VK_EXTENSION_SUBSET() {
]]..reset_all..[[
}

]])




end

do
  generate("extensions_vk",
    "/* auto generated by extensions_vk.lua */",
    extensionSubset)
end