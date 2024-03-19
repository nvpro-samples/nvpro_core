/*
 * Copyright (c) 2022-2023, NVIDIA CORPORATION.  All rights reserved.
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

#include "imgui.h"
#include <glm/glm.hpp>

/*  @DOC_START -------------------------------------------------------
 
 Function `Axis(ImVec2 pos, const glm::mat4& modelView, float size = 20.f)`
 which display right-handed axis in a ImGui window.

 Example

 ```cpp
       {  // Display orientation axis at the bottom left corner of the window
        float  axisSize = 25.F;
        ImVec2 pos      = ImGui::GetWindowPos();
        pos.y += ImGui::GetWindowSize().y;
        pos += ImVec2(axisSize * 1.1F, -axisSize * 1.1F) * ImGui::GetWindowDpiScale();  // Offset
        ImGuiH::Axis(pos, CameraManip.getMatrix(), axisSize);
      }
```      

--- @DOC_END ------------------------------------------------------- */

// The API
namespace ImGuiH {

// This utility is adding the 3D axis at `pos`, using the matrix `modelView`
IMGUI_API void Axis(ImVec2 pos, const glm::mat4& modelView, float size = 20.f);

};  // namespace ImGuiH
