/*
 * Copyright (c) 2023, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */


// This file is included in the shaders to provide access to the vertex data


#ifndef VERTEX_ACCESSOR_H
#define VERTEX_ACCESSOR_H

#include "nvvkhl/shaders/dh_scn_desc.h"


// clang-format off
layout(buffer_reference, scalar) readonly buffer RenderNodeBuf      { RenderNode _[]; };
layout(buffer_reference, scalar) readonly buffer RenderPrimitiveBuf { RenderPrimitive _[]; };
layout(buffer_reference, scalar) readonly buffer TriangleIndices    { uvec3 _[]; };
layout(buffer_reference, scalar) readonly buffer VertexPosition     { vec3 _[]; };
layout(buffer_reference, scalar) readonly buffer VertexNormal       { vec3 _[]; };
layout(buffer_reference, scalar) readonly buffer VertexTexCoord0    { vec2 _[]; };
layout(buffer_reference, scalar) readonly buffer VertexTangent      { vec4 _[]; };
layout(buffer_reference, scalar) readonly buffer VertexColor        { uint _[]; };
// clang-format on


uvec3 getTriangleIndices(RenderPrimitive renderPrim, uint idx)
{
  return TriangleIndices(renderPrim.indexAddress)._[idx];
}

vec3 getVertexPosition(RenderPrimitive renderPrim, uint idx)
{
  return VertexPosition(renderPrim.vertexBuffer.positionAddress)._[idx];
}

vec3 getInterpolatedVertexPosition(RenderPrimitive renderPrim, uvec3 idx, vec3 barycentrics)
{
  VertexPosition positions = VertexPosition(renderPrim.vertexBuffer.positionAddress);
  vec3           pos[3];
  pos[0] = positions._[idx.x];
  pos[1] = positions._[idx.y];
  pos[2] = positions._[idx.z];
  return pos[0] * barycentrics.x + pos[1] * barycentrics.y + pos[2] * barycentrics.z;
}

bool hasVertexNormal(RenderPrimitive renderPrim)
{
  return renderPrim.vertexBuffer.normalAddress != 0;
}

vec3 getVertexNormal(RenderPrimitive renderPrim, uint idx)
{
  if(!hasVertexNormal(renderPrim))
    return vec3(0, 0, 1);
  return VertexNormal(renderPrim.vertexBuffer.normalAddress)._[idx];
}

vec3 getInterpolatedVertexNormal(RenderPrimitive renderPrim, uvec3 idx, vec3 barycentrics)
{
  if(!hasVertexNormal(renderPrim))
    return vec3(0, 0, 1);
  VertexNormal normals = VertexNormal(renderPrim.vertexBuffer.normalAddress);
  vec3         nrm[3];
  nrm[0] = normals._[idx.x];
  nrm[1] = normals._[idx.y];
  nrm[2] = normals._[idx.z];
  return nrm[0] * barycentrics.x + nrm[1] * barycentrics.y + nrm[2] * barycentrics.z;
}

bool hasVertexTexCoord0(RenderPrimitive renderPrim)
{
  return renderPrim.vertexBuffer.texCoord0Address != 0;
}

vec2 getVertexTexCoord0(RenderPrimitive renderPrim, uint idx)
{
  if(!hasVertexTexCoord0(renderPrim))
    return vec2(0, 0);
  return VertexTexCoord0(renderPrim.vertexBuffer.texCoord0Address)._[idx];
}

vec2 getInterpolatedVertexTexCoord0(RenderPrimitive renderPrim, uvec3 idx, vec3 barycentrics)
{
  if(!hasVertexTexCoord0(renderPrim))
    return vec2(0, 0);
  VertexTexCoord0 texcoords = VertexTexCoord0(renderPrim.vertexBuffer.texCoord0Address);
  vec2            uv[3];
  uv[0] = texcoords._[idx.x];
  uv[1] = texcoords._[idx.y];
  uv[2] = texcoords._[idx.z];
  return uv[0] * barycentrics.x + uv[1] * barycentrics.y + uv[2] * barycentrics.z;
}

bool hasVertexTangent(RenderPrimitive renderPrim)
{
  return renderPrim.vertexBuffer.tangentAddress != 0;
}

vec4 getVertexTangent(RenderPrimitive renderPrim, uint idx)
{
  if(!hasVertexTangent(renderPrim))
    return vec4(1, 0, 0, 1);
  return VertexTangent(renderPrim.vertexBuffer.tangentAddress)._[idx];
}

vec4 getInterpolatedVertexTangent(RenderPrimitive renderPrim, uvec3 idx, vec3 barycentrics)
{
  if(!hasVertexTangent(renderPrim))
    return vec4(1, 0, 0, 1);

  VertexTangent tangents = VertexTangent(renderPrim.vertexBuffer.tangentAddress);
  vec4          tng[3];
  tng[0] = tangents._[idx.x];
  tng[1] = tangents._[idx.y];
  tng[2] = tangents._[idx.z];
  return tng[0] * barycentrics.x + tng[1] * barycentrics.y + tng[2] * barycentrics.z;
}


bool hasVertexColor(RenderPrimitive renderPrim)
{
  return renderPrim.vertexBuffer.colorAddress != 0;
}

vec4 getVertexColor(RenderPrimitive renderPrim, uint idx)
{
  if(!hasVertexColor(renderPrim))
    return vec4(1, 1, 1, 1);
  return unpackUnorm4x8(VertexColor(renderPrim.vertexBuffer.colorAddress)._[idx]);
}

vec4 getInterpolatedVertexColor(RenderPrimitive renderPrim, uvec3 idx, vec3 barycentrics)
{
  if(!hasVertexColor(renderPrim))
    return vec4(1, 1, 1, 1);

  VertexColor colors = VertexColor(renderPrim.vertexBuffer.colorAddress);
  vec4        col[3];
  col[0] = unpackUnorm4x8(colors._[idx.x]);
  col[1] = unpackUnorm4x8(colors._[idx.y]);
  col[2] = unpackUnorm4x8(colors._[idx.z]);
  return col[0] * barycentrics.x + col[1] * barycentrics.y + col[2] * barycentrics.z;
}

#endif  // VERTEX_ACCESSOR_H
