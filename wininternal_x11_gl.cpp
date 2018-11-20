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

#include<GL/glew.h>

/*
  GL Includes
*/
#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glxext.h>
#include<GL/glu.h>

/*
  XLib Includes
 */

#include<X11/Xlib.h>
#include<X11/Xatom.h>
#include<X11/keysym.h>
#include<X11/extensions/xf86vmode.h>

/*
  Request delcaration of wininternal
 */
#define DECL_WININTERNAL
#include"main.h"

/*
  Misc Includes for framework functionality
 */
#include"nv_helpers/misc.hpp"

/*
  System includes
 */
#include<stdio.h>
#include<fcntl.h>
#include<stdio.h>
#include<iostream>
#include<fstream>
#include<algorithm>
#include<string>



extern Display *g_dpy;

/*
  Global Window List
 */
extern std::vector<NVPWindow*> g_windows;

typedef GLXContext(*glXCreateContextAttribsARBProc)(Display*,GLXFBConfig,GLXContext,Bool, const int*);
 

struct WINinternalGL:WINinternal{

  //  GLXContext m_hRC;
  //  Window m_window;
  //  GLXFBConfig m_glx_fb_config;
  WINinternalGL(NVPWindow *win):
    WINinternal(win)
    {
  }


  virtual bool initBase(const NVPWindow::ContextFlags *cflags, NVPWindow *sourceWindow);
  virtual int sysExtensionSupported(const char *name);
  virtual void swapInternal(int i){ /*glXSwapIntervalEXT(m_dpy,m_window,i);*/ }
  virtual void swapBuffers() {glXSwapBuffers(m_dpy, m_window); }
  virtual void *getHGLRC() { return m_glx_context; }
  virtual NVPWindow::NVPproc sysGetProcAddress(const char *name){
    return (NVPWindow::NVPproc) glXGetProcAddress((const GLubyte *)name);
  }
  
  virtual void terminate();

  virtual void makeContextCurrent(){ glXMakeCurrent(m_dpy,m_window,m_glx_context); }

  virtual void makeContextNonCurrent() { glXMakeCurrent(m_dpy, m_window, 0); }

  virtual void screenshot(const char *filename, int x, int y, int width, int height, unsigned char *data);

  static WINinternal *alloc(NVPWindow *win);

};


/*******************************************************
Temporary Context Error Handler
Records errors during context creation
*******************************************************/

static bool ctxErrorOccurred(false);
static int ctxErrorHandler(Display *dpy, XErrorEvent *evt){
  ctxErrorOccurred = true;
  return 0;

}



/*******************************************************
Create this specific WINinternal
*******************************************************/
WINinternal *newWINinternalGL(NVPWindow *win){
  return new WINinternalGL(win);
}

/******************************************************
OGL Callback
******************************************************/

#ifdef _DEBUG

static void APIENTRY myOpenGLCallback(
				      GLenum source,
				      GLenum type,
				      GLuint id,
				      GLenum severity,
				      GLsizei lenght,
				      const GLchar *message,
				      const GLvoid *userParam){


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
//------------------------------------------------------------------------------



bool WINinternalGL::initBase(const NVPWindow::ContextFlags* cflags, NVPWindow *sourceWindow){

  printf("Initialising WINinternalGL for X11.\n");

  /*
    Take any settings
   */
  NVPWindow::ContextFlags settings;
  if(cflags){
    settings = *cflags;
  }

  const char *glxExts = glXQueryExtensionsString(m_dpy, DefaultScreen(m_dpy));
  
  printf("GLX Extensions : %s.\n",glxExts);

  glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

  ctxErrorOccurred = false;
  int (*oldHandler)(Display *,XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

  settings.core = 1;

  int contextAttribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, settings.major,
    GLX_CONTEXT_MINOR_VERSION_ARB, settings.minor,
    GLX_CONTEXT_PROFILE_MASK_ARB,settings.core?GLX_CONTEXT_CORE_PROFILE_BIT_ARB:GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,0
};

  printf("Creating %s context of version %i.%i.\n",(settings.core?"CORE":"COMPAT"),settings.major, settings.minor);
  
  printf("FB Config : %i.\n",m_glx_fb_config);

  m_glx_context = glXCreateContextAttribsARB(m_dpy,m_glx_fb_config,0,True,contextAttribs);

  printf("GLX Context : %i Created.\n");

  XSetErrorHandler(oldHandler);

  if(!glXMakeCurrent(m_dpy,m_window,m_glx_context)){
    perror("Error making glx context current.\n");
  }else{
    printf("Context Made Current.\n");
  }

  glewExperimental = true;

  GLenum glewErr = glewInit();

  if(GLEW_OK != glewErr){
    perror("Error initializing extensions.\n");
    printf("GLEW Error : %i\n", glewErr);
  }
  
  return true;
}



// from GLFW 3.0
static int stringInExtensionString(const char* string, const char* exts)
{
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

int WINinternalGL::sysExtensionSupported( const char* name )
{
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
  WINinternalGL* wininternal = static_cast<WINinternalGL*>(g_windows[0]->m_internal);

  /*  if (WGLEW_ARB_extensions_string){
    exts = wglGetExtensionsStringARB(wininternal->m_hDC);
  }
  if (!exts && WGLEW_EXT_extensions_string){
    exts = wglGetExtensionsStringEXT();
  }
  if (!exts) {
    return FALSE;
    }*/
  
  return 0;//stringInExtensionString(name,exts);
}

//------------------------------------------------------------------------------
// screen shot
//------------------------------------------------------------------------------
void WINinternalGL::screenshot(const char* filename, int x, int y, int width, int height, unsigned char* data)
{
    glFinish();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glReadPixels(x,y,width,height, GL_BGRA, GL_UNSIGNED_BYTE, data);

    if(filename)
        nv_helpers::saveBMP(filename, width, height, data);
}


void WINinternalGL::terminate(){


}

