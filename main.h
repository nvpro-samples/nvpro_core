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

#ifndef __MAIN_H__
#define __MAIN_H__
#pragma warning(disable:4996) // preventing snprintf >> _snprintf_s

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <map>
#ifdef WIN32
    #ifdef MEMORY_LEAKS_CHECK
        #   pragma message("build will Check for Memory Leaks!")
        #   define _CRTDBG_MAP_ALLOC
        #   include <stdlib.h>
        #   include <crtdbg.h>
        inline void* operator new(size_t size, const char *file, int line)
        {
           return ::operator new(size, 1, file, line);
        }

        inline void __cdecl operator delete(void *ptr, const char *file, int line) 
        {
           ::operator delete(ptr, _NORMAL_BLOCK, file, line);
        }

        #define DEBUG_NEW new( __FILE__, __LINE__)
        #define MALLOC_DBG(x) _malloc_dbg(x, 1, __FILE__, __LINE__);
        #define malloc(x) MALLOC_DBG(x)
        #define new DEBUG_NEW
    #endif

    #define VK_USE_PLATFORM_WIN32_KHR

#ifdef USEDIRECTX11
  #include <d3d11.h>
  #include <nv_helpers_dx11/window_dx11.hpp>
#endif
#ifdef USEDIRECTX12
  #include <d3d12.h>
  #include <dxgi1_5.h>
  #include <D3Dcompiler.h>
  #include <nv_helpers_dx12/window_dx12.hpp>
  #include <nv_helpers_dx12/swapchain_dx12.hpp>
#endif

#endif // WIN32

#ifdef USEVULKANSDK
  #include <nv_helpers_vk/window_vk.hpp>
#endif



#ifdef LINUX

#ifdef USEOPENGL
  #include <GL/gl.h>
  #include <GL/glx.h>
  #include <GL/glxext.h>
  #include <GL/glu.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/xf86vmode.h>

#endif

#ifdef SUPPORT_NVTOOLSEXT
#include "nv_helpers/nsightevents.h"
#else
// Note: they are defined inside "nsightevents.h"
// but let's define them again here as empty defines for the case when NSIGHT is not needed at all
#	define NX_RANGE int
#	define NX_MARK(name)
#	define NX_RANGESTART(name) 0
#	define NX_RANGEEND(id)
#	define NX_RANGEPUSH(name)
#	define NX_RANGEPUSHCOL(name, c)
#	define NX_RANGEPOP()
#	define NXPROFILEFUNC(name)
#	define NXPROFILEFUNCCOL(name, c)
#	define NXPROFILEFUNCCOL2(name, c, a)
#endif

#include <nv_helpers/nvprint.hpp>

//----------------- to be declared in the code of the sample: so the sample can decide how to display messages
class NVPWindow
{
public:

  // these are taken from GLFW3 and should be kept in a matching state

  enum ButtonAction {
    BUTTON_RELEASE = 0,
    BUTTON_PRESS = 1,
    BUTTON_REPEAT = 2,
  };

  enum MouseButton
  {
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_MIDDLE,
    NUM_MOUSE_BUTTONIDX,
  };

  enum MouseButtonFlag
  {
    MOUSE_BUTTONFLAG_NONE = 0,
    MOUSE_BUTTONFLAG_LEFT = (1 << MOUSE_BUTTON_LEFT),
    MOUSE_BUTTONFLAG_RIGHT = (1 << MOUSE_BUTTON_RIGHT),
    MOUSE_BUTTONFLAG_MIDDLE = (1 << MOUSE_BUTTON_MIDDLE)
  };

  enum KeyCode {
    KEY_UNKNOWN     = -1,
    KEY_SPACE       = 32,
    KEY_APOSTROPHE            = 39  /* ' */,
    KEY_LEFT_PARENTHESIS      = 40  /* ( */,
    KEY_RIGHT_PARENTHESIS     = 41  /* ) */,
    KEY_ASTERISK              = 42  /* * */,
    KEY_PLUS                  = 43  /* + */,
    KEY_COMMA                 = 44  /* , */,
    KEY_MINUS                 = 45  /* - */,
    KEY_PERIOD                = 46  /* . */,
    KEY_SLASH                 = 47  /* / */,
    KEY_0                     = 48,
    KEY_1                     = 49,
    KEY_2                     = 50,
    KEY_3                     = 51,
    KEY_4                     = 52,
    KEY_5                     = 53,
    KEY_6                     = 54,
    KEY_7                     = 55,
    KEY_8                     = 56,
    KEY_9                     = 57,
    KEY_COLON                 = 58  /* : */,
    KEY_SEMICOLON             = 59  /* ; */,
    KEY_LESS                  = 60  /* < */,
    KEY_EQUAL                 = 61  /* = */,
    KEY_GREATER               = 62  /* > */,
    KEY_A                     = 65,
    KEY_B                     = 66,
    KEY_C                     = 67,
    KEY_D                     = 68,
    KEY_E                     = 69,
    KEY_F                     = 70,
    KEY_G                     = 71,
    KEY_H                     = 72,
    KEY_I                     = 73,
    KEY_J                     = 74,
    KEY_K                     = 75,
    KEY_L                     = 76,
    KEY_M                     = 77,
    KEY_N                     = 78,
    KEY_O                     = 79,
    KEY_P                     = 80,
    KEY_Q                     = 81,
    KEY_R                     = 82,
    KEY_S                     = 83,
    KEY_T                     = 84,
    KEY_U                     = 85,
    KEY_V                     = 86,
    KEY_W                     = 87,
    KEY_X                     = 88,
    KEY_Y                     = 89,
    KEY_Z                     = 90,
    KEY_LEFT_BRACKET          = 91  /* [ */,
    KEY_BACKSLASH             = 92  /* \ */,
    KEY_RIGHT_BRACKET         = 93  /* ] */,
    KEY_GRAVE_ACCENT          = 96  /* ` */,
    KEY_WORLD_1               = 161 /* non-US #1 */,
    KEY_WORLD_2               = 162 /* non-US #2 */,
    /* Function keys */
    KEY_ESCAPE             = 256,
    KEY_ENTER              = 257,
    KEY_TAB                = 258,
    KEY_BACKSPACE          = 259,
    KEY_INSERT             = 260,
    KEY_DELETE             = 261,
    KEY_RIGHT              = 262,
    KEY_LEFT               = 263,
    KEY_DOWN               = 264,
    KEY_UP                 = 265,
    KEY_PAGE_UP            = 266,
    KEY_PAGE_DOWN          = 267,
    KEY_HOME               = 268,
    KEY_END                = 269,
    KEY_CAPS_LOCK          = 280,
    KEY_SCROLL_LOCK        = 281,
    KEY_NUM_LOCK           = 282,
    KEY_PRINT_SCREEN       = 283,
    KEY_PAUSE              = 284,
    KEY_F1                 = 290,
    KEY_F2                 = 291,
    KEY_F3                 = 292,
    KEY_F4                 = 293,
    KEY_F5                 = 294,
    KEY_F6                 = 295,
    KEY_F7                 = 296,
    KEY_F8                 = 297,
    KEY_F9                 = 298,
    KEY_F10                = 299,
    KEY_F11                = 300,
    KEY_F12                = 301,
    KEY_F13                = 302,
    KEY_F14                = 303,
    KEY_F15                = 304,
    KEY_F16                = 305,
    KEY_F17                = 306,
    KEY_F18                = 307,
    KEY_F19                = 308,
    KEY_F20                = 309,
    KEY_F21                = 310,
    KEY_F22                = 311,
    KEY_F23                = 312,
    KEY_F24                = 313,
    KEY_F25                = 314,
    KEY_KP_0               = 320,
    KEY_KP_1               = 321,
    KEY_KP_2               = 322,
    KEY_KP_3               = 323,
    KEY_KP_4               = 324,
    KEY_KP_5               = 325,
    KEY_KP_6               = 326,
    KEY_KP_7               = 327,
    KEY_KP_8               = 328,
    KEY_KP_9               = 329,
    KEY_KP_DECIMAL         = 330,
    KEY_KP_DIVIDE          = 331,
    KEY_KP_MULTIPLY        = 332,
    KEY_KP_SUBTRACT        = 333,
    KEY_KP_ADD             = 334,
    KEY_KP_ENTER           = 335,
    KEY_KP_EQUAL           = 336,
    KEY_LEFT_SHIFT         = 340,
    KEY_LEFT_CONTROL       = 341,
    KEY_LEFT_ALT           = 342,
    KEY_LEFT_SUPER         = 343,
    KEY_RIGHT_SHIFT        = 344,
    KEY_RIGHT_CONTROL      = 345,
    KEY_RIGHT_ALT          = 346,
    KEY_RIGHT_SUPER        = 347,
    KEY_MENU               = 348,
    KEY_LAST               = KEY_MENU,
  };

  enum KeyModifiers {
    KMOD_SHIFT            = 0x0001,
    KMOD_CONTROL          = 0x0002,
    KMOD_ALT              = 0x0004,
    KMOD_SUPER            = 0x0008,
  };

  //////////////////////////////////////////////////////////////////////////


  enum WindowApi {
    WINDOW_API_NONE,
#ifdef USEOPENGL
    WINDOW_API_OPENGL,
#endif
#ifdef USEVULKANSDK
    WINDOW_API_VULKAN,
#endif
#ifdef USEDIRECTX11
    WINDOW_API_DX11,
#endif
#ifdef USEDIRECTX12
    WINDOW_API_DX12,
#endif
  };

  typedef void ContextFlagsBase;
  typedef struct WINinternal* WINhandle;


#ifdef USEOPENGL
  // OpenGL specific
  // do we want to add RGB+A # of bits (Android) ?
  struct ContextFlagsGL {
    int         major;
    int         minor;
    int         device;

    int         MSAA;
    int         depth;
    int         stencil;
    bool        debug;
    bool        robust;
    bool        core;
    bool        forward;
    bool        stereo;
    NVPWindow*  share;

    ContextFlagsGL(int _major=4, int _minor=3, 
      bool _core=false, int _MSAA=0, int _depth=24, int _stencil=8,
      bool _debug=false, bool _robust=false, 
      bool _forward=false, bool _stereo=false, NVPWindow* _share=0)
    {
      major = _major;
      minor = _minor;
      MSAA = _MSAA;
      depth = _depth;
      stencil = _stencil;
      core = _core;
      debug = _debug;
      robust = _robust;
      forward = _forward;
      stereo = _stereo;
      share = _share;
      device = 0;
    }
  };
#endif

#ifdef USEVULKANSDK
  typedef nv_helpers_vk::BasicContextInfo ContextFlagsVK;

  // if you want to handle the device and swapchain creation yourself
  // pass a nullptr as ContextFlags and instead us createSurfaceVK
#endif

#ifdef USEDIRECTX11
  struct ContextFlagsDX11 {
    D3D_FEATURE_LEVEL         level;
  };
#endif

#ifdef USEDIRECTX12
  #ifndef D3D12_SWAP_CHAIN_SIZE
  #define D3D12_SWAP_CHAIN_SIZE 2
  #endif
  struct ContextFlagsDX12 {
    D3D_FEATURE_LEVEL         level;
  };
#endif

  std::string   m_deviceName;

  unsigned int  m_debugFilter;
  std::string   m_debugTitle;

  WINhandle     m_internal;
  WindowApi     m_api;
  
  int         m_renderCnt;
  int         m_curX;
  int         m_curY;
  int         m_wheel;
  int         m_winSz[2];
  int         m_mods;
  bool        m_isFullScreen;

  NVPWindow()
    : m_renderCnt(1)
    , m_internal(0)
    , m_debugFilter(0)
    , m_isFullScreen(false)
  {

  }

  // Accessors
  inline void         setWinSz(int w, int h) { m_winSz[0]=w; m_winSz[1]=h; }
  inline const int*   getWinSz() const { return m_winSz; }
  inline int          getWidth() const { return m_winSz[0]; }
  inline int          getHeight() const { return m_winSz[1]; }
  inline const int    getWheel() const { return m_wheel; }
  inline int          getMods() const { return m_mods; }
  inline void         setMods(int m) { m_mods = m; }
  inline void         setCurMouse(int x, int y) { m_curX = x; m_curY = y; }
  inline int          getCurX() { return m_curX; }
  inline int          getCurY() { return m_curY; }
  inline bool         isFullScreen() { return m_isFullScreen; }


  // Set the swapchain type that a window
  // should implicitly create. Requires appropriate
  // ContextFlags to be provided as well.
  // After window activation you can query details 
  // via "getBasicWindowAPI".
  bool activate(WindowApi api, int width, int height, const char* title, const ContextFlagsBase* cflags, int posX=0, int posY=0);

  void setTitle(const char* title);

  void maximize();
  void restore();
  void minimize();
  void windowPos(int x, int y, int w, int h);

  void postRedisplay(int n = 1);
  void postQuit();

  void swapPrepare();
  void swapBuffers();
  void swapInterval(int i);

  bool isOpen();

#ifdef USEOPENGL
  void makeContextCurrentGL();
  void makeContextNonCurrentGL();
  void* getProcAddressGL(const char* name);
  int   extensionSupportedGL(const char* name);
#endif

#ifdef USEVULKANSDK
  // if ContextFlags was non-null, retrieve the context and swapchain information
  const nv_helpers_vk::BasicWindow*   getBasicWindowVK();

  // if ContextFlags was null, use this function to create a surface. Useful for fine-grained control over device and swapchain creation.
  VkResult                            createSurfaceVK(VkInstance instance, const VkAllocationCallbacks *allocator, VkSurfaceKHR* surface);
#endif

#ifdef USEDIRECTX11
  const nv_helpers_dx11::BasicWindow* getBasicWindowDX11();
#endif

#ifdef USEDIRECTX12
  const nv_helpers_dx12::BasicWindow*   getBasicWindowDX12();
#endif


  //
  // from NVPWindow
  //
  virtual bool init() { return true; }
  virtual void shutdown() {}
  virtual void reshape(int w, int h) { }
  virtual void motion(int x, int y) {}
  virtual void mousewheel(int delta) {}
  virtual void mouse(MouseButton button, ButtonAction action, int mods, int x, int y) {}
  virtual void keyboard(KeyCode key, ButtonAction action, int mods, int x, int y) {}
  virtual void keyboardchar(unsigned char key, int mods, int x, int y) {}
  virtual void display();


  //
  // added for remote Socket system, must be implemented by the sample
  //
  virtual void continuousRefresh(bool bYes) {}  // up to the sample
  virtual void timingRequest() {}      // up to the sample
  virtual void setArg(char token, int arg0, int arg1, int arg2, int arg3) {}
  virtual void setArg(char token, float arg0, float arg1, float arg2, float arg3) {}
  //
  // added for remote socket system, implemented by main
  //
  void fullScreen(bool bYes);
  void screenshot(int idx, int x, int y, int w, int h);
  void postTiming(float ms, int fps, const char *details = NULL);
  void postscreenshot(unsigned char* data, size_t sz, int w, int h);

  //////////////////////////////////////////////////////////////////////////
  // system related

  static void     sysInit();
  static void     sysDeinit();

  static bool     sysPollEvents(bool bLoop);
  static void     sysWaitEvents();

  static double   sysGetTime(); // in seconds
  static void     sysSleep( double seconds );

  static std::string sysExePath();

#ifdef USEOPENGL
  // uses first created window
  static void*    sysGetProcAddressGL(const char* name);
  static int      sysExtensionSupportedGL(const char* name);
#endif

#ifdef USEVULKANSDK
  static const char** sysGetRequiredSurfaceExtensionsVK(uint32_t &numExtensions);
#endif

  ////////////////////////////////////////////////////////////////////////////
  //
private:
  bool activateInternal(int width, int height, int xpos, int ypos, const char* title, const ContextFlagsBase* cflags, struct WINinternal* internal);


};

extern int  sample_main(int argc, const char**argv);

// sample-specific implementation, called by nvprintfLevel. For example to redirect the message to a specific window or part of the viewport
extern void sample_print(int level, const char * fmt);


//------------------------------------------------------------------------------
// INTERNAL use from within main_xxx.cpp and wininternal...
// Win Internal contains stuff specific to win32 or Linux etc. It depends
// (so, linux will have a different one)
// But doesn't contain anything related to Vulkan,OpenGL or D3D
//------------------------------------------------------------------------------
#ifdef DECL_WININTERNAL
struct WINinternal
{
  NVPWindow * m_win;
  std::string m_deviceName;

#pragma message ("Compiling WINinternal...")
  

#ifdef WIN32
  HDC   m_hDC;
  HWND  m_hWnd;
  HWND  m_hWndDummy;
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

  WINinternal (NVPWindow *win)
    : m_win(win)
#ifdef WIN32
    , m_hDC(NULL)
    , m_hWnd(NULL)
    , m_hWndDummy(NULL)
#endif
#ifdef LINUX
    , m_dpy(NULL)
    , m_visual(NULL)
    , m_screen(0)
#endif
    , m_iconified(false)
    , m_visible(true)
  { }

  virtual ~WINinternal() {}

  virtual bool create(const char* title, int width, int height, int xPos, int yPos, int inSamples = 0);
  virtual void terminate();
  virtual bool initBase(const NVPWindow::ContextFlagsBase* cflags, NVPWindow* sourcewindow) { return true; }
  virtual void reshape(int w, int h) { }

  virtual void swapPrepare()        { }
  virtual void swapInterval(int i)  { }
  virtual void swapBuffers()        { }
  virtual void display()            { }

  virtual void screenshot(const char* filename, int x, int y, int width, int height, unsigned char* data) { assert(!"Not implemented!"); }
};

#ifdef USEVULKANSDK
extern WINinternal* newWINinternalVK(NVPWindow *win);
#endif
#ifdef USEOPENGL
extern WINinternal* newWINinternalGL(NVPWindow *win);
#endif
#ifdef USEDIRECTX11
extern WINinternal* newWINinternalDX11(NVPWindow *win);
#endif
#ifdef USEDIRECTX12
extern WINinternal* newWINinternalDX12(NVPWindow *win);
#endif

extern WINinternal* newWINinternal(NVPWindow *win);


#endif

#endif
