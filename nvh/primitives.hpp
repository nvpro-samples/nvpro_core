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

#pragma once
#include <vector>
#include <cstdint>
#include "nvmath/nvmath.h"

// PrimitiveMesh:
//  - Common primitive type, made of vertices: position, normal and texture coordinates.
//  - All primitives are triangles, and each 3 indices is forming a triangle.
//
// Node:
//  - Structure to hold a reference to a mesh, with a material and transformation.


namespace nvh {
struct PrimitiveVertex
{
  nvmath::vec3f p;  // Position
  nvmath::vec3f n;  // Normal
  nvmath::vec2f t;  // Texture Coordinates
};

struct PrimitiveTriangle
{
  nvmath::vec3ui v;  // vertex indices
};

struct PrimitiveMesh
{
  std::vector<PrimitiveVertex>   vertices;   // Array of all vertex
  std::vector<PrimitiveTriangle> triangles;  // Indices forming triangles
};

struct Node
{
  nvmath::vec3f             translation{};         //
  nvmath::quaternion<float> rotation{0, 0, 0, 1};  //
  nvmath::vec3f             scale{1.0F};           //
  nvmath::mat4f             matrix{1};             // Added with the above transformations
  int                       material{0};
  int                       mesh{-1};

  nvmath::mat4f localMatrix() const
  {
    nvmath::mat4f mrot, mscale, mtrans;
    rotation.to_matrix(mrot);
    mscale.as_scale(scale);
    mtrans.as_translation(translation);
    return mtrans * mrot * mscale * matrix;
  }
};

PrimitiveMesh createTetrahedron();
PrimitiveMesh createIcosahedron();
PrimitiveMesh createOctahedron();
PrimitiveMesh createPlane(int steps = 1, float width = 1.0F, float depth = 1.0F);
PrimitiveMesh createCube(float width = 1.0F, float height = 1.0F, float depth = 1.0F);
PrimitiveMesh createSphereUv(float radius = 0.5F, int sectors = 20, int stacks = 20);
PrimitiveMesh createConeMesh(float radius = 0.5F, float height = 1.0F, int segments = 16);
PrimitiveMesh createSphereMesh(float radius = 0.5F, int subdivisions = 3);
PrimitiveMesh createTorusMesh(float majorRadius = 0.5F, float minorRadius = 0.25F, int majorSegments = 32, int minorSegments = 16);

std::vector<Node> mengerSpongeNodes(int level = 3, float probability = -1.f, int seed = 1);

// Utilities
PrimitiveMesh mergeNodes(const std::vector<Node>& nodes, const std::vector<PrimitiveMesh> meshes);
PrimitiveMesh removeDuplicateVertices(const PrimitiveMesh& mesh, bool testNormal = true, bool testUv = true);
PrimitiveMesh wobblePrimitive(const PrimitiveMesh& mesh, float amplitude = 0.05F);

}  // namespace nvh
