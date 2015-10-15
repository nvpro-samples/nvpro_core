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

#pragma warning(disable:4244) // dble to float conversion warning

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef WIN32
#include <direct.h>
#define MKDIR(file) _mkdir((file))
#else
#include <sys/stat.h>
#define MKDIR(file) mkdir((file),0777)
#endif
#include <errno.h>
#include <math.h>
#include <stdio.h>

#include <GL/glew.h>

#include "tracedisplay.h"
#include "nv_math/nv_math.h"

using namespace nv_math;

#define TGA_NOIMPL
#include "../OpenGLText/tga.h"

#ifdef USESVCUI
void nvprintfLevel(int level, const char * fmt, ...);
#   define  LOGI(...)  { nvprintfLevel(0, __VA_ARGS__); }
#   define  LOGW(...)  { nvprintfLevel(1, __VA_ARGS__); }
#   define  LOGE(...)  { nvprintfLevel(2, __VA_ARGS__); }
#   define  LOGOK(...)  { nvprintfLevel(7, __VA_ARGS__); }
#else
#   define  LOGI(...)  printf(__VA_ARGS__)
#   define  LOGW(...)  printf(__VA_ARGS__)
#   define  LOGE(...)  printf(__VA_ARGS__)
#   define  LOGOK(...)  printf(__VA_ARGS__)
#   define  UPDATEUI(v)
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace nves2 {
extern int g_Width;
extern int g_Height;
}

/////////////////////////////////////////////
// TOCHANGE
const unsigned int c_fontNbChars = 128;
const unsigned int c_fontHeight = 16;
const unsigned int c_fontWidth = 724;

struct TCanvas
{
    float w,h;
    float ratio;
};
TCanvas m_canvas;
GLuint m_widgetProgram = 0;
GLuint m_canvasVar = 0;
GLuint m_gradientTex = 0;
GLenum m_gradientType;
GLuint m_gradientSampler = 0;
float m_vertexDepth = 1.f;
int m_indexOffset = 0;
GLuint CompileGLSLShader( GLenum target, const char* shader);
GLuint LinkGLSLProgram( GLuint vertexShader, GLuint fragmentShader);

static char const* VSSource = {
    "#version 120\n\
    uniform vec4 canvas; \n\
    in vec4 Position;\n\
    in vec4 Col;\n\
    void main()\n\
    {\n\
        //gl_Position = vec4( (((Position.x) / window.z)*2.0 - 1.0), (((Position.y) / window.w)*2.0 - 1.0), 0, 1.0); \n\
        gl_Position = vec4( (((Position.x) / canvas.x)*canvas.z*2.0 - 1.0), \n\
                   (((Position.y) / canvas.y)*2.0 - 1.0), 0, 1.0); \n\
        gl_TexCoord[0] = Col; \n\
        vec2 vd = Position.xy - Position.zw; \n\
        gl_TexCoord[1] = vec4(vd, dot(vd,vd), 0); \n\
    }\n\
    "};

static char const* FSSource = {
    "#version 120\n\
    uniform sampler1D gradientSampler;\n\
    void main()\n\
    {\n\
        vec4 color; \n\
        color = gl_TexCoord[0];\n\
        float d = gl_TexCoord[1].z > 0.0 ? 1.0-((dot(gl_TexCoord[1].xy,gl_TexCoord[1].xy))/gl_TexCoord[1].z) : 1.0;\n\
        gl_FragColor = color; \n\
        gl_FragColor.a *= d; \n\
    }\n\
    "};

static int locPos = 0;
static int locCol = 0;

std::vector< unsigned int >             COGLTraceDisplay::m_indices;
std::vector< COGLTraceDisplay::Vertex > COGLTraceDisplay::m_vertices;

GLuint COGLTraceDisplay::m_vbo = 0;

float   CTraceDisplay::s_edge_color[4] = {0.9f,0.7f,0.7f,0.8f};
float   CTraceDisplay::s_grid_color[4] = {0.8f,0.8f,0.8f,0.8f};

float   CTraceDisplay::s_fill_color0[4] = {1.0f,0.0f,0.0f,0.3f};
float   CTraceDisplay::s_fill_color1[4] = {1.0f,0.0f,0.0f,0.3f};


int     CTraceDisplay::s_cNbColors = 24;
float   CTraceDisplay::s_colors[][4] = 
{
    {1.0f, 1.0f, 1.0f, 0.3f},
    {0.7f, 0.0f, 0.0f, 0.6f},
    {0.0f, 0.0f, 0.7f, 0.5f},
    {0.7f, 0.0f, 0.7f, 0.5f},
    {0.7f, 0.7f, 0.0f, 1.0f},
    {0.7f, 0.7f, 0.0f, 1.0f},
    {0.7f, 0.7f, 0.7f, 1.0f},
    {0.0f, 0.7f, 0.7f, 1.0f},
    {0.4f, 0.4f, 0.4f, 1.0f},
    {0.4f, 0.7f, 0.4f, 1.0f},
    {0.7f, 0.4f, 0.4f, 1.0f},
    {0.4f, 0.4f, 0.7f, 1.0f},
    {0.7f, 0.4f, 0.7f, 1.0f},
    {0.7f, 0.7f, 0.4f, 1.0f},
    {0.7f, 0.7f, 0.4f, 1.0f},
    {0.4f, 0.7f, 0.7f, 1.0f},
    {0.0f, 0.0f, 0.5f, 0.8f},
    {0.0f, 0.0f, 0.8f, 0.8f},
    {0.5f, 0.0f, 0.0f, 0.8f},
    {0.0f, 0.7f, 0.0f, 0.8f},
    {0.7f, 0.0f, 0.7f, 0.8f},
    {0.3f, 0.3f, 0.7f, 0.7f},
    {0.3f, 0.7f, 0.3f, 0.7f},
    {0.6f, 0.7f, 0.3f, 0.7f},
};

void COGLTraceDisplay::pushIndex( int vi )
{
    if ((int) m_indices.size() == m_indexOffset)
        m_indices.push_back( vi );
    else if ((int) m_indices.size() <= m_indexOffset)
    {
        m_indices.resize( m_indexOffset );
        m_indices[m_indexOffset] = vi;
    }
    else
        m_indices[m_indexOffset] = vi;
    m_indexOffset++;
}

void COGLTraceDisplay::beginStrip()
{
    //if (m_indices.size() > 0)
    //{
    //    pushIndex( m_vertices.size());
    //}
}

void COGLTraceDisplay::endStrip()
{
    //pushIndex( m_vertices.size() - 1 );
    pushIndex( - 1 );
}
void COGLTraceDisplay::pushVertex( Vertex* v )
{
    pushIndex( (unsigned int)m_vertices.size() );
    m_vertices.push_back( *v );
}

void COGLTraceDisplay::changeCanvas(int w, int h)
{
    m_canvas.w = w;
    m_canvas.h = h;
}
void COGLTraceDisplay::changeSize(int w, int h)
{
    m_canvas.ratio = ((float)h*m_canvas.w)/((float)w*m_canvas.h);
}
/**************************************************/
void COGLTraceDisplay::Begin()
{
    m_indexOffset = 0;
}

/**************************************************/
void COGLTraceDisplay::End()
{
    if(!m_vertices.empty())
    {
        //glPushAttrib( GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT );
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable( GL_CULL_FACE );
        glDisable(GL_STENCIL_TEST);
        glStencilMask( 0 );
        glDisable(GL_DEPTH_TEST);
        glDepthMask( GL_FALSE );
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
        glPrimitiveRestartIndex(-1);
        glEnable(GL_PRIMITIVE_RESTART);
        glEnableVertexAttribArray( locPos );

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)* m_vertices.size(), m_vertices[0].pos, GL_STREAM_DRAW);

        glVertexAttribPointer( locPos, 4, GL_FLOAT, false, sizeof(Vertex), ((COGLTraceDisplay::Vertex*)NULL)->pos );
        if(locCol >=0)
        {
            glEnableVertexAttribArray( locCol );
            glVertexAttribPointer( locCol, 4, GL_FLOAT, false, sizeof(Vertex), ((COGLTraceDisplay::Vertex*)NULL)->color );
        }
        for(int i=2; i<16; i++)
            glDisableVertexAttribArray( i );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

        glUseProgram( m_widgetProgram );

        glUniform4f( m_canvasVar, (float)m_canvas.w, (float)m_canvas.h, (float)m_canvas.ratio, 0 );

        glDrawElements( GL_TRIANGLE_STRIP, m_indexOffset, GL_UNSIGNED_INT, &(m_indices.front()) );

        glUseProgram( 0 );
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        glDepthMask( GL_TRUE );
        glDisable(GL_BLEND);
        glDisable(GL_PRIMITIVE_RESTART);


        // switch vertex attribs back off
        glDisableVertexAttribArray( locPos );
        if(locCol >= 0)
            glDisableVertexAttribArray( locCol );
        //CHECKGLERRORS();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_vertices.resize(0);
        m_indices.resize(0);
        m_indexOffset = 0;
    }
}

/**************************************************/
void COGLTraceDisplay::Background()
{
    if (m_draw_back)
    {
        Vertex v;
        v.color[0] = m_background_color[0];//(float)(color>>24 & 0xFF)/255.0;
        v.color[1] = m_background_color[1];//(float)(color>>16 & 0xFF)/255.0;
        v.color[2] = m_background_color[2];//(float)(color>>8 & 0xFF)/255.0;
        v.color[3] = m_background_color[3];//(float)(color & 0xFF)/255.0;
        beginStrip();
            // zw contains the same so that in the shader the distance is 0 and we don't do any smoothing op.
            v.xyzw( m_left ,       m_bottom,         m_left ,       m_bottom);           pushVertex(&v);
            v.xyzw( m_left ,       m_bottom+m_height, m_left ,       m_bottom+m_height);   pushVertex(&v);
            v.xyzw( m_left+m_width, m_bottom,         m_left+m_width, m_bottom);           pushVertex(&v);
            v.xyzw( m_left+m_width, m_bottom+m_height, m_left+m_width, m_bottom+m_height);   pushVertex(&v);
        endStrip();
    }
}

/**************************************************/
void COGLTraceDisplay::Text(const Entries::iterator &it,const float count)
{
#if 0
    int w, h;
    w = nves2::g_Width;
    h = nves2::g_Height;
    // NOTE: the font rendering is using GLUT. Sorry : if no glut, no fonts...
    if (m_draw_text_column) {
        const float deltax = float(m_width)/m_entries.size();
        float centerAlign;
        char msg[256];
        int n, p;
        int line = 0;

        // Name
        n = sprintf(msg,"%s", it->trace->name().c_str());
        if(msg[0] != '\0')
            line++;
        p = 0;
        // we don't use glut... only for text rendering here. Needed to replace some functions
        centerAlign = (((float)p / (float)w)) / 2.0f;

        drawString( w*(m_left + centerAlign + (count * deltax)), h*(m_bottom) - line*c_fontHeight, msg, 0, it->colorID, 1.0f);
        line++;
        if (m_draw_double_column)
        {
            n = sprintf(msg,"[%d, %d]", (int)it->trace->secondToLast(), (int)it->trace->last());
            p = 0;
            w = nves2::g_Width;
            centerAlign = (((float)p / (float)w)) / 2.0f;

            drawString( w*(m_left + centerAlign + (count * deltax)), h*(m_bottom) - line*c_fontHeight, msg, 0, it->colorID, 1.0f);
        }
        else 
        {
            n = sprintf(msg,"[%.2f]", (float)it->trace->last());
            drawString(w*(m_left + (count * deltax)), h*(m_bottom) - line*c_fontHeight, msg, 0, it->colorID, 1.0f);
        }

    } else if (m_draw_text)
    {
        const float deltax = float(m_width)/2.0f;
        char msg[256];
        int n = sprintf(msg,"%s [%.2f]",it->trace->name().c_str(), (float)it->trace->last());
        drawString(w*(m_left + (((int)count % 2) * deltax)), h*(m_bottom) - c_fontHeight - ((int)(count / 2) * c_fontHeight), msg, 0, it->colorID, 1.0f);
    }
#endif
}

/**************************************************/
void COGLTraceDisplay::DisplayLineStream(int textOnlyForId)
{
    float count = 0;
    Vertex v;
    //
    // render the edges
    //
    v.color[0] = s_edge_color[0];
    v.color[1] = s_edge_color[1];
    v.color[2] = s_edge_color[2];
    v.color[3] = s_edge_color[3];
    //beginStrip();
    //    v.xyzw( m_left-_gridSz*2.0f, m_bottom, m_left-m_gridSz         , m_bottom, m_smooth_line); pushVertex(&v);
    //    v.xyzw( m_left             , m_bottom, m_left-m_gridSz         , m_bottom, m_smooth_line); pushVertex(&v);
    //    v.xyzw( m_left-m_gridSz*2.0f, m_bottom+m_height, m_left-m_gridSz , m_bottom+m_height, m_smooth_line); pushVertex(&v);
    //    v.xyzw( m_left             , m_bottom+m_height, m_left-m_gridSz , m_bottom+m_height, m_smooth_line); pushVertex(&v);
    //endStrip();
    beginStrip();
        v.color[3] = 0;
        v.xyzw( m_left-m_gridSz*2.0f, m_bottom          , m_left-m_gridSz*2.0f, m_bottom-m_gridSz  , m_smooth_line); pushVertex(&v);
        v.xyzw( m_left-m_gridSz*2.0f, m_bottom-m_gridSz*2.0f, m_left-m_gridSz*2.0f, m_bottom-m_gridSz  , m_smooth_line); pushVertex(&v);
        v.color[3] = s_edge_color[3];
        v.xyzw( m_left+m_width   , m_bottom          , m_left+m_width   , m_bottom-m_gridSz  , m_smooth_line); pushVertex(&v);
        v.xyzw( m_left+m_width   , m_bottom-m_gridSz*2.0f, m_left+m_width   , m_bottom-m_gridSz  , m_smooth_line); pushVertex(&v);
    endStrip();
    //
    // Horizontal grid
    //
    v.color[0] = s_grid_color[0];
    v.color[1] = s_grid_color[1];
    v.color[2] = s_grid_color[2];
    v.color[3] = s_grid_color[3];
    float l = m_dataScale;
    float e = (float)log10f(l);
    e = floor(e-0.5);
    l = powf(10.0, e);
    e= (fmodf(-m_dataBias,l));
    //float dy = l * stepY;
    float i = 0;
    do {
        float y = m_bottom + m_height * ((i*l + e)) / m_dataScale;
        i += 1.0f;
        if(y > m_bottom + m_height)
            break;
        beginStrip();
            v.color[3] = 0.0;
            v.xyzw( m_left          , y          , m_left, y-m_gridSz*0.5, m_smooth_line); pushVertex(&v);
            v.xyzw( m_left          , y-m_gridSz  , m_left, y-m_gridSz*0.5, m_smooth_line); pushVertex(&v);
            if(((int)i%4) == 0)
                v.color[3] = CTraceDisplay::s_colors[3][3];
            else
                v.color[3] = CTraceDisplay::s_colors[3][3]*0.2;
            v.xyzw( m_left+m_width   , y          , m_left+m_width, y-m_gridSz*0.5, m_smooth_line); pushVertex(&v);
            v.xyzw( m_left+m_width   , y-m_gridSz  , m_left+m_width, y-m_gridSz*0.5, m_smooth_line); pushVertex(&v);
        endStrip();
    } while(1);
    //
    // Render the title as an overlay
    //
    float textDim[2];
    if(!m_title.empty())
    {
        m_pTextHi->stringSize(m_title.c_str(), textDim);
        m_pTextHi->drawString((int)(m_left + m_width*0.5 - textDim[0]*0.5) , 
                             (int)(m_bottom + m_height*0.5 - textDim[1]*0.5 + 6), 
                             m_title.c_str(), 0, CTraceDisplay::s_colors[m_txt_name_color]);
    }
    //
    // Render the curves
    //
    for(Entries::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        float min = it->trace->min();
        float max = it->trace->max();
        float magic = max - min;
        vec2f p;
        vec2f p0;
        vec2f p1;
        //
        // render below the curve
        //
        if(m_graphs_fill_in)
        {
            beginStrip();
            int N = (int)it->trace->size();
            for (size_t i=0;i<N;++i)
            {
                p0.x = m_left+m_width*float(i)/float(it->trace->capacity()-1);
                //y = m_bottom+m_height*((*(it->trace))(i) - min)/magic;
                //y = y < min ? min : (y > max ? max : (y != y ? m_bottom : y));
                float fval = (*(it->trace))(i);
                p0.y = m_height * (fval - m_dataBias) / m_dataScale;
                p0.y = p0.y < 0.0 ? 0.0 : (p0.y > m_height ? m_height : p0.y);
                p0.y += m_bottom;
                if(i>1)
                {
                    float f = 1.0 - ((float)i/(float)N);
                    float hf1 = ((fval - m_dataBias) / m_dataScale);
                    if(hf1 > 1.0) hf1 = 1.0;
                    else if(hf1 < 0.0) hf1 = 0.0;
                    float hf0 = 1.0 - hf1;
                    float alpha = (1.0-f*f*f) * 0.6;
                    if(m_smooth_line)
                    {
                        v.color[0] = s_fill_color0[0];
                        v.color[1] = s_fill_color0[1];
                        v.color[2] = s_fill_color0[2];
                        v.color[3] = s_fill_color0[3]*alpha;
                        v.xyzw( p0.x, m_bottom, p0.x, m_bottom); pushVertex(&v);
                        v.color[0] = s_fill_color0[0] * hf0 + s_fill_color1[0] * hf1;
                        v.color[1] = s_fill_color0[1] * hf0 + s_fill_color1[1] * hf1;
                        v.color[2] = s_fill_color0[2] * hf0 + s_fill_color1[2] * hf1;
                        v.color[3] = (s_fill_color0[3] * hf0 + s_fill_color1[3] * hf1)*alpha;
                        v.xyzw( p0.x, p0.y, p0.x, p0.y); pushVertex(&v);
                    } else {
                        v.color[0] = s_fill_color0[0];
                        v.color[1] = s_fill_color0[1];
                        v.color[2] = s_fill_color0[2];
                        v.color[3] = s_fill_color0[3]*alpha;
                        v.xyzw( p0.x, m_bottom); pushVertex(&v);
                        v.color[0] = s_fill_color0[0] * hf0 + s_fill_color1[0] * hf1;
                        v.color[1] = s_fill_color0[1] * hf0 + s_fill_color1[1] * hf1;
                        v.color[2] = s_fill_color0[2] * hf0 + s_fill_color1[2] * hf1;
                        v.color[3] = (s_fill_color0[3] * hf0 + s_fill_color1[3] * hf1)*alpha;
                        v.xyzw( p0.x, p0.y); pushVertex(&v);
                    }
                }
            }
            endStrip();
        }
        //
        // render the line of the curve
        //
        float alpha;
        if(count == textOnlyForId)
        {
            v.color[0] = 1.0;
            v.color[1] = 1.0;
            v.color[2] = 1.0;
            alpha = 1.0;
        } else {
            v.color[0] = CTraceDisplay::s_colors[it->colorID][0];
            v.color[1] = CTraceDisplay::s_colors[it->colorID][1];
            v.color[2] = CTraceDisplay::s_colors[it->colorID][2];
            alpha = CTraceDisplay::s_colors[it->colorID][3];
        }
        beginStrip();
        int sz = (int)it->trace->size();
        for (size_t i=0;i<sz;++i)
        {
            p0.x = m_left+m_width*float(i)/float(it->trace->capacity()-1);
            //y = m_bottom+m_height*((*(it->trace))(i) - min)/magic;
            //y = y < min ? min : (y > max ? max : (y != y ? m_bottom : y));
            float fval = (*(it->trace))(i);
            p0.y = m_height * (fval - m_dataBias) / m_dataScale;
            p0.y = p0.y < 0.0 ? 0.0 : (p0.y > m_height ? m_height : p0.y);
            p0.y += m_bottom;
            if(i==sz-1)
            {
                if(!m_valueString.empty())
                {
                    static char valstr[50];
                    sprintf(valstr, m_valueString.c_str(), fval);
                    m_pTextLo->stringSize(valstr, textDim);
                    m_pTextLo->drawString(
                        m_graphs_static_disp ? m_left+2 : (int)(p0.x - textDim[0]), 
                        m_graphs_static_disp ? m_bottom + 2 : (int)p0.y + 5, 
                        valstr, 0, CTraceDisplay::s_colors[m_txt_val_color<0?it->colorID:m_txt_val_color]);
                }
            }
            if(i>1)
            {
                vec2f dir = p0 - p1;
                dir = normalize(dir);
                dir = vec2f(-dir.y, dir.x);
                dir *= m_line_thickness;
                vec2f pp1 = p - dir;
                vec2f pp2 = p + dir;
                v.color[3] = ((float)i/(float)sz) * alpha;
                if(m_smooth_line)
                {
                    v.xyzw( pp1.x, pp1.y, p.x, p.y); pushVertex(&v);
                    v.xyzw( pp2.x, pp2.y, p.x, p.y); pushVertex(&v);
                } else {
                    v.xyzw( pp1.x, pp1.y); pushVertex(&v);
                    v.xyzw( pp2.x, pp2.y); pushVertex(&v);
                }
            }
            p1 = p;
            p = p0;
        }
        endStrip();

        if((textOnlyForId < 0)||((textOnlyForId == count)))
                Text(it,count);
        ++count;
    } // for(Entries...
}

/**************************************************/
void COGLTraceDisplay::DisplayLineWrap(int textOnlyForId)
{
    // TODO: need to convert the code for non-immediate mode
#if 0
    float count = 0;
    for(Entries::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        if(count == textOnlyForId)
            glColor4f(CTraceDisplay::s_colors[it->colorID][0],CTraceDisplay::s_colors[it->colorID][1],CTraceDisplay::s_colors[it->colorID][2],1.0f);
        else
            glColor4fv(CTraceDisplay::s_colors[it->colorID]);
        glBegin(GL_LINE_STRIP);
        float magic = it->trace->max() - it->trace->min();
        for (size_t i=0;i<it->trace->size();++i)
        {
            glVertex2f(m_left+m_width*float(i)/float(it->trace->capacity()-1),
            m_bottom+m_height*((*(it->trace))[i]-it->trace->min())/magic);
        }
        glEnd();
        if((textOnlyForId < 0)||((textOnlyForId == count)))
                Text(it,count);
        ++count;
    }
#endif
}

/**************************************************/
void COGLTraceDisplay::DisplayNeedle(int textOnlyForId)
{
    // TODO: need to convert the code for non-immediate mode
#if 0
    float base_percent = 15.0f;
    float count = 0;
    for(Entries::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        if(count == textOnlyForId)
            glColor4f(CTraceDisplay::s_colors[it->colorID][0],CTraceDisplay::s_colors[it->colorID][1],CTraceDisplay::s_colors[it->colorID][2],1.0f);
        else
            glColor4fv(CTraceDisplay::s_colors[it->colorID]);
        glBegin(GL_TRIANGLES);
        float magic = it->trace->max() - it->trace->min();
        float a = float(1) - (it->trace->last()-it->trace->min())/magic;
        a *= float(M_PI);
        glVertex2f(m_left+m_width/float(2)+m_width*cosf(a+M_PI/float(2))/base_percent,m_bottom+m_height*sinf(a+M_PI/float(2))/base_percent);
        glVertex2f(m_left+m_width/float(2)+m_width*cosf(a-M_PI/float(2))/base_percent,m_bottom+m_height*sinf(a-M_PI/float(2))/base_percent);
        glVertex2f(m_left+m_width/float(2)+m_width*cosf(a)/float(2),m_bottom+m_height*sinf(a));
        glEnd();
        if((textOnlyForId < 0)||((textOnlyForId == count)))
                Text(it,count);
        ++count;
    }
#endif
}

/**************************************************/
void COGLTraceDisplay::DisplayBar(int textOnlyForId)
{
    // TODO: need to convert the code for non-immediate mode
#if 0
    float count = 0;
    float max = (float)m_entries.size();
    if (m_draw_double_column) {
        for(Entries::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
        {
            float magic = it->trace->max() - it->trace->min();
            float data0 = it->trace->secondToLast();
            float data1 = it->trace->last();

            float localLeft   = m_left + ((((0.0f + count)*m_width)/max) * 1.02f);
            float localMiddle = m_left + ((((0.5f + count)*m_width)/max) * 1.00f);
            float localRight  = m_left + ((((1.0f + count)*m_width)/max) * 0.98f);

            glBegin(GL_QUADS);

            if(count == textOnlyForId)
                glColor4f(CTraceDisplay::s_colors[it->colorID][0],CTraceDisplay::s_colors[it->colorID][1],CTraceDisplay::s_colors[it->colorID][2],1.0f);
            else
                glColor4fv(CTraceDisplay::s_colors[it->colorID]);
            glVertex2f(localLeft,   m_bottom);
            glVertex2f(localMiddle, m_bottom);
            glVertex2f(localMiddle, m_bottom+m_height*(data0-it->trace->min())/magic);
            glVertex2f(localLeft,   m_bottom+m_height*(data0-it->trace->min())/magic);

            if(count == textOnlyForId)
                glColor4f(CTraceDisplay::s_colors[it->colorID][0],CTraceDisplay::s_colors[it->colorID][1],CTraceDisplay::s_colors[it->colorID][2],1.0f);
            else
                glColor4fv(CTraceDisplay::s_colors[it->colorID]);
            glVertex2f(localMiddle, m_bottom);
            glVertex2f(localRight,  m_bottom);
            glVertex2f(localRight,  m_bottom+m_height*(data1-it->trace->min())/magic);
            glVertex2f(localMiddle, m_bottom+m_height*(data1-it->trace->min())/magic);

            glEnd();

            if((textOnlyForId < 0)||((textOnlyForId == count)))
                Text(it,count);
            ++count;
        }
    } else {
        for(Entries::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
        {
            if(count == textOnlyForId)
                glColor4f(CTraceDisplay::s_colors[it->colorID][0],CTraceDisplay::s_colors[it->colorID][1],CTraceDisplay::s_colors[it->colorID][2],1.0f);
            else
                glColor4fv(CTraceDisplay::s_colors[it->colorID]);
            glBegin(GL_QUADS);
            float magic = it->trace->max() - it->trace->min();
//glColor4fv(m_background_color);
//glBegin(GL_QUADS);
//glVertex2f(m_left,            m_bottom);
//glVertex2f(m_left+m_width,    m_bottom);
//glVertex2f(m_left+m_width,    m_bottom+m_height);
//glVertex2f(m_left,            m_bottom+m_height);
//glEnd();

            glVertex2f(m_left + count*m_width/max,        m_bottom);
            glVertex2f(m_left + (1.0f+count)*m_width/max,    m_bottom);
            glVertex2f(m_left + (1.0f+count)*m_width/max,    m_bottom+m_height*(it->trace->last()-it->trace->min())/magic);
            glVertex2f(m_left + count*m_width/max,        m_bottom+m_height*(it->trace->last()-it->trace->min())/magic);
            glEnd();
            if((textOnlyForId < 0)||((textOnlyForId == count)))
                Text(it,count);
            ++count;
        }
    }
#endif
}

COGLTraceDisplay::COGLTraceDisplay(const float left, const float bottom, const float width, const float height) :
    CTraceDisplay(left, bottom, width, height), m_draw_text_column(false), m_draw_double_column(false)
{
    m_pTextHi    = NULL;
    m_pTextLo    = NULL;
}

COGLTraceDisplay::~COGLTraceDisplay()
{
    //terminate();
}

bool COGLTraceDisplay::Init(int w, int h)
{
    GLuint vShader = CompileGLSLShader( GL_VERTEX_SHADER, VSSource);
    GLuint fShader = CompileGLSLShader( GL_FRAGMENT_SHADER, FSSource);
    m_widgetProgram = LinkGLSLProgram( vShader, fShader );
    glUseProgram(0);
    locPos = glGetAttribLocation( m_widgetProgram, "Position" );
    locCol = glGetAttribLocation( m_widgetProgram, "Col" );
    m_canvasVar         = glGetUniformLocation( m_widgetProgram, "canvas" );
    m_canvas.w = w;
    m_canvas.h = h;
    m_canvas.ratio = 1.0;

    glGenBuffers(1, &m_vbo);
    //m_gradientSampler       = glGetUniformLocation( m_widgetProgram, "gradientSampler" );
    ////
    //// Load the Font if possible
    ////
    //static char fontName[] = "art_asset\\heat_ramp.dds";
    ////if(fontName)
    //{
    //    char fname[200];
    //    sprintf(fname, "%s.tga", fontName);
    //    TGA* fontTGA = new TGA;
    //    TGA::TGAError err = fontTGA->load(fname);
    //    if(err != TGA::TGA_NO_ERROR)
    //        return false;
    //    // Change the shaders to use for this case

    //    int u = fontTGA->m_nImageWidth;
    //    int h = fontTGA->m_nImageHeight;
    //    m_gradientType = GL_TEXTURE_2D;
    //    if(h==1)
    //        m_gradientType = GL_TEXTURE_1D;
    //    if(m_gradientTex == 0)
    //        glGenTextures( 1, &m_gradientTex );
    //    glActiveTexture( GL_TEXTURE0 );
    //    glBindTexture( m_gradientType, m_gradientTex );
    //    glTexParameterf(m_gradientType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //    glTexParameterf(m_gradientType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //    glTexParameterf(m_gradientType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //    glTexParameterf(m_gradientType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //    GLenum extFmt;
    //    switch(fontTGA->m_texFormat)
    //    {
    //    case TGA::RGB:
    //        extFmt = GL_RGB;
    //        break;
    //    case TGA::RGBA:
    //        extFmt = GL_RGBA;
    //        break;
    //    case TGA::ALPHA:
    //        extFmt = GL_LUMINANCE;
    //        break;
    //    }
    //    if(h==1)
    //        glTexImage1D(GL_TEXTURE_1D, 0, extFmt, u, 0, extFmt, GL_UNSIGNED_BYTE, fontTGA->m_nImageData);
    //    else
    //        glTexImage2D(GL_TEXTURE_2D, 0, extFmt, u, h, 0, extFmt, GL_UNSIGNED_BYTE, fontTGA->m_nImageData);
    //    GLenum glerr = glGetError();

    //    glBindTexture( m_gradientType, 0 );
    //    delete fontTGA;
    //}
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
GLuint CompileGLSLShader( GLenum target, const char* shader)
{
    GLuint object;

    object = glCreateShader( target);

    if (!object)
        return object;

    glShaderSource( object, 1, &shader, NULL);

    glCompileShader(object);

    // check if shader compiled
    GLint compiled = 0;
    glGetShaderiv(object, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        char temp[256] = "";
        glGetShaderInfoLog( object, 256, NULL, temp);
        LOGE( "Compile failed:\n%s\n", temp);
        glDeleteShader( object);
        return 0;
    }

    return object;
}

// Create a program composed of vertex and fragment shaders.
GLuint LinkGLSLProgram( GLuint vertexShader, GLuint fragmentShader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    // Test linker result.
    GLint linkSucceed = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkSucceed);
    if(!linkSucceed)
    {
        // Get error log.
        GLint charsWritten, infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        char * infoLog = new char[infoLogLength];
        glGetProgramInfoLog(program, infoLogLength, &charsWritten, infoLog);
        LOGE(infoLog);
        delete [] infoLog;
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

