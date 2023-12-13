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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2023 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include "nvh/cameramanipulator.hpp"
#include "nvvkhl/application.hpp"

/*--------------------------------------------------------------------------------------------------

This is catching the interaction of the mouse and keyboard to modify the Singleton Camera of the scene.

--------------------------------------------------------------------------------------------------*/


namespace nvvkhl {

struct ElementCamera : public nvvkhl::IAppElement
{
  // Return true if the current window is active
  bool isWindowHovered(ImGuiWindow* ref_window, ImGuiHoveredFlags flags)
  {
    ImGuiContext& g = *ImGui::GetCurrentContext();
    if(g.HoveredWindow != ref_window)
      return false;
    if(!ImGui::IsWindowContentHoverable(ref_window, ImGuiFocusedFlags_RootWindow))
      return false;
    if(g.ActiveId != 0 && !g.ActiveIdAllowOverlap && g.ActiveId != ref_window->MoveId)
      return false;

    // Cancel if over the title bar
    {
      if(g.IO.ConfigWindowsMoveFromTitleBarOnly)
        if(!(ref_window->Flags & ImGuiWindowFlags_NoTitleBar) || ref_window->DockIsActive)
          if(ref_window->TitleBarRect().Contains(g.IO.MousePos))
            return false;
    }

    return true;
  }

  void onUIRender() override
  {
    CameraManip.updateAnim();  // This makes the camera to transition smoothly to the new position

    ImGuiWindow* viewportWindow = ImGui::FindWindowByName("Viewport");
    if(!viewportWindow)
      return;

    // If the mouse cursor is over the "Viewport", check for all inputs that can manipulate
    // the camera.
    if(isWindowHovered(viewportWindow, ImGuiFocusedFlags_RootWindow))
    {
      CameraManip.setWindowSize(static_cast<int>(viewportWindow->Size.x), static_cast<int>(viewportWindow->Size.y));
      updateCamera();
    }
  }

  //--------------------------------------------------------------------------------------------------
  // Fit the camera to the Bounding box
  //
  void fitCamera(const glm::vec3& boxMin, const glm::vec3& boxMax, bool instantFit /*= true*/) const
  {
    float aspect_ratio = static_cast<float>(m_viewportSize.x / m_viewportSize.y);
    CameraManip.fit(boxMin, boxMax, instantFit, false, aspect_ratio);
  }

  void setSceneRadius(float r) { m_sceneRadius = r; }

private:
  void updateCamera()
  {
    // measure one frame at a time
    float factor = ImGui::GetIO().DeltaTime * 5.0F * m_sceneRadius;

    m_inputs.lmb   = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    m_inputs.rmb   = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    m_inputs.mmb   = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    m_inputs.ctrl  = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    m_inputs.shift = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
    m_inputs.alt   = ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt);
    auto mouse_pos = ImGui::GetMousePos();

    // For all pressed keys - apply the action
    CameraManip.keyMotion(0, 0, nvh::CameraManipulator::NoAction);

    if(!(ImGui::IsKeyDown(ImGuiMod_Alt) || ImGui::IsKeyDown(ImGuiMod_Ctrl) || ImGui::IsKeyDown(ImGuiMod_Shift)))
    {
      if(ImGui::IsKeyDown(ImGuiKey_W))
      {
        CameraManip.keyMotion(factor, 0, nvh::CameraManipulator::Dolly);
      }

      if(ImGui::IsKeyDown(ImGuiKey_S))
      {
        CameraManip.keyMotion(-factor, 0, nvh::CameraManipulator::Dolly);
      }

      if(ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow))
      {
        CameraManip.keyMotion(factor, 0, nvh::CameraManipulator::Pan);
      }

      if(ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow))
      {
        CameraManip.keyMotion(-factor, 0, nvh::CameraManipulator::Pan);
      }

      if(ImGui::IsKeyDown(ImGuiKey_UpArrow))
      {
        CameraManip.keyMotion(0, factor, nvh::CameraManipulator::Pan);
      }

      if(ImGui::IsKeyDown(ImGuiKey_DownArrow))
      {
        CameraManip.keyMotion(0, -factor, nvh::CameraManipulator::Pan);
      }
    }

    if(ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)
       || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
      CameraManip.setMousePosition(static_cast<int>(mouse_pos.x), static_cast<int>(mouse_pos.y));
    }

    if(ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.0F) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 1.0F)
       || ImGui::IsMouseDragging(ImGuiMouseButton_Right, 1.0F))
    {
      CameraManip.mouseMove(static_cast<int>(mouse_pos.x), static_cast<int>(mouse_pos.y), m_inputs);
    }

    // Mouse Wheel
    if(ImGui::GetIO().MouseWheel != 0.0F)
    {
      CameraManip.wheel(static_cast<int>(ImGui::GetIO().MouseWheel * 3), m_inputs);
    }
  }

  ImVec2                         m_viewportSize{0, 0};
  float                          m_sceneRadius{10.0F};
  nvh::CameraManipulator::Inputs m_inputs;  // Mouse button pressed
};

}  // namespace nvvkhl
