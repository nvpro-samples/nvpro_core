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
#include <nvpwindow.hpp>

#include "profiler.hpp"
#include "parametertools.hpp"

#define NV_PROFILE_BASE_SECTION(name)                    nvh::Profiler::Section _tempTimer(m_profiler, name)
#define NV_PROFILE_BASE_SPLIT()                          m_profiler.accumulationSplit()

namespace nvh
{

  /*
    Project by default quits with ESC
    and allows toggling vsync with V
  */

  class AppWindowProfiler : public NVPWindow {
  public:
    class WindowState
    {
    public:
      WindowState() 
        : m_mouseButtonFlags(0)
        , m_mouseWheel(0)
      {
        memset(m_keyPressed, 0, sizeof(m_keyPressed));
        memset(m_keyToggled, 0, sizeof(m_keyToggled));
      }

      int   m_viewSize[2];
      int   m_mouseCurrent[2];
      int   m_mouseButtonFlags;
      int   m_mouseWheel;

      bool  m_keyPressed[KEY_LAST+1];
      bool  m_keyToggled[KEY_LAST+1];

      bool onPress(int key) {
        return m_keyPressed[key] && m_keyToggled[key];
      }
    };

    //////////////////////////////////////////////////////////////////////////

    WindowState   m_windowState;
    nvh::Profiler m_profiler;
    bool          m_profilerPrint;
    bool          m_hadProfilerPrint;
    bool          m_timeInTitle;
    bool          m_singleThreaded;
    bool          m_doSwap;

    ParameterList m_parameterList;


    AppWindowProfiler(bool singleThreaded = true, bool doSwap = true)
      : m_profilerPrint(true)
      , m_vsync(false)
      , m_singleThreaded(singleThreaded)
      , m_doSwap(doSwap)
      , m_active(false)
      , m_timeInTitle(true)
      , m_isShutdown(false)
      , m_hadScreenshot(false)
    {
      setupParameters();
    }

    // Sample Related
    //////////////////////////////////////////////////////////////////////////

    virtual bool begin() { return false; }
    virtual void end() {}
    virtual void think(double time) {}
    virtual void resize(int width, int height) {}

    // return true to prevent m_window state updates
    virtual bool mouse_pos    (int x, int y) {return false; }
    virtual bool mouse_button (int button, int action) {return false; }
    virtual bool mouse_wheel  (int wheel) {return false; }
    virtual bool key_button   (int button, int action, int modifier) {return false; }
    virtual bool key_char     (int button) {return false; }

    virtual void parseConfig(int argc, const char** argv, const std::string& path) {
      // if you want to handle parameters not represented in
      // m_parameterList then override this function accordingly.
      m_parameterList.applyTokens( argc, argv, "-", path.c_str() );
      // This function is called before "begin" and provided with the commandline used in "run".
      // It can also be called by the benchmarking system, and parseConfigFile.
    }
    virtual bool validateConfig() {
      // override if you want to test the state of app after parsing configs
      // returning false terminates app
      return true;
    } 

    virtual void postProfiling() { }
    virtual void postEnd() {}
    virtual void postBenchmarkAdvance() {}
    virtual void postConfigPreContext() {};

    
    //////////////////////////////////////////////////////////////////////////

    int  run(const std::string& name, int argc, const char** argv, int width, int height);
    void leave();

    void        parseConfigFile(const char* filename);
    std::string specialStrings(const char* original);

    void waitEvents();

    void setVsync(bool state);
    bool getVsync() const { return m_vsync; }

    //////////////////////////////////////////////////////////////////////////
    // Context Window (if desired, not mandatory )

    virtual void contextInit() {}
    virtual void contextDeinit() {}
    virtual void contextScreenshot(const char* bmpfilename, int width, int height) {}
    virtual const char* contextGetDeviceName() { return NULL; }

    virtual void swapResize(int width, int height) {}
    virtual void swapPrepare() {}
    virtual void swapBuffers() {}
    virtual void swapVsync(bool state) {}


    //////////////////////////////////////////////////////////////////////////

    // from NVPWindow
    void shutdown() override;
    void reshape(int w, int h) override;
    void motion(int x, int y) override;
    void mousewheel(int delta) override;
    void mouse(MouseButton button, ButtonAction action, int mods, int x, int y) override;
    void keyboard(KeyCode key, ButtonAction action, int mods, int x, int y) override;
    void keyboardchar(unsigned char key, int mods, int x, int y) override;
    void display() override { } // leave empty, we call redraw ourselves in think    
  
  private:

    struct Benchmark {
      std::string                   filename;
      std::string                   content;
      nvh::ParameterSequence        sequence;
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
      std::string dumpatexitFilename;
      std::string screenshotFilename;
      std::string logFilename;
      std::string configFilename;

      Config() {
        winpos[0] = 0;
        winpos[1] = 0;
        winsize[0] = 0;
        winsize[1] = 0;
      }
    };

    void parameterCallback(uint32_t param);

    void setupParameters();
    void exitScreenshot();

    void initBenchmark();
    void advanceBenchmark();

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
    uint32_t      m_paramCfg;
    uint32_t      m_paramBat;
  };
}


#endif


