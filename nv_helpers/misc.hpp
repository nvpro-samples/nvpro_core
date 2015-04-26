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

#ifndef NV_MISC_INCLUDED
#define NV_MISC_INCLUDED

#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <sstream>
#include <fstream>

namespace nv_helpers
{

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

  inline std::string loadFile( std::string const & infilename)
  {
    std::string result;
    std::string filename = infilename;

    std::ifstream stream(filename.c_str(), std::ios::binary | std::ios::in);
    if(!stream.is_open()){
      nvprintfLevel(LOGLEVEL_WARNING,"file not found:%s\n",filename.c_str());
      return result;
    }

    stream.seekg(0, std::ios::end);
    result.reserve(stream.tellg());
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
      data[i] = (unsigned int)i;
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
