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
*/ //--------------------------------------------------------------------

#include "nvpsystem.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef NVP_SUPPORTS_SOCKETS
#include "socketSampleMessages.h"
#endif

#include <algorithm>

static bool        s_sysInit = false;

static void cb_errorfun(int, const char* str)
{
  LOGE(str);
  LOGE("\n");
}

//---------------------------------------------------------------------------
// Message pump
void NVPSystem::pollEvents()
{
#ifdef NVP_SUPPORTS_SOCKETS
  // check the stack of messages from remote connection, first
  processRemoteMessages();
#endif
  glfwPollEvents();
}

void NVPSystem::waitEvents()
{
  glfwWaitEvents();
}

void NVPSystem::postTiming(float ms, int fps, const char* details)
{
#ifdef NVP_SUPPORTS_SOCKETS
  ::postTiming(ms, fps, details);
#endif
}


double NVPSystem::getTime()
{
  return glfwGetTime();
}


void NVPSystem::init(const char* projectName)
{
  std::string logfile = std::string("log_") + std::string(projectName) + std::string(".txt");
  nvprintSetLogFileName(logfile.c_str());

  int result = glfwInit();
  if (!result) {
    LOGE("could not init glfw\n");
    exit(-1);
  }

  glfwSetErrorCallback(cb_errorfun);

  //initNSight();
#ifdef NVP_SUPPORTS_SOCKETS
  //
  // Socket init if needed
  //
  startSocketServer(/*port*/ 1056);
#endif

  s_sysInit = true;
  platformInit();
}

void NVPSystem::deinit()
{
  platformDeinit();
  glfwTerminate();
}

bool NVPSystem::isInited() {
  return s_sysInit;
}
