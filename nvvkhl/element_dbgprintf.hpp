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

#include "application.hpp"
#include "nvh/nvprint.hpp"
#include "nvvk/error_vk.hpp"

#include "imgui.h"
#include "imgui_internal.h"


namespace nvvkhl {

/** @DOC_START
# class nvvkhl::ElementDbgPrintf

>  This class is an element of the application that is responsible for the debug printf in the shader. It is using the `VK_EXT_debug_printf` extension to print information from the shader.

To use this class, you need to add it to the `nvvkhl::Application` using the `addElement` method.

  Create the element such that it will be available to the target application
  - Example:
    ```cpp
    std::shared_ptr<nvvkhl::ElementDbgPrintf> g_dbgPrintf = std::make_shared<nvvkhl::ElementDbgPrintf>();
    ```
  
  Add to main
  - Before creating the nvvkhl::Application, set:
    ` spec.vkSetup.instanceCreateInfoExt = g_dbgPrintf->getFeatures(); `
  - Add the Element to the Application
    ` app->addElement(g_dbgPrintf); `
  - In the target application, push the mouse coordinated
    ` m_pushConst.mouseCoord = g_dbgPrintf->getMouseCoord(); `

  In the Shader, do:
  - Add the extension
    ` #extension GL_EXT_debug_printf : enable `
  - Where to get the information
    ```cpp
    ivec2 fragCoord = ivec2(floor(gl_FragCoord.xy));
    if(fragCoord == ivec2(pushC.mouseCoord))
      debugPrintfEXT("Value: %f\n", myVal);
    ```
@DOC_END */

class ElementDbgPrintf : public nvvkhl::IAppElement
{
public:
  ElementDbgPrintf() = default;

  static VkLayerSettingsCreateInfoEXT* getFeatures()
  {
    // #debug_printf
    // Adding the GPU debug information to the KHRONOS validation layer
    // See: https://vulkan.lunarg.com/doc/sdk/1.3.275.0/linux/khronos_validation_layer.html
    static const char*    layer_name           = "VK_LAYER_KHRONOS_validation";
    static const char*    validate_gpu_based[] = {"GPU_BASED_DEBUG_PRINTF"};
    static const VkBool32 printf_verbose       = VK_FALSE;
    static const VkBool32 printf_to_stdout     = VK_FALSE;
    static const int32_t  printf_buffer_size   = 1024;

    static const VkLayerSettingEXT settings[] = {
        {layer_name, "validate_gpu_based", VK_LAYER_SETTING_TYPE_STRING_EXT,
         static_cast<uint32_t>(std::size(validate_gpu_based)), &validate_gpu_based},
        {layer_name, "printf_verbose", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &printf_verbose},
        {layer_name, "printf_to_stdout", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &printf_to_stdout},
        {layer_name, "printf_buffer_size", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &printf_buffer_size},
    };

    static VkLayerSettingsCreateInfoEXT layer_settings_create_info = {
        .sType        = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
        .settingCount = static_cast<uint32_t>(std::size(settings)),
        .pSettings    = settings,
    };
    return &layer_settings_create_info;
  }

  // Return the relative mouse coordinates in the window named "Viewport"
  glm::vec2 getMouseCoord() { return m_mouseCoord; }


  void onAttach(Application* app) override
  {
    m_instance = app->getContext()->m_instance;
    // Vulkan message callback - for receiving the printf in the shader
    // Note: there is already a callback in nvvk::Context, but by defaut it is not printing INFO severity
    //       this callback will catch the message and will make it clean for display.
    auto dbg_messenger_callback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                     const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) -> VkBool32 {
      // Get rid of all the extra message we don't need
      std::string clean_msg = callbackData->pMessage;

      const std::string searchStr = "vkQueueSubmit(): ";
      std::size_t       pos       = clean_msg.find(searchStr);
      if(pos != std::string::npos)
      {
        clean_msg = clean_msg.substr(pos + searchStr.size() + 1);  // Remove everything before the search string
      }
      LOGI("%s", clean_msg.c_str());  // <- This will end up in the Logger
      return VK_FALSE;                // to continue
    };

    // Creating the callback
    VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    dbg_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    dbg_messenger_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    dbg_messenger_create_info.pfnUserCallback = dbg_messenger_callback;
    NVVK_CHECK(vkCreateDebugUtilsMessengerEXT(m_instance, &dbg_messenger_create_info, nullptr, &m_dbgMessenger));
  }

  void onDetach() override { vkDestroyDebugUtilsMessengerEXT(m_instance, m_dbgMessenger, nullptr); }

  void onUIRender() override
  {
    // Pick the mouse coordinate if the mouse is down
    if(ImGui::GetIO().MouseDown[0])
    {
      ImGuiWindow* window = ImGui::FindWindowByName("Viewport");
      assert(window);
      const glm::vec2 mouse_pos = {ImGui::GetMousePos().x, ImGui::GetMousePos().y};  // Current mouse pos in window
      const glm::vec2 corner    = {window->Pos.x, window->Pos.y};                    // Corner of the viewport
      m_mouseCoord              = mouse_pos - corner;
    }
    else
    {
      m_mouseCoord = {-1, -1};
    }
  }

private:
  VkInstance               m_instance     = {};
  glm::vec2                m_mouseCoord   = {-1, -1};
  VkDebugUtilsMessengerEXT m_dbgMessenger = {};
};

}  // namespace nvvkhl