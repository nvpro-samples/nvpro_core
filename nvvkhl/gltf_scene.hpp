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

#include "nvh/cameramanipulator.hpp"
#include "nvh/gltfscene.hpp"
#include "nvh/timesampler.hpp"
#include "tiny_gltf.h"

namespace nvvkhl {

class Scene
{
public:
  // Loading .gltf or .glb, `request` attribute and `force` to create if not present
  void load(const std::string& filename,
            nvh::GltfAttributes requested = nvh::GltfAttributes::Normal | nvh::GltfAttributes::Texcoord_0 | nvh::GltfAttributes::Tangent,
            nvh::GltfAttributes forced = nvh::GltfAttributes::Normal | nvh::GltfAttributes::Texcoord_0 | nvh::GltfAttributes::Tangent)
  {
    m_scene    = {};
    m_model    = {};
    m_filename = filename;

    nvh::Stopwatch sw;

    tinygltf::TinyGLTF tcontext;
    std::string        warn;
    std::string        error;

    LOGI("- Loading file:\n\t %s\n", filename.c_str());

    auto ext = std::filesystem::path(filename).extension().string();
    bool result{false};
    if(ext == ".gltf")
    {
      result = tcontext.LoadASCIIFromFile(&m_model, &error, &warn, filename);
    }
    else if(ext == ".glb")
    {
      result = tcontext.LoadBinaryFromFile(&m_model, &error, &warn, filename);
    }

    if(!result)
    {
      LOGW("%s\n", warn.c_str());
      LOGE("%s\n", error.c_str());
      assert(!"Error while loading scene");
    }

    m_scene.importMaterials(m_model);
    m_scene.importDrawableNodes(m_model, requested, forced);
    LOGI("= Total %.3f ms\n\n", sw.elapsed());
  }

  // Getters
  const nvh::GltfScene&  scene() const { return m_scene; }
  const tinygltf::Model& model() const { return m_model; }
  bool                   valid() const { return !m_scene.m_nodes.empty(); }
  const std::string&     filename() const { return m_filename; }

  // Clearing loaded model
  void clearModel() { m_model = {}; }

  void destroy()
  {
    m_scene    = {};
    m_model    = {};
    m_filename = {};
  }

private:
  nvh::GltfScene  m_scene;
  tinygltf::Model m_model;
  std::string     m_filename;
};

}  // namespace nvvkhl
