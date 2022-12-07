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

// Various Application utilities
// - Display a menu with File/Quit
// - Display basic information in the window title

#include <filesystem>
#include "imgui_camera_widget.h"
#include "nvh/cameramanipulator.hpp"
#include "nvh/gltfscene.hpp"

namespace nvvkhl {

//--------------------------------------------------------------------------------------------------
// Setting up the camera in the GUI from the camera found in the scene
// or, fit the camera to see the scene.
//
static void setCameraFromScene(const std::string& m_filename, const nvh::GltfScene& m_scene)
{
  ImGuiH::SetCameraJsonFile(std::filesystem::path(m_filename).stem().string());
  if(!m_scene.m_cameras.empty())
  {
    const auto& c = m_scene.m_cameras[0];
    CameraManip.setCamera({c.eye, c.center, c.up, static_cast<float>(rad2deg(c.cam.perspective.yfov))});
    ImGuiH::SetHomeCamera({c.eye, c.center, c.up, static_cast<float>(rad2deg(c.cam.perspective.yfov))});

    for(const auto& cam : m_scene.m_cameras)
    {
      ImGuiH::AddCamera({cam.eye, cam.center, cam.up, static_cast<float>(rad2deg(cam.cam.perspective.yfov))});
    }
  }
  else
  {
    // Re-adjusting camera to fit the new scene
    CameraManip.fit(m_scene.m_dimensions.min, m_scene.m_dimensions.max, true);
    ImGuiH::SetHomeCamera(CameraManip.getCamera());
  }

  CameraManip.setClipPlanes(nvmath::vec2f(0.001F * m_scene.m_dimensions.radius, 100.0F * m_scene.m_dimensions.radius));
}

}  // namespace nvvkhl
