/*-----------------------------------------------------------------------
    Copyright (c) 2013, NVIDIA. All rights reserved.

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

    feedback to tlorach@nvidia.com (Tristan Lorach)
*/ //--------------------------------------------------------------------

#include "main.h"

#include <stdio.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <assert.h>
#include <stdarg.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <io.h>
#include <windows.h>
#include <windowsx.h>
#include <assert.h>
#include "resources.h"

HINSTANCE   g_hInstance = 0;
LPSTR       g_lpCmdLine;
int         g_nCmdShow;

extern "C" {
  _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
};
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

static std::vector<NVPWindow*>  s_windows;
static bool s_glew = false;

static void APIENTRY debugoutput_callback
  (
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const GLvoid* userParam
  )
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
    const char* debSource;
    const char* debType;
    const char* debSev;

    if(source == GL_DEBUG_SOURCE_API_ARB)
      debSource = "OpenGL";
    else if(source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
      debSource = "Windows";
    else if(source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
      debSource = "Shader Compiler";
    else if(source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
      debSource = "Third Party";
    else if(source == GL_DEBUG_SOURCE_APPLICATION_ARB)
      debSource = "Application";
    else if (source == GL_DEBUG_SOURCE_OTHER_ARB)
      debSource = "Other";
    else
      assert(0);

    if(type == GL_DEBUG_TYPE_ERROR)
      debType = "error";
    else if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
      debType = "deprecated behavior";
    else if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
      debType = "undefined behavior";
    else if(type == GL_DEBUG_TYPE_PORTABILITY)
      debType = "portability";
    else if(type == GL_DEBUG_TYPE_PERFORMANCE)
      debType = "performance";
    else if(type == GL_DEBUG_TYPE_OTHER)
      debType = "message";
    else if(type == GL_DEBUG_TYPE_MARKER)
      debType = "marker";
    else if(type == GL_DEBUG_TYPE_PUSH_GROUP)
      debType = "push group";
    else if(type == GL_DEBUG_TYPE_POP_GROUP)
      debType = "pop group";
    else
      assert(0);

    if(severity == GL_DEBUG_SEVERITY_HIGH_ARB)
      debSev = "high";
    else if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
      debSev = "medium";
    else if(severity == GL_DEBUG_SEVERITY_LOW_ARB)
      debSev = "low";
    else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
      debSev = "notification";
    else
      assert(0);

    fprintf(stderr,"%s: %s(%s) %d: %s\n", debSource, debType, debSev, id, message);
  }
}


inline int version(int major, int minor) {
  return major * 100 + minor * 10;
}

void NVPWindow::sysInit()
{
  glfwInit();
}

void NVPWindow::sysDeinit()
{
  for(int i=0; i<s_windows.size(); i++)
  {
    NVPWindow *pWin = s_windows[i];
    pWin->shutdown();
    glfwDestroyWindow((GLFWwindow*)pWin->m_internal);
  }

  glfwTerminate();
}

static std::string s_path;
std::string NVPWindow::sysExePath()
{
  return s_path;
}

double NVPWindow::sysGetTime()
{
    return glfwGetTime();
}

void NVPWindow::sysSleep(double seconds)
{
  // no more available ?
  //glfwSleep(seconds);
}

bool NVPWindow::sysPollEvents(bool loop)
{
  glfwPollEvents();
  do {
    for(int i=0; i<s_windows.size(); i++)
    {
      NVPWindow *pWin = s_windows[i];
      if(glfwWindowShouldClose( (GLFWwindow*)pWin->m_internal))
        return false;
      if(pWin->m_renderCnt > 0 && pWin->isOpen())
      {
        pWin->m_renderCnt--;
        pWin->display();
      }
    }
  } while (loop);

  return true;
}


void NVPWindow::sysWaitEvents()
{
  glfwWaitEvents();
}

NVPWindow::NVPproc NVPWindow::sysGetProcAddress( const char* name )
{
  return glfwGetProcAddress(name);
}

int NVPWindow::sysExtensionSupported( const char* name )
{
  return glfwExtensionSupported(name);
}

//////////////////////////////////////////////////////////////////////////

static void mouse_position_callback(GLFWwindow* win, double x, double y)
{
  NVPWindow* window = (NVPWindow*)glfwGetWindowUserPointer(win);
  
  window->motion(int(x),int(y));

  window->m_curX = int(x);
  window->m_curY = int(y);
}

static void mouse_button_callback(GLFWwindow* win,int Button, int Action, int Mods)
{
  NVPWindow* window = (NVPWindow*)glfwGetWindowUserPointer(win);

  window->mouse((NVPWindow::MouseButton)Button,(NVPWindow::ButtonAction)Action,Mods,window->m_curX,window->m_curY);

  window->m_mods = Mods;
}

static void mouse_wheel_callback(GLFWwindow* win,double x, double y)
{
  NVPWindow* window = (NVPWindow*)glfwGetWindowUserPointer(win);

  int iy = int(y) * 120;  // compatibility with main_win32

  window->mousewheel(iy);

  window->m_wheel = iy;
}

static void key_callback(GLFWwindow* win, int key, int scan, int action, int mods)
{
  NVPWindow* window = (NVPWindow*)glfwGetWindowUserPointer(win);

  window->keyboard((NVPWindow::KeyCode)key,(NVPWindow::ButtonAction)action,mods,window->m_curX, window->m_curY);
}

static void char_callback(GLFWwindow* win, unsigned int key)
{
  NVPWindow* window = (NVPWindow*)glfwGetWindowUserPointer(win);

  window->keyboardchar(key, window->m_mods, window->m_curX, window->m_curY);
}

static void refresh_callback(GLFWwindow* win)
{
  NVPWindow* window = (NVPWindow*)glfwGetWindowUserPointer(win);
  
  window->m_renderCnt++;
}


static void close_callback(GLFWwindow* win)
{
  NVPWindow* window = (NVPWindow*)glfwGetWindowUserPointer(win);
  
  window->postQuit();
}

static void resize_callback(GLFWwindow* win, int width, int height)
{
  NVPWindow* window = (NVPWindow*)glfwGetWindowUserPointer(win);

  if (width == 0 && height == 0)
    return;

  window->reshape(width,height);

  window->m_winSz[0] = width;
  window->m_winSz[1] = height;
}

bool NVPWindow::create(const char* title, const ContextFlags* cflags, int width, int height)
{
  GLFWwindow* win = NULL;
  m_winSz[0] = width;
  m_winSz[1] = height;
  if(version(cflags->major, cflags->minor) >= version(3, 2) && cflags->core)
  {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, cflags->major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, cflags->minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  }

  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, cflags->debug ? GL_TRUE : GL_FALSE);

  win = glfwCreateWindow(width, height, title, NULL, (GLFWwindow*)cflags->share);
  glfwSetWindowUserPointer(win,this);

  glfwSetInputMode(win,GLFW_STICKY_KEYS,GL_TRUE);

  glfwMakeContextCurrent(win);

  GLint ctxMajor = 0;
  GLint ctxMinor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &ctxMajor);
  glGetIntegerv(GL_MINOR_VERSION, &ctxMinor);

  if (version(ctxMajor,ctxMinor) < version(cflags->major,cflags->minor)){
    return false;
  }

  if (!s_glew){
    //glewExperimental = 1;
    glewInit();
    glGetError();
    s_glew = true;
  }

  s_windows.push_back( this );
  m_internal = (NVPWindow::WINhandle)win;
  if(!init()){
    return false;
  }
  //TODO this is where we should show the window

  glfwSetFramebufferSizeCallback(win,resize_callback);
  glfwSetWindowRefreshCallback(win,refresh_callback);
  glfwSetWindowCloseCallback(win,close_callback);

  glfwSetMouseButtonCallback(win, mouse_button_callback);
  glfwSetCursorPosCallback(win, mouse_position_callback);
  glfwSetScrollCallback(win, mouse_wheel_callback);
  glfwSetKeyCallback(win, key_callback);
  glfwSetCharCallback(win, char_callback);

#if !defined (NDEBUG)
  if (GLEW_ARB_debug_output){
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    glDebugMessageCallbackARB(debugoutput_callback, this);
  }
#endif
  
  return true;
}

bool NVPWindow::activate(int width, int height, const char* title, const ContextFlags* cflags, int invisible)
{
  // FIXME invisibile handling!
  return create(title,cflags,width,height);
}

void NVPWindow::deactivate()
{
  // FIXME should remove from g_windows
}

void NVPWindow::setTitle( const char* title )
{
  glfwSetWindowTitle((GLFWwindow*)m_internal,title);
}

void NVPWindow::postQuit()
{
  glfwSetWindowShouldClose((GLFWwindow*)m_internal, GL_TRUE);
}

void NVPWindow::makeContextCurrent()
{
  glfwMakeContextCurrent((GLFWwindow*)m_internal);
}

void NVPWindow::makeContextNonCurrent()
{
  glfwMakeContextCurrent(0);
}

void NVPWindow::swapBuffers()
{
  glfwSwapBuffers((GLFWwindow*)m_internal);
}

void NVPWindow::swapInterval( int interval )
{
  glfwSwapInterval(interval);
}

bool NVPWindow::isOpen()
{
  return glfwGetWindowAttrib((GLFWwindow*)m_internal,GLFW_VISIBLE) && !glfwGetWindowAttrib((GLFWwindow*)m_internal,GLFW_ICONIFIED);
}

#ifdef WIN32
static const WORD MAX_CONSOLE_LINES = 500;
static bool s_isConsole = false;

using namespace std;

void NVPWindow::sysVisibleConsole()
{
  if (s_isConsole) return;

  int hConHandle;
  long lStdHandle;

  CONSOLE_SCREEN_BUFFER_INFO coninfo;

  FILE *fp;

  // allocate a console for this app
  AllocConsole();

  // set the screen buffer to be big enough to let us scroll text
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),
    &coninfo);

  coninfo.dwSize.Y = MAX_CONSOLE_LINES;
  SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),
    coninfo.dwSize);

  // redirect unbuffered STDOUT to the console
  lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
  hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  fp = _fdopen( hConHandle, "w" );

  *stdout = *fp;

  setvbuf( stdout, NULL, _IONBF, 0 );
  // redirect unbuffered STDIN to the console
  lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
  hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  fp = _fdopen( hConHandle, "r" );

  *stdin = *fp;

  setvbuf( stdin, NULL, _IONBF, 0 );
  // redirect unbuffered STDERR to the console
  lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
  hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  fp = _fdopen( hConHandle, "w" );

  *stderr = *fp;

  setvbuf( stderr, NULL, _IONBF, 0 );

  // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog

  // point to console as well

  ios::sync_with_stdio();
}
// http://www.codeguru.com/cpp/w-p/win32/article.php/c1427/A-Simple-Win32-CommandLine-Parser.htm
class CmdLineArgs : public std::vector<char*>
{
public:
  CmdLineArgs ()
  {
    // Save local copy of the command line string, because
    // ParseCmdLine() modifies this string while parsing it.
    PSZ cmdline = GetCommandLineA();
    m_cmdline = new char [strlen (cmdline) + 1];
    if (m_cmdline)
    {
      strcpy (m_cmdline, cmdline);
      ParseCmdLine();
    }
  }
  ~CmdLineArgs()
  {
    delete m_cmdline;
  }
private:
  PSZ m_cmdline; // the command line string
  ////////////////////////////////////////////////////////////////////////////////
  // Parse m_cmdline into individual tokens, which are delimited by spaces. If a
  // token begins with a quote, then that token is terminated by the next quote
  // followed immediately by a space or terminator. This allows tokens to contain
  // spaces.
  // This input string: This "is" a ""test"" "of the parsing" alg"o"rithm.
  // Produces these tokens: This, is, a, "test", of the parsing, alg"o"rithm
  ////////////////////////////////////////////////////////////////////////////////
  void ParseCmdLine ()
  {
    enum { TERM = '\0',
          QUOTE = '\"' };
    bool bInQuotes = false;
    PSZ pargs = m_cmdline;
    while (*pargs)
    {
      while (isspace (*pargs)) // skip leading whitespace
        pargs++;
      bInQuotes = (*pargs == QUOTE); // see if this token is quoted
      if (bInQuotes) // skip leading quote
        pargs++;
      push_back (pargs); // store position of current token
      // Find next token.
      // NOTE: Args are normally terminated by whitespace, unless the
      // arg is quoted. That's why we handle the two cases separately,
      // even though they are very similar.
      if (bInQuotes)
      {
        // find next quote followed by a space or terminator
        while (*pargs &&
          !(*pargs == QUOTE && (isspace (pargs[1]) || pargs[1] == TERM)))
          pargs++;
        if (*pargs)
        {
          *pargs = TERM; // terminate token
          if (pargs[1]) // if quoted token not followed by a terminator
            pargs += 2; // advance to next token
        }
      }
      else
      {
        // skip to next non-whitespace character
        while (*pargs && !isspace (*pargs))
          pargs++;
        if (*pargs && isspace (*pargs)) // end of token
        {
          *pargs = TERM; // terminate token
          pargs++; // advance to next token or terminator
        }
      }
    } // while (*pargs)
  } // ParseCmdLine()
}; // class CmdLineArgs
#endif //WIN32

#ifdef WIN32
//------------------------------------------------------------------------------
int WINAPI WinMain(    HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     lpCmdLine,
                    int       nCmdShow )
{
    g_hInstance = hInstance;
    g_lpCmdLine = lpCmdLine;
    g_nCmdShow = nCmdShow;

    CmdLineArgs args;

    std::string exe = args[0];
    std::replace(exe.begin(),exe.end(),'\\','/');

    size_t last = exe.rfind('/');
    if (last != std::string::npos){
      s_path = exe.substr(0,last) + std::string("/");
    }



    //initNSight();
#ifdef MEMORY_LEAKS_CHECK
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); 
    _CrtSetReportMode ( _CRT_ERROR, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_WNDW);
#endif
    NVPWindow::sysInit();
    //
    // relay the "main" to the sample
    // the sample will create the window(s)
    //
    sample_main((int)args.size(), (const char**)&args[0]);

#else //WIN32

void NVPWindow::sysVisibleConsole() {}
int main(int argc, const char **argv)
{
    NVPWindow::sysInit();
    sample_main(argc, argv);
#endif //WIN32
    //
    // Terminate
    //
    NVPWindow::sysDeinit();
#ifdef WIN32
    UnregisterClass( "MY_WINDOWS_CLASS", hInstance );
#endif
#ifdef MEMORY_LEAKS_CHECK
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}

#ifdef WIN32

int main(int argc, char **argv)
{
  HINSTANCE hinstance = GetModuleHandle(NULL);
  s_isConsole = true;

  WinMain(hinstance, NULL, NULL, 1);
  return 0;
}

#endif

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
    char *prefix = "";
    switch(level)
    {
    case LOGLEVEL_WARNING:
        prefix = "LOG *WARNING* >> ";
        break;
    case LOGLEVEL_ERROR:
        prefix = "LOG **ERROR** >> ";
        break;
    case LOGLEVEL_OK:
        prefix = "LOG !OK! >> ";
        break;
    case LOGLEVEL_INFO:
        prefix = "LOG Message >> ";
        break;
    default:
        break;
    }
#ifdef WIN32
    OutputDebugStringA(prefix);
    OutputDebugStringA(fmt2);
#ifdef _DEBUG
    if(bLogReady == false)
    {
        fd = fopen("Log.txt", "w");
        bLogReady = true;
    }
    if(fd)
    {
        fprintf(fd, prefix);
        fprintf(fd, fmt2);
    }
#endif
#endif
    sample_print(level, fmt2);
    ::printf(prefix);
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
