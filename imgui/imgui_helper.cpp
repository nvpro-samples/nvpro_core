/*-----------------------------------------------------------------------
Copyright (c) 2018, NVIDIA. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Neither the name of its contributors may be used to endorse
or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/
#include "imgui_helper.h"

namespace ImGuiH {

  void Init(int width, int height, void* userData)
  {
    ImGui::CreateContext();
    auto &imgui_io = ImGui::GetIO();
    imgui_io.IniFilename = nullptr;
    imgui_io.Fonts->AddFontDefault();
    imgui_io.UserData = userData;
    imgui_io.DisplaySize = ImVec2(float(width), float(height));
    imgui_io.KeyMap[ImGuiKey_Tab] = NVPWindow::KEY_TAB;
    imgui_io.KeyMap[ImGuiKey_LeftArrow] = NVPWindow::KEY_LEFT;
    imgui_io.KeyMap[ImGuiKey_RightArrow] = NVPWindow::KEY_RIGHT;
    imgui_io.KeyMap[ImGuiKey_UpArrow] = NVPWindow::KEY_UP;
    imgui_io.KeyMap[ImGuiKey_DownArrow] = NVPWindow::KEY_DOWN;
    imgui_io.KeyMap[ImGuiKey_PageUp] = NVPWindow::KEY_PAGE_UP;
    imgui_io.KeyMap[ImGuiKey_PageDown] = NVPWindow::KEY_PAGE_DOWN;
    imgui_io.KeyMap[ImGuiKey_Home] = NVPWindow::KEY_HOME;
    imgui_io.KeyMap[ImGuiKey_End] = NVPWindow::KEY_END;
    imgui_io.KeyMap[ImGuiKey_Insert] = NVPWindow::KEY_INSERT;
    imgui_io.KeyMap[ImGuiKey_Delete] = NVPWindow::KEY_DELETE;
    imgui_io.KeyMap[ImGuiKey_Backspace] = NVPWindow::KEY_BACKSPACE;
    imgui_io.KeyMap[ImGuiKey_Space] = NVPWindow::KEY_SPACE;
    imgui_io.KeyMap[ImGuiKey_Enter] = NVPWindow::KEY_ENTER;
    imgui_io.KeyMap[ImGuiKey_Escape] = NVPWindow::KEY_ESCAPE;
    imgui_io.KeyMap[ImGuiKey_A] = NVPWindow::KEY_A;
    imgui_io.KeyMap[ImGuiKey_C] = NVPWindow::KEY_C;
    imgui_io.KeyMap[ImGuiKey_V] = NVPWindow::KEY_V;
    imgui_io.KeyMap[ImGuiKey_X] = NVPWindow::KEY_X;
    imgui_io.KeyMap[ImGuiKey_Y] = NVPWindow::KEY_Y;
    imgui_io.KeyMap[ImGuiKey_Z] = NVPWindow::KEY_Z;
  }

  void Combo(const char* label, size_t numEnums, const Enum* enums, void* valuePtr, ImGuiComboFlags flags, ValueType valueType, bool *valueChanged)
  {
    int*   ivalue = (int*)valuePtr;
    float* fvalue = (float*)valuePtr;

    size_t idx = 0;
    bool found = false;
    for (size_t i = 0; i < numEnums; i++) {
      switch (valueType) {
      case TYPE_INT:
        if (enums[i].ivalue == *ivalue) {
          idx = i;
          found = true;
        }
        break;
      case TYPE_FLOAT:
        if (enums[i].fvalue == *fvalue) {
          idx = i;
          found = true;
        }
        break;
      }
    }

    if (ImGui::BeginCombo(label, enums[idx].name.c_str(), flags)) // The second parameter is the label previewed before opening the combo.
    {
      for (size_t i = 0; i < numEnums; i++)
      {
        bool is_selected = i == idx;
        if (ImGui::Selectable(enums[i].name.c_str(), is_selected)) {
          switch (valueType) {
          case TYPE_INT:
            *ivalue = enums[i].ivalue;
            break;
          case TYPE_FLOAT:
            *fvalue = enums[i].fvalue;
            break;
          }
          if (valueChanged) *valueChanged = true;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
        }
      }
      ImGui::EndCombo();
    }
  }
}

