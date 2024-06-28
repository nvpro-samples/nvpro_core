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


#pragma once
#include <glm/glm.hpp>
#include "tiny_gltf.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string.h>
#include <unordered_map>
#include <vector>
#include "fileformats/tinygltf_utils.hpp"
#include "boundingbox.hpp"

#define KHR_LIGHTS_PUNCTUAL_EXTENSION_NAME "KHR_lights_punctual"

namespace nvh {
namespace gltf {

// The render node is the instance of a primitive in the scene that will be rendered
struct RenderNode
{
  glm::mat4 worldMatrix  = glm::mat4(1.0f);
  int       materialID   = 0;   // Reference to the material
  int       renderPrimID = -1;  // Reference to the unique primitive
  int       primID       = -1;  // Reference to the tinygltf::Primitive
  int       refNodeID    = -1;  // Reference to the tinygltf::Node
};

// The RenderPrimitive is a unique primitive in the scene
struct RenderPrimitive
{
  tinygltf::Primitive primitive;
  int                 vertexCount = 0;
  int                 indexCount  = 0;
};

struct RenderCamera
{
  enum CameraType
  {
    ePerspective,
    eOrthographic
  };

  CameraType type   = ePerspective;
  glm::vec3  eye    = {0.0f, 0.0f, 0.0f};
  glm::vec3  center = {0.0f, 0.0f, 0.0f};
  glm::vec3  up     = {0.0f, 1.0f, 0.0f};

  // Perspective
  double yfov = {0.0};  // in radians

  // Orthographic
  double xmag = {0.0};
  double ymag = {0.0};

  double znear = {0.0};
  double zfar  = {0.0};
};

// See: https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md
struct RenderLight
{
  glm::mat4       worldMatrix = glm::mat4(1.0f);
  tinygltf::Light light;
};


/** @DOC_START
* 
* # nvh::gltf::Scene 
* 
* The Scene class is responsible for loading and managing a glTF scene.
* - It is used to load a glTF file and parse it into a scene representation.
* - It can be used to save the scene back to a glTF file.
* - It can be used to manage the animations of the scene.
* - What it returns is a list of RenderNodes, RenderPrimitives, RenderCameras, and RenderLights.
* -  RenderNodes are the instances of the primitives in the scene that will be rendered.
* -  RenderPrimitives are the unique primitives in the scene.
* 
* Note: The Scene class is a more advanced and light weight version of the GltfScene class.
*       But it is to the user to retrieve the primitive data from the RenderPrimitives.
*       Check the tinygltf_utils.hpp for more information on how to extract the primitive data.
* 
** @DOC_END */


class Scene
{
public:
  // Used to specify the type of pipeline to be used
  enum PipelineType
  {
    eRasterSolid,
    eRasterSolidDoubleSided,
    eRasterBlend,
    eRasterAll
  };

  // File Management
  bool               load(const std::string& filename);  // Load the glTF file, .gltf or .glb
  bool               save(const std::string& filename);  // Save the glTF file, .gltf or .glb
  const std::string& getFilename() const { return m_filename; }
  void               takeModel(tinygltf::Model&& model);  // Use a model that has been loaded

  // Getters
  const tinygltf::Model& getModel() const { return m_model; }
  tinygltf::Model&       getModel() { return m_model; }
  bool                   valid() const { return !m_renderNodes.empty(); }

  // Animation Management
  void updateRenderNodes();  // Update the render nodes matrices and materials
  bool updateAnimations(float deltaTime);
  bool updateAnimation(uint32_t animationIndex, float deltaTime, bool reset = false);
  bool resetAnimations();  // Reset the animations
  int  getNumAnimations() const { return static_cast<int>(m_animations.size()); }
  bool hasAnimation() const { return !m_animations.empty(); }

  // Resource Management
  void destroy();  // Destroy the loaded resources

  // Light Management
  const std::vector<gltf::RenderLight>& getRenderLights(bool force);

  // Camera Management
  const std::vector<gltf::RenderCamera>& getRenderCameras(bool force = false);
  void                                   setSceneCamera(const gltf::RenderCamera& camera);

  // Render Node Management
  const std::vector<gltf::RenderNode>& getRenderNodes() const { return m_renderNodes; }

  // Render Primitive Management
  const std::vector<gltf::RenderPrimitive>& getRenderPrimitives() const { return m_renderPrimitives; }
  const gltf::RenderPrimitive&              getRenderPrimitive(size_t ID) const { return m_renderPrimitives[ID]; }
  size_t                                    getNumRenderPrimitives() const { return m_renderPrimitives.size(); }

  // Scene Management
  void           setCurrentScene(int sceneID);  // Parse the scene and create the render nodes, call when changing scene
  int            getCurrentScene() const { return m_currentScene; }
  tinygltf::Node getSceneRootNode() const;
  void           setSceneRootNode(const tinygltf::Node& node);

  // Variant Management
  void                            setCurrentVariant(int variant);  // Set the variant to be used
  const std::vector<std::string>& getVariants() const { return m_variants; }
  int                             getCurrentVariant() const { return m_currentVariant; }

  // Shading Management
  std::vector<uint32_t> getShadedNodes(PipelineType type);  // Get the nodes that will be shaded by the pipeline type

  // Statistics
  int       getNumTriangles() const { return m_numTriangles; }
  nvh::Bbox getSceneBounds();


private:
  struct AnimationChannel
  {
    enum PathType
    {
      eTranslation,
      eRotation,
      eScale,
      eWeights
    };
    PathType path         = eTranslation;
    int      node         = -1;
    uint32_t samplerIndex = 0;
  };

  struct AnimationSampler
  {
    enum InterpolationType
    {
      eLinear,
      eStep,
      eCubicSpline
    };
    InterpolationType               interpolation = eLinear;
    std::vector<float>              inputs;
    std::vector<glm::vec4>          outputsVec4;
    std::vector<std::vector<float>> outputsFloat;
  };

  struct Animation
  {
    float                         start       = std::numeric_limits<float>::max();
    float                         end         = std::numeric_limits<float>::min();
    float                         currentTime = 0.0f;
    std::vector<AnimationSampler> samplers;
    std::vector<AnimationChannel> channels;
  };


  void parseScene();                    // Parse the scene and create the render nodes
  void clearParsedData();               // Clear the parsed data
  void parseAnimations();               // Parse the animations
  void parseVariants();                 // Parse the variants
  void setSceneElementsDefaultNames();  // Set a default name for the scene elements
  void createSceneCamera();             // Create a camera for the scene
  void createRootIfMultipleNodes(tinygltf::Scene& scene);

  int getUniqueRenderPrimitive(const tinygltf::Primitive& primitive);
  int getMaterialVariantIndex(const tinygltf::Primitive& primitive, int currentVariant);

  bool   handleRenderNode(int nodeID, glm::mat4 worldMatrix);
  size_t handleGpuInstancing(const tinygltf::Value& attributes, gltf::RenderNode renderNode, glm::mat4 worldMatrix);
  bool   handleCameraTraversal(int nodeID, const glm::mat4& worldMatrix);

  tinygltf::Model                      m_model;                 // The glTF model
  std::string                          m_filename;              // Filename of the glTF
  std::vector<gltf::RenderNode>        m_renderNodes;           // Render nodes
  std::vector<gltf::RenderPrimitive>   m_renderPrimitives;      // Unique primitives from key
  std::vector<gltf::RenderCamera>      m_cameras;               // Cameras
  std::vector<gltf::RenderLight>       m_lights;                // Lights
  std::vector<Animation>               m_animations;            // Animations
  std::vector<std::string>             m_variants;              // KHR_materials_variants
  std::unordered_map<std::string, int> m_uniquePrimitiveIndex;  // Key: primitive, Value: renderPrimID
  int                                  m_numTriangles    = 0;   // Stat - Number of triangles
  int                                  m_currentScene    = 0;   // Scene index
  int                                  m_currentVariant  = 0;   // Variant index
  int                                  m_sceneRootNode   = -1;  // Node index of the root
  int                                  m_sceneCameraNode = -1;  // Node index of the camera
  nvh::Bbox                            m_sceneBounds;           // Scene bounds
};

}  // namespace gltf


// ***********************************************************************************************************************
// DEPRICATED SECTION
// ***********************************************************************************************************************

/** @DOC_START
# `nvh::GltfScene` **DEPRECATED**

  These utilities are for loading glTF models in a
  canonical scene representation. From this representation
  you would create the appropriate 3D API resources (buffers
  and textures).
 
  ```cpp
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
  ```

@DOC_END */


// https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#reference-material
struct GltfMaterial
{
  // pbrMetallicRoughness
  glm::vec4 baseColorFactor{1.f, 1.f, 1.f, 1.f};
  int       baseColorTexture{-1};
  float     metallicFactor{1.f};
  float     roughnessFactor{1.f};
  int       metallicRoughnessTexture{-1};

  int       emissiveTexture{-1};
  glm::vec3 emissiveFactor{0, 0, 0};
  int       alphaMode{0};
  float     alphaCutoff{0.5f};
  int       doubleSided{0};

  int   normalTexture{-1};
  float normalTextureScale{1.f};
  int   occlusionTexture{-1};
  float occlusionTextureStrength{1};

  // Extensions
  KHR_materials_specular          specular;
  KHR_texture_transform           textureTransform;
  KHR_materials_clearcoat         clearcoat;
  KHR_materials_sheen             sheen;
  KHR_materials_transmission      transmission;
  KHR_materials_unlit             unlit;
  KHR_materials_anisotropy        anisotropy;
  KHR_materials_ior               ior;
  KHR_materials_volume            volume;
  KHR_materials_displacement      displacement;
  KHR_materials_emissive_strength emissiveStrength;

  // Tiny Reference
  const tinygltf::Material* tmaterial{nullptr};
};

// Similar to nvh::gltf::RenderNode but will not support instancing
struct GltfNode
{
  glm::mat4             worldMatrix{1};
  int                   primMesh{0};
  const tinygltf::Node* tnode{nullptr};
};

// Similar to nvh::gltf::RenderPrimitive but refer duplicated geometry
// and may appears multiple times in the scene, where as RenderPrimitive is unique
struct GltfPrimMesh
{
  uint32_t firstIndex{0};
  uint32_t indexCount{0};
  uint32_t vertexOffset{0};
  uint32_t vertexCount{0};
  int      materialIndex{0};

  glm::vec3   posMin{0, 0, 0};
  glm::vec3   posMax{0, 0, 0};
  std::string name;

  // Tiny Reference
  const tinygltf::Mesh*      tmesh{nullptr};
  const tinygltf::Primitive* tprim{nullptr};
};

// This information could be extracted directly form the tinygltf::Model
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

// Similar to nvh::gltf::RenderCamera but is missing type, fov, znear, zfar, and xmag, ymag
struct GltfCamera
{
  glm::mat4 worldMatrix{1};
  glm::vec3 eye{0, 0, 0};
  glm::vec3 center{0, 0, 0};
  glm::vec3 up{0, 1, 0};

  tinygltf::Camera cam;
};

// Similar to nvh::gltf::RenderLight
// See: https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md
struct GltfLight
{
  glm::mat4       worldMatrix{1};
  tinygltf::Light light;
};

// Attributes that can be requested
enum class GltfAttributes : uint8_t
{
  NoAttribs  = 0,
  Position   = 1,
  Normal     = 2,
  Texcoord_0 = 4,
  Texcoord_1 = 8,
  Tangent    = 16,
  Color_0    = 32,
  All        = 0xFF
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
// - This is deprecated and should be replaced by nvh::gltf::Scene
// - This version copies all the data in a flat structure
// - Create the data when it does not exist, making it easy to use, but slow and memory consuming
//--------------------------------------------------------------------------------------------------
struct GltfScene
{
  // Importing all materials in a vector of GltfMaterial structure
  void importMaterials(const tinygltf::Model& tmodel);

  // Import all Mesh and primitives in a vector of GltfPrimMesh,
  // - Reads all requested GltfAttributes and create them if `forceRequested` contains it.
  // - Create a vector of GltfNode, GltfLight and GltfCamera
  void importDrawableNodes(const tinygltf::Model& tmodel,
                           GltfAttributes         requestedAttributes,
                           GltfAttributes         forceRequested = GltfAttributes::All);

  void exportDrawableNodes(tinygltf::Model& tmodel, GltfAttributes requestedAttributes);

  // Compute the scene bounding box
  void computeSceneDimensions();

  // Removes everything
  void destroy();

  static GltfStats getStatistics(const tinygltf::Model& tinyModel);

  // Scene data
  std::vector<GltfMaterial> m_materials;   // Material for shading
  std::vector<GltfNode>     m_nodes;       // Drawable nodes, flat hierarchy
  std::vector<GltfPrimMesh> m_primMeshes;  // Primitive promoted to meshes
  std::vector<GltfCamera>   m_cameras;
  std::vector<GltfLight>    m_lights;

  // Attributes, all same length if valid
  std::vector<glm::vec3> m_positions;
  std::vector<uint32_t>  m_indices;
  std::vector<glm::vec3> m_normals;
  std::vector<glm::vec4> m_tangents;
  std::vector<glm::vec2> m_texcoords0;
  std::vector<glm::vec2> m_texcoords1;
  std::vector<glm::vec4> m_colors0;

  // #TODO - Adding support for Skinning
  //using vec4us = vector4<unsigned short>;
  //std::vector<vec4us>        m_joints0;
  //std::vector<glm::vec4> m_weights0;

  // Size of the scene
  struct Dimensions
  {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::min());
    glm::vec3 size{0.f};
    glm::vec3 center{0.f};
    float     radius{0};
  } m_dimensions;


private:
  void processNode(const tinygltf::Model& tmodel, int& nodeIdx, const glm::mat4& parentMatrix);
  void processMesh(const tinygltf::Model&     tmodel,
                   const tinygltf::Primitive& tmesh,
                   GltfAttributes             requestedAttributes,
                   GltfAttributes             forceRequested,
                   const std::string&         name);

  void createNormals(GltfPrimMesh& resultMesh);
  void createTexcoords(GltfPrimMesh& resultMesh);
  void createTangents(GltfPrimMesh& resultMesh);
  void createColors(GltfPrimMesh& resultMesh);

  // Temporary data
  std::unordered_map<int, std::vector<uint32_t>> m_meshToPrimMeshes;
  std::vector<uint32_t>                          primitiveIndices32u;
  std::vector<uint16_t>                          primitiveIndices16u;
  std::vector<uint8_t>                           primitiveIndices8u;

  std::unordered_map<std::string, GltfPrimMesh> m_cachePrimMesh;

  void computeCamera();
  void checkRequiredExtensions(const tinygltf::Model& tmodel);
  void findUsedMeshes(const tinygltf::Model& tmodel, std::set<uint32_t>& usedMeshes, int nodeIdx);
};

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

}  // namespace nvh
