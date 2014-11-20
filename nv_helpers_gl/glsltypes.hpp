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

#ifndef NV_GLSLTYPES_INCLUDED
#define NV_GLSLTYPES_INCLUDED

#include <GL/glew.h>

#include <nv_math/nv_math_glsltypes.h>


namespace nv_helpers_gl
{
  typedef GLuint64 sampler1D;
  typedef GLuint64 sampler2D;
  typedef GLuint64 sampler2DMS;
  typedef GLuint64 sampler3D;
  typedef GLuint64 samplerBuffer;
  typedef GLuint64 samplerCube;
  typedef GLuint64 sampler1DArray;
  typedef GLuint64 sampler2DArray;
  typedef GLuint64 sampler2DMSArray;
  typedef GLuint64 samplerCubeArray;

  typedef GLuint64 usampler1D;
  typedef GLuint64 usampler2D;
  typedef GLuint64 usampler2DMS;
  typedef GLuint64 usampler3D;
  typedef GLuint64 usamplerBuffer;
  typedef GLuint64 usamplerCube;
  typedef GLuint64 usampler1DArray;
  typedef GLuint64 usampler2DArray;
  typedef GLuint64 usampler2DMSArray;
  typedef GLuint64 usamplerCubeArray;
  
  typedef GLuint64 isampler1D;
  typedef GLuint64 isampler2D;
  typedef GLuint64 isampler2DMS;
  typedef GLuint64 isampler3D;
  typedef GLuint64 isamplerBuffer;
  typedef GLuint64 isamplerCube;
  typedef GLuint64 isampler1DArray;
  typedef GLuint64 isampler2DArray;
  typedef GLuint64 isampler2DMSArray;
  typedef GLuint64 isamplerCubeArray;
  
  typedef GLuint64 image1D;
  typedef GLuint64 image2D;
  typedef GLuint64 image2DMS;
  typedef GLuint64 image3D;
  typedef GLuint64 imageBuffer;
  typedef GLuint64 imageCube;
  typedef GLuint64 image1DArray;
  typedef GLuint64 image2DArray;
  typedef GLuint64 image2DMSArray;
  typedef GLuint64 imageCubeArray;

  typedef GLuint64 uimage1D;
  typedef GLuint64 uimage2D;
  typedef GLuint64 uimage2DMS;
  typedef GLuint64 uimage3D;
  typedef GLuint64 uimageBuffer;
  typedef GLuint64 uimageCube;
  typedef GLuint64 uimage1DArray;
  typedef GLuint64 uimage2DArray;
  typedef GLuint64 uimage2DMSArray;
  typedef GLuint64 uimageCubeArray;
  
  typedef GLuint64 iimage1D;
  typedef GLuint64 iimage2D;
  typedef GLuint64 iimage2DMS;
  typedef GLuint64 iimage3D;
  typedef GLuint64 iimageBuffer;
  typedef GLuint64 iimageCube;
  typedef GLuint64 iimage1DArray;
  typedef GLuint64 iimage2DArray;
  typedef GLuint64 iimage2DMSArray;
  typedef GLuint64 iimageCubeArray;
}

#endif
