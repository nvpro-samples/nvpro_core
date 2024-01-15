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

/************************************************************************

The profiler element, is there to help profiling the time parts of the 
computation is done on the GPU. To use it, follow those simple steps

In the main() program, create an instance of the profiler and add it to the
nvvkhl::Application

```
  std::shared_ptr<nvvkhl::ElementProfiler> profiler = std::make_shared<nvvkhl::ElementProfiler>();
  app->addElement(profiler);
```

In the application where profiling needs to be done, add profiling sections

```
void mySample::onRender(VkCommandBuffer cmd)
{
    auto sec = m_profiler->timeRecurring(__FUNCTION__, cmd);
    ...
    // Subsection
    {
      auto sec = m_profiler->timeRecurring("Dispatch", cmd);
      vkCmdDispatch(cmd, (size.width + (GROUP_SIZE - 1)) / GROUP_SIZE, (size.height + (GROUP_SIZE - 1)) / GROUP_SIZE, 1);
    }
...
```

This is it and the execution time on the GPU for each part will be showing in the Profiler window.

************************************************************************/

#include <implot.h>
#include <imgui_internal.h>

#include "application.hpp"

#include "nvh/commandlineparser.hpp"
#include "nvh/nvprint.hpp"
#include "nvh/timesampler.hpp"
#include "nvpsystem.hpp"
#include "nvvk/error_vk.hpp"
#include "nvvk/profiler_vk.hpp"

namespace nvvkhl {

class ElementProfiler : public nvvkhl::IAppElement, public nvvk::ProfilerVK
{
public:
  ElementProfiler(bool showWindow = true)
      : m_showWindow(showWindow)
  {
    addSettingsHandler();
  };
  ~ElementProfiler() = default;

  void onAttach(Application* app) override
  {
    m_app = app;

    nvvk::ProfilerVK::init(m_app->getDevice(), m_app->getPhysicalDevice());
    nvvk::ProfilerVK::setLabelUsage(m_app->getContext()->hasInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));
    nvvk::ProfilerVK::beginFrame();
  }

  void onDetach() override
  {
    nvvk::ProfilerVK::endFrame();
    vkDeviceWaitIdle(m_app->getDevice());
    nvvk::ProfilerVK::deinit();
  }

  void onUIMenu() override
  {
    if(ImGui::BeginMenu("View"))
    {
      ImGui::MenuItem("Profiler", "", &m_showWindow);
      ImGui::EndMenu();
    }
  }  // This is the menubar to create


  void onUIRender() override
  {
    constexpr float frequency    = (1.0f / 60.0f);
    static float    s_minElapsed = 0;
    s_minElapsed += ImGui::GetIO().DeltaTime;

    if(!m_showWindow)
      return;

    // Opening the window
    if(!ImGui::Begin("Profiler", &m_showWindow))
    {
      ImGui::End();
      return;
    }

    if(s_minElapsed >= frequency)
    {
      s_minElapsed = 0;
      m_node.child.clear();
      m_node.name    = "Frame";
      m_node.cpuTime = m_data->cpuTime.getAveraged() / 1000.f;
      m_single.child.clear();
      m_single.name = "Single";
      addEntries(m_node.child, 0, m_data->numLastSections, 0);
    }

    bool copyToClipboard = ImGui::SmallButton("Copy");
    if(copyToClipboard)
      ImGui::LogToClipboard();

    if(ImGui::BeginTabBar("Profiler Tabs"))
    {
      if(ImGui::BeginTabItem("Table"))
      {
        renderTable();
        ImGui::EndTabItem();
      }
      if(ImGui::BeginTabItem("PieChart"))
      {
        renderPieChart();
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }


    if(copyToClipboard)
      ImGui::LogFinish();

    ImGui::End();
  }


  void onRender(VkCommandBuffer /*cmd*/) override
  {
    nvvk::ProfilerVK::endFrame();
    nvvk::ProfilerVK::beginFrame();
  }

private:
  struct MyEntryNode
  {
    std::string              name;
    float                    cpuTime = 0.f;
    float                    gpuTime = -1.f;
    std::vector<MyEntryNode> child;
  };

  uint32_t addEntries(std::vector<MyEntryNode>& nodes, uint32_t startIndex, uint32_t endIndex, uint32_t currentLevel)
  {
    for(uint32_t curIndex = startIndex; curIndex < endIndex; curIndex++)
    {
      Entry& entry = m_data->entries[curIndex];
      if(entry.level < currentLevel)
        return curIndex;

      MyEntryNode entryNode;
      entryNode.name    = entry.name.empty() ? "N/A" : entry.name;
      entryNode.gpuTime = entry.gpuTime.getAveraged() / 1000.f;
      entryNode.cpuTime = entry.cpuTime.getAveraged() / 1000.f;

      if(entry.level == LEVEL_SINGLESHOT)
      {
        m_single.child.push_back(entryNode);
        continue;
      }

      uint32_t nextLevel = curIndex + 1 < endIndex ? m_data->entries[curIndex + 1].level : currentLevel;
      if(nextLevel > currentLevel)
      {
        curIndex = addEntries(entryNode.child, curIndex + 1, endIndex, nextLevel);
      }
      nodes.push_back(entryNode);
    }
    return endIndex;
  }


  void displayTableNode(const MyEntryNode& node)
  {
    ImGuiTableFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAllColumns;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    const bool is_folder = (node.child.empty() == false);
    flags = is_folder ? flags : flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    bool open = ImGui::TreeNodeEx(node.name.c_str(), flags);
    ImGui::TableNextColumn();
    if(node.gpuTime <= 0)
      ImGui::TextDisabled("--");
    else
      ImGui::Text("%3.3f", node.gpuTime);
    ImGui::TableNextColumn();
    if(node.cpuTime <= 0)
      ImGui::TextDisabled("--");
    else
      ImGui::Text("%3.3f", node.cpuTime);
    if(open && is_folder)
    {
      for(int child_n = 0; child_n < node.child.size(); child_n++)
        displayTableNode(node.child[child_n]);
      ImGui::TreePop();
    }
  }


  void renderTable()
  {
    // Using those as a base value to create width/height that are factor of the size of our font
    const float textBaseWidth = ImGui::CalcTextSize("A").x;

    static ImGuiTableFlags s_flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable
                                     | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

    if(ImGui::BeginTable("EntryTable", 3, s_flags))
    {
      // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
      ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
      ImGui::TableSetupColumn("GPU", ImGuiTableColumnFlags_WidthFixed, textBaseWidth * 4.0f);
      ImGui::TableSetupColumn("CPU", ImGuiTableColumnFlags_WidthFixed, textBaseWidth * 4.0f);
      ImGui::TableHeadersRow();

      displayTableNode(m_node);
      displayTableNode(m_single);

      ImGui::EndTable();
    }
  }

  //-------------------------------------------------------------------------------------------------
  // Rendering the data as a PieChart, showing the percentage of utilization
  //
  void renderPieChart()
  {
    static bool s_showSubLevel = false;
    ImGui::Checkbox("Show SubLevel 1", &s_showSubLevel);

    if(ImPlot::BeginPlot("##Pie1", ImVec2(-1, -1), ImPlotFlags_NoMouseText))
    {
      ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Lock,
                        ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Lock);
      ImPlot::SetupAxesLimits(0, 1, 0, 1, ImPlotCond_Always);

      // Get all Level 0
      std::vector<const char*> labels1(m_node.child.size());
      std::vector<float>       data1(m_node.child.size());
      double                   angle0 = 90;
      for(size_t i = 0; i < m_node.child.size(); i++)
      {
        labels1[i] = m_node.child[i].name.c_str();
        data1[i]   = m_node.child[i].gpuTime / m_node.cpuTime;
      }

      ImPlot::PlotPieChart(labels1.data(), data1.data(), data1.size(), 0.5, 0.5, 0.4, "%.2f", angle0);

      // Level 1
      if(s_showSubLevel)
      {
        double a0 = angle0;
        for(size_t i = 0; i < m_node.child.size(); i++)
        {
          auto& currentNode = m_node.child[i];
          if(!currentNode.child.empty())
          {
            labels1.resize(currentNode.child.size());
            data1.resize(currentNode.child.size());
            for(size_t j = 0; j < currentNode.child.size(); j++)
            {
              labels1[j] = currentNode.child[j].name.c_str();
              data1[j]   = currentNode.child[j].gpuTime / m_node.cpuTime;
            }

            ImPlot::PlotPieChart(labels1.data(), data1.data(), data1.size(), 0.5, 0.5, 0.1, "", a0, ImPlotPieChartFlags_None);
          }

          // Increment the position of the next sub-element
          double percent = currentNode.gpuTime / m_node.cpuTime;
          a0 += a0 + 360 * percent;
        }
      }

      ImPlot::EndPlot();
    }
  }


  // This goes in the .ini file and remember the state of the window [open/close]
  void addSettingsHandler()
  {
    // Persisting the window
    ImGuiSettingsHandler iniHandler{};
    iniHandler.TypeName   = "ElementProfiler";
    iniHandler.TypeHash   = ImHashStr("ElementProfiler");
    iniHandler.ClearAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler*) {};
    iniHandler.ApplyAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler*) {};
    iniHandler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char* name) -> void* { return (void*)1; };
    iniHandler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, void* entry, const char* line) {
      ElementProfiler* s = (ElementProfiler*)handler->UserData;
      int              x;
      if(sscanf(line, "ShowWindow=%d", &x) == 1)
      {
        s->m_showWindow = (x == 1);
      }
    };
    iniHandler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
      ElementProfiler* s = (ElementProfiler*)handler->UserData;
      buf->appendf("[%s][State]\n", handler->TypeName);
      buf->appendf("ShowWindow=%d\n", s->m_showWindow ? 1 : 0);
      buf->appendf("\n");
    };
    iniHandler.UserData = this;
    ImGui::AddSettingsHandler(&iniHandler);
  }


  //---
  Application* m_app{nullptr};
  MyEntryNode  m_node;
  MyEntryNode  m_single;
  bool         m_showWindow = true;
};

}  // namespace nvvkhl
