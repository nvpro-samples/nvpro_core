/* Copyright (c) 2014-2019, NVIDIA CORPORATION. All rights reserved.
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

/**
  # namespace nvh::gltf

  These utilities are for loading glTF models in a
  canonical scene representation. From this representation
  you would create the appropriate 3D API resources (buffers
  and textures).
 
  Some part of the code comes from [Sascha Willems](https://www.saschawillems.de/)
  His code is licensed under the MIT license (MIT) (https://opensource.org/licenses/MIT)

  ~~~ C++
  // Typical Usage
  // Load the GLTF Scene using TinyGLTF
 
  tinygltf::Model    gltfModel;
  tinygltf::TinyGLTF gltfContext;
  fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warn, m_filename);
 
  // Fill the data in the gltfScene

  nvh::gltf::Scene   gltfScene;
  nvh::gltf::Scene::VertexData vertices;
  std::vector<uint32_t> indices;
  gltfScene.loadMaterials(gltfModel);
  vertices.attributes["NORMAL"]     = {0, 1, 0};  // Attributes we are interested in
  vertices.attributes["COLOR_0"]    = {1, 1, 1};
  vertices.attributes["TEXCOORD_0"] = {0, 0};
  gltfScene.loadMeshes(gltfModel, indices, vertices);
  gltfScene.loadNodes(gltfModel);
  gltfScene.computeSceneDimensions();

  // Todo in App:
  //   create buffers for vertices and indices
  //   create textures from images
  //   create descriptorSet for material textures and push constant for material values
  ~~~

*/

#pragma once


#include <string>
#include <unordered_map>
#include <vector>

#include <fileformats/tiny_gltf.h>
#include <nvmath/nvmath.h>


namespace nvh {

namespace gltf {
struct Node;

// This is the vertex structure to load information on the mesh
struct VertexData
{
  std::vector<nvmath::vec3f>                          position;
  std::unordered_map<std::string, std::vector<float>> attributes;
};


// Matrices of each Node pushed to VK buffer
struct NodeMatrices
{
  nvmath::mat4f world;
  nvmath::mat4f worldIT;
};

struct TextureIDX
{
  uint32_t index = ~0;

  TextureIDX() {}
  TextureIDX(uint32_t idx) { index = idx; }
  TextureIDX(int idx) { index = (uint32_t)idx; }

  bool isValid() const { return index != ~0; }

  operator bool() const { return isValid(); };
  operator uint32_t() const { return index; }
  operator size_t() const { return index; }
};

enum class AlphaMode
{
  eOpaque,
  eMask,
  eBlend
};

enum class PathType
{
  eTranslation,
  eRotation,
  eScale
};

enum class InterpolationType
{
  eLinear,
  eStep,
  eCubicSpline
};

//--------------------------------------------------------------------------------------------------
// glTF Material representation
//
struct Material
{
  // See: https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
  // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness
  struct PushC
  {
    nvmath::vec4f baseColorFactor{1.f, 1.f, 1.f, 1.f};  // pbrMetallicRoughness
    nvmath::vec3f emissiveFactor{0, 0, 0};              // pbrMetallicRoughness
    float         metallicFactor{1.0f};                 // pbrMetallicRoughness
    nvmath::vec3f specularFactor{1.f, 1.f, 1.f};        // pbrSpecularGlossiness
    float         roughnessFactor{1.0f};                // pbrMetallicRoughness
    int           alphaMode{0};                         //
    float         alphaCutoff{0.5f};                    //
    float         glossinessFactor{1.0f};               // pbrSpecularGlossiness
    int           shadingModel{0};                      // 0: metallic-roughness, 1: specular-glossiness
    int           doubleSided{0};
    int           pad0{0};
    int           pad1{0};
    int           pad2{0};
  };
  PushC      m_mat{};
  TextureIDX m_baseColorTexture{};
  TextureIDX m_metallicRoughnessTexture{};
  TextureIDX m_normalTexture{};
  TextureIDX m_occlusionTexture{};
  TextureIDX m_emissiveTexture{};
};


//--------------------------------------------------------------------------------------------------
// glTF Primitive representation
//
struct Primitive
{
  uint32_t m_firstIndex{0};
  uint32_t m_indexCount{0};
  uint32_t m_vertexOffset{0};
  uint32_t m_materialIndex{0};

  struct Dimensions
  {
    nvmath::vec3f min{nvmath::vec3f(FLT_MAX)};
    nvmath::vec3f max{nvmath::vec3f(-FLT_MAX)};
    nvmath::vec3f size{0.f};
    nvmath::vec3f center{0.f};
    float         radius{0};
  } m_dimensions;

  void setDimensions(const nvmath::vec3f& min, const nvmath::vec3f& max);

  Primitive() {}
  Primitive(uint32_t first, uint32_t count, uint32_t vOffset, uint32_t materialIndex)
      : m_firstIndex(first)
      , m_indexCount(count)
      , m_vertexOffset(vOffset)
      , m_materialIndex(materialIndex)
  {
  }
};


//--------------------------------------------------------------------------------------------------
// glTF Mesh representation
//
struct Mesh
{
  Mesh(const nvmath::mat4f& matrix) { m_uniformBlock.matrix = matrix; }

  // All primitives used by the mesh
  std::vector<Primitive> m_primitives;

  // Uniform block to push into buffer (currently ignored)
  struct UniformBlock
  {
    nvmath::mat4f matrix;
    nvmath::mat4f jointMatrix[64]{};
    float         jointcount{0};
  } m_uniformBlock;
};


//--------------------------------------------------------------------------------------------------
// glTF Skin representation
//
struct Skin
{
  std::string                m_name;
  Node*                      m_skeletonRoot{nullptr};
  std::vector<nvmath::mat4f> m_inverseBindMatrices;
  std::vector<Node*>         m_joints;
};

//--------------------------------------------------------------------------------------------------
// glTF Node representation
//
struct Node
{
  Node*              m_parent{nullptr};
  uint32_t           m_index{0};
  std::vector<Node*> m_children;
  nvmath::mat4f      m_matrix;
  std::string        m_name;
  uint32_t           m_mesh{~0u};
  Skin*              m_skin{nullptr};
  int32_t            m_skinIndex{-1};
  nvmath::vec3f      m_translation{0.f, 0.f, 0.f};
  nvmath::vec3f      m_scale{1.0f};
  nvmath::quatf      m_rotation = {0.f, 0.f, 0.f, 0.f};

  nvmath::mat4f localMatrix() const;
  nvmath::mat4f worldMatrix() const;

  ~Node()
  {
    for(auto& child : m_children)
    {
      delete child;
    }
  }
};

//--------------------------------------------------------------------------------------------------
//
// glTF animation channel
//
struct AnimationChannel
{
  PathType m_path{PathType::eTranslation};
  Node*    m_node{nullptr};
  uint32_t m_samplerIndex{0};
};

//--------------------------------------------------------------------------------------------------
// glTF animation sampler
//
struct AnimationSampler
{
  InterpolationType          m_interpolation{InterpolationType::eLinear};
  std::vector<float>         m_inputs;
  std::vector<nvmath::vec4f> m_outputsVec4;
};

//--------------------------------------------------------------------------------------------------
// glTF animation
//
struct Animation
{
  std::string                   m_name;
  std::vector<AnimationSampler> m_samplers;
  std::vector<AnimationChannel> m_channels;
  float                         m_start{std::numeric_limits<float>::max()};
  float                         m_end{std::numeric_limits<float>::min()};
};

//--------------------------------------------------------------------------------------------------
// Holds the entire scene and methods to load and c
//
struct Scene
{
  uint32_t               m_numTextures{0};
  std::vector<Node*>     m_nodes;
  std::vector<Node*>     m_linearNodes;
  std::vector<Skin*>     m_skins;
  std::vector<Material>  m_materials;
  std::vector<Animation> m_animations;
  std::vector<Mesh*>     m_linearMeshes;

  // Size of the scene
  struct Dimensions
  {
    nvmath::vec3f min = nvmath::vec3f(std::numeric_limits<float>::max());
    nvmath::vec3f max = nvmath::vec3f(std::numeric_limits<float>::min());
    nvmath::vec3f size{0.f};
    nvmath::vec3f center{0.f};
    float         radius{0};
  } m_dimensions;

  void destroy();

  void loadNode(Node* parentNode, const tinygltf::Node& tinyNode, uint32_t nodeIndex, const tinygltf::Model& tinyModel);
  void loadMeshes(const tinygltf::Model& tinyModel, std::vector<uint32_t>& indexBuffer, VertexData& vertexData);
  void loadNodes(const tinygltf::Model& gltfModel);
  void loadSkins(const tinygltf::Model& gltfModel);
  void loadMaterials(tinygltf::Model& gltfModel);
  void loadAnimations(const tinygltf::Model& gltfModel);
  void getNodeDimensions(const gltf::Node* node, nvmath::vec3f& min, nvmath::vec3f& max);
  void computeSceneDimensions();
  void updateAnimation(uint32_t index, float time);
  Node* nodeFromIndex(uint32_t index);
};

}  // namespace gltf
}  // namespace nvh
