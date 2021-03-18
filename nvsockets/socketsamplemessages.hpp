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

#include <stack>

#include "socketserver.hpp"

#define MSG_MAX_SZ 1000
#define DG_REGISTER     'A' // send by client to register
#define DG_UNREGISTER   'U' // send by client to unregister
#define DG_MSG          'B'
#define DG_TIMING       'C'
#define DG_QUIT         'D'
#define DG_FULLSCREEN   'E'
#define DG_PAINT        'F'
#define DG_CONTINUOUS_REFRESH 'G'
#define DG_NO_REFRESH	'H'
#define DG_CURWINDOW	'I'
#define DG_SCREENSHOT	'J'
#define DG_RESIZE		'K'
#define DG_KEYPRESS		'L'
#define DG_FARG4        'M'
#define DG_IARG4        'N'
#define DG_MINIMIZE     'O'
#define DG_MAXIMIZE     'P'
#define DG_MOUSEPOS     'Q'
#define DG_LEFTCLICK    'R'
#define DG_RIGHTCLICK   'S'
#define DG_MIDDLECLICK  'T'
#define DG_MOUSEWHEEL   'U'
#define DG_KEYCODE		'V'
#define DG_SCREENSHOT_IMAGE	'W'

struct SockMsgHeader {
    short   token;
    int sz;
};

union MiscData {
	unsigned int uiVal;
	int iVal;
	float fVal;
	int iVec4[4];
	unsigned int uiVec4[4];
	float fVec4[4];
};

struct SockMsgMisc : SockMsgHeader {
    SockMsgMisc(short _token) { 
        token = _token;
        sz = sizeof(SockMsgMisc);
    }
    MiscData data;
};

struct SockMsgText : SockMsgHeader {
    SockMsgText() { 
        token = DG_MSG;
        sz = sizeof(SockMsgText);
        txt[0] = '\0';
    }
    char txt[1];
};
	
struct DataTiming {
    float ms;
	int   fps;
	char timingMsg[1];
};
struct SockMsgTiming : SockMsgHeader, DataTiming {
    SockMsgTiming() { 
        token = DG_TIMING;
        sz = sizeof(SockMsgTiming);
        timingMsg[0] = '\0';
    }
};
	
struct DataFArg4 {
	int tokenArg;
	float v[4];
};
struct SockMsgFArg4 : SockMsgHeader, DataFArg4 {
    SockMsgFArg4() { 
        token = DG_FARG4;
        sz = sizeof(SockMsgFArg4);
    }
};

struct DataIArg4 {
	int tokenArg;
	int v[4];
};
struct SockMsgIArg4 : SockMsgHeader, DataIArg4 {
    SockMsgIArg4() { 
        token = DG_IARG4;
        sz = sizeof(SockMsgIArg4);
    }
};

struct DataScreenshotImage {
    unsigned int size;
    short w,h;
    unsigned char b[1];
};
struct SockMsgScreenshotImage : SockMsgHeader, DataScreenshotImage {
    SockMsgScreenshotImage() { 
        token = DG_SCREENSHOT_IMAGE;
        sz = sizeof(SockMsgScreenshotImage);
    }
};

struct DataScreenshot {
    int idx;
    short x,y;
    short w,h;
};
struct SockMsgScreenshot : SockMsgHeader, DataScreenshot {
    SockMsgScreenshot(int idx_=0, short x_=0, short y_=0, short w_=0, short h_=0)
    { 
        token = DG_SCREENSHOT;
        x = x_; y = y_; idx = idx_; w = w_; h = h_;
        sz = sizeof(SockMsgScreenshot);
    }
};

struct DataResize {
    int x,y;
    int w,h;
};
struct SockMsgResize : SockMsgHeader, DataResize {
    SockMsgResize() { 
        token = DG_RESIZE;
        sz = sizeof(SockMsgResize);
    }
};

struct DataKeyChar {
    int key;
    int mods;
    int x, y;
};
struct SockMsgKeyChar : SockMsgHeader, DataKeyChar {
    SockMsgKeyChar() { 
        token = DG_KEYPRESS;
        sz = sizeof(SockMsgKeyChar);
    }
};

struct DataKeyCode {
    int key;
    int action;
    int mods;
    int x, y;
};
struct SockMsgKeyCode : SockMsgHeader, DataKeyCode {
    SockMsgKeyCode() { 
        token = DG_KEYCODE;
        sz = sizeof(SockMsgKeyCode);
    }
};

struct DataMouse {
    int x, y;
};
struct SockMsgMouse : SockMsgHeader, DataMouse {
    SockMsgMouse() { 
        token = DG_MOUSEPOS;
        sz = sizeof(SockMsgMouse);
    }
};

struct DataMouseClick {
    int action;
    int mods;
    int x, y;
};
struct SockMsgMouseLClick : SockMsgHeader, DataMouseClick {
    SockMsgMouseLClick() { 
        token = DG_LEFTCLICK;
        sz = sizeof(SockMsgMouseLClick);
    }
};
struct SockMsgMouseRClick : SockMsgHeader, DataMouseClick {
    SockMsgMouseRClick() { 
        token = DG_RIGHTCLICK;
        sz = sizeof(SockMsgMouseRClick);
    }
};
struct SockMsgMouseMClick : SockMsgHeader, DataMouseClick {
    SockMsgMouseMClick() { 
        token = DG_MIDDLECLICK;
        sz = sizeof(SockMsgMouseMClick);
    }
};

struct DataMouseWheel {
    int val;
};
struct SockMsgMouseWheel : SockMsgHeader, DataMouseWheel {
    SockMsgMouseWheel() { 
        token = DG_MOUSEWHEEL;
        sz = sizeof(SockMsgMouseWheel);
    }
};

struct DataFullscreen {
    int bYes;
};
struct SockMsgFullscreen : SockMsgHeader, DataFullscreen {
    SockMsgFullscreen() { 
        token = DG_FULLSCREEN;
        sz = sizeof(SockMsgFullscreen);
        bYes = true;
    }
};

union MessageData {
    MiscData   misc;
    DataTiming timing;
    DataFArg4  farg4;
    DataIArg4  iarg4;
    DataScreenshot screenshot;
    DataScreenshotImage screenshotImage;
    DataResize resize;
    DataKeyChar keychar;
    DataKeyCode keycode;
    DataMouse mouse;
    DataMouseWheel mousewheel;
    DataMouseClick mouseclick;
    DataFullscreen fullscreen;
};

//
// Message is what is put in the stack of messages for the main process to read them
// NOTE: only messages with a max size of the union is accepted.
// No message with undefined size can be put here (Text, screenshot, timing with additional text...)
// reason: we don't do fancy allocation... simple enough for now.
//
struct Message
{
	Message(char _token, char _windowID) { memset(this, 0, sizeof(Message)); token = _token; windowID = _windowID; }
    char token;
	char windowID;
    MessageData data;
};

class CServer : public CBaseServer
{
private:
	int     port;
	int		curWindowID;
public:
    CServer(int _port) : CBaseServer(), CThread(/*startNow*/false, /*Critical*/false)
    {
		port = _port; 
		curWindowID = 0;
	}
    virtual void CThreadProc();
};

void startSocketServer(int port);
bool endSocketServer();
void processRemoteMessages();
void postScreenshot(unsigned char* data, size_t sz, int w, int h);
void postTiming(float ms, int fps, const char *details);