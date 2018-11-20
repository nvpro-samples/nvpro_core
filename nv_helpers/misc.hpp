/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
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
 */

#ifndef NV_MISC_INCLUDED
#define NV_MISC_INCLUDED

#include <assert.h>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <math.h>
#include <string>
#include <vector>
#include <algorithm>

#include <nv_helpers/nvprint.hpp>

namespace nv_helpers
{

  inline std::string stringFormat(const char* msg, ...)
  {
    std::size_t const STRING_BUFFER(8192);
    char text[STRING_BUFFER];
    va_list list;

    if (msg == 0)
      return std::string();

    va_start(list, msg);
    vsprintf(text, msg, list);
    va_end(list);

    return std::string(text);
  }

  inline bool fileExists(const char* filename)
  {
    std::ifstream stream;
    stream.open(filename);
    return stream.is_open();
  }

  inline std::string findFile( std::string const & infilename, std::vector<std::string> directories)
  {
    std::ifstream stream;

    for (size_t i = 0; i < directories.size(); i++ ){
      std::string filename = directories[i] + "/" + infilename;
      stream.open(filename.c_str());
      if (stream.is_open()) return filename;
    }
    return infilename;
  }

  inline std::string loadFile( std::string const & infilename, bool warn=false)
  {
    std::string result;
    std::string filename = infilename;

    std::ifstream stream(filename.c_str(), std::ios::binary | std::ios::in);
    if(!stream.is_open()){
      if (warn) {
        nvprintfLevel(LOGLEVEL_WARNING, "file not found:%s\n", filename.c_str());
      }
      return result;
    }

    stream.seekg(0, std::ios::end);
    result.reserve(size_t(stream.tellg()));
    stream.seekg(0, std::ios::beg);

    result.assign(
      (std::istreambuf_iterator<char>(stream)),
      std::istreambuf_iterator<char>());

    return result;
  }

  inline std::string getFileName( std::string const & fullPath)
  {
    size_t istart;
    for( istart = fullPath.size() - 1;  istart != -1
      && fullPath[istart] != '\\'
      && fullPath[istart] != '/'; istart-- );
    return std::string( &fullPath[istart+1] );
  }

  inline std::string getFilePath(const char* filename)
  {
    std::string path;
    // find path in filename
    {
      std::string filepath(filename);

      size_t pos0 = filepath.rfind('\\');
      size_t pos1 = filepath.rfind('/');

      pos0 = pos0 == std::string::npos ? 0 : pos0;
      pos1 = pos1 == std::string::npos ? 0 : pos1;

      path = filepath.substr(0, std::max(pos0, pos1));
    }

    if (path.empty()) {
      path = ".";
    }

    return path;
  }

  inline void saveBMP( const char* bmpfilename, int width, int height, const unsigned char* bgra)
  {
#pragma pack(push, 1)
    struct {
      unsigned short  bfType;
      unsigned int    bfSize;
      unsigned int    bfReserved;
      unsigned int    bfOffBits;

      unsigned int    biSize;
      signed   int    biWidth;
      signed   int    biHeight;
      unsigned short  biPlanes;
      unsigned short  biBitCount;
      unsigned int    biCompression;
      unsigned int    biSizeImage;
      signed   int    biXPelsPerMeter;
      signed   int    biYPelsPerMeter;
      unsigned int    biClrUsed;
      unsigned int    biClrImportant;
    } bmpinfo;
#pragma pack(pop)

    bmpinfo.bfType = 19778;
    bmpinfo.bfSize = sizeof(bmpinfo) + width * height * 4 * sizeof(unsigned char);
    bmpinfo.bfReserved = 0;
    bmpinfo.bfOffBits = 54;

    bmpinfo.biSize = 40;
    bmpinfo.biWidth = width;
    bmpinfo.biHeight = height;
    bmpinfo.biPlanes = 1;
    bmpinfo.biBitCount = 32;
    bmpinfo.biCompression = 0;
    bmpinfo.biSizeImage = 0;
    bmpinfo.biXPelsPerMeter = 0;
    bmpinfo.biYPelsPerMeter = 0;
    bmpinfo.biClrUsed = 0;
    bmpinfo.biClrImportant = 0;

    FILE* bmpfile = fopen(bmpfilename, "wb");
    fwrite(&bmpinfo, sizeof(bmpinfo), 1, bmpfile);
    fwrite(bgra, sizeof(char), width * height * 4 * sizeof(unsigned char), bmpfile);
    fclose(bmpfile);
  }

  inline float frand(){
    return float( rand() % RAND_MAX ) / float(RAND_MAX);
  }

  inline int mipMapLevels(int size) {
    int num = 0;
    while (size){
      num++;
      size /= 2;
    }
    return num;
  }

  inline void permutation(std::vector<unsigned int> &data)
  {
    size_t size = data.size();
    assert( size < RAND_MAX );

    for (size_t i = 0; i < size; i++){
      data[i] = (unsigned int)(i);
    }

    for (size_t i = size-1; i > 0 ; i--){
      size_t other = rand() % (i+1);
      std::swap(data[i],data[other]);
    }
  }

  class Frustum 
  {
  public:
    enum {
      PLANE_NEAR,
      PLANE_FAR,
      PLANE_LEFT,
      PLANE_RIGHT,
      PLANE_TOP,
      PLANE_BOTTOM,
      NUM_PLANES
    };

    static inline void init(float planes[Frustum::NUM_PLANES][4], const float viewProj[16])
    {
      const float *clip = viewProj;

      planes[PLANE_RIGHT][0] = clip[3] - clip[0];
      planes[PLANE_RIGHT][1] = clip[7] - clip[4];
      planes[PLANE_RIGHT][2] = clip[11] - clip[8];
      planes[PLANE_RIGHT][3] = clip[15] - clip[12];

      planes[PLANE_LEFT][0] = clip[3] + clip[0];
      planes[PLANE_LEFT][1] = clip[7] + clip[4];
      planes[PLANE_LEFT][2] = clip[11] + clip[8];
      planes[PLANE_LEFT][3] = clip[15] + clip[12];

      planes[PLANE_BOTTOM][0] = clip[3] + clip[1];
      planes[PLANE_BOTTOM][1] = clip[7] + clip[5];
      planes[PLANE_BOTTOM][2] = clip[11] + clip[9];
      planes[PLANE_BOTTOM][3] = clip[15] + clip[13];

      planes[PLANE_TOP][0] = clip[3] - clip[1];
      planes[PLANE_TOP][1] = clip[7] - clip[5];
      planes[PLANE_TOP][2] = clip[11] - clip[9];
      planes[PLANE_TOP][3] = clip[15] - clip[13];

      planes[PLANE_FAR][0] = clip[3] - clip[2];
      planes[PLANE_FAR][1] = clip[7] - clip[6];
      planes[PLANE_FAR][2] = clip[11] - clip[10];
      planes[PLANE_FAR][3] = clip[15] - clip[14];

      planes[PLANE_NEAR][0] = clip[3] + clip[2];
      planes[PLANE_NEAR][1] = clip[7] + clip[6];
      planes[PLANE_NEAR][2] = clip[11] + clip[10];
      planes[PLANE_NEAR][3] = clip[15] + clip[14];

      for (int i = 0; i < NUM_PLANES; i++){
        float length    = sqrtf(planes[i][0]*planes[i][0] + planes[i][1]*planes[i][1] + planes[i][2]*planes[i][2]);
        float magnitude = 1.0f/length;

        for (int n = 0; n < 4; n++){
          planes[i][n] *= magnitude;
        }
      }
    }

    Frustum() {}
    Frustum(const float viewProj[16]){
      init(m_planes,viewProj);
    }

    float  m_planes[NUM_PLANES][4];
  };
  
}

#endif
