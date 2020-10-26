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
#include "nvmath/nvmath.h"
#include "nvmath/nvmath_glsltypes.h"

#include <fstream>

namespace ImGuiH {

void Init(int width, int height, void* userData)
{
  ImGui::CreateContext();
  auto& imgui_io       = ImGui::GetIO();
  imgui_io.IniFilename = nullptr;
  imgui_io.Fonts->AddFontDefault();
  imgui_io.UserData                    = userData;
  imgui_io.DisplaySize                 = ImVec2(float(width), float(height));
  imgui_io.KeyMap[ImGuiKey_Tab]        = NVPWindow::KEY_TAB;
  imgui_io.KeyMap[ImGuiKey_LeftArrow]  = NVPWindow::KEY_LEFT;
  imgui_io.KeyMap[ImGuiKey_RightArrow] = NVPWindow::KEY_RIGHT;
  imgui_io.KeyMap[ImGuiKey_UpArrow]    = NVPWindow::KEY_UP;
  imgui_io.KeyMap[ImGuiKey_DownArrow]  = NVPWindow::KEY_DOWN;
  imgui_io.KeyMap[ImGuiKey_PageUp]     = NVPWindow::KEY_PAGE_UP;
  imgui_io.KeyMap[ImGuiKey_PageDown]   = NVPWindow::KEY_PAGE_DOWN;
  imgui_io.KeyMap[ImGuiKey_Home]       = NVPWindow::KEY_HOME;
  imgui_io.KeyMap[ImGuiKey_End]        = NVPWindow::KEY_END;
  imgui_io.KeyMap[ImGuiKey_Insert]     = NVPWindow::KEY_INSERT;
  imgui_io.KeyMap[ImGuiKey_Delete]     = NVPWindow::KEY_DELETE;
  imgui_io.KeyMap[ImGuiKey_Backspace]  = NVPWindow::KEY_BACKSPACE;
  imgui_io.KeyMap[ImGuiKey_Space]      = NVPWindow::KEY_SPACE;
  imgui_io.KeyMap[ImGuiKey_Enter]      = NVPWindow::KEY_ENTER;
  imgui_io.KeyMap[ImGuiKey_Escape]     = NVPWindow::KEY_ESCAPE;
  imgui_io.KeyMap[ImGuiKey_A]          = NVPWindow::KEY_A;
  imgui_io.KeyMap[ImGuiKey_C]          = NVPWindow::KEY_C;
  imgui_io.KeyMap[ImGuiKey_V]          = NVPWindow::KEY_V;
  imgui_io.KeyMap[ImGuiKey_X]          = NVPWindow::KEY_X;
  imgui_io.KeyMap[ImGuiKey_Y]          = NVPWindow::KEY_Y;
  imgui_io.KeyMap[ImGuiKey_Z]          = NVPWindow::KEY_Z;
}

void Combo(const char* label, size_t numEnums, const Enum* enums, void* valuePtr, ImGuiComboFlags flags, ValueType valueType, bool* valueChanged)
{
  int*   ivalue = (int*)valuePtr;
  float* fvalue = (float*)valuePtr;

  size_t idx   = 0;
  bool   found = false;
  for(size_t i = 0; i < numEnums; i++)
  {
    switch(valueType)
    {
      case TYPE_INT:
        if(enums[i].ivalue == *ivalue)
        {
          idx   = i;
          found = true;
        }
        break;
      case TYPE_FLOAT:
        if(enums[i].fvalue == *fvalue)
        {
          idx   = i;
          found = true;
        }
        break;
    }
  }

  if(ImGui::BeginCombo(label, enums[idx].name.c_str(), flags))  // The second parameter is the label previewed before opening the combo.
  {
    for(size_t i = 0; i < numEnums; i++)
    {
      bool is_selected = i == idx;
      if(ImGui::Selectable(enums[i].name.c_str(), is_selected))
      {
        switch(valueType)
        {
          case TYPE_INT:
            *ivalue = enums[i].ivalue;
            break;
          case TYPE_FLOAT:
            *fvalue = enums[i].fvalue;
            break;
        }
        if(valueChanged)
          *valueChanged = true;
      }
      if(is_selected)
      {
        ImGui::SetItemDefaultFocus();  // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
      }
    }
    ImGui::EndCombo();
  }
}

//--------------------------------------------------------------------------------------------------
// Setting a dark style for the GUI
//
void setStyle()
{
  ImGui::StyleColorsDark();

  ImGuiStyle& style                  = ImGui::GetStyle();
  style.WindowRounding               = 0.0f;
  style.WindowBorderSize             = 0.0f;
  style.ColorButtonPosition          = ImGuiDir_Left;
  style.FrameRounding                = 2.0f;
  style.FrameBorderSize              = 1.0f;
  style.GrabRounding                 = 4.0f;
  style.IndentSpacing                = 12.0f;
  style.Colors[ImGuiCol_WindowBg]    = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
  style.Colors[ImGuiCol_MenuBarBg]   = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
  style.Colors[ImGuiCol_PopupBg]     = ImVec4(0.135f, 0.135f, 0.135f, 1.0f);
  style.Colors[ImGuiCol_Border]      = ImVec4(0.4f, 0.4f, 0.4f, 0.5f);
  style.Colors[ImGuiCol_FrameBg]     = ImVec4(0.05f, 0.05f, 0.05f, 0.5f);

  std::vector<ImGuiCol> to_change;
  to_change.push_back(ImGuiCol_Header);
  to_change.push_back(ImGuiCol_HeaderActive);
  to_change.push_back(ImGuiCol_HeaderHovered);
  to_change.push_back(ImGuiCol_SliderGrab);
  to_change.push_back(ImGuiCol_SliderGrabActive);
  to_change.push_back(ImGuiCol_Button);
  to_change.push_back(ImGuiCol_ButtonActive);
  to_change.push_back(ImGuiCol_ButtonHovered);
  to_change.push_back(ImGuiCol_FrameBgActive);
  to_change.push_back(ImGuiCol_FrameBgHovered);
  to_change.push_back(ImGuiCol_CheckMark);
  to_change.push_back(ImGuiCol_ResizeGrip);
  to_change.push_back(ImGuiCol_ResizeGripActive);
  to_change.push_back(ImGuiCol_ResizeGripHovered);
  to_change.push_back(ImGuiCol_TextSelectedBg);
  to_change.push_back(ImGuiCol_Separator);
  to_change.push_back(ImGuiCol_SeparatorHovered);
  to_change.push_back(ImGuiCol_SeparatorActive);
  for(auto c : to_change)
  {
    style.Colors[c].x = 0.465f;
    style.Colors[c].y = 0.495f;
    style.Colors[c].z = 0.525f;
  }

  style.Colors[ImGuiCol_TitleBgActive]    = ImVec4(0.465f, 0.465f, 0.465f, 1.0f);
  style.Colors[ImGuiCol_TitleBg]          = ImVec4(0.125f, 0.125f, 0.125f, 1.0f);
  style.Colors[ImGuiCol_Tab]              = ImVec4(0.05f, 0.05f, 0.05f, 0.5f);
  style.Colors[ImGuiCol_TabHovered]       = ImVec4(0.465f, 0.495f, 0.525f, 1.0f);
  style.Colors[ImGuiCol_TabActive]        = ImVec4(0.282f, 0.290f, 0.302f, 1.0f);
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.465f, 0.465f, 0.465f, 0.350f);

  //Colors_ext[ImGuiColExt_Warning] = ImVec4 (1.0f, 0.43f, 0.35f, 1.0f);

  ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
}

//
// Local, return true if the filename exist
//
static bool fileExists(const char* filename)
{
  std::ifstream stream;
  stream.open(filename);
  return stream.is_open();
}

//--------------------------------------------------------------------------------------------------
// Looking for TTF fonts, first on the VULKAN SDK, then Windows default fonts
//
void setFonts()
{
  ImGuiIO& io = ImGui::GetIO();

  // Nicer fonts
  ImFont*     font    = nullptr;
  const char* vk_path = getenv("VK_SDK_PATH");
  if(vk_path)
  {
    const std::string p = std::string(vk_path) + R"(\Samples\Layer-Samples\data\FreeSans.ttf)";
    if(fileExists(p.c_str()))
      font = io.Fonts->AddFontFromFileTTF(p.c_str(), 16.0f);
  }
  if(font == nullptr)
  {
    const std::string p = R"(C:\\Windows\\Fonts\\SegoeWP.ttf)";
    if(fileExists(p.c_str()))
      font = io.Fonts->AddFontFromFileTTF(p.c_str(), 16.0f);
  }

  if(font == nullptr)
    io.Fonts->AddFontDefault();
}

// ------------------------------------------------------------------------------------------------
template <>
void Control::show_tooltip(const char* description)
{
  if(!description || strlen(description) == 0)
    return;

  ImGui::BeginTooltip();
  ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
  ImGui::TextUnformatted(description);
  ImGui::PopTextWrapPos();
  ImGui::EndTooltip();
}

// ------------------------------------------------------------------------------------------------
template <>
void Control::show_tooltip(const std::string& description)
{
  if(description.empty())
    return;
  show_tooltip<const char*>(description.c_str());
}

// ------------------------------------------------------------------------------------------------

namespace {

template <typename TScalar, ImGuiDataType type, uint8_t dim>
bool show_slider_control_scalar(TScalar* value, TScalar* min, TScalar* max, const char* format)
{
  static const char* visible_labels[] = {"x:", "y:", "z:", "w:"};

  if(dim == 1)
    return ImGui::SliderScalar("##hidden", type, &value[0], &min[0], &max[0], format);

  float indent  = ImGui::GetCursorPos().x;
  bool  changed = false;
  for(uint8_t c = 0; c < dim; ++c)
  {
    ImGui::PushID(c);
    if(c > 0)
    {
      ImGui::NewLine();
      ImGui::SameLine(indent);
    }
    ImGui::Text("%s", visible_labels[c]);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    changed |= ImGui::SliderScalar("##hidden", type, &value[c], &min[c], &max[c], format);
    ImGui::PopID();
  }
  return changed;
}


}  // namespace

template <>
bool Control::show_slider_control<float>(float* value, float& min, float& max, const char* format)
{
  return show_slider_control_scalar<float, ImGuiDataType_Float, 1>(value, &min, &max, format ? format : "%.3f");
}

template <>
bool Control::show_slider_control<nvmath::vec2f>(nvmath::vec2f* value, nvmath::vec2f& min, nvmath::vec2f& max, const char* format)
{
  return show_slider_control_scalar<float, ImGuiDataType_Float, 2>(&value->x, &min.x, &max.x, format ? format : "%.3f");
}

template <>
bool Control::show_slider_control<nvmath::vec3f>(nvmath::vec3f* value, nvmath::vec3f& min, nvmath::vec3f& max, const char* format)
{
  return show_slider_control_scalar<float, ImGuiDataType_Float, 3>(&value->x, &min.x, &max.x, format ? format : "%.3f");
}

template <>
bool Control::show_slider_control<nvmath::vec4f>(nvmath::vec4f* value, nvmath::vec4f& min, nvmath::vec4f& max, const char* format)
{
  return show_slider_control_scalar<float, ImGuiDataType_Float, 4>(&value->x, &min.x, &max.x, format ? format : "%.3f");
}

template <>
bool Control::show_drag_control<float>(float* value, float speed, float& min, float& max, const char* format)
{
  return show_drag_control_scalar<float, ImGuiDataType_Float, 1>(value, speed, &min, &max, format ? format : "%.3f");
}

template <>
bool Control::show_drag_control<nvmath::vec2f>(nvmath::vec2f* value, float speed, nvmath::vec2f& min, nvmath::vec2f& max, const char* format)
{
  return show_drag_control_scalar<float, ImGuiDataType_Float, 2>(&value->x, speed, &min.x, &max.x, format ? format : "%.3f");
}

template <>
bool Control::show_drag_control<nvmath::vec3>(nvmath::vec3* value, float speed, nvmath::vec3& min, nvmath::vec3& max, const char* format)
{
  return show_drag_control_scalar<float, ImGuiDataType_Float, 3>(&value->x, speed, &min.x, &max.x, format ? format : "%.3f");
}

template <>
bool Control::show_drag_control<nvmath::vec4f>(nvmath::vec4f* value, float speed, nvmath::vec4f& min, nvmath::vec4f& max, const char* format)
{
  return show_drag_control_scalar<float, ImGuiDataType_Float, 4>(&value->x, speed, &min.x, &max.x, format ? format : "%.3f");
}


template <>
bool Control::show_slider_control<int>(int* value, int& min, int& max, const char* format)
{
  return show_slider_control_scalar<int, ImGuiDataType_S32, 1>(value, &min, &max, format ? format : "%d");
}

template <>
bool Control::show_slider_control<nvmath::vec2i>(nvmath::vec2i* value, nvmath::vec2i& min, nvmath::vec2i& max, const char* format)
{
  return show_slider_control_scalar<int, ImGuiDataType_S32, 2>(&value->x, &min.x, &max.x, format ? format : "%d");
}

template <>
bool Control::show_slider_control<nvmath::vec3i>(nvmath::vec3i* value, nvmath::vec3i& min, nvmath::vec3i& max, const char* format)
{
  return show_slider_control_scalar<int, ImGuiDataType_S32, 3>(&value->x, &min.x, &max.x, format ? format : "%d");
}

template <>
bool Control::show_slider_control<nvmath::vec4i>(nvmath::vec4i* value, nvmath::vec4i& min, nvmath::vec4i& max, const char* format)
{
  return show_slider_control_scalar<int, ImGuiDataType_S32, 4>(&value->x, &min.x, &max.x, format ? format : "%d");
}

template <>
bool Control::show_drag_control<int>(int* value, float speed, int& min, int& max, const char* format)
{
  return show_drag_control_scalar<int, ImGuiDataType_S32, 1>(value, speed, &min, &max, format ? format : "%d");
}

template <>
bool Control::show_drag_control<nvmath::vec2i>(nvmath::vec2i* value, float speed, nvmath::vec2i& min, nvmath::vec2i& max, const char* format)
{
  return show_drag_control_scalar<int, ImGuiDataType_S32, 2>(&value->x, speed, &min.x, &max.x, format ? format : "%d");
}

template <>
bool Control::show_drag_control<nvmath::vec3i>(nvmath::vec3i* value, float speed, nvmath::vec3i& min, nvmath::vec3i& max, const char* format)
{
  return show_drag_control_scalar<int, ImGuiDataType_S32, 3>(&value->x, speed, &min.x, &max.x, format ? format : "%d");
}

template <>
bool Control::show_drag_control<nvmath::vec4i>(nvmath::vec4i* value, float speed, nvmath::vec4i& min, nvmath::vec4i& max, const char* format)
{
  return show_drag_control_scalar<int, ImGuiDataType_S32, 4>(&value->x, speed, &min.x, &max.x, format ? format : "%d");
}


template <>
bool Control::show_slider_control<uint32_t>(uint32_t* value, uint32_t& min, uint32_t& max, const char* format)
{
  return show_slider_control_scalar<uint32_t, ImGuiDataType_U32, 1>(value, &min, &max, format ? format : "%d");
}
//
//template <>
//bool Control::show_slider_control<uint32_t_2>(uint32_t_2* value, uint32_t_2& min, uint32_t_2& max, const char* format)
//{
//  return show_slider_control_scalar<uint32_t, ImGuiDataType_U32, 2>(&value->x, &min.x, &max.x, format ? format : "%d");
//}
//
//template <>
//bool Control::show_slider_control<uint32_t_3>(uint32_t_3* value, uint32_t_3& min, uint32_t_3& max, const char* format)
//{
//  return show_slider_control_scalar<uint32_t, ImGuiDataType_U32, 3>(&value->x, &min.x, &max.x, format ? format : "%d");
//}
//
//template <>
//bool Control::show_slider_control<uint32_t_4>(uint32_t_4* value, uint32_t_4& min, uint32_t_4& max, const char* format)
//{
//  return show_slider_control_scalar<uint32_t, ImGuiDataType_U32, 4>(&value->x, &min.x, &max.x, format ? format : "%d");
//}
//
//template <>
//bool Control::show_drag_control<uint32_t>(uint32_t* value, float speed, uint32_t& min, uint32_t& max, const char* format)
//{
//  return show_drag_control_scalar<uint32_t, ImGuiDataType_U32, 1>(value, speed, &min, &max, format ? format : "%d");
//}
//
//template <>
//bool Control::show_drag_control<uint32_t_2>(uint32_t_2* value, float speed, uint32_t_2& min, uint32_t_2& max, const char* format)
//{
//  return show_drag_control_scalar<uint32_t, ImGuiDataType_U32, 2>(&value->x, speed, &min.x, &max.x, format ? format : "%d");
//}
//
//template <>
//bool Control::show_drag_control<uint32_t_3>(uint32_t_3* value, float speed, uint32_t_3& min, uint32_t_3& max, const char* format)
//{
//  return show_drag_control_scalar<uint32_t, ImGuiDataType_U32, 3>(&value->x, speed, &min.x, &max.x, format ? format : "%d");
//}
//
//template <>
//bool Control::show_drag_control<uint32_t_4>(uint32_t_4* value, float speed, uint32_t_4& min, uint32_t_4& max, const char* format)
//{
//  return show_drag_control_scalar<uint32_t, ImGuiDataType_U32, 4>(&value->x, speed, &min.x, &max.x, format ? format : "%d");
//}


template <>
bool Control::show_slider_control<size_t>(size_t* value, size_t& min, size_t& max, const char* format)
{
  return show_slider_control_scalar<size_t, ImGuiDataType_U64, 1>(value, &min, &max, format ? format : "%d");
}

template <>
bool Control::show_drag_control<size_t>(size_t* value, float speed, size_t& min, size_t& max, const char* format)
{
  return show_drag_control_scalar<size_t, ImGuiDataType_U64, 1>(value, speed, &min, &max, format ? format : "%d");
}

// Static member declaration
Panel::Style   Panel::style{};
Control::Style Control::style{};

}  // namespace ImGuiH
