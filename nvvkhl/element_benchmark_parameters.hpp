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
#pragma once

#include <array>

#include "application.hpp"
#include "nvh/commandlineparser.hpp"
#include "nvh/fileoperations.hpp"
#include "nvh/nvprint.hpp"
#include "nvh/parametertools.hpp"
#include "nvh/profiler.hpp"
#include "nvh/timesampler.hpp"
#include "nvpsystem.hpp"
#include "nvpwindow.hpp"
#include "nvvk/error_vk.hpp"
#include "nvvk/profiler_vk.hpp"

#include "GLFW/glfw3.h"

namespace nvvkhl {

/** @DOC_START
# class nvvkhl::ElementBenchmarkParameters

This element allows you to control an application with command line parameters. There are default 
parameters, but others can be added using the parameterLists().add(..) function.

It can also use a file containing several sets of parameters, separated by "benchmark" and 
which can be used to benchmark an application. 

If a profiler is set, the measured performance at the end of each benchmark group is logged.


There are default parameters that can be used:
-logfile            Set a logfile.txt. If string contains $DEVICE$ it will be replaced by the GPU device name
-winsize            Set window size (width and height)
-winpos             Set window position (x and y)
-vsync              Enable or disable vsync
-screenshot         Save a screenshot into this file
-benchmarkframes    Set number of benchmarkframes
-benchmark          Set benchmark filename
-test               Enabling Testing
-test-frames        If test is on, number of frames to run
-test-time          If test is on, time that test will run

Example of Setup:

```cpp
std::shared_ptr<nvvkhl::ElementBenchmarkParameters> g_benchmark;
std::shared_ptr<nvvkhl::ElementProfiler>  g_profiler;

main() { 
  ...
  g_benchmark   = std::make_shared<nvvkhl::ElementBenchmarkParameters>(argc, argv);
  g_profiler = std::make_shared<nvvkhl::ElementProfiler>(false);
  g_benchmark->setProfiler(g_profiler);
  app->addElement(g_profiler);
  app->addElement(g_benchmark);
  ...
}
```


Applications can also get their parameters modified: 

```cpp
void MySample::MySample() { 
    g_benchmark->parameterLists().add("speed|The speed", &m_speed);
    g_benchmark->parameterLists().add("color", &m_color, nullptr, 3);
    g_benchmark->parameterLists().add("complex", &m_complex, [&](int p){ doSomething(); });
```cpp


Example of a benchmark.txt could look like

\code{.bat}
#how many frames to average
-benchmarkframes 12
-winpos 10 10
-winsize 500 500

benchmark "No vsync"
-vsync 0
-benchmarkframes 100
-winpos 500 500
-winsize 100 100

benchmark "Image only"
-screenshot "temporal_mdi.jpg"
```

@DOC_END */


class ElementBenchmarkParameters : public nvvkhl::IAppElement
{
public:
  struct Benchmark
  {
    bool                   initialized = false;
    std::string            filename;
    std::string            content;
    nvh::ParameterSequence sequence;
    uint32_t               frameLength = 256;
    uint32_t               frame       = 0;
  };

  struct Config
  {
    int32_t     winpos[2]  = {0, 0};
    int32_t     winsize[2] = {0, 0};
    bool        vsyncstate = true;
    std::string screenshotFilename;
    std::string logFilename;
    bool        testEnabled   = false;
    uint32_t    testMaxFrames = 0;
    float       testMaxTime   = 0;
  };

  ElementBenchmarkParameters(int argc, char** argv)
      : m_argc(argc - 1)  // Skip executable
      , m_argv(argv + 1)
  {
    setupParameters();  // All parameter handled by benchmark
    // By default this class increase the frame every time it goes through onRender.
    // But this could be override externally to provide the actual rendered frame it is looking for.
    getCurrentFrame = [&]() -> int { return ++m_currentFrame; };
  }

  ~ElementBenchmarkParameters() = default;

  // Get access to the parameter list, to add parameters that application wants modified
  nvh::ParameterList& parameterLists() { return m_parameterList; }

  // Add a callback when advancing in the benchmark
  void addPostBenchmarkAdvanceCallback(std::function<void()>&& func) { m_postCallback.emplace_back(func); }

  // Set the frame number from an external view
  void setCurrentFrame(std::function<int()>&& func) { getCurrentFrame = func; }

  // External profiler, if profiling is required.
  void setProfiler(std::shared_ptr<nvvk::ProfilerVK> profiler) { m_profiler = profiler; }

  int errorCode() { return m_errorMessages.empty() ? 0 : 1; }

  const Benchmark& benchmark() const { return m_benchmark; }
  const Config&    config() const { return m_config; }

  ///
  /// IAppElement implementation
  ///

  void onAttach(Application* app) override
  {
    m_app = app;

    // Parse all arguments, now that the application is attached
    parseConfig(m_argc, const_cast<const char**>(m_argv), ".");

    initBenchmark();  // -benchmark <file.txt>
    initTesting();    // -test

    m_startTime = ImGui::GetTime();
  }

  void onDetach() override { deinitTesting(); }

  void onUIRender() override { advanceBenchmark(); }

  void onRender(VkCommandBuffer /*cmd*/) override
  {
    m_currentFrame = getCurrentFrame();
    executeTest();
  }


protected:
  virtual void parseConfig(int argc, const char** argv, const std::string& path)
  {
    // if you want to handle parameters not represented in
    // m_parameterList then override this function accordingly.
    m_parameterList.applyTokens(argc, argv, "-", path.c_str());
    // This function is called before "begin" and provided with the commandline used in "run".
    // It can also be called by the benchmarking system, and parseConfigFile.
  }

private:
  // This function replaces all occurrences of the substring 'from' with the substring 'to' in the given string 'str'.
  void replace(std::string& str, const std::string& from, const std::string& to)
  {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }
  }

  void setLogfile()
  {
    if(!m_config.logFilename.empty() && m_app)
    {
      std::string deviceName  = m_app->getContext()->m_physicalInfo.properties10.deviceName;
      std::string logfileName = m_config.logFilename;  // Replace "$DEVICE$" with the GPU device name
      replace(logfileName, "$DEVICE$", deviceName);
      std::replace_if(  // Replace characters not allowed in filenames with underscores
          logfileName.begin(), logfileName.end(),
          [](char c) { return !(std::isalnum(c) || c == '_' || c == '/' || c == '.' || c == '-'); }, '_');
      nvprintSetLogFileName(logfileName.c_str());
    }
  }

  void setupParameters()
  {
    m_parameterList.add(
        "winsize|Set window size (width and height)", m_config.winsize,
        [&](uint32_t) { glfwSetWindowSize(m_app->getWindowHandle(), m_config.winsize[0], m_config.winsize[1]); }, 2);

    m_parameterList.add(
        "winpos|Set window position (x and y)", m_config.winpos,
        [&](uint32_t) { glfwSetWindowPos(m_app->getWindowHandle(), m_config.winpos[0], m_config.winpos[1]); }, 2);

    m_parameterList.add("vsync|Enable or disable vsync", &m_config.vsyncstate,
                        [&](uint32_t) { m_app->setVsync(m_config.vsyncstate); });

    m_parameterList.addFilename("logfile|Set logfile", &m_config.logFilename, [&](uint32_t) { setLogfile(); });

    m_parameterList.add("screenshot|makes a screenshot into this file", &m_config.screenshotFilename, [&](uint32_t) {
      // We don't want to capture the command line, only when it is part of the benchmark
      if(!m_config.screenshotFilename.empty() && !m_benchmark.content.empty())
      {
        m_app->screenShot(m_config.screenshotFilename.c_str());
      }
    });

    m_parameterList.add("benchmarkframes|Set number of benchmarkframes", &m_benchmark.frameLength);
    m_parameterList.addFilename("benchmark|Set benchmark filename", &m_benchmark.filename);
    m_parameterList.add("test|Testing Mode", &m_config.testEnabled, true);
    m_parameterList.add("test-frames|If test is on, number of frames to run", &m_config.testMaxFrames);
    m_parameterList.add("test-time|If test is on, time that test will run", &m_config.testMaxTime);
  }


  void initBenchmark()
  {
    if(m_benchmark.initialized)
      return;

    m_benchmark.initialized = true;

    if(m_benchmark.filename.empty())
      return;

    m_benchmark.content = nvh::loadFile(m_benchmark.filename.c_str(), false);
    if(!m_benchmark.content.empty())
    {
      std::vector<const char*> tokens;
      nvh::ParameterList::tokenizeString(m_benchmark.content, tokens);

      std::string path = nvh::getFilePath(m_benchmark.filename.c_str());

      m_benchmark.sequence.init(&m_parameterList, tokens);

      // do first iteration manually, due to custom arg parsing
      uint32_t argBegin;
      uint32_t argCount;
      if(!m_benchmark.sequence.advanceIteration("benchmark", 1, argBegin, argCount))
      {
        parseConfig(argCount, &tokens[argBegin], path);
      }

      m_benchmark.frame = 0;
    }
  }

  void advanceBenchmark()
  {
    if(!m_benchmark.sequence.isActive())
      return;

    m_benchmark.frame++;

    if(m_benchmark.frame > m_benchmark.frameLength + nvh::Profiler::CONFIG_DELAY + nvh::Profiler::FRAME_DELAY)
    {
      m_benchmark.frame = 0;

      std::string stats;
      if(m_profiler)
        m_profiler->print(stats);
      LOGI("BENCHMARK %d \"%s\" {\n", m_benchmark.sequence.getIteration(), m_benchmark.sequence.getSeparatorArg(0));
      LOGI("%s}\n\n", stats.c_str());

      bool done = m_benchmark.sequence.applyIteration("benchmark", 1, "-");
      if(m_profiler)
        m_profiler->reset(nvh::Profiler::CONFIG_DELAY);

      // Callback all registered functions
      for(auto& func : m_postCallback)
        func();

      if(done)
      {
        m_app->close();  // request to stop
      }
    }
  }

  void initTesting()
  {
    if(!m_config.testEnabled)
      return;

    // Setting defaults
    if(m_config.testMaxFrames == 0 && m_config.testMaxTime == 0)
      m_config.testMaxFrames = 1;
    if(m_config.testMaxFrames == 0)
      m_config.testMaxFrames = std::numeric_limits<uint32_t>::max();
    if(m_config.testMaxTime == 0)
      m_config.testMaxTime = std::numeric_limits<float>::max();


    // The following is adding a callback for Vulkan messages, and collect all error messages.
    // If errors are found errorCode will return 1, otherwise 0

    VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    dbg_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT   // Vulkan issues
                                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;  // Invalid usage
    dbg_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT           // Other
                                            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;     // Violation of spec
    dbg_messenger_create_info.pUserData = this;

    dbg_messenger_create_info.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                   VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
                                                   const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
      ElementBenchmarkParameters* elementBenchParamClass = reinterpret_cast<ElementBenchmarkParameters*>(userData);
      if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
      {
        elementBenchParamClass->addError(callbackData->pMessage);
      }
      return VK_FALSE;
    };
    NVVK_CHECK(vkCreateDebugUtilsMessengerEXT(m_app->getContext()->m_instance, &dbg_messenger_create_info, nullptr, &m_dbgMessenger));
  }

  void deinitTesting()
  {
    if(m_dbgMessenger != nullptr)
      vkDestroyDebugUtilsMessengerEXT(m_app->getContext()->m_instance, m_dbgMessenger, nullptr);
  }

  void executeTest()
  {
    if(!m_config.testEnabled)
      return;

    bool     closingApp = false;
    double   elapseTime = (ImGui::GetTime() - m_startTime);
    uint32_t maxFrames =
        std::max(m_config.testMaxFrames, m_profiler ? nvh::Profiler::CONFIG_DELAY + nvh::Profiler::FRAME_DELAY : 0);

    closingApp |= elapseTime >= m_config.testMaxTime;
    closingApp |= m_currentFrame >= static_cast<int>(maxFrames);
    if(closingApp)
    {
      if(!m_config.screenshotFilename.empty())
      {
        m_app->screenShot(m_config.screenshotFilename.c_str());
      }

      if(m_profiler)
      {
        std::string stats;
        m_profiler->print(stats);
        LOGI("%s", stats.c_str());
      }

      LOGI("Number of frames: %d\n", m_currentFrame);
      LOGI("Testing Time: %.3f s\n", elapseTime);

      // Signal errors
      if(!m_errorMessages.empty())
      {
        LOGE("+-+ ERRORS  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
        for(auto& e : m_errorMessages)
          LOGE("%s\n", e.c_str());
        LOGE("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
      }

      m_app->close();  // Request to close
    }
  }

  void addError(const char* msg) { m_errorMessages.emplace_back(msg); }

  //---------------------------------
  Application*             m_app          = nullptr;
  VkDebugUtilsMessengerEXT m_dbgMessenger = nullptr;


  std::vector<std::function<void()>> m_postCallback;   // To get called after a new benchmark setting
  std::function<int()>               getCurrentFrame;  // To get the current frame from possible external source
  std::vector<std::string>           m_errorMessages;  // Collect Vulkan error messages

  Benchmark          m_benchmark;      // Benchmark file setting
  Config             m_config;         // Current settings
  nvh::ParameterList m_parameterList;  // List of all command line parameters, from this class and external when set

  int    m_currentFrame = 0;  // Current states
  double m_startTime    = 0.f;

  // Keeping the command line argument, until the application attachment
  int    m_argc;
  char** m_argv;

  std::shared_ptr<nvvk::ProfilerVK> m_profiler;  // [optional]
};

}  // namespace nvvkhl
