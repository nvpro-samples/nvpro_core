/*
 * Copyright (c) 2022-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "imgui_handler.h"

#include <imgui_internal.h>

ImGuiH::SettingsHandler::SettingsHandler(const std::string& name)
{
  setHandlerName(name);
}

ImGuiH::SettingsHandler::SettingsHandler() {}

// This is reading the section in the .ini file and for each entry read or write the value
// ex:
// [Application][State]
// WindowWidth=1513
// WindowHeight=871
//
void ImGuiH::SettingsHandler::addImGuiHandler()
{
  assert(!handlerName.empty());
  ImGuiSettingsHandler ini_handler{};
  ini_handler.TypeName   = handlerName.c_str();
  ini_handler.TypeHash   = ImHashStr(handlerName.c_str());
  ini_handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char*) -> void* { return (void*)1; };
  // Read line by line and send the string after the `=` as value
  ini_handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, void*, const char* line) {
    SettingsHandler* s = static_cast<SettingsHandler*>(handler->UserData);
    char             key[64], value[256];
    key[63]    = 0;  // zero terminate, protection
    value[255] = 0;  // zero terminate, protection
#ifdef _MSC_VER
    if(sscanf_s(line, "%63[^=]=%255[^\n]", key, (unsigned)_countof(key), value, (unsigned)_countof(value)) == 2)
#else
    if(sscanf(line, "%63[^=]=%255[^\n]", key, value) == 2)
#endif
    {
      auto it = s->settings.find(key);
      if(it != s->settings.end())
      {
        it->second.fromString(value);
      }
    }
  };
  // Write the ["name"][State], then one line for each setting
  ini_handler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
    SettingsHandler* s = static_cast<SettingsHandler*>(handler->UserData);
    buf->appendf("[%s][State]\n", handler->TypeName);
    for(const auto& [key, entry] : s->settings)
    {
      buf->appendf("%s=%s\n", key.c_str(), entry.toString().c_str());
    }
    buf->appendf("\n");
  };
  ini_handler.UserData = this;
  ImGui::AddSettingsHandler(&ini_handler);
}
