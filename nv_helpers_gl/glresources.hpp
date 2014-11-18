/*
 * Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

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
