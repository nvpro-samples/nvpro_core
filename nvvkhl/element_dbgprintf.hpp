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

//--------------------------------------------------------------------------------------------------
// This is for easily adding debug printf to a sample using nvvkhl::application.
//
//  Create the element such that it will be available to the target application
//    - Example:
//        std::shared_ptr<nvvkhl::ElementDbgPrintf> g_dbgPrintf = std::make_shared<nvvkhl::ElementDbgPrintf>();
//  Add to main
//    - Before creating the nvvkhl::Application, set:
//        spec.vkSetup.instanceCreateInfoExt = g_dbgPrintf->getFeatures();
//    - Add the Element to the Application
//        app->addElement(g_dbgPrintf);
//  In the target application, push the mouse coordinated
//    - m_pushConst.mouseCoord = g_dbgPrintf->getMouseCoord();
//
//  In the Shader, do:
//    - Add the extension
//        #extension GL_EXT_debug_printf : enable
//    - Where to get the information
//        ivec2 fragCoord = ivec2(floor(gl_FragCoord.xy));
//        if(fragCoord == ivec2(pushC.mouseCoord))
//          debugPrintfEXT("Value: %f\n", myVal);
//
class ElementDbgPrintf : public nvvkhl::IAppElement
{
public:
  ElementDbgPrintf() = default;

  static VkValidationFeaturesEXT* getFeatures()
  {
    // #debug_printf
    static VkValidationFeaturesEXT                    features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    static std::vector<VkValidationFeatureEnableEXT>  enables{VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};
    static std::vector<VkValidationFeatureDisableEXT> disables{};
    features.enabledValidationFeatureCount  = static_cast<uint32_t>(enables.size());
    features.pEnabledValidationFeatures     = enables.data();
    features.disabledValidationFeatureCount = static_cast<uint32_t>(disables.size());
    features.pDisabledValidationFeatures    = disables.data();
    return &features;
  }

  // Return the relative mouse coordinates in the window named "Viewport"
  nvmath::vec2f getMouseCoord() { return m_mouseCoord; }


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
      clean_msg             = clean_msg.substr(clean_msg.find_last_of('|') + 1);
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
      const nvmath::vec2f mouse_pos = {ImGui::GetMousePos().x, ImGui::GetMousePos().y};  // Current mouse pos in window
      const nvmath::vec2f corner    = {window->Pos.x, window->Pos.y};                    // Corner of the viewport
      m_mouseCoord                  = mouse_pos - corner;
    }
    else
    {
      m_mouseCoord = {-1, -1};
    }
  }

private:
  VkInstance               m_instance     = {};
  nvmath::vec2f            m_mouseCoord   = {-1, -1};
  VkDebugUtilsMessengerEXT m_dbgMessenger = {};
};

}  // namespace nvvkhl