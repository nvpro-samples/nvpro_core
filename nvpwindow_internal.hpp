/*-----------------------------------------------------------------------
* Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
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

#include "nvpwindow.hpp"

#ifdef WIN32
#include "windows.h"
#endif

struct NVPWindowInternal
{
  NVPWindow * m_win;
  //NVTL: std::string m_deviceName;

#ifdef WIN32
  HDC   m_hDC;
  HWND  m_hWnd;
  // is used to store the windows position and size when switching to fullscreen
  // when switching back to windowed mode, the stored rect is applied again
  RECT  m_windowedRect;
#else
  int m_screen;
  GLXContext m_glx_context;
  GLXFBConfig m_glx_fb_config;
  Display *m_dpy;
  Window m_window;
  XVisualInfo *m_visual;
  XF86VidModeModeInfo m_mode;
  XSetWindowAttributes winAttributes;
#endif

  bool  m_iconified;
  bool  m_visible;

  NVPWindowInternal(NVPWindow *win)
    : m_win(win)
#ifdef WIN32
    , m_hDC(NULL)
    , m_hWnd(NULL)
#endif
#ifdef LINUX
    , m_dpy(NULL)
    , m_visual(NULL)
    , m_screen(0)
#endif
    , m_iconified(false)
    , m_visible(true)
  { }

  ~NVPWindowInternal() {}

  bool create(int xPos, int yPos, int width, int height, const char* title);
  void destroy();

  void screenshot(const char* filename);
  void clear(uint32_t r, uint32_t g, uint32_t b);
  void setFullScreen(bool bYes);
  void setTitle( const char* title );
  void maximize();
  void restore();
  void minimize();
  void setWindowPos(int x, int y, int w, int h);

  static bool sysPollEvents();
  static void sysWaitEvents();
  static void sysPostQuit();
  static double sysGetTime();
  static void sysSleep(double seconds);
  static void sysInit();
  static void sysDeinit();
};

extern NVPWindowInternal* newWINinternal(NVPWindow *win);


