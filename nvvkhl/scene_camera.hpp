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

// Various Application utilities
// - Display a menu with File/Quit
// - Display basic information in the window title

#pragma once

#include <filesystem>
#include "imgui/imgui_camera_widget.h"
#include "nvh/cameramanipulator.hpp"
#include "nvh/gltfscene.hpp"
#include "nvh/boundingbox.hpp"

namespace nvvkhl {


/** @DOC_START
# Function nvvkhl::setCameraFromScene
>  Set the camera from the scene, if no camera is found, it will fit the camera to the scene.
@DOC_END */

inline void setCameraFromScene(const std::string& m_filename, const nvh::GltfScene& m_scene)
{
  ImGuiH::SetCameraJsonFile(std::filesystem::path(m_filename).stem().string());
  if(!m_scene.m_cameras.empty())
  {
    const auto& c = m_scene.m_cameras[0];
    CameraManip.setCamera({c.eye, c.center, c.up, static_cast<float>(glm::degrees(c.cam.perspective.yfov))});
    CameraManip.setClipPlanes({c.cam.perspective.znear, c.cam.perspective.zfar});
    ImGuiH::SetHomeCamera({c.eye, c.center, c.up, static_cast<float>(glm::degrees(c.cam.perspective.yfov))});

    for(const auto& cam : m_scene.m_cameras)
    {
      ImGuiH::AddCamera({cam.eye, cam.center, cam.up, static_cast<float>(glm::degrees(cam.cam.perspective.yfov))});
    }
  }
  else
  {
    // Re-adjusting camera to fit the new scene
    CameraManip.fit(m_scene.m_dimensions.min, m_scene.m_dimensions.max, true);
    ImGuiH::SetHomeCamera(CameraManip.getCamera());
  }

  CameraManip.setClipPlanes(glm::vec2(0.001F * m_scene.m_dimensions.radius, 100.0F * m_scene.m_dimensions.radius));
}


inline void setCamera(const std::string& m_filename, const std::vector<nvh::gltf::RenderCamera>& cameras, const nvh::Bbox& sceneBbox)
{
  ImGuiH::SetCameraJsonFile(std::filesystem::path(m_filename).stem().string());
  if(!cameras.empty())
  {
    const auto& camera = cameras[0];
    CameraManip.setCamera({camera.eye, camera.center, camera.up, static_cast<float>(glm::degrees(camera.yfov))});
    CameraManip.setClipPlanes({camera.znear, camera.zfar});
    ImGuiH::SetHomeCamera({camera.eye, camera.center, camera.up, static_cast<float>(glm::degrees(camera.yfov))});

    for(const auto& cam : cameras)
    {
      ImGuiH::AddCamera({cam.eye, cam.center, cam.up, static_cast<float>(glm::degrees(cam.yfov))});
    }
  }
  else
  {
    // Re-adjusting camera to fit the new scene
    CameraManip.fit(sceneBbox.min(), sceneBbox.max(), true);
    ImGuiH::SetHomeCamera(CameraManip.getCamera());
  }

  CameraManip.setClipPlanes(glm::vec2(0.001F * sceneBbox.radius(), 100.0F * sceneBbox.radius()));
}


}  // namespace nvvkhl
