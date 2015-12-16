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
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/glew.h>

#ifdef USEOPENGLTEXT
#	define NV_REPORT_COMPILE_ERRORS
#   include "OpenGLText.h"
#   include "tracedisplay.h"
#endif
#include "nv_helpers/TimeSampler.h"
#include "nv_helpers/InertiaCamera.h"

#include <map>
using std::map;

#ifndef WINDOWINERTIACAMERA_EXTERN
#ifdef USEOPENGLTEXT
#include "arial_10.h"
#include "arial_10_bitmap.h"
#include "baub_16.h"
#include "baub_16_bitmap.h"
#endif
#endif

#define KEYTAU 0.10f
//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------
#ifndef WIN32
struct POINT {
  int x;
  int y;
};
#endif
#ifdef WINDOWINERTIACAMERA_EXTERN
extern std::map<char, bool*>    g_toggleMap;
#else
std::map<char, bool*>    g_toggleMap;
#endif
inline void addToggleKey(char c, bool* target, const char* desc)
{
	LOGI(desc);
	g_toggleMap[c] = target;
}
//-----------------------------------------------------------------------------
// Derive the Window for this sample
//-----------------------------------------------------------------------------
class WindowInertiaCamera: public NVPWindow
{
public:
    WindowInertiaCamera(const vec3f eye=vec3f(0.0f,1.0f,-3.0f), const vec3f focus=vec3f(0,0,0), const vec3f object=vec3f(0,0,0), float fov_=50.0, float near_=0.01f, float far_=10.0) :
      m_camera(eye, focus, object)
	{
		m_bCameraMode = true;
		m_bContinue = true;
		m_moveStep = 0.2f;
		m_ptLastMousePosit.x = m_ptLastMousePosit.y  = 0;
		m_ptCurrentMousePosit.x = m_ptCurrentMousePosit.y = 0;
		m_ptOriginalMousePosit.x = m_ptOriginalMousePosit.y = 0;
		m_bMousing = false;
		m_bRMousing = false;
		m_bMMousing = false;
		m_bNewTiming = false;
		m_bAdjustTimeScale = true;
#ifdef USEOPENGLTEXT
		m_textColor = 0xE0E0FFA0;
#endif
        m_fov = fov_;
        m_near = near_;
        m_far = far_;
	}

	#ifdef USEOPENGLTEXT
	//---------------------- TEXT TEST ---------------------------------------------
	OpenGLText			m_oglText;
	OpenGLText			m_oglTextBig;
	unsigned int		m_textColor;
	//---------------------- TRACE TEST --------------------------------------------
	CTrace<float>       m_trace;
	COGLTraceDisplay	m_traceDisp;
	#endif

	bool			m_bCameraMode;
    bool			m_bContinue;
    float			m_moveStep;
    POINT			m_ptLastMousePosit;
    POINT			m_ptCurrentMousePosit;
    POINT			m_ptOriginalMousePosit;
    bool			m_bMousing;
    bool			m_bRMousing;
    bool			m_bMMousing;
    bool			m_bNewTiming;
	bool			m_bAdjustTimeScale;

    TimeSampler		m_realtime;
    InertiaCamera	m_camera;
    mat4f			m_projection;
    float           m_fov, m_near, m_far;
public:    
    inline mat4f&   projMat() { return m_projection; }
    inline mat4f&   viewMat() { return m_camera.m4_view; }
    inline bool&    nonStopRendering() { return m_realtime.bNonStopRendering; }

    virtual bool init();
    virtual void shutdown();
    virtual void reshape(int w, int h);
    virtual void motion(int x, int y);
    virtual void mousewheel(int delta);
    virtual void mouse(NVPWindow::MouseButton button, ButtonAction action, int mods, int x, int y);
    virtual void keyboard(WindowInertiaCamera::KeyCode key, ButtonAction action, int mods, int x, int y);
    virtual void keyboardchar(unsigned char key, int mods, int x, int y);
    virtual void idle();
    virtual void display();
    virtual void displayHUD();      // use this one for normal default HUD
    virtual void beginDisplayHUD(); // or use begin/end to insert text in-between
    virtual void endDisplayHUD();
    const char* getHelpText(int *lines=NULL) {
        if(lines) *lines = 7;
        return 
        "Left mouse button: rotate arount target\n"
        "Right mouse button: translate target forward backward (+ Y axis rotate)\n"
        "Middle mouse button: Pan target along view plane\n"
        "Mouse wheel or PgUp/PgDn: zoom in/out\n"
        "Arrow keys: rotate arount target\n"
        "Ctrl+Arrow keys: Pan target\n"
        "Ctrl+PgUp/PgDn: translate target forward/backward\n"
        ;
    }
};
#ifndef WINDOWINERTIACAMERA_EXTERN

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool WindowInertiaCamera::init()
{
    assert(glewInit() == GLEW_OK);
    m_realtime.bNonStopRendering = true;
#ifdef USEOPENGLTEXT
    //
    // Text initialization
    //
    m_oglText.init(arial_10::image, (OpenGLText::FileHeader*)&arial_10::font, m_winSz[0], m_winSz[1]);
    m_oglTextBig.init(baub_16::image, (OpenGLText::FileHeader*)&baub_16::font, m_winSz[0], m_winSz[1]);
    //
    // Graphs initialization
    //
    COGLTraceDisplay::Init(m_winSz[0], m_winSz[1]);
    m_traceDisp.Position((float)(m_winSz[0]-100-10),(float)(m_winSz[1]-100-10), 100.0f, 100);
    m_traceDisp.Clear();
    m_trace.init((int)50, "time");
    for(int i=0; i<50; i++)
    {
        m_trace.insert(0.0f);
    }
    m_traceDisp.Insert(&m_trace, 0);
    m_traceDisp.setScaleBias(1.0f, 0.0f);
    m_traceDisp.setFonts(&m_oglTextBig, &m_oglText);
    m_traceDisp.setTitle("timing");
    m_traceDisp.setValueString("%.2fms");
    m_traceDisp.BackgroundColor(0.0f, 0.2f, 0.0f, 0.7f);
    m_traceDisp.DrawBack(false);
    m_traceDisp.DrawText(true);
    m_traceDisp.SetTextDrawColumn(true);
    m_traceDisp.SetDrawDoubleColumn(false);
    m_traceDisp.setStaticLabel(true);
    m_traceDisp.setFillGraph(true);
    m_traceDisp.setLineThickness(2.0);
    m_traceDisp.setSmoothLine(1);
    m_traceDisp.setNameColorIdx(0);
    m_traceDisp.setValueColorIdx(-1);
#endif

    const int w = m_winSz[0];
    const int h = m_winSz[1];
    glViewport(0, 0, w, h);
    float r = (float)w / (float)h;
    m_projection = perspective(m_fov, r, m_near, m_far);

    return true;
}

void WindowInertiaCamera::shutdown()
{
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
#define CAMERATAU 0.03f
void WindowInertiaCamera::motion(int x, int y)
{
    m_ptCurrentMousePosit.x = x;
    m_ptCurrentMousePosit.y = y;
    //---------------------------- LEFT
    if(m_bMousing)
    {
        float hval = 2.0f*(float)(m_ptCurrentMousePosit.x - m_ptLastMousePosit.x)/(float)getWidth();
        float vval = 2.0f*(float)(m_ptCurrentMousePosit.y - m_ptLastMousePosit.y)/(float)getHeight();
        m_camera.tau = CAMERATAU;
        m_camera.rotateH(hval);
        m_camera.rotateV(vval);
        postRedisplay();
    }
    //---------------------------- MIDDLE
    if( m_bMMousing )
    {
        float hval = 2.0f*(float)(m_ptCurrentMousePosit.x - m_ptLastMousePosit.x)/(float)getWidth();
        float vval = 2.0f*(float)(m_ptCurrentMousePosit.y - m_ptLastMousePosit.y)/(float)getHeight();
        m_camera.tau = CAMERATAU;
        m_camera.rotateH(hval, true);
        m_camera.rotateV(vval, true);
        postRedisplay();
    }
    //---------------------------- RIGHT
    if( m_bRMousing )
    {
        float hval = 2.0f*(float)(m_ptCurrentMousePosit.x - m_ptLastMousePosit.x)/(float)getWidth();
        float vval = -2.0f*(float)(m_ptCurrentMousePosit.y - m_ptLastMousePosit.y)/(float)getHeight();
        m_camera.tau = CAMERATAU;
        m_camera.rotateH(hval, !!(getMods()&KMOD_CONTROL));
        m_camera.move(vval, !!(getMods()&KMOD_CONTROL));
        postRedisplay();
    }
    
    m_ptLastMousePosit.x = m_ptCurrentMousePosit.x;
    m_ptLastMousePosit.y = m_ptCurrentMousePosit.y;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void WindowInertiaCamera::mousewheel(int delta)
{
    m_camera.tau = KEYTAU;
    m_camera.move(delta > 0 ? m_moveStep: -m_moveStep, !!(getMods()&KMOD_CONTROL));
    postRedisplay();
}
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void WindowInertiaCamera::mouse(NVPWindow::MouseButton button, NVPWindow::ButtonAction state, int mods, int x, int y)
{
    switch(button)
    {
    case NVPWindow::MOUSE_BUTTON_LEFT:
        if(state == NVPWindow::BUTTON_PRESS)
        {
            // TODO: equivalent of glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED/NORMAL);
            m_bMousing = true;
            postRedisplay();
            if(getMods()&KMOD_CONTROL)
            {
            }
            else if(getMods()&KMOD_SHIFT)
            {
            }
        }
        else {
            m_bMousing = false;
            postRedisplay();
        }
        break;
    case NVPWindow::MOUSE_BUTTON_RIGHT:
        if(state == NVPWindow::BUTTON_PRESS)
        {
            m_ptLastMousePosit.x = m_ptCurrentMousePosit.x = x;
            m_ptLastMousePosit.y = m_ptCurrentMousePosit.y = y;
            m_bRMousing = true;
            postRedisplay();
            if(getMods()&KMOD_CONTROL)
            {
            }
        }
        else {
            m_bRMousing = false;
            postRedisplay();
        }
        break;
    case NVPWindow::MOUSE_BUTTON_MIDDLE:
        if(state == NVPWindow::BUTTON_PRESS)
        {
            m_ptLastMousePosit.x = m_ptCurrentMousePosit.x = x;
            m_ptLastMousePosit.y = m_ptCurrentMousePosit.y = y;
            m_bMMousing = true;
            postRedisplay();
        }
        else {
            m_bMMousing = false;
            postRedisplay();
        }
        break;
    }
}
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void WindowInertiaCamera::keyboard(NVPWindow::KeyCode key, NVPWindow::ButtonAction action, int mods, int x, int y)
{
    if(action == NVPWindow::BUTTON_RELEASE)
        return;
    switch(key)
    {
    case NVPWindow::KEY_F1:
        break;
    case NVPWindow::KEY_F2:
        break;
    case NVPWindow::KEY_F3:
    case NVPWindow::KEY_F4:
    case NVPWindow::KEY_F5:
    case NVPWindow::KEY_F6:
    case NVPWindow::KEY_F7:
    case NVPWindow::KEY_F8:
    case NVPWindow::KEY_F9:
    case NVPWindow::KEY_F10:
    case NVPWindow::KEY_F11:
        break;
    case NVPWindow::KEY_F12:
        break;
    case NVPWindow::KEY_LEFT:
        m_camera.tau = KEYTAU;
        m_camera.rotateH(m_moveStep, !!(getMods()&KMOD_CONTROL));
        break;
    case NVPWindow::KEY_UP:
        m_camera.tau = KEYTAU;
        m_camera.rotateV(m_moveStep, !!(getMods()&KMOD_CONTROL));
        break;
    case NVPWindow::KEY_RIGHT:
        m_camera.tau = KEYTAU;
        m_camera.rotateH(-m_moveStep, !!(getMods()&KMOD_CONTROL));
        break;
    case NVPWindow::KEY_DOWN:
        m_camera.tau = KEYTAU;
        m_camera.rotateV(-m_moveStep, !!(getMods()&KMOD_CONTROL));
        break;
    case NVPWindow::KEY_PAGE_UP:
        m_camera.tau = KEYTAU;
        m_camera.move(m_moveStep, !!(getMods()&KMOD_CONTROL));
        break;
    case NVPWindow::KEY_PAGE_DOWN:
        m_camera.tau = KEYTAU;
        m_camera.move(-m_moveStep, !!(getMods()&KMOD_CONTROL));
        break;
    case NVPWindow::KEY_ESCAPE:
        postQuit();
        break;
    }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void WindowInertiaCamera::keyboardchar( unsigned char key, int mods, int x, int y )
{
    // check registered toggles
    std::map<char, bool*>::iterator it = g_toggleMap.find(key);
    if(it != g_toggleMap.end())
    {
        *it->second = *it->second ? false : true;
    }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void WindowInertiaCamera::idle()
{
    //
    // if requested: trigger again the next frame for rendering
    //
    if (m_bContinue || m_realtime.bNonStopRendering)
        postRedisplay();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void WindowInertiaCamera::display()
{
    //
    // Camera motion
    //
    m_bContinue = m_camera.update((float)m_realtime.getTiming());
    //
    // time sampling
    //
    if(m_realtime.update(m_bContinue))
    {
#ifdef USEOPENGLTEXT
		float ms = 1000.0f * (float)m_realtime.getTiming();
		if(m_bAdjustTimeScale) {
			m_traceDisp.setScaleBias(10.0f*(1+1*(int)(ms/10.0f)), 0.0f);
			m_bAdjustTimeScale = false;
		}
		m_trace.insert(ms);
#endif
    }
}

//------------------------------------------------------------------------------
// when the sample wants to add some text
//------------------------------------------------------------------------------
void WindowInertiaCamera::beginDisplayHUD()
{
#ifdef USEOPENGLTEXT
    OpenGLText::BackupStates();
    //
    // Graph need texts
    //
    m_oglText.beginString();
    m_oglTextBig.beginString();
#endif
}
void WindowInertiaCamera::endDisplayHUD()
{
#ifdef USEOPENGLTEXT
    //
    // Start batching graphs
    //
    COGLTraceDisplay::Begin();
    m_traceDisp.Display(CTraceDisplay::LINE_STREAM);
    //
    // Kick-off rendering of graphs
    //
    COGLTraceDisplay::End();
    //
    // Additional text
    //
    char fpsStr[10];
    snprintf(fpsStr, 9, "%d FPS", m_realtime.getFPS());
    m_oglTextBig.drawString(m_winSz[0]-80, m_winSz[1]-50-30, fpsStr, 1, m_textColor);
    //
    // Kick-off rendering of text
    //
    m_oglText.endString();
    m_oglTextBig.endString();
    OpenGLText::RestoreStates();
#endif
}
//------------------------------------------------------------------------------
// when only default HUD needed
//------------------------------------------------------------------------------
void WindowInertiaCamera::displayHUD()
{
#ifdef USEOPENGLTEXT
    beginDisplayHUD();
    endDisplayHUD();
#endif
}
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void WindowInertiaCamera::reshape(int w, int h)
{
    NVPWindow::reshape(w,h);
    //
    // reshape graphs and texts
    //
#ifdef USEOPENGLTEXT
    m_oglText.changeSize(w,h);
    m_oglText.changeCanvas(w,h);
    m_oglTextBig.changeSize(w,h);
    m_oglTextBig.changeCanvas(w,h);
    m_traceDisp.changeSize(w,h);
    m_traceDisp.changeCanvas(w,h);
    m_traceDisp.Position((float)(w-100-10),(float)(h-100-10), 100.0f, 100.0f);
    COGLTraceDisplay::changeSize(w,h);
#endif
    //
    // Let's validate again the base of resource management to make sure things keep consistent
    //

    glViewport(0, 0, w, h);

    float r = (float)w / (float)h;
    m_projection = perspective(m_fov, r, m_near, m_far);
}
#endif //WINDOWINERTIACAMERA_EXTERN
