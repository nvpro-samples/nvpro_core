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

#ifndef NV_GEOMETRY_INCLUDED
#define NV_GEOMETRY_INCLUDED

#include <nv_math/nv_math.h>

#include <vector>

namespace nv_helpers{
  namespace geometry{
    struct Vertex
    {
      Vertex
        (
        nv_math::vec3f const & position,
        nv_math::vec3f const & normal,
        nv_math::vec2f const & texcoord
        ) :
          position( nv_math::vec4f(position,1.0f)),
          normal  ( nv_math::vec4f(normal,0.0f)),
          texcoord( nv_math::vec4f(texcoord,0.0f,0.0f))
      {}

      nv_math::vec4f position;
      nv_math::vec4f normal;
      nv_math::vec4f texcoord;
    };

    template <class TVertex>
    class Mesh {
    public:
      std::vector<TVertex>        m_vertices;
      std::vector<nv_math::vec3ui>     m_indicesTriangles;
      std::vector<nv_math::vec2ui>     m_indicesOutline;

      void append(Mesh<TVertex>& geo)
      {
        m_vertices.reserve(geo.m_vertices.size() + m_vertices.size());
        m_indicesTriangles.reserve(geo.m_indicesTriangles.size() + m_indicesTriangles.size());
        m_indicesOutline.reserve(geo.m_indicesOutline.size() + m_indicesOutline.size());

        nv_math::uint offset = nv_math::uint(m_vertices.size());

        for (size_t i = 0; i < geo.m_vertices.size(); i++){
          m_vertices.push_back(geo.m_vertices[i]);
        }

        for (size_t i = 0; i < geo.m_indicesTriangles.size(); i++){
          m_indicesTriangles.push_back(geo.m_indicesTriangles[i] + nv_math::vec3ui(offset));
        }

        for (size_t i = 0; i < geo.m_indicesOutline.size(); i++){
          m_indicesOutline.push_back(geo.m_indicesOutline[i] +  nv_math::vec2ui(offset));
        }
      }

      void flipWinding()
      {
        for (size_t i = 0; i < m_indicesTriangles.size(); i++){
          std::swap(m_indicesTriangles[i].x,m_indicesTriangles[i].z);
        }
      }

      size_t getTriangleIndicesSize() const{
        return m_indicesTriangles.size() * sizeof(nv_math::vec3ui);
      }

      nv_math::uint getTriangleIndicesCount() const{
        return (nv_math::uint)m_indicesTriangles.size() * 3;
      }

      size_t getOutlineIndicesSize() const{
        return m_indicesOutline.size() * sizeof(nv_math::vec2ui);
      }

      nv_math::uint getOutlineIndicesCount() const{
        return (nv_math::uint)m_indicesOutline.size() * 2;
      }

      size_t getVerticesSize() const{
        return m_vertices.size() * sizeof(TVertex);
      }

      nv_math::uint getVerticesCount() const{
        return (nv_math::uint)m_vertices.size();
      }
    };

    template <class TVertex>
    class Plane : public Mesh<TVertex> {
    public:
      static void add(Mesh<TVertex>& geo, const nv_math::mat4f& mat, int w, int h)
      {
        int xdim = w;
        int ydim = h;

        float xmove = 1.0f/(float)xdim;
        float ymove = 1.0f/(float)ydim;

        int width = (xdim + 1);

        nv_math::uint vertOffset = (nv_math::uint)geo.m_vertices.size();

        int x,y;
        for (y = 0; y < ydim + 1; y++){
          for (x = 0; x < xdim + 1; x++){
            float xpos = ((float)x * xmove);
            float ypos = ((float)y * ymove);
            nv_math::vec3f pos;
            nv_math::vec2f uv;
            nv_math::vec3f normal;

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
              nv_math::vec3ui(
              (x)      + (y + 1) * width + vertOffset,
              (x)      + (y)     * width + vertOffset,
              (x + 1)  + (y + 1) * width + vertOffset
              )
              );
            // lower tris
            geo.m_indicesTriangles.push_back(
              nv_math::vec3ui(
              (x + 1)  + (y + 1) * width + vertOffset,
              (x)      + (y)     * width + vertOffset,
              (x + 1)  + (y)     * width + vertOffset
              )
              );
          }
        }

        for (y = 0; y < ydim; y++){
          geo.m_indicesOutline.push_back(
            nv_math::vec2ui(
            (y)     * width + vertOffset,
            (y + 1) * width + vertOffset
            )
            );
        }
        for (y = 0; y < ydim; y++){
          geo.m_indicesOutline.push_back(
            nv_math::vec2ui(
            (y)     * width + xdim + vertOffset,
            (y + 1) * width + xdim + vertOffset
            )
            );
        }
        for (x = 0; x < xdim; x++){
          geo.m_indicesOutline.push_back(
            nv_math::vec2ui(
            (x)     + vertOffset,
            (x + 1) + vertOffset
            )
            );
        }
        for (x = 0; x < xdim; x++){
          geo.m_indicesOutline.push_back(
            nv_math::vec2ui(
            (x)     + ydim * width + vertOffset,
            (x + 1) + ydim * width + vertOffset
            )
            );
        }
      }

      Plane (int segments = 1 ){
        add(*this,nv_math::mat4f(1),segments,segments);
      }
    };

    template <class TVertex>
    class Box : public Mesh<TVertex> {
    public:
      static void add(Mesh<TVertex>& geo, const nv_math::mat4f& mat, int w, int h, int d)
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
          nv_math::mat4f matrixRot(1);

          switch (side)
          {
          case 0:
            break;
          case 1:
            matrixRot = nv_math::rotation_mat4_y(nv_pi);
            break;
          case 2:
            matrixRot = nv_math::rotation_mat4_y(nv_pi * 0.5f);
            break;
          case 3:
            matrixRot = nv_math::rotation_mat4_y(nv_pi * 1.5f);
            break;
          case 4:
            matrixRot = nv_math::rotation_mat4_x(nv_pi * 0.5f);
            break;
          case 5:
            matrixRot = nv_math::rotation_mat4_x(nv_pi * 1.5f);
            break;
          }

          nv_math::mat4f matrixMove = nv_math::translation_mat4(0.0f,0.0f,1.0f);

          Plane<TVertex>::add(geo, mat * matrixRot * matrixMove,configs[side][0],configs[side][1]);
        }
      }

      Box (int segments = 1 ){
        add(*this,nv_math::mat4f(1),segments,segments,segments);
      }
    };

    template <class TVertex>
    class Sphere : public Mesh<TVertex> {
    public:
      static void add(Mesh<TVertex>& geo, const nv_math::mat4f& mat, int w, int h)
      {
        int xydim = w;
        int zdim  = h;

        nv_math::uint vertOffset = (nv_math::uint)geo.m_vertices.size();

        float xyshift = 1.0f / (float)xydim;
        float zshift  = 1.0f / (float)zdim;
        int width = xydim + 1;


        int index = 0;
        int xy,z;
        for (z = 0; z < zdim + 1; z++){
          for (xy = 0; xy < xydim + 1; xy++){
            nv_math::vec3f pos;
            nv_math::vec3f normal;
            nv_math::vec2f uv;
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
            nv_math::vec3ui indices;
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
          nv_math::vec2ui indices;
          indices[0] = middlez * width + xy + vertOffset;
          indices[1] = middlez * width + xy + 1 + vertOffset;
          geo.m_indicesOutline.push_back(indices);
        }

        for (int i = 0; i < 4; i++){
          int x = (xydim * i) / 4;
          for (z = 0; z < zdim; z++){
            nv_math::vec2ui indices;
            indices[0] = x + width * (z) + vertOffset;
            indices[1] = x + width * (z + 1) + vertOffset;
            geo.m_indicesOutline.push_back(indices);
          }
        }
      }

      Sphere (int w=16, int h=8 ){
        add(*this,nv_math::mat4f(1),w,h);
      }
    };
  }
}


#endif

