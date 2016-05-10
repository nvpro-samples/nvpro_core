/*-----------------------------------------------------------------------
    Copyright (c) 2013, NVIDIA. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of its contributors may be used to endorse 
       or promote products derived from this software without specific
       prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    feedback to tlorach@nvidia.com (Tristan Lorach)
*/ //--------------------------------------------------------------------
#ifndef __MAIN_H__
#define __MAIN_H__
#pragma warning(disable:4996) // preventing snprintf >> _snprintf_s

#include "platform.h"


// trick for pragma message so we can write:
// #pragma message(__FILE__"("S__LINE__"): blah")
#define S__(x) #x
#define S_(x) S__(x)
#define S__LINE__ S_(__LINE__)

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
#endif

#ifdef SUPPORT_NVTOOLSEXT
#include "NSightEvents.h"
#else
// Note: they are defined inside "NSightEvents.h"
// but let's define them again here as empty defines for the case when NSIGHT is not needed at all
// (NSightEvents.h also does this test but here we avoid including the file NSightEvents.h)
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

#   define  LOGLEVEL_INFO 0
#   define  LOGLEVEL_WARNING 1
#   define  LOGLEVEL_ERROR 2
#   define  LOGLEVEL_OK 7
#   define  LOGI(...)  { nvprintfLevel(0, __VA_ARGS__); }
#   define  LOGW(...)  { nvprintfLevel(1, __VA_ARGS__); }
#   define  LOGE(...)  { nvprintfLevel(2, __FILE__ "(" S__LINE__ "): **ERROR**:\n" __VA_ARGS__); }
#   define  LOGOK(...)  { nvprintfLevel(7, __VA_ARGS__); }

#if _MSC_VER
    #define snprintf _snprintf
#endif
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

  typedef struct WINinternal* WINhandle;
  typedef void (*NVPproc)(void);

  // OpenGL specific
  struct ContextFlags {
    int         major;
    int         minor;
    int         MSAA;
    int         depth;
    int         stencil;
    bool        debug;
    bool        robust;
    bool        core;
    bool        forward;
    bool        stereo;
    NVPWindow*  share;

    ContextFlags(int _major=4, int _minor=3, bool _core=false, int _MSAA=0, int _depth=24, int _stencil=8,bool _debug=false, bool _robust=false, bool _forward=false, bool _stereo=false, NVPWindow* _share=0)
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
    }

  };
  unsigned int  m_debugFilter;
  std::string   m_debugTitle;

  WINhandle     m_internal;
  
  int         m_renderCnt;
  int         m_curX;
  int         m_curY;
  int         m_wheel;
  int         m_winSz[2];
  int         m_mods;

  NVPWindow()
    : m_renderCnt(1)
    , m_internal(0)
    , m_debugFilter(0)
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


  // activate and deactivate are not thread-safe, need to be wrapped in mutex if called from multiple threads
  // invisible windows will not have any active callbacks, nor will they be affected by sysEvents
  bool activate(int width, int height, const char* title, const ContextFlags* flags, int invisible=0);
  void deactivate();
  // compatibility hack
  bool create(const char* title=NULL, const ContextFlags* cflags=0, int width=1024, int height=768);

  void setTitle(const char* title);

  void maximize();
  void restore();
  void minimize();

  void postRedisplay(int n=1) { m_renderCnt=n; }

  void postQuit();

  void makeContextCurrent();
  void makeContextNonCurrent();
  void swapBuffers();
  void swapInterval(int i);

  bool isOpen();

  // from NVPWindow
  virtual bool init() { return true; }
  virtual void shutdown() {}
  virtual void reshape(int w, int h) { }
  virtual void motion(int x, int y) {}
  virtual void mousewheel(int delta) {}
  virtual void mouse(MouseButton button, ButtonAction action, int mods, int x, int y) {}
  virtual void keyboard(KeyCode key, ButtonAction action, int mods, int x, int y) {}
  virtual void keyboardchar(unsigned char key, int mods, int x, int y) {}
  virtual void display() {}

  //////////////////////////////////////////////////////////////////////////
  // system related

  static void     sysInit();
  static void     sysDeinit();

  static bool     sysPollEvents(bool bLoop);
  static void     sysWaitEvents();

  static NVPproc  sysGetProcAddress(const char* name);
  static int      sysExtensionSupported(const char* name);
  static double   sysGetTime(); // in seconds
  static void     sysSleep( double seconds );

  static void     sysVisibleConsole();

  static std::string sysExePath();

};

extern int  sample_main(int argc, const char**argv);

// sample-specific implementation, called by nvprintfLevel. For example to redirect the message to a specific window or part of the viewport
extern void sample_print(int level, const char * fmt2);

extern void checkGL( char* msg );

// sample-specific implementation, called by nvprintf*. For example to redirect the message to a specific window or part of the viewport
extern void sample_print(int level, const char * fmt);

void nvprintf(const char * fmt, ...);
void nvprintfLevel(int level, const char * fmt, ...);
void nvprintSetLevel(int l);
int  nvprintGetLevel();
void nvprintSetLogging(bool b);

#endif