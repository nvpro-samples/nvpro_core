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

#ifndef NV_GEOMETRY_INCLUDED
#define NV_GEOMETRY_INCLUDED

#include <nvmath/nvmath.h>

#include <vector>

namespace nvh{
  namespace geometry{
    struct Vertex
    {
      Vertex
        (
        nvmath::vec3f const & position,
        nvmath::vec3f const & normal,
        nvmath::vec2f const & texcoord
        ) :
          position( nvmath::vec4f(position,1.0f)),
          normal  ( nvmath::vec4f(normal,0.0f)),
          texcoord( nvmath::vec4f(texcoord,0.0f,0.0f))
      {}

      nvmath::vec4f position;
      nvmath::vec4f normal;
      nvmath::vec4f texcoord;
    };

    template <class TVertex>
    class Mesh {
    public:
      std::vector<TVertex>        m_vertices;
      std::vector<nvmath::vec3ui>     m_indicesTriangles;
      std::vector<nvmath::vec2ui>     m_indicesOutline;

      void append(Mesh<TVertex>& geo)
      {
        m_vertices.reserve(geo.m_vertices.size() + m_vertices.size());
        m_indicesTriangles.reserve(geo.m_indicesTriangles.size() + m_indicesTriangles.size());
        m_indicesOutline.reserve(geo.m_indicesOutline.size() + m_indicesOutline.size());

        nvmath::uint offset = nvmath::uint(m_vertices.size());

        for (size_t i = 0; i < geo.m_vertices.size(); i++){
          m_vertices.push_back(geo.m_vertices[i]);
        }

        for (size_t i = 0; i < geo.m_indicesTriangles.size(); i++){
          m_indicesTriangles.push_back(geo.m_indicesTriangles[i] + nvmath::vec3ui(offset));
        }

        for (size_t i = 0; i < geo.m_indicesOutline.size(); i++){
          m_indicesOutline.push_back(geo.m_indicesOutline[i] +  nvmath::vec2ui(offset));
        }
      }

      void flipWinding()
      {
        for (size_t i = 0; i < m_indicesTriangles.size(); i++){
          std::swap(m_indicesTriangles[i].x,m_indicesTriangles[i].z);
        }
      }

      size_t getTriangleIndicesSize() const{
        return m_indicesTriangles.size() * sizeof(nvmath::vec3ui);
      }

      nvmath::uint getTriangleIndicesCount() const{
        return (nvmath::uint)m_indicesTriangles.size() * 3;
      }

      size_t getOutlineIndicesSize() const{
        return m_indicesOutline.size() * sizeof(nvmath::vec2ui);
      }

      nvmath::uint getOutlineIndicesCount() const{
        return (nvmath::uint)m_indicesOutline.size() * 2;
      }

      size_t getVerticesSize() const{
        return m_vertices.size() * sizeof(TVertex);
      }

      nvmath::uint getVerticesCount() const{
        return (nvmath::uint)m_vertices.size();
      }
    };

    template <class TVertex>
    class Plane : public Mesh<TVertex> {
    public:
      static void add(Mesh<TVertex>& geo, const nvmath::mat4f& mat, int w, int h)
      {
        int xdim = w;
        int ydim = h;

        float xmove = 1.0f/(float)xdim;
        float ymove = 1.0f/(float)ydim;

        int width = (xdim + 1);

        nvmath::uint vertOffset = (nvmath::uint)geo.m_vertices.size();

        int x,y;
        for (y = 0; y < ydim + 1; y++){
          for (x = 0; x < xdim + 1; x++){
            float xpos = ((float)x * xmove);
            float ypos = ((float)y * ymove);
            nvmath::vec3f pos;
            nvmath::vec2f uv;
            nvmath::vec3f normal;

            pos[0] = (xpos - 0.5f) * 2.0f;
            pos[1] = (ypos - 0.5f) * 2.0f;
            pos[2] = 0;

            uv[0] = xpos;
            uv[1] = ypos;

            normal[0] = 0.0f;
            normal[1] = 0.0f;
            normal[2] = 1.0f;

            Vertex vert = Vertex(pos,normal,uv);
            vert.position = mat * vert.position;
            vert.normal   = mat * vert.normal;
            geo.m_vertices.push_back(TVertex(vert));
          }
        }

        for (y = 0; y < ydim; y++){
          for (x = 0; x < xdim; x++){
            // upper tris
            geo.m_indicesTriangles.push_back(
              nvmath::vec3ui(
              (x)      + (y + 1) * width + vertOffset,
              (x)      + (y)     * width + vertOffset,
              (x + 1)  + (y + 1) * width + vertOffset
              )
              );
            // lower tris
            geo.m_indicesTriangles.push_back(
              nvmath::vec3ui(
              (x + 1)  + (y + 1) * width + vertOffset,
              (x)      + (y)     * width + vertOffset,
              (x + 1)  + (y)     * width + vertOffset
              )
              );
          }
        }

        for (y = 0; y < ydim; y++){
          geo.m_indicesOutline.push_back(
            nvmath::vec2ui(
            (y)     * width + vertOffset,
            (y + 1) * width + vertOffset
            )
            );
        }
        for (y = 0; y < ydim; y++){
          geo.m_indicesOutline.push_back(
            nvmath::vec2ui(
            (y)     * width + xdim + vertOffset,
            (y + 1) * width + xdim + vertOffset
            )
            );
        }
        for (x = 0; x < xdim; x++){
          geo.m_indicesOutline.push_back(
            nvmath::vec2ui(
            (x)     + vertOffset,
            (x + 1) + vertOffset
            )
            );
        }
        for (x = 0; x < xdim; x++){
          geo.m_indicesOutline.push_back(
            nvmath::vec2ui(
            (x)     + ydim * width + vertOffset,
            (x + 1) + ydim * width + vertOffset
            )
            );
        }
      }

      Plane (int segments = 1 ){
        add(*this,nvmath::mat4f(1),segments,segments);
      }
    };

    template <class TVertex>
    class Box : public Mesh<TVertex> {
    public:
      static void add(Mesh<TVertex>& geo, const nvmath::mat4f& mat, int w, int h, int d)
      {
        int configs[6][2] = {
          {w,h},
          {w,h},

          {d,h},
          {d,h},

          {w,d},
          {w,d},
        };

        for (int side = 0; side < 6; side++){
          nvmath::mat4f matrixRot(1);

          switch (side)
          {
          case 0:
            break;
          case 1:
            matrixRot = nvmath::rotation_mat4_y(nv_pi);
            break;
          case 2:
            matrixRot = nvmath::rotation_mat4_y(nv_pi * 0.5f);
            break;
          case 3:
            matrixRot = nvmath::rotation_mat4_y(nv_pi * 1.5f);
            break;
          case 4:
            matrixRot = nvmath::rotation_mat4_x(nv_pi * 0.5f);
            break;
          case 5:
            matrixRot = nvmath::rotation_mat4_x(nv_pi * 1.5f);
            break;
          }

          nvmath::mat4f matrixMove = nvmath::translation_mat4(0.0f,0.0f,1.0f);

          Plane<TVertex>::add(geo, mat * matrixRot * matrixMove,configs[side][0],configs[side][1]);
        }
      }

      Box (int segments = 1 ){
        add(*this,nvmath::mat4f(1),segments,segments,segments);
      }
    };

    template <class TVertex>
    class Sphere : public Mesh<TVertex> {
    public:
      static void add(Mesh<TVertex>& geo, const nvmath::mat4f& mat, int w, int h)
      {
        int xydim = w;
        int zdim  = h;

        nvmath::uint vertOffset = (nvmath::uint)geo.m_vertices.size();

        float xyshift = 1.0f / (float)xydim;
        float zshift  = 1.0f / (float)zdim;
        int width = xydim + 1;


        int index = 0;
        int xy,z;
        for (z = 0; z < zdim + 1; z++){
          for (xy = 0; xy < xydim + 1; xy++){
            nvmath::vec3f pos;
            nvmath::vec3f normal;
            nvmath::vec2f uv;
            float curxy = xyshift * (float)xy;
            float curz  = zshift  * (float)z;
            float anglexy = curxy * nv_pi * 2.0f;
            float anglez  = (1.0f-curz) * nv_pi;
            pos[0] = cosf(anglexy) * sinf(anglez);
            pos[1] = sinf(anglexy) * sinf(anglez);
            pos[2] = cosf(anglez);
            normal = pos;
            uv[0]  = curxy;
            uv[1]  = curz;

            Vertex vert = Vertex(pos,normal,uv);
            vert.position = mat * vert.position;
            vert.normal   = mat * vert.normal;

            geo.m_vertices.push_back(TVertex(vert));
          }
        }

        int vertex = 0;
        for (z = 0; z < zdim; z++){
          for (xy = 0; xy < xydim; xy++, vertex++){
            nvmath::vec3ui indices;
            if (z != zdim-1){
              indices[2] = vertex + vertOffset;
              indices[1] = vertex + width + vertOffset;
              indices[0] = vertex + width + 1 + vertOffset;
              geo.m_indicesTriangles.push_back(indices);
            }

            if (z != 0){
              indices[2] = vertex + width + 1 + vertOffset;
              indices[1] = vertex + 1 + vertOffset;
              indices[0] = vertex + vertOffset;
              geo.m_indicesTriangles.push_back(indices);
            }

          }
          vertex++;
        }

        int middlez = zdim / 2;

        for (xy = 0; xy < xydim; xy++){
          nvmath::vec2ui indices;
          indices[0] = middlez * width + xy + vertOffset;
          indices[1] = middlez * width + xy + 1 + vertOffset;
          geo.m_indicesOutline.push_back(indices);
        }

        for (int i = 0; i < 4; i++){
          int x = (xydim * i) / 4;
          for (z = 0; z < zdim; z++){
            nvmath::vec2ui indices;
            indices[0] = x + width * (z) + vertOffset;
            indices[1] = x + width * (z + 1) + vertOffset;
            geo.m_indicesOutline.push_back(indices);
          }
        }
      }

      Sphere (int w=16, int h=8 ){
        add(*this,nvmath::mat4f(1),w,h);
      }
    };

    template <class TVertex>
    class RandomMengerSponge : public Mesh<TVertex> {
    public:
      static void add(Mesh<TVertex>& geo, const nvmath::mat4f& mat, int w, int h, int d, int level=3, float probability=-1.f)
      {
        struct Cube {
          nvmath::vec3f m_topLeftFront;
          float m_size;

          void split(std::vector<Cube> &cubes) {
            float size = m_size / 3.f;
            nvmath::vec3f topLeftFront = m_topLeftFront;
            for (int x = 0; x < 3; x++) {
              topLeftFront[0] = m_topLeftFront[0] + static_cast<float>(x) * size;
              for (int y = 0; y < 3; y++) {
                if (x == 1 && y == 1)
                  continue;
                topLeftFront[1] = m_topLeftFront[1] + static_cast<float>(y) * size;
                for (int z = 0; z < 3; z++) {
                  if (x == 1 && z == 1)
                    continue;
                  if (y == 1 && z == 1)
                    continue;

                  topLeftFront[2] =
                    m_topLeftFront[2] + static_cast<float>(z) * size;
                  cubes.push_back({ topLeftFront, size });
                }
              }
            }
          }

          void splitProb(std::vector<Cube> &cubes, float prob) {

            float size = m_size / 3.f;
            nvmath::vec3f topLeftFront = m_topLeftFront;
            for (int x = 0; x < 3; x++) {
              topLeftFront[0] = m_topLeftFront[0] + static_cast<float>(x) * size;
              for (int y = 0; y < 3; y++) {
                topLeftFront[1] = m_topLeftFront[1] + static_cast<float>(y) * size;
                for (int z = 0; z < 3; z++) {
                  float sample = rand() / static_cast<float>(RAND_MAX);
                  if (sample > prob)
                    continue;
                  topLeftFront[2] =
                    m_topLeftFront[2] + static_cast<float>(z) * size;
                  cubes.push_back({ topLeftFront, size });
                }
              }
            }
          }
        };

        Cube cube = { nvmath::vec3f(-0.25, -0.25, -0.25), 0.5f };
        //Cube cube = { nvmath::vec3f(-25, -25, -25), 50.f };
        //Cube cube = { nvmath::vec3f(-40, -40, -40), 10.f };

        std::vector<Cube> cubes1 = { cube };
        std::vector<Cube> cubes2 = {};

        auto previous = &cubes1;
        auto next = &cubes2;

        for (int i = 0; i < level; i++) {
          size_t cubeCount = previous->size();
          for (Cube &c : *previous) {
            if (probability < 0.f)
              c.split(*next);
            else
              c.splitProb(*next, probability);
          }
          auto temp = previous;
          previous = next;
          next = temp;
          next->clear();
        }
        for (Cube &c : *previous) {
          nvmath::mat4f matrixMove = nvmath::translation_mat4(c.m_topLeftFront);
          nvmath::mat4f matrixScale = nvmath::scale_mat4(nvmath::vec3f(c.m_size));;
          Box<TVertex>::add(geo, matrixMove*matrixScale, 1, 1, 1);
        }

      }
    };

  }
}


#endif

