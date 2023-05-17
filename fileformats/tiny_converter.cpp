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

#include "tiny_converter.hpp"


void TinyConverter::convert(tinygltf::Model& gltf, const tinyobj::ObjReader& reader)
{
  // Default assets
  gltf.asset.copyright = "NVIDIA Corporation";
  gltf.asset.generator = "OBJ converter";
  gltf.asset.version   = "2.0";  // glTF version 2.0

  // Adding one buffer
  gltf.buffers.emplace_back();
  auto& tBuffer = gltf.buffers.back();

  // Materials
  for(const auto& mat : reader.GetMaterials())
    convertMaterial(gltf, mat);

  if(gltf.materials.empty())
    gltf.materials.emplace_back();  // Default material

  // Unordered map of unique Vertex
  auto hash  = [&](const Vertex& v) { return makeHash(v); };
  auto equal = [&](const Vertex& l, const Vertex& r) { return l == r; };
  std::unordered_map<Vertex, size_t, decltype(hash), decltype(equal)> vertexToIdx(0, hash, equal);

  // Building unique vertices
  auto&                      attrib = reader.GetAttrib();
  std::vector<nvmath::vec3f> vertices;
  std::vector<nvmath::vec3f> normals;
  std::vector<nvmath::vec2f> texcoords;
  vertices.reserve((int)(attrib.vertices.size()) / 3);
  normals.reserve((int)(attrib.normals.size()) / 3);
  texcoords.reserve((int)(attrib.texcoords.size()) / 2);

  Bbox bb;
  for(const auto& shape : reader.GetShapes())
  {
    for(const auto& index : shape.mesh.indices)
    {
      const auto v = getVertex(attrib, index);
      if(vertexToIdx.find(v) == vertexToIdx.end())
      {
        vertexToIdx[v] = vertexToIdx.size();
        vertices.push_back(v.pos);
        bb.insert(v.pos);
        if(!attrib.normals.empty())
          normals.push_back(v.nrm);
        if(!attrib.texcoords.empty())
          texcoords.push_back(v.tex);
      }
    }
  }
  vertices.shrink_to_fit();
  normals.shrink_to_fit();
  texcoords.shrink_to_fit();

  // Number of unique vertices
  uint32_t nbVertices = (uint32_t)vertexToIdx.size();

  // Estimate size of buffer before appending data
  uint32_t nbIndices{0};
  for(const auto& shape : reader.GetShapes())
    nbIndices += (uint32_t)shape.mesh.indices.size();
  size_t bufferEstimateSize{0};
  bufferEstimateSize += nbVertices * sizeof(nvmath::vec3f);
  bufferEstimateSize += normals.empty() ? 0 : nbVertices * sizeof(nvmath::vec3f);
  bufferEstimateSize += texcoords.empty() ? 0 : nbVertices * sizeof(nvmath::vec2f);
  bufferEstimateSize += nbIndices * sizeof(uint32_t);
  tBuffer.data.reserve(bufferEstimateSize);  // Reserving to make the allocations faster


  // Storing the information in the glTF buffer
  {
    struct OffsetLen
    {
      uint32_t offset{0};
      uint32_t len{0};
    };

    // Make buffer of attribs
    OffsetLen olIdx, olPos, olNrm, olTex;
    auto&     tBuffer = gltf.buffers.back();
    olPos.offset      = static_cast<uint32_t>(tBuffer.data.size());
    olPos.len         = appendData(tBuffer, vertices);
    olNrm.offset      = static_cast<uint32_t>(tBuffer.data.size());
    olNrm.len         = appendData(tBuffer, normals);
    olTex.offset      = static_cast<uint32_t>(tBuffer.data.size());
    olTex.len         = appendData(tBuffer, texcoords);

    // Same buffer views for all shapes
    int posBufferView{-1};
    int nrmBufferView{-1};
    int texBufferView{-1};

    // Buffer View (POSITION)
    {
      gltf.bufferViews.emplace_back();
      auto& tBufferView      = gltf.bufferViews.back();
      tBufferView.buffer     = 0;
      tBufferView.byteOffset = olPos.offset;
      tBufferView.byteStride = 3 * sizeof(float);
      tBufferView.byteLength = nbVertices * tBufferView.byteStride;

      // Accessor (POSITION)
      gltf.accessors.emplace_back();
      auto& tAccessor         = gltf.accessors.back();
      tAccessor.bufferView    = static_cast<int>(gltf.bufferViews.size() - 1);
      tAccessor.byteOffset    = 0;
      tAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
      tAccessor.count         = nbVertices;
      tAccessor.type          = TINYGLTF_TYPE_VEC3;
      tAccessor.minValues     = {bb.min()[0], bb.min()[1], bb.min()[2]};
      tAccessor.maxValues     = {bb.max()[0], bb.max()[1], bb.max()[2]};
      assert(tAccessor.count > 0);
      posBufferView = (int)gltf.accessors.size() - 1;
    }

    // Buffer View (NORMAL)
    if(!attrib.normals.empty())
    {
      gltf.bufferViews.emplace_back();
      auto& tBufferView      = gltf.bufferViews.back();
      tBufferView.buffer     = 0;
      tBufferView.byteOffset = olNrm.offset;
      tBufferView.byteStride = 3 * sizeof(float);
      tBufferView.byteLength = nbVertices * tBufferView.byteStride;

      // Accessor (NORMAL)
      gltf.accessors.emplace_back();
      auto& tAccessor         = gltf.accessors.back();
      tAccessor.bufferView    = static_cast<int>(gltf.bufferViews.size() - 1);
      tAccessor.byteOffset    = 0;
      tAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
      tAccessor.count         = nbVertices;
      tAccessor.type          = TINYGLTF_TYPE_VEC3;
      nrmBufferView           = (int)gltf.accessors.size() - 1;
    }

    // Buffer View (TEXCOORD_0)
    if(!attrib.texcoords.empty())
    {
      gltf.bufferViews.emplace_back();
      auto& tBufferView      = gltf.bufferViews.back();
      tBufferView.buffer     = 0;
      tBufferView.byteOffset = olTex.offset;
      tBufferView.byteStride = 2 * sizeof(float);
      tBufferView.byteLength = nbVertices * tBufferView.byteStride;

      // Accessor (TEXCOORD_0)
      gltf.accessors.emplace_back();
      auto& tAccessor         = gltf.accessors.back();
      tAccessor.bufferView    = static_cast<int>(gltf.bufferViews.size() - 1);
      tAccessor.byteOffset    = 0;
      tAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
      tAccessor.count         = nbVertices;
      tAccessor.type          = TINYGLTF_TYPE_VEC2;
      texBufferView           = (int)gltf.accessors.size() - 1;
    }

    // Create one node/mesh/primitive per shape
    for(const auto& shape : reader.GetShapes())
    {
      uint32_t idxBufferView{0};
      // Finding the unique vertex index for the shape
      std::vector<uint32_t> indices;
      indices.reserve(shape.mesh.indices.size());
      for(const auto& index : shape.mesh.indices)
      {
        const auto v   = getVertex(attrib, index);
        size_t     idx = vertexToIdx[v];
        indices.push_back((uint32_t)idx);
      }

      // Appending the index data to the glTF buffer
      auto& tBuffer = gltf.buffers.back();
      olIdx.offset  = static_cast<uint32_t>(tBuffer.data.size());
      olIdx.len     = appendData(tBuffer, indices);

      // Adding a Buffer View (INDICES)
      {
        gltf.bufferViews.emplace_back();
        auto& tBufferView      = gltf.bufferViews.back();
        tBufferView.buffer     = 0;
        tBufferView.byteOffset = olIdx.offset;
        tBufferView.byteStride = 0;  // "bufferView.byteStride must not be defined for indices accessor." ;
        tBufferView.byteLength = sizeof(uint32_t) * indices.size();

        // Accessor (INDICES)
        gltf.accessors.emplace_back();
        auto& tAccessor         = gltf.accessors.back();
        tAccessor.bufferView    = static_cast<int>(gltf.bufferViews.size() - 1);
        tAccessor.byteOffset    = 0;
        tAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
        tAccessor.count         = indices.size();
        tAccessor.type          = TINYGLTF_TYPE_SCALAR;
        idxBufferView           = static_cast<int>(gltf.accessors.size() - 1);
      }


      // Adding a glTF mesh
      tinygltf::Mesh mesh;
      mesh.name = shape.name;

      // One primitive under the mesh
      mesh.primitives.emplace_back();
      auto& tPrim = mesh.primitives.back();
      tPrim.mode  = TINYGLTF_MODE_TRIANGLES;

      // Material reference
      // #TODO - We assume all primitives have the same material
      tPrim.material = shape.mesh.material_ids.empty() ? 0 : shape.mesh.material_ids[0];
      tPrim.material = std::max(0, tPrim.material);

      // Setting all buffer views
      tPrim.indices                = idxBufferView;
      tPrim.attributes["POSITION"] = posBufferView;
      if(nrmBufferView > 0)
        tPrim.attributes["NORMAL"] = nrmBufferView;
      if(texBufferView > 0)
        tPrim.attributes["TEXCOORD_0"] = texBufferView;

      // Adding the mesh
      gltf.meshes.emplace_back(mesh);

      // Adding the node referencing the mesh we just have created
      tinygltf::Node node;
      node.name = mesh.name;
      node.mesh = static_cast<int>(gltf.meshes.size() - 1);
      gltf.nodes.emplace_back(node);
    }
  }


  // Scene
  gltf.defaultScene = 0;
  tinygltf::Scene scene;
  for(int n = 0; n < (int)gltf.nodes.size(); n++)
    scene.nodes.push_back(n);
  gltf.scenes.emplace_back(scene);

  // Shrink back
  tBuffer.data.shrink_to_fit();
}

TinyConverter::Vertex TinyConverter::getVertex(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index)
{
  Vertex       v;
  const float* vp = &attrib.vertices[3ULL * index.vertex_index];
  v.pos           = {*(vp + 0), *(vp + 1), *(vp + 2)};
  if(!attrib.normals.empty() && index.normal_index >= 0)
  {
    const float* np = &attrib.normals[3ULL * index.normal_index];
    v.nrm           = {*(np + 0), *(np + 1), *(np + 2)};
  }
  if(!attrib.texcoords.empty() && index.texcoord_index >= 0)
  {
    const float* tp = &attrib.texcoords[2ULL * index.texcoord_index + 0];
    v.tex           = {*tp, 1.0f - *(tp + 1)};
  }
  return v;
}

void TinyConverter::convertMaterial(tinygltf::Model& gltf, const tinyobj::material_t& mat)
{
  tinygltf::TextureInfo          baseColorTexture;
  tinygltf::TextureInfo          emissiveTexture;
  tinygltf::NormalTextureInfo    normalTexture;
  tinygltf::OcclusionTextureInfo occlusionTexture;
  tinygltf::TextureInfo metallicRoughnessTexture = createMetallicRoughnessTexture(mat.metallic_texname, mat.roughness_texname);

  baseColorTexture.index = convertTexture(gltf, mat.diffuse_texname);
  emissiveTexture.index  = convertTexture(gltf, mat.emissive_texname);
  normalTexture.index    = convertTexture(gltf, mat.normal_texname);
  occlusionTexture.index = convertTexture(gltf, mat.ambient_texname);


  tinygltf::Material gMat;
  gMat.name                                 = mat.name;
  gMat.emissiveFactor                       = {mat.emission[0], mat.emission[1], mat.emission[2]};
  gMat.pbrMetallicRoughness.baseColorFactor = {mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1};
  gMat.pbrMetallicRoughness.metallicFactor  = (mat.specular[0] + mat.specular[1] + mat.specular[2]) / 3.0f;
  gMat.pbrMetallicRoughness.roughnessFactor = mat.shininess;

  gMat.doubleSided                                   = false;
  gMat.normalTexture                                 = normalTexture;
  gMat.occlusionTexture                              = occlusionTexture;
  gMat.emissiveTexture                               = emissiveTexture;
  gMat.pbrMetallicRoughness.baseColorTexture         = baseColorTexture;
  gMat.pbrMetallicRoughness.metallicRoughnessTexture = metallicRoughnessTexture;


  gltf.materials.emplace_back(gMat);
}

int TinyConverter::convertTexture(tinygltf::Model& gltf, const std::string& diffuse_texname)
{
  if(diffuse_texname.empty())
    return -1;

  int sourceImg = findImage(gltf, diffuse_texname);
  if(sourceImg < 0)
  {
    tinygltf::Image img;
    img.uri = diffuse_texname;
    gltf.images.emplace_back(img);
    sourceImg = (int)gltf.images.size() - 1;
  }

  int sourceTex = findTexture(gltf, sourceImg);
  if(sourceTex < 0)
  {
    tinygltf::Texture tex;
    tex.source = sourceImg;
    gltf.textures.emplace_back(tex);
    sourceTex = (int)gltf.textures.size() - 1;
  }
  return sourceTex;
}


int TinyConverter::findImage(tinygltf::Model& gltf, const std::string& texname)
{
  int idx{-1};
  for(const auto& i : gltf.images)
  {
    ++idx;
    if(i.uri == texname)
      return idx;
  }
  return -1;
}

int TinyConverter::findTexture(tinygltf::Model& gltf, int source)
{
  int idx{-1};
  for(const auto& t : gltf.textures)
  {
    ++idx;
    if(t.source == source)
      return idx;
  }
  return -1;
}

tinygltf::TextureInfo TinyConverter::createMetallicRoughnessTexture(std::string metallic_texname, std::string roughness_texname)
{
  tinygltf::TextureInfo tex;
  return tex;

  // #TODO Mix metallic and roughness in one channel and add inline image or save to disk?
}
