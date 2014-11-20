#ifndef TRACEDISPLAY_H__
#define TRACEDISPLAY_H__
/*
    Copyright (c) 2012 NVIDIA Corporation. All rights reserved.

    TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
    *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
    OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
    OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
    CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
    OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
    OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
    EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

    Please direct any questions to tlorach@nvidia.com (Tristan Lorach)
*/

#include "trace.h"
#include "../OpenGLText/OpenGLText.h"

/*****************************************************************************************
 * Measuring the current window as [0,1] in width and height, with 0,0 being the bottom left,
 * CTraceDisplay displays a CTrace<float> in a subwindow positioned at _left,_bottom
 * with _width and _height (both [0,1]).
 *
 * You can set the background of the subwindow and also for each trace displayed (including alpha if you'd like).
 *****************************************************************************************/
class CTraceDisplay
{
public:
    /// Different ways to display the Trace
    enum DISPLAY {
        LINE_STREAM,
        LINE_WRAP,
        NEEDLE,
        BAR,
    };
    
    CTraceDisplay(const float left = 0.0f, const float bottom = 0.0f, const float width = 0.5f, const float height = 0.33f, const float line_thickness = 2.0f, bool smooth_line=true)
        :m_left(left), m_bottom(bottom), m_width(width), m_height(height), m_draw_back(true), m_draw_text(true), m_bDisplayMinMax(true), m_line_thickness(line_thickness), m_smooth_line(smooth_line?1:0)
    {
        m_graphs_fill_in = false;
        m_gridSz = 3.0f;
        m_dataScale = 1.0f;
        m_dataBias = 0.0f;
        BackgroundColor(0.1f);
        m_graphs_static_disp = false;
        m_txt_val_color = -1;
        m_txt_name_color = 0;
    }
    virtual ~CTraceDisplay() {}
    
    virtual void Insert(CTrace<float> *const t, const int colorID)
        {m_entries.push_back(Entry(t, colorID));}
    void Clear()
    {
        m_entries.clear();
    }
    void Remove(CTrace<float> *const t)
    {
        Entries::iterator iE = m_entries.begin();
        while(iE != m_entries.end() )
        {
            if(iE->trace == t)
            {
                m_entries.erase(iE);
                return;
            }
            ++iE;
        }
    }
    size_t getNumEntries() {return m_entries.size();}
  
    void Display(const DISPLAY &d, int textOnlyForId=-1)
    {
        Background();
        switch (d){
            case LINE_STREAM: DisplayLineStream(textOnlyForId);break;
            case LINE_WRAP: DisplayLineWrap(textOnlyForId);break;
            case NEEDLE: DisplayNeedle(textOnlyForId);break;
            case BAR: DisplayBar(textOnlyForId);break;
        }
    }
    
    /// generic interface functions
    void BackgroundColor(const float r, const float g, const float b, const float a = 1.0f)
        {m_background_color[0] = r; m_background_color[1] = g; m_background_color[2] = b; m_background_color[3] = a;}
    void BackgroundColor(const float s)
        {BackgroundColor(s, s, s, 0.8f);}
  
    void Position(const float left, const float bottom, const float width, const float height)
    {m_left = left; m_bottom = bottom; m_width = width; m_height = height;}
    void Position(const float left, const float bottom)
    {m_left = left; m_bottom = bottom;}
  
    void DrawBack(const bool b) {m_draw_back = b;}
    bool DrawBack() const {return m_draw_back;}
    void DrawText(const bool b) {m_draw_text=b;}
    bool DrawText() const {return m_draw_text;}
    void DrawRange(const bool b) {m_bDisplayMinMax = b;}
    bool DrawRange() const {return m_bDisplayMinMax;}

    virtual void setFonts(OpenGLText* oglTextHi, OpenGLText* oglText) = 0;
    void setScaleBias(float sc, float bias) { m_dataScale = sc; m_dataBias = bias; }
    void setStaticLabel(bool graphs_static_disp)    { m_graphs_static_disp = graphs_static_disp; }
    void setFillGraph(bool graphs_fill_in)          { m_graphs_fill_in = graphs_fill_in; }
    void setTitle(const char* title)                { m_title = title; }
    void setValueString(const char* vs)             { m_valueString = vs; }
    void setLineThickness(float v)                  { m_line_thickness = v; }
    void setSmoothLine(int smooth_line)             { m_smooth_line = smooth_line; }
    void setNameColorIdx(int txt_name_color)        { m_txt_name_color = txt_name_color; }
    void setValueColorIdx(int txt_val_color)        { m_txt_val_color = txt_val_color; }
protected:
    float     m_dataScale;
    float     m_dataBias;
    float     m_line_thickness;
    float     m_gridSz;
    int       m_smooth_line;
    bool      m_graphs_static_disp;
    bool      m_graphs_fill_in;
    int       m_txt_val_color;
    int       m_txt_name_color;

    std::string m_title;
    std::string m_valueString;

    static    int     s_cNbColors;
    static    float   s_colors[][4];
    static    float   s_edge_color[4];
    static    float   s_grid_color[4];
    static    float   s_fill_color0[4];
    static    float   s_fill_color1[4];

    class Entry {
    public:
        Entry(CTrace<float> *const t, const int colorID_)
            :trace(t),colorID(colorID_) {}
            
        CTrace<float> *trace;
        int colorID;
    };
    
    typedef std::vector<Entry> Entries;
    
    virtual void Background() = 0;
    virtual void Text(const Entries::iterator& it,const float count) = 0;
    virtual void DisplayLineStream(int textOnlyForId=-1) = 0;
    virtual void DisplayLineWrap(int textOnlyForId=-1) = 0;
    virtual void DisplayNeedle(int textOnlyForId=-1) = 0;
    virtual void DisplayBar(int textOnlyForId=-1) = 0;
  
    Entries m_entries;
    float   m_left;
    float   m_bottom;
    float   m_width;
    float   m_height;
    float   m_background_color[4];
    bool    m_draw_back;
    bool    m_draw_text;
    bool    m_bDisplayMinMax;

private:
    CTraceDisplay(const CTraceDisplay &that);
    const CTraceDisplay &operator=(const CTraceDisplay &that);
};

/*****************************************************************************************
 *
 *
 *
 *****************************************************************************************/
class COGLTraceDisplay : public CTraceDisplay {
public:
    COGLTraceDisplay(const float left = 0.0f, const float bottom = 0.0f, const float width = 0.5f, const float height = 0.33f);
    ~COGLTraceDisplay();
    
    void SetTextDrawColumn(bool b) { m_draw_text_column = b; }
    void SetDrawDoubleColumn(bool b) { m_draw_double_column = b; }

    static bool Init(int w, int h); // needed for string rendering
    static void changeSize(int w, int h);
    static void changeCanvas(int w, int h);
    static void Begin();
    static void End();

    virtual void setFonts(OpenGLText* oglTextHi, OpenGLText* oglText)  { m_pTextHi = oglTextHi; m_pTextLo = oglText; }

protected:
    OpenGLText*  m_pTextHi;
    OpenGLText*  m_pTextLo;

    static GLuint m_vbo;
    bool m_draw_text_column;
    bool m_draw_double_column;
    struct Vertex
    {
        float pos[4];
        float color[4];

        Vertex()
        { memset(this, 0, sizeof(Vertex)); }

        void xy( float fx, float fy )
        {
            pos[0] = fx; pos[1] = fy;
        }
        void xyzw( float fx, float fy, float fz, float fw )
        {
            pos[0] = fx; pos[1] = fy;
            pos[2] = fz; pos[3] = fw;
        }
        void xyzw( float fx, float fy, float fz, float fw, int smooth )
        {
            if(smooth)
            {
                pos[0] = fx; pos[1] = fy;
                pos[2] = fz; pos[3] = fw;
            } else {
                pos[0] = fx; pos[1] = fy;
                pos[2] = fx; pos[3] = fy;
            }
        }
        void xyzw( float fx, float fy )
        {
            pos[0] = fx; pos[1] = fy;
            pos[2] = fx; pos[3] = fy;
        }
    };

    static std::vector< unsigned int > m_indices;
    static std::vector< Vertex >       m_vertices;

    static void pushIndex( int vi );
    static void beginStrip();
    static void endStrip();
    static void pushVertex( Vertex* v );

    virtual void Background();
    virtual void Text(const Entries::iterator& it,const float count);
    virtual void DisplayLineStream(int textOnlyForId=-1);
    virtual void DisplayLineWrap(int textOnlyForId=-1);
    virtual void DisplayNeedle(int textOnlyForId=-1);
    virtual void DisplayBar(int textOnlyForId=-1);
};

#endif
