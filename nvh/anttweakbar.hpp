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

#ifndef NV_ANTTWEAKBAR_INCLUDED
#define NV_ANTTWEAKBAR_INCLUDED

#include <nvpwindow.hpp>
#include <AntTweakBar.h>
  
namespace nvh
{
  //////////////////////////////////////////////////////////////////////////
  // handleTwKeyPressed aids the translation of window events
  // to AntTweakVar events.

  static bool handleTwKeyPressed(int button, int action, int mods)
  {
    int twkey = button;
    switch (button){
    case NVPWindow::KEY_BACKSPACE:
      twkey = TW_KEY_BACKSPACE; break;
    case NVPWindow::KEY_ENTER:
    case NVPWindow::KEY_KP_ENTER:
      twkey = TW_KEY_RETURN; break;
    case NVPWindow::KEY_TAB: 
      twkey = TW_KEY_TAB; break;
    case NVPWindow::KEY_PAUSE: 
      twkey = TW_KEY_PAUSE; break;
    case NVPWindow::KEY_ESCAPE: 
      twkey = TW_KEY_ESCAPE; break;
    case NVPWindow::KEY_SPACE: 
      twkey = TW_KEY_SPACE; break;
    case NVPWindow::KEY_DELETE: 
      twkey = TW_KEY_DELETE; break;
    case NVPWindow::KEY_INSERT: 
      twkey = TW_KEY_INSERT; break;
    case NVPWindow::KEY_UP: 
      twkey = TW_KEY_UP; break;
    case NVPWindow::KEY_DOWN: 
      twkey = TW_KEY_DOWN; break;
    case NVPWindow::KEY_RIGHT: 
      twkey = TW_KEY_RIGHT; break;
    case NVPWindow::KEY_LEFT: 
      twkey = TW_KEY_LEFT; break;
    case NVPWindow::KEY_END: 
      twkey = TW_KEY_END; break;
    case NVPWindow::KEY_HOME: 
      twkey = TW_KEY_HOME; break;
    case NVPWindow::KEY_PAGE_UP:
      twkey = TW_KEY_PAGE_UP; break;
    case NVPWindow::KEY_PAGE_DOWN:
      twkey = TW_KEY_PAGE_DOWN; break;
    case NVPWindow::KEY_F1:
    case NVPWindow::KEY_F2:
    case NVPWindow::KEY_F3:
    case NVPWindow::KEY_F4:
    case NVPWindow::KEY_F5:
    case NVPWindow::KEY_F6:
    case NVPWindow::KEY_F7:
    case NVPWindow::KEY_F8:
    case NVPWindow::KEY_F9:
    case NVPWindow::KEY_F10:
    case NVPWindow::KEY_F11:
    case NVPWindow::KEY_F12:
    case NVPWindow::KEY_F13:
    case NVPWindow::KEY_F14:
    case NVPWindow::KEY_F15:
      twkey = TW_KEY_F1 + button - NVPWindow::KEY_F1;
      break;
    case NVPWindow::KEY_KP_0:
    case NVPWindow::KEY_KP_1:
    case NVPWindow::KEY_KP_2:
    case NVPWindow::KEY_KP_3:
    case NVPWindow::KEY_KP_4:
    case NVPWindow::KEY_KP_5:
    case NVPWindow::KEY_KP_6:
    case NVPWindow::KEY_KP_7:
    case NVPWindow::KEY_KP_8:
    case NVPWindow::KEY_KP_9:
      twkey = '0' + button - NVPWindow::KEY_KP_0;
      break;
    case NVPWindow::KEY_KP_DECIMAL:
      twkey = '.';
      break;
    }

    int twmods = 0;
    if (mods & NVPWindow::KMOD_SHIFT)
      twmods |= TW_KMOD_SHIFT;
    if (mods & NVPWindow::KMOD_ALT)
      twmods |= TW_KMOD_ALT;
    if (mods & NVPWindow::KMOD_CONTROL)
      twmods |= TW_KMOD_CTRL;
    if (mods & NVPWindow::KMOD_SUPER)
      twmods |= TW_KMOD_META;

    if (action == NVPWindow::BUTTON_PRESS || action == NVPWindow::BUTTON_REPEAT)
      return !!TwKeyPressed(twkey,twmods);
    else
      return false;
  }
}

#endif