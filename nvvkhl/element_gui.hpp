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

#include "imgui.h"

#include "application.hpp"
#include "GLFW/glfw3.h"

// Use:
//  include this file at the end of all other includes,
//  and add engines
//
// Ex:
//   app->addEngine(std::make_shared<AppDefaultMenu>());
//

namespace nvvkhl {
//--------------------------------------------------------------------------------------------------
// Simple default Quit menu
//
class ElementDefaultMenu : public nvvkhl::IAppElement
{
public:
  void onAttach(nvvkhl::Application* app) override { m_app = app; }

  void onUIMenu() override
  {
    static bool close_app{false};
    bool        v_sync = m_app->isVsync();
#ifndef NDEBUG
    static bool show_demo{false};
#endif
    if(ImGui::BeginMenu("File"))
    {
      if(ImGui::MenuItem("Exit", "Ctrl+Q"))
      {
        close_app = true;
      }
      ImGui::EndMenu();
    }
    if(ImGui::BeginMenu("Tools"))
    {
      ImGui::MenuItem("V-Sync", "Ctrl+Shift+V", &v_sync);
#ifndef NDEBUG
      ImGui::MenuItem("Show Demo", nullptr, &show_demo);
#endif  // !NDEBUG
      ImGui::EndMenu();
    }

    // Shortcuts
    if(ImGui::IsKeyPressed(ImGuiKey_Q) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
    {
      close_app = true;
    }

    if(ImGui::IsKeyPressed(ImGuiKey_V) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyDown(ImGuiKey_LeftShift))
    {
      v_sync = !v_sync;
    }

    if(close_app)
    {
      m_app->close();
    }
#ifndef NDEBUG
    if(show_demo)
    {
      ImGui::ShowDemoWindow(&show_demo);
    }
#endif  // !NDEBUG

    if(m_app->isVsync() != v_sync)
    {
      m_app->setVsync(v_sync);
    }
  }

private:
  nvvkhl::Application* m_app{nullptr};
};


//--------------------------------------------------------------------------------------------------
// Display simple information in the window title
//
class ElementDefaultWindowTitle : public nvvkhl::IAppElement
{
public:
  void onAttach(nvvkhl::Application* app) override { m_app = app; }

  void onUIRender() override
  {
    // Window Title
    m_dirtyTimer += ImGui::GetIO().DeltaTime;
    if(m_dirtyTimer > 1.0F)  // Refresh every seconds
    {
      const auto&           size = m_app->getViewportSize();
      std::array<char, 256> buf{};
      if(snprintf(buf.data(), buf.size(), "%s %dx%d | %d FPS / %.3fms", PROJECT_NAME, static_cast<int>(size.width),
                  static_cast<int>(size.height), static_cast<int>(ImGui::GetIO().Framerate), 1000.F / ImGui::GetIO().Framerate)
         > 0)
      {
        glfwSetWindowTitle(m_app->getWindowHandle(), buf.data());
      }
      m_dirtyTimer = 0;
    }
  }

private:
  nvvkhl::Application* m_app{nullptr};
  float                m_dirtyTimer{0.0F};
};

}  // namespace nvvkhl
