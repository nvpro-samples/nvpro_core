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

#include "appwindowprofiler.hpp"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>

#include <nv_helpers/misc.hpp>
#include <nv_helpers/assetsloader.hpp>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX 
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace nv_helpers
{
  static AppWindowProfiler* s_project;

  void AppWindowProfiler::motion(int x, int y)
  {
    AppWindowProfiler::Window &window = m_window;

    if (!window.m_mouseButtonFlags && mouse_pos(x,y)) return;

    window.m_mouseCurrent[0] = x;
    window.m_mouseCurrent[1] = y;
  }

  void AppWindowProfiler::mouse(MouseButton Button, ButtonAction Action, int mods, int x, int y)
  {
    AppWindowProfiler::Window  &window   = m_window;
    m_profiler.reset();

    if (mouse_button(Button,Action)) return;

    switch(Action)
    {
    case BUTTON_PRESS:
      {
        switch(Button)
        {
        case MOUSE_BUTTON_LEFT:
          {
            window.m_mouseButtonFlags |= MOUSE_BUTTONFLAG_LEFT;
          }
          break;
        case MOUSE_BUTTON_MIDDLE:
          {
            window.m_mouseButtonFlags |= MOUSE_BUTTONFLAG_MIDDLE;
          }
          break;
        case MOUSE_BUTTON_RIGHT:
          {
            window.m_mouseButtonFlags |= MOUSE_BUTTONFLAG_RIGHT;
          }
          break;
        }
      }
      break;
    case BUTTON_RELEASE:
      {
        if (!window.m_mouseButtonFlags) break;

        switch(Button)
        {
        case MOUSE_BUTTON_LEFT:
          {
            window.m_mouseButtonFlags &= ~MOUSE_BUTTONFLAG_LEFT;
          }
          break;
        case MOUSE_BUTTON_MIDDLE:
          {
            window.m_mouseButtonFlags &= ~MOUSE_BUTTONFLAG_MIDDLE;
          }
          break;
        case MOUSE_BUTTON_RIGHT:
          {
            window.m_mouseButtonFlags &= ~MOUSE_BUTTONFLAG_RIGHT;
          }
          break;
        }
      }
      break;
    }
  }

  void AppWindowProfiler::mousewheel(int y)
  {
    AppWindowProfiler::Window &window = m_window;
    m_profiler.reset();

    if (mouse_wheel(y)) return;

    window.m_wheel += y;
  }

  void AppWindowProfiler::keyboard(KeyCode key, ButtonAction action, int mods, int x, int y)
  {
    AppWindowProfiler::Window  &window   = m_window;
    m_profiler.reset();

    if (key_button(key,action,mods)) return;

    bool newState;

    switch(action)
    {
    case BUTTON_PRESS:
    case BUTTON_REPEAT:
      {
        newState = true;
        break;
      }
    case BUTTON_RELEASE:
      {
        newState = false;
        break;
      }
    }

    window.m_keyToggled[key] = window.m_keyPressed[key] != newState;
    window.m_keyPressed[key] = newState;
  }

  void AppWindowProfiler::keyboardchar(unsigned char key, int mods, int x, int y)
  {
    m_profiler.reset();

    if (key_char(key)) return;
  }

  void AppWindowProfiler::parseConfigFile(const char* filename)
  {
    std::string result = AssetLoadTextFile(filename);
    if (result.empty()) {
      LOGW("file not found: %s\n", filename);
      return;
    }
    std::vector<const char*> args;
    ParameterList::tokenizeString(result, args);

    std::string path = getFilePath(filename);

    parseConfig(uint32_t(args.size()), args.data(), path);
  }

  void AppWindowProfiler::shutdown()
  {
    m_isShutdown = true;
    exitScreenshot();
  }

  void AppWindowProfiler::reshape(int width, int height)
  {
    AppWindowProfiler::Window  &window   = m_window;
    m_profiler.reset();

    if (width == 0 && height == 0)
    {
      return;
    }

    window.m_viewsize[0] = width;
    window.m_viewsize[1] = height;
    if( m_active )
    {
      resize(width,height);
    }
  }


  void AppWindowProfiler::vsync(bool state)
  {
    swapInterval(state ? 1 : 0);
    m_config.vsyncstate = state;
    m_vsync = state;
    LOGI("vsync: %s\n", state ? "on" : "off");
  }

  void AppWindowProfiler::waitEvents()
  {
    sysWaitEvents();
  }

  int AppWindowProfiler::run
  (
    const std::string& title,
    int argc, const char** argv,
    int width, int height,
    int apiMajor, int apiMinor
  )
  {
    s_project = this;

#if _WIN32 && 0
    // do not use this anymore, too much side-effects
    if (m_singleThreaded)
    {
      HANDLE proc = GetCurrentProcess();
      size_t procmask;
      size_t sysmask;
      // pin to one physical cpu for smoother timings, disable hyperthreading
      GetProcessAffinityMask(proc, (PDWORD_PTR)&procmask, (PDWORD_PTR)&sysmask);
      if (sysmask & 8) {
        // quadcore, use last core
        procmask = 8;
      }
      else if (sysmask & 2) {
        // dualcore, use last core
        procmask = 2;
      }
      SetProcessAffinityMask(proc, (DWORD_PTR)procmask);
    }
#endif

    m_config.winsize[0] = m_config.winsize[0] ? m_config.winsize[0] : width;
    m_config.winsize[1] = m_config.winsize[1] ? m_config.winsize[1] : height;

    m_parameterList.applyTokens(argc, argv, "-", ".");

    const ContextFlagsBase* contextInfo = preWindowContext(apiMajor, apiMinor);
    if (!activate(m_windowApi, m_config.winsize[0], m_config.winsize[1], title.c_str(), contextInfo, m_config.winpos[0], m_config.winpos[1])) {
        LOGE("Could not create window context: %d.%d\n",apiMajor, apiMinor);
      return EXIT_FAILURE;
    }

    m_window.m_viewsize[0] = m_config.winsize[0];
    m_window.m_viewsize[1] = m_config.winsize[1];

    // hack to react on $DEVICE$ filename
    if (!m_config.logfile.empty()) {
      parameterCallback(m_paramLog);
    }

    LOGI("Window device: %s\n", m_deviceName.c_str());

    m_profiler.init();
    postWindow();

    vsync(m_config.vsyncstate);
    
    initBenchmark();
    bool Run = begin();
    m_active = true;

    bool quickExit = false;
    if (m_config.frameLimit) {
      m_profiler.setDefaultGPUInterface(nullptr);
      m_profilerPrint = false;
      quickExit = true;
    }

    double timeStart = sysGetTime();
    double timeBegin = sysGetTime();
    double frames = 0;

    bool   lastVsync = m_vsync;

    m_hadProfilerPrint = false;


    if(Run)
    {
      while(true)
      {
        if (!NVPWindow::sysPollEvents(false) || m_isShutdown){
          break;
        }

        while ( !isOpen() ){
          NVPWindow::sysWaitEvents();
        }

        if (m_window.onPress(KEY_V)){
          vsync(!m_vsync);
        }
        
        std::string stats;
        {
          bool benchmarkActive = m_benchmark.sequence.isActive();
          nv_helpers::Profiler::FrameHelper helper(m_profiler,sysGetTime(), m_profilerPrint && !benchmarkActive ? float(m_config.intervalSeconds) : float(FLT_MAX), stats);
          if (m_doSwap)
          {
            swapPrepare();
          }
          {
            NV_PROFILE_SECTION("Frame");
            think(sysGetTime() - timeStart);
          }
          memset(m_window.m_keyToggled, 0, sizeof(m_window.m_keyToggled)); 
          if( m_doSwap )
          {
            swapBuffers();
          }
        }

        m_hadProfilerPrint = false;

        if (m_profilerPrint && !stats.empty()){
          if (!m_config.timerLimit || m_config.timerLimit == 1){
            LOGI("%s\n", stats.c_str());
            m_hadProfilerPrint = true;
          }
          if (m_config.timerLimit == 1){
            m_config.frameLimit = 1;
          }
          if (m_config.timerLimit){
            m_config.timerLimit--;
          }
        }

        advanceBenchmark();
        postProfiling();

        frames++;

        double timeCurrent = sysGetTime();
        double timeDelta = timeCurrent - timeBegin;
        if (timeDelta > double(m_config.intervalSeconds) || lastVsync != m_vsync || m_config.frameLimit==1){
          std::ostringstream combined; 

          if (lastVsync != m_vsync){
            timeDelta = 0;
          }

          if (m_timeInTitle) {
            combined << title << ": " << (timeDelta*1000.0/(frames)) << " [ms]" << (m_vsync ? " (vsync on - V for toggle)" : "");
            setTitle(combined.str().c_str());
          }

          if (m_config.frameLimit==1){
            LOGI("frametime: %f ms\n", (timeDelta*1000.0/(frames)));
          }

          frames = 0;
          timeBegin = timeCurrent;
          lastVsync = m_vsync;
        }

        if(m_window.m_keyPressed[KEY_ESCAPE] || m_config.frameLimit==1)
          break;

        if (m_config.frameLimit) m_config.frameLimit--;
      }
    }

    exitScreenshot();

    if (quickExit) {
      exit(0);
      return EXIT_SUCCESS;
    }

    end();
    m_active = false;

    m_profiler.deinit();
    postEnd();

    return Run ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  void AppWindowProfiler::leave()
  {
    m_config.frameLimit = 1;
  }

  static void replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }
  }

  std::string AppWindowProfiler::specialStrings(const char* original)
  {
    std::string str(original);

    if (!m_deviceName.empty()) {
      std::string deviceName = m_deviceName;
      replace(deviceName, "INTEL(R) ", "");
      replace(deviceName, "AMD ", "");
      replace(deviceName, "DRI ", "");
      replace(deviceName, "(TM) ", "");
      replace(deviceName, " Series", "");
      replace(deviceName, " Graphics", "");
      replace(deviceName, "/PCIe/SSE2", "");
      std::replace(deviceName.begin(), deviceName.end(), ' ', '_');
      std::remove(deviceName.begin(), deviceName.end(), '/');
      std::remove(deviceName.begin(), deviceName.end(), '\\');
      std::remove(deviceName.begin(), deviceName.end(), ':');
      std::remove(deviceName.begin(), deviceName.end(), '?');
      std::remove(deviceName.begin(), deviceName.end(), '*');
      std::remove(deviceName.begin(), deviceName.end(), '<');
      std::remove(deviceName.begin(), deviceName.end(), '>');
      std::remove(deviceName.begin(), deviceName.end(), '|');
      std::remove(deviceName.begin(), deviceName.end(), '"');
      std::remove(deviceName.begin(), deviceName.end(), ',');
      
      // replace $DEVICE$
      replace(str, "$DEVICE$", deviceName);
    }

    return str;
  }
  
  void AppWindowProfiler::parameterCallback(uint32_t param)
  {
    if (param == m_paramLog) {
      std::string logfileName = specialStrings(m_config.logfile.c_str());
      nvprintSetLogFileName(logfileName.c_str());
    }

    if (!m_active) return;

    if (param == m_paramWinsize) {
      AppWindowProfiler::reshape(m_config.winsize[0], m_config.winsize[1]);
    }
    else if (param == m_paramVsync) {
      vsync(m_config.vsyncstate);
    }
    else if (param == m_paramScreenshot) {
      std::string filename = specialStrings(m_config.screenshot.c_str());
      dumpScreenshot(filename.c_str(), m_window.m_viewsize[0], m_window.m_viewsize[1]);
    }
  }

  void AppWindowProfiler::setupParameters()
  {
    m_paramWinsize = m_parameterList.add("winsize|Set window size (width and height)", m_config.winsize, (ParameterList::Callback*)this, 2);
    m_paramVsync   = m_parameterList.add("vsync|Enable or disable vsync", &m_config.vsyncstate, (ParameterList::Callback*)this);
    m_paramLog     = m_parameterList.addFilename("logfile|Set logfile", &m_config.logfile, (ParameterList::Callback*)this);
    m_parameterList.add("winpos|Set window position (x and y)", m_config.winpos, nullptr, 2);
    m_parameterList.add("frames|Set number of frames to render before exit", &m_config.frameLimit);
    m_parameterList.add("timerprints|Set number of timerprints to do, before exit", &m_config.timerLimit);
    m_parameterList.add("timerinterval|Set interval of timer prints in seconds", &m_config.intervalSeconds);
    m_parameterList.add("bmpatexit|Set file to store a bitmap image of the last frame at exit", &m_config.dumpatexit);
    m_parameterList.addFilename("benchmark|Set benchmark filename", &m_benchmark.filename);
    m_parameterList.add("benchmarkframes|Set number of benchmarkframes", &m_benchmark.frameLength);
    m_paramScreenshot = m_parameterList.add("screenshot|Set a file to store a screenshot into", &m_config.screenshot, (ParameterList::Callback*)this);
  }

  void AppWindowProfiler::exitScreenshot()
  {
    if (!m_config.dumpatexit.empty() && !m_hadScreenshot) {
      dumpScreenshot(m_config.dumpatexit.c_str(), m_window.m_viewsize[0], m_window.m_viewsize[1]);
      m_hadScreenshot = true;
    }
  }

  void AppWindowProfiler::initBenchmark()
  {
    if (m_benchmark.filename.empty()) return;

    m_benchmark.content = AssetLoadTextFile(m_benchmark.filename.c_str());
    if (!m_benchmark.content.empty()) {
      std::vector<const char*> tokens;
      ParameterList::tokenizeString(m_benchmark.content, tokens);

      std::string path = getFilePath(m_benchmark.filename.c_str());

      m_benchmark.sequence.init(&m_parameterList, tokens);

      // do first iteration manually, due to custom arg parsing
      uint32_t argBegin;
      uint32_t argCount;
      if (!m_benchmark.sequence.advanceIteration("benchmark", 1, argBegin, argCount)) {
        parseConfig(argCount, &tokens[argBegin], path);
      }

      m_profiler.reset(nv_helpers::Profiler::CONFIG_DELAY);

      m_benchmark.frame = 0;
      m_profilerPrint = false;
    }
  }

  void AppWindowProfiler::advanceBenchmark()
  {
    if (!m_benchmark.sequence.isActive()) return;

    m_benchmark.frame++;

    if (m_benchmark.frame > m_benchmark.frameLength + nv_helpers::Profiler::CONFIG_DELAY + nv_helpers::Profiler::FRAME_DELAY) {
      m_benchmark.frame = 0;

      std::string stats;
      m_profiler.print(stats);
      LOGI("BENCHMARK %d \"%s\" {\n", m_benchmark.sequence.getIteration(), m_benchmark.sequence.getSeparatorArg(0));
      LOGI("%s}\n\n", stats.c_str());

      bool done = m_benchmark.sequence.applyIteration("benchmark", 1, "-");
      m_profiler.reset(nv_helpers::Profiler::CONFIG_DELAY);

      postBenchmarkAdvance();

      if (done) {
        leave();
      }
    }
  }

}


