/*-----------------------------------------------------------------------
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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
*/ //--------------------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include "resources.h"

#include "nvpwindow_internal.hpp"

#ifdef USESOCKETS
#include "socketSampleMessages.h"
#endif

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

extern "C" { _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; }

extern std::vector<NVPWindow *> g_windows;

HINSTANCE   g_hInstance = 0;
MSG  uMsg;

LRESULT CALLBACK WindowProc( HWND   m_hWnd, 
                             UINT   msg, 
                             WPARAM wParam, 
                             LPARAM lParam );

//------------------------------------------------------------------------------
// creation of generic window's internal info
//------------------------------------------------------------------------------
NVPWindowInternal* newWINinternal(NVPWindow *win)
{
    return new NVPWindowInternal(win);
}
//------------------------------------------------------------------------------
// destruction generic to win32
// put here because it can be shared between any API (OpenGL, Vulkan...)
//------------------------------------------------------------------------------
void NVPWindowInternal::destroy()
{
    if( m_hDC != NULL )
    {
      ReleaseDC(m_hWnd, m_hDC);
      m_hDC = NULL;
    }
    m_hWnd = NULL;
}
//------------------------------------------------------------------------------
// creation is generic to win32
// put here because it can be shared between any API (OpenGL, Vulkan...)
//------------------------------------------------------------------------------
bool NVPWindowInternal::create(int xPos, int yPos, int width, int height, const char* title)
{
  WNDCLASSEX winClass;

  winClass.lpszClassName = "MY_WINDOWS_CLASS";
  winClass.cbSize        = sizeof(WNDCLASSEX);
  winClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
  winClass.lpfnWndProc   = WindowProc;
  winClass.hInstance     = g_hInstance;
  winClass.hIcon           = LoadIcon(g_hInstance, (LPCTSTR)IDI_OPENGL_ICON);
  winClass.hIconSm       = LoadIcon(g_hInstance, (LPCTSTR)IDI_OPENGL_ICON);
  winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  winClass.lpszMenuName  = NULL;
  winClass.cbClsExtra    = 0;
  winClass.cbWndExtra    = 0;

  if(!RegisterClassEx(&winClass) )
    return false;

  DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;
  DWORD styleEx = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

  RECT rect = { 0, 0, width, height };
  AdjustWindowRectEx(&rect, style,
    FALSE, styleEx);

  m_hWnd = CreateWindowEx( styleEx, "MY_WINDOWS_CLASS",
    title ? title : "Viewer",
    style, xPos, yPos, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, 
    g_hInstance, (LPVOID)NULL );
  winClass.lpszClassName = "DUMMY";
  winClass.lpfnWndProc   = DefWindowProc;
  if(!RegisterClassEx(&winClass) )
    return false;

  if( m_hWnd == NULL )
    return false;

  SetWindowLongPtr(m_hWnd, GWLP_USERDATA, g_windows.size());
  UpdateWindow(m_hWnd);
  ShowWindow(m_hWnd, SW_SHOW);
  return true;
}

// from https://docs.microsoft.com/en-us/windows/desktop/gdi/capturing-an-image

static int CaptureAnImage(HWND hWnd, const char* filename)
{
  HDC hdcScreen;
  HDC hdcWindow;
  HDC hdcMemDC = NULL;
  HBITMAP hbmScreen = NULL;
  BITMAP bmpScreen;

  // Retrieve the handle to a display device context for the client 
  // area of the window. 
  hdcScreen = GetDC(NULL);
  hdcWindow = GetDC(hWnd);

  // Create a compatible DC which is used in a BitBlt from the window DC
  hdcMemDC = CreateCompatibleDC(hdcWindow);

  if (!hdcMemDC)
  {
    LOGE("CreateCompatibleDC has failed\n");
    goto done;
  }

  // Get the client area for size calculation
  RECT rcClient;
  GetClientRect(hWnd, &rcClient);

  //This is the best stretch mode
  SetStretchBltMode(hdcWindow, HALFTONE);

  //The source DC is the entire screen and the destination DC is the current window (HWND)
  if (!StretchBlt(hdcWindow,
    0, 0,
    rcClient.right, rcClient.bottom,
    hdcScreen,
    0, 0,
    GetSystemMetrics(SM_CXSCREEN),
    GetSystemMetrics(SM_CYSCREEN),
    SRCCOPY))
  {
    LOGE("StretchBlt has failed\n");
    goto done;
  }

  // Create a compatible bitmap from the Window DC
  hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

  if (!hbmScreen)
  {
    LOGE("CreateCompatibleBitmap Failed\n");
    goto done;
  }

  // Select the compatible bitmap into the compatible memory DC.
  SelectObject(hdcMemDC, hbmScreen);

  // Bit block transfer into our compatible memory DC.
  if (!BitBlt(hdcMemDC,
    0, 0,
    rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
    hdcWindow,
    0, 0,
    SRCCOPY))
  {
    LOGE ("BitBlt has failed\n");
    goto done;
  }

  // Get the BITMAP from the HBITMAP
  GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

  BITMAPFILEHEADER   bmfHeader;
  BITMAPINFOHEADER   bi;

  bi.biSize = sizeof(BITMAPINFOHEADER);
  bi.biWidth = bmpScreen.bmWidth;
  bi.biHeight = bmpScreen.bmHeight;
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;

  DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

  // Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
  // call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
  // have greater overhead than HeapAlloc.
  HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
  char *lpbitmap = (char *)GlobalLock(hDIB);

  // Gets the "bits" from the bitmap and copies them into a buffer 
  // which is pointed to by lpbitmap.
  GetDIBits(hdcWindow, hbmScreen, 0,
    (UINT)bmpScreen.bmHeight,
    lpbitmap,
    (BITMAPINFO *)&bi, DIB_RGB_COLORS);

  // A file is created, this is where we will save the screen capture.
  HANDLE hFile = CreateFileA(filename,
    GENERIC_WRITE,
    0,
    NULL,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL, NULL);

  // Add the size of the headers to the size of the bitmap to get the total file size
  DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

  //Offset to where the actual bitmap bits start.
  bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

  //Size of the file
  bmfHeader.bfSize = dwSizeofDIB;

  //bfType must always be BM for Bitmaps
  bmfHeader.bfType = 0x4D42; //BM   

  DWORD dwBytesWritten = 0;
  WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
  WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
  WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);
#ifdef USESOCKETS
    // TODO!!
    unsigned char* data = NULL;
    size_t sz = 0;
    int w = 0, h = 0;
    ::postScreenshot(data, sz, w,h);
#endif

  //Unlock and Free the DIB from the heap
  GlobalUnlock(hDIB);
  GlobalFree(hDIB);

  //Close the handle for the file that was created
  CloseHandle(hFile);

  //Clean up
done:
  DeleteObject(hbmScreen);
  DeleteObject(hdcMemDC);
  ReleaseDC(NULL, hdcScreen);
  ReleaseDC(hWnd, hdcWindow);

  return 0;
}

void NVPWindowInternal::screenshot(const char* filename)
{
  CaptureAnImage(m_hWnd, filename);
}

void NVPWindowInternal::setFullScreen(bool bYes)
{
    HWND window = m_hWnd;

    if(!window)
        return;

    if(bYes)
    {
        if(!m_win->isFullScreen())
        {
            // if not fullscreen save old rect
            GetWindowRect(window, &m_windowedRect);
        }

        // remove border and caption from window
        LONG_PTR windowStyle = GetWindowLongPtr(window, GWL_STYLE);
        windowStyle &= ~WS_BORDER;
        windowStyle &= ~WS_CAPTION;
        windowStyle &= ~WS_SIZEBOX;
        SetWindowLongPtr(window, GWL_STYLE, windowStyle);

        // get monitor for window
        HMONITOR    windowMonitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(windowMonitor, &monitorInfo);

        // make window screen/monitor sized and always on top
        int x = monitorInfo.rcMonitor.left;
        int y = monitorInfo.rcMonitor.top;
        int w = monitorInfo.rcMonitor.right - x;
        int h = monitorInfo.rcMonitor.bottom - y;
        SetWindowPos(window, HWND_TOPMOST, x, y, w, h, SWP_SHOWWINDOW);
    }
    else
    {
        LONG_PTR windowStyle = GetWindowLongPtr(window, GWL_STYLE);
        // put back border and caption
        windowStyle |= WS_BORDER | WS_CAPTION | WS_SIZEBOX;
        SetWindowLongPtr(window, GWL_STYLE, windowStyle);
        // reset window to old rect and not always on top
        int x = m_windowedRect.left;
        int y = m_windowedRect.top;
        int w = m_windowedRect.right - x;
        int h = m_windowedRect.bottom - y;
        SetWindowPos(window, HWND_NOTOPMOST, x, y, w, h, SWP_SHOWWINDOW);
    }
}

void NVPWindowInternal::setTitle( const char* title )
{
    SetWindowTextA(m_hWnd, title);
}
void NVPWindowInternal::maximize()
{
  ShowWindow(m_hWnd, SW_MAXIMIZE);
}

void NVPWindowInternal::restore()
{
  ShowWindow(m_hWnd, SW_RESTORE);
}

void NVPWindowInternal::minimize()
{
  ShowWindow(m_hWnd, SW_MINIMIZE);
}

void NVPWindowInternal::setWindowPos(int x, int y, int w, int h)
{
    SetWindowPos(m_hWnd, NULL, x,y,w,h, 0);
}
bool NVPWindowInternal::sysPollEvents()
{
    if( PeekMessage( &uMsg, NULL, 0, 0, PM_REMOVE ) )
    { 
        TranslateMessage( &uMsg );
        DispatchMessage( &uMsg );
    }
    return uMsg.message != WM_QUIT;
}

void NVPWindowInternal::sysWaitEvents()
{
    WaitMessage();
    sysPollEvents();
}

void NVPWindowInternal::sysPostQuit()
{
  PostQuitMessage(0);
}

static double s_frequency;
double NVPWindowInternal::sysGetTime()
{
    LARGE_INTEGER time;
    if (QueryPerformanceCounter(&time)){
      return (double(time.QuadPart) / s_frequency);
    }
    return 0;
}

void NVPWindowInternal::sysSleep(double seconds)
{
  Sleep(DWORD(seconds * 1000.0));
}

void NVPWindowInternal::sysInit()
{
  g_hInstance = GetModuleHandle(NULL);

  memset(&uMsg,0,sizeof(uMsg));

  LARGE_INTEGER sysfrequency;
  if (QueryPerformanceFrequency(&sysfrequency)){
    s_frequency = (double)sysfrequency.QuadPart;
  }
  else{
    s_frequency = 1;
  }

  //initNSight();
#ifdef MEMORY_LEAKS_CHECK
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); 
  _CrtSetReportMode ( _CRT_ERROR, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_WNDW);
#endif
}
void NVPWindowInternal::sysDeinit()
{
  UnregisterClass( "MY_WINDOWS_CLASS", g_hInstance );

#ifdef MEMORY_LEAKS_CHECK
  _CrtDumpMemoryLeaks();
#endif
}

static int getKeyMods()
{
  int mods = 0;

  if (GetKeyState(VK_SHIFT) & (1 << 31))
    mods |= NVPWindow::KMOD_SHIFT;
  if (GetKeyState(VK_CONTROL) & (1 << 31))
    mods |= NVPWindow::KMOD_CONTROL;
  if (GetKeyState(VK_MENU) & (1 << 31))
    mods |= NVPWindow::KMOD_ALT;
  if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & (1 << 31))
    mods |= NVPWindow::KMOD_SUPER;

  return mods;
}

// Translates a Windows key to the corresponding GLFW key
//
#define INTERNAL_KEY_INVALID -2

static int translateKey(WPARAM wParam, LPARAM lParam)
{
  // Check for numeric keypad keys
  // NOTE: This way we always force "NumLock = ON", which is intentional since
  //       the returned key code should correspond to a physical location.
  if ((HIWORD(lParam) & 0x100) == 0)
  {
    switch (MapVirtualKey(HIWORD(lParam) & 0xFF, 1))
    {
    case VK_INSERT:   return NVPWindow::KEY_KP_0;
    case VK_END:      return NVPWindow::KEY_KP_1;
    case VK_DOWN:     return NVPWindow::KEY_KP_2;
    case VK_NEXT:     return NVPWindow::KEY_KP_3;
    case VK_LEFT:     return NVPWindow::KEY_KP_4;
    case VK_CLEAR:    return NVPWindow::KEY_KP_5;
    case VK_RIGHT:    return NVPWindow::KEY_KP_6;
    case VK_HOME:     return NVPWindow::KEY_KP_7;
    case VK_UP:       return NVPWindow::KEY_KP_8;
    case VK_PRIOR:    return NVPWindow::KEY_KP_9;
    case VK_DIVIDE:   return NVPWindow::KEY_KP_DIVIDE;
    case VK_MULTIPLY: return NVPWindow::KEY_KP_MULTIPLY;
    case VK_SUBTRACT: return NVPWindow::KEY_KP_SUBTRACT;
    case VK_ADD:      return NVPWindow::KEY_KP_ADD;
    case VK_DELETE:   return NVPWindow::KEY_KP_DECIMAL;
    default:          break;
    }
  }

  // Check which key was pressed or released
  switch (wParam)
  {
    // The SHIFT keys require special handling
  case VK_SHIFT:
    {
      // Compare scan code for this key with that of VK_RSHIFT in
      // order to determine which shift key was pressed (left or
      // right)
      const DWORD scancode = MapVirtualKey(VK_RSHIFT, 0);
      if ((DWORD) ((lParam & 0x01ff0000) >> 16) == scancode)
        return NVPWindow::KEY_RIGHT_SHIFT;

      return NVPWindow::KEY_LEFT_SHIFT;
    }

    // The CTRL keys require special handling
  case VK_CONTROL:
    {
      MSG next;
      DWORD time;

      // Is this an extended key (i.e. right key)?
      if (lParam & 0x01000000)
        return NVPWindow::KEY_RIGHT_CONTROL;

      // Here is a trick: "Alt Gr" sends LCTRL, then RALT. We only
      // want the RALT message, so we try to see if the next message
      // is a RALT message. In that case, this is a false LCTRL!
      time = GetMessageTime();

      if (PeekMessage(&next, NULL, 0, 0, PM_NOREMOVE))
      {
        if (next.message == WM_KEYDOWN ||
          next.message == WM_SYSKEYDOWN ||
          next.message == WM_KEYUP ||
          next.message == WM_SYSKEYUP)
        {
          if (next.wParam == VK_MENU &&
            (next.lParam & 0x01000000) &&
            next.time == time)
          {
            // Next message is a RALT down message, which
            // means that this is not a proper LCTRL message
            return INTERNAL_KEY_INVALID;
          }
        }
      }

      return NVPWindow::KEY_LEFT_CONTROL;
    }

    // The ALT keys require special handling
  case VK_MENU:
    {
      // Is this an extended key (i.e. right key)?
      if (lParam & 0x01000000)
        return NVPWindow::KEY_RIGHT_ALT;

      return NVPWindow::KEY_LEFT_ALT;
    }

    // The ENTER keys require special handling
  case VK_RETURN:
    {
      // Is this an extended key (i.e. right key)?
      if (lParam & 0x01000000)
        return NVPWindow::KEY_KP_ENTER;

      return NVPWindow::KEY_ENTER;
    }

    // Funcion keys (non-printable keys)
  case VK_ESCAPE:        return NVPWindow::KEY_ESCAPE;
  case VK_TAB:           return NVPWindow::KEY_TAB;
  case VK_BACK:          return NVPWindow::KEY_BACKSPACE;
  case VK_HOME:          return NVPWindow::KEY_HOME;
  case VK_END:           return NVPWindow::KEY_END;
  case VK_PRIOR:         return NVPWindow::KEY_PAGE_UP;
  case VK_NEXT:          return NVPWindow::KEY_PAGE_DOWN;
  case VK_INSERT:        return NVPWindow::KEY_INSERT;
  case VK_DELETE:        return NVPWindow::KEY_DELETE;
  case VK_LEFT:          return NVPWindow::KEY_LEFT;
  case VK_UP:            return NVPWindow::KEY_UP;
  case VK_RIGHT:         return NVPWindow::KEY_RIGHT;
  case VK_DOWN:          return NVPWindow::KEY_DOWN;
  case VK_F1:            return NVPWindow::KEY_F1;
  case VK_F2:            return NVPWindow::KEY_F2;
  case VK_F3:            return NVPWindow::KEY_F3;
  case VK_F4:            return NVPWindow::KEY_F4;
  case VK_F5:            return NVPWindow::KEY_F5;
  case VK_F6:            return NVPWindow::KEY_F6;
  case VK_F7:            return NVPWindow::KEY_F7;
  case VK_F8:            return NVPWindow::KEY_F8;
  case VK_F9:            return NVPWindow::KEY_F9;
  case VK_F10:           return NVPWindow::KEY_F10;
  case VK_F11:           return NVPWindow::KEY_F11;
  case VK_F12:           return NVPWindow::KEY_F12;
  case VK_F13:           return NVPWindow::KEY_F13;
  case VK_F14:           return NVPWindow::KEY_F14;
  case VK_F15:           return NVPWindow::KEY_F15;
  case VK_F16:           return NVPWindow::KEY_F16;
  case VK_F17:           return NVPWindow::KEY_F17;
  case VK_F18:           return NVPWindow::KEY_F18;
  case VK_F19:           return NVPWindow::KEY_F19;
  case VK_F20:           return NVPWindow::KEY_F20;
  case VK_F21:           return NVPWindow::KEY_F21;
  case VK_F22:           return NVPWindow::KEY_F22;
  case VK_F23:           return NVPWindow::KEY_F23;
  case VK_F24:           return NVPWindow::KEY_F24;
  case VK_NUMLOCK:       return NVPWindow::KEY_NUM_LOCK;
  case VK_CAPITAL:       return NVPWindow::KEY_CAPS_LOCK;
  case VK_SNAPSHOT:      return NVPWindow::KEY_PRINT_SCREEN;
  case VK_SCROLL:        return NVPWindow::KEY_SCROLL_LOCK;
  case VK_PAUSE:         return NVPWindow::KEY_PAUSE;
  case VK_LWIN:          return NVPWindow::KEY_LEFT_SUPER;
  case VK_RWIN:          return NVPWindow::KEY_RIGHT_SUPER;
  case VK_APPS:          return NVPWindow::KEY_MENU;

    // Numeric keypad
  case VK_NUMPAD0:       return NVPWindow::KEY_KP_0;
  case VK_NUMPAD1:       return NVPWindow::KEY_KP_1;
  case VK_NUMPAD2:       return NVPWindow::KEY_KP_2;
  case VK_NUMPAD3:       return NVPWindow::KEY_KP_3;
  case VK_NUMPAD4:       return NVPWindow::KEY_KP_4;
  case VK_NUMPAD5:       return NVPWindow::KEY_KP_5;
  case VK_NUMPAD6:       return NVPWindow::KEY_KP_6;
  case VK_NUMPAD7:       return NVPWindow::KEY_KP_7;
  case VK_NUMPAD8:       return NVPWindow::KEY_KP_8;
  case VK_NUMPAD9:       return NVPWindow::KEY_KP_9;
  case VK_DIVIDE:        return NVPWindow::KEY_KP_DIVIDE;
  case VK_MULTIPLY:      return NVPWindow::KEY_KP_MULTIPLY;
  case VK_SUBTRACT:      return NVPWindow::KEY_KP_SUBTRACT;
  case VK_ADD:           return NVPWindow::KEY_KP_ADD;
  case VK_DECIMAL:       return NVPWindow::KEY_KP_DECIMAL;

    // Printable keys are mapped according to US layout
  case VK_SPACE:         return NVPWindow::KEY_SPACE;
  case 0x30:             return NVPWindow::KEY_0;
  case 0x31:             return NVPWindow::KEY_1;
  case 0x32:             return NVPWindow::KEY_2;
  case 0x33:             return NVPWindow::KEY_3;
  case 0x34:             return NVPWindow::KEY_4;
  case 0x35:             return NVPWindow::KEY_5;
  case 0x36:             return NVPWindow::KEY_6;
  case 0x37:             return NVPWindow::KEY_7;
  case 0x38:             return NVPWindow::KEY_8;
  case 0x39:             return NVPWindow::KEY_9;
  case 0x41:             return NVPWindow::KEY_A;
  case 0x42:             return NVPWindow::KEY_B;
  case 0x43:             return NVPWindow::KEY_C;
  case 0x44:             return NVPWindow::KEY_D;
  case 0x45:             return NVPWindow::KEY_E;
  case 0x46:             return NVPWindow::KEY_F;
  case 0x47:             return NVPWindow::KEY_G;
  case 0x48:             return NVPWindow::KEY_H;
  case 0x49:             return NVPWindow::KEY_I;
  case 0x4A:             return NVPWindow::KEY_J;
  case 0x4B:             return NVPWindow::KEY_K;
  case 0x4C:             return NVPWindow::KEY_L;
  case 0x4D:             return NVPWindow::KEY_M;
  case 0x4E:             return NVPWindow::KEY_N;
  case 0x4F:             return NVPWindow::KEY_O;
  case 0x50:             return NVPWindow::KEY_P;
  case 0x51:             return NVPWindow::KEY_Q;
  case 0x52:             return NVPWindow::KEY_R;
  case 0x53:             return NVPWindow::KEY_S;
  case 0x54:             return NVPWindow::KEY_T;
  case 0x55:             return NVPWindow::KEY_U;
  case 0x56:             return NVPWindow::KEY_V;
  case 0x57:             return NVPWindow::KEY_W;
  case 0x58:             return NVPWindow::KEY_X;
  case 0x59:             return NVPWindow::KEY_Y;
  case 0x5A:             return NVPWindow::KEY_Z;
  case 0xBD:             return NVPWindow::KEY_MINUS;
  case 0xBB:             return NVPWindow::KEY_EQUAL;
  case 0xDB:             return NVPWindow::KEY_LEFT_BRACKET;
  case 0xDD:             return NVPWindow::KEY_RIGHT_BRACKET;
  case 0xDC:             return NVPWindow::KEY_BACKSLASH;
  case 0xBA:             return NVPWindow::KEY_SEMICOLON;
  case 0xDE:             return NVPWindow::KEY_APOSTROPHE;
  case 0xC0:             return NVPWindow::KEY_GRAVE_ACCENT;
  case 0xBC:             return NVPWindow::KEY_COMMA;
  case 0xBE:             return NVPWindow::KEY_PERIOD;
  case 0xBF:             return NVPWindow::KEY_SLASH;
  case 0xDF:             return NVPWindow::KEY_WORLD_1;
  case 0xE2:             return NVPWindow::KEY_WORLD_2;
  default:               break;
  }

  // No matching translation was found
  return NVPWindow::KEY_UNKNOWN;
}


//------------------------------------------------------------------------------
LRESULT CALLBACK WindowProc( HWND   m_hWnd, 
                             UINT   msg, 
                             WPARAM wParam, 
                             LPARAM lParam )
{
    bool bRes = false;
    // get back the correct window
    int index = (int)GetWindowLongPtr(m_hWnd, GWLP_USERDATA);
    NVPWindow *pWin = NULL;
    if(g_windows.size() == 0)
      return DefWindowProc( m_hWnd, msg, wParam, lParam );

    pWin = g_windows[index];
    //
    // Pass the messages to our UI, first
    //
    if(!bRes) switch( msg )
    {
        case WM_ACTIVATE:
            if(pWin && pWin->m_internal)
                pWin->m_internal->m_iconified = HIWORD(wParam) ? true : false;
            break;
        case WM_SHOWWINDOW:
            if(pWin && pWin->m_internal)
                pWin->m_internal->m_visible = wParam ? true : false;
            break;
        case WM_PAINT:
            if (pWin && pWin->m_internal)
                pWin->display();
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
          {
                const int scancode = (lParam >> 16) & 0xff;
                const int key = translateKey(wParam, lParam);
                if (key == INTERNAL_KEY_INVALID)
                  break;

                pWin->setKeyModifiers(getKeyMods());

                pWin->keyboard( (NVPWindow::KeyCode)key,NVPWindow::BUTTON_PRESS, pWin->getKeyModifiers(), pWin->getMouseX(), pWin->getMouseY());
                break;
          }

        case WM_KEYUP:
        case WM_SYSKEYUP:
            {
                const int scancode = (lParam >> 16) & 0xff;
                const int key = translateKey(wParam, lParam);
                if (key == INTERNAL_KEY_INVALID)
                  break;

                pWin->setKeyModifiers(getKeyMods());

                pWin->keyboard( (NVPWindow::KeyCode)key,NVPWindow::BUTTON_RELEASE, pWin->getKeyModifiers(), pWin->getMouseX(), pWin->getMouseY());
                break;
            }

        case WM_CHAR:
        case WM_SYSCHAR:
            {
                unsigned int key = (unsigned int)wParam;
                if (key < 32 || (key > 126 && key < 160))
                    break;

                pWin->keyboardchar(key, pWin->getKeyModifiers(), pWin->getMouseX(), pWin->getMouseY());
            }
            break;
        case WM_MOUSEWHEEL:
            pWin->mousewheel((short)HIWORD(wParam));
            break;
        case WM_LBUTTONDBLCLK:
            pWin->setMouse(GET_X_LPARAM(lParam), GET_Y_LPARAM (lParam));
            pWin->mouse(NVPWindow::MOUSE_BUTTON_LEFT, NVPWindow::BUTTON_REPEAT, pWin->getKeyModifiers(), pWin->getMouseX(), pWin->getMouseY());
            break;
        case WM_LBUTTONDOWN:
            pWin->setMouse( GET_X_LPARAM(lParam), GET_Y_LPARAM (lParam));
            pWin->mouse(NVPWindow::MOUSE_BUTTON_LEFT, NVPWindow::BUTTON_PRESS, pWin->getKeyModifiers(), pWin->getMouseX(), pWin->getMouseY());
            break;
        case WM_LBUTTONUP:
            pWin->setMouse( GET_X_LPARAM(lParam), GET_Y_LPARAM (lParam));
            pWin->mouse(NVPWindow::MOUSE_BUTTON_LEFT, NVPWindow::BUTTON_RELEASE, pWin->getKeyModifiers(), pWin->getMouseX(), pWin->getMouseY());
            break;
        case WM_RBUTTONDOWN:
            pWin->setMouse( GET_X_LPARAM(lParam), GET_Y_LPARAM (lParam));
            pWin->mouse(NVPWindow::MOUSE_BUTTON_RIGHT, NVPWindow::BUTTON_PRESS, pWin->getKeyModifiers(), pWin->getMouseX(), pWin->getMouseY());
            break;
        case WM_RBUTTONUP:
            pWin->setMouse( GET_X_LPARAM(lParam), GET_Y_LPARAM (lParam));
            pWin->mouse(NVPWindow::MOUSE_BUTTON_RIGHT, NVPWindow::BUTTON_RELEASE, pWin->getKeyModifiers(), pWin->getMouseX(), pWin->getMouseY());
            break;
        case WM_MBUTTONDOWN:
            pWin->setMouse( GET_X_LPARAM(lParam), GET_Y_LPARAM (lParam));
            pWin->mouse(NVPWindow::MOUSE_BUTTON_MIDDLE, NVPWindow::BUTTON_PRESS, pWin->getKeyModifiers(), pWin->getMouseX(), pWin->getMouseY());
            break;
        case WM_MBUTTONUP:
            pWin->setMouse( GET_X_LPARAM(lParam), GET_Y_LPARAM (lParam));
            pWin->mouse(NVPWindow::MOUSE_BUTTON_MIDDLE, NVPWindow::BUTTON_RELEASE, pWin->getKeyModifiers(), pWin->getMouseX(), pWin->getMouseY());
            break;
        case WM_MOUSEMOVE:
            pWin->setMouse( GET_X_LPARAM(lParam), GET_Y_LPARAM (lParam));
            pWin->motion(pWin->getMouseX(), pWin->getMouseY());
            break;
        case WM_SIZE:
            if (lParam == 0) { // Zero size window is minimized fully
              if (pWin->m_internal) {
                pWin->m_internal->m_iconified = true;
              }
            } else {
              pWin->setWindowSize(LOWORD(lParam), HIWORD(lParam));
              if (pWin->m_internal) {
                pWin->m_internal->m_iconified = false;
                //pWin->m_internal->reshape(LOWORD(lParam), HIWORD(lParam));
              }
              pWin->reshape(LOWORD(lParam), HIWORD(lParam));
            }
            break;
        case WM_CLOSE:
            break;
        case WM_DESTROY:
            if (pWin) {
                pWin->shutdown();
            }
            PostQuitMessage(0);
            pWin->m_isClosing = true;
            break;
        default:
            break;
    }
    return DefWindowProc( m_hWnd, msg, wParam, lParam );
}

