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

#include "gltf_scene_vk.hpp"

#include <inttypes.h>
#include <mutex>
#include <sstream>
#include "stb_image.h"

#include "nvvk/images_vk.hpp"
#include "nvh/parallel_work.hpp"
#include "nvh/timesampler.hpp"
#include "nvvk/buffers_vk.hpp"

#include "shaders/dh_scn_desc.h"

nvvkhl::SceneVk::SceneVk(nvvk::Context* ctx, AllocVma* alloc)
    : m_ctx(ctx)
    , m_alloc(alloc)
{
  m_dutil = std::make_unique<nvvk::DebugUtil>(ctx->m_device);  // Debug utility
}

//--------------------------------------------------------------------------------------------------
// Create all Vulkan resources to hold a nvvkhl::Scene
//
void nvvkhl::SceneVk::create(VkCommandBuffer cmd, const nvvkhl::Scene& scn)
{
  nvh::ScopedTimer st(__FUNCTION__);
  destroy();  // Make sure not to leave allocated buffers

  namespace fs     = std::filesystem;
  fs::path basedir = fs::path(scn.filename()).parent_path();
  createMaterialBuffer(cmd, scn.scene());
  createInstanceInfoBuffer(cmd, scn.scene());
  createVertexBuffer(cmd, scn.scene());
  createTextureImages(cmd, scn.model(), basedir);

  // Buffer references
  SceneDescription scene_desc{};
  scene_desc.materialAddress = nvvk::getBufferDeviceAddress(m_ctx->m_device, m_bMaterial.buffer);
  scene_desc.primInfoAddress = nvvk::getBufferDeviceAddress(m_ctx->m_device, m_bPrimInfo.buffer);
  scene_desc.instInfoAddress = nvvk::getBufferDeviceAddress(m_ctx->m_device, m_bInstances.buffer);
  m_bSceneDesc               = m_alloc->createBuffer(cmd, sizeof(SceneDescription), &scene_desc,
                                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  m_dutil->DBG_NAME(m_bSceneDesc.buffer);
}

//--------------------------------------------------------------------------------------------------
// Create a buffer of all materials, with only the elements we need
//
void nvvkhl::SceneVk::createMaterialBuffer(VkCommandBuffer cmd, const nvh::GltfScene& scn)
{
  nvh::ScopedTimer st(__FUNCTION__);

  std::vector<GltfShadeMaterial> shade_materials;
  shade_materials.reserve(scn.m_materials.size());
  for(const auto& m : scn.m_materials)
  {
    GltfShadeMaterial s{};
    s.emissiveFactor               = m.emissiveFactor;
    s.emissiveTexture              = m.emissiveTexture;
    s.khrDiffuseFactor             = m.specularGlossiness.diffuseFactor;
    s.khrDiffuseTexture            = m.specularGlossiness.diffuseTexture;
    s.khrSpecularFactor            = m.specularGlossiness.specularFactor;
    s.khrGlossinessFactor          = m.specularGlossiness.glossinessFactor;
    s.khrSpecularGlossinessTexture = m.specularGlossiness.specularGlossinessTexture;
    s.normalTexture                = m.normalTexture;
    s.normalTextureScale           = m.normalTextureScale;
    s.pbrBaseColorFactor           = m.baseColorFactor;
    s.pbrBaseColorTexture          = m.baseColorTexture;
    s.pbrMetallicFactor            = m.metallicFactor;
    s.pbrMetallicRoughnessTexture  = m.metallicRoughnessTexture;
    s.pbrRoughnessFactor           = m.roughnessFactor;
    s.shadingModel                 = m.shadingModel;
    s.alphaMode                    = m.alphaMode;
    s.alphaCutoff                  = m.alphaCutoff;
    // KHR_materials_transmission
    s.transmissionFactor  = m.transmission.factor;
    s.transmissionTexture = m.transmission.texture;
    // KHR_materials_ior
    s.ior = m.ior.ior;
    // KHR_materials_volume
    s.attenuationColor    = m.volume.attenuationColor;
    s.thicknessFactor     = m.volume.thicknessFactor;
    s.thicknessTexture    = m.volume.thicknessTexture;
    s.attenuationDistance = m.volume.attenuationDistance;
    // KHR_materials_clearcoat
    s.clearcoatFactor    = m.clearcoat.factor;
    s.clearcoatRoughness = m.clearcoat.roughnessFactor;
    s.clearcoatTexture   = m.clearcoat.roughnessTexture;
    s.clearcoatTexture   = m.clearcoat.texture;
    // KHR_materials_emissive_strength
    s.emissiveFactor *= m.emissiveStrength.emissiveStrength;

    shade_materials.emplace_back(s);
  }
  m_bMaterial = m_alloc->createBuffer(cmd, shade_materials,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  m_dutil->DBG_NAME(m_bMaterial.buffer);
}

//--------------------------------------------------------------------------------------------------
// Array of instance information
// - Use by the vertex shader to retrieve the position of the instance
void nvvkhl::SceneVk::createInstanceInfoBuffer(VkCommandBuffer cmd, const nvh::GltfScene& scn)
{
  nvh::ScopedTimer st(__FUNCTION__);

  std::vector<InstanceInfo> inst_info;
  for(const auto& node : scn.m_nodes)
  {
    InstanceInfo info{};
    info.objectToWorld = node.worldMatrix;
    info.worldToObject = nvmath::invert(node.worldMatrix);
    inst_info.emplace_back(info);
  }
  m_bInstances = m_alloc->createBuffer(cmd, inst_info, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  m_dutil->DBG_NAME(m_bInstances.buffer);
}

//--------------------------------------------------------------------------------------------------
// Creating information per primitive
// - Create a buffer of Vertex and Index for each primitive
// - Each primInfo has a reference to the vertex and index buffer, and which material id it uses
//
void nvvkhl::SceneVk::createVertexBuffer(VkCommandBuffer cmd, const nvh::GltfScene& scn)
{
  nvh::ScopedTimer st(__FUNCTION__);

  std::vector<PrimMeshInfo> prim_info;  // The array of all primitive information
  uint32_t                  prim_idx{0};

  auto usage_flag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                    | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

  // Primitives in glTF can be reused, this allow to retrieve them
  std::unordered_map<std::string, nvvk::Buffer> cache_primitive;

  prim_info.resize(scn.m_primMeshes.size());
  m_bVertices.resize(scn.m_primMeshes.size());
  m_bIndices.resize(scn.m_primMeshes.size());

  for(const auto& prim_mesh : scn.m_primMeshes)
  {
    // Create a key to find a primitive that is already uploaded
    std::stringstream o;
    o << prim_mesh.vertexOffset << ":" << prim_mesh.vertexCount;
    std::string key = o.str();

    nvvk::Buffer v_buffer;  // Vertex buffer result
    auto         it = cache_primitive.find(key);
    if(it == cache_primitive.end())
    {
      // Filling in parallel the vector of vertex used on the GPU
      std::vector<Vertex> vertices(prim_mesh.vertexCount);
      for(uint32_t v_ctx = 0; v_ctx < prim_mesh.vertexCount; v_ctx++)
      {
        size_t               idx = prim_mesh.vertexOffset + v_ctx;
        const nvmath::vec3f& p   = scn.m_positions[idx];
        const nvmath::vec3f& n   = scn.m_normals[idx];
        const nvmath::vec4f& t   = scn.m_tangents[idx];
        const nvmath::vec2f& u   = scn.m_texcoords0[idx];

        Vertex& v  = vertices[v_ctx];
        v.position = nvmath::vec4f(p, u.x);  // Adding texcoord to the end of position and normal vector
        v.normal   = nvmath::vec4f(n, u.y);
        v.tangent  = t;
      }

      // Buffer of Vertex per primitive
      v_buffer = m_alloc->createBuffer(cmd, vertices, usage_flag | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
      m_dutil->DBG_NAME_IDX(v_buffer.buffer, prim_idx);
      cache_primitive[key] = v_buffer;
    }
    else
    {
      v_buffer = it->second;
    }
    m_bVertices[prim_idx] = v_buffer;

    // Buffer of indices
    std::vector<uint32_t> indices(prim_mesh.indexCount);
    memcpy(indices.data(), &scn.m_indices[prim_mesh.firstIndex], sizeof(uint32_t) * prim_mesh.indexCount);

    auto i_buffer = m_alloc->createBuffer(cmd, indices, usage_flag | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    m_dutil->DBG_NAME_IDX(i_buffer.buffer, prim_idx);
    m_bIndices[prim_idx] = i_buffer;

    // Primitive information, material Id and addresses of buffers
    prim_info[prim_idx].materialIndex = prim_mesh.materialIndex;
    prim_info[prim_idx].vertexAddress = nvvk::getBufferDeviceAddress(m_ctx->m_device, v_buffer.buffer);
    prim_info[prim_idx].indexAddress  = nvvk::getBufferDeviceAddress(m_ctx->m_device, i_buffer.buffer);

    prim_idx++;
  }

  // Creating the buffer of all primitive information
  m_bPrimInfo = m_alloc->createBuffer(cmd, prim_info, usage_flag);
  m_dutil->DBG_NAME(m_bPrimInfo.buffer);
}

//--------------------------------------------------------------------------------------------------------------
// Returning the Vulkan sampler information from the information in the tinygltf
//
VkSamplerCreateInfo getSampler(const tinygltf::Model& tiny, int index)
{
  VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.minFilter  = VK_FILTER_LINEAR;
  samplerInfo.magFilter  = VK_FILTER_LINEAR;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.maxLod     = FLT_MAX;

  if(index < 0)
    return samplerInfo;

  const auto& sampler = tiny.samplers[index];

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
void nvvkhl::SceneVk::createTextureImages(VkCommandBuffer cmd, const tinygltf::Model& tiny, const std::filesystem::path& basedir)
{
  nvh::ScopedTimer st(std::string(__FUNCTION__) + "\n");

  VkSamplerCreateInfo default_sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  default_sampler.minFilter  = VK_FILTER_LINEAR;
  default_sampler.magFilter  = VK_FILTER_LINEAR;
  default_sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  default_sampler.maxLod     = FLT_MAX;

  // Find and all textures/images that should be sRgb encoded.
  findSrgbImages(tiny);

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
  m_images.resize(tiny.images.size());
  uint32_t          num_threads = std::min((uint32_t)tiny.images.size(), std::thread::hardware_concurrency());
  const std::string indent      = st.indent();
  nvh::parallel_batches<1>(  // Not batching
      tiny.images.size(),
      [&](uint64_t i) {
        const auto& image = tiny.images[i];
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
  if(tiny.images.empty())
  {
    m_images.resize(1);
    addDefaultImage(0, {255, 255, 255, 255});
  }

  // Creating the textures using the above images
  m_textures.reserve(tiny.textures.size());
  for(size_t i = 0; i < tiny.textures.size(); i++)
  {
    int source_image = tiny.textures[i].source;
    if(source_image >= tiny.images.size() || source_image < 0)
    {
      addDefaultTexture();  // Incorrect source image
      continue;
    }

    VkSamplerCreateInfo sampler = getSampler(tiny, tiny.textures[i].sampler);

    SceneImage&           scn_image = m_images[source_image];
    VkImageViewCreateInfo iv_info   = nvvk::makeImageViewCreateInfo(scn_image.nvvkImage.image, scn_image.createInfo);
    m_textures.emplace_back(m_alloc->createTexture(scn_image.nvvkImage, iv_info, sampler));
  }

  // Add a default texture, cannot work with empty descriptor set
  if(tiny.textures.empty())
  {
    addDefaultTexture();
  }
}

//-------------------------------------------------------------------------------------------------
// Some images must be sRgb encoded, we find them and will be uploaded with the _SRGB format.
//
void nvvkhl::SceneVk::findSrgbImages(const tinygltf::Model& tiny)
{
  // Lambda helper functions
  auto addImage = [&](int texID) {
    if(texID > -1)
      m_sRgbImages.insert(tiny.textures[texID].source);
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
  for(size_t matID = 0; matID < tiny.materials.size(); matID++)
  {
    const auto& mat = tiny.materials[matID];
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
  for(size_t texID = 0; texID < tiny.textures.size(); texID++)
  {
    const auto& texture = tiny.textures[texID];
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
  std::string imgName   = uri.filename().string();
  std::string img_uri   = fs::path(basedir / uri).string();

  if(!extension.empty())
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
  else
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
  vkGetPhysicalDeviceFormatProperties(m_ctx->m_physicalDevice, format, &format_properties);
  if((format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) == VK_FORMAT_FEATURE_BLIT_DST_BIT)
    can_generate_mipmaps = true;
  if(image.mipData.size() > 1)  // Use only the number of levels defined
    image_create_info.mipLevels = (uint32_t)image.mipData.size();
  if(image.mipData.size() == 1 && can_generate_mipmaps == false)
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

void nvvkhl::SceneVk::destroy()
{
  try
  {
    std::set<VkBuffer> v_set;  // Vertex buffer can be shared
    for(auto& v : m_bVertices)
    {
      if(v_set.find(v.buffer) == v_set.end())
      {
        v_set.insert(v.buffer);
        m_alloc->destroy(v);  // delete only the one that was not deleted
      }
    }
  }
  catch(const std::bad_alloc& /* e */)
  {
    assert(!"Failed to allocate memory to identify which vertex buffers to destroy!");
  }
  m_bVertices.clear();

  for(auto& i : m_bIndices)
  {
    m_alloc->destroy(i);
  }
  m_bIndices.clear();

  m_alloc->destroy(m_bMaterial);
  m_alloc->destroy(m_bPrimInfo);
  m_alloc->destroy(m_bInstances);
  m_alloc->destroy(m_bSceneDesc);

  for(auto& i : m_images)
  {
    m_alloc->destroy(i.nvvkImage);
  }
  m_images.clear();

  for(auto& t : m_textures)
  {
    vkDestroyImageView(m_ctx->m_device, t.descriptor.imageView, nullptr);
  }
  m_textures.clear();

  m_sRgbImages.clear();
}
