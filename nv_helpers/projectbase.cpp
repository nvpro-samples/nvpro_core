/*
 * Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

#include "projectbase.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX 
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace nv_helpers
{
  static ProjectBase* s_project;

  void ProjectBase::motion(int x, int y)
  {
    ProjectBase::Window &window = m_window;

    if (!window.m_mouseButtonFlags && mouse_pos(x,y)) return;

    window.m_mouseCurrent[0] = x;
    window.m_mouseCurrent[1] = y;
  }

  void ProjectBase::mouse(MouseButton Button, ButtonAction Action, int mods, int x, int y)
  {
    ProjectBase::Window  &window   = m_window;
    m_profiler.reset();

    if (!window.m_mouseButtonFlags && mouse_button(Button,Action)) return;

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

  void ProjectBase::mousewheel(int y)
  {
    ProjectBase::Window &window = m_window;
    m_profiler.reset();

    if (mouse_wheel(y)) return;

    window.m_wheel = y;
  }

  void ProjectBase::keyboard(KeyCode key, ButtonAction action, int mods, int x, int y)
  {
    ProjectBase::Window  &window   = m_window;
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

  void ProjectBase::keyboardchar(unsigned char key, int mods, int x, int y)
  {
    m_profiler.reset();

    if (key_char(key)) return;
  }

  void ProjectBase::reshape( int width, int height )
  {
    ProjectBase::Window  &window   = m_window;
    m_profiler.reset();

    if (width == 0 && height == 0)
      return;

    window.m_viewsize[0] = width;
    window.m_viewsize[1] = height;

    resize(width,height);
  }


  void ProjectBase::vsync(bool state)
  {
    swapInterval(state ? 1 : 0);
    m_vsync = state;
    printf("vsync: %s\n", state ? "on" : "off");
  }

  void ProjectBase::waitEvents()
  {
    sysWaitEvents();
  }


  int ProjectBase::run
  (
   const std::string& title,
   int argc, const char** argv, 
   int width, int height,
   int Major, int Minor
   )
  {
    s_project = this;

#if _WIN32
    {
      HANDLE proc = GetCurrentProcess();
      size_t procmask;
      size_t sysmask;
      // pin to one physical cpu for smoother timings, disable hyperthreading
      GetProcessAffinityMask(proc,(PDWORD_PTR)&procmask,(PDWORD_PTR)&sysmask);
      if (sysmask & 8){
        // quadcore, use last core
        procmask = 8;
      }
      else if (sysmask & 2){
        // dualcore, use last core
        procmask = 2;
      }
      SetProcessAffinityMask(proc,(DWORD_PTR)procmask);
    }
#endif

    sysVisibleConsole();

    ContextFlags flags;
    flags.major = Major;
    flags.minor = Minor;
    flags.robust = 0;
    flags.core  = 0;
#ifdef NDEBUG
    flags.debug = 0;
#else
    flags.debug = 1;
#endif
    flags.share = NULL;

    if (!create(title.c_str(),&flags,width,height)){
      printf("Could not create GL context: %d.%d\n",flags.major,flags.minor);
      return EXIT_FAILURE;
    }

    m_window.m_viewsize[0] = width;
    m_window.m_viewsize[1] = height;

    vsync(true);

    m_profiler.init();

    bool Run = begin();

    double timeStart = sysGetTime();
    double timeBegin = sysGetTime();
    double frames = 0;

    bool   lastVsync = m_vsync;

    if(Run)
    {
      while(true)
      {
        if(m_window.m_keyPressed[KEY_ESCAPE])
          break;

        if (!sysPollEvents(false)){
          break;
        }

        while ( !isOpen() ){
          sysWaitEvents();
        }

        if (m_window.onPress(KEY_V)){
          vsync(!m_vsync);
        }
        
        std::string stats;
        {
          Profiler::FrameHelper helper(m_profiler,sysGetTime(), 2.0, stats);
          {
            PROFILE_SECTION("Frame");
            think(sysGetTime() - timeStart);
          }
          memset(m_window.m_keyToggled, 0, sizeof(m_window.m_keyToggled)); 
          swapBuffers();
        }
        if (m_profilerPrint && !stats.empty()){
          fprintf(stdout,"%s\n",stats.c_str());
        }

        frames++;

        double timeCurrent = sysGetTime();
        double timeDelta = timeCurrent - timeBegin;
        if (timeDelta > 2.0 || lastVsync != m_vsync){
          std::ostringstream combined; 

          if (lastVsync != m_vsync){
            timeDelta = 0;
          }

          combined << title << ": " << (timeDelta*1000.0/(frames)) << " [ms]" << (m_vsync ? " (vsync on - V for toggle)" : "");

          setTitle(combined.str().c_str());

          frames = 0;
          timeBegin = timeCurrent;
          lastVsync = m_vsync;
        }
      }
    }
    end();

    return Run ? EXIT_SUCCESS : EXIT_FAILURE;
  }

}


