/*
 * Copyright (c) 2022-2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2024, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gltf_scene_vk.hpp"

#include <inttypes.h>
#include <mutex>
#include <sstream>
#include "stb_image.h"

#include "fileformats/texture_formats.h"
#include "fileformats/nv_dds.h"
#include "fileformats/tinygltf_utils.hpp"
#include "nvh/gltfscene.hpp"
#include "nvh/parallel_work.hpp"
#include "nvh/timesampler.hpp"
#include "nvvk/buffers_vk.hpp"
#include "nvvk/images_vk.hpp"
#include "shaders/dh_scn_desc.h"
#include "shaders/dh_lighting.h"

//--------------------------------------------------------------------------------------------------
// Forward declaration
std::vector<nvvkhl_shaders::Light> getShaderLights(const std::vector<nvh::gltf::RenderLight>& rlights,
                                                   const std::vector<tinygltf::Light>&        gltfLights);

//-------------------------------------------------------------------------------------------------
//
//
nvvkhl::SceneVk::SceneVk(VkDevice device, VkPhysicalDevice physicalDevice, nvvk::ResourceAllocator* alloc)
    : m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_alloc(alloc)
{
  m_dutil = std::make_unique<nvvk::DebugUtil>(m_device);  // Debug utility
}

//--------------------------------------------------------------------------------------------------
// Create all Vulkan resources to hold a nvh::gltf::Scene
//
void nvvkhl::SceneVk::create(VkCommandBuffer cmd, const nvh::gltf::Scene& scn)
{
  nvh::ScopedTimer st(__FUNCTION__);
  destroy();  // Make sure not to leave allocated buffers

  namespace fs     = std::filesystem;
  fs::path basedir = fs::path(scn.getFilename()).parent_path();
  createMaterialBuffer(cmd, scn.getModel().materials);
  createRenderNodeBuffer(cmd, scn);
  createVertexBuffers(cmd, scn);
  createTextureImages(cmd, scn.getModel(), basedir);
  createLightBuffer(cmd, scn);

  // Buffer references
  nvvkhl_shaders::SceneDescription scene_desc{};
  scene_desc.materialAddress        = m_bMaterial.address;
  scene_desc.renderPrimitiveAddress = m_bRenderPrim.address;
  scene_desc.renderNodeAddress      = m_bRenderNode.address;
  scene_desc.lightAddress           = m_bLights.address;
  scene_desc.numLights              = static_cast<uint32_t>(scn.getRenderLights().size());
  m_bSceneDesc                      = m_alloc->createBuffer(cmd, sizeof(nvvkhl_shaders::SceneDescription), &scene_desc,
                                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  m_dutil->DBG_NAME(m_bSceneDesc.buffer);
}


void nvvkhl::SceneVk::update(VkCommandBuffer cmd, const nvh::gltf::Scene& scn)
{
  updateMaterialBuffer(cmd, scn);
  updateRenderNodeBuffer(cmd, scn);
}

template <typename T>
inline nvvkhl_shaders::GltfTextureInfo getTextureInfo(const T& tinfo)
{
  const auto& transform = tinygltf::utils::getTextureTransform(tinfo);
  const int   texCoord  = std::min(tinfo.texCoord, 1);  // Only 2 texture coordinates

  return nvvkhl_shaders::GltfTextureInfo{
      .uvTransform = transform.uvTransform,
      .index       = tinfo.index,
      .texCoord    = texCoord,
  };
}

static nvvkhl_shaders::GltfShadeMaterial getShaderMaterial(const tinygltf::Material& srcMat)
{
  nvvkhl_shaders::GltfShadeMaterial dstMat = nvvkhl_shaders::defaultGltfMaterial();
  if(!srcMat.emissiveFactor.empty())
    dstMat.emissiveFactor = glm::make_vec3<double>(srcMat.emissiveFactor.data());
  dstMat.emissiveTexture             = getTextureInfo(srcMat.emissiveTexture);
  dstMat.normalTexture               = getTextureInfo(srcMat.normalTexture);
  dstMat.normalTextureScale          = static_cast<float>(srcMat.normalTexture.scale);
  dstMat.pbrBaseColorFactor          = glm::make_vec4<double>(srcMat.pbrMetallicRoughness.baseColorFactor.data());
  dstMat.pbrBaseColorTexture         = getTextureInfo(srcMat.pbrMetallicRoughness.baseColorTexture);
  dstMat.pbrMetallicFactor           = static_cast<float>(srcMat.pbrMetallicRoughness.metallicFactor);
  dstMat.pbrMetallicRoughnessTexture = getTextureInfo(srcMat.pbrMetallicRoughness.metallicRoughnessTexture);
  dstMat.pbrRoughnessFactor          = static_cast<float>(srcMat.pbrMetallicRoughness.roughnessFactor);
  dstMat.alphaMode   = srcMat.alphaMode == "OPAQUE" ? 0 : (srcMat.alphaMode == "MASK" ? 1 : 2 /*BLEND*/);
  dstMat.alphaCutoff = static_cast<float>(srcMat.alphaCutoff);

  KHR_materials_transmission transmission = tinygltf::utils::getTransmission(srcMat);
  dstMat.transmissionFactor               = transmission.factor;
  dstMat.transmissionTexture              = getTextureInfo(transmission.texture);

  KHR_materials_ior ior = tinygltf::utils::getIor(srcMat);
  dstMat.ior            = ior.ior;

  KHR_materials_volume volume = tinygltf::utils::getVolume(srcMat);
  dstMat.attenuationColor     = volume.attenuationColor;
  dstMat.thicknessFactor      = volume.thicknessFactor;
  dstMat.thicknessTexture     = getTextureInfo(volume.thicknessTexture);
  dstMat.attenuationDistance  = volume.attenuationDistance;

  KHR_materials_clearcoat clearcoat = tinygltf::utils::getClearcoat(srcMat);
  dstMat.clearcoatFactor            = clearcoat.factor;
  dstMat.clearcoatRoughness         = clearcoat.roughnessFactor;
  dstMat.clearcoatRoughnessTexture  = getTextureInfo(clearcoat.roughnessTexture);
  dstMat.clearcoatTexture           = getTextureInfo(clearcoat.texture);
  dstMat.clearcoatNormalTexture     = getTextureInfo(clearcoat.normalTexture);

  KHR_materials_specular specular = tinygltf::utils::getSpecular(srcMat);
  dstMat.specularFactor           = specular.specularFactor;
  dstMat.specularTexture          = getTextureInfo(specular.specularTexture);
  dstMat.specularColorFactor      = specular.specularColorFactor;
  dstMat.specularColorTexture     = getTextureInfo(specular.specularColorTexture);

  KHR_materials_emissive_strength emissiveStrength = tinygltf::utils::getEmissiveStrength(srcMat);
  dstMat.emissiveFactor *= emissiveStrength.emissiveStrength;

  KHR_materials_unlit unlit = tinygltf::utils::getUnlit(srcMat);
  dstMat.unlit              = unlit.active ? 1 : 0;

  KHR_materials_iridescence iridescence = tinygltf::utils::getIridescence(srcMat);
  dstMat.iridescenceFactor              = iridescence.iridescenceFactor;
  dstMat.iridescenceTexture             = getTextureInfo(iridescence.iridescenceTexture);
  dstMat.iridescenceIor                 = iridescence.iridescenceIor;
  dstMat.iridescenceThicknessMaximum    = iridescence.iridescenceThicknessMaximum;
  dstMat.iridescenceThicknessMinimum    = iridescence.iridescenceThicknessMinimum;
  dstMat.iridescenceThicknessTexture    = getTextureInfo(iridescence.iridescenceThicknessTexture);

  KHR_materials_anisotropy anisotropy = tinygltf::utils::getAnisotropy(srcMat);
  dstMat.anisotropyRotation = glm::vec2(glm::sin(anisotropy.anisotropyRotation), glm::cos(anisotropy.anisotropyRotation));
  dstMat.anisotropyStrength = anisotropy.anisotropyStrength;
  dstMat.anisotropyTexture  = getTextureInfo(anisotropy.anisotropyTexture);

  KHR_materials_sheen sheen    = tinygltf::utils::getSheen(srcMat);
  dstMat.sheenColorFactor      = sheen.sheenColorFactor;
  dstMat.sheenColorTexture     = getTextureInfo(sheen.sheenColorTexture);
  dstMat.sheenRoughnessFactor  = sheen.sheenRoughnessFactor;
  dstMat.sheenRoughnessTexture = getTextureInfo(sheen.sheenRoughnessTexture);

  return dstMat;
}

//--------------------------------------------------------------------------------------------------
// Create a buffer of all materials, with only the elements we need
//
void nvvkhl::SceneVk::createMaterialBuffer(VkCommandBuffer cmd, const std::vector<tinygltf::Material>& materials)
{
  nvh::ScopedTimer st(__FUNCTION__);

  using namespace tinygltf;

  std::vector<nvvkhl_shaders::GltfShadeMaterial> shade_materials;
  shade_materials.reserve(materials.size());
  for(const auto& srcMat : materials)
  {
    shade_materials.emplace_back(getShaderMaterial(srcMat));
  }

  m_bMaterial = m_alloc->createBuffer(cmd, shade_materials,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  m_dutil->DBG_NAME(m_bMaterial.buffer);
}

void nvvkhl::SceneVk::updateMaterialBuffer(VkCommandBuffer cmd, const nvh::gltf::Scene& scn)
{
  const std::vector<tinygltf::Material>& materials = scn.getModel().materials;

  std::vector<nvvkhl_shaders::GltfShadeMaterial> shade_materials;
  shade_materials.reserve(materials.size());
  for(const auto& srcMat : materials)
  {
    shade_materials.emplace_back(getShaderMaterial(srcMat));
  }
  m_alloc->getStaging()->cmdToBuffer(cmd, m_bMaterial.buffer, 0, shade_materials.size() * sizeof(nvvkhl_shaders::GltfShadeMaterial),
                                     shade_materials.data());
}


//--------------------------------------------------------------------------------------------------
// Array of instance information
// - Use by the vertex shader to retrieve the position of the instance
void nvvkhl::SceneVk::createRenderNodeBuffer(VkCommandBuffer cmd, const nvh::gltf::Scene& scn)
{
  nvh::ScopedTimer st(__FUNCTION__);

  std::vector<nvvkhl_shaders::RenderNode> inst_info;
  for(const auto& obj : scn.getRenderNodes())
  {
    nvvkhl_shaders::RenderNode info{};
    info.objectToWorld = obj.worldMatrix;
    info.worldToObject = glm::inverse(obj.worldMatrix);
    info.materialID    = obj.materialID;
    info.renderPrimID  = obj.renderPrimID;
    inst_info.emplace_back(info);
  }
  m_bRenderNode = m_alloc->createBuffer(cmd, inst_info, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  m_dutil->DBG_NAME(m_bRenderNode.buffer);
}

void nvvkhl::SceneVk::updateRenderNodeBuffer(VkCommandBuffer cmd, const nvh::gltf::Scene& scn)
{
  // nvh::ScopedTimer st(__FUNCTION__);

  std::vector<nvvkhl_shaders::RenderNode> inst_info;
  inst_info.reserve(scn.getRenderNodes().size());
  for(const auto& obj : scn.getRenderNodes())
  {
    nvvkhl_shaders::RenderNode info{};
    info.objectToWorld = obj.worldMatrix;
    info.worldToObject = glm::inverse(obj.worldMatrix);
    info.materialID    = obj.materialID;
    info.renderPrimID  = obj.renderPrimID;
    inst_info.emplace_back(info);
  }
  m_alloc->getStaging()->cmdToBuffer(cmd, m_bRenderNode.buffer, 0,
                                     inst_info.size() * sizeof(nvvkhl_shaders::RenderNode), inst_info.data());

  std::vector<nvvkhl_shaders::Light> shaderLights = getShaderLights(scn.getRenderLights(), scn.getModel().lights);
  if(shaderLights.size() > 0)
  {
    m_alloc->getStaging()->cmdToBuffer(cmd, m_bLights.buffer, 0, shaderLights.size() * sizeof(nvvkhl_shaders::Light),
                                       shaderLights.data());
  }
}


// Function to create attribute buffers in Vulkan only if the attribute is present
template <typename T>
void createAttributeBuffer(const std::string&         attributeName,  // Name of the attribute: POSITION, NORMAL, ...
                           const tinygltf::Model&     model,          // GLTF model
                           const tinygltf::Primitive& primitive,      // GLTF primitive
                           VkCommandBuffer            cmd,            // Command buffer to record the copy
                           nvvk::ResourceAllocator*   alloc,          // Allocator to create the buffer
                           int                        usageFlag,      // Usage of the buffer
                           nvvk::Buffer&              attributeBuffer)             // Buffer to be created
{
  if(primitive.attributes.find(attributeName) != primitive.attributes.end())
  {
    const tinygltf::Accessor&   accessor = model.accessors[primitive.attributes.at(attributeName)];
    const tinygltf::BufferView& view     = model.bufferViews[accessor.bufferView];

    // The most common case is that the buffer is directly readable as T
    if((view.byteStride == 0 || view.byteStride == sizeof(T)) && !accessor.sparse.isSparse)
    {
      const float* bufferData =
          reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
      attributeBuffer = alloc->createBuffer(cmd, sizeof(T) * accessor.count, bufferData, usageFlag);
    }
    else
    {
      // Get accessor data will make a copy of the data, the way we need it
      std::vector<T> tempData;
      tinygltf::utils::getAccessorData(model, accessor, tempData);
      attributeBuffer = alloc->createBuffer(cmd, tempData, usageFlag);
    }
  }
}


// Function to update attribute buffers in Vulkan only if the attribute is present
template <typename T>
void updateAttributeBuffer(const std::string&         attributeName,  // Name of the attribute: POSITION, NORMAL, ...
                           const tinygltf::Model&     model,          // GLTF model
                           const tinygltf::Primitive& primitive,      // GLTF primitive
                           VkCommandBuffer            cmd,            // Command buffer to record the copy
                           nvvk::ResourceAllocator*   alloc,          // Allocator to create the buffer
                           nvvk::Buffer&              attributeBuffer)             // Buffer to be created
{
  if(primitive.attributes.find(attributeName) != primitive.attributes.end())
  {
    const tinygltf::Accessor&   accessor = model.accessors[primitive.attributes.at(attributeName)];
    const tinygltf::BufferView& view     = model.bufferViews[accessor.bufferView];

    // The most common case is that the buffer is directly readable as T
    if((view.byteStride == 0 || view.byteStride == sizeof(T)) && !accessor.sparse.isSparse)
    {
      const float* bufferData =
          reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
      alloc->getStaging()->cmdToBuffer(cmd, attributeBuffer.buffer, 0, sizeof(T) * accessor.count, bufferData);
    }
    else
    {
      // Get accessor data will make a copy of the data, the way we need it
      std::vector<T> tempData;
      tinygltf::utils::getAccessorData(model, accessor, tempData);
      alloc->getStaging()->cmdToBuffer(cmd, attributeBuffer.buffer, 0, sizeof(T) * accessor.count, tempData.data());
    }
  }
}


//--------------------------------------------------------------------------------------------------
// Creating information per primitive
// - Create a buffer of Vertex and Index for each primitive
// - Each primInfo has a reference to the vertex and index buffer, and which material id it uses
//
void nvvkhl::SceneVk::createVertexBuffers(VkCommandBuffer cmd, const nvh::gltf::Scene& scene)
{
  nvh::ScopedTimer st(__FUNCTION__);

  //const auto& gltfScene = scene.scene();
  const auto& model = scene.getModel();

  std::vector<nvvkhl_shaders::RenderPrimitive> renderPrim;  // The array of all primitive information

  auto usageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  // Buffer read/write access within shaders, without size limitation
                   | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT  // The buffer can be referred to using its address instead of a binding
                   | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR  // Usage as a data source for acceleration structure builds
                   | VK_BUFFER_USAGE_TRANSFER_DST_BIT                                      // Buffer can be copied into
                   | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;  // Buffer can be copied from (e.g. for inspection)


  size_t numUniquePrimitive = scene.getNumRenderPrimitives();
  m_bIndices.resize(numUniquePrimitive);
  m_vertexBuffers.resize(numUniquePrimitive);
  renderPrim.resize(numUniquePrimitive);

  for(size_t primID = 0; primID < scene.getNumRenderPrimitives(); primID++)
  {
    const tinygltf::Primitive& primitive     = scene.getRenderPrimitive(primID).primitive;
    VertexBuffers&             vertexBuffers = m_vertexBuffers[primID];

    createAttributeBuffer<glm::vec3>("POSITION", model, primitive, cmd, m_alloc,
                                     usageFlag | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffers.position);
    createAttributeBuffer<glm::vec3>("NORMAL", model, primitive, cmd, m_alloc, usageFlag, vertexBuffers.normal);
    createAttributeBuffer<glm::vec2>("TEXCOORD_0", model, primitive, cmd, m_alloc, usageFlag, vertexBuffers.texCoord0);
    createAttributeBuffer<glm::vec2>("TEXCOORD_1", model, primitive, cmd, m_alloc, usageFlag, vertexBuffers.texCoord1);
    createAttributeBuffer<glm::vec4>("TANGENT", model, primitive, cmd, m_alloc, usageFlag, vertexBuffers.tangent);

    if(tinygltf::utils::hasElementName(primitive.attributes, "COLOR_0"))
    {
      // For color, we need to pack it into a single int
      const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("COLOR_0")];
      std::vector<uint32_t>     tempIntData(accessor.count);
      if(accessor.type == TINYGLTF_TYPE_VEC3)
      {
        std::vector<glm::vec3> tempData;
        tinygltf::utils::getAccessorData(model, accessor, tempData);
        for(size_t i = 0; i < accessor.count; i++)
        {
          tempIntData[i] = glm::packUnorm4x8(glm::vec4(tempData[i], 1));
        }
      }
      else if(accessor.type == TINYGLTF_TYPE_VEC4)
      {
        std::vector<glm::vec4> tempData;
        tinygltf::utils::getAccessorData(model, accessor, tempData);
        for(size_t i = 0; i < accessor.count; i++)
        {
          tempIntData[i] = glm::packUnorm4x8(tempData[i]);
        }
      }
      else
      {
        assert(!"Unknown color type");
      }

      vertexBuffers.color = m_alloc->createBuffer(cmd, tempIntData, usageFlag);
    }

    // Debug name
    if(vertexBuffers.position.buffer != VK_NULL_HANDLE)
      m_dutil->DBG_NAME_IDX(vertexBuffers.position.buffer, primID);
    if(vertexBuffers.normal.buffer != VK_NULL_HANDLE)
      m_dutil->DBG_NAME_IDX(vertexBuffers.normal.buffer, primID);
    if(vertexBuffers.texCoord0.buffer != VK_NULL_HANDLE)
      m_dutil->DBG_NAME_IDX(vertexBuffers.texCoord0.buffer, primID);
    if(vertexBuffers.texCoord1.buffer != VK_NULL_HANDLE)
      m_dutil->DBG_NAME_IDX(vertexBuffers.texCoord1.buffer, primID);
    if(vertexBuffers.tangent.buffer != VK_NULL_HANDLE)
      m_dutil->DBG_NAME_IDX(vertexBuffers.tangent.buffer, primID);
    if(vertexBuffers.color.buffer != VK_NULL_HANDLE)
      m_dutil->DBG_NAME_IDX(vertexBuffers.color.buffer, primID);


    // Buffer of indices
    std::vector<uint32_t> indexBuffer;
    if(primitive.indices > -1)
    {
      const tinygltf::Accessor&   accessor   = model.accessors[primitive.indices];
      const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
      const tinygltf::Buffer&     buffer     = model.buffers[bufferView.buffer];
      assert(accessor.sparse.isSparse == false);
      indexBuffer.resize(accessor.count);
      switch(accessor.componentType)
      {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
          const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
          std::memcpy(indexBuffer.data(), buf, accessor.count * sizeof(uint32_t));
          break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
          const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
          for(size_t index = 0; index < accessor.count; index++)
          {
            indexBuffer[index] = buf[index];
          }
          break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
          const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
          for(size_t index = 0; index < accessor.count; index++)
          {
            indexBuffer[index] = buf[index];
          }
          break;
        }
      }
    }
    else
    {  // Primitive without indices, creating them
      const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("POSITION")];

      indexBuffer.resize(accessor.count);
      for(auto i = 0; i < accessor.count; i++)
        indexBuffer[i] = i;
    }

    // Creating the buffer for the indices
    nvvk::Buffer& i_buffer = m_bIndices[primID];
    i_buffer               = m_alloc->createBuffer(cmd, indexBuffer, usageFlag | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    m_dutil->DBG_NAME_IDX(i_buffer.buffer, primID);

    // Filling the primitive information
    renderPrim[primID].indexAddress = i_buffer.address;

    nvvkhl_shaders::VertexBuffers vBuf = {};
    vBuf.positionAddress               = vertexBuffers.position.address;
    vBuf.normalAddress                 = vertexBuffers.normal.address;
    vBuf.tangentAddress                = vertexBuffers.tangent.address;
    vBuf.texCoord0Address              = vertexBuffers.texCoord0.address;
    vBuf.texCoord1Address              = vertexBuffers.texCoord1.address;
    vBuf.colorAddress                  = vertexBuffers.color.address;
    renderPrim[primID].vertexBuffer    = vBuf;
  }

  // Creating the buffer of all primitive information
  m_bRenderPrim = m_alloc->createBuffer(cmd, renderPrim, usageFlag);
  m_dutil->DBG_NAME(m_bRenderPrim.buffer);

  // Barrier to make sure the data is in the GPU
  VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                       VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

// This version updates all the vertex buffers
void nvvkhl::SceneVk::updateVertexBuffers(VkCommandBuffer cmd, const nvh::gltf::Scene& scene)
{
  const auto& model = scene.getModel();

  for(size_t primID = 0; primID < scene.getNumRenderPrimitives(); primID++)
  {
    const tinygltf::Primitive& primitive     = scene.getRenderPrimitive(primID).primitive;
    VertexBuffers&             vertexBuffers = m_vertexBuffers[primID];
    updateAttributeBuffer<glm::vec3>("POSITION", model, primitive, cmd, m_alloc, vertexBuffers.position);
    updateAttributeBuffer<glm::vec3>("NORMAL", model, primitive, cmd, m_alloc, vertexBuffers.normal);
    updateAttributeBuffer<glm::vec2>("TEXCOORD_0", model, primitive, cmd, m_alloc, vertexBuffers.texCoord0);
    updateAttributeBuffer<glm::vec2>("TEXCOORD_1", model, primitive, cmd, m_alloc, vertexBuffers.texCoord1);
    updateAttributeBuffer<glm::vec4>("TANGENT", model, primitive, cmd, m_alloc, vertexBuffers.tangent);
  }
}


//--------------------------------------------------------------------------------------------------------------
// Returning the Vulkan sampler information from the information in the tinygltf
//
static VkSamplerCreateInfo getSampler(const tinygltf::Model& model, int index)
{
  VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.minFilter  = VK_FILTER_LINEAR;
  samplerInfo.magFilter  = VK_FILTER_LINEAR;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.maxLod     = FLT_MAX;

  if(index < 0)
    return samplerInfo;

  const auto& sampler = model.samplers[index];

  const std::map<int, VkFilter> filters = {{9728, VK_FILTER_NEAREST}, {9729, VK_FILTER_LINEAR},
                                           {9984, VK_FILTER_NEAREST}, {9985, VK_FILTER_LINEAR},
                                           {9986, VK_FILTER_NEAREST}, {9987, VK_FILTER_LINEAR}};

  const std::map<int, VkSamplerMipmapMode> mipmapModes = {
      {9728, VK_SAMPLER_MIPMAP_MODE_NEAREST}, {9729, VK_SAMPLER_MIPMAP_MODE_LINEAR},
      {9984, VK_SAMPLER_MIPMAP_MODE_NEAREST}, {9985, VK_SAMPLER_MIPMAP_MODE_LINEAR},
      {9986, VK_SAMPLER_MIPMAP_MODE_NEAREST}, {9987, VK_SAMPLER_MIPMAP_MODE_LINEAR}};

  const std::map<int, VkSamplerAddressMode> wrapModes = {
      {TINYGLTF_TEXTURE_WRAP_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT},
      {TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE},
      {TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT}};

  if(sampler.minFilter > -1)
    samplerInfo.minFilter = filters.at(sampler.minFilter);
  if(sampler.magFilter > -1)
  {
    samplerInfo.magFilter  = filters.at(sampler.magFilter);
    samplerInfo.mipmapMode = mipmapModes.at(sampler.magFilter);
  }
  samplerInfo.addressModeU = wrapModes.at(sampler.wrapS);
  samplerInfo.addressModeV = wrapModes.at(sampler.wrapT);

  return samplerInfo;
}

//--------------------------------------------------------------------------------------------------------------
// This is creating all images stored in textures
//
void nvvkhl::SceneVk::createTextureImages(VkCommandBuffer cmd, const tinygltf::Model& model, const std::filesystem::path& basedir)
{
  nvh::ScopedTimer st(std::string(__FUNCTION__) + "\n");

  VkSamplerCreateInfo default_sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  default_sampler.minFilter  = VK_FILTER_LINEAR;
  default_sampler.magFilter  = VK_FILTER_LINEAR;
  default_sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  default_sampler.maxLod     = FLT_MAX;

  // Find and all textures/images that should be sRgb encoded.
  findSrgbImages(model);

  // Make dummy image(1,1), needed as we cannot have an empty array
  auto addDefaultImage = [&](uint32_t idx, const std::array<uint8_t, 4>& color) {
    VkImageCreateInfo image_create_info = nvvk::makeImage2DCreateInfo(VkExtent2D{1, 1});
    nvvk::Image       image             = m_alloc->createImage(cmd, 4, color.data(), image_create_info);
    assert(idx < m_images.size());
    m_images[idx] = {image, image_create_info};
    m_dutil->setObjectName(m_images[idx].nvvkImage.image, "Dummy");
  };

  // Make dummy texture/image(1,1), needed as we cannot have an empty array
  auto addDefaultTexture = [&]() {
    assert(!m_images.empty());
    SceneImage&           scn_image = m_images[0];
    VkImageViewCreateInfo iv_info   = nvvk::makeImageViewCreateInfo(scn_image.nvvkImage.image, scn_image.createInfo);
    m_textures.emplace_back(m_alloc->createTexture(scn_image.nvvkImage, iv_info, default_sampler));
  };

  // Load images in parallel
  m_images.resize(model.images.size());
  uint32_t          num_threads = std::min((uint32_t)model.images.size(), std::thread::hardware_concurrency());
  const std::string indent      = st.indent();
  nvh::parallel_batches<1>(  // Not batching
      model.images.size(),
      [&](uint64_t i) {
        const auto& image = model.images[i];
        LOGI("%s(%" PRIu64 ") %s \n", indent.c_str(), i, image.uri.c_str());
        loadImage(basedir, image, static_cast<int>(i));
      },
      num_threads);

  // Create Vulkan images
  for(size_t i = 0; i < m_images.size(); i++)
  {
    if(!createImage(cmd, m_images[i]))
    {
      addDefaultImage((uint32_t)i, {255, 0, 255, 255});  // Image not present or incorrectly loaded (image.empty)
    }
  }

  // Add default image if nothing was loaded
  if(model.images.empty())
  {
    m_images.resize(1);
    addDefaultImage(0, {255, 255, 255, 255});
  }

  // Creating the textures using the above images
  m_textures.reserve(model.textures.size());
  for(size_t i = 0; i < model.textures.size(); i++)
  {
    const auto& texture      = model.textures[i];
    int         source_image = texture.source;

    // MSFT_texture_dds: if the texture is a DDS file, we need to get the source image from the extension
    if(tinygltf::utils::hasElementName(texture.extensions, MSFT_TEXTURE_DDS_NAME))
    {
      const tinygltf::Value& ext = tinygltf::utils::getElementValue(texture.extensions, MSFT_TEXTURE_DDS_NAME);
      tinygltf::utils::getValue(ext, "source", source_image);
    }

    if(source_image >= model.images.size() || source_image < 0)
    {
      addDefaultTexture();  // Incorrect source image
      continue;
    }

    VkSamplerCreateInfo sampler = getSampler(model, texture.sampler);

    SceneImage&           scn_image = m_images[source_image];
    VkImageViewCreateInfo iv_info   = nvvk::makeImageViewCreateInfo(scn_image.nvvkImage.image, scn_image.createInfo);
    m_textures.emplace_back(m_alloc->createTexture(scn_image.nvvkImage, iv_info, sampler));
  }

  // Add a default texture, cannot work with empty descriptor set
  if(model.textures.empty())
  {
    addDefaultTexture();
  }
}

//-------------------------------------------------------------------------------------------------
// Some images must be sRgb encoded, we find them and will be uploaded with the _SRGB format.
//
void nvvkhl::SceneVk::findSrgbImages(const tinygltf::Model& model)
{
  // Lambda helper functions
  auto addImage = [&](int texID) {
    if(texID > -1)
      m_sRgbImages.insert(model.textures[texID].source);
  };

  // For images in extensions
  auto addImageFromExtension = [&](const tinygltf::Material& mat, const std::string extName, const std::string name) {
    const auto& ext = mat.extensions.find(extName);
    if(ext != mat.extensions.end())
    {
      if(ext->second.Has(name))
        addImage(ext->second.Get(name).Get("index").Get<int>());
    }
  };

  // Loop over all materials and find the sRgb textures
  for(size_t matID = 0; matID < model.materials.size(); matID++)
  {
    const auto& mat = model.materials[matID];
    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metallic-roughness-material
    addImage(mat.pbrMetallicRoughness.baseColorTexture.index);
    addImage(mat.emissiveTexture.index);

    // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_specular/README.md#extending-materials
    addImageFromExtension(mat, "KHR_materials_specular", "specularColorTexture");

    // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_sheen/README.md#sheen
    addImageFromExtension(mat, "KHR_materials_sheen", "sheenColorTexture");

    // **Deprecated** but still used with some scenes
    // https://kcoley.github.io/glTF/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness
    addImageFromExtension(mat, "KHR_materials_pbrSpecularGlossiness", "diffuseTexture");
    addImageFromExtension(mat, "KHR_materials_pbrSpecularGlossiness", "specularGlossinessTexture");
  }

  // Special, if the 'extra' in the texture has a gamma defined greater than 1.0, it is sRGB
  for(size_t texID = 0; texID < model.textures.size(); texID++)
  {
    const auto& texture = model.textures[texID];
    if(texture.extras.Has("gamma") && texture.extras.Get("gamma").GetNumberAsDouble() > 1.0)
    {
      m_sRgbImages.insert(texture.source);
    }
  }
}

//--------------------------------------------------------------------------------------------------
// Loading images from disk
//
void nvvkhl::SceneVk::loadImage(const std::filesystem::path& basedir, const tinygltf::Image& gltfImage, int imageID)
{
  namespace fs = std::filesystem;

  auto& image   = m_images[imageID];
  bool  is_srgb = m_sRgbImages.find(imageID) != m_sRgbImages.end();

  std::string uri_decoded;
  tinygltf::URIDecode(gltfImage.uri, &uri_decoded, nullptr);  // ex. whitespace may be represented as %20
  fs::path    uri       = fs::path(uri_decoded);
  std::string extension = uri.extension().string();
  for(char& c : extension)
  {
    c = tolower(c);
  }
  std::string imgName = uri.filename().string();
  image.imgName       = imgName;
  std::string img_uri = fs::path(basedir / uri).string();

  if(extension == ".dds")
  {
    nv_dds::Image         ddsImage;
    nv_dds::ReadSettings  settings{.mips = false};
    nv_dds::ErrorWithText readResult = ddsImage.readFromFile(img_uri.c_str(), settings);
    if(readResult.has_value())
    {
      LOGE("Failed to read %s: %s\n", img_uri.c_str(), readResult.value().c_str());
      return;
    }

    image.srgb        = is_srgb;
    image.size.width  = ddsImage.getWidth(0);
    image.size.height = ddsImage.getHeight(0);
    if(ddsImage.getDepth(0) > 1)
    {
      LOGE("This DDS image had a depth of %u, but loadImage() cannot handle volume textures.\n", ddsImage.getDepth(0));
      return;
    }
    if(ddsImage.getNumFaces() > 1)
    {
      LOGE("This DDS image had %u faces, but loadImage() cannot handle cubemaps.\n", ddsImage.getNumFaces());
      return;
    }
    if(ddsImage.getNumLayers() > 1)
    {
      LOGE("This DDS image had %u array elements, but loadImage() cannot handle array textures.\n", ddsImage.getNumLayers());
      return;
    }
    image.format = texture_formats::dxgiToVulkan(ddsImage.dxgiFormat);
    image.format = texture_formats::tryForceVkFormatTransferFunction(image.format, image.srgb);
    if(VK_FORMAT_UNDEFINED == image.format)
    {
      LOGE("Could not determine a VkFormat for DXGI format %u (%s).\n", ddsImage.dxgiFormat,
           texture_formats::getDXGIFormatName(ddsImage.dxgiFormat));
    }

    const nv_dds::Subresource& mip0      = ddsImage.subresource(0, 0, 0);
    const uint8_t*             mip0Bytes = reinterpret_cast<const uint8_t*>(mip0.data.data());
    image.mipData                        = {{mip0Bytes, mip0Bytes + mip0.data.size()}};
  }
  else if(!extension.empty())
  {
    stbi_uc* data;
    int      w = 0, h = 0, comp = 0;

    // Read the header once to check how many channels it has. We can't trivially use RGB/VK_FORMAT_R8G8B8_UNORM and
    // need to set req_comp=4 in such cases.
    if(!stbi_info(img_uri.c_str(), &w, &h, &comp))
    {
      LOGE("Failed to read %s\n", img_uri.c_str());
      return;
    }

    // Read the header again to check if it has 16 bit data, e.g. for a heightmap.
    bool is_16Bit = stbi_is_16_bit(img_uri.c_str());

    // Load the image
    size_t bytes_per_pixel;
    int    req_comp = comp == 1 ? 1 : 4;
    if(is_16Bit)
    {
      auto data16     = stbi_load_16(img_uri.c_str(), &w, &h, &comp, req_comp);
      bytes_per_pixel = sizeof(*data16) * req_comp;
      data            = reinterpret_cast<stbi_uc*>(data16);
    }
    else
    {
      data            = stbi_load(img_uri.c_str(), &w, &h, &comp, req_comp);
      bytes_per_pixel = sizeof(*data) * req_comp;
    }
    switch(req_comp)
    {
      case 1:
        image.format = is_16Bit ? VK_FORMAT_R16_UNORM : VK_FORMAT_R8_UNORM;
        break;
      case 4:
        image.format = is_16Bit ? VK_FORMAT_R16G16B16A16_UNORM :
                       is_srgb  ? VK_FORMAT_R8G8B8A8_SRGB :
                                  VK_FORMAT_R8G8B8A8_UNORM;

        break;
      default:
        assert(false);
        image.format = VK_FORMAT_UNDEFINED;
    }

    // Make a copy of the image data to be uploaded to vulkan later
    if(data && w > 0 && h > 0 && image.format != VK_FORMAT_UNDEFINED)
    {
      VkDeviceSize buffer_size = static_cast<VkDeviceSize>(w) * h * bytes_per_pixel;
      image.size               = VkExtent2D{(uint32_t)w, (uint32_t)h};
      image.mipData            = {{data, data + buffer_size}};
    }

    stbi_image_free(data);
  }
  else if(gltfImage.width > 0 && gltfImage.height > 0 && !gltfImage.image.empty())
  {  // Loaded internally using GLB
    image.size   = VkExtent2D{(uint32_t)gltfImage.width, (uint32_t)gltfImage.height};
    image.format = is_srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    image.mipData.emplace_back(gltfImage.image);
  }
}

bool nvvkhl::SceneVk::createImage(const VkCommandBuffer& cmd, SceneImage& image)
{
  if(image.size.width == 0 || image.size.height == 0)
    return false;

  VkFormat          format            = image.format;
  VkExtent2D        img_size          = image.size;
  VkImageCreateInfo image_create_info = nvvk::makeImage2DCreateInfo(img_size, format, VK_IMAGE_USAGE_SAMPLED_BIT, true);

  // Check if we can generate mipmap with the the incoming image
  bool               can_generate_mipmaps = false;
  VkFormatProperties format_properties;
  vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &format_properties);
  if((format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) == VK_FORMAT_FEATURE_BLIT_DST_BIT)
    can_generate_mipmaps = true;
  if(image.mipData.size() > 1)  // Use only the number of levels defined
    image_create_info.mipLevels = (uint32_t)image.mipData.size();
  if(image.mipData.size() == 1 || can_generate_mipmaps == false)
    image_create_info.mipLevels = 1;  // Cannot use cmdGenerateMipmaps

  // Keep info for the creation of the texture
  image.createInfo = image_create_info;

  VkDeviceSize buffer_size  = image.mipData[0].size();
  nvvk::Image  result_image = m_alloc->createImage(cmd, buffer_size, image.mipData[0].data(), image_create_info);

  if(image.mipData.size() == 1 && can_generate_mipmaps)
  {
    nvvk::cmdGenerateMipmaps(cmd, result_image.image, format, img_size, image_create_info.mipLevels);
  }
  else
  {
    // Create all mip-levels
    nvvk::cmdBarrierImageLayout(cmd, result_image.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    auto staging = m_alloc->getStaging();
    for(uint32_t mip = 1; mip < (uint32_t)image.mipData.size(); mip++)
    {
      image_create_info.extent.width  = std::max(1u, image.size.width >> mip);
      image_create_info.extent.height = std::max(1u, image.size.height >> mip);

      VkOffset3D               offset{};
      VkImageSubresourceLayers subresource{};
      subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      subresource.layerCount = 1;
      subresource.mipLevel   = mip;

      std::vector<uint8_t>& mipresource = image.mipData[mip];
      VkDeviceSize          bufferSize  = mipresource.size();
      if(image_create_info.extent.width > 0 && image_create_info.extent.height > 0)
      {
        staging->cmdToImage(cmd, result_image.image, offset, image_create_info.extent, subresource, bufferSize,
                            mipresource.data());
      }
    }
    nvvk::cmdBarrierImageLayout(cmd, result_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }

  if(!image.imgName.empty())
  {
    m_dutil->setObjectName(result_image.image, image.imgName);
  }
  else
  {
    m_dutil->DBG_NAME(result_image.image);
  }

  // Clear image.mipData as it is no longer needed
  image = {result_image, image_create_info, image.srgb, image.imgName};

  return true;
}

void nvvkhl::SceneVk::createLightBuffer(VkCommandBuffer cmd, const nvh::gltf::Scene& scn)
{
  const std::vector<nvh::gltf::RenderLight>& rlights = scn.getRenderLights();
  if(rlights.empty())
    return;

  std::vector<nvvkhl_shaders::Light> shaderLights = getShaderLights(rlights, scn.getModel().lights);

  m_bLights = m_alloc->createBuffer(cmd, shaderLights, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  m_dutil->setObjectName(m_bLights.buffer, "Lights");
}

std::vector<nvvkhl_shaders::Light> getShaderLights(const std::vector<nvh::gltf::RenderLight>& renderlights,
                                                   const std::vector<tinygltf::Light>&        gltfLights)
{
  std::vector<nvvkhl_shaders::Light> lightsInfo;
  lightsInfo.reserve(renderlights.size());
  for(auto& l : renderlights)
  {
    const auto& gltfLight = gltfLights[l.light];

    nvvkhl_shaders::Light info{};
    info.position   = l.worldMatrix[3];
    info.direction  = -l.worldMatrix[2];  // glm::vec3(l.worldMatrix * glm::vec4(0, 0, -1, 0));
    info.innerAngle = static_cast<float>(gltfLight.spot.innerConeAngle);
    info.outerAngle = static_cast<float>(gltfLight.spot.outerConeAngle);
    if(gltfLight.color.size() == 3)
      info.color = glm::vec3(gltfLight.color[0], gltfLight.color[1], gltfLight.color[2]);
    else
      info.color = glm::vec3(1, 1, 1);  // default color (white)
    info.intensity = static_cast<float>(gltfLight.intensity);
    info.type      = gltfLight.type == "point" ? nvvkhl_shaders::eLightTypePoint :
                     gltfLight.type == "spot"  ? nvvkhl_shaders::eLightTypeSpot :
                                                 nvvkhl_shaders::eLightTypeDirectional;

    info.radius = gltfLight.extras.Has("radius") ? float(gltfLight.extras.Get("radius").GetNumberAsDouble()) : 0.0f;

    if(info.type == nvvkhl_shaders::eLightTypeDirectional)
    {
      const double sun_distance     = 149597870.0;  // km
      double       angular_size_rad = 2.0 * std::atan(info.radius / sun_distance);
      info.angularSizeOrInvRange    = static_cast<float>(angular_size_rad);
    }
    else
    {
      info.angularSizeOrInvRange = (gltfLight.range > 0.0) ? 1.0f / static_cast<float>(gltfLight.range) : 0.0f;
    }

    lightsInfo.emplace_back(info);
  }
  return lightsInfo;
}

void nvvkhl::SceneVk::destroy()
{
  for(auto& vb : m_vertexBuffers)
  {
    m_alloc->destroy(vb.position);
    m_alloc->destroy(vb.normal);
    m_alloc->destroy(vb.tangent);
    m_alloc->destroy(vb.texCoord0);
    m_alloc->destroy(vb.texCoord1);
    m_alloc->destroy(vb.color);
  }
  m_vertexBuffers.clear();

  for(auto& i : m_bIndices)
  {
    m_alloc->destroy(i);
  }
  m_bIndices.clear();

  m_alloc->destroy(m_bMaterial);
  m_alloc->destroy(m_bLights);
  m_alloc->destroy(m_bRenderPrim);
  m_alloc->destroy(m_bRenderNode);
  m_alloc->destroy(m_bSceneDesc);

  for(auto& i : m_images)
  {
    m_alloc->destroy(i.nvvkImage);
  }
  m_images.clear();

  for(auto& t : m_textures)
  {
    vkDestroyImageView(m_device, t.descriptor.imageView, nullptr);
  }
  m_textures.clear();

  m_sRgbImages.clear();
}
