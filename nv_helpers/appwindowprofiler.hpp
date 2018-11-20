/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
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

#ifndef NV_PROJECTBASE_INCLUDED
#define NV_PROJECTBASE_INCLUDED

#include <string.h> // for memset
#include <main.h>

#include "profiler.hpp"
#include "parametertools.hpp"

#define NV_PROFILE_SECTION(name)                    nv_helpers::Profiler::Section _tempTimer(m_profiler, name)
#define NV_PROFILE_SECTION_EX(name, gpui, flush)    nv_helpers::Profiler::Section _tempTimer(m_profiler, name, gpui, flush)
#define NV_PROFILE_SPLIT()                          m_profiler.accumulationSplit()

namespace nv_helpers
{

  /*
    Project by default quits with ESC
    and allows toggling vsync with V
  */

  class AppWindowProfiler : public NVPWindow, public ParameterList::Callback {
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

    struct Benchmark {
      std::string                   filename;
      std::string                   content;
      nv_helpers::ParameterSequence sequence;
      uint32_t                      frameLength = 256;
      uint32_t                      frame = 0;
    };

    struct Config {
      int32_t winpos[2];
      int32_t winsize[2];
      bool vsyncstate = true;
      uint32_t intervalSeconds = 2;
      uint32_t frameLimit = 0;
      uint32_t timerLimit = 0;
      std::string dumpatexit;
      std::string screenshot;
      std::string logfile;

      Config() {
        winpos[0] = 0;
        winpos[1] = 0;
        winsize[0] = 0;
        winsize[1] = 0;
      }
    };

    AppWindowProfiler(WindowApi api, bool singleThreaded = true, bool doSwap = true)
      : m_profilerPrint(true)
      , m_vsync(false)
      , m_singleThreaded(singleThreaded)
      , m_doSwap(doSwap)
      , m_active(false)
      , m_timeInTitle(true)
      , m_isShutdown(false)
      , m_hadScreenshot(false)
      , m_windowApi(api)
    {
      setupParameters();
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

    virtual void postProfiling() { }

    virtual void parseConfig(int argc, const char** argv, const std::string& path) {}
    virtual void postBenchmarkAdvance() {}

    void parseConfigFile(const char* filename);

    std::string specialStrings(const char* original);

    // from NVPWindow
    void shutdown() override;
    void reshape(int w, int h) override;
    void motion(int x, int y) override;
    void mousewheel(int delta) override;
    void mouse(MouseButton button, ButtonAction action, int mods, int x, int y) override;
    void keyboard(KeyCode key, ButtonAction action, int mods, int x, int y) override;
    void keyboardchar(unsigned char key, int mods, int x, int y) override;
    void display() override { } // leave empty, we call redraw ourselves in think

    

    Window                    m_window;
    nv_helpers::Profiler      m_profiler;
    
    bool          m_profilerPrint;
    bool          m_hadProfilerPrint;
    bool          m_timeInTitle;
    bool          m_singleThreaded;
    bool          m_doSwap;

    ParameterList m_parameterList;

    void vsync(bool state);
    void waitEvents();

    int run(const std::string &name,
      int argc, const char** argv, 
      int width, int height,
      int apiMajor, int apiMinor);
    
    void leave();

    virtual const ContextFlagsBase* preWindowContext(int apiMajor, int apiMinor) { return nullptr;};
    virtual void  postWindow() {};
    virtual void  postEnd() {};
    virtual void  dumpScreenshot(const char* bmpfilename, int width, int height) {};

    bool getVsync() const 
    {
      return m_vsync;
    }

    virtual void parameterCallback(uint32_t param);
  
  private:
    void setupParameters();
    void exitScreenshot();

    void initBenchmark();
    void advanceBenchmark();

    WindowApi     m_windowApi;
    bool          m_active;
    bool          m_vsync;
    bool          m_isShutdown;
    bool          m_hadScreenshot;
    Config        m_config;
    Benchmark     m_benchmark;

    uint32_t      m_paramWinsize;
    uint32_t      m_paramVsync;
    uint32_t      m_paramScreenshot;
    uint32_t      m_paramLog;
  };
}


#endif


