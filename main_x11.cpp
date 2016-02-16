/*-----------------------------------------------------------------------
    Copyright (c) 2013 - 2016, NVIDIA. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of its contributors may be used to endorse 
       or promote products derived from this software without specific
       prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    feedback to chebert@nvidia.com (Chris Hebert)
*/ //--------------------------------------------------------------------

#define GLX_GLXEXT_PROTOTYPES

#include<GL/glew.h>

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

 
static bool ctxErrorOccurred;
static int ctxErrorHandler(Display *dpy, XErrorEvent *evt){
    ctxErrorOccurred = true;
    return 0;
}

//------------------------------------------------------------------------------
// Debug Callback
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
#ifdef _DEBUG
static void APIENTRY myOpenGLCallback(  GLenum source,
                        GLenum type,
                        GLuint id,
                        GLenum severity,
                        GLsizei length,
                        const GLchar* message,
                        const GLvoid* userParam)
{

  NVPWindow* window = (NVPWindow*)userParam;

  GLenum filter = window->m_debugFilter;
  GLenum severitycmp = severity;
  // minor fixup for filtering so notification becomes lowest priority
  if (GL_DEBUG_SEVERITY_NOTIFICATION == filter){
    filter = GL_DEBUG_SEVERITY_LOW_ARB+1;
  }
  if (GL_DEBUG_SEVERITY_NOTIFICATION == severitycmp){
    severitycmp = GL_DEBUG_SEVERITY_LOW_ARB+1;
  }

  if (!filter|| severitycmp <= filter )
  {
  
    //static std::map<GLuint, bool> ignoreMap;
    //if(ignoreMap[id] == true)
    //    return;
    char *strSource = "0";
    char *strType = strSource;
    switch(source)
    {
    case GL_DEBUG_SOURCE_API_ARB:
        strSource = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
        strSource = "WINDOWS";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
        strSource = "SHADER COMP.";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
        strSource = "3RD PARTY";
        break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB:
        strSource = "APP";
        break;
    case GL_DEBUG_SOURCE_OTHER_ARB:
        strSource = "OTHER";
        break;
    }
    switch(type)
    {
    case GL_DEBUG_TYPE_ERROR_ARB:
        strType = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        strType = "Deprecated";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        strType = "Undefined";
        break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
        strType = "Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
        strType = "Performance";
        break;
    case GL_DEBUG_TYPE_OTHER_ARB:
        strType = "Other";
        break;
    }
    switch(severity)
    {
    case GL_DEBUG_SEVERITY_HIGH_ARB:
        LOGE("ARB_debug : %s High - %s - %s : %s\n", window->m_debugTitle.c_str(), strSource, strType, message);
        break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB:
        LOGW("ARB_debug : %s Medium - %s - %s : %s\n", window->m_debugTitle.c_str(), strSource, strType, message);
        break;
    case GL_DEBUG_SEVERITY_LOW_ARB:
        LOGI("ARB_debug : %s Low - %s - %s : %s\n", window->m_debugTitle.c_str(), strSource, strType, message);
        break;
    default:
        //LOGI("ARB_debug : comment - %s - %s : %s\n", strSource, strType, message);
        break;
    }
  }
}

//------------------------------------------------------------------------------
void checkGL( char* msg )
{
    GLenum errCode;
    //const GLubyte* errString;
    errCode = glGetError();
    if (errCode != GL_NO_ERROR) {
        //printf ( "%s, ERROR: %s\n", msg, gluErrorString(errCode) );
        LOGE("%s, ERROR: 0x%x\n", msg, errCode );
    }
}
#endif

struct WINinternal{
    NVPWindow *m_win;
    int m_screen;
    GLXContext m_glx_context;
    GLXFBConfig m_glx_fb_config;
    Display *m_dpy;
    Window m_window;
    XVisualInfo *m_visual;
    XF86VidModeModeInfo m_mode;
    XSetWindowAttributes winAttributes;
    bool m_visible;

    WINinternal(NVPWindow *win):
    m_win(win),
    m_screen(0),
    m_glx_context(0),
    m_glx_fb_config(0),
    m_dpy(0),
    m_window(0),
    m_visual(0),
    m_visible(true)
{}

bool create(const char *title, int width, int height);
bool initBase(const NVPWindow::ContextFlags *cflags, NVPWindow *sourceWindow);

};

bool WINinternal::initBase(const NVPWindow::ContextFlags *cflags, NVPWindow *sourceWindow){

    NVPWindow::ContextFlags settings;
    if(cflags){
        settings = *cflags;
    }

    const char *glxExts = glXQueryExtensionsString(m_dpy,DefaultScreen(m_dpy));

    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB  = (glXCreateContextAttribsARBProc) glXGetProcAddressARB((const GLubyte *) "glXCreateContextAttribsARB");

 
    ctxErrorOccurred = false;
    int (*oldHandler)(Display *, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

    int contextattribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, settings.major,
        GLX_CONTEXT_MINOR_VERSION_ARB, settings.minor,
        GLX_CONTEXT_PROFILE_MASK_ARB, settings.core?GLX_CONTEXT_CORE_PROFILE_BIT_ARB:GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
        0
    }; 

   
    m_glx_context = glXCreateContextAttribsARB(m_dpy,m_glx_fb_config,0,True,contextattribs);


    XSetErrorHandler(oldHandler);



    if(!glXMakeCurrent(m_dpy,m_window,m_glx_context)){
        printf("Error making glx context current.\n");
    }

   GLenum glewErr = glewInit();

   if(GLEW_OK != glewErr){
    printf("Error initialising glew: %s.\n",glewGetErrorString(glewErr));
   }


   return true;
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

bool NVPWindow::create(const char *title, const ContextFlags *cflags, int width, int height){

    m_winSz[0] = width;
    m_winSz[1] = height;

    m_internal = new WINinternal(this);

    m_debugTitle = title ? title:"Sample";

    if(m_internal->create(m_debugTitle.c_str(),width,height)){

        g_windows.push_back(this);

        /*
            Update and draw.
        */

        if(m_internal->initBase(cflags,this)){
            if(init()){
                //show window
                return true;
            }else{
                printf("Init Failed.\n");
            }
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

}

void NVPWindow::maximize(){
    //handle maximize
}

void NVPWindow::restore(){
    //handle restore
}

void NVPWindow::minimize(){
    //handle minimize
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
    glXSwapIntervalEXT(m_internal->m_dpy,m_internal->m_window,i);
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

static int stringInExtensionString(const char *string, const char *exts){
const GLubyte* extensions = (const GLubyte*) exts;
  const GLubyte* start;
  GLubyte* where;
  GLubyte* terminator;

  // It takes a bit of care to be fool-proof about parsing the
  // OpenGL extensions string. Don't be fooled by sub-strings,
  // etc.
  start = extensions;
  for (;;)
  {
    where = (GLubyte*) strstr((const char*) start, string);
    if (!where)
      return GL_FALSE;

    terminator = where + strlen(string);
    if (where == start || *(where - 1) == ' ')
    {
      if (*terminator == ' ' || *terminator == '\0')
        break;
    }

    start = terminator;
  }

  return GL_TRUE;

}

int NVPWindow::sysExtensionSupported(const char *name){

     // we are not using the glew query, as glew will only report
  // those extension it knows about, not what the actual driver may support

  int i;
  GLint count;

  // Check if extension is in the modern OpenGL extensions string list
  // This should be safe to use since GL 3.0 is around for a long time :)

  glGetIntegerv(GL_NUM_EXTENSIONS, &count);

  for (i = 0;  i < count;  i++)
  {
    const char* en = (const char*) glGetStringi(GL_EXTENSIONS, i);
    if (!en)
    {
      return GL_FALSE;
    }

    if (strcmp(en, name) == 0)
      return GL_TRUE;
  }

  // Check platform specifc gets

  const char* exts = NULL;
  NVPWindow* win = g_windows[0];

 
  if (!exts) {
    return 0;
  }
  
  return stringInExtensionString(name,exts);

}

NVPWindow::NVPproc NVPWindow::sysGetProcAddress(const char *name){
    return (NVPWindow::NVPproc)glXGetProcAddress((const GLubyte *)name);
}

void NVPWindow::sysWaitEvents(){
    //Handle wait for XEvents.

}

static int getMilliCount(){
    timeb tb;
    ftime(&tb);

    int nCount = tb.millitm + (tb.time & 0xffffff) * 1000;
    return nCount;

}

static double s_frequency;
double NVPWindow::sysGetTime(){

    //Handle timeb get time.
    return 1.0/(double)getMilliCount();
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



bool WINinternal::create(const char *title,int width, int height){

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
        GLX_STENCIL_SIZE,8,
        GLX_SAMPLE_BUFFERS,1,
        GLX_SAMPLES,8,
        GLX_DOUBLEBUFFER, True,
        None
    };

    int glxMajor,glxMinor;

    if(!glXQueryVersion(m_dpy,&glxMajor,&glxMinor) || ((glxMajor == 1) && (glxMinor < 3)) || (glxMajor < 1)){
        printf("Invalid GLX Version.\n");
        return false;
    }

    int fbCount;

    GLXFBConfig *fbc = glXChooseFBConfig(m_dpy,DefaultScreen(m_dpy), visual_attribs,&fbCount);
    if(!fbc){
        printf("Could not get FB config.\n");
      return false;
    } 

    int bestfbc = -1,worstfbc = -1, best_num_sample = -1, worst_num_sample = 999;

    int i;

    for(i=0;i<fbCount;++i){
        XVisualInfo *vi = glXGetVisualFromFBConfig(m_dpy,fbc[i]);

        if(vi){
                int sampleBuf, samples;
                glXGetFBConfigAttrib(m_dpy,fbc[i],GLX_SAMPLE_BUFFERS, &sampleBuf);
                glXGetFBConfigAttrib(m_dpy,fbc[i],GLX_SAMPLES,&samples);

                printf("GLX Config Sample Buffers : %d.\n",sampleBuf);
                printf("GLX Config Samples : %d.\n",samples);

                if(bestfbc < 0 || sampleBuf && samples > best_num_sample) bestfbc = i,best_num_sample = samples;
                if(worstfbc < 0 || !sampleBuf || samples < worst_num_sample) worstfbc = i, worst_num_sample = samples;
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


    printf("Creating Window.\n");

    printf("Width : %d Height : %d.\n",width,height);

    m_window = XCreateWindow(m_dpy,RootWindow(m_dpy,vi->screen),0,0,width,height,0,vi->depth,InputOutput,vi->visual,
        CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,&swa);
    sleep(1);

    printf("Window : %d.\n",m_window);
    XFree(vi);
    
    XSetStandardProperties(m_dpy,m_window,title,title,None,NULL,0,NULL);
    wmDelete = XInternAtom(m_dpy,"WM_DELETE_WINDOW",True);
    XSetWMProtocols(m_dpy,m_window,&wmDelete,1);
    XMapRaised(m_dpy,m_window);
    XFlush(m_dpy);


    printf("Window Created.\n");
    return true;

}


int main(int argc, char **argv){


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

                }
                delete pWin->m_internal;
            }

    }

    return 0;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
static size_t fmt2_sz    = 0;
static char *fmt2 = NULL;
static FILE *fd = NULL;
static bool bLogReady = false;
static bool bPrintLogging = true;
static int  printLevel = -1; // <0 mean no level prefix
void nvprintSetLevel(int l)
{
    printLevel = l;
}
int nvprintGetLevel()
{
    return printLevel;
}
void nvprintSetLogging(bool b)
{
    bPrintLogging = b;
}
void nvprintf2(va_list &vlist, const char * fmt, int level)
{
    if(bPrintLogging == false)
        return;
    if(fmt2_sz == 0) {
        fmt2_sz = 1024;
        fmt2 = (char*)malloc(fmt2_sz);
    }
    while((vsnprintf(fmt2, fmt2_sz, fmt, vlist)) < 0) // means there wasn't anough room
    {
        fmt2_sz *= 2;
        if(fmt2) free(fmt2);
        fmt2 = (char*)malloc(fmt2_sz);
    }
    //char *prefix = "";
    //switch(level)
    //{
    //case LOGLEVEL_WARNING:
    //    prefix = "LOG *WARNING* >> ";
    //    break;
    //case LOGLEVEL_ERROR:
    //    prefix = "LOG **ERROR** >> ";
    //    break;
    //case LOGLEVEL_OK:
    //    prefix = "LOG !OK! >> ";
    //    break;
    //case LOGLEVEL_INFO:
    //default:
    //    break;
    //}
#ifdef WIN32
    //OutputDebugStringA(prefix);
    OutputDebugStringA(fmt2);
#ifdef _DEBUG
    if(bLogReady == false)
    {
        fd = fopen("Log.txt", "w");
        bLogReady = true;
    }
    if(fd)
    {
        //fprintf(fd, prefix);
        fprintf(fd, fmt2);
    }
#endif
#endif
    sample_print(level, fmt2);
    //::printf(prefix);
    ::printf(fmt2);
}
void nvprintf(const char * fmt, ...)
{
//    int r = 0;
    va_list  vlist;
    va_start(vlist, fmt);
    nvprintf2(vlist, fmt, printLevel);
}
void nvprintfLevel(int level, const char * fmt, ...)
{
    va_list  vlist;
    va_start(vlist, fmt);
    nvprintf2(vlist, fmt, level);
}
//------------------------------------------------------------------------------
