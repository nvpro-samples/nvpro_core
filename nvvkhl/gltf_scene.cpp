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

#include "gltf_scene.hpp"

#include <filesystem>
#include "nvh/timesampler.hpp"


void nvvkhl::Scene::load(const std::string& filename, nvh::GltfAttributes requested, nvh::GltfAttributes forced)
{
  nvh::ScopedTimer st(std::string(__FUNCTION__) + "\n");
  LOGI("%s%s\n", nvh::ScopedTimer::indent().c_str(), filename.c_str());

  m_scene    = {};
  m_model    = {};
  m_filename = filename;

  nvh::Stopwatch sw;

  tinygltf::TinyGLTF tcontext;
  std::string        warn;
  std::string        error;

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
    LOGW("%s%s\n", st.indent().c_str(), warn.c_str());
    LOGE("%s%s\n", st.indent().c_str(), error.c_str());
    assert(!"Error while loading scene");
  }

  m_scene.importMaterials(m_model);
  m_scene.importDrawableNodes(m_model, requested, forced);
}

void nvvkhl::Scene::destroy()
{
  m_scene    = {};
  m_model    = {};
  m_filename = {};
}
