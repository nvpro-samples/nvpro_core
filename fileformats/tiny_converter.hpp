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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>

#include "nvmath/nvmath.h"

#include "tiny_gltf.h"
#include "tiny_obj_loader.h"


class TinyConverter
{
public:
  void convert(tinygltf::Model& gltf, const tinyobj::ObjReader& reader);


private:
  //---- Hash Combination ----
  // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3876.pdf
  template <typename T>
  void hashCombine(std::size_t& seed, const T& val)
  {
    seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  // Auxiliary generic functions to create a hash value using a seed
  template <typename T, typename... Types>
  void hashCombine(std::size_t& seed, const T& val, const Types&... args)
  {
    hashCombine(seed, val);
    hashCombine(seed, args...);
  }
  // Optional auxiliary generic functions to support hash_val() without arguments
  void hashCombine(std::size_t& seed) {}
  // Generic function to create a hash value out of a heterogeneous list of arguments
  template <typename... Types>
  std::size_t hashVal(const Types&... args)
  {
    std::size_t seed = 0;
    hashCombine(seed, args...);
    return seed;
  }
  //--------------

  struct Vertex
  {
    nvmath::vec3f pos;
    nvmath::vec3f nrm;
    nvmath::vec2f tex;

    bool operator==(const Vertex& l) const { return this->pos == l.pos && this->nrm == l.nrm && this->tex == l.tex; }
  };


  Vertex getVertex(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index);
  void   convertMaterial(tinygltf::Model& gltf, const tinyobj::material_t& mat);
  int    convertTexture(tinygltf::Model& gltf, const std::string& diffuse_texname);

  int findImage(tinygltf::Model& gltf, const std::string& texname);
  int findTexture(tinygltf::Model& gltf, int source);

  tinygltf::TextureInfo createMetallicRoughnessTexture(std::string metallic_texname, std::string roughness_texname);

  // This is appending the incoming data to the binary buffer and return the amount in byte of data that was added.
  template <class T>
  uint32_t appendData(tinygltf::Buffer& buffer, const T& inData)
  {
    auto*    pData = reinterpret_cast<const char*>(inData.data());
    uint32_t len   = static_cast<uint32_t>(sizeof(inData[0]) * inData.size());
    buffer.data.insert(buffer.data.end(), pData, pData + len);
    return len;
  }


  struct Bbox
  {
    Bbox() = default;

    void insert(const nvmath::vec3f& v)
    {
      m_min = {std::min(m_min[0], v[0]), std::min(m_min[1], v[1]), std::min(m_min[2], v[2])};
      m_max = {std::max(m_max[0], v[0]), std::max(m_max[1], v[1]), std::max(m_max[2], v[2])};
    }
    inline nvmath::vec3f min() { return m_min; }
    inline nvmath::vec3f max() { return m_max; }

  private:
    nvmath::vec3f m_min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    nvmath::vec3f m_max{-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
                        -std::numeric_limits<float>::max()};
  };

  std::size_t makeHash(const Vertex& v)
  {
    return hashVal(v.pos.x, v.pos.y, v.pos.z, v.nrm.x, v.nrm.y, v.nrm.z, v.tex.x, v.tex.y);
  }
};
