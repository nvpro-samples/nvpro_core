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
/*
 * This file contains code derived from glf by Christophe Riccio, www.g-truc.net
 */
 
#ifndef _ERROR_INCLUDED
#define _ERROR_INCLUDED

#include "common.hpp"

namespace nv_helpers
{
  bool checkError(const char* Title);
  bool checkGLVersion(GLint MajorVersionRequire, GLint MinorVersionRequire);
  bool checkExtension(char const * String);

}//namespace helpers

//////////////////////////////////////////////////////////////////////////

namespace nv_helpers
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
      fprintf(stdout, "OpenGL Error(%s): %s\n", ErrorString.c_str(), Title);
    }
    return Error == GL_NO_ERROR;
  }
}//namespace nv_helpers


#endif//_ERROR_INCLUDED
