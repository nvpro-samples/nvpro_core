/*
    developed by Tristan Lorach - Copyright (c) 2012 NVIDIA Corporation. All rights reserved.

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
    ===================================================================================
    Example on how to use it.
    init time:
        OpenGLText             oglText;
        ...
        if(!oglText.init(font_name, canvas_width, canvas_height))
            return false;

    render time:
        oglText.beginString();
        float bbStr[2];
        char *tmpStr = "Hello world";
        oglText.stringSize(tmpStr, bbStr);
        oglText.drawString(posX - bbStr[0]*0.5, posY - bbStr[1], tmpStr, 0,0xF0F0F0F0);
        ...
        oglText.endString(); // will render the whole at once
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

#include <math.h>

#if defined(__APPLE__)
    #include <GLUT/glut.h>
#else
    #include <GL/glew.h>
#endif
#define GL(f) gl##f

#include "OpenGLText.h"
#include "tga.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

///////////////////////////////////////////////////////////////////////////////////////
// Backup only the states that we really need to setup
// the rendering. We might want to back them up once
//
struct TextBackupState
{
    struct vtxAttribData {
        GLvoid* ptr;
        int     enabled;
        int     size;
        int     type;
        int     normalized;
        int     stride;
        int     bufferBinding;
    };
    TextBackupState() { valid = false; }
    ~TextBackupState() { if(valid) {
        free(attribs);
    } }
    void backup()
    {
        GL(GetIntegerv(GL_POLYGON_MODE, (GLint*)mode));
        cull = GL(IsEnabled(GL_CULL_FACE));
        //GL(GetIntegerv(GL_CULL_FACE_MODE, (GLint*)&));
        stenciltest = GL(IsEnabled(GL_STENCIL_TEST));
        GL(GetIntegerv(GL_STENCIL_VALUE_MASK, (GLint*)&stencilmask));
        depthtest = GL(IsEnabled(GL_DEPTH_TEST));
        GL(GetIntegerv(GL_DEPTH_WRITEMASK, (GLint*)&depthmask));
        blend = GL(IsEnabled(GL_BLEND));
        GL(GetIntegerv(GL_BLEND_SRC, (GLint*)&sfactor));
        GL(GetIntegerv(GL_BLEND_DST, (GLint*)&dfactor));
        GL(GetIntegerv(GL_COLOR_WRITEMASK, (GLint*)&red));
        primrestart = GL(IsEnabled(GL_PRIMITIVE_RESTART));
        GL(GetIntegerv(GL_PRIMITIVE_RESTART_INDEX, (GLint*)&primrestartid));
        GL(GetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVtxAttribs));
        attribs = (vtxAttribData*)malloc(sizeof(vtxAttribData)*maxVtxAttribs);
        for(int i=0; i<maxVtxAttribs; i++)
        {
            GL(GetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &(attribs[i].enabled)));
            GL(GetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &(attribs[i].size)));
            GL(GetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &(attribs[i].type)));
            GL(GetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &(attribs[i].normalized)));
            GL(GetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &(attribs[i].stride)));
            GL(GetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &(attribs[i].bufferBinding)));
            GL(GetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &(attribs[i].ptr)));
        }
        valid = true;
    }
    void restore()
    {
        if(!valid)
            return;
        GL(PolygonMode(GL_FRONT_AND_BACK, mode[0]));
        //GL(PolygonMode(GL_BACK, mode[1])); deprecated in Core GL !!!!!!
        if(cull) GL(Enable(GL_CULL_FACE));
        else     GL(Disable(GL_CULL_FACE));
        if(stenciltest) GL(Enable(GL_STENCIL_TEST));
        else            GL(Disable(GL_STENCIL_TEST));
        GL(StencilMask( stencilmask ));
        if(depthtest)   GL(Enable(GL_DEPTH_TEST));
        else            GL(Disable(GL_DEPTH_TEST));
        GL(DepthMask( depthmask ));
        if(blend)       GL(Enable(GL_BLEND));
        else            GL(Disable(GL_BLEND));
        GL(BlendFunc(sfactor, dfactor));
        GL(ColorMask( red,green,blue,alpha ));
        GL(PrimitiveRestartIndex(primrestartid));
        if(primrestart) GL(Enable(GL_PRIMITIVE_RESTART));
        else            GL(Disable(GL_PRIMITIVE_RESTART));
        for(int i=0; i<maxVtxAttribs; i++)
        {
            if(attribs[i].enabled)
                GL(EnableVertexAttribArray(i));
            else
                GL(DisableVertexAttribArray(i));
            GL(BindBuffer(GL_ARRAY_BUFFER, attribs[i].bufferBinding));
            GL(VertexAttribPointer(i, attribs[i].size, attribs[i].type, attribs[i].normalized, attribs[i].stride, attribs[i].ptr));
        }
        GL(BindBuffer(GL_ARRAY_BUFFER, 0));
        // TODO: texture
    }
    void setStates()
    {
        // fill mode always
        GL(PolygonMode(GL_FRONT_AND_BACK, GL_FILL));
        GL(Disable( GL_CULL_FACE ));
        // Stencil / Depth buffer and test disabled
        GL(Disable(GL_STENCIL_TEST));
        GL(StencilMask( 0 ));
        GL(Disable(GL_DEPTH_TEST));
        GL(DepthMask( GL_FALSE ));
        // Blend on for alpha
        GL(Enable(GL_BLEND));
        GL(BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        // Color active
        GL(ColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE ));
        GL(PrimitiveRestartIndex(-1));
        GL(Enable(GL_PRIMITIVE_RESTART));
    }
    bool    valid;
    int     maxVtxAttribs;
    vtxAttribData *attribs;

    GLenum mode[2];
    GLenum cull;
    GLenum stenciltest;
    int stencilmask;
    GLenum depthtest;
    GLint depthmask;
    GLenum blend;
    GLenum sfactor; GLenum dfactor;
    GLint red; GLint green; GLint blue; GLint alpha;
    GLuint primrestartid;
    GLenum primrestart;
};
static TextBackupState bs;

///////////////////////////////////////////////////////////////////////////////////////
//
//
OpenGLText::OpenGLText()
{
    c_fontNbChars           = 128;
    c_fontHeight            = 16;
    c_fontWidth             = 724;
    m_widgetProgram         = 0;
    m_vShader               = 0;
    m_fShader               = 0;
    m_ebo                   = 0;
    m_vbo                   = 0;
    m_ebosz                 = 0;
    m_vbosz                 = 0;
    m_canvasVar             = 0;
    m_fontTex               = 0;
    m_vertexDepth           = 1.f;
    m_indexOffset           = 0;
    glyphInfos              = NULL;
    allocated               = false;
    //m_window;
};
OpenGLText::~OpenGLText()
{
    //if(m_vShader)
        //glDeleteSh
    m_widgetProgram         = 0;
    m_vShader               = 0;
    m_fShader               = 0;
    if(glyphInfos && allocated)
        delete glyphInfos;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
inline GLuint OpenGLText::CompileGLSLShader( GLenum target, const char* shader)
{
    GLuint object;

    object = GL(CreateShader( target));

    if (!object)
        return object;

    GL(ShaderSource( object, 1, &shader, NULL));

    GL(CompileShader(object));

    // check if shader compiled
    GLint compiled = 0;
    GL(GetShaderiv(object, GL_COMPILE_STATUS, &compiled));

    if (!compiled)
    {
#ifdef NV_REPORT_COMPILE_ERRORS
        char temp[256] = "";
        GL(GetShaderInfoLog( object, 256, NULL, temp));
        fprintf( stderr, "Compile failed:\n%s\n", temp);
#endif
        GL(DeleteShader( object));
        return 0;
    }

    return object;
}

// Create a program composed of vertex and fragment shaders.
inline GLuint OpenGLText::LinkGLSLProgram( GLuint vertexShader, GLuint fragmentShader)
{
    GLuint program = GL(CreateProgram());
    GL(AttachShader(program, vertexShader));
    GL(AttachShader(program, fragmentShader));
    GL(LinkProgram(program));

#ifdef NV_REPORT_COMPILE_ERRORS
    // Get error log.
    GLint charsWritten, infoLogLength;
    GL(GetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength));

    char * infoLog = new char[infoLogLength];
    GL(GetProgramInfoLog(program, infoLogLength, &charsWritten, infoLog));
    printf(infoLog);
    delete [] infoLog;
#endif

    // Test linker result.
    GLint linkSucceed = GL_FALSE;
    GL(GetProgramiv(program, GL_LINK_STATUS, &linkSucceed));
    
    if (linkSucceed == GL_FALSE)
    {
        GL(DeleteProgram(program));
        return 0;
    }

    return program;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
char* OpenGLText::cWidgetVSSource2 = {
    "#version 120\n\
    uniform vec4 canvas; \n\
\n\
    in vec4 Position;\n\
    in vec4 TexCoord;\n\
    in vec4 Color;\n\
\n\
    void main()\n\
    {\n\
        gl_Position = vec4( (((Position.x) / canvas.x)*canvas.z*2.0 - 1.0), \n\
                   (((Position.y) / canvas.y)*2.0 - 1.0), 0, 1.0); \n\
        gl_TexCoord[0] = TexCoord; \n\
        gl_TexCoord[1] = Color; \n\
    }\n\
    "};

char* OpenGLText::cWidgetFSSource2 = {
    "#version 120\n\
    uniform sampler2D fontTex;\n\
\n\
    void main()\n\
    {\n\
        vec2 texUV = gl_TexCoord[0].xy; \n\
        vec4 color; \n\
        float distance = (texture2D( fontTex, texUV.xy ).x); \n\
        color = gl_TexCoord[1];\n\
        color.a *= distance;\n\
        gl_FragColor = color; \n\
    }\n\
    "};

///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::changeCanvas(int w, int h)
{
    m_canvas.w = w;
    m_canvas.h = h;
    m_canvas.ratio = ((float)m_canvas.winh*w)/((float)m_canvas.winw*h);
}
///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::changeSize(int w, int h)
{
    m_canvas.winw = w;
    m_canvas.winh = h;
    m_canvas.ratio = ((float)h*m_canvas.w)/((float)w*m_canvas.h);
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
bool OpenGLText::init(unsigned char *imageData, FileHeader *glyphInfos_, int w, int h)
{
    if(glyphInfos && allocated)
        delete glyphInfos;
    glyphInfos = glyphInfos_;
    allocated = false;

    if(m_fontTex == 0)
        GL(GenTextures( 1, &m_fontTex ));
    GL(ActiveTexture( GL_TEXTURE0 ));
    GL(BindTexture( GL_TEXTURE_2D, m_fontTex ));
    GL(TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL(TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL(TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL(TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GLenum extFmt = GL_RED;
    GL(TexImage2D(GL_TEXTURE_2D, 0, extFmt, glyphInfos->texwidth, glyphInfos->texheight, 0, extFmt, GL_UNSIGNED_BYTE, imageData));
    GLenum err = GL(GetError());

    GL(BindTexture( GL_TEXTURE_2D, 0 ));
    return init(w,h);
}
///////////////////////////////////////////////////////////////////////////////////////
//
//
bool OpenGLText::init(const char * fontName, int w, int h)
{
    //
    // Load the Font if possible
    //
    if(fontName)
    {
        char fname[200];
        sprintf(fname, "%s.tga", fontName);
        TGA* fontTGA = new TGA;
        TGA::TGAError err = fontTGA->load(fname);
        if(err != TGA::TGA_NO_ERROR)
            return false;
        sprintf(fname, "%s.bin", fontName);
        FILE *fd = fopen(fname, "rb");
        if(fd)
        {
            if(glyphInfos && allocated)
                delete glyphInfos;
            glyphInfos = new FileHeader;
            allocated = true;
            int r = (int)fread(glyphInfos, 1, sizeof(FileHeader), fd);
            fclose(fd);
            // Change the shaders to use for this case

            int u = fontTGA->m_nImageWidth;
            int h = fontTGA->m_nImageHeight;

            if(m_fontTex == 0)
                GL(GenTextures( 1, &m_fontTex ));
            GL(ActiveTexture( GL_TEXTURE0 ));
            GL(BindTexture( GL_TEXTURE_2D, m_fontTex ));
            GL(TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            GL(TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            GL(TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GL(TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
            GLenum extFmt;
            switch(fontTGA->m_texFormat)
            {
            case TGA::RGB:
                extFmt = GL_RGB;
                break;
            case TGA::RGBA:
                extFmt = GL_RGBA;
                break;
            case TGA::ALPHA:
                extFmt = GL_LUMINANCE;
                break;
            }
            GL(TexImage2D(GL_TEXTURE_2D, 0, extFmt, u, h, 0, extFmt, GL_UNSIGNED_BYTE, fontTGA->m_nImageData));
            GLenum err = GL(GetError());

            GL(BindTexture( GL_TEXTURE_2D, 0 ));
        }
        delete fontTGA;
    }
    return init(w,h);
}
bool OpenGLText::init(int w, int h)
{
    m_canvas.w = w;
    m_canvas.h = h;
    m_canvas.ratio = 1.0;
    if (m_widgetProgram == 0)
    {
        //#if defined(NV_WINDOWS)
        //#define GET_PROC_ADDRESS(name)          GL(GetProcAddress(name))
        //#elif defined(NV_MACINTOSH_OSX)
        //#define GET_PROC_ADDRESS(name)          dlsym(RTLD_NEXT, name)
        //#elif defined(NV_LINUX)
        //#define GET_PROC_ADDRESS(name)          glXGetProcAddressARB((GLubyte *) name)
        //#endif
        //#define INITGL(n)\
        //    if(!gl##n)\
        //    {\
        //    gl##n = (PFN_##n)GET_PROC_ADDRESS("gl" #n);\
        //    if(!gl##n) {\
        //    PRINTF(("ERROR>> App queried " #n " - unsupported by driver\n"));\
        //            return;\
        //        }\
        //    }
        //INITGL(CreateShader);
        //INITGL(ShaderSource);
        //INITGL(CompileShader);
        //INITGL(GetShaderiv);
        //INITGL(DeleteShader);
        //INITGL(CreateProgram);
        //INITGL(AttachShader);
        //INITGL(LinkProgram);
        //INITGL(GetProgramiv);
        //INITGL(GetProgramInfoLog);
        //INITGL(UseProgram);
        //INITGL(GetUniformLocation);
        //INITGL(GetAttribLocation);
        //INITGL(Uniform4fv);
        //INITGL(Uniform4f);
        //INITGL(Uniform1i);
        //INITGL(ActiveTexture);
        //INITGL(PrimitiveRestartIndex);
        //INITGL(EnableVertexAttribArray);
        //INITGL(DisableVertexAttribArray);
        //INITGL(VertexAttribPointer);
        //INITGL(BindBuffer);

        m_vShader = CompileGLSLShader( GL_VERTEX_SHADER, cWidgetVSSource2);
        if (!m_vShader) 
            fprintf(stderr, "Vertex shader compile failed\n");
        m_fShader = CompileGLSLShader( GL_FRAGMENT_SHADER, cWidgetFSSource2);
        if (!m_fShader) 
            fprintf(stderr, "Fragment shader compile failed\n");

        //CHECKGLERRORS();

        m_widgetProgram = LinkGLSLProgram( m_vShader, m_fShader );
        //CHECKGLERRORS();

        GLint fontTexLoc = GL(GetUniformLocation(m_widgetProgram, "fontTex"));
        m_canvasVar     = GL(GetUniformLocation( m_widgetProgram, "canvas" ));
        GL(UseProgram(m_widgetProgram));
        {
            GL(Uniform1i(fontTexLoc, 0)); //Texture unit 0 is for font Tex.
        }
        GL(UseProgram(0));

        locTc  = GL(GetAttribLocation( m_widgetProgram, "TexCoord" ));
        locCol = GL(GetAttribLocation( m_widgetProgram, "Color" ));
        locPos = GL(GetAttribLocation( m_widgetProgram, "Position" ));

    }
    GL(GenBuffers(2, &m_ebo/*followed by m_vbo*/));
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::pushIndex( int vi )
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


///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::beginStrip()
{
    //if (m_indices.size() > 0)
    //{
    //    pushIndex( m_vertices.size());
    //}
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::endStrip()
{
    //pushIndex( m_vertices.size() - 1 );
    pushIndex( - 1 );
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::pushVertex( Vertex* v )
{
    pushIndex( (unsigned int)m_vertices.size() );
    m_vertices.push_back( *v );
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::beginString()
{
    m_indexOffset = 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::endString()
{
    if(!m_vertices.empty())
    {
        bs.setStates();
        GL(BindBuffer( GL_ARRAY_BUFFER, m_vbo ));
        GL(BindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_ebo ));
        // Note: we could also downsize the buffer if the size is really smaller than the one currently allocated
        // this could be done after few iterations, if we see that the size if still smaller than the allocation...
        if(m_vbosz < m_vertices.size())
        {
            GL(BufferData(GL_ARRAY_BUFFER, m_vertices.size()*sizeof(Vertex), &(m_vertices.front()), GL_STREAM_DRAW));
            m_vbosz = m_vertices.size();
        } else
            GL(BufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size()*sizeof(Vertex), &(m_vertices.front())));
        if(m_ebosz < m_indices.size())
        {
            GL(BufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size()*sizeof(unsigned int), &(m_indices.front()), GL_STREAM_DRAW));
            m_ebosz = m_indices.size();
        } else
            GL(BufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indices.size()*sizeof(unsigned int), &(m_indices.front())));
        GL(EnableVertexAttribArray( locPos ));
        GL(EnableVertexAttribArray( locTc ));
        GL(EnableVertexAttribArray( locCol ));

        static Vertex* pVtxOffset = NULL;
        GL(VertexAttribPointer( locPos, 4, GL_FLOAT, false, sizeof(Vertex), pVtxOffset->pos ));
        GL(VertexAttribPointer( locTc , 4, GL_FLOAT, false, sizeof(Vertex), pVtxOffset->tc ));
        GL(VertexAttribPointer( locCol, 4, GL_FLOAT, false, sizeof(Vertex), pVtxOffset->color ));

        //GL(ActiveTexture( GL_TEXTURE0 ));
        GL(BindTexture(GL_TEXTURE_2D, m_fontTex));

        GL(UseProgram( m_widgetProgram ));
        GL(Uniform4f( m_canvasVar, m_canvas.w, m_canvas.h, m_canvas.ratio, 0 ));

        GL(DrawElements( GL_TRIANGLE_STRIP, m_indexOffset, GL_UNSIGNED_INT, NULL));

        GL(UseProgram( 0 ));

        // switch vertex attribs back off
        GL(DisableVertexAttribArray( locPos ));
        GL(DisableVertexAttribArray( locTc ));
        GL(DisableVertexAttribArray( locCol ));
        //CHECKGLERRORS();
        m_vertices.resize(0);
        m_indices.resize(0);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::stringSize(const char *text, float *sz)
{
    int h = glyphInfos->pix.ascent + glyphInfos->pix.descent + glyphInfos->pix.linegap;
    float lPosX = 0+1;
    float lPosY = 0 ;

    float lLinePosX = lPosX;
    float lLinePosY = lPosY;
    const char* c = text;
    while (*c != '\0')
    {
        if ( *c == '\n')
        {
            lPosX = lLinePosX;
            lLinePosY -= h;
            lPosY = lLinePosY; 
        }
        else if ( *c > 128 || *c < 0 )
        {

        }
        else
        {
            GlyphInfo &g = glyphInfos->glyphs[*c];
            //int pX = lPosX + g.pix.offX;
            //int pY = lPosY - g.pix.height - g.pix.offY;
            lPosX += g.pix.advance;
            lPosY += 0;
        }
        c++;
    }
    sz[0] = lPosX;
    sz[1] = lPosY + h;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::drawString( int x, int y, const char * text, int nbLines, unsigned long color)
{
    float color4f[4];
    color4f[0] = (float)(color>>24 & 0xFF)/255.0;
    color4f[1] = (float)(color>>16 & 0xFF)/255.0;
    color4f[2] = (float)(color>>8 & 0xFF)/255.0;
    color4f[3] = (float)(color & 0xFF)/255.0;
    drawString( x, y, text, nbLines, color4f);
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::drawString( int x, int y, const char * text, int nbLines, float * color4f)
{
    int h = glyphInfos->pix.ascent + glyphInfos->pix.descent + glyphInfos->pix.linegap;
    float lPosX = x+1;
    float lPosY = y ;
    if (nbLines > 1)
        lPosY +=  h * (nbLines - 1);

    float lLinePosX = lPosX;
    float lLinePosY = lPosY;
    const char* c = text;
    Vertex v;
    v.color[0] = color4f[0];
    v.color[1] = color4f[1];
    v.color[2] = color4f[2];
    v.color[3] = color4f[3];
 
    m_vertexDepth = 1.0;
    while (*c != '\0')
    {
        if ( *c == '\n')
        {
            lPosX = lLinePosX;
            lLinePosY -= h;
            lPosY = lLinePosY; 
        }
        else if ( *c > 128 || *c < 0 )
        {

        }
        else
        {
            GlyphInfo &g = glyphInfos->glyphs[*c];
            int pX = lPosX + g.pix.offX;
            int pY = lPosY - g.pix.height - g.pix.offY;
            beginStrip();
                v.xyuv( pX ,             pY,                  g.norm.u,            g.norm.v);           pushVertex(&v);
                v.xyuv( pX ,             pY + g.pix.height,    g.norm.u,            g.norm.v+g.norm.height);  pushVertex(&v);
                v.xyuv( pX + g.pix.width, pY,                  g.norm.u+g.norm.width,    g.norm.v);           pushVertex(&v);
                v.xyuv( pX + g.pix.width, pY + g.pix.height,    g.norm.u+g.norm.width,    g.norm.v+g.norm.height);  pushVertex(&v);
            endStrip();
            lPosX += g.pix.advance;
            lPosY += 0;
        }

        c++;
    }
    m_vertexDepth = 1.f;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
void OpenGLText::BackupStates()
{
    if(!bs.valid)
        bs.backup();
}
void OpenGLText::RestoreStates()
{
    bs.restore();
}

