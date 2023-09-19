/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#include <array>

#define _USE_MATH_DEFINES
#include <math.h>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include "primitives.hpp"
#include "container_utils.hpp"


namespace nvh {
static uint32_t addPos(PrimitiveMesh& mesh, nvmath::vec3f p)
{
  PrimitiveVertex v{};
  v.p = p;
  mesh.vertices.emplace_back(v);
  return static_cast<uint32_t>(mesh.vertices.size()) - 1;
}

static void addTriangle(PrimitiveMesh& mesh, uint32_t a, uint32_t b, uint32_t c)
{
  mesh.triangles.push_back({{a, b, c}});
}

static void addTriangle(PrimitiveMesh& mesh, nvmath::vec3f a, nvmath::vec3f b, nvmath::vec3f c)
{
  mesh.triangles.push_back({{addPos(mesh, a), addPos(mesh, b), addPos(mesh, c)}});
}

static void generateFacetedNormals(PrimitiveMesh& mesh)
{
  auto num_indices = static_cast<int>(mesh.triangles.size());
  for(int i = 0; i < num_indices; i++)
  {
    auto& v0 = mesh.vertices[mesh.triangles[i].v[0]];
    auto& v1 = mesh.vertices[mesh.triangles[i].v[1]];
    auto& v2 = mesh.vertices[mesh.triangles[i].v[2]];

    nvmath::vec3f n = nvmath::normalize(nvmath::cross(nvmath::normalize(v1.p - v0.p), nvmath::normalize(v2.p - v0.p)));

    v0.n = n;
    v1.n = n;
    v2.n = n;
  }
}

// Function to generate texture coordinates
static void generateTexCoords(PrimitiveMesh& mesh)
{
  for(auto& vertex : mesh.vertices)
  {
    nvmath::vec3f n = normalize(vertex.p);
    float         u = 0.5f + std::atan2(n.z, n.x) / (2.0F * float(M_PI));
    float         v = 0.5f - std::asin(n.y) / float(M_PI);
    vertex.t        = {u, v};
  }
}

// Generates a tetrahedron mesh (four triangular faces)
PrimitiveMesh createTetrahedron()
{
  PrimitiveMesh mesh;

  // choose coordinates on the unit sphere
  float a = 1.0F / 3.0F;
  float b = sqrt(8.0F / 9.0F);
  float c = sqrt(2.0F / 9.0F);
  float d = sqrt(2.0F / 3.0F);

  // 4 vertices
  nvmath::vec3f v0 = nvmath::vec3f{0.0F, 1.0F, 0.0F} * 0.5F;
  nvmath::vec3f v1 = nvmath::vec3f{-c, -a, d} * 0.5F;
  nvmath::vec3f v2 = nvmath::vec3f{-c, -a, -d} * 0.5F;
  nvmath::vec3f v3 = nvmath::vec3f{b, -a, 0.0F} * 0.5F;

  // 4 triangles
  addTriangle(mesh, v0, v2, v1);
  addTriangle(mesh, v0, v3, v2);
  addTriangle(mesh, v0, v1, v3);
  addTriangle(mesh, v3, v1, v2);

  generateFacetedNormals(mesh);
  generateTexCoords(mesh);

  return mesh;
}

// Generates an icosahedron mesh (twenty equilateral triangular faces)
PrimitiveMesh createIcosahedron()
{
  PrimitiveMesh mesh;

  float sq5 = sqrt(5.0F);
  float a   = 2.0F / (1.0F + sq5);
  float b   = sqrt((3.0F + sq5) / (1.0F + sq5));
  a /= b;
  float r = 0.5F;

  std::vector<nvmath::vec3f> v;
  v.emplace_back(0.0F, r * a, r / b);
  v.emplace_back(0.0F, r * a, -r / b);
  v.emplace_back(0.0F, -r * a, r / b);
  v.emplace_back(0.0F, -r * a, -r / b);
  v.emplace_back(r * a, r / b, 0.0F);
  v.emplace_back(r * a, -r / b, 0.0F);
  v.emplace_back(-r * a, r / b, 0.0F);
  v.emplace_back(-r * a, -r / b, 0.0F);
  v.emplace_back(r / b, 0.0F, r * a);
  v.emplace_back(r / b, 0.0F, -r * a);
  v.emplace_back(-r / b, 0.0F, r * a);
  v.emplace_back(-r / b, 0.0F, -r * a);

  addTriangle(mesh, v[1], v[6], v[4]);
  addTriangle(mesh, v[0], v[4], v[6]);
  addTriangle(mesh, v[0], v[10], v[2]);
  addTriangle(mesh, v[0], v[2], v[8]);
  addTriangle(mesh, v[1], v[9], v[3]);
  addTriangle(mesh, v[1], v[3], v[11]);
  addTriangle(mesh, v[2], v[7], v[5]);
  addTriangle(mesh, v[3], v[5], v[7]);
  addTriangle(mesh, v[6], v[11], v[10]);
  addTriangle(mesh, v[7], v[10], v[11]);
  addTriangle(mesh, v[4], v[8], v[9]);
  addTriangle(mesh, v[5], v[9], v[8]);
  addTriangle(mesh, v[0], v[6], v[10]);
  addTriangle(mesh, v[0], v[8], v[4]);
  addTriangle(mesh, v[1], v[11], v[6]);
  addTriangle(mesh, v[1], v[4], v[9]);
  addTriangle(mesh, v[3], v[7], v[11]);
  addTriangle(mesh, v[3], v[9], v[5]);
  addTriangle(mesh, v[2], v[10], v[7]);
  addTriangle(mesh, v[2], v[5], v[8]);

  generateFacetedNormals(mesh);
  generateTexCoords(mesh);

  return mesh;
}

// Generates an octahedron mesh (eight faces), this is like two four-sided pyramids placed base to base.
PrimitiveMesh createOctahedron()
{
  PrimitiveMesh mesh;

  std::vector<nvmath::vec3f> v;
  v.emplace_back(0.5F, 0.0F, 0.0F);
  v.emplace_back(-0.5F, 0.0F, 0.0F);
  v.emplace_back(0.0F, 0.5F, 0.0F);
  v.emplace_back(0.0F, -0.5F, 0.0F);
  v.emplace_back(0.0F, 0.0F, 0.5F);
  v.emplace_back(0.0F, 0.0F, -0.5F);

  addTriangle(mesh, v[0], v[2], v[4]);
  addTriangle(mesh, v[0], v[4], v[3]);
  addTriangle(mesh, v[0], v[5], v[2]);
  addTriangle(mesh, v[0], v[3], v[5]);
  addTriangle(mesh, v[1], v[4], v[2]);
  addTriangle(mesh, v[1], v[3], v[4]);
  addTriangle(mesh, v[1], v[5], v[3]);
  addTriangle(mesh, v[2], v[5], v[1]);

  generateFacetedNormals(mesh);
  generateTexCoords(mesh);

  return mesh;
}

// Generates a flat plane mesh with the specified number of steps, width, and depth.
// The plane is essentially a grid with the specified number of subdivisions (steps)
// in both the X and Z directions. It creates vertices, normals, and texture coordinates
// for each point on the grid and forms triangles to create the plane's surface.
PrimitiveMesh createPlane(int steps, float width, float depth)
{
  PrimitiveMesh mesh;

  float increment = 1.0F / static_cast<float>(steps);
  for(int sz = 0; sz <= steps; sz++)
  {
    for(int sx = 0; sx <= steps; sx++)
    {
      PrimitiveVertex v{};

      v.p = nvmath::vec3f(-0.5F + (static_cast<float>(sx) * increment), 0.0F, -0.5F + (static_cast<float>(sz) * increment));
      v.p *= nvmath::vec3f(width, 1.0F, depth);
      v.n = nvmath::vec3f(0.0F, 1.0F, 0.0F);
      v.t = nvmath::vec2f(static_cast<float>(sx) / static_cast<float>(steps),
                          static_cast<float>(steps - sz) / static_cast<float>(steps));
      mesh.vertices.emplace_back(v);
    }
  }

  for(int sz = 0; sz < steps; sz++)
  {
    for(int sx = 0; sx < steps; sx++)
    {
      addTriangle(mesh, sx + sz * (steps + 1), sx + 1 + (sz + 1) * (steps + 1), sx + 1 + sz * (steps + 1));
      addTriangle(mesh, sx + sz * (steps + 1), sx + (sz + 1) * (steps + 1), sx + 1 + (sz + 1) * (steps + 1));
    }
  }

  return mesh;
}

// Generates a cube mesh with the specified width, height, and depth
// Start with 8 vertex, 6 normal and 4 uv, then 12 triangles and 24
// unique PrimitiveVertex
PrimitiveMesh createCube(float width /*= 1*/, float height /*= 1*/, float depth /*= 1*/)
{
  PrimitiveMesh mesh;

  nvmath::vec3f              s   = nvmath::vec3f(width, height, depth) * 0.5F;
  std::vector<nvmath::vec3f> pnt = {{-s.x, -s.y, -s.z}, {-s.x, -s.y, s.z}, {-s.x, s.y, -s.z}, {-s.x, s.y, s.z},
                                    {s.x, -s.y, -s.z},  {s.x, -s.y, s.z},  {s.x, s.y, -s.z},  {s.x, s.y, s.z}};
  std::vector<nvmath::vec3f> nrm = {{-1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F},  {1.0F, 0.0F, 0.0F},
                                    {0.0F, 0.0F, -1.0F}, {0.0F, -1.0F, 0.0F}, {0.0F, 1.0F, 0.0F}};
  std::vector<nvmath::vec2f> uv  = {{0.0F, 0.0F}, {0.0F, 1.0F}, {1.0F, 1.0F}, {1.0F, 0.0F}};

  // cube topology
  std::vector<std::vector<int>> cube_polygons = {{0, 1, 3, 2}, {1, 5, 7, 3}, {5, 4, 6, 7},
                                                 {4, 0, 2, 6}, {4, 5, 1, 0}, {2, 3, 7, 6}};

  for(int i = 0; i < 6; ++i)
  {
    auto index = static_cast<int>(mesh.vertices.size());
    for(int j = 0; j < 4; ++j)
      mesh.vertices.push_back({pnt[cube_polygons[i][j]], nrm[i], uv[j]});
    addTriangle(mesh, index, index + 1, index + 2);
    addTriangle(mesh, index, index + 2, index + 3);
  }

  return mesh;
}

// Generates a UV-sphere mesh with the specified radius, number of sectors (horizontal subdivisions)
// and stacks (vertical subdivisions). It uses latitude-longitude grid generation to create vertices
// with proper positions, normals, and texture coordinates.
PrimitiveMesh createSphereUv(float radius, int sectors, int stacks)
{
  PrimitiveMesh mesh;

  float omega{0.0F};                 // rotation around the X axis
  float phi{0.0F};                   // rotation around the Y axis
  float length_inv = 1.0F / radius;  // vertex normal

  const float math_pi     = static_cast<float>(M_PI);
  float       sector_step = 2.0F * math_pi / static_cast<float>(sectors);
  float       stack_step  = math_pi / static_cast<float>(stacks);
  float       sector_angle{0.0F};
  float       stack_angle{0.0F};

  for(int i = 0; i <= stacks; ++i)
  {
    stack_angle = math_pi / 2.0F - static_cast<float>(i) * stack_step;  // starting from pi/2 to -pi/2
    phi         = radius * cosf(stack_angle);                           // r * cos(u)
    omega       = radius * sinf(stack_angle);                           // r * sin(u)

    // add (sectorCount+1) vertices per stack
    // the first and last vertices have same position and normal, but different tex coords
    for(int j = 0; j <= sectors; ++j)
    {
      PrimitiveVertex v{};

      sector_angle = static_cast<float>(j) * sector_step;  // starting from 0 to 2pi

      // vertex position (x, y, z)
      v.p.x = phi * cosf(sector_angle);  // r * cos(u) * cos(v)
      v.p.z = phi * sinf(sector_angle);  // r * cos(u) * sin(v)
      v.p.y = omega;

      // normalized vertex normal
      v.n = v.p * length_inv;

      // vertex tex coord (s, t) range between [0, 1]
      v.t.x = 1.0F - static_cast<float>(j) / static_cast<float>(sectors);
      v.t.y = static_cast<float>(i) / static_cast<float>(stacks);

      mesh.vertices.emplace_back(v);
    }
  }

  // indices
  //  k2---k2+1
  //  | \  |
  //  |  \ |
  //  k1---k1+1
  int k1{0};
  int k2{0};
  for(int i = 0; i < stacks; ++i)
  {
    k1 = i * (sectors + 1);  // beginning of current stack
    k2 = k1 + sectors + 1;   // beginning of next stack

    for(int j = 0; j < sectors; ++j, ++k1, ++k2)
    {
      // 2 triangles per sector excluding 1st and last stacks
      if(i != 0)
      {
        addTriangle(mesh, k1, k1 + 1, k2);  // k1---k2---k1+1
      }

      if(i != (stacks - 1))
      {
        addTriangle(mesh, k1 + 1, k2 + 1, k2);  // k1+1---k2---k2+1
      }
    }
  }

  return mesh;
}

// Function to create a cone
// radius   :Adjust this to change the size of the cone
// height   :Adjust this to change the height of the cone
// segments :Adjust this for the number of segments forming the base circle
PrimitiveMesh createConeMesh(float radius, float height, int segments)
{
  PrimitiveMesh mesh;

  float halfHeight = height * 0.5f;

  const float math_pi     = static_cast<float>(M_PI);
  float       sector_step = 2.0F * math_pi / static_cast<float>(segments);
  float       sector_angle{0.0F};

  // length of the flank of the cone
  float flank_len = sqrtf(radius * radius + 1.0F);
  // unit vector along the flank of the cone
  float cone_x = radius / flank_len;
  float cone_y = -1.0F / flank_len;

  nvmath::vec3f tip = {0.0F, halfHeight, 0.0F};

  // Sides
  for(int i = 0; i <= segments; ++i)
  {
    PrimitiveVertex v{};
    sector_angle = static_cast<float>(i) * sector_step;

    // Position
    v.p.x = radius * cosf(sector_angle);  // r * cos(u) * cos(v)
    v.p.z = radius * sinf(sector_angle);  // r * cos(u) * sin(v)
    v.p.y = -halfHeight;
    // Normal
    v.n.x = -cone_y * cosf(sector_angle);
    v.n.y = cone_x;
    v.n.z = -cone_y * sinf(sector_angle);
    // TexCoord
    v.t.x = static_cast<float>(i) / static_cast<float>(segments);
    v.t.y = 0.0F;
    mesh.vertices.emplace_back(v);

    // Tip point
    v.p = tip;
    // Normal
    sector_angle += 0.5F * sector_step;  // Half way to next triangle
    v.n.x = -cone_y * cosf(sector_angle);
    v.n.y = cone_x;
    v.n.z = -cone_y * sinf(sector_angle);
    // TexCoord
    v.t.x += 0.5F / static_cast<float>(segments);
    v.t.y = 1.0F;

    mesh.vertices.emplace_back(v);
  }

  for(int j = 0; j < segments; ++j)
  {
    int k1 = j * 2;
    addTriangle(mesh, k1, k1 + 1, k1 + 2);
  }

  // Bottom plate (normal are different)
  for(int i = 0; i <= segments; ++i)
  {
    PrimitiveVertex v{};
    sector_angle = static_cast<float>(i) * sector_step;  // starting from 0 to 2pi

    v.p.x = radius * cosf(sector_angle);  // r * cos(u) * cos(v)
    v.p.z = radius * sinf(sector_angle);  // r * cos(u) * sin(v)
    v.p.y = -halfHeight;
    //
    v.n = {0.0F, -1.0F, 0.0F};
    //
    v.t.x = static_cast<float>(i) / static_cast<float>(segments);
    v.t.y = 0.0F;
    mesh.vertices.emplace_back(v);

    v.p = -tip;
    v.t.x += 0.5F / static_cast<float>(segments);
    v.t.y = 1.0F;
    mesh.vertices.emplace_back(v);
  }

  for(int j = 0; j < segments; ++j)
  {
    int k1 = (j + segments + 1) * 2;
    addTriangle(mesh, k1, k1 + 2, k1 + 1);
  }


  return mesh;
}

// Generates a sphere mesh with the specified radius and subdivisions (level of detail).
// It uses the icosahedron subdivision technique to iteratively refine the mesh by
// subdividing triangles into smaller triangles to approximate a more spherical shape.
// It calculates vertex positions, normals, and texture coordinates for each vertex
// and constructs triangles accordingly.
// Note: There will be duplicated vertices with this method.
//       Use removeDuplicateVertices to avoid duplicated vertices.
PrimitiveMesh createSphereMesh(float radius, int subdivisions)
{

  const float                t        = (1.0F + std::sqrt(5.0F)) / 2.0F;  // Golden ratio
  std::vector<nvmath::vec3f> vertices = {{-1, t, 0},  {1, t, 0},  {-1, -t, 0}, {1, -t, 0}, {0, -1, t},  {0, 1, t},
                                         {0, -1, -t}, {0, 1, -t}, {t, 0, -1},  {t, 0, 1},  {-t, 0, -1}, {-t, 0, 1}};

  // Function to calculate the midpoint between two vertices
  auto midpoint = [](const nvmath::vec3f& v1, const nvmath::vec3f& v2) { return (v1 + v2) * 0.5f; };

  auto texCoord = [](const nvmath::vec3f& v1) {
    return nvmath::vec2f{0.5f + std::atan2(v1.z, v1.x) / (2 * M_PI), 0.5f - std::asin(v1.y) / M_PI};
  };

  std::vector<PrimitiveVertex> primitiveVertices;
  for(const auto& vertex : vertices)
  {
    nvmath::vec3f n = normalize(vertex);
    primitiveVertices.push_back({n * radius, n, texCoord(n)});
  }

  std::vector<PrimitiveTriangle> triangles = {{{0, 11, 5}}, {{0, 5, 1}},  {{0, 1, 7}},   {{0, 7, 10}}, {{0, 10, 11}},
                                              {{1, 5, 9}},  {{5, 11, 4}}, {{11, 10, 2}}, {{10, 7, 6}}, {{7, 1, 8}},
                                              {{3, 9, 4}},  {{3, 4, 2}},  {{3, 2, 6}},   {{3, 6, 8}},  {{3, 8, 9}},
                                              {{4, 9, 5}},  {{2, 4, 11}}, {{6, 2, 10}},  {{8, 6, 7}},  {{9, 8, 1}}};


  for(int i = 0; i < subdivisions; ++i)
  {
    std::vector<PrimitiveTriangle> subTriangles;
    for(const auto& tri : triangles)
    {
      // Subdivide each triangle into 4 sub-triangles
      nvmath::vec3f mid1 = midpoint(primitiveVertices[tri.v[0]].p, primitiveVertices[tri.v[1]].p);
      nvmath::vec3f mid2 = midpoint(primitiveVertices[tri.v[1]].p, primitiveVertices[tri.v[2]].p);
      nvmath::vec3f mid3 = midpoint(primitiveVertices[tri.v[2]].p, primitiveVertices[tri.v[0]].p);

      nvmath::vec3f mid1Normalized = normalize(mid1);
      nvmath::vec3f mid2Normalized = normalize(mid2);
      nvmath::vec3f mid3Normalized = normalize(mid3);

      nvmath::vec2f mid1Uv = texCoord(mid1Normalized);
      nvmath::vec2f mid2Uv = texCoord(mid2Normalized);
      nvmath::vec2f mid3Uv = texCoord(mid3Normalized);

      primitiveVertices.push_back({mid1Normalized * radius, mid1Normalized, mid1Uv});
      primitiveVertices.push_back({mid2Normalized * radius, mid2Normalized, mid2Uv});
      primitiveVertices.push_back({mid3Normalized * radius, mid3Normalized, mid3Uv});

      uint32_t m1 = static_cast<uint32_t>(primitiveVertices.size()) - 3U;
      uint32_t m2 = m1 + 1U;
      uint32_t m3 = m2 + 1U;

      // Create 4 new triangles from the subdivided triangle
      subTriangles.push_back({{tri.v[0], m1, m3}});
      subTriangles.push_back({{m1, tri.v[1], m2}});
      subTriangles.push_back({{m2, tri.v[2], m3}});
      subTriangles.push_back({{m1, m2, m3}});
    }

    triangles = subTriangles;
  }

  return {primitiveVertices, triangles};
}


// Generates a torus mesh, which is a 3D geometric shape resembling a donut
// majorRadius: This represents the distance from the center of the torus to the center of the tube (the larger circle's radius).
// minorRadius: This represents the radius of the tube (the smaller circle's radius).
// majorSegments: The number of segments used to approximate the larger circle that forms the torus.
// minorSegments: The number of segments used to approximate the smaller circle (tube) within the torus.
nvh::PrimitiveMesh createTorusMesh(float majorRadius, float minorRadius, int majorSegments, int minorSegments)
{
  nvh::PrimitiveMesh mesh;

  float majorStep = 2.0f * float(M_PI) / float(majorSegments);
  float minorStep = 2.0f * float(M_PI) / float(minorSegments);

  for(int i = 0; i <= majorSegments; ++i)
  {
    float         angle1 = i * majorStep;
    nvmath::vec3f center = {majorRadius * std::cos(angle1), 0.0f, majorRadius * std::sin(angle1)};

    for(int j = 0; j <= minorSegments; ++j)
    {
      float angle2 = j * minorStep;
      nvmath::vec3f position = {center.x + minorRadius * std::cos(angle2) * std::cos(angle1), minorRadius * std::sin(angle2),
                                center.z + minorRadius * std::cos(angle2) * std::sin(angle1)};

      nvmath::vec3f normal = {std::cos(angle2) * std::cos(angle1), std::sin(angle2), std::cos(angle2) * std::sin(angle1)};

      nvmath::vec2f texCoord = {static_cast<float>(i) / majorSegments, static_cast<float>(j) / minorSegments};
      mesh.vertices.push_back({position, normal, texCoord});
    }
  }

  for(int i = 0; i < majorSegments; ++i)
  {
    for(int j = 0; j < minorSegments; ++j)
    {
      uint32_t idx1 = i * (minorSegments + 1) + j;
      uint32_t idx2 = (i + 1) * (minorSegments + 1) + j;
      uint32_t idx3 = idx1 + 1;
      uint32_t idx4 = idx2 + 1;

      mesh.triangles.push_back({{idx1, idx3, idx2}});
      mesh.triangles.push_back({{idx3, idx4, idx2}});
    }
  }

  return mesh;
}

//------------------------------------------------------------------------
// Create a vector of nodes that represent the Menger Sponge
// Nodes have a different translation and scale, which can be used with
// different objects.
std::vector<nvh::Node> mengerSpongeNodes(int level, float probability, int seed)
{
  srand(seed);

  struct MengerSponge
  {
    nvmath::vec3f m_topLeftFront;
    float         m_size;

    void split(std::vector<MengerSponge>& cubes)
    {
      float         size         = m_size / 3.f;
      nvmath::vec3f topLeftFront = m_topLeftFront;
      for(int x = 0; x < 3; x++)
      {
        topLeftFront[0] = m_topLeftFront[0] + static_cast<float>(x) * size;
        for(int y = 0; y < 3; y++)
        {
          if(x == 1 && y == 1)
            continue;
          topLeftFront[1] = m_topLeftFront[1] + static_cast<float>(y) * size;
          for(int z = 0; z < 3; z++)
          {
            if(x == 1 && z == 1)
              continue;
            if(y == 1 && z == 1)
              continue;

            topLeftFront[2] = m_topLeftFront[2] + static_cast<float>(z) * size;
            cubes.push_back({topLeftFront, size});
          }
        }
      }
    }

    void splitProb(std::vector<MengerSponge>& cubes, float prob)
    {
      float         size         = m_size / 3.f;
      nvmath::vec3f topLeftFront = m_topLeftFront;
      for(int x = 0; x < 3; x++)
      {
        topLeftFront[0] = m_topLeftFront[0] + static_cast<float>(x) * size;
        for(int y = 0; y < 3; y++)
        {
          topLeftFront[1] = m_topLeftFront[1] + static_cast<float>(y) * size;
          for(int z = 0; z < 3; z++)
          {
            float sample = rand() / static_cast<float>(RAND_MAX);
            if(sample > prob)
              continue;
            topLeftFront[2] = m_topLeftFront[2] + static_cast<float>(z) * size;
            cubes.push_back({topLeftFront, size});
          }
        }
      }
    }
  };

  // Starting element
  MengerSponge element = {nvmath::vec3f(-0.5, -0.5, -0.5), 1.f};

  std::vector<MengerSponge> elements1 = {element};
  std::vector<MengerSponge> elements2 = {};

  auto previous = &elements1;
  auto next     = &elements2;

  for(int i = 0; i < level; i++)
  {
    for(MengerSponge& c : *previous)
    {
      if(probability < 0.f)
        c.split(*next);
      else
        c.splitProb(*next, probability);
    }
    auto temp = previous;
    previous  = next;
    next      = temp;
    next->clear();
  }

  std::vector<nvh::Node> nodes;
  for(MengerSponge& c : *previous)
  {
    nvh::Node node{};
    node.translation = c.m_topLeftFront;
    node.scale       = c.m_size;
    node.mesh        = 0;  // default to the first mesh
    nodes.push_back(node);
  }

  return nodes;
}


//---------------------------------------------------------------------------
// Merge all nodes meshes into a single one
// - nodes: the nodes to merge
// - meshes: the mesh array that the nodes is referring to
nvh::PrimitiveMesh mergeNodes(const std::vector<nvh::Node>& nodes, const std::vector<nvh::PrimitiveMesh> meshes)
{
  nvh::PrimitiveMesh resultMesh;

  // Find how many triangles and vertices the merged mesh will have
  size_t nb_triangles = 0;
  size_t nb_vertices  = 0;
  for(const auto& n : nodes)
  {
    nb_triangles += meshes[n.mesh].triangles.size();
    nb_vertices += meshes[n.mesh].vertices.size();
  }
  resultMesh.triangles.reserve(nb_triangles);
  resultMesh.vertices.reserve(nb_vertices);

  // Merge all nodes meshes into a single one
  for(const auto& n : nodes)
  {
    const nvmath::mat4f mat = n.localMatrix();

    uint32_t                  tIndex = resultMesh.vertices.size();
    const nvh::PrimitiveMesh& mesh   = meshes[n.mesh];

    for(auto v : mesh.vertices)
    {
      v.p = nvmath::vec3f(mat * nvmath::vec4f(v.p, 1));
      resultMesh.vertices.push_back(v);
    }
    for(auto t : mesh.triangles)
    {
      t.v += tIndex;
      resultMesh.triangles.push_back(t);
    }
  }

  return resultMesh;
}


// Takes a 3D mesh as input and modifies its vertices by adding random displacements within a
// specified `amplitude` range to create a wobbling effect. The intensity of the wobbling effect
// can be controlled by adjusting the `amplitude` parameter.
// The function returns the modified mesh.
nvh::PrimitiveMesh wobblePrimitive(const nvh::PrimitiveMesh& mesh, float amplitude)
{
  // Seed the random number generator with a random device
  std::random_device rd;
  std::mt19937       gen(rd());

  // Define the range for the random number generation (-1.0 to 1.0)
  std::uniform_real_distribution<float> distribution(-1.0, 1.0);

  // Our random function
  auto rand = [&] { return distribution(gen); };

  std::vector<PrimitiveVertex> newVertices;
  for(auto& vertex : mesh.vertices)
  {
    nvmath::vec3f originalPosition = vertex.p;
    nvmath::vec3f displacement     = nvmath::vec3f(rand(), rand(), rand());
    displacement *= amplitude;
    nvmath::vec3f newPosition = originalPosition + displacement;

    newVertices.push_back({newPosition, vertex.n, vertex.t});
  }

  return {newVertices, mesh.triangles};
}

// Takes a 3D mesh as input and returns a new mesh with duplicate vertices removed.
// This function iterates through each triangle in the original PrimitiveMesh,
// compares its vertices, and creates a new set of unique vertices in uniqueVertices.
// We use an unordered_map called vertexIndexMap to keep track of the mapping between
// the original vertices and their corresponding indices in the uniqueVertices vector.
PrimitiveMesh removeDuplicateVertices(const PrimitiveMesh& mesh, bool testNormal, bool testUv)
{
  auto hash = [&](const PrimitiveVertex& v) {
    if(testNormal)
    {
      if(testUv)
        return nvh::hashVal(v.p.x, v.p.y, v.p.z, v.n.x, v.n.y, v.n.z, v.t.x, v.t.y);
      else
        return nvh::hashVal(v.p.x, v.p.y, v.p.z, v.n.x, v.n.y, v.n.z);
    }
    else if(testUv)
      return nvh::hashVal(v.p.x, v.p.y, v.p.z, v.t.x, v.t.y);
    return nvh::hashVal(v.p.x, v.p.y, v.p.z);
  };
  auto equal = [&](const PrimitiveVertex& l, const PrimitiveVertex& r) {
    return (l.p == r.p) && (testNormal ? l.n == r.n : true) && (testUv ? l.t == r.t : true);
  };
  std::unordered_map<PrimitiveVertex, uint32_t, decltype(hash), decltype(equal)> vertexIndexMap(0, hash, equal);

  std::vector<PrimitiveVertex>   uniqueVertices;
  std::vector<PrimitiveTriangle> uniqueTriangles;

  for(const auto& triangle : mesh.triangles)
  {
    PrimitiveTriangle uniqueTriangle = {};
    for(int i = 0; i < 3; i++)
    {
      const PrimitiveVertex& vertex = mesh.vertices[triangle.v[i]];

      // Check if the vertex is already in the uniqueVertices list
      auto it = vertexIndexMap.find(vertex);
      if(it == vertexIndexMap.end())
      {
        // Vertex not found, add it to uniqueVertices and update the index map
        uint32_t newIndex      = static_cast<uint32_t>(uniqueVertices.size());
        vertexIndexMap[vertex] = newIndex;
        uniqueVertices.push_back(vertex);
        uniqueTriangle.v[i] = newIndex;
      }
      else
      {
        // Vertex found, use its index in uniqueVertices
        uniqueTriangle.v[i] = it->second;
      }
    }
    uniqueTriangles.push_back(uniqueTriangle);
  }

  // nvprintf("Before: %d vertex, %d triangles\n", mesh.vertices.size(), mesh.triangles.size());
  // nvprintf("After: %d vertex, %d triangles\n", uniqueVertices.size(), uniqueTriangles.size());

  return {uniqueVertices, uniqueTriangles};
}


}  // namespace nvh
