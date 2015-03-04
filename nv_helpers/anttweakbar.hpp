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

#ifndef NV_ANTTWEAKBAR_INCLUDED
#define NV_ANTTWEAKBAR_INCLUDED

#include <main.h>
#include <AntTweakBar.h>
  
namespace nv_helpers
{
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