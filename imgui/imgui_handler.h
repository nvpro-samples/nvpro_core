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


// This is a helper class to manage settings in ImGui. It allows to easily add settings to the ImGui settings handler

// Example
// m_settingsHandler.setHandlerName("MyHandlerName");
// m_settingsHandler.setSetting("ShowLog", &m_showLog);
// m_settingsHandler.setSetting("LogLevel", &m_logger);
// m_settingsHandler.addImGuiHandler();
#pragma once

#include <string>
#include <sstream>
#include <glm/glm.hpp>
#include <functional>

namespace ImGuiH {

class SettingsHandler
{

public:
  SettingsHandler();
  explicit SettingsHandler(const std::string& name);

  void setHandlerName(const std::string& name) { handlerName = name; }
  void addImGuiHandler();

  template <typename T>
  void setSetting(const std::string& key, T* value);

private:
  struct SettingEntry
  {
    void*                                   ptr{};
    std::function<void(const std::string&)> fromString;
    std::function<std::string()>            toString;
  };

  std::string                                   handlerName;
  std::unordered_map<std::string, SettingEntry> settings;
};

// Default setting for int, float, double, vec2, vec3, bool, ...
template <typename T>
void ImGuiH::SettingsHandler::setSetting(const std::string& key, T* value)
{
  SettingEntry entry{.ptr = value};
  // value=23.3,45.2
  if constexpr(std::is_same_v<T, glm::ivec2> || std::is_same_v<T, glm::uvec2> || std::is_same_v<T, glm::vec2>)
  {
    entry.fromString = [value](const std::string& str) {
      std::stringstream ss(str);
      char              comma;
      ss >> value->x >> comma >> value->y;
    };
    entry.toString = [value]() { return std::to_string(value->x) + "," + std::to_string(value->y); };
  }
  // value=2.1,4.3,6.5
  else if constexpr(std::is_same_v<T, glm::ivec3> || std::is_same_v<T, glm::uvec3> || std::is_same_v<T, glm::vec3>)
  {
    entry.fromString = [value](const std::string& str) {
      std::stringstream ss(str);
      char              comma;
      ss >> value->x >> comma >> value->y >> comma >> value->z;
    };
    entry.toString = [value]() {
      return std::to_string(value->x) + "," + std::to_string(value->y) + "," + std::to_string(value->z);
    };
  }
  // value=true or value=false
  else if constexpr(std::is_same_v<T, bool>)
  {
    entry.fromString = [value](const std::string& str) { *value = (str == "true"); };
    entry.toString   = [value]() { return (*value) ? "true" : "false"; };
  }
  // All other type
  else
  {
    entry.fromString = [value](const std::string& str) {
      std::istringstream iss(str);
      iss >> *value;
    };
    entry.toString = [value]() {
      std::ostringstream oss;
      oss << *value;
      return oss.str();
    };
  }
  settings[key] = std::move(entry);
}


}  // namespace ImGuiH
