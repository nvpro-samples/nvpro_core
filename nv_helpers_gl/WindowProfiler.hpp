/*-----------------------------------------------------------------------
  Copyright (c) 2014, NVIDIA. All rights reserved.

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
-----------------------------------------------------------------------*/

#ifndef NV_PROJECTBASE_INCLUDED
#define NV_PROJECTBASE_INCLUDED

#include "profilertimersgl.hpp"
#include <string.h> // for memset
#include <main.h>

#define NV_PROFILE_SECTION(name)                    nv_helpers::Profiler::Section _tempTimer(m_profiler ,name, &m_gltimers)
#define NV_PROFILE_SECTION_EX(name, gpui, flush)    nv_helpers::Profiler::Section _tempTimer(m_profiler, name, gpui, flush)
#define NV_PROFILE_SPLIT()                          m_profiler.accumulationSplit()

namespace nv_helpers_gl
{

  /*
    Project by default quits with ESC
    and allows toggling vsync with V
  */

  class WindowProfiler : public NVPWindow {
  public:

    class Window
    {
    public:
      Window() 
        : m_mouseButtonFlags(0)
        , m_wheel(0)
      {
        memset(m_keyPressed, 0, sizeof(m_keyPressed));
        memset(m_keyToggled, 0, sizeof(m_keyToggled));
      }

      int         m_viewsize[2];
      int         m_mouseCurrent[2];
      int         m_mouseButtonFlags;
      int         m_wheel;

      bool m_keyPressed[KEY_LAST+1];
      bool m_keyToggled[KEY_LAST+1];

      bool onPress(int key) {
        return m_keyPressed[key] && m_keyToggled[key];
      }
    };

    WindowProfiler(bool singleThreaded = true, bool doSwap = true) 
      : m_profilerPrint(true)
      , m_vsync(false)
      , m_singleThreaded(singleThreaded)
      , m_doSwap(doSwap)
      , m_active(false)
      , m_timeInTitle(true)
    {
      m_debugFilter = GL_DEBUG_SEVERITY_MEDIUM;

      m_cflags.robust = false;
      m_cflags.core   = false;
#ifdef NDEBUG
      m_cflags.debug  = false;
#else
      m_cflags.debug  = true;
#endif
      m_cflags.share  = NULL;
    }

    virtual bool begin() { return false; }
    virtual void end() {}
    virtual void think(double time) {}
    virtual void resize(int width, int height) {}

    // return true to prevent m_window updates
    virtual bool mouse_pos    (int x, int y) {return false; }
    virtual bool mouse_button (int button, int action) {return false; }
    virtual bool mouse_wheel  (int wheel) {return false; }
    virtual bool key_button   (int button, int action, int modifier) {return false; }
    virtual bool key_char     (int button) {return false; }

    // from NVPWindow
    //void shutdown() {}
    void reshape(int w, int h);
    void motion(int x, int y);
    void mousewheel(int delta);
    void mouse(MouseButton button, ButtonAction action, int mods, int x, int y);
    void keyboard(KeyCode key, ButtonAction action, int mods, int x, int y);
    void keyboardchar(unsigned char key, int mods, int x, int y);
    //void display() { } // leave empty, we call redraw ourselves in think

    Window        m_window;
    nv_helpers::Profiler            m_profiler;
    nv_helpers_gl::ProfilerTimersGL m_gltimers;
    bool          m_profilerPrint;
    bool          m_timeInTitle;
    bool          m_singleThreaded;
    bool          m_doSwap;
    bool          m_active;

    ContextFlags  m_cflags;

    void vsync(bool state);
    void waitEvents();

    int run(const std::string &name,
      int argc, const char** argv, 
      int width, int height,
      int Major, int Minor);

  private:
    bool          m_vsync;
  };
}


#endif


