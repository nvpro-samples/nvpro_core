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

static std::string s_path;

std::vector<NVPWindow *> g_windows;

bool NVPWindow::create(int posX, int posY, int width, int height, const char* title)
{
  m_windowSize[0] = width;
  m_windowSize[1] = height;
  m_internal = newWINinternal(this);

  m_windowName = title ? title : "Sample";

  if (m_internal->create(posX, posY, width, height, m_windowName.c_str()))
  {
    // Keep track of the windows
    g_windows.push_back(this);
    return true;
  }

  delete m_internal;
  m_internal = NULL;
  
  return false;
}

void NVPWindow::destroy()
{
  m_windowSize[0] = 0;
  m_windowSize[1] = 0;
  m_internal->destroy();
  delete m_internal;
  m_internal = NULL;

  m_windowName = "Sample";

  for(auto it = g_windows.begin(); it < g_windows.end(); it++)
    if(*it == this) {
      g_windows.erase(it);
      break;
    }
}

void NVPWindow::setTitle( const char* title )
{
    m_internal->setTitle(title);
}

void NVPWindow::maximize()
{
  m_internal->maximize();
}

void NVPWindow::restore()
{
  m_internal->restore();
}

void NVPWindow::minimize()
{
  m_internal->minimize();
}

bool NVPWindow::isOpen()
{
  return m_internal->m_visible && !m_internal->m_iconified;
}

void NVPWindow::setWindowPos(int x, int y, int w, int h)
{
    m_internal->setWindowPos(x,y,w,h);
}

void NVPWindow::setFullScreen(bool bYes)
{
    m_internal->setFullScreen(bYes);
    m_isFullScreen = bYes;

}

void NVPWindow::screenshot(const char* filename)
{
  m_internal->screenshot(filename);
}

//---------------------------------------------------------------------------
// Message pump
bool NVPWindow::sysPollEvents(bool bLoop)
{
    bool bContinue;
    do {
#ifdef USESOCKETS
		// check the stack of messages from remote connection, first
        processRemoteMessages();
#endif
        // hack to get to the allocated implementation for the right platform
        bContinue = NVPWindowInternal::sysPollEvents();
    } while( bContinue && bLoop );
    return bContinue;
}

void NVPWindow::sysWaitEvents()
{
    NVPWindowInternal::sysWaitEvents();
}

void NVPWindow::sysPostQuit()
{
  NVPWindowInternal::sysPostQuit();
}

void NVPWindow::sysPostTiming(float ms, int fps, const char *details)
{
#ifdef USESOCKETS
    ::postTiming(ms, fps, details);
#endif
}


double NVPWindow::sysGetTime()
{
    return NVPWindowInternal::sysGetTime();
}

void NVPWindow::sysSleep(double seconds)
{
  NVPWindowInternal::sysSleep(seconds);
}

void NVPWindow::sysInit(const char* exeFileName, const char* projectName)
{
  std::string logfile;
  logfile = std::string("log_") + std::string(projectName) + std::string(".txt");
  nvprintSetLogFileName(logfile.c_str() );

  std::string exe = exeFileName;
  std::replace(exe.begin(),exe.end(),'\\','/');

  size_t last = exe.rfind('/');
  if (last != std::string::npos){
    s_path = exe.substr(0,last) + std::string("/");
  }

  NVPWindowInternal::sysInit();

  //initNSight();
#ifdef USESOCKETS
	//
	// Socket init if needed
	//
	startSocketServer(/*port*/1056);
#endif
}
void NVPWindow::sysDeinit()
{
  //
  // Terminate
  //
  for(size_t i=0; i<g_windows.size(); i++)
  {
    NVPWindow *pWin = g_windows[i];
    // pWin->shutdown(); // This might have already been called by the WM_DESTROY or equivalent in Linux etc.
    if (pWin->m_internal){
      pWin->m_internal->destroy();
      delete pWin->m_internal;
      pWin->m_internal = NULL;
    }
  }
  g_windows.clear();
  NVPWindowInternal::sysDeinit();
}
std::string NVPWindow::sysExePath()
{
  return s_path;
}
