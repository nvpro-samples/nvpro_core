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

#ifndef NV_IMGUI_INCLUDED
#define NV_IMGUI_INCLUDED

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS 1
#include "imgui.h"

#include <nvpwindow.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace ImGuiH {
//////////////////////////////////////////////////////////////////////////

// NVPWindow callbacks

inline bool mouse_pos(int x, int y)
{
  auto& io    = ImGui::GetIO();
  io.MousePos = ImVec2(float(x), float(y));
  return io.WantCaptureMouse;
}

inline bool mouse_button(int button, int action)
{
  auto& io             = ImGui::GetIO();
  io.MouseDown[button] = action == NVPWindow::BUTTON_PRESS;
  return io.WantCaptureMouse;
}

inline bool mouse_wheel(int wheel)
{
  auto& io      = ImGui::GetIO();
  io.MouseWheel = static_cast<float>(wheel);
  return io.WantCaptureMouse;
}

inline bool key_char(int button)
{
  auto& io = ImGui::GetIO();
  io.AddInputCharacter(static_cast<unsigned int>(button));
  return io.WantCaptureKeyboard;
}

inline bool key_button(int button, int action, int mods)
{
  if(button == NVPWindow::KEY_KP_ENTER)
  {
    button = NVPWindow::KEY_ENTER;
  }

  auto& io            = ImGui::GetIO();
  io.KeyCtrl          = (mods & NVPWindow::KMOD_CONTROL) != 0;
  io.KeyShift         = (mods & NVPWindow::KMOD_SHIFT) != 0;
  io.KeyAlt           = (mods & NVPWindow::KMOD_ALT) != 0;
  io.KeySuper         = (mods & NVPWindow::KMOD_SUPER) != 0;
  io.KeysDown[button] = action == NVPWindow::BUTTON_PRESS;
  return io.WantCaptureKeyboard;
}

//////////////////////////////////////////////////////////////////////////

void Init(int width, int height, void* userData);

template <typename T>
bool Clamped(bool changed, T* value, T min, T max)
{
  *value = std::max(min, std::min(max, *value));
  return changed;
}

inline bool InputIntClamped(const char*         label,
                            unsigned int*       v,
                            int                 min       = INT_MIN,
                            int                 max       = INT_MAX,
                            int                 step      = 1,
                            int                 step_fast = 100,
                            ImGuiInputTextFlags flags     = 0)
{
  return Clamped(ImGui::InputInt(label, (int*)v, step, step_fast, flags), (int*)v, min, max);
}

inline bool InputIntClamped(const char* label, int* v, int min = INT_MIN, int max = INT_MAX, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0)
{
  return Clamped(ImGui::InputInt(label, v, step, step_fast, flags), v, min, max);
}

inline bool InputFloatClamped(const char*         label,
                              float*              v,
                              float               min       = 0.0,
                              float               max       = 1.0,
                              float               step      = 0.1f,
                              float               step_fast = 1.0f,
                              const char*         fmt       = "%.3f",
                              ImGuiInputTextFlags flags     = 0)
{
  return Clamped(ImGui::InputFloat(label, v, step, step_fast, fmt, flags), v, min, max);
}

enum ValueType
{
  TYPE_INT,
  TYPE_FLOAT,
  TYPE_BOOL
};


struct Enum
{
  union
  {
    int   ivalue;
    float fvalue;
    bool  bvalue;
  };
  std::string name;
};

void Combo(const char*     label,
           size_t          numEnums,
           const Enum*     enums,
           void*           value,
           ImGuiComboFlags flags        = 0,
           ValueType       valueType    = TYPE_INT,
           bool*           valueChanged = NULL);

class Registry
{
private:
  struct Entry
  {
    std::vector<Enum> enums;
    ValueType         valueType;
    bool              valueChanged;
  };
  std::vector<Entry> entries;

public:
  const std::vector<Enum>& getEnums(uint32_t type) const { return entries[type].enums; }

  void checkboxAdd(uint32_t type, bool value = false)
  {
    if(type >= entries.size())
    {
      entries.resize(type + 1);
    }
    entries[type].enums.push_back({value, std::string("Bool")});
    entries[type].valueChanged = false;
    entries[type].valueType    = TYPE_BOOL;
  }

  void inputIntAdd(uint32_t type, int value = 0)
  {
    if(type >= entries.size())
    {
      entries.resize(type + 1);
    }
    entries[type].enums.push_back({value, std::string("iI")});
    entries[type].valueChanged = false;
    entries[type].valueType    = TYPE_INT;
  }

  void inputFloatAdd(uint32_t type, float value = .0f)
  {
    if(type >= entries.size())
    {
      entries.resize(type + 1);
    }
    Enum e;
    e.fvalue = value;
    e.name   = std::string("iF");
    entries[type].enums.push_back(e);
    entries[type].valueChanged = false;
    entries[type].valueType    = TYPE_FLOAT;
  }

  void enumAdd(uint32_t type, int value, const char* name)
  {
    if(type >= entries.size())
    {
      entries.resize(type + 1);
    }
    entries[type].enums.push_back({value, name});
    entries[type].valueChanged = false;
    entries[type].valueType = TYPE_INT;  // the user must be consistent so that he adds only the same type for the same combo !
  }

  void enumAdd(uint32_t type, float value, const char* name)
  {
    if(type >= entries.size())
    {
      entries.resize(type + 1);
    }
    Enum e;
    e.fvalue = value;
    e.name   = name;
    entries[type].enums.push_back(e);
    entries[type].valueChanged = false;
    entries[type].valueType = TYPE_FLOAT;  // the user must be consistent so that he adds only the same type for the same combo !
  }

  void enumReset(uint32_t type)
  {
    if(type < entries.size())
    {
      entries[type].enums.clear();
      entries[type].valueChanged = false;
      entries[type].valueType    = TYPE_INT;
    }
  }

  void enumCombobox(uint32_t type, const char* label, void* value, ImGuiComboFlags flags = 0, bool* valueChanged = NULL)
  {
    Combo(label, entries[type].enums.size(), entries[type].enums.data(), value, flags, entries[type].valueType,
          &entries[type].valueChanged);
    if(valueChanged)
    {
      *valueChanged = entries[type].valueChanged;
    }
  }

  void checkbox(uint32_t type, const char* label, void* value, ImGuiComboFlags flags = 0, bool* valueChanged = NULL)
  {
    bool* pB        = &entries[type].enums[0].bvalue;
    bool  prevValue = *pB;
    ImGui::Checkbox(label, pB);
    if(prevValue != *pB)
    {
      entries[type].valueChanged = true;
    }
    if(valueChanged)
    {
      *valueChanged = entries[type].valueChanged;
    }
    if(value)
    {
      ((bool*)value)[0] = pB[0];
    }
  }

  void inputIntClamped(uint32_t type, const char* label, int* v, int min = INT_MIN, int max = INT_MAX, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0)
  {
    int* pI        = &entries[type].enums[0].ivalue;
    int  prevValue = *pI;
    bool bRes      = Clamped(ImGui::InputInt(label, pI, step, step_fast, flags), pI, min, max);
    if(prevValue != *pI)
    {
      entries[type].valueChanged = true;
    }
    if(v)
    {
      ((int*)v)[0] = pI[0];
    }
  }

  //void inputFloatClamped(uint32_t type, const char* label, float* v, float min = 0.0f, int max = 100.0f, int step = 0.1, int step_fast = 1.0, ImGuiInputTextFlags flags = 0) {
  //    float *pF = &entries[type].enums[0].fvalue;
  //    float prevValue = *pF;
  //    bool bRes = Clamped(ImGui::InputFloat(label, pF, step, step_fast, flags), pF, min, max);
  //    if (prevValue != *pF)
  //        entries[type].valueChanged = true;
  //    if (v)
  //        ((float*)v)[0] = pF[0];
  //}

  bool checkValueChange(uint32_t type, bool reset = true)
  {
    bool changed = entries[type].valueChanged;
    if(reset && changed)
    {
      entries[type].valueChanged = false;
    }
    return changed;
  }
};
}  // namespace ImGuiH

#endif
