/*-----------------------------------------------------------------------
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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
*/ //--------------------------------------------------------------------

#include <cstdio>
#include <iostream>
#include <sstream>

#include "gltf_optix.h"


#pragma warning(push)
#pragma warning(disable : 4100)
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <gltf/tiny_gltf.h>
#pragma warning(pop)


#include "util_optix.h"
#include <nvh/stopwatch.hpp>

namespace nvmath {
template <typename T>
vec4f make_vec4(T* dvalue)
{
  return {dvalue[0], dvalue[1], dvalue[2], dvalue[3]};
}

template <typename T>
vec3f make_vec3(const T* dvalue)
{
  return {dvalue[0], dvalue[1], dvalue[2]};
}

template <typename T>
vec2f make_vec2(const T* dvalue)
{
  return {dvalue[0], dvalue[1]};
}

template <typename T>
quatf make_quat(const T* dvalue)
{
  return {dvalue[0], dvalue[1], dvalue[2], dvalue[3]};
}

template <typename T>
mat4 make_mat4x4(const T* dvalue)
{
  mat4   result;
  float* pmat = result.get_value();
  for(int i = 0; i < 16; ++i)
  {
    *pmat = static_cast<float>(dvalue[i]);
    pmat++;
  }

  return result;
}


}  // namespace nvmath


namespace nvoptix {

optix::TextureSampler gltfTexture::m_optixDefaultOne;


//--------------------------------------------------------------------------------------------------
// Loading a glTF file into OptiX world
//
void OptixGLtf::loadFromFile(optix::Context context, const std::string& filename)
{
  m_context = context;

  // Create a dummy texture
  if(gltfTexture::m_optixDefaultOne.get() == nullptr)
  {
    gltfTexture::m_optixDefaultOne = m_context->createTextureSampler();
    optix::Buffer buffer           = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_UNSIGNED_BYTE4, 1, 1);
    auto          buffer_data      = static_cast<unsigned char*>(buffer->map());
    buffer_data[0]                 = 255;
    buffer_data[1]                 = 255;
    buffer_data[2]                 = 255;
    buffer_data[3]                 = 255;
    buffer->unmap();
    gltfTexture::m_optixDefaultOne->setBuffer(0u, 0u, buffer);
  }

  tinygltf::TinyGLTF    gltfContext;
  std::string           error;
  tinygltf::Model       gltfModel;
  nvh::Stopwatch sw;

  // Reading the scene
  sw.start();
  if(gltfContext.LoadASCIIFromFile(&gltfModel, &error, filename.c_str()) == false)
    throw optix::Exception("GLTF: error loading file " + filename);
  sw.stop();
  std::cout << "Time ( " << sw.elapsed() / 1000.0 << "s ) for loading scene " << filename << std::endl;

  // Converting the textures and materials
  loadImages(gltfModel);
  loadMaterials(gltfModel);

  // Converting all nodes (meshes, ...)
  sw.startNew();
  if(gltfModel.defaultScene < 0)
    gltfModel.defaultScene = 0;
  const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene];
  for(size_t i = 0; i < scene.nodes.size(); i++)
  {
    const tinygltf::Node& node = gltfModel.nodes[scene.nodes[i]];
    loadNode(nullptr, node, scene.nodes[i], gltfModel);
  }

  // Creating a flat group all all primitives
  m_optixTlas                 = m_context->createGroup();
  optix::Acceleration accTlas = m_context->createAcceleration("Trbvh");
  accTlas->setProperty("refit", "1");
  m_optixTlas->setAcceleration(accTlas);
  for(auto n : m_linearNodes)
  {
    gltfMesh* mesh = n->m_mesh;
    if(mesh != nullptr)
    {
      for(auto p : mesh->m_primitives)
        m_optixTlas->addChild(p->m_optixBlas);
    }
  }
  sw.stop();
  std::cout << "Time ( " << sw.elapsed() / 1000.0 << "s ) for converting to OptiX" << std::endl;

  // Computing the scene's dimensions. Can be useful to set the camera
  getSceneDimensions();

  // Updating the cameras
  if(m_cameras.empty())
  {
    auto camera = new gltfCamera(nullptr);
    camera->frameBox(m_dimensions.min, m_dimensions.max);
    m_cameras.push_back(camera);
  }
  else
  {
    for(auto& c : m_cameras)
    {
      if(c->m_parent)
      {
        nvmath::mat4f m   = c->m_parent->getMatrix();
        nvmath::vec4f pos = m * nvmath::vec4f(0, 0, 0, 1);
        c->m_eye           = nvmath::vec3f(pos);
      }
    }
  }


  // Printing out statistics
  std::cout << "Statistics:" << std::endl;
  std::cout << " - Elements  " << std::to_string(m_optixTlas->getChildCount()) << std::endl;
  std::cout << " - Triangles " << std::to_string(m_nbTriangles) << std::endl;
  std::cout << " - Materials " << std::to_string(m_materials.size() - 1) << std::endl;
  std::cout << " - Textures  " << std::to_string(m_textures.size()) << std::endl;
  std::cout << " - Size " << std::to_string(m_dimensions.size.x) << ", " << std::to_string(m_dimensions.size.y) << ", "
            << std::to_string(m_dimensions.size.z) << std::endl;
  std::cout << " - Center " << std::to_string(m_dimensions.center.x) << ", " << std::to_string(m_dimensions.center.y)
            << ", " << std::to_string(m_dimensions.center.z) << std::endl;
}


//--------------------------------------------------------------------------------------------------
// Converting all images to OptiX
//
void OptixGLtf::loadImages(tinygltf::Model& gltfModel)
{
  for(tinygltf::Image& image : gltfModel.images)
  {
    gltfTexture texture;
    texture.fromglTfImage(m_context, image);
    m_textures.push_back(texture);
  }
}


//--------------------------------------------------------------------------------------------------
// Grabbing all values of the material and setting the OptiX counter part.
//
void OptixGLtf::loadMaterials(tinygltf::Model& gltfModel)
{
  m_materials.reserve(gltfModel.materials.size() + 1);

  // Creating a default material
  {
    gltfMaterial material{};
    material.updateOptix(m_context);
    m_materials.push_back(material);
  }


  for(tinygltf::Material& mat : gltfModel.materials)
  {
    gltfMaterial material{};


    if(mat.values.find("baseColorFactor") != mat.values.end())
    {
      material.m_baseColorFactor = nvmath::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
    }

    if(mat.values.find("baseColorTexture") != mat.values.end())
    {
      material.m_baseColorTexture = &m_textures[gltfModel.textures[mat.values["baseColorTexture"].TextureIndex()].source];
    }

    if(mat.values.find("metallicRoughnessTexture") != mat.values.end())
    {
      material.m_metallicRoughnessTexture =
          &m_textures[gltfModel.textures[mat.values["metallicRoughnessTexture"].TextureIndex()].source];
    }

    if(mat.values.find("roughnessFactor") != mat.values.end())
    {
      material.m_roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
    }

    if(mat.values.find("metallicFactor") != mat.values.end())
    {
      material.m_metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
    }

    if(mat.additionalValues.find("normalTexture") != mat.additionalValues.end())
    {
      material.m_normalTexture = &m_textures[gltfModel.textures[mat.additionalValues["normalTexture"].TextureIndex()].source];
    }

    if(mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end())
    {
      material.m_emissiveTexture =
          &m_textures[gltfModel.textures[mat.additionalValues["emissiveTexture"].TextureIndex()].source];
    }

    if(mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end())
    {
      material.m_occlusionTexture =
          &m_textures[gltfModel.textures[mat.additionalValues["occlusionTexture"].TextureIndex()].source];
    }

    if(mat.additionalValues.find("alphaMode") != mat.additionalValues.end())
    {
      tinygltf::Parameter param = mat.additionalValues["alphaMode"];

      if(param.string_value == "BLEND")
        material.m_alphaMode = gltfMaterial::ALPHAMODE_BLEND;

      if(param.string_value == "MASK")
        material.m_alphaMode = gltfMaterial::ALPHAMODE_MASK;
    }

    if(mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end())
    {
      material.m_alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
    }

    // KHR_materials_pbrSpecularGlossiness
    // Extensions
    if(mat.extPBRValues.size() > 0)
    {
      // KHR_materials_pbrSpecularGlossiness
      if(mat.extPBRValues.find("specularGlossinessTexture") != mat.extPBRValues.end())
      {
        material.m_metallicRoughnessTexture =
            &m_textures[gltfModel.textures[mat.extPBRValues["specularGlossinessTexture"].TextureIndex()].source];
        material.m_pbrSpecularGlossiness = true;
      }
      if(mat.extPBRValues.find("diffuseTexture") != mat.extPBRValues.end())
      {
        material.m_baseColorTexture = &m_textures[gltfModel.textures[mat.extPBRValues["diffuseTexture"].TextureIndex()].source];
      }
      if(mat.extPBRValues.find("diffuseFactor") != mat.extPBRValues.end())
      {
        material.m_diffuseFactor = nvmath::make_vec4(mat.extPBRValues["diffuseFactor"].ColorFactor().data());
      }
      if(mat.extPBRValues.find("specularFactor") != mat.extPBRValues.end())
      {
        material.m_specularFactor =
            nvmath::vec4f(nvmath::make_vec3(mat.extPBRValues["specularFactor"].ColorFactor().data()), 1.0);
      }
    }


    // Optix conversion
    material.updateOptix(m_context);


    m_materials.push_back(material);
  }
}

//--------------------------------------------------------------------------------------------------
// Nodes refers to meshes or cameras, grab all information
//
void OptixGLtf::loadNode(gltfNode* parentNode, const tinygltf::Node& tinyNode, uint32_t nodeIndex, const tinygltf::Model& tinyModel)
{
  gltfNode* newNode    = new gltfNode{};
  newNode->m_index     = nodeIndex;
  newNode->m_parent    = parentNode;
  newNode->m_name      = tinyNode.name;
  newNode->m_skinIndex = tinyNode.skin;
  newNode->m_matrix    = nvmath::mat4f().identity();

  // Generate local node matrix
  if(tinyNode.translation.size() == 3)
    newNode->m_translation = nvmath::make_vec3(tinyNode.translation.data());
  if(tinyNode.rotation.size() == 4)
    newNode->m_rotation = nvmath::make_quat(tinyNode.rotation.data());
  if(tinyNode.scale.size() == 3)
    newNode->m_scale = nvmath::make_vec3(tinyNode.scale.data());
  if(tinyNode.matrix.size() == 16)
    newNode->m_matrix = nvmath::make_mat4x4(tinyNode.matrix.data());

  // Node with children
  if(!tinyNode.children.empty())
  {
    for(int i : tinyNode.children)
      loadNode(newNode, tinyModel.nodes[i], i, tinyModel);
  }


  // Node contains mesh data
  if(tinyNode.mesh > -1)
    loadMesh(tinyModel, tinyNode, newNode);

  if(tinyNode.camera > -1)
    loadCamera(tinyModel, tinyNode, newNode);


  if(parentNode)
  {
    parentNode->m_children.push_back(newNode);
  }
  else
  {
    m_nodes.push_back(newNode);
  }

  m_linearNodes.push_back(newNode);
}

//--------------------------------------------------------------------------------------------------
// Loading the Mesh which can be made of multiple primitives, each having its own material
//
void OptixGLtf::loadMesh(const tinygltf::Model& tinyModel, const tinygltf::Node& tinyNode, gltfNode* newNode)
{

  const tinygltf::Mesh mesh    = tinyModel.meshes[tinyNode.mesh];
  gltfMesh*            newMesh = new gltfMesh(newNode->m_matrix);
  for(const tinygltf::Primitive& primitive : mesh.primitives)
  {
    if(primitive.indices < 0)
      continue;

    // Index 0 of the material is the default one
    gltfMaterial&  primMaterial = m_materials[primitive.material + 1];
    gltfPrimitive* newPrimitive = new gltfPrimitive(0, 0, primMaterial);


    bool hasNormal = false;

    // Vertices
    {
      const float*    bufferPos       = nullptr;
      const float*    bufferNormals   = nullptr;
      const float*    bufferTexCoords = nullptr;
      const uint16_t* bufferJoints    = nullptr;
      const float*    bufferWeights   = nullptr;

      // Position attribute is required
      if(primitive.attributes.find("POSITION") == primitive.attributes.end())
        throw optix::Exception("glTF loader: missing position");

      const tinygltf::Accessor&   posAccessor = tinyModel.accessors[primitive.attributes.find("POSITION")->second];
      const tinygltf::BufferView& posView     = tinyModel.bufferViews[posAccessor.bufferView];
      bufferPos =
          reinterpret_cast<const float*>(&(tinyModel.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));


      // Keep track of the size of this primitive
      newPrimitive->m_dimensions.set(
          nvmath::vec3f(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]),
          nvmath::vec3f(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]));


      if(primitive.attributes.find("NORMAL") != primitive.attributes.end())
      {
        const tinygltf::Accessor&   normAccessor = tinyModel.accessors[primitive.attributes.find("NORMAL")->second];
        const tinygltf::BufferView& normView     = tinyModel.bufferViews[normAccessor.bufferView];

        bufferNormals = reinterpret_cast<const float*>(
            &(tinyModel.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
        hasNormal = true;
      }

      if(primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
      {
        const tinygltf::Accessor&   uvAccessor = tinyModel.accessors[primitive.attributes.find("TEXCOORD_0")->second];
        const tinygltf::BufferView& uvView     = tinyModel.bufferViews[uvAccessor.bufferView];

        bufferTexCoords =
            reinterpret_cast<const float*>(&(tinyModel.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
      }

      // Skinning
      // Joints
      if(primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
      {
        const tinygltf::Accessor&   jointAccessor = tinyModel.accessors[primitive.attributes.find("JOINTS_0")->second];
        const tinygltf::BufferView& jointView     = tinyModel.bufferViews[jointAccessor.bufferView];

        bufferJoints = reinterpret_cast<const uint16_t*>(
            &(tinyModel.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
      }

      if(primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
      {
        const tinygltf::Accessor&   uvAccessor = tinyModel.accessors[primitive.attributes.find("WEIGHTS_0")->second];
        const tinygltf::BufferView& uvView     = tinyModel.bufferViews[uvAccessor.bufferView];

        bufferWeights =
            reinterpret_cast<const float*>(&(tinyModel.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
      }

      bool hasSkin = (bufferJoints && bufferWeights);
      newPrimitive->m_vertexBuffer.resize(posAccessor.count);
      for(size_t v = 0; v < posAccessor.count; v++)
      {
        gltfVertex& vert = newPrimitive->m_vertexBuffer[v];
        vert.pos         = nvmath::vec4f(nvmath::make_vec3(&bufferPos[v * 3]), 1.0f);
        vert.normal      = bufferNormals ? nvmath::make_vec3(&bufferNormals[v * 3]) : nvmath::vec3f(0.0f, 0.0f, 1.0f);
        vert.uv          = bufferTexCoords ? nvmath::make_vec2(&bufferTexCoords[v * 2]) : nvmath::vec3f(0.0f);

        vert.joint0  = hasSkin ? nvmath::vec4f(nvmath::make_vec4(&bufferJoints[v * 4])) : nvmath::vec4f(0.0f);
        vert.weight0 = hasSkin ? nvmath::make_vec4(&bufferWeights[v * 4]) : nvmath::vec4f(0.0f);
      }
    }

    // Indices
    {
      uint32_t indexTriangleCount = 0;

      const tinygltf::Accessor&   accessor   = tinyModel.accessors[primitive.indices];
      const tinygltf::BufferView& bufferView = tinyModel.bufferViews[accessor.bufferView];
      const tinygltf::Buffer&     buffer     = tinyModel.buffers[bufferView.buffer];

      indexTriangleCount = static_cast<uint32_t>(accessor.count) / 3;

      newPrimitive->m_indexBuffer.resize(indexTriangleCount);
      const void* pIndexBuf = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

      switch(accessor.componentType)
      {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
        {
          memcpy(&newPrimitive->m_indexBuffer[0], pIndexBuf, accessor.count * sizeof(uint32_t));
          break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
        {
          const uint16_t* pbuf16 = reinterpret_cast<const uint16_t*>(pIndexBuf);
          for(uint32_t i = 0; i < indexTriangleCount; ++i)
          {
            newPrimitive->m_indexBuffer[i] = {*pbuf16, *(pbuf16 + 1), *(pbuf16 + 2)};
            pbuf16 += 3;
          }
          break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
        {
          const uint8_t* pbuf8 = reinterpret_cast<const uint8_t*>(pIndexBuf);
          for(uint32_t i = 0; i < indexTriangleCount; ++i)
          {
            newPrimitive->m_indexBuffer[i] = {*pbuf8, *(pbuf8 + 1), *(pbuf8 + 2)};
            pbuf8 += 3;
          }
          break;
        }
        default:
        {
          std::stringstream o;
          o << "Index component type " << accessor.componentType << " not supported!" << std::endl;
          throw optix::Exception(o.str());
        }
      }
    }

    // When there are no normals, compute geometric normal for each triangles
    if(hasNormal == false)
      computeNormals(newPrimitive->m_vertexBuffer, newPrimitive->m_indexBuffer);

    // Optix Conversion
    optix::GeometryInstance instOptix = m_context->createGeometryInstance();
    instOptix->addMaterial(primMaterial.m_optixMat);
    optix::GeometryTriangles geoTri = createGeometryTriangle(instOptix, newPrimitive->m_vertexBuffer, newPrimitive->m_indexBuffer);
    instOptix->setGeometryTriangles(geoTri);
    optix::Transform blas     = createBlas(instOptix, newNode->getMatrix());
    newPrimitive->m_optixBlas = blas;
    m_nbTriangles += static_cast<uint32_t>(newPrimitive->m_indexBuffer.size());

    newMesh->m_primitives.push_back(newPrimitive);
  }

  newNode->m_mesh = newMesh;
}

//--------------------------------------------------------------------------------------------------
// Loading the Camera
//
void OptixGLtf::loadCamera(const tinygltf::Model& tinyModel, const tinygltf::Node& tinyNode, gltfNode* newNode)
{
  const tinygltf::Camera camera = tinyModel.cameras[tinyNode.camera];

  auto newCamera = new gltfCamera(newNode);
  if(camera.type == "perspective")
  {
    newCamera->m_perspective = true;
    newCamera->m_fov         = camera.perspective.yfov;
  }
  else if(camera.type == "orthographic")
  {
    newCamera->m_perspective = false;
  }

  m_cameras.push_back(newCamera);
}

void OptixGLtf::getSceneDimensions()
{
  nvmath::vec3f nodeMin(FLT_MAX);
  nvmath::vec3f nodeMax(-FLT_MAX);
  for(auto node : m_nodes)
    getNodeDimensions(node, nodeMin, nodeMax);
  m_dimensions.set(nodeMin, nodeMax);
}

void OptixGLtf::getNodeDimensions(gltfNode* node, nvmath::vec3f& min, nvmath::vec3f& max)
{
  if(node->m_mesh)
  {
    for(gltfPrimitive* primitive : node->m_mesh->m_primitives)
    {
      nvmath::mat4  nodeMatrix = node->getMatrix();
      nvmath::vec3f vbox       = nvmath::vec3f(primitive->m_dimensions.max - primitive->m_dimensions.min) / 2.0f;
      for(int i = 0; i < 8; i++)
      {
        nvmath::vec4f vct(i & 1 ? vbox.x : -vbox.x, i & 2 ? vbox.y : -vbox.y, i & 4 ? vbox.z : -vbox.z, 0.0f);
        vct = nodeMatrix * vct;
        min = nvmath::nv_min(min, nvmath::vec3f(vct));
        max = nvmath::nv_max(max, nvmath::vec3f(vct));
      }
    }
  }

  for(auto child : node->m_children)
    getNodeDimensions(child, min, max);
}

// Add a Bottom-Level-Acceleration-Structure (BLAS)
optix::Transform OptixGLtf::createBlas(const optix::GeometryInstance& geoInstance, const nvmath::mat4& matrix)
{
  optix::GeometryGroup geometrygroup = m_context->createGeometryGroup();
  geometrygroup->addChild(geoInstance);
  geometrygroup->setAcceleration(m_context->createAcceleration("Trbvh"));

  optix::Transform    transform   = m_context->createTransform();
  const nvmath::mat4 inverse_mat = nvmath::invert(matrix);
  transform->setMatrix(true, matrix.get_value(), inverse_mat.get_value());
  transform->setChild(geometrygroup);
  return transform;
}


// Create a geometry triangle with the vertices and indices
// The Geometry Instance will have access to the two buffers
optix::GeometryTriangles OptixGLtf::createGeometryTriangle(optix::GeometryInstance            geoInstance,
                                                           const std::vector<gltfVertex>&     triVertices,
                                                           const std::vector<nvmath::uvec3>& triIndices)
{
  // Vertex buffer creation and initialization.
  uint32_t      nb_vertices = static_cast<uint32_t>(triVertices.size());
  optix::Buffer vbuffer     = m_context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_USER, nb_vertices);
  vbuffer->setElementSize(sizeof(gltfVertex));
  void* vertex_buffer_data = vbuffer->map();
  memcpy(vertex_buffer_data, triVertices.data(), sizeof(gltfVertex) * nb_vertices);
  vbuffer->unmap();

  // Index buffer creation and initialization.
  uint32_t      nb_indices = static_cast<uint32_t>(triIndices.size());
  optix::Buffer ibuffer    = m_context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_USER, nb_indices);
  ibuffer->setElementSize(sizeof(nvmath::uvec3));
  void* index_buffer_data = ibuffer->map();
  memcpy(index_buffer_data, triIndices.data(), sizeof(nvmath::uvec3) * nb_indices);
  ibuffer->unmap();

  // Creation of the indexed triangle
  optix::GeometryTriangles triGeo = m_context->createGeometryTriangles();
  triGeo->setIndexedTriangles(nb_indices, ibuffer, 0, sizeof(nvmath::uvec3), RT_FORMAT_UNSIGNED_INT3,  //
                              nb_vertices, vbuffer, 0, sizeof(gltfVertex), RT_FORMAT_FLOAT3,            //
                              RT_GEOMETRYBUILDFLAGS_NONE);

  // Make the buffers available in the Closest Hit
  geoInstance["vertex_buffer"]->setBuffer(vbuffer);
  geoInstance["index_buffer"]->setBuffer(ibuffer);

  return triGeo;
}

void OptixGLtf::computeNormals(std::vector<gltfVertex>& vertexBuffer, std::vector<nvmath::uvec3>& indexBuffer)
{
  for(auto& i : indexBuffer)
  {
    const nvmath::vec3f& p0 = vertexBuffer[i.x].pos;
    const nvmath::vec3f& p1 = vertexBuffer[i.y].pos;
    const nvmath::vec3f& p2 = vertexBuffer[i.z].pos;

    nvmath::vec3f geoNormal = nvmath::cross(p2 - p0, p1 - p0);

    vertexBuffer[i.x].normal = geoNormal;
    vertexBuffer[i.y].normal = geoNormal;
    vertexBuffer[i.z].normal = geoNormal;
  }
}

//--------------------------------------------------------------------------------------------------
// Associating the closest hit program to all materials
//
void OptixGLtf::setClosestHit(int rayType, const optix::Program& prog)
{
  for(auto& m : m_materials)
    m.m_optixMat->setClosestHitProgram(rayType, prog);
}

//--------------------------------------------------------------------------------------------------
// Associating the any hit program to all materials
//
void OptixGLtf::setAnyHit(int rayType, const optix::Program& prog)
{
  for(auto& m : m_materials)
    m.m_optixMat->setAnyHitProgram(rayType, prog);
}

nvmath::mat4 gltfNode::getMatrix()
{
  nvmath::mat4 m = localMatrix();
  gltfNode*     p = m_parent;
  while(p)
  {
    m = p->localMatrix() * m;
    p = p->m_parent;
  }
  return m;
}

nvmath::mat4 gltfNode::localMatrix()
{
  nvmath::mat4 translation{1}, rotation{1}, scale{1};
  translation.translate(m_translation);
  rotation.rotate(m_rotation);
  scale.scale(m_scale);

  return translation * rotation * scale * m_matrix;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void gltfNode::update()
{
  if(m_mesh)
  {
    nvmath::mat4 m = getMatrix();
    if(m_skin)
    {
      m_mesh->m_uniformBlock.matrix = m;
      // Update join matrices
      nvmath::mat4 inverseTransform = nvmath::invert(m);
      for(size_t i = 0; i < m_skin->m_joints.size(); i++)
      {
        gltfNode*     jointNode               = m_skin->m_joints[i];
        nvmath::mat4 jointMat                = jointNode->getMatrix() * m_skin->m_inverseBindMatrices[i];
        jointMat                              = inverseTransform * jointMat;
        m_mesh->m_uniformBlock.jointMatrix[i] = jointMat;
      }
      m_mesh->m_uniformBlock.jointcount = (float)m_skin->m_joints.size();
      //                memcpy( m_mesh->m_uniformBuffer.mapped, &m_mesh->m_uniformBlock, sizeof( m_mesh->m_uniformBlock ) );
    }
    else
    {
      //              memcpy( m_mesh->m_uniformBuffer.mapped, &m, sizeof( nvmath::mat4 ) );
    }
  }

  for(auto& child : m_children)
  {
    child->update();
  }
}

void gltfMaterial::updateOptix(optix::Context context)
{
  m_optixMat = context->createMaterial();

  m_optixMat["workflow"]->setFloat(m_pbrSpecularGlossiness ? 1.0f : 0.0f);

  m_optixMat["baseColorFactor"]->set4fv(m_baseColorFactor.get_value());
  m_optixMat["diffuseFactor"]->set4fv(m_diffuseFactor.get_value());
  m_optixMat["specularFactor"]->set4fv(m_specularFactor.get_value());

  m_optixMat["metallicFactor"]->setFloat(m_metallicFactor);
  m_optixMat["roughnessFactor"]->setFloat(m_roughnessFactor);

  m_optixMat["hasBaseColorTexture"]->setFloat(m_baseColorTexture ? 1.f : 0.f);
  m_optixMat["hasMetallicRoughnessTexture"]->setFloat(m_metallicRoughnessTexture ? 1.f : 0.f);
  m_optixMat["hasNormalTexture"]->setFloat(m_normalTexture ? 1.f : 0.f);
  m_optixMat["hasOcclusionTexture"]->setFloat(m_occlusionTexture ? 1.f : 0.f);
  m_optixMat["hasEmissiveTexture"]->setFloat(m_emissiveTexture ? 1.f : 0.f);

  optix::TextureSampler one = gltfTexture::m_optixDefaultOne;
  m_optixMat["albedoMap"]->setTextureSampler(m_baseColorTexture ? m_baseColorTexture->m_optixTexture : one);
  m_optixMat["normalMap"]->setTextureSampler(m_normalTexture ? m_normalTexture->m_optixTexture : one);
  m_optixMat["aoMap"]->setTextureSampler(m_occlusionTexture ? m_occlusionTexture->m_optixTexture : one);
  m_optixMat["metallicMap"]->setTextureSampler(m_metallicRoughnessTexture ? m_metallicRoughnessTexture->m_optixTexture : one);
  m_optixMat["emissiveMap"]->setTextureSampler(m_emissiveTexture ? m_emissiveTexture->m_optixTexture : one);
}

void gltfTexture::fromglTfImage(optix::Context context, tinygltf::Image& gltfimage)
{
  // Buffer
  m_buffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_UNSIGNED_BYTE4, gltfimage.width, gltfimage.height);
  std::vector<uint8_t> tmpBuffer;
  uint8_t*             pImage = gltfimage.image.data();
  if(gltfimage.component < 4)
  {
    tmpBuffer.resize(gltfimage.width * gltfimage.height * 4);
    uint8_t* pDest   = tmpBuffer.data();
    pImage           = tmpBuffer.data();
    uint8_t* pSource = gltfimage.image.data();
    for(int i = 0; i < gltfimage.width * gltfimage.height; ++i)
    {
      *pDest++ = *pSource++;
      *pDest++ = *pSource++;
      *pDest++ = *pSource++;
      *pDest++ = 255;
    }
  }


  void* buffer_data = m_buffer->map();
  memcpy(buffer_data, pImage, sizeof(uint8_t) * gltfimage.width * gltfimage.height * 4);
  m_buffer->unmap();


  m_optixTexture = context->createTextureSampler();
  m_optixTexture->setWrapMode(0, RT_WRAP_REPEAT);
  m_optixTexture->setWrapMode(1, RT_WRAP_REPEAT);
  m_optixTexture->setWrapMode(2, RT_WRAP_REPEAT);
  m_optixTexture->setFilteringModes(RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_FILTER_NONE);
  //    m_optixTexture->setIndexingMode( RT_TEXTURE_INDEX_NORMALIZED_COORDINATES );
  m_optixTexture->setReadMode(RT_TEXTURE_READ_NORMALIZED_FLOAT);
  m_optixTexture->setMaxAnisotropy(1.f);
  m_optixTexture->setMipLevelCount(1);
  m_optixTexture->setArraySize(1);
  m_optixTexture->setBuffer(0, 0, m_buffer);
}

//-----------------------------------------------------------------------------
// Frame the camera to the bbox
//
void gltfCamera::frameBox(const nvmath::vec3f& min, const nvmath::vec3f& max)
{
  // The camera will look to the middle of the bbox
  m_center = nvmath::vec3f(max[1] + min[0]) / 2.0f;

  // Make sure the position of the camera is not on the center of the bbox
  if((m_eye.x == m_center.x) && (m_eye.z == m_center.z))
    m_eye.z = m_center.z + 10.0f;

  // Make the matrix to transform the corners of the bbox to camera space
  nvmath::mat4 mcam = nvmath::look_at(m_eye, m_center, nvmath::vec3f(0, 1, 0));

  // Find the distance from the center of the BBox needed to see all corners of the BBox
  float          max_dist = 0;
  nvmath::vec3f vbox     = nvmath::vec3f(max[1] - min[0]) / 2.0f;

  float aspect = 1.0f;

  for(int i = 0; i < 8; i++)
  {
    nvmath::vec4f vct(i & 1 ? vbox.x : -vbox.x, i & 2 ? vbox.y : -vbox.y, i & 4 ? vbox.z : -vbox.z, 0.0f);
    vct = mcam * vct;

    float l    = std::max(fabs(vct.x), fabs(vct.y) * aspect);
    float dist = vct.z + l * m_fov * 2.0f;
    max_dist   = std::max(max_dist, dist);
  }

  // Make sure the bbox won't be clipped by the near plane
  if(max_dist < 0.01f)
  {
    max_dist = 0.01f;
  }

  // Add a 1% so it does not touch the border of the frame
  max_dist *= 1.01f;

  // Place the position of the camera to 'max_dist' from the
  // center of the BBox, which is also 'm_center'
  nvmath::vec3f line_of_sight = nvmath::normalize(m_eye - m_center);

  m_eye = m_center + (line_of_sight * max_dist);
}

}  // namespace nvoptix
