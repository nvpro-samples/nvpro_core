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

#include "socketSamplemessages.hpp"

#include <nvpwindow.hpp>
extern std::vector<NVPWindow *> g_windows;

typedef std::stack<Message> MsgStack;

MsgStack g_msgStackIn;
sock::CCriticalSection* csTraceUpdate = NULL;
CServer server(1056); // the one client : where the experiment is happening

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// specialized Thread for listening to the messages coming from sockets.
// this thread will then push the messages to a stack that can be read 
// by the message pump of the sample
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void CServer::CThreadProc()
{
    short msgBuf[MSG_MAX_SZ];
    unsigned char *splilledData = NULL;
    int l;

    while (1)
    {
        if((l=recieve(msgBuf, MSG_MAX_SZ, &splilledData)) == -1)
        {
            Sleep(1000);
        }
        else
        {
            //LOGI("Token %c...", data->token);
            Message msg((char)msgBuf[0], curWindowID);
            switch(msgBuf[0])
            {
            case DG_MSG:
                {
                    SockMsgText* pMsgText = (SockMsgText*)msgBuf;
                    LOGI("Message: %s", pMsgText->txt);
                }
                break;
            case DG_CURWINDOW:
                {
                    SockMsgMisc* pMsgMisc = (SockMsgMisc*)msgBuf;
                    LOGI("Window focus: %d", pMsgMisc->data.iVal);
					curWindowID = pMsgMisc->data.iVal;
                }
                break;
			default:
                {
				// this parts transmits the messages to the sample: copies all the data following the token
				// note that the Socket data don't match what will be pushed in the stack of messages
				// the pushed messages are more restrictive than SBuffer
                SockMsgMisc* pMsgMisc = (SockMsgMisc*)msgBuf;
				memcpy(&msg.data, &pMsgMisc->data, sizeof(MessageData));
				csTraceUpdate->Enter();
				{
					g_msgStackIn.push(msg);
				}
				csTraceUpdate->Exit();
                }
			}
        }
        if(splilledData)
        {
            free(splilledData);
            splilledData = NULL;
        }
        Sleep(1);
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//
// functions to initialize, terminate then process socket messages for samples
//
/////////////////////////////////////////////////////////////////////////////////////////////////

void startSocketServer(int port)
{
    LOGI("Strating Socket thread\n");
    csTraceUpdate = new sock::CCriticalSection;
    server.Init(port);
}

bool endSocketServer()
{
    LOGI("Terminating Socket thread\n");
    server.Close();
    if(csTraceUpdate)
    {
        delete csTraceUpdate;
        csTraceUpdate = NULL;
    }
    return true;
}

void processRemoteMessages()
{
	if(csTraceUpdate == NULL) // no mutex means the remote stuff is not initialized yet
		return;

	csTraceUpdate->Enter();
    while(g_msgStackIn.size() > 0)
    {
      const Message &msg = g_msgStackIn.top();

		  // get back the correct window
		  int index = (int)msg.windowID;
		  NVPWindow *pWin = NULL;
		  if(g_windows.size() > 0)
      {
			  pWin = g_windows[index];
        switch(msg.token)
        {
        case DG_TIMING:
			      pWin->requestTiming();
            break;
        case DG_QUIT:
            NVPWindow::sysPostQuit();
            break;
        case DG_FULLSCREEN:
			      pWin->setFullScreen(msg.data.fullscreen.bYes?true:false);
            break;
		case DG_PAINT:
            pWin->requestPaint();
            break;
		case DG_CONTINUOUS_REFRESH:
            pWin->requestContinuousRefresh(true);
            break;
		case DG_NO_REFRESH:
			      pWin->requestContinuousRefresh(false);
            break;
		case DG_SCREENSHOT:
			      pWin->screenshot("Socket_capture");//msg.data.screenshot.idx, msg.data.screenshot.x, msg.data.screenshot.y, msg.data.screenshot.w, msg.data.screenshot.h);
            break;
		case DG_RESIZE:
			      pWin->setWindowPos(msg.data.resize.x, msg.data.resize.y, msg.data.resize.w, msg.data.resize.h);
            break;
		case DG_KEYPRESS:
			      pWin->keyboardchar(msg.data.keychar.key, msg.data.keychar.mods, msg.data.keychar.x, msg.data.keychar.y);
            break;
		case DG_KEYCODE:
			      pWin->keyboard((NVPWindow::KeyCode)msg.data.keycode.key, (NVPWindow::ButtonAction)msg.data.keycode.action, msg.data.keycode.mods, msg.data.keycode.x, msg.data.keycode.y);
            break;
		case DG_FARG4:
			      pWin->requestSetArg(msg.data.farg4.tokenArg, msg.data.farg4.v[0], msg.data.farg4.v[1], msg.data.farg4.v[2], msg.data.farg4.v[3]);
            break;
		case DG_IARG4:
			      pWin->requestSetArg(msg.data.iarg4.tokenArg, msg.data.iarg4.v[0], msg.data.iarg4.v[1], msg.data.iarg4.v[2], msg.data.iarg4.v[3]);
            break;
		case DG_MINIMIZE:
			      pWin->minimize();
            break;
		case DG_MAXIMIZE:
			      pWin->maximize();
            break;
		case DG_MOUSEPOS:
			      pWin->motion(msg.data.mouse.x, msg.data.mouse.y);
            break;
		case DG_LEFTCLICK:
			      pWin->mouse(NVPWindow::MOUSE_BUTTON_LEFT, (NVPWindow::ButtonAction)msg.data.mouseclick.action, msg.data.mouseclick.mods, msg.data.mouseclick.x, msg.data.mouseclick.y);
            break;
		case DG_RIGHTCLICK:
			      pWin->mouse(NVPWindow::MOUSE_BUTTON_RIGHT, (NVPWindow::ButtonAction)msg.data.mouseclick.action, msg.data.mouseclick.mods, msg.data.mouseclick.x, msg.data.mouseclick.y);
            break;
		case DG_MIDDLECLICK:
			      pWin->mouse(NVPWindow::MOUSE_BUTTON_MIDDLE, (NVPWindow::ButtonAction)msg.data.mouseclick.action, msg.data.mouseclick.mods, msg.data.mouseclick.x, msg.data.mouseclick.y);
            break;
		case DG_MOUSEWHEEL:
			      pWin->mousewheel(msg.data.mousewheel.val);
            break;
        }
    }
        g_msgStackIn.pop();
    }
    csTraceUpdate->Exit();
}

void postScreenshot(unsigned char* data, size_t sz, int w, int h)
{
    int totalsz = (int)(sizeof(SockMsgScreenshotImage) + sz - 1);
    SockMsgScreenshotImage *pBuff = (SockMsgScreenshotImage *)malloc(totalsz);
    pBuff->token = DG_SCREENSHOT_IMAGE;
    pBuff->sz = totalsz;
    pBuff->size = (unsigned int)sz;
    pBuff->w = w;
    pBuff->h = h;
    memcpy(pBuff->b, data, sz);
    server.tcpSendToCurrent((char*)pBuff, totalsz);
    free(pBuff);
}

void postTiming(float ms, int fps, const char *details)
{
    SockMsgTiming *pBuff = NULL;
    int len = 0;
    if(details)
        len = (int)strlen(details);
    len += sizeof(SockMsgHeader) + sizeof(SockMsgTiming);
    pBuff = (SockMsgTiming*)malloc(len);
    pBuff->token = DG_TIMING;
    pBuff->sz = len;
    pBuff->ms = ms;
    pBuff->fps = fps;
    if(details)
        strcpy(pBuff->timingMsg, details);
    server.tcpSendToCurrent((char*)pBuff, pBuff->sz);
}
