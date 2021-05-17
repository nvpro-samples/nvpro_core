--[[
/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

]]

--
-- run in this directory
-- ..\shared_internal\luajit\win_x64\luajit.exe docgen.lua

local lfs = require "lfs"

function extractSymbols(file, symbols)
  local f  = io.open(file, "rt")
  local tx = f:read("*a")
  f:close()
  
  local tx = string.gsub(tx, "\r\n", "\n")
  tx = tx:gsub("//%*[^\n]*",""):gsub("/%*(.-)%*/",function(str) return str:gsub("[^\n]","") end):gsub("//[^\n]*","")

  for class in tx:gmatch("struct%s+([%w_]+)%s+[{:]") do
    symbols[class] = class
  end
  for class in tx:gmatch("class%s+([%w_]+)%s+[{:]") do
    symbols[class] = class
  end
end

function buildSymbols(path, symbols)
  for file in lfs.dir(path) do
    if file ~= "." and file ~= ".." then
      local f = path..'/'..file
      local attr = lfs.attributes (f)
      assert (type(attr) == "table")
      if attr.mode ~= "directory" and file:lower():match("%.hp?p?$")then
        extractSymbols(f, symbols)
      end
    end
  end
end

function extractComment(file, filename, symbols, toc)
  local f = io.open(file, "rt")
  local str = f:read("*a")
  f:close()
  
  local function asLink(txt)
    return "(#"..txt:gsub("#+%s+",""):gsub("%s+","-"):gsub("[^%w_%-]",""):lower()..")"
  end
  
  local out = ""
  table.insert(toc, "- ["..filename..":]"..asLink(filename))
  
  for cap in str:gmatch("/%*%*%s*\n(.-)%s+%*/") do
    -- find original indentation in first content line
    local function removeIndentation(txt)
      local leading = txt:match("^%s*")
      -- remove indentation
      txt = "\n"..txt
      txt = txt:gsub("\n"..leading,"\n")
      return txt
    end
    
    cap = removeIndentation(cap)
    
    local classdef = cap:match("(#+%s+class%s+[^\n]+)\n")
    if (classdef) then
      local class = classdef:match("#+%s+class%s+(.+)")
      --print(class)
      table.insert(toc, "  - class ["..class.."]"..asLink(classdef))
    end
    
    -- find inline code sections and rebase indentation as well
    -- also preserve those sections from further formating
    local preserved = {}
    local function preserve(txt)
      table.insert(preserved, txt)
      return "$$"..#preserved
    end
    
    local function rebaseCode(pre,code,post)
      code = removeIndentation(code)
      return preserve(pre..code..post)
    end
    
    cap = cap:gsub("(~~~+[^\n]*)\n(.-)(\n%s*~~~+)", rebaseCode)
    cap = cap:gsub("(```+[^\n]*)\n(.-)(\n%s*```+)", rebaseCode)
    
    cap = cap:gsub("(`[%w+][^`]-`)", preserve)
    
    -- find special api symbol names and mark them with ` sym `
    local function markApiSymbol(pre,sym,post)
      return pre.."`"..sym.."`"..post
    end
    
    local function markListSymbol(pre,sym,post)
      return pre.."**"..sym.."**"..post
    end
    
    local function markInternalSymbol(pre,sym)
      local name = sym:match("[%w_]+$")
      --if name and not symbols[name] then
      --  print(name)
      --end
      if name and (symbols[name] or symbols[name:sub(1,-2)]) then
        return pre.."**"..sym.."**"
      else
        return nil
      end
    end
    
    -- VkBlah or vkBlah
    cap = cap:gsub("(%s)([Vv]k[A-Z][%w_]+)([%s,;%.!%?])",markApiSymbol)
    -- vk::Blah
    cap = cap:gsub("(%s)(vk::[%w_]+)([%s,;%.!%?])",markApiSymbol)
    
    -- find special function lists
    --   - symbol : description text
    cap = cap:gsub("(\n%s*%-%s*)([%w_]+)( : )",markListSymbol)
    
    -- find regular symbols and flag them bold
    cap = cap:gsub("([%s,;%.]+)([%w][%w_:]+)", markInternalSymbol)
      
    -- add header depth + 2
    cap = cap:gsub("(\n%s*)(#+)", function(pre, depth)
        return pre..depth.."##"
        end)


    -- bring back preserved sections
    cap = cap:gsub("$$(%d+)", function(d) return preserved[tonumber(d)] end)
    
    out = out..cap.."\n\n"
  end
  if (out ~= "") then
    print("found",filename)
  else
    table.remove(toc)
  end
  return out ~= "" and "## "..filename.."\n"..out or ""
end

function generateReadme(path, about, symbols, xtra)
  local str = ""
  local toc = {"Table of Contents:"}
  
  for file in lfs.dir(path) do
    if file ~= "." and file ~= ".." then
      local f = path..'/'..file
      local attr = lfs.attributes (f)
      assert (type(attr) == "table")
      if attr.mode ~= "directory" and file:lower():match("%.hp?p?$")then
        str = str..extractComment(f, file, symbols, toc)
      end
    end
  end
  
  if (str ~= "") then
    local out = "# "..about.."\n\n"
    out = out.."Non-exhaustive list of utilities provided in the `"..path.."` directory\n\n"
    out = out..(xtra or "")
    out = out..table.concat(toc, "\n").."\n\n"
    out = out.."_____\n\n"
    out = out..str.."\n\n\n"
    out = out.."_____\n"
    out = out.."auto-generated by `docgen.lua`\n"
    local fname = path.."/README.md"
    local f = io.open(fname, "wt")
    assert(f, "could not open file for write access: "..fname)
    f:write(out)
    f:close()
    print("update", fname)
  end
end

local symbols = {}
buildSymbols("nvh", symbols)
buildSymbols("nvvk", symbols)
buildSymbols("nvgl", symbols)

generateReadme("nvh", "Generic Helpers", symbols)
generateReadme("nvvk", "Vulkan Api Helpers", symbols, "If you intend to use the Vulkan C++ api, include <vulkan/vulkan.hpp> before including the helper files.\n\n")
generateReadme("nvgl", "OpenGL Api Helpers", symbols)