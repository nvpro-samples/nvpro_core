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

#pragma once

#include <filesystem>

#include "nvvk/debug_util_vk.hpp"
#include "nvvkhl/alloc_vma.hpp"

#include "gltf_scene.hpp"


namespace nvvkhl {
// Create the Vulkan version of the Scene
// Allocate the buffers, etc.
class SceneVk
{
public:
  SceneVk(nvvk::Context* ctx, AllocVma* alloc);
  virtual ~SceneVk() { destroy(); }

  virtual void create(VkCommandBuffer cmd, const nvvkhl::Scene& scn);
  virtual void destroy();

  // Getters
  const nvvk::Buffer&               material() const { return m_bMaterial; }
  const nvvk::Buffer&               primInfo() const { return m_bPrimInfo; }
  const nvvk::Buffer&               instances() const { return m_bInstances; }
  const nvvk::Buffer&               sceneDesc() const { return m_bSceneDesc; }
  const std::vector<nvvk::Buffer>&  vertices() const { return m_bVertices; }
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

  virtual void createMaterialBuffer(VkCommandBuffer cmd, const nvh::GltfScene& scn);
  virtual void createInstanceInfoBuffer(VkCommandBuffer cmd, const nvh::GltfScene& scn);
  virtual void createVertexBuffer(VkCommandBuffer cmd, const nvh::GltfScene& scn);
  virtual void createTextureImages(VkCommandBuffer cmd, const tinygltf::Model& tiny, const std::filesystem::path& basedir);

  void findSrgbImages(const tinygltf::Model& tiny);

  virtual void loadImage(const std::filesystem::path& basedir, const tinygltf::Image& gltfImage, int imageID);
  virtual bool createImage(const VkCommandBuffer& cmd, SceneImage& image);

  //--
  nvvk::Context*                   m_ctx;
  AllocVma*                        m_alloc;
  std::unique_ptr<nvvk::DebugUtil> m_dutil;

  nvvk::Buffer              m_bMaterial;
  nvvk::Buffer              m_bPrimInfo;
  nvvk::Buffer              m_bInstances;
  nvvk::Buffer              m_bSceneDesc;
  std::vector<nvvk::Buffer> m_bVertices;
  std::vector<nvvk::Buffer> m_bIndices;

  std::vector<SceneImage>    m_images;
  std::vector<nvvk::Texture> m_textures;  // Vector of all textures of the scene

  std::set<int> m_sRgbImages;  // All images that are in sRGB (typically, only the one used by baseColorTexture)
};

}  // namespace nvvkhl
