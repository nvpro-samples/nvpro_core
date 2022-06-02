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
 //--------------------------------------------------------------------

#include "nvpsystem.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

#include <vector>
#include <algorithm>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <string>
#include <assert.h>

#ifdef NVP_SUPPORTS_SOCKETS
#include "socketSampleMessages.h"
#endif

#include "linux_file_dialog.h"

// from https://docs.microsoft.com/en-us/windows/desktop/gdi/capturing-an-image


void NVPSystem::windowScreenshot(struct GLFWwindow* glfwin, const char* filename)
{
  Window hwnd = glfwGetX11Window(glfwin);
  assert(0 && "not yet implemented");
}

void NVPSystem::windowClear(struct GLFWwindow* glfwin, uint32_t r, uint32_t g, uint32_t b)
{
  Window hwnd = glfwGetX11Window(glfwin);
  assert(0 && "not yet implemented");
}

static void fixSingleFilter(std::string* pFilter);

static std::vector<std::string> toFilterArgs(const char* exts)
{
  // Convert exts list to filter format recognized by portable-file-dialogs
  // Try to match implemented nvpsystem on Windows behavior:
  // Alternate between human-readable descriptions and filter strings.
  // | separates strings
  // ; separates filters e.g. .png|.gif
  // Case-insensitive e.g. .png = .PNG = .pNg

  // Split strings by |
  std::vector<std::string> filterArgs(1);
  for(const char* pC = exts; pC != nullptr && *pC != '\0'; ++pC)
  {
    char c = *pC;
    if(c == '|')
      filterArgs.emplace_back();
    else
      filterArgs.back().push_back(c);
  }

  // Default arguments
  if(filterArgs.size() < 2)
  {
    filterArgs = {"All files", "*"};
  }

  // Split filters by ; and fix those filters.
  for(size_t i = 1; i < filterArgs.size(); i += 2)
  {
    std::string& arg = filterArgs[i];
    std::string  newArg;
    std::string  singleFilter;
    for(char c : arg)
    {
      if(c == ';')
      {
        fixSingleFilter(&singleFilter);
        newArg += std::move(singleFilter);
        singleFilter.clear();
        newArg += ' ';  // portable-file-dialogs uses spaces to separate... win32 wants no spaces in filters at all so presumably this is fine.
      }
      else
      {
        singleFilter.push_back(c);
      }
    }
    fixSingleFilter(&singleFilter);
    newArg += std::move(singleFilter);
    arg = std::move(newArg);
  }

  return filterArgs;
}

static void fixSingleFilter(std::string* pFilter)
{
  // Make case insensitive.
  std::string newFilter;
  for(char c : *pFilter)
  {
    if(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))
    {
      // replace c with [cC] to make case-insensitive. TODO: Unicode support, not sure how to implement for multibyte utf-8 characters.
      newFilter.push_back('[');
      newFilter.push_back(c);
      newFilter.push_back(char(c ^ 32));
      newFilter.push_back(']');
    }
    else
    {
      newFilter.push_back(c);
    }
  }
  *pFilter = std::move(newFilter);
}

std::string NVPSystem::windowOpenFileDialog(struct GLFWwindow* glfwin, const char* title, const char* exts)
{
  // Not sure yet how to use this; maybe make as a child window somehow?
  [[maybe_unused]] Window hwnd = glfwGetX11Window(glfwin);

  std::vector<std::string> filterArgs   = toFilterArgs(exts);
  std::vector<std::string> resultVector = open_file(title, ".", filterArgs).result();
  assert(resultVector.size() <= 1);
  return resultVector.empty() ? "" : std::move(resultVector[0]);
}

std::string NVPSystem::windowSaveFileDialog(struct GLFWwindow* glfwin, const char* title, const char* exts)
{
  // Not sure yet how to use this; maybe make as a child window somehow?
  [[maybe_unused]] Window hwnd = glfwGetX11Window(glfwin);

  std::vector<std::string> filterArgs = toFilterArgs(exts);
  return save_file(title, ".", filterArgs).result();
}

void NVPSystem::sleep(double seconds)
{
  ::sleep(seconds);
}

void NVPSystem::platformInit()
{
}

void NVPSystem::platformDeinit()
{
}

static bool        s_exePathInit = false;

std::string NVPSystem::exePath()
{
  static std::string s_exePath;

  if(!s_exePathInit)
  {
    char modulePath[PATH_MAX];
    ssize_t modulePathLength = readlink( "/proc/self/exe", modulePath, PATH_MAX );

    s_exePath = std::string(modulePath, modulePathLength > 0 ? modulePathLength : 0);

    size_t last = s_exePath.rfind('/');
    if(last != std::string::npos)
    {
      s_exePath = s_exePath.substr(0, last) + std::string("/");
    }

    s_exePathInit = true;
  }

  return s_exePath;
}
