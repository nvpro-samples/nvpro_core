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

#pragma once

#include <filesystem>
#include <set>

#include "nvvk/debug_util_vk.hpp"
#include "nvvk/context_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"

#include "nvh/gltfscene.hpp"


/** @DOC_START
# class nvvkhl::SceneVk

>  This class is responsible for the Vulkan version of the scene. 

It is using `nvvkhl::Scene` to create the Vulkan buffers and images.

@DOC_END */

namespace nvvkhl {
// Create the Vulkan version of the Scene
// Allocate the buffers, etc.
class SceneVk
{
public:
  // Those are potential buffers that can be created for vertices
  struct VertexBuffers
  {
    nvvk::Buffer position;
    nvvk::Buffer normal;
    nvvk::Buffer tangent;
    nvvk::Buffer texCoord0;
    nvvk::Buffer color;
  };

  SceneVk(VkDevice device, VkPhysicalDevice physicalDevice, nvvk::ResourceAllocator* alloc);
  virtual ~SceneVk() { destroy(); }

  virtual void create(VkCommandBuffer cmd, const nvh::gltf::Scene& scn);
  void         update(VkCommandBuffer cmd, const nvh::gltf::Scene& scn);
  void         updateRenderNodeBuffer(VkCommandBuffer cmd, const nvh::gltf::Scene& scn);
  void         updateMaterialBuffer(VkCommandBuffer cmd, const nvh::gltf::Scene& scn);
  void         updateVertexBuffers(VkCommandBuffer cmd, const nvh::gltf::Scene& scene);
  virtual void destroy();

  // Getters
  const nvvk::Buffer&               material() const { return m_bMaterial; }
  const nvvk::Buffer&               primInfo() const { return m_bRenderPrim; }
  const nvvk::Buffer&               instances() const { return m_bRenderNode; }
  const nvvk::Buffer&               sceneDesc() const { return m_bSceneDesc; }
  const std::vector<VertexBuffers>& vertexBuffers() const { return m_vertexBuffers; }
  const std::vector<nvvk::Buffer>&  indices() const { return m_bIndices; }
  const std::vector<nvvk::Texture>& textures() const { return m_textures; }
  uint32_t                          nbTextures() const { return static_cast<uint32_t>(m_textures.size()); }

protected:
  struct SceneImage  // Image to be loaded and created
  {
    nvvk::Image       nvvkImage;
    VkImageCreateInfo createInfo;

    // Loading information
    bool                              srgb{false};
    std::string                       imgName;
    VkExtent2D                        size{0, 0};
    VkFormat                          format{VK_FORMAT_UNDEFINED};
    std::vector<std::vector<uint8_t>> mipData;
  };

  virtual void createMaterialBuffer(VkCommandBuffer cmd, const std::vector<tinygltf::Material>& materials);
  virtual void createRenderNodeBuffer(VkCommandBuffer cmd, const nvh::gltf::Scene& scn);
  virtual void createVertexBuffers(VkCommandBuffer cmd, const nvh::gltf::Scene& scn);
  virtual void createTextureImages(VkCommandBuffer cmd, const tinygltf::Model& model, const std::filesystem::path& basedir);
  virtual void createLightBuffer(VkCommandBuffer cmd, const nvh::gltf::Scene& scn);

  void findSrgbImages(const tinygltf::Model& model);

  virtual void loadImage(const std::filesystem::path& basedir, const tinygltf::Image& gltfImage, int imageID);
  virtual bool createImage(const VkCommandBuffer& cmd, SceneImage& image);

  //--
  VkDevice         m_device{VK_NULL_HANDLE};
  VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};

  nvvk::ResourceAllocator*         m_alloc;
  std::unique_ptr<nvvk::DebugUtil> m_dutil;

  nvvk::Buffer               m_bMaterial;
  nvvk::Buffer               m_bLights;
  nvvk::Buffer               m_bRenderPrim;
  nvvk::Buffer               m_bRenderNode;
  nvvk::Buffer               m_bSceneDesc;
  std::vector<nvvk::Buffer>  m_bIndices;
  std::vector<VertexBuffers> m_vertexBuffers;
  std::vector<SceneImage>    m_images;
  std::vector<nvvk::Texture> m_textures;  // Vector of all textures of the scene

  std::set<int> m_sRgbImages;  // All images that are in sRGB (typically, only the one used by baseColorTexture)
};

}  // namespace nvvkhl
