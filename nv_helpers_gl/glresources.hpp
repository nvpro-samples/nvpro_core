/*-----------------------------------------------------------------------
  Copyright (c) 2014, NVIDIA. All rights reserved.

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
-----------------------------------------------------------------------*/

#ifndef NV_GLRESOURCES_INCLUDED
#define NV_GLRESOURCES_INCLUDED

#include <GL/glew.h>

#include <cstdio>
#include <main.h>


#define NV_BUFFER_OFFSET(i) ((char *)NULL + (i))

namespace nv_helpers_gl
{
  inline size_t uboAligned(size_t size){
    return ((size+255)/256)*256;
  }

  class ResourceGLuint {
  public:
    GLuint  m_value;

    ResourceGLuint() : m_value(0) {}

    ResourceGLuint( GLuint b) : m_value(b) {}
    operator GLuint() const { return m_value; }
    operator GLuint&() { return m_value; }
    ResourceGLuint& operator=( GLuint b) { m_value = b; return *this; }
  };

  class ResourceGLuint64 {
  public:
    GLuint64  m_value;

    ResourceGLuint64() : m_value(0) {}

    ResourceGLuint64( GLuint64 b) : m_value(b) {}
    operator GLuint64() const { return m_value; }
    operator GLuint64&() { return m_value; }
    ResourceGLuint64& operator=( GLuint64 b) { m_value = b; return *this; }
  };

  inline void newBuffer(GLuint &glid)
  {
    if (glid) glDeleteBuffers(1,&glid);
    glGenBuffers(1,&glid);
  }

  inline void deleteBuffer(GLuint &glid)
  {
    if (glid) glDeleteBuffers(1,&glid);
    glid = 0;
  }

  inline void newTexture(GLuint &glid)
  {
    if (glid) glDeleteTextures(1,&glid);
    glGenTextures(1,&glid);
  }

  inline void deleteTexture( GLuint &glid )
  {
    if (glid) glDeleteTextures(1,&glid);
    glid = 0;
  }

  inline void newFramebuffer(GLuint &glid)
  {
    if (glid) glDeleteFramebuffers(1,&glid);
    glGenFramebuffers(1,&glid);
  }

  inline void deleteFramebuffer(GLuint &glid)
  {
    if (glid) glDeleteFramebuffers(1,&glid);
    glid = 0;
  }

  inline void newSampler(GLuint &glid)
  {
    if (glid) glDeleteSamplers(1,&glid);
    glGenSamplers(1,&glid);
  }

  inline void deleteSampler(GLuint &glid)
  {
    if (glid) glDeleteSamplers(1,&glid);
    glid = 0;
  }

  inline void newQuery(GLuint &glid)
  {
    if (glid) glDeleteQueries(1,&glid);
    glGenQueries(1,&glid);
  }

  inline void deleteQuery(GLuint &glid)
  {
    if (glid) glDeleteQueries(1,&glid);
    glid = 0;
  }
  
  inline bool checkNamedFramebuffer(GLuint fbo)
  {
    GLenum status = glCheckNamedFramebufferStatusEXT(fbo,GL_FRAMEBUFFER);
    switch(status)
    {
    case GL_FRAMEBUFFER_UNDEFINED:
      nvprintfLevel(LOGLEVEL_ERROR,"OpenGL Error(%s)\n", "GL_FRAMEBUFFER_UNDEFINED");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      nvprintfLevel(LOGLEVEL_ERROR,"OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      nvprintfLevel(LOGLEVEL_ERROR,"OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      nvprintfLevel(LOGLEVEL_ERROR,"OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      nvprintfLevel(LOGLEVEL_ERROR,"OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      nvprintfLevel(LOGLEVEL_ERROR,"OpenGL Error(%s)\n", "GL_FRAMEBUFFER_UNSUPPORTED");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
      nvprintfLevel(LOGLEVEL_ERROR,"OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
      nvprintfLevel(LOGLEVEL_ERROR,"OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
      break;
    }

    return status != GL_FRAMEBUFFER_COMPLETE;
  }
}

#endif
