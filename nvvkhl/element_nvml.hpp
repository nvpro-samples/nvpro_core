/*
 * SPDX-FileCopyrightText: Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */


#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <numeric>
#include <type_traits>
#include <fmt/core.h>

#include "application.hpp"
#include "nvh/nvml_monitor.hpp"
#include "imgui/imgui_helper.h"

namespace nvvkhl {

#define SAMPLING_NUM 100       // Show 100 measurements
#define SAMPLING_INTERVAL 100  // Sampling every 100 ms


//-----------------------------------------------------------------------------
int metricFormatter(double value, char* buff, int size, void* data)
{
  const char*        unit       = (const char*)data;
  static double      s_value[]  = {1000000000, 1000000, 1000, 1, 0.001, 0.000001, 0.000000001};
  static const char* s_prefix[] = {"G", "M", "k", "", "m", "u", "n"};
  if(value == 0)
  {
    return snprintf(buff, size, "0 %s", unit);
  }
  for(int i = 0; i < 7; ++i)
  {
    if(fabs(value) >= s_value[i])
    {
      return snprintf(buff, size, "%g %s%s", value / s_value[i], s_prefix[i], unit);
    }
  }
  return snprintf(buff, size, "%g %s%s", value / s_value[6], s_prefix[6], unit);
}


// utility structure for averaging values
template <typename T>
struct AverageCircularBuffer
{
  int            offset   = 0;
  T              totValue = 0;
  std::vector<T> data;
  AverageCircularBuffer(int max_size = 100) { data.reserve(max_size); }
  void addValue(T x)
  {
    if(data.size() < data.capacity())
    {
      data.push_back(x);
      totValue += x;
    }
    else
    {
      totValue -= data[offset];
      totValue += x;
      data[offset] = x;
      offset       = (offset + 1) % data.capacity();
    }
  }

  T average() { return totValue / data.size(); }
};


//extern SampleAppLog g_logger;
struct ElementNvml : public nvvkhl::IAppElement
{
  explicit ElementNvml(bool show = false)
      : m_showWindow(show)
  {
#if defined(NVP_SUPPORTS_NVML)
    m_nvmlMonitor = std::make_unique<NvmlMonitor>(SAMPLING_INTERVAL, SAMPLING_NUM);
#endif
    addSettingsHandler();
  }

  virtual ~ElementNvml() = default;

  void onUIRender() override
  {
#if defined(NVP_SUPPORTS_NVML)
    m_nvmlMonitor->refresh();
#endif
    if(!m_showWindow)
      return;

    ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize({400, 200}, ImGuiCond_Appearing);
    ImGui::SetNextWindowBgAlpha(0.7F);
    if(ImGui::Begin("NVML Monitor", &m_showWindow))
    {
#if defined(NVP_SUPPORTS_NVML)
      if(m_nvmlMonitor->isValid() == false)
      {
        ImGui::Text("NVML wasn't loaded");
        ImGui::End();
        return;
      }

      // Averaging CPU values
      const NvmlMonitor::SysInfo& cpuMeasure = m_nvmlMonitor->getSysInfo();

      {  // Averaging the CPU sampling, but limit the
        static double s_refreshRate = ImGui::GetTime();
        if((ImGui::GetTime() - s_refreshRate) > SAMPLING_INTERVAL / 1000.0)
        {
          m_avgCpu.addValue(cpuMeasure.cpu[m_nvmlMonitor->getOffset()]);
          s_refreshRate = ImGui::GetTime();
        }
      }


      if(ImGui::BeginTabBar("MonitorTabs"))
      {
        if(ImGui::BeginTabItem("All"))
        {
          drawProgressBars();
          ImGui::EndTabItem();
        }

        // Display Graphs for each GPU
        for(uint32_t gpuIndex = 0; gpuIndex < m_nvmlMonitor->getGpuCount(); gpuIndex++)  // Number of gpu
        {
          const std::string gpuTabName = fmt::format("GPU-{}", gpuIndex);
          if(ImGui::BeginTabItem(gpuTabName.c_str()))
          {
            drawGraphLines(gpuIndex);
            ImGui::EndTabItem();
          }
        }
        ImGui::EndTabBar();
      }

#else
      ImGui::Text("NVML wasn't loaded");
#endif
    }
    ImGui::End();
  }

  //-------------------------------------------------------------------------------------------------
  // Adding an access though the menu
  void onUIMenu() override
  {
    if(ImGui::BeginMenu("View"))
    {
      ImGui::MenuItem("NVML Monitor", nullptr, &m_showWindow);
      ImGui::EndMenu();
    }
  }


  void drawGraphLines(uint32_t gpuIndex)
  {
#if defined(NVP_SUPPORTS_NVML)
    const NvmlMonitor::SysInfo& cpuMeasure = m_nvmlMonitor->getSysInfo();

    const int   offset    = m_nvmlMonitor->getOffset();
    std::string cpuString = fmt::format("CPU: {:3.1f}%", m_avgCpu.average());

    // Display Graphs
    const NvmlMonitor::GpuInfo&    gpuInfo    = m_nvmlMonitor->getGpuInfo(gpuIndex);
    const NvmlMonitor::GpuMeasure& gpuMeasure = m_nvmlMonitor->getMeasures(gpuIndex);

    std::string lineString = fmt::format("Load: {}%", gpuMeasure.load[offset]);
    float       memUsage   = static_cast<float>(gpuMeasure.memory[offset]) / gpuInfo.maxMem * 100.f;
    std::string memString  = fmt::format("Memory: {}%", static_cast<int>(memUsage));

    static ImPlotFlags     s_plotFlags = ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotFlags_Crosshairs;
    static ImPlotAxisFlags s_axesFlags = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoLabel;
    static ImColor         s_lineColor = ImColor(0.07f, 0.9f, 0.06f, 1.0f);
    static ImColor         s_memColor  = ImColor(0.06f, 0.6f, 0.97f, 1.0f);
    static ImColor         s_cpuColor  = ImColor(0.96f, 0.96f, 0.0f, 1.0f);

    if(ImPlot::BeginPlot(gpuInfo.name.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, -1), s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, "Load", s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);
      ImPlot::SetupAxis(ImAxis_Y2, "Mem", ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_Opposite);
      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, 100);
      ImPlot::SetupAxisLimits(ImAxis_Y2, 0, float(gpuInfo.maxMem));
      ImPlot::SetupAxisFormat(ImAxis_Y2, metricFormatter, (void*)"iB");

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_lineColor);
      ImPlot::PlotShaded(lineString.c_str(), gpuMeasure.load.data(), (int)gpuMeasure.load.size(), -INFINITY, 1.0, 0.0,
                         0, offset + 1);
      ImPlot::SetNextLineStyle(s_lineColor);
      ImPlot::PlotLine(lineString.c_str(), gpuMeasure.load.data(), (int)gpuMeasure.load.size(), 1.0, 0.0, 0, offset + 1);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
      ImPlot::SetNextFillStyle(s_memColor);
      ImPlot::PlotShaded(memString.c_str(), gpuMeasure.memory.data(), (int)gpuMeasure.memory.size(), -INFINITY, 1.0,
                         0.0, 0, offset + 1);
      ImPlot::SetNextLineStyle(s_memColor);
      ImPlot::PlotLine(memString.c_str(), gpuMeasure.memory.data(), (int)gpuMeasure.memory.size(), 1.0, 0.0, 0, offset + 1);
      ImPlot::PopStyleVar();

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextLineStyle(s_cpuColor);
      ImPlot::PlotLine(cpuString.c_str(), cpuMeasure.cpu.data(), (int)cpuMeasure.cpu.size(), 1.0, 0.0, 0, offset + 1);

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse     = ImPlot::GetPlotMousePos();
        int         gpuOffset = (int(mouse.x) + offset) % (int)gpuMeasure.memory.size();
        int         cpuOffset = (int(mouse.x) + offset) % (int)cpuMeasure.cpu.size();

        char buff[32];
        metricFormatter(gpuMeasure.memory[gpuOffset], buff, 32, (void*)"iB");

        ImGui::BeginTooltip();
        ImGui::Text("Load: %3.0f%%", gpuMeasure.load[gpuOffset]);
        ImGui::Text("Memory: %s", buff);
        ImGui::Text("Cpu: %3.0f%%", cpuMeasure.cpu[cpuOffset]);
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }
#endif
  }

  void drawProgressBars()
  {
#if defined(NVP_SUPPORTS_NVML)
    const int offset = m_nvmlMonitor->getOffset();

    for(uint32_t gpuIndex = 0; gpuIndex < m_nvmlMonitor->getGpuCount(); gpuIndex++)  // Number of gpu
    {
      const NvmlMonitor::GpuInfo&    gpuInfo   = m_nvmlMonitor->getGpuInfo(gpuIndex);
      const NvmlMonitor::GpuMeasure& gpuMesure = m_nvmlMonitor->getMeasures(gpuIndex);

      char   progtext[64];
      double GiBvalue = 1'000'000'000;
      sprintf(progtext, "%3.2f/%3.2f GiB", static_cast<double>(gpuMesure.memory[offset]) / GiBvalue, gpuInfo.maxMem / GiBvalue);

      // Load
      ImGui::Text("GPU: %s", gpuInfo.name.c_str());
      ImGuiH::PropertyEditor::begin();
      ImGuiH::PropertyEditor::entry("Load", [&] {
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor::HSV(0.3F, 0.5F, 0.5F));
        ImGui::ProgressBar(gpuMesure.load[offset] / 100.F);
        ImGui::PopStyleColor();
        return false;
      });

      // Memory
      ImGuiH::PropertyEditor::entry("Memory", [&] {
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor::HSV(0.6F, 0.5F, 0.5F));
        float memUsage = static_cast<float>((gpuMesure.memory[offset] * 1000) / gpuInfo.maxMem) / 1000.0F;
        ImGui::ProgressBar(memUsage, ImVec2(-1.f, 0.f), progtext);
        ImGui::PopStyleColor();
        return false;
      });

      ImGuiH::PropertyEditor::end();
    }


    ImGuiH::PropertyEditor::begin();
    ImGuiH::PropertyEditor::entry("CPU", [&] {
      ImGui::ProgressBar(m_avgCpu.average() / 100.F);
      return false;
    });
    ImGuiH::PropertyEditor::end();
#endif
  }


  // This goes in the .ini file and remember the state of the window [open/close]
  void addSettingsHandler()
  {
    // Persisting the window
    ImGuiSettingsHandler iniHandler{};
    iniHandler.TypeName   = "ElementNvml";
    iniHandler.TypeHash   = ImHashStr("ElementNvml");
    iniHandler.ClearAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler*) {};
    iniHandler.ApplyAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler*) {};
    iniHandler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char* name) -> void* { return (void*)1; };
    iniHandler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, void* entry, const char* line) {
      ElementNvml* s = (ElementNvml*)handler->UserData;
      int          x;
      if(sscanf(line, "ShowLoader=%d", &x) == 1)
      {
        s->m_showWindow = (x == 1);
      }
    };
    iniHandler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
      ElementNvml* s = (ElementNvml*)handler->UserData;
      buf->appendf("[%s][State]\n", handler->TypeName);
      buf->appendf("ShowLoader=%d\n", s->m_showWindow ? 1 : 0);
      buf->appendf("\n");
    };
    iniHandler.UserData = this;
    ImGui::AddSettingsHandler(&iniHandler);
  }

private:
  bool m_showWindow{false};
#if defined(NVP_SUPPORTS_NVML)
  std::unique_ptr<NvmlMonitor> m_nvmlMonitor;
  AverageCircularBuffer<float> m_avgCpu = {SAMPLING_NUM};
#endif
};


}  // namespace nvvkhl
