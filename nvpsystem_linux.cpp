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

std::string NVPSystem::windowOpenFileDialog(struct GLFWwindow* glfwin, const char* title, const char* exts)
{
  Window hwnd = glfwGetX11Window(glfwin);
  assert(0 && "not yet implemented");

  return std::string();
}

std::string NVPSystem::windowSaveFileDialog(struct GLFWwindow* glfwin, const char* title, const char* exts)
{
  Window hwnd = glfwGetX11Window(glfwin);
  assert(0 && "not yet implemented");

  return std::string();
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

static std::string s_exePath;
static bool        s_exePathInit = false;

std::string NVPSystem::exePath()
{
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
