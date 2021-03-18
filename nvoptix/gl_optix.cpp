/*-----------------------------------------------------------------------
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/ //--------------------------------------------------------------------

#include <cstdint>
#include <string>

#include "gl_optix.h"
#include <optixu/optixpp_namespace.h>

namespace nvgl {


enum bufferPixelFormat
{
  BUFFER_PIXEL_FORMAT_DEFAULT,  // The default depending on the buffer type
  BUFFER_PIXEL_FORMAT_RGB,      // The buffer is RGB or RGBA
  BUFFER_PIXEL_FORMAT_BGR,      // The buffer is BGR or BGRA
};

// Converts the buffer format to gl format
static GLenum glFormatFromBufferFormat(bufferPixelFormat pixel_format, RTformat buffer_format)
{
  if(buffer_format == RT_FORMAT_UNSIGNED_BYTE4)
  {
    switch(pixel_format)
    {
      case BUFFER_PIXEL_FORMAT_DEFAULT:
        return GL_BGRA;
      case BUFFER_PIXEL_FORMAT_RGB:
        return GL_RGBA;
      case BUFFER_PIXEL_FORMAT_BGR:
        return GL_BGRA;
      default:
        throw optix::Exception("Unknown buffer pixel format");
    }
  }
  else if(buffer_format == RT_FORMAT_FLOAT4)
  {
    switch(pixel_format)
    {
      case BUFFER_PIXEL_FORMAT_DEFAULT:
        return GL_RGBA;
      case BUFFER_PIXEL_FORMAT_RGB:
        return GL_RGBA;
      case BUFFER_PIXEL_FORMAT_BGR:
        return GL_BGRA;
      default:
        throw optix::Exception("Unknown buffer pixel format");
    }
  }
  else if(buffer_format == RT_FORMAT_FLOAT3)
    switch(pixel_format)
    {
      case BUFFER_PIXEL_FORMAT_DEFAULT:
        return GL_RGB;
      case BUFFER_PIXEL_FORMAT_RGB:
        return GL_RGB;
      case BUFFER_PIXEL_FORMAT_BGR:
        return GL_BGR;
      default:
        throw optix::Exception("Unknown buffer pixel format");
    }
  else if(buffer_format == RT_FORMAT_FLOAT)
    return GL_R8;
  else
    throw optix::Exception("Unknown buffer format");
}

//--------------------------------------------------------------------------------------------------
// Display the current incoming buffer in full screen
//
void OptixGL::displayBuffer(optix::Buffer image_buffer)
{
  // Query buffer information
  RTsize buffer_width_rts, buffer_height_rts;
  image_buffer->getSize(buffer_width_rts, buffer_height_rts);
  uint32_t width         = static_cast<int>(buffer_width_rts);
  uint32_t height        = static_cast<int>(buffer_height_rts);
  RTformat buffer_format = image_buffer->getFormat();

  GLboolean use_SRGB = GL_FALSE;
  //   if (!g_disable_srgb_conversion &&
  //       (buffer_format == RT_FORMAT_FLOAT4 || buffer_format == RT_FORMAT_FLOAT3))
  //   {
  //     glGetBooleanv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &use_SRGB);
  //     if (use_SRGB)
  //       glEnable(GL_FRAMEBUFFER_SRGB_EXT);
  //   }

  if(!m_bufferTexId)
  {
    glGenTextures(1, &m_bufferTexId);
    glBindTexture(GL_TEXTURE_2D, m_bufferTexId);

    // Change these to GL_LINEAR for super- or sub-sampling
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // GL_CLAMP_TO_EDGE for linear filtering, not relevant for nearest.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  glBindTexture(GL_TEXTURE_2D, m_bufferTexId);

  // send PBO or host-mapped image data to texture
  const unsigned pboId     = image_buffer->getGLBOId();
  GLvoid*        imageData = nullptr;
  if(pboId)
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboId);
  else
    imageData = image_buffer->map(0, RT_BUFFER_MAP_READ);

  RTsize elmt_size = image_buffer->getElementSize();
  if(elmt_size % 8 == 0)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
  else if(elmt_size % 4 == 0)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  else if(elmt_size % 2 == 0)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
  else
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  bufferPixelFormat g_image_buffer_format = BUFFER_PIXEL_FORMAT_DEFAULT;
  GLenum            pixel_format          = glFormatFromBufferFormat(g_image_buffer_format, buffer_format);

  if(buffer_format == RT_FORMAT_UNSIGNED_BYTE4)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, pixel_format, GL_UNSIGNED_BYTE, imageData);
  else if(buffer_format == RT_FORMAT_FLOAT4)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, pixel_format, GL_FLOAT, imageData);
  else if(buffer_format == RT_FORMAT_FLOAT3)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, pixel_format, GL_FLOAT, imageData);
  else if(buffer_format == RT_FORMAT_FLOAT)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, pixel_format, GL_FLOAT, imageData);
  else
    throw optix::Exception("Unknown buffer format");

  if(pboId)
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  else
    image_buffer->unmap();

  if(m_texQuadProgram == 0)
  {
    std::string vertex_shader2 = {R"(
            #version 450
            layout (location = 0) out vec2 outUV;

            void main()
            {
              outUV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
              gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
            }
          )"};

    std::string fragment_shader2 = {R"(
            #version 450
            layout (location = 0) in vec2 outUV;
            layout(binding = 0) uniform sampler2D texSampler;
            layout(location = 0) out vec4 outColor;

            void main()
            {
              outColor = texture(texSampler, outUV);
            }
          )"};

    GLuint      vs = glCreateShader(GL_VERTEX_SHADER);
    const char* s  = vertex_shader2.c_str();
    glShaderSource(vs, 1, &s, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    s         = fragment_shader2.c_str();
    glShaderSource(fs, 1, &s, nullptr);
    glCompileShader(fs);

    m_texQuadProgram = glCreateProgram();
    glAttachShader(m_texQuadProgram, fs);
    glAttachShader(m_texQuadProgram, vs);
    glLinkProgram(m_texQuadProgram);
  }

  // Drawing the quad
  {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_bufferTexId);
    glUseProgram(m_texQuadProgram);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  if(use_SRGB)
    glDisable(GL_FRAMEBUFFER_SRGB);
}

}  // namespace nvgl
