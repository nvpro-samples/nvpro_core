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

#include "nvh/commandlineparser.hpp"
#include "nvh/nvprint.hpp"
#include "nvp/nvpsystem.hpp"
#include "nvh/timesampler.hpp"
#include "nvvk/error_vk.hpp"

namespace nvvkhl {

//--------------------------------------------------------------------------------------------------
// This testing element allow to
//  - Capture Vulkan validation errors, if any return error code 1
//  - Dump the result image to disk (CurrentPath/"name of project".bmp)
//
// At startup, it looks for arguments
//  --test (bool) Enable testing
//  --snapshot (bool) Saving or not image
//  --frames: (int) number of iteration frame the application do before sending a close signal

class ElementTesting : public nvvkhl::IAppElement
{
  struct Settings
  {
    bool     enabled{false};
    bool     snapshot{false};
    uint32_t maxFrames{5};
  } m_settings;

public:
  ElementTesting(int argc, char** argv)
  {
    nvh::CommandLineParser cmd_parser("");
    cmd_parser.addArgument({"--test"}, &m_settings.enabled, "Enable testing");
    cmd_parser.addArgument({"--snapshot"}, &m_settings.snapshot, "Take and save a snapshot");
    cmd_parser.addArgument({"--frames"}, &m_settings.maxFrames, "Max number of frames");
    cmd_parser.parse(argc, argv);
  };
  ~ElementTesting() = default;

  void onAttach(Application* app) override
  {
    m_app = app;
    m_startTime.reset();
    if(m_settings.enabled)
    {
      VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
      dbg_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT   // Vulkan issues
                                                  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;  // Invalid usage
      dbg_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT           // Other
                                              | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;     // Violation of spec
      dbg_messenger_create_info.pUserData = this;

      // Trapping in the callback the validation errors that could show up. If errors are found errorCode will return 1, otherwise 0
      dbg_messenger_create_info.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT        messageType,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
        ElementTesting* testing = reinterpret_cast<ElementTesting*>(userData);
        if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
          testing->addError(callbackData->pMessage);
        }
        return VK_FALSE;
      };
      NVVK_CHECK(vkCreateDebugUtilsMessengerEXT(m_app->getContext()->m_instance, &dbg_messenger_create_info, nullptr, &m_dbgMessenger));
    }
  }

  void onDetach() override
  {
    if(m_settings.enabled)
    {
      vkDestroyDebugUtilsMessengerEXT(m_app->getContext()->m_instance, m_dbgMessenger, nullptr);
      // Signal errors
      if(!m_errorMessages.empty())
      {
        LOGE("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
        for(auto& e : m_errorMessages)
          LOGE("%s\n", e.c_str());
        LOGE("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
      }
    }
  }

  void onRender(VkCommandBuffer cmd) override
  {
    if(!m_settings.enabled)
      return;

    m_counter++;
    if(m_counter >= m_settings.maxFrames)
    {
      if(m_settings.snapshot)
      {
        std::string name = std::string("snap_") + std::string(PROJECT_NAME) + std::string(".png");
        NVPSystem::windowScreenshot(m_app->getWindowHandle(), name.c_str());
        LOGI("Saving image: %s \n", name.c_str());
      }

      LOGI("Testing Time: %.3f ms\n", m_startTime.elapsed());

      m_app->close();  // request to stop
    }
  }

  void addError(const char* msg) { m_errorMessages.emplace_back(msg); }
  int  errorCode() { return m_errorMessages.empty() ? 0 : 1; }
  bool enabled() { return m_settings.enabled; }


private:
  Application*             m_app{nullptr};
  uint32_t                 m_counter{0};
  VkDebugUtilsMessengerEXT m_dbgMessenger = nullptr;

  std::vector<std::string> m_errorMessages;
  nvh::Stopwatch           m_startTime;
};

}  // namespace nvvkhl
