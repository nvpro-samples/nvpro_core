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
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <commdlg.h>
#include <windows.h>
#include <windowsx.h>

#include "resources.h"

#include <algorithm>
#include <io.h>
#include <stdio.h>
#include <string>
#include <vector>

// Samples include their own definitions of stb_image. Use STB_IMAGE_WRITE_STATIC to avoid issues with multiple
// definitions in the nvpro_core static lib at the cost of having the code exist multiple times.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include <stb_image_write.h>
#include <memory>

// Executables (but not DLLs) exporting this symbol with this value will be
// automatically directed to the high-performance GPU on Nvidia Optimus systems
// with up-to-date drivers
//
extern "C" {
_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

// from https://docs.microsoft.com/en-us/windows/desktop/gdi/capturing-an-image

static int CaptureAnImage(HWND hWnd, const char* filename)
{
  struct LocalResources
  {
    LocalResources(HWND _hWnd)
        : hWnd(_hWnd)
    {
    }
    ~LocalResources()
    {
      if(hMemoryDC != nullptr && hOldBitmap != nullptr)
        SelectObject(hMemoryDC, hOldBitmap);
      if(hBitmap != nullptr)
        DeleteObject(hBitmap);
      if(hMemoryDC != nullptr)
        DeleteDC(hMemoryDC);
      if(hWnd != nullptr && hWindowDC != nullptr)
        ReleaseDC(hWnd, hWindowDC);
    }
    HDC     hMemoryDC  = nullptr;
    HDC     hWindowDC  = nullptr;
    HBITMAP hOldBitmap = nullptr;
    HBITMAP hBitmap    = nullptr;
    HWND    hWnd       = nullptr;
  };
  std::unique_ptr<LocalResources> res = std::make_unique<LocalResources>(hWnd);

  // Get the window's device context
  res->hWindowDC = GetDC(hWnd);
  if(res->hWindowDC == nullptr)
  {
    LOGE("Failed to Create Compatible Bitmap");
    return -1;
  }

  // Get the window's width and height
  RECT rect;
  if(!GetClientRect(res->hWnd, &rect))
  {
    LOGE("Failed to retrieves a handle to a device context");
    return -1;
  }

  // Create a compatible device context
  res->hMemoryDC = CreateCompatibleDC(res->hWindowDC);
  if(res->hMemoryDC == nullptr)
  {
    LOGE("Failed to creates a memory device context");
    return -1;
  }

  int width  = rect.right - rect.left;
  int height = rect.bottom - rect.top;

  // Create a bitmap and select it into the device context
  res->hBitmap = CreateCompatibleBitmap(res->hWindowDC, width, height);
  if(res->hBitmap == nullptr)
  {
    LOGE("Failed to creates a bitmap compatible with the device");
    return -1;
  }

  res->hOldBitmap = (HBITMAP)SelectObject(res->hMemoryDC, res->hBitmap);
  if(res->hOldBitmap == nullptr)
  {
    LOGE("Failed to selects an object into the specified device context");
    return -1;
  }

  // Copy the window's device context to the bitmap
  if(BitBlt(res->hMemoryDC, 0, 0, width, height, res->hWindowDC, 0, 0, SRCCOPY) == 0)
  {
    LOGE("Failed to bit-block transfer");
    return -1;
  }

  // Prepare the bitmap info header
  BITMAPINFOHEADER bi = {};
  bi.biSize           = sizeof(BITMAPINFOHEADER);
  bi.biWidth          = width;
  bi.biHeight         = -height;  // Negative height to ensure top-down orientation
  bi.biPlanes         = 1;
  bi.biBitCount       = 24;  // 24 bits per pixel (RGB)
  bi.biCompression    = BI_RGB;
  bi.biSizeImage      = 0;
  bi.biXPelsPerMeter  = 0;
  bi.biYPelsPerMeter  = 0;
  bi.biClrUsed        = 0;
  bi.biClrImportant   = 0;

  // Allocate memory for the bitmap data
  std::vector<uint8_t> pixels(width * height * 3);

  // Copy the bitmap data into the pixel buffer
  if(GetDIBits(res->hMemoryDC, res->hBitmap, 0, height, pixels.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS) == 0)
  {
    LOGE("Failed to retrieves the bits of the specified compatible bitmap");
    return -1;
  }

  // Convert BGR to RGB
  for(int i = 0; i < width * height; ++i)
  {
    uint8_t temp      = pixels[i * 3];
    pixels[i * 3]     = pixels[i * 3 + 2];
    pixels[i * 3 + 2] = temp;
  }

  // Save the bitmap as a PNG file using stb_image
  if(stbi_write_png(filename, width, height, 3, pixels.data(), width * 3) == 0)
  {
    LOGE("Failed to write: %s", filename);
    return -1;
  }

  return 0;
}


void NVPSystem::windowScreenshot(struct GLFWwindow* glfwin, const char* filename)
{
  if(!glfwin)
  {
    assert(!"Attempted to fall windowScreenshot() on null window!");
    return;
  }
  CaptureAnImage(glfwGetWin32Window(glfwin), filename);
}

void NVPSystem::windowClear(struct GLFWwindow* glfwin, uint32_t r, uint32_t g, uint32_t b)
{
  if(!glfwin)
  {
    assert(!"Attempted to fall windowClear() on null window!");
    return;
  }
  HWND hwnd = glfwGetWin32Window(glfwin);

  HDC hdcWindow = GetDC(hwnd);

  RECT rcClient;
  GetClientRect(hwnd, &rcClient);
  HBRUSH hbr = CreateSolidBrush(RGB(r, g, b));

  FillRect(hdcWindow, &rcClient, hbr);

  ReleaseDC(hwnd, hdcWindow);
  DeleteBrush(hbr);
}

static std::string fileDialog(struct GLFWwindow* glfwin, const char* title, const char* exts, bool openToLoad)
{
  if(!glfwin)
  {
    assert(!"Attempted to fall fileDialog() on null window!");
    return std::string();
  }
  HWND hwnd = glfwGetWin32Window(glfwin);

  std::vector<char> extsfixed;
  for(size_t i = 0; i < strlen(exts); i++)
  {
    if(exts[i] == '|')
    {
      extsfixed.push_back(0);
    }
    else
    {
      extsfixed.push_back(exts[i]);
    }
  }
  extsfixed.push_back(0);
  extsfixed.push_back(0);

  OPENFILENAME ofn;             // common dialog box structure
  char         szFile[1024]{};  // buffer for file name

  // Initialize OPENFILENAME
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner   = hwnd;
  ofn.lpstrFile   = szFile;
  // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
  // use the contents of szFile to initialize itself.
  ofn.lpstrFile[0]    = '\0';
  ofn.nMaxFile        = sizeof(szFile);
  ofn.lpstrFilter     = extsfixed.data();
  ofn.nFilterIndex    = 1;
  ofn.lpstrFileTitle  = NULL;
  ofn.nMaxFileTitle   = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags           = OFN_PATHMUSTEXIST;
  ofn.lpstrTitle      = title;

  // Display the Open dialog box.

  if(openToLoad)
  {
    ofn.Flags |= OFN_FILEMUSTEXIST;
    if(GetOpenFileNameA(&ofn) == TRUE)
    {
      return ofn.lpstrFile;
    }
  }
  else
  {
    ofn.Flags |= OFN_OVERWRITEPROMPT;
    if(GetSaveFileNameA(&ofn) == TRUE)
    {
      return ofn.lpstrFile;
    }
  }

  return std::string();
}

std::string NVPSystem::windowOpenFileDialog(struct GLFWwindow* glfwin, const char* title, const char* exts)
{
  return fileDialog(glfwin, title, exts, true);
}

std::string NVPSystem::windowSaveFileDialog(struct GLFWwindow* glfwin, const char* title, const char* exts)
{
  return fileDialog(glfwin, title, exts, false);
}

void NVPSystem::sleep(double seconds)
{
  ::Sleep(DWORD(seconds * 1000.0));
}

void NVPSystem::platformInit()
{
#ifdef MEMORY_LEAKS_CHECK
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
#endif
}

void NVPSystem::platformDeinit()
{
  // Because we set _CRTDBG_LEAK_CHECK_DF in platformInit(), we don't need to
  // call _CrtDumpMemoryLeaks() in platformDeinit(). If we did, then we might
  // see allocations for static objects that haven't been destroyed yet.
}

static std::string s_exePath;
static bool        s_exePathInit = false;

std::string NVPSystem::exePath()
{
  if(!s_exePathInit)
  {
    char   modulePath[MAX_PATH];
    size_t modulePathLength = GetModuleFileNameA(NULL, modulePath, MAX_PATH);
    s_exePath               = std::string(modulePath, modulePathLength);

    std::replace(s_exePath.begin(), s_exePath.end(), '\\', '/');
    size_t last = s_exePath.rfind('/');
    if(last != std::string::npos)
    {
      s_exePath = s_exePath.substr(0, last) + std::string("/");
    }

    s_exePathInit = true;
  }

  return s_exePath;
}
