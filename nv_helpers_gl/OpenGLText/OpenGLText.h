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
#ifndef OpenGLText_H__
#define OpenGLText_H__

#include <vector>
#include <string.h>

class OpenGLText
{
public:
    //
    // Header and structure for what is in the .bin file of the font
    //
    struct GlyphInfo
    {
       struct Pix // pixel oriented data
       {
           int u, v;
           int width, height;
           int advance;
           int offX, offY;
       };
       struct Norm // normalized data
       {
           float u, v; // position in the map in normalized coords
           float width, height;
           float advance;
           float offX, offY;
       };
       Pix  pix;
       Norm norm;
    };
    struct FileHeader
    {
       int texwidth, texheight;
       struct Pix
       {
           int ascent;
           int descent;
           int linegap;
       };
       struct Norm
       {
           float ascent;
           float descent;
           float linegap;
       };
       Pix  pix;
       Norm norm;
       GlyphInfo glyphs[256];
    };

    OpenGLText();
    ~OpenGLText();
    static void BackupStates();
    static void RestoreStates();
    void beginString();
    void endString();
    void stringSize(const char *text, float *sz);
    void drawString( int x, int y, const char * text, int nbLines, unsigned long color);
    void drawString( int x, int y, const char * text, int nbLines, float * color4f);
    bool init(const char * fontName, int w, int h);
    bool init(unsigned char *imageData, FileHeader *glyphInfos, int w, int h);
    void changeCanvas(int w, int h);
    void changeSize(int w, int h);
private:
    bool init(int w, int h);
    static char*         cWidgetVSSource2;
    static char*         cWidgetFSSource2;
    unsigned int        c_fontNbChars;
    unsigned int        c_fontHeight;
    unsigned int        c_fontWidth;
    unsigned int        m_widgetProgram;
    unsigned int        locPos;
    unsigned int        locTc;
    unsigned int        locCol;
    unsigned int        m_vShader;
    unsigned int        m_fShader;
    unsigned int        m_canvasVar;
    unsigned int        m_depthNFRSVar;
    unsigned int        m_fontTex;
    float               m_vertexDepth;
    int                 m_indexOffset;
    unsigned int        m_ebo, m_vbo;
    unsigned int        m_ebosz, m_vbosz;
    struct TCanvas
    {
        float w,h;
        float winw,winh;
        float ratio;
    };
    TCanvas             m_canvas;

    struct Vertex
    {
        float pos[4];
        float tc[4];
        float color[4];

        Vertex()
        { memset(this, 0, sizeof(Vertex)); }

        void xy( float fx, float fy )
        {
            pos[0] = fx; pos[1] = fy;
        }

        void xyuv( float fx, float fy, float fu, float fv )
        {
            pos[0] = fx; pos[1] = fy; tc[0] = fu; tc[1] = fv;
        }
    };

    std::vector< unsigned int > m_indices;
    std::vector< Vertex >       m_vertices;

    FileHeader *glyphInfos;
    bool        allocated;

    unsigned int CompileGLSLShader( GLenum target, const char* shader);
    unsigned int LinkGLSLProgram( GLuint vertexShader, GLuint fragmentShader);
    void pushIndex( int vi );
    void beginStrip();
    void endStrip();
    void pushVertex( Vertex* v );
};

#endif
