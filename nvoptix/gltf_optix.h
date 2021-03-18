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

#pragma once

#include <array>
#include <string>

#include "optixu/optixpp_namespace.h"

#include <nvmath/nvmath.h>
#include <nvmath/nvmath_glsltypes.h>


// Forward declaration tinygltf
namespace tinygltf {
struct Image;
class Node;
class Model;
};  // namespace tinygltf

namespace nvoptix {


struct gltfAnimation;
struct gltfAnimationChannel;
struct gltfAnimationSampler;
struct gltfCamera;
struct gltfMaterial;
struct gltfMesh;
struct gltfNode;
struct gltfPrimitive;
struct gltfSkin;
struct gltfTexture;
struct gltfVertex;

struct gltfDimensions
{
  nvmath::vec3f min = nvmath::vec3f(FLT_MAX);
  nvmath::vec3f max = nvmath::vec3f(-FLT_MAX);
  nvmath::vec3f size{0.f};
  nvmath::vec3f center{0.f};
  float          radius = 0;

  void set(nvmath::vec3f _min, nvmath::vec3f _max)
  {
    min    = _min;
    max    = _max;
    size   = _max - _min;
    center = (_min + _max) / 2.0f;
    radius = nvmath::length(_min - _max) / 2.0f;
  }
};


//--------------------------------------------------------------------------------------------------
// Class to load and holding glTF
// Usage:
// - Load the scene using  loadFromFile
// - An OptiX group will be created containing all the nodes to render
// - Set the `closest hit` and `any hit` programs. All parameters will be set, but it is to the
//   application to provide the proper program.
//
class OptixGLtf
{
public:
  OptixGLtf()  = default;
  ~OptixGLtf() = default;

  // Load a glTF file into OptiX representation
  void loadFromFile(optix::Context context, const std::string& filename);

  // Retrieve the OptiX top group
  optix::Group getOptixGroup() { return m_optixTlas; }

  // Set the programs for all materials
  void setClosestHit(int rayType, const optix::Program& prog);
  void setAnyHit(int rayType, const optix::Program& prog);

  // Retrieve the bounding box of the scene
  const gltfDimensions& getDimensions() { return m_dimensions; }

  std::vector<gltfCamera*>& getCameras() { return m_cameras; }

private:
  void loadImages(tinygltf::Model& gltfModel);
  void loadMaterials(tinygltf::Model& gltfModel);
  void loadNode(gltfNode* parentNode, const tinygltf::Node& tinyNode, uint32_t nodeIndex, const tinygltf::Model& tinyModel);

  void loadMesh(const tinygltf::Model& tinyModel, const tinygltf::Node& tinyNode, gltfNode* newNode);

  void loadCamera(const tinygltf::Model& tinyModel, const tinygltf::Node& tinyNode, gltfNode* newNode);

  void getSceneDimensions();

  void getNodeDimensions(gltfNode* node, nvmath::vec3f& min, nvmath::vec3f& max);

  optix::Transform         createBlas(const optix::GeometryInstance& geoInstance, const nvmath::mat4& matrix);
  optix::GeometryTriangles createGeometryTriangle(optix::GeometryInstance            geoInstance,
                                                  const std::vector<gltfVertex>&     triVertices,
                                                  const std::vector<nvmath::uvec3>& triIndices);
  void computeNormals(std::vector<gltfVertex>& vertexBuffer, std::vector<nvmath::uvec3>& indexBuffer);


  std::vector<gltfNode*>   m_nodes;
  std::vector<gltfNode*>   m_linearNodes;
  std::vector<gltfSkin*>   m_skins;
  std::vector<gltfCamera*> m_cameras;


  std::vector<gltfTexture>   m_textures;
  std::vector<gltfMaterial>  m_materials;
  std::vector<gltfAnimation> m_animations;

  uint32_t       m_nbTriangles = 0;
  gltfDimensions m_dimensions;


  optix::Context m_context;
  optix::Group   m_optixTlas;
};


// glTF animation
struct gltfAnimation
{
  std::string                       m_name;
  std::vector<gltfAnimationSampler> m_samplers;
  std::vector<gltfAnimationChannel> m_channels;
  float                             start = std::numeric_limits<float>::max();
  float                             end   = std::numeric_limits<float>::min();
};

// glTF animation channel
struct gltfAnimationChannel
{
  enum PathType
  {
    TRANSLATION,
    ROTATION,
    SCALE
  };

  PathType  m_path;
  gltfNode* m_node         = nullptr;
  uint32_t  m_samplerIndex = 0;
};

// glTF animation sampler
struct gltfAnimationSampler
{
  enum InterpolationType
  {
    LINEAR,
    STEP,
    CUBICSPLINE
  };

  InterpolationType           m_interpolation = LINEAR;
  std::vector<float>          m_inputs;
  std::vector<nvmath::vec4f> m_outputsVec4;
};

struct gltfCamera
{
  gltfNode*      m_parent = nullptr;
  nvmath::vec3f m_eye{0, 1, 5};
  nvmath::vec3f m_center{0, 0, 0};
  float          m_fov         = deg2rad(60.f);
  bool           m_perspective = true;

  void frameBox(const nvmath::vec3f& min, const nvmath::vec3f& max);
  gltfCamera(gltfNode* node)
      : m_parent(node)
  {
  }
};


// glTF material class
struct gltfMaterial
{
  enum AlphaMode
  {
    ALPHAMODE_OPAQUE,
    ALPHAMODE_MASK,
    ALPHAMODE_BLEND
  };


  AlphaMode      m_alphaMode                = ALPHAMODE_OPAQUE;
  float          m_alphaCutoff              = 1.0f;
  float          m_metallicFactor           = 1.0f;
  float          m_roughnessFactor          = 1.0f;
  nvmath::vec4f m_baseColorFactor          = nvmath::vec4f(1.0f);
  gltfTexture*   m_baseColorTexture         = nullptr;
  gltfTexture*   m_metallicRoughnessTexture = nullptr;
  gltfTexture*   m_normalTexture            = nullptr;
  gltfTexture*   m_occlusionTexture         = nullptr;
  gltfTexture*   m_emissiveTexture          = nullptr;

  // PBR_WORKFLOW_SPECULAR_GLOSINESS
  nvmath::vec4f m_diffuseFactor         = nvmath::vec4f(1.0f);
  nvmath::vec3f m_specularFactor        = nvmath::vec3f(0.0f);
  bool           m_pbrSpecularGlossiness = false;

  optix::Material m_optixMat;

  void updateOptix(optix::Context context);
};

// glTF mesh
struct gltfMesh
{
  std::vector<gltfPrimitive*> m_primitives;

  struct UniformBlock
  {
    nvmath::mat4 matrix;
    nvmath::mat4 jointMatrix[64]{};
    float         jointcount{0};
  } m_uniformBlock;

  gltfMesh(nvmath::mat4 matrix)
  {
    m_uniformBlock.matrix = matrix;

    //         vkctx->createBuffer( sizeof( m_uniformBlock ), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    //                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //                              m_uniformBuffer.buffer, m_uniformBuffer.memory, &m_uniformBlock );
    //         VK_CHECK( vkMapMemory( vkctx->getDevice(), m_uniformBuffer.memory, 0, sizeof( m_uniformBlock ), 0,
    //                                &m_uniformBuffer.mapped ) );
    //         m_uniformBuffer.descriptor = {m_uniformBuffer.buffer, 0, sizeof( m_uniformBlock )};
  };

  ~gltfMesh() = default;
};

// glTF node
struct gltfNode
{
  gltfNode*              m_parent = nullptr;
  uint32_t               m_index  = 0;
  std::vector<gltfNode*> m_children;
  nvmath::mat4          m_matrix;
  std::string            m_name;
  gltfMesh*              m_mesh      = nullptr;
  gltfSkin*              m_skin      = nullptr;
  int32_t                m_skinIndex = -1;
  nvmath::vec3f         m_translation{0, 0, 0};
  nvmath::vec3f         m_scale{1.0f, 1.0f, 1.0f};
  nvmath::quatf         m_rotation{0, 0, 0, 1};

  nvmath::mat4 localMatrix();
  nvmath::mat4 getMatrix();

  void update();

  ~gltfNode()
  {
    delete m_mesh;
    for(auto& child : m_children)
      delete child;
  }
};

// glTF primitive
struct gltfPrimitive
{
  uint32_t      m_firstIndex = 0;
  uint32_t      m_indexCount = 0;
  gltfMaterial& m_material;

  std::vector<nvmath::uvec3> m_indexBuffer;
  std::vector<gltfVertex>     m_vertexBuffer;
  gltfDimensions              m_dimensions;

  optix::Transform m_optixBlas;

  gltfPrimitive(uint32_t firstIndex, uint32_t indexCount, gltfMaterial& material)
      : m_firstIndex(firstIndex)
      , m_indexCount(indexCount)
      , m_material(material){};
};

// glTF skin
struct gltfSkin
{
  std::string                m_name;
  gltfNode*                  m_skeletonRoot = nullptr;
  std::vector<nvmath::mat4> m_inverseBindMatrices;
  std::vector<gltfNode*>     m_joints;
};

// glTF texture loading class
struct gltfTexture
{
  uint32_t m_width      = 0;
  uint32_t m_height     = 0;
  uint32_t m_mipLevels  = 0;
  uint32_t m_layerCount = 0;

  optix::TextureSampler m_optixTexture;
  optix::Buffer         m_buffer;

  static optix::TextureSampler m_optixDefaultOne;

  // Load a texture from a glTF image (stored as vector of chars loaded via stb_image)
  void fromglTfImage(optix::Context context, tinygltf::Image& gltfimage);
};

struct gltfVertex
{
  nvmath::vec3f pos;
  nvmath::vec3f normal;
  nvmath::vec2f uv;
  nvmath::vec4f joint0;
  nvmath::vec4f weight0;
};

}  // namespace nvoptix
