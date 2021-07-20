/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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


/**
  \namespace nvh::gltf

  These utilities are for loading glTF models in a
  canonical scene representation. From this representation
  you would create the appropriate 3D API resources (buffers
  and textures).
 
  \code{.cpp}
  // Typical Usage
  // Load the GLTF Scene using TinyGLTF
 
  tinygltf::Model    gltfModel;
  tinygltf::TinyGLTF gltfContext;
  fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warn, m_filename);
 
  // Fill the data in the gltfScene
  gltfScene.getMaterials(tmodel);
  gltfScene.getDrawableNodes(tmodel, GltfAttributes::Normal | GltfAttributes::Texcoord_0);

  // Todo in App:
  //   create buffers for vertices and indices, from gltfScene.m_position, gltfScene.m_index
  //   create textures from images: using tinygltf directly
  //   create descriptorSet for material using directly gltfScene.m_materials
  \endcode

*/

#pragma once
#pragma once
#include "nvmath/nvmath.h"
#include "nvmath/nvmath_glsltypes.h"
#include "tiny_gltf.h"
#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

using namespace nvmath;
#define KHR_LIGHTS_PUNCTUAL_EXTENSION_NAME "KHR_lights_punctual"

namespace nvh {

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/README.md
#define KHR_MATERIALS_PBRSPECULARGLOSSINESS_EXTENSION_NAME "KHR_materials_pbrSpecularGlossiness"
struct KHR_materials_pbrSpecularGlossiness
{
  vec4  diffuseFactor{1.f, 1.f, 1.f, 1.f};
  int   diffuseTexture{-1};
  vec3  specularFactor{1.f, 1.f, 1.f};
  float glossinessFactor{1.f};
  int   specularGlossinessTexture{-1};
};

// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_texture_transform
#define KHR_TEXTURE_TRANSFORM_EXTENSION_NAME "KHR_texture_transform"
struct KHR_texture_transform
{
  vec2  offset{0.f, 0.f};
  float rotation{0.f};
  vec2  scale{1.f};
  int   texCoord{0};
  mat3  uvTransform{1};  // Computed transform of offset, rotation, scale
};


// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_clearcoat/README.md
#define KHR_MATERIALS_CLEARCOAT_EXTENSION_NAME "KHR_materials_clearcoat"
struct KHR_materials_clearcoat
{
  float factor{0.f};
  int   texture{-1};
  float roughnessFactor{0.f};
  int   roughnessTexture{-1};
  int   normalTexture{-1};
};

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_sheen/README.md
#define KHR_MATERIALS_SHEEN_EXTENSION_NAME "KHR_materials_sheen"
struct KHR_materials_sheen
{
  vec3  colorFactor{0.f, 0.f, 0.f};
  int   colorTexture{-1};
  float roughnessFactor{0.f};
  int   roughnessTexture{-1};
};

// https://github.com/DassaultSystemes-Technology/glTF/tree/KHR_materials_volume/extensions/2.0/Khronos/KHR_materials_transmission
#define KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME "KHR_materials_transmission"
struct KHR_materials_transmission
{
  float factor{0.f};
  int   texture{-1};
};

// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_unlit
#define KHR_MATERIALS_UNLIT_EXTENSION_NAME "KHR_materials_unlit"
struct KHR_materials_unlit
{
  int active{0};
};

// PBR Next : KHR_materials_anisotropy
#define KHR_MATERIALS_ANISOTROPY_EXTENSION_NAME "KHR_materials_anisotropy"
struct KHR_materials_anisotropy
{
  float factor{0.f};
  vec3  direction{1.f, 0.f, 0.f};
  int   texture{-1};
};


// https://github.com/DassaultSystemes-Technology/glTF/tree/KHR_materials_ior/extensions/2.0/Khronos/KHR_materials_ior
#define KHR_MATERIALS_IOR_EXTENSION_NAME "KHR_materials_ior"
struct KHR_materials_ior
{
  float ior{1.5f};
};

// https://github.com/DassaultSystemes-Technology/glTF/tree/KHR_materials_volume/extensions/2.0/Khronos/KHR_materials_volume
#define KHR_MATERIALS_VOLUME_EXTENSION_NAME "KHR_materials_volume"
struct KHR_materials_volume
{
  float thicknessFactor{0};
  int   thicknessTexture{-1};
  float attenuationDistance{std::numeric_limits<float>::max()};
  vec3  attenuationColor{1.f, 1.f, 1.f};
};

// https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#reference-material
struct GltfMaterial
{
  int shadingModel{0};  // 0: metallic-roughness, 1: specular-glossiness

  // pbrMetallicRoughness
  vec4  baseColorFactor{1.f, 1.f, 1.f, 1.f};
  int   baseColorTexture{-1};
  float metallicFactor{1.f};
  float roughnessFactor{1.f};
  int   metallicRoughnessTexture{-1};

  int   emissiveTexture{-1};
  vec3  emissiveFactor{0, 0, 0};
  int   alphaMode{0};
  float alphaCutoff{0.5f};
  int   doubleSided{0};

  int   normalTexture{-1};
  float normalTextureScale{1.f};
  int   occlusionTexture{-1};
  float occlusionTextureStrength{1};

  // Extensions
  KHR_materials_pbrSpecularGlossiness specularGlossiness;
  KHR_texture_transform               textureTransform;
  KHR_materials_clearcoat             clearcoat;
  KHR_materials_sheen                 sheen;
  KHR_materials_transmission          transmission;
  KHR_materials_unlit                 unlit;
  KHR_materials_anisotropy            anisotropy;
  KHR_materials_ior                   ior;
  KHR_materials_volume                volume;
};


struct GltfNode
{
  nvmath::mat4f worldMatrix{1};
  int           primMesh{0};
};

struct GltfPrimMesh
{
  uint32_t firstIndex{0};
  uint32_t indexCount{0};
  uint32_t vertexOffset{0};
  uint32_t vertexCount{0};
  int      materialIndex{0};

  nvmath::vec3f posMin{0, 0, 0};
  nvmath::vec3f posMax{0, 0, 0};
  std::string   name;
};

struct GltfStats
{
  uint32_t nbCameras{0};
  uint32_t nbImages{0};
  uint32_t nbTextures{0};
  uint32_t nbMaterials{0};
  uint32_t nbSamplers{0};
  uint32_t nbNodes{0};
  uint32_t nbMeshes{0};
  uint32_t nbLights{0};
  uint32_t imageMem{0};
  uint32_t nbUniqueTriangles{0};
  uint32_t nbTriangles{0};
};

struct GltfCamera
{
  nvmath::mat4f worldMatrix{1};
  nvmath::vec3f eye{0, 0, 0};
  nvmath::vec3f center{0, 0, 0};
  nvmath::vec3f up{0, 1, 0};

  tinygltf::Camera cam;
};

// See: https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md
struct GltfLight
{
  nvmath::mat4f   worldMatrix{1};
  tinygltf::Light light;
};


enum class GltfAttributes : uint8_t
{
  Position   = 0,
  Normal     = 1,
  Texcoord_0 = 2,
  Texcoord_1 = 4,
  Tangent    = 8,
  Color_0    = 16,
  //Joints_0   = 32, // #TODO - Add support for skinning
  //Weights_0  = 64,
};
using GltfAttributes_t = std::underlying_type_t<GltfAttributes>;

inline GltfAttributes operator|(GltfAttributes lhs, GltfAttributes rhs)
{
  return static_cast<GltfAttributes>(static_cast<GltfAttributes_t>(lhs) | static_cast<GltfAttributes_t>(rhs));
}

inline GltfAttributes operator&(GltfAttributes lhs, GltfAttributes rhs)
{
  return static_cast<GltfAttributes>(static_cast<GltfAttributes_t>(lhs) & static_cast<GltfAttributes_t>(rhs));
}

//--------------------------------------------------------------------------------------------------
// Class to convert gltfScene in simple draw-able format
//
struct GltfScene
{
  void importMaterials(const tinygltf::Model& tmodel);
  void importDrawableNodes(const tinygltf::Model& tmodel, GltfAttributes attributes);
  void computeSceneDimensions();
  void destroy();

  static GltfStats getStatistics(const tinygltf::Model& tinyModel);

  // Scene data
  std::vector<GltfMaterial> m_materials;   // Material for shading
  std::vector<GltfNode>     m_nodes;       // Drawable nodes, flat hierarchy
  std::vector<GltfPrimMesh> m_primMeshes;  // Primitive promoted to meshes
  std::vector<GltfCamera>   m_cameras;
  std::vector<GltfLight>    m_lights;

  // Attributes, all same length if valid
  std::vector<nvmath::vec3f> m_positions;
  std::vector<uint32_t>      m_indices;
  std::vector<nvmath::vec3f> m_normals;
  std::vector<nvmath::vec4f> m_tangents;
  std::vector<nvmath::vec2f> m_texcoords0;
  std::vector<nvmath::vec2f> m_texcoords1;
  std::vector<nvmath::vec4f> m_colors0;

  // #TODO - Adding support for Skinning
  //using vec4us = vector4<unsigned short>;
  //std::vector<vec4us>        m_joints0;
  //std::vector<nvmath::vec4f> m_weights0;

  // Size of the scene
  struct Dimensions
  {
    nvmath::vec3f min = nvmath::vec3f(std::numeric_limits<float>::max());
    nvmath::vec3f max = nvmath::vec3f(std::numeric_limits<float>::min());
    nvmath::vec3f size{0.f};
    nvmath::vec3f center{0.f};
    float         radius{0};
  } m_dimensions;


private:
  void processNode(const tinygltf::Model& tmodel, int& nodeIdx, const nvmath::mat4f& parentMatrix);
  void processMesh(const tinygltf::Model& tmodel, const tinygltf::Primitive& tmesh, GltfAttributes attributes, const std::string& name);

  // Temporary data
  std::unordered_map<int, std::vector<uint32_t>> m_meshToPrimMeshes;
  std::vector<uint32_t>                          primitiveIndices32u;
  std::vector<uint16_t>                          primitiveIndices16u;
  std::vector<uint8_t>                           primitiveIndices8u;

  std::unordered_map<std::string, GltfPrimMesh> m_cachePrimMesh;

  void computeCamera();
  void checkRequiredExtensions(const tinygltf::Model& tmodel);
};

nvmath::mat4f getLocalMatrix(const tinygltf::Node& tnode);

// Return a vector of data for a tinygltf::Value
template <typename T>
static inline std::vector<T> getVector(const tinygltf::Value& value)
{
  std::vector<T> result{0};
  if(!value.IsArray())
    return result;
  result.resize(value.ArrayLen());
  for(int i = 0; i < value.ArrayLen(); i++)
  {
    result[i] = static_cast<T>(value.Get(i).IsNumber() ? value.Get(i).Get<double>() : value.Get(i).Get<int>());
  }
  return result;
}

static inline void getFloat(const tinygltf::Value& value, const std::string& name, float& val)
{
  if(value.Has(name))
  {
    val = static_cast<float>(value.Get(name).Get<double>());
  }
}

static inline void getInt(const tinygltf::Value& value, const std::string& name, int& val)
{
  if(value.Has(name))
  {
    val = value.Get(name).Get<int>();
  }
}

static inline void getVec2(const tinygltf::Value& value, const std::string& name, vec2& val)
{
  if(value.Has(name))
  {
    auto s = getVector<float>(value.Get(name));
    val    = vec2{s[0], s[1]};
  }
}

static inline void getVec3(const tinygltf::Value& value, const std::string& name, vec3& val)
{
  if(value.Has(name))
  {
    auto s = getVector<float>(value.Get(name));
    val    = vec3{s[0], s[1], s[2]};
  }
}

static inline void getVec4(const tinygltf::Value& value, const std::string& name, vec4& val)
{
  if(value.Has(name))
  {
    auto s = getVector<float>(value.Get(name));
    val    = vec4{s[0], s[1], s[2], s[3]};
  }
}

static inline void getTexId(const tinygltf::Value& value, const std::string& name, int& val)
{
  if(value.Has(name))
  {
    val = value.Get(name).Get("index").Get<int>();
  }
}

// Appending to \p attribVec, all the values of \p attribName
// Return false if the attribute is missing
template <typename T>
static bool getAttribute(const tinygltf::Model& tmodel, const tinygltf::Primitive& primitive, std::vector<T>& attribVec, const std::string& attribName)
{
  if(primitive.attributes.find(attribName) == primitive.attributes.end())
    return false;

  // Retrieving the data of the attribute
  const auto& accessor = tmodel.accessors[primitive.attributes.find(attribName)->second];
  const auto& bufView  = tmodel.bufferViews[accessor.bufferView];
  const auto& buffer   = tmodel.buffers[bufView.buffer];
  const auto  bufData  = reinterpret_cast<const T*>(&(buffer.data[accessor.byteOffset + bufView.byteOffset]));
  const auto  nbElems  = accessor.count;

  // Supporting KHR_mesh_quantization
  assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

  // Copying the attributes
  if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
  {
    if(bufView.byteStride == 0)
    {
      attribVec.insert(attribVec.end(), bufData, bufData + nbElems);
    }
    else
    {
      // With stride, need to add one by one the element
      auto bufferByte = reinterpret_cast<const uint8_t*>(bufData);
      for(size_t i = 0; i < nbElems; i++)
      {
        attribVec.push_back(*reinterpret_cast<const T*>(bufferByte));
        bufferByte += bufView.byteStride;
      }
    }
  }
  else
  {
    // The component is smaller than float and need to be converted

    // VEC3 or VEC4
    int nbComponents = accessor.type == TINYGLTF_TYPE_VEC2 ? 2 : (accessor.type == TINYGLTF_TYPE_VEC3) ? 3 : 4;
    // UNSIGNED_BYTE or UNSIGNED_SHORT
    size_t strideComponent = accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ? 1 : 2;

    size_t byteStride = bufView.byteStride > 0 ? bufView.byteStride : size_t(nbComponents) * strideComponent;
    auto   bufferByte = reinterpret_cast<const uint8_t*>(bufData);
    for(size_t i = 0; i < nbElems; i++)
    {
      T vecValue;

      auto bufferByteData = bufferByte;
      for(int c = 0; c < nbComponents; c++)
      {
        float value = *reinterpret_cast<const float*>(bufferByteData);
        switch(accessor.componentType)
        {
          case TINYGLTF_COMPONENT_TYPE_BYTE:
            vecValue[c] = std::max(value / 127.f, -1.f);
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            vecValue[c] = value / 255.f;
            break;
          case TINYGLTF_COMPONENT_TYPE_SHORT:
            vecValue[c] = std::max(value / 32767.f, -1.f);
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            vecValue[c] = value / 65535.f;
            break;
          default:
            assert(!"KHR_mesh_quantization unsupported format");
            break;
        }
        bufferByteData += strideComponent;
      }
      bufferByte += byteStride;
      attribVec.push_back(vecValue);
    }
  }


  return true;
}

inline bool hasExtension(const tinygltf::ExtensionMap& extensions, const std::string& name)
{
  return extensions.find(name) != extensions.end();
}


}  // namespace nvh
