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


#define GLX_GLXEXT_PROTOTYPES
#define DECL_WININTERNAL

#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glxext.h>
#include<GL/glu.h>

#include<X11/Xlib.h>
#include<X11/Xatom.h>
#include<X11/keysym.h>
#include<X11/extensions/xf86vmode.h>

#include"main.h"
#include<stdio.h>
#include<fcntl.h>
#include<iostream>
#include<fstream>
#include<algorithm>
#include<string>
#include<stdarg.h>
#include<unistd.h>
#include<sys/timeb.h>
#include <sys/time.h>

Display *g_dpy = 0;

std::vector<NVPWindow*> g_windows;

XEvent uMsg;

typedef GLXContext(*glXCreateContextAttribsARBProc)(Display *,GLXFBConfig, GLXContext,Bool, const int*);

static int attrListDbl[] = {
    GLX_RGBA,GLX_DOUBLEBUFFER,
    GLX_RED_SIZE,4,
    GLX_GREEN_SIZE,4,
    GLX_BLUE_SIZE,4,
    GLX_DEPTH_SIZE,16,0
};

 
WINinternal *newWINinternal(NVPWindow *win){
  return new WINinternal(win);
}



static int getKeyMods(XEvent &evt){

    int mods = 0;

    if(evt.xkey.state & ShiftMask){
        mods |= NVPWindow::KMOD_SHIFT;
    }
    if(evt.xkey.state & ControlMask ){
        mods |= NVPWindow::KMOD_CONTROL;
    }
    if(evt.xkey.state & Mod1Mask){
        mods |= NVPWindow::KMOD_ALT;
    }

    return mods;
}

static int translateKey(XEvent &evt){

    if(evt.type != KeyPress && evt.type != KeyRelease) return 0;

    unsigned int key = evt.xkey.keycode;


    KeySym ksym = XLookupKeysym(&evt.xkey,0);


    switch(ksym){

        /*
            XLib has separate key symbols for left and right
            verions of mod keys. 
        */
        case XK_Shift_L:
            return NVPWindow::KEY_LEFT_SHIFT;
        break;

        case XK_Shift_R:
            return NVPWindow::KEY_RIGHT_SHIFT;
        break;

        case XK_Control_L:
            return NVPWindow::KEY_LEFT_CONTROL;
        break;

        case XK_Control_R:
            return NVPWindow::KEY_RIGHT_CONTROL;
        break;

        case XK_Alt_L:
            return NVPWindow::KEY_LEFT_ALT;
        break;

        case XK_Alt_R:
            return NVPWindow::KEY_RIGHT_ALT;
        break;

        case XK_Return:
            return NVPWindow::KEY_ENTER;
        break;

        case XK_Escape:
            return NVPWindow::KEY_ESCAPE;
        break;

        case XK_Tab:
            return NVPWindow::KEY_TAB;
        break;

        case XK_BackSpace:
            return NVPWindow::KEY_BACKSPACE;
        break;

        case XK_Home:
            return NVPWindow::KEY_HOME;
        break;

        case XK_End:
            return NVPWindow::KEY_END;
        break;

        case XK_Prior:
            return NVPWindow::KEY_PAGE_UP;
        break;

        case XK_Next:
            return NVPWindow::KEY_PAGE_DOWN;
        break;

        case XK_Insert:
            return NVPWindow::KEY_INSERT;
        break;

        case XK_Delete:
            return NVPWindow::KEY_DELETE;
        break;

        case XK_Left:
            return NVPWindow::KEY_LEFT;
        break;

        case XK_Right:
            return NVPWindow::KEY_RIGHT;
        break;

        case XK_Up:
            return NVPWindow::KEY_UP;
        break;

        case XK_Down:
            return NVPWindow::KEY_DOWN;
        break;

        case XK_F1:
            return NVPWindow::KEY_F1;
        break;

        case XK_F2:
            return NVPWindow::KEY_F2;
        break;

        case XK_F3:
            return NVPWindow::KEY_F3;
        break;

        case XK_F4:
            return NVPWindow::KEY_F4;
        break;

        case XK_F5:
            return NVPWindow::KEY_F5;
        break;

        case XK_F6:
            return NVPWindow::KEY_F6;
        break;

        case XK_F7:
            return NVPWindow::KEY_F7;
        break;

        case XK_F8:
            return NVPWindow::KEY_F8;
        break;

        case XK_F9:
            return NVPWindow::KEY_F9;
        break;

        case XK_F10:
            return NVPWindow::KEY_F10;
        break;

       case XK_F11:
            return NVPWindow::KEY_F11;
        break;

       case XK_F12:
            return NVPWindow::KEY_F12;
        break;

       case XK_F13:
            return NVPWindow::KEY_F13;
        break;

       case XK_F14:
            return NVPWindow::KEY_F14;
        break;

       case XK_F15:
            return NVPWindow::KEY_F15;
        break;

       case XK_F16:
            return NVPWindow::KEY_F16;
        break;

       case XK_F17:
            return NVPWindow::KEY_F17;
        break;

       case XK_F18:
            return NVPWindow::KEY_F18;
        break;

       case XK_F19:
            return NVPWindow::KEY_F19;
        break;

       case XK_F20:
            return NVPWindow::KEY_F20;
        break;

        case XK_Num_Lock:
            return NVPWindow::KEY_NUM_LOCK;
        break;

        case XK_Caps_Lock:
            return NVPWindow::KEY_CAPS_LOCK;
        break;

        case XK_Scroll_Lock:
            return NVPWindow::KEY_SCROLL_LOCK;
        break;

        case XK_Pause:
            return NVPWindow::KEY_PAUSE;
        break;

        case XK_KP_0:
            return NVPWindow::KEY_KP_0;
        break;

        case XK_KP_1:
            return NVPWindow::KEY_KP_1;
        break;

        case XK_KP_2:
            return NVPWindow::KEY_KP_2;
        break;

        case XK_KP_3:
            return NVPWindow::KEY_KP_3;
        break;

        case XK_KP_4:
            return NVPWindow::KEY_KP_4;
        break;

        case XK_KP_5:
            return NVPWindow::KEY_KP_5;
        break;

        case XK_KP_6:
            return NVPWindow::KEY_KP_6;
        break;

        case XK_KP_7:
            return NVPWindow::KEY_KP_7;
        break;

        case XK_KP_8:
            return NVPWindow::KEY_KP_8;
        break;

        case XK_KP_9:
            return NVPWindow::KEY_KP_9;
        break;

        case XK_KP_Divide:
            return NVPWindow::KEY_KP_DIVIDE;
        break;

        case XK_KP_Multiply:
            return NVPWindow::KEY_KP_MULTIPLY;
        break;

        case XK_KP_Subtract:
            return NVPWindow::KEY_KP_SUBTRACT;
        break;

        case XK_KP_Add:
            return NVPWindow::KEY_KP_ADD;
        break;

        case XK_KP_Decimal:
            return NVPWindow::KEY_KP_DECIMAL;
        break;

        case XK_space:
            return NVPWindow::KEY_SPACE;
        break;

        case XK_0:
            return NVPWindow::KEY_0;
        break;

       case XK_1:
            return NVPWindow::KEY_1;
        break;

       case XK_2:
            return NVPWindow::KEY_2;
        break;

       case XK_3:
            return NVPWindow::KEY_3;
        break;

       case XK_4:
            return NVPWindow::KEY_4;
        break;

       case XK_5:
            return NVPWindow::KEY_5;
        break;

       case XK_6:
            return NVPWindow::KEY_6;
        break;

       case XK_7:
            return NVPWindow::KEY_7;
        break;

       case XK_8:
            return NVPWindow::KEY_8;
        break;

       case XK_9:
            return NVPWindow::KEY_9;
        break;

        case XK_a:
            return NVPWindow::KEY_A;
        break;

       case XK_b:
            return NVPWindow::KEY_B;
        break;

       case XK_c:
            return NVPWindow::KEY_C;
        break;

       case XK_d:
            return NVPWindow::KEY_D;
        break;

       case XK_e:
            return NVPWindow::KEY_E;
        break;

       case XK_f:
            return NVPWindow::KEY_F;
        break;

       case XK_g:
            return NVPWindow::KEY_G;
        break;

       case XK_h:
            return NVPWindow::KEY_H;
        break;

       case XK_i:
            return NVPWindow::KEY_I;
        break;

       case XK_j:
            return NVPWindow::KEY_J;
        break;

       case XK_k:
            return NVPWindow::KEY_K;
        break;

       case XK_l:
            return NVPWindow::KEY_L;
        break;

       case XK_m:
            return NVPWindow::KEY_M;
        break;

       case XK_n:
            return NVPWindow::KEY_N;
        break;

       case XK_o:
            return NVPWindow::KEY_O;
        break;

       case XK_p:
            return NVPWindow::KEY_P;
        break;

       case XK_q:
            return NVPWindow::KEY_Q;
        break;

       case XK_r:
            return NVPWindow::KEY_R;
        break;

       case XK_s:
            return NVPWindow::KEY_S;
        break;

       case XK_t:
            return NVPWindow::KEY_T;
        break;

       case XK_u:
            return NVPWindow::KEY_U;
        break;

       case XK_v:
            return NVPWindow::KEY_V;
        break;

       case XK_w:
            return NVPWindow::KEY_W;
        break;

       case XK_x:
            return NVPWindow::KEY_X;
        break;

       case XK_y:
            return NVPWindow::KEY_Y;
        break;

       case XK_z:
            return NVPWindow::KEY_Z;
        break;

        case XK_minus:
            return NVPWindow::KEY_MINUS;
        break;

        case XK_equal:
            return NVPWindow::KEY_EQUAL;
        break;

        case XK_bracketleft:
            return NVPWindow::KEY_LEFT_BRACKET;
        break;

        case XK_bracketright:
            return NVPWindow::KEY_RIGHT_BRACKET;
        break;

        case XK_backslash:
            return NVPWindow::KEY_BACKSLASH;
        break;

        case XK_semicolon:
            return NVPWindow::KEY_SEMICOLON;
        break;

        case XK_comma:
            return NVPWindow::KEY_COMMA;
        break;

        case XK_period:
            return NVPWindow::KEY_PERIOD;
        break;


        default:
        break;
    }



    /**************/



    return NVPWindow::KEY_UNKNOWN;
}

bool NVPWindow::activate(int width, int height, const char *title, const ContextFlags *cflags, int invisible){


    return create(title,cflags,width,height);
}

void NVPWindow::deactivate(){

}

/******************************************************
Create For Open GL
******************************************************/

bool NVPWindow::create(const char *title, const ContextFlags *cflags, int width, int height){


#ifdef USEOPENGL
  
  printf("Creating Open GL Window.\n");

  m_winSz[0] = width;
  m_winSz[1] = height;

  m_internal = newWINinternalGL(this);
  
  m_debugTitle = title ? title : "Sample";
  
  if(m_internal->create(m_debugTitle.c_str(), width,height,cflags->MSAA)){
    g_windows.push_back(this);
    //TODO - Handle Update Window
    
    printf("calling init base.\n");
    if(m_internal->initBase(cflags,this)){
      printf("Init base returned true.\n");
      if(init()){
	//TODO - Handle Show Window
	return true;
      }
    }
  }

  delete m_internal;
  m_internal = NULL;
#endif
  
  return false;

}

bool NVPWindow::create(const char *title, int width, int height){

    m_winSz[0] = width;
    m_winSz[1] = height;

    m_internal = new WINinternal(this);

    m_debugTitle = title ? title:"Sample";

    if(m_internal->create(m_debugTitle.c_str(),width,height)){

        g_windows.push_back(this);

        /*
            Update and draw.
        */

            if(init()){
                //show window
                return true;
            }else{
                printf("Init Failed.\n");
            }


    }

    delete m_internal;
    m_internal = NULL;
    return false;
}

void NVPWindow::postQuit(){
    //handle X11 quit.
}

void NVPWindow::swapBuffers(){
    WINinternal *win = m_internal;
    glXSwapBuffers(win->m_dpy,win->m_window);
}

void NVPWindow::setTitle(const char *title){
  
  //XStoreName(m_internal->m_dpy,m_internal->m_window,title);
    XSetStandardProperties(m_internal->m_dpy,m_internal->m_window,title,title,None,NULL,0,NULL);
    printf("Setting Title : %s.\n",title);
}

void NVPWindow::maximize(){
    //handle maximize
  printf("Maximizing Window.\n");
}

void NVPWindow::restore(){
    //handle restore
  printf("Restoring Window.\n");
}

void NVPWindow::minimize(){
    //handle minimize
  printf("Minimizing Window.\n");
}

bool NVPWindow::isOpen(){
    return m_internal->m_visible;
}

void NVPWindow::makeContextCurrent(){
    glXMakeCurrent(m_internal->m_dpy,m_internal->m_window,m_internal->m_glx_context);
}

void NVPWindow::makeContextNonCurrent(){
    glXMakeCurrent(m_internal->m_dpy,None,NULL);
}

void NVPWindow::swapInterval(int i){
    //do nothing.
  //    glXSwapIntervalEXT(m_internal->m_dpy,m_internal->m_window,i);
}


bool NVPWindow::sysPollEvents(bool bLoop){

    bool done = false;
    XEvent event;

   do{



    for(uint32_t i=0;i<g_windows.size();++i){

        NVPWindow *pWin = g_windows[i];
         
        while(XPending(pWin->m_internal->m_dpy) > 0){

            XNextEvent(pWin->m_internal->m_dpy,&event);

            switch(event.type){
                case Expose:
                break;

                case GraphicsExpose:

                break;

                case ConfigureNotify:

                   pWin->setWinSz(event.xconfigure.width, event.xconfigure.height);
		   pWin->reshape(event.xconfigure.width, event.xconfigure.height);

                break;


           

                case ButtonPress:{

        
                    if((event.xbutton.button == Button1) ){
                        pWin->setCurMouse(event.xbutton.x,event.xbutton.y);
                        pWin->mouse(NVPWindow::MOUSE_BUTTON_LEFT,NVPWindow::BUTTON_PRESS,pWin->getMods(),pWin->getCurX(),pWin->getCurY());
                        break;
                    }else

                    if((event.xbutton.button == Button2) ){
                        pWin->setCurMouse(event.xbutton.x,event.xbutton.y);
                        pWin->mouse(NVPWindow::MOUSE_BUTTON_RIGHT,NVPWindow::BUTTON_PRESS,pWin->getMods(),pWin->getCurX(),pWin->getCurY());
                        break;
                    }else

                    if((event.xbutton.button == Button3) ){
                        pWin->setCurMouse(event.xbutton.x,event.xbutton.y);
                        pWin->mouse(NVPWindow::MOUSE_BUTTON_MIDDLE,NVPWindow::BUTTON_PRESS,pWin->getMods(),pWin->getCurX(),pWin->getCurY());
                        break;
                    }

                    /*
                        There is no notion of mouse wheel in XLib.
                        instead, the OS maps mouse wheel up/down
                        to Buttons 4 and 5.

                    */

                    static short mouseWheelScale = 5;

                       if((event.xbutton.button == Button4)){
                        pWin->mousewheel(-mouseWheelScale);
                        break;
                    }

                      
                    if((event.xbutton.button == Button5)){
                        pWin->mousewheel(mouseWheelScale);
                    }


                }break;

                case ButtonRelease:{

                  

                    if((event.xbutton.button ) == Button1){
                        pWin->setCurMouse(event.xbutton.x,event.xbutton.y);
                        pWin->mouse(NVPWindow::MOUSE_BUTTON_LEFT,NVPWindow::BUTTON_RELEASE,pWin->getMods(),pWin->getCurX(),pWin->getCurY());
                        break;
                    }else

                    if((event.xbutton.button ) == Button2){
                        pWin->setCurMouse(event.xbutton.x,event.xbutton.y);
                        pWin->mouse(NVPWindow::MOUSE_BUTTON_RIGHT,NVPWindow::BUTTON_RELEASE,pWin->getMods(),pWin->getCurX(),pWin->getCurY());
                        break;
                    }else

                    if((event.xbutton.button ) == Button3){
                        pWin->setCurMouse(event.xbutton.x,event.xbutton.y);
                        pWin->mouse(NVPWindow::MOUSE_BUTTON_MIDDLE,NVPWindow::BUTTON_RELEASE,pWin->getMods(),pWin->getCurX(),pWin->getCurY());
                        break;
                    }

                  

              
                   

                }break;

                case MotionNotify:
                        pWin->setCurMouse(event.xmotion.x,event.xmotion.y);
                        pWin->motion(pWin->getCurX(),pWin->getCurY());
               break;

                case KeyPress:{

                    int translatedKeyCode = translateKey(event);
                    if(translatedKeyCode == NVPWindow::KEY_UNKNOWN) break;

                    pWin->setMods(getKeyMods(event));

                    pWin->keyboard((NVPWindow::KeyCode)translatedKeyCode,NVPWindow::BUTTON_PRESS,pWin->getMods(),pWin->getCurX(),pWin->getCurY());
              
              }break;

                case KeyRelease:{

                    int translatedKeyCode = translateKey(event);
                    if(translatedKeyCode == NVPWindow::KEY_UNKNOWN) break;

                    pWin->setMods(getKeyMods(event));

                    pWin->keyboard((NVPWindow::KeyCode)translatedKeyCode,NVPWindow::BUTTON_RELEASE,pWin->getMods(),pWin->getCurX(),pWin->getCurY());


                }break;

               

                case ClientMessage:
                    if(strcmp(XGetAtomName(pWin->m_internal->m_dpy,event.xclient.message_type),"WM_PROTOCOLS") == 0){
                        pWin->shutdown();
                        done = True;
                    }
                 break;

            
            }
        }
        XSync(pWin->m_internal->m_dpy,True);
      }

   }while(!done && bLoop);


   return !done;

}


int NVPWindow::sysExtensionSupported(const char *name){
  
  NVPWindow *win = g_windows[0];
  return win->m_internal->sysExtensionSupported(name);
}

NVPWindow::NVPproc NVPWindow::sysGetProcAddress(const char *name){
  NVPWindow *win = g_windows[0];
  return win->m_internal->sysGetProcAddress(name);
}


void NVPWindow::display(){
  
  m_internal->display();
}


void NVPWindow::sysWaitEvents(){
    //Handle wait for XEvents.
  
}


static double s_frequency;
double NVPWindow::sysGetTime(){
    timeval time;
    gettimeofday(&time, 0);

    //Handle timeb get time.
    return double(time.tv_sec) + double(time.tv_usec) / 1000000.0;
}

void NVPWindow::sysSleep(double seconds){
    //handle process sleep.
    /*
        Uses Unistd takes ms as argument.
    */
    sleep(seconds);

}

void NVPWindow::sysInit(){
    //NOOP
}

void NVPWindow::sysDeinit(){
    //NOOP
}

static std::string s_path;
std::string NVPWindow::sysExePath(){
    return s_path;
}

void NVPWindow::sysVisibleConsole(){
    //Handle console stdout display.

}

void WINinternal::terminate(){
/*
  Handle Window Termination
*/

}


bool WINinternal::create(const char *title,int width, int height, int inSamples){

    m_dpy = XOpenDisplay(0);
   Atom wmDelete;
  
    static int visual_attribs[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE,8,
        GLX_GREEN_SIZE,8,
        GLX_BLUE_SIZE,8,
        GLX_ALPHA_SIZE,8,
        GLX_DEPTH_SIZE,24,
        GLX_DOUBLEBUFFER, True,
        None
    };

    int glxMajor,glxMinor;

    if(!glXQueryVersion(m_dpy,&glxMajor,&glxMinor) || ((glxMajor == 1) && (glxMinor < 3)) || (glxMajor < 1)){
        printf("Invalid GLX Version.\n");
        return false;
    }

    int fbCount(0);
    GLXFBConfig *fbc = glXChooseFBConfig(m_dpy,DefaultScreen(m_dpy), visual_attribs,&fbCount);
    if(!fbc){
        printf("Could not get FB config.\n");
      return false;
    } 

    int i,bestfbc=0;
    for(i=0;i<fbCount;++i){
        XVisualInfo *vi = glXGetVisualFromFBConfig(m_dpy,fbc[i]);

        if(vi){
                int sampleBuf, samples;
                glXGetFBConfigAttrib(m_dpy,fbc[i],GLX_SAMPLE_BUFFERS, &sampleBuf);
                glXGetFBConfigAttrib(m_dpy,fbc[i],GLX_SAMPLES,&samples);

		if(samples == inSamples){
		  bestfbc = i;
		  break;
		}
         }
        XFree(vi);
    }

    m_glx_fb_config = fbc[bestfbc];

    XFree(fbc);

    XVisualInfo *vi = glXGetVisualFromFBConfig(m_dpy,m_glx_fb_config);

    XSetWindowAttributes swa;
    Colormap cmap;

    swa.colormap = cmap = XCreateColormap(m_dpy,RootWindow(m_dpy,vi->screen),vi->visual,AllocNone);
    swa.background_pixmap = None;
    swa.border_pixel = 0;
    swa.event_mask = 
    ExposureMask | 
    KeyPressMask | 
    KeyReleaseMask | 
    ButtonPressMask | 
    ButtonReleaseMask |
    PointerMotionMask |
    StructureNotifyMask;
    swa.override_redirect = False;

    m_window = XCreateWindow(m_dpy,RootWindow(m_dpy,vi->screen),0,0,width,height,0,vi->depth,InputOutput,vi->visual,
        CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,&swa);
    sleep(1);
    XFree(vi);
    
    XSetStandardProperties(m_dpy,m_window,title,title,None,NULL,0,NULL);
    wmDelete = XInternAtom(m_dpy,"WM_DELETE_WINDOW",True);
    XSetWMProtocols(m_dpy,m_window,&wmDelete,1);
    XMapRaised(m_dpy,m_window);
    XFlush(m_dpy);

    return true;

}


int main(int argc, char **argv)
{
    nvprintSetCallback(sample_print);

    std::string exe = std::string(argv[0]);
    std::replace(exe.begin(),exe.end(),'\\','/');

    size_t last = exe.rfind('/');
    if(last != std::string::npos){
        //s_path = exe.substr(0,last) + std::string("/");


     }
       printf("Sys Exe Path : %s.\n",s_path.c_str());

    sample_main(argc,(const char **)&argv[0]);

    for(int i=0;i<g_windows.size();++i){
            NVPWindow *pWin = g_windows[i];
            if(pWin->m_internal){
                if(pWin->m_internal->m_glx_context != NULL){
                    //Handle GLX cleanup
		  pWin->m_internal->terminate();
                }
                delete pWin->m_internal;
            }

    }

    return 0;
}
