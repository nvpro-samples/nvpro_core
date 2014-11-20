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
/*
 * This file contains code derived from glf by Christophe Riccio, www.g-truc.net
 */
 
#ifndef NV_ERROR_INCLUDED
#define NV_ERROR_INCLUDED

#include <GL/glew.h>

namespace nv_helpers_gl
{
  bool checkError(const char* Title);
  bool checkGLVersion(GLint MajorVersionRequire, GLint MinorVersionRequire);
  bool checkExtension(char const * String);

}//namespace nvglf

//////////////////////////////////////////////////////////////////////////

namespace nv_helpers_gl
{

  inline bool checkGLVersion(GLint MajorVersionRequire, GLint MinorVersionRequire)
  {
    GLint MajorVersionContext = 0;
    GLint MinorVersionContext = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &MajorVersionContext);
    glGetIntegerv(GL_MINOR_VERSION, &MinorVersionContext);
    return (MajorVersionContext * 100 +  MinorVersionContext * 10) 
      >= (MajorVersionRequire* 100 + MinorVersionRequire * 10);
  }

  inline bool checkExtension(char const * String)
  {
    GLint ExtensionCount = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &ExtensionCount);
    for(GLint i = 0; i < ExtensionCount; ++i)
      if(std::string((char const*)glGetStringi(GL_EXTENSIONS, i)) == std::string(String))
        return true;
    return false;
  }

  inline bool checkError(const char* Title)
  {
    int Error;
    if((Error = glGetError()) != GL_NO_ERROR)
    {
      std::string ErrorString;
      switch(Error)
      {
      case GL_INVALID_ENUM:
        ErrorString = "GL_INVALID_ENUM";
        break;
      case GL_INVALID_VALUE:
        ErrorString = "GL_INVALID_VALUE";
        break;
      case GL_INVALID_OPERATION:
        ErrorString = "GL_INVALID_OPERATION";
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        ErrorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
        break;
      case GL_OUT_OF_MEMORY:
        ErrorString = "GL_OUT_OF_MEMORY";
        break;
      default:
        ErrorString = "UNKNOWN";
        break;
      }
      nvprintfLevel(LOGLEVEL_ERROR,"OpenGL Error(%s): %s\n", ErrorString.c_str(), Title);
    }
    return Error == GL_NO_ERROR;
  }
}//namespace nvglf


#endif//NV_ERROR_INCLUDED
