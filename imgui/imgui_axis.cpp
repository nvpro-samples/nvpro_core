/*
 * Copyright (c) 2022-2023, NVIDIA CORPORATION.  All rights reserved.
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

#include <vector>
#include <array>

#define _USE_MATH_DEFINES
#include <math.h>

#include "imgui_axis.hpp"
#include "nvmath/nvmath.h"

//////////////////////////////////////////////////////////////////////////
// This IMGUI widget draw axis in red, green and blue at the position
// defined.
//

struct AxisGeom
{
  AxisGeom()
  {
    const float asize   = 1.0f;   // length of arrow
    const float aradius = 0.11f;  // width of arrow tip
    const float abase   = 0.66f;  // 1/3 of arrow length
    const int   asubdiv = 8;

    // Cone
    red.push_back({asize, 0, 0});  // 0 Tip
    for(int i = 0; i <= asubdiv; ++i)
    {
      float a0 = 2.0F * float(M_PI) * (float(i)) / asubdiv;  // Counter-clockwise
      float y0 = cosf(a0) * aradius;
      float z0 = sinf(a0) * aradius;
      red.push_back({abase, y0, z0});
    }
    for(int i = 0; i <= asubdiv - 1; ++i)  // Triangle fan
    {
      indices.push_back(0);
      indices.push_back(i + 1);
      indices.push_back(i + 2);
    }

    // Under Cap
    int center = static_cast<int>(red.size());
    red.push_back({abase, 0, 0});  // Center of cap
    for(int i = 0; i <= asubdiv; ++i)
    {
      float a0 = -2.0F * float(M_PI) * (float(i)) / asubdiv;  // Clockwise
      float y0 = cosf(a0) * aradius;
      float z0 = sinf(a0) * aradius;
      red.push_back({abase, y0, z0});
    }
    for(int i = 0; i <= asubdiv - 1; ++i)
    {
      indices.push_back(center);
      indices.push_back(center + i + 1);
      indices.push_back(center + i + 2);
    }

    // Start of arrow
    red.push_back({0, 0, 0});

    // Other arrows are permutations of the Red arrow
    for(const auto& v : red)
    {
      green.push_back({v.z, v.x, v.y});
      blue.push_back({v.y, v.z, v.x});
    }
  }

  std::vector<nvmath::vec3f> red;
  std::vector<nvmath::vec3f> green;
  std::vector<nvmath::vec3f> blue;
  std::vector<int>           indices;

  // Return the transformed arrow
  std::vector<nvmath::vec3f> transform(const std::vector<nvmath::vec3f>& in_vec, const ImVec2& pos, const nvmath::mat4f& modelView, float size)
  {
    std::vector<nvmath::vec3f> temp(in_vec.size());

    for(size_t i = 0; i < in_vec.size(); ++i)
    {
      temp[i] = nvmath::vec3f(modelView * nvmath::vec4f(in_vec[i], 0.F));  // Rotate
      temp[i].x *= size;                                                   // Scale
      temp[i].y *= -size;                                                  // - invert Y
      temp[i].x += pos.x;                                                  // Translate
      temp[i].y += pos.y;
    }
    return temp;
  }

  void drawTriangle(ImVec2 v0, ImVec2 v1, ImVec2 v2, const ImVec2& uv, ImU32 col)
  {
    auto draw_list = ImGui::GetWindowDrawList();

    ImVec2 d0 = ImVec2(v1.x - v0.x, v1.y - v0.y);
    ImVec2 d1 = ImVec2(v2.x - v0.x, v2.y - v0.y);
    float  c  = (d0.x * d1.y) - (d0.y * d1.x);  // Cross

    if(c > 0.0f)  // Culling to avoid z-fighting
    {
      v1 = v0;  // Culled triangles are degenerated to
      v2 = v0;  // avoid displaying them
    }

    draw_list->PrimVtx(v0, uv, col);
    draw_list->PrimVtx(v1, uv, col);
    draw_list->PrimVtx(v2, uv, col);
  }

  // Draw the arrow
  void draw(const std::vector<nvmath::vec3f>& vertex, ImU32 col)
  {
    auto         draw_list = ImGui::GetWindowDrawList();
    const ImVec2 uv        = ImGui::GetFontTexUvWhitePixel();

    int num_indices = static_cast<int>(indices.size());
    draw_list->PrimReserve(num_indices, num_indices);  // num vert/indices

    // Draw all triangles
    for(int i = 0; i < num_indices; i += 3)
    {
      int    i0 = indices[i];
      int    i1 = indices[i + 1];
      int    i2 = indices[i + 2];
      ImVec2 v0 = {vertex[i0].x, vertex[i0].y};
      ImVec2 v1 = {vertex[i1].x, vertex[i1].y};
      ImVec2 v2 = {vertex[i2].x, vertex[i2].y};
      drawTriangle(v0, v1, v2, uv, col);
    }

    // Draw the line
    draw_list->AddLine(ImVec2(vertex[0].x, vertex[0].y), ImVec2(vertex.back().x, vertex.back().y), col,
                       1.0F * ImGui::GetWindowDpiScale());
  }
};


void ImGuiH::Axis(ImVec2 pos, const nvmath::mat4f& modelView, float size /*= 20.f*/)
{
  static AxisGeom a;

  struct Arrow
  {
    std::vector<nvmath::vec3f> v;
    ImU32                      c{0};
  };

  size *= ImGui::GetWindowDpiScale();

  std::array<Arrow, 3> arrow;
  arrow[0].v = a.transform(a.red, pos, modelView, size);
  arrow[0].c = IM_COL32(200, 0, 0, 255);
  arrow[1].v = a.transform(a.green, pos, modelView, size);
  arrow[1].c = IM_COL32(0, 200, 0, 255);
  arrow[2].v = a.transform(a.blue, pos, modelView, size);
  arrow[2].c = IM_COL32(0, 0, 200, 255);

  // Sort from smallest Z to nearest (Painter algorithm)
  if(arrow[1].v[0].z < arrow[0].v[0].z)
    std::swap(arrow[0], arrow[1]);
  if(arrow[2].v[0].z < arrow[1].v[0].z)
  {
    std::swap(arrow[1], arrow[2]);
    if(arrow[1].v[0].z < arrow[0].v[0].z)
      std::swap(arrow[1], arrow[0]);
  }

  // Draw all axis
  a.draw(arrow[0].v, arrow[0].c);
  a.draw(arrow[1].v, arrow[1].c);
  a.draw(arrow[2].v, arrow[2].c);
}
