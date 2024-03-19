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
#include "imgui/imgui_icon.h"
#include "nvh/timesampler.hpp"


/** @DOC_START
# class nvvkhl::ElementNvml

>  This class is an element of the application that is responsible for the NVML monitoring. It is using the `NVML` library to get information about the GPU and display it in the application.

To use this class, you need to add it to the `nvvkhl::Application` using the `addElement` method.

@DOC_END */

namespace nvvkhl {

#define SAMPLING_NUM 100       // Show 100 measurements
#define SAMPLING_INTERVAL 100  // Sampling every 100 ms

// Time (in ms) during which a throttle reason is shown as currently happening
#define THROTTLE_SHOW_COOLDOWN_TIME 1000
// Time (in ms) during which the last throttle reason is shown
#define THROTTLE_COOLDOWN_TIME 5000
#define MIB_SIZE 1'000'000

//-----------------------------------------------------------------------------
inline int metricFormatter(double value, char* buff, int size, void* data)
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

  void pushThrottleTabColor()
  {
    if(m_throttleDetected)
    {
      ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(1, 0, 0, 1));
      ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.8f, 0, 0, 1));
      ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.8f, 0, 0, 1));
    }
  }

  void popThrottleTabColor()
  {
    if(m_throttleDetected)
    {
      ImGui::PopStyleColor(3);
    }
  }

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
          imguiProgressBars();
          ImGui::EndTabItem();
        }

        // Display Graphs for each GPU
        for(uint32_t gpuIndex = 0; gpuIndex < m_nvmlMonitor->getGpuCount(); gpuIndex++)  // Number of gpu
        {
          const std::string gpuTabName = fmt::format("GPU-{}", gpuIndex);
          pushThrottleTabColor();
          if(ImGui::BeginTabItem(gpuTabName.c_str()))
          {
            popThrottleTabColor();
            const std::string gpuTabBarName = fmt::format("GPU-{}TabBar", gpuIndex);
            if(ImGui::BeginTabBar(gpuTabBarName.c_str()))
            {
              if(ImGui::BeginTabItem("Overview"))
              {
                imguiGraphLines(gpuIndex);
                ImGui::EndTabItem();
              }

              const std::string gpuInfoTabName = fmt::format("Details###GPU-{}InfoTab", gpuIndex);
              if(ImGui::BeginTabItem(gpuInfoTabName.c_str()))
              {
                imguiDeviceInfo(gpuIndex);
                ImGui::EndTabItem();
              }

              const std::string perfStateTabName = fmt::format("Performance State###PerfStateGPU - {}InfoTab", gpuIndex);
              pushThrottleTabColor();
              if(ImGui::BeginTabItem(perfStateTabName.c_str()))
              {

                imguiDevicePerformanceState(gpuIndex);
                ImGui::EndTabItem();
              }
              popThrottleTabColor();

              const std::string powerStateTabName = fmt::format("Power State###PowerStateGPU - {}InfoTab", gpuIndex);
              if(ImGui::BeginTabItem(powerStateTabName.c_str()))
              {
                imguiDevicePowerState(gpuIndex);
                ImGui::EndTabItem();
              }

              const std::string utilizationTabName = fmt::format("Utilization###UtilizationGPU - {}InfoTab", gpuIndex);
              if(ImGui::BeginTabItem(utilizationTabName.c_str()))
              {
                imguiDeviceUtilization(gpuIndex);
                ImGui::EndTabItem();
              }
              const std::string memoryTabName = fmt::format("Memory###MemoryGPU - {}InfoTab", gpuIndex);
              if(ImGui::BeginTabItem(memoryTabName.c_str()))
              {
                imguiDeviceMemory(gpuIndex);
                ImGui::EndTabItem();
              }


              const std::string clockSetupTabName = fmt::format("Clock Setup###ClockSetupGPU - {}InfoTab", gpuIndex);
              if(ImGui::BeginTabItem(clockSetupTabName.c_str()))
              {
                imguiClockSetup(gpuIndex);
                ImGui::EndTabItem();
              }


              ImGui::EndTabBar();
            }

            ImGui::EndTabItem();
          }
          else
          {
            popThrottleTabColor();
          }
        }
        ImGui::EndTabBar();
      }
      for(uint32_t deviceIndex = 0; deviceIndex < m_nvmlMonitor->getGpuCount(); deviceIndex++)
      {

        const NvmlMonitor::DevicePerformanceState& performanceState = m_nvmlMonitor->getDevicePerformanceState(deviceIndex);
        uint32_t offset                = m_nvmlMonitor->getOffset();
        uint64_t currentThrottleReason = performanceState.throttleReasons.get()[offset];
        if(currentThrottleReason > 1)
        {
          std::string message =
              fmt::format("Throttle detected for GPU {}: {} - Performance numbers will be unreliable", deviceIndex,
                          NvmlMonitor::DevicePerformanceState::getThrottleReasonStrings(currentThrottleReason)[0]);
          ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), message.c_str());
          m_throttleDetected = true;

          if(m_lastThrottleReason != currentThrottleReason)
          {
            LOGW((message + "\n").c_str(), deviceIndex);
          }
          m_lastThrottleReason = currentThrottleReason;
          m_throttleCooldownTimer.reset();
        }
        else
        {
          if(m_throttleDetected)
          {
            if(m_throttleCooldownTimer.elapsed() > THROTTLE_COOLDOWN_TIME)
            {
              m_throttleDetected = false;
            }
            else
            {
              if(m_throttleCooldownTimer.elapsed() > THROTTLE_SHOW_COOLDOWN_TIME)
              {
                ImGui::TextColored(
                    ImVec4(0.8f, 0.2f, 0.f, 1.f),
                    "Throttle detected for GPU %d: %s - %.1f s ago - Performance numbers will be unreliable", deviceIndex,
                    NvmlMonitor::DevicePerformanceState::getThrottleReasonStrings(m_lastThrottleReason)[0].c_str(),
                    static_cast<float>(m_throttleCooldownTimer.elapsed() / 1000.f));
              }
              else
              {
                ImGui::TextColored(
                    ImVec4(1.f, 0.f, 0.f, 1.f), "Throttle detected for GPU %d: %s - Performance numbers will be unreliable", deviceIndex,
                    NvmlMonitor::DevicePerformanceState::getThrottleReasonStrings(m_lastThrottleReason)[0].c_str());
              }
            }
          }
        }
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


  void imguiGraphLines(uint32_t gpuIndex)
  {
#if defined(NVP_SUPPORTS_NVML)
    const NvmlMonitor::SysInfo& cpuMeasure = m_nvmlMonitor->getSysInfo();

    const int   offset    = m_nvmlMonitor->getOffset();
    std::string cpuString = fmt::format("CPU: {:3.1f}%", m_avgCpu.average());

    // Display Graphs
    const NvmlMonitor::DeviceInfo& deviceInfo = m_nvmlMonitor->getDeviceInfo(gpuIndex);

    const NvmlMonitor::DeviceMemory&      deviceMemory      = m_nvmlMonitor->getDeviceMemory(gpuIndex);
    const NvmlMonitor::DeviceUtilization& deviceUtilization = m_nvmlMonitor->getDeviceUtilization(gpuIndex);

    std::string lineString = fmt::format("Load: {}%", deviceUtilization.gpuUtilization.get()[offset]);
    float memUsage = static_cast<float>(deviceMemory.memoryUsed.get()[offset]) / deviceMemory.memoryTotal.get() * 100.f;
    std::string memString = fmt::format("Memory: {}%", static_cast<int>(memUsage));

    static ImPlotFlags     s_plotFlags = ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotFlags_Crosshairs;
    static ImPlotAxisFlags s_axesFlags = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoLabel;
    static ImColor         s_lineColor = ImColor(0.07f, 0.9f, 0.06f, 1.0f);
    static ImColor         s_memColor  = ImColor(0.06f, 0.6f, 0.97f, 1.0f);
    static ImColor         s_cpuColor  = ImColor(0.96f, 0.96f, 0.0f, 1.0f);

    if(ImPlot::BeginPlot(deviceInfo.deviceName.get().c_str(), ImVec2(ImGui::GetContentRegionAvail().x, -1), s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, "Load", s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);
      ImPlot::SetupAxis(ImAxis_Y2, "Mem", ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_Opposite);
      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, 100);
      ImPlot::SetupAxisLimits(ImAxis_Y2, 0, float(deviceMemory.memoryTotal.get()));
      ImPlot::SetupAxisFormat(ImAxis_Y2, metricFormatter, (void*)"iB");

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_lineColor);
      ImPlot::PlotShaded(lineString.c_str(), deviceUtilization.gpuUtilization.get().data(),
                         (int)deviceUtilization.gpuUtilization.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);
      ImPlot::SetNextLineStyle(s_lineColor);
      ImPlot::PlotLine(lineString.c_str(), deviceUtilization.gpuUtilization.get().data(),
                       (int)deviceUtilization.gpuUtilization.get().size(), 1.0, 0.0, 0, offset + 1);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
      ImPlot::SetNextFillStyle(s_memColor);
      // Cast to unsigned long long for Linux compilation, where ImPlot functions are not instantiated with uint64_t
      ImPlot::PlotShaded(memString.c_str(), reinterpret_cast<const unsigned long long*>(deviceMemory.memoryUsed.get().data()),
                         (int)deviceMemory.memoryUsed.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);
      ImPlot::SetNextLineStyle(s_memColor);
      // Cast to unsigned long long for Linux compilation, where ImPlot functions are not instantiated with uint64_t
      ImPlot::PlotLine(memString.c_str(), reinterpret_cast<const unsigned long long*>(deviceMemory.memoryUsed.get().data()),
                       (int)deviceMemory.memoryUsed.get().size(), 1.0, 0.0, 0, offset + 1);
      ImPlot::PopStyleVar();

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextLineStyle(s_cpuColor);
      ImPlot::PlotLine(cpuString.c_str(), cpuMeasure.cpu.data(), (int)cpuMeasure.cpu.size(), 1.0, 0.0, 0, offset + 1);

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse     = ImPlot::GetPlotMousePos();
        int         gpuOffset = (int(mouse.x) + offset) % (int)deviceMemory.memoryUsed.get().size();
        int         cpuOffset = (int(mouse.x) + offset) % (int)cpuMeasure.cpu.size();

        char buff[32];
        metricFormatter(static_cast<double>(deviceMemory.memoryUsed.get()[gpuOffset]), buff, 32, (void*)"iB");

        ImGui::BeginTooltip();
        ImGui::Text("Load: %d%%", deviceUtilization.gpuUtilization.get()[gpuOffset]);
        ImGui::Text("Memory: %s", buff);
        ImGui::Text("Cpu: %3.0f%%", cpuMeasure.cpu[cpuOffset]);
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }
#endif
  }

  void imguiProgressBars()
  {
#if defined(NVP_SUPPORTS_NVML)
    const int offset = m_nvmlMonitor->getOffset();

    for(uint32_t gpuIndex = 0; gpuIndex < m_nvmlMonitor->getGpuCount(); gpuIndex++)  // Number of gpu
    {
      const NvmlMonitor::DeviceInfo&        gpuInfo           = m_nvmlMonitor->getDeviceInfo(gpuIndex);
      const NvmlMonitor::DeviceMemory&      deviceMemoryInfo  = m_nvmlMonitor->getDeviceMemory(gpuIndex);
      const NvmlMonitor::DeviceUtilization& deviceUtilization = m_nvmlMonitor->getDeviceUtilization(gpuIndex);

      char   progtext[64];
      double GiBvalue = 1'000'000'000;
      sprintf(progtext, "%3.2f/%3.2f GiB", static_cast<double>(deviceMemoryInfo.memoryUsed.get()[offset]) / GiBvalue,
              deviceMemoryInfo.memoryTotal.get() / GiBvalue);

      // Load
      ImGui::Text("GPU: %s", gpuInfo.deviceName.get().c_str());
      ImGuiH::PropertyEditor::begin();
      ImGuiH::PropertyEditor::entry("Load", [&] {
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor::HSV(0.3F, 0.5F, 0.5F));
        ImGui::ProgressBar(deviceUtilization.gpuUtilization.get()[offset] / 100.F);
        ImGui::PopStyleColor();
        return false;
      });

      // Memory
      ImGuiH::PropertyEditor::entry("Memory", [&] {
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor::HSV(0.6F, 0.5F, 0.5F));
        float memUsage =
            static_cast<float>((deviceMemoryInfo.memoryUsed.get()[offset] * 1000) / deviceMemoryInfo.memoryTotal.get()) / 1000.0F;
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


  static void imguiCopyableText(const std::string& text, uint64_t uniqueId)
  {
    std::string textString = fmt::format("{}###{}", text, uniqueId);
    ImGui::Text("%s", text.c_str());
    if(ImGui::BeginPopupContextItem(textString.c_str()))
    {
      if(ImGui::Button(fmt::format("Copy###CopyTextToClipboard{}", uniqueId).c_str()))
      {
        ImGui::SetClipboardText(text.c_str());
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }

  template <typename T>
  void imguiNvmlField(const NvmlMonitor::NVMLField<T>& field, const std::string& name, const std::string& unit = "")
  {
    if(field.isSupported)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      ImGui::Text("%s", fmt::format("{}", name).c_str());
      ImGui::TableNextColumn();
      imguiCopyableText(fmt::format("{} {}", field.get(), unit), reinterpret_cast<uint64_t>(&field));
    }
  }


  void imguiDeviceInfo(uint32_t deviceIndex)
  {
#if defined(NVP_SUPPORTS_NVML)
    const NvmlMonitor::DeviceInfo& deviceInfo = m_nvmlMonitor->getDeviceInfo(deviceIndex);

    ImGui::BeginTable(fmt::format("Device Info###DevInfo{}", deviceIndex).c_str(), 2,
                      ImGuiTableFlags_Borders | ImGuiTableFlags_HighlightHoveredColumn | ImGuiTableFlags_RowBg);

    imguiNvmlField(deviceInfo.deviceName, "Device name");
    imguiNvmlField(deviceInfo.brand, "Brand");
    imguiNvmlField(deviceInfo.computeCapabilityMajor, "Compute capability major");
    imguiNvmlField(deviceInfo.computeCapabilityMinor, "Compute capability minor");
    imguiNvmlField(deviceInfo.pcieLinkGen, "PCIe link generation");
    imguiNvmlField(deviceInfo.pcieLinkWidth, "PCIe link width");
    imguiNvmlField(deviceInfo.vbiosVersion, "VBIOS version");

    imguiNvmlField(deviceInfo.boardId, "Board ID");
    imguiNvmlField(deviceInfo.partNumber, "Part number");

    imguiNvmlField(deviceInfo.currentDriverModel, "Current driver model");
    imguiNvmlField(deviceInfo.currentDriverModel, "Pending driver model");

    imguiNvmlField(deviceInfo.cpuAffinity, "CPU affinity");
    imguiNvmlField(deviceInfo.computeMode, "Compute mode");


    imguiNvmlField(deviceInfo.clockDefaultGraphics, "Default clock graphics", "MHz");
    imguiNvmlField(deviceInfo.clockMaxGraphics, "Max clock graphics", "MHz");
    imguiNvmlField(deviceInfo.clockBoostGraphics, "Boost clock graphics", "MHz");

    imguiNvmlField(deviceInfo.clockDefaultSM, "Default clock SM", "MHz");
    imguiNvmlField(deviceInfo.clockMaxSM, "Max clock SM", "MHz");
    imguiNvmlField(deviceInfo.clockBoostSM, "Boost clock SM", "MHz");

    imguiNvmlField(deviceInfo.clockDefaultMem, "Default clock memory", "MHz");
    imguiNvmlField(deviceInfo.clockMaxMem, "Max clock memory", "MHz");
    imguiNvmlField(deviceInfo.clockBoostMem, "Boost clock memory", "MHz");

    imguiNvmlField(deviceInfo.clockDefaultVideo, "Default clock video", "MHz");
    imguiNvmlField(deviceInfo.clockMaxVideo, "Max clock video", "MHz");
    imguiNvmlField(deviceInfo.clockBoostVideo, "Boost clock video", "MHz");

    imguiNvmlField(deviceInfo.currentEccMode, "Current ECC mode");
    imguiNvmlField(deviceInfo.pendingEccMode, "Pending ECC mode");
    imguiNvmlField(deviceInfo.encoderCapacityH264, "Encoder capacity H264", "%");
    imguiNvmlField(deviceInfo.encoderCapacityHEVC, "Encoder capacity HEVC", "%");
    imguiNvmlField(deviceInfo.infoROMImageVersion, "InfoROM image version");
    imguiNvmlField(deviceInfo.infoROMOEMVersion, "InfoROM OEM version");
    imguiNvmlField(deviceInfo.infoROMECCVersion, "InfoROM ECC version");
    imguiNvmlField(deviceInfo.infoROMPowerVersion, "InfoROM power version");
    imguiNvmlField(deviceInfo.supportedClocksThrottleReasons, "Supported clock throttle reasons");

    imguiNvmlField(deviceInfo.maxLinkGen, "Max PCIe link generation");
    imguiNvmlField(deviceInfo.maxLinkWidth, "Max PCIe link width");
    imguiNvmlField(deviceInfo.minorNumber, "Minor number");
    imguiNvmlField(deviceInfo.multiGpuBool, "Multi-GPU setup");


    imguiNvmlField(deviceInfo.tempThresholdShutdown, "Temperature threshold HW Shutdown", "C");
    imguiNvmlField(deviceInfo.tempThresholdHWSlowdown, "Temperature threshold HW Slowdown", "C");
    imguiNvmlField(deviceInfo.tempThresholdSWSlowdown, "Temperature threshold SW Slowdown", "C");
    imguiNvmlField(deviceInfo.tempThresholdDropBelowBaseClock, "Temperature threshold before dropping below base clocks", "C");

    imguiNvmlField(deviceInfo.powerLimit, "Maximum power draw", "W");


    ImGui::EndTable();
#endif
  }

  void imguiDeviceMemory(uint32_t deviceIndex)
  {
#if defined(NVP_SUPPORTS_NVML)
    const NvmlMonitor::DeviceMemory& memory = m_nvmlMonitor->getDeviceMemory(deviceIndex);

    const int offset = m_nvmlMonitor->getOffset();


    std::string bar1Line = fmt::format("BAR1: {}MiB ({}%)", memory.bar1Used.get()[offset] / MIB_SIZE,
                                       (memory.bar1Used.get()[offset] * 100) / memory.bar1Total.get());
    std::string memLine  = fmt::format("Memory: {}MiB ({}%)", memory.memoryUsed.get()[offset] / MIB_SIZE,
                                       (memory.memoryUsed.get()[offset] * 100) / memory.memoryTotal.get());

    static ImPlotFlags     s_plotFlags     = ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotFlags_Crosshairs;
    static ImPlotAxisFlags s_axesFlags     = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoLabel;
    static ImColor         s_graphicsColor = ImColor(0.07f, 0.9f, 0.06f, 1.0f);

    ImVec2 plotSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y / 2);

    // Ensure minimum height to avoid overly squished graphics
    plotSize.y = std::max(plotSize.y, ImGui::GetTextLineHeight() * 5);

    if(ImPlot::BeginPlot("Memory", plotSize, s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, "Bytes", s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);
      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, static_cast<double>(memory.memoryTotal.get()));

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_graphicsColor);
      // Cast to unsigned long long for Linux compilation, where ImPlot functions are not instantiated with uint64_t
      ImPlot::PlotShaded(memLine.c_str(), reinterpret_cast<const unsigned long long*>(memory.memoryUsed.get().data()),
                         (int)memory.memoryUsed.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);
      ImPlot::PopStyleVar();

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse       = ImPlot::GetPlotMousePos();
        int         mouseOffset = (int(mouse.x) + offset) % (int)memory.memoryUsed.get().size();
        ImGui::BeginTooltip();
        ImGui::Text(fmt::format("Used Memory: {}MiB", memory.memoryUsed.get()[mouseOffset] / MIB_SIZE).c_str());
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }

    if(ImPlot::BeginPlot("BAR1", plotSize, s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, "Bytes", s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);
      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, static_cast<double>(memory.bar1Total.get()));

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_graphicsColor);
      // Cast to unsigned long long for Linux compilation, where ImPlot functions are not instantiated with uint64_t
      ImPlot::PlotShaded(bar1Line.c_str(), reinterpret_cast<const unsigned long long*>(memory.bar1Used.get().data()),
                         (int)memory.bar1Used.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);
      ImPlot::PopStyleVar();

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse       = ImPlot::GetPlotMousePos();
        int         mouseOffset = (int(mouse.x) + offset) % (int)memory.bar1Used.get().size();

        ImGui::BeginTooltip();
        ImGui::Text(fmt::format("Used BAR1 Memory: {}MiB", memory.bar1Used.get()[mouseOffset] / MIB_SIZE).c_str());
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }

#endif
  }


  void imguiDevicePerformanceState(uint32_t deviceIndex)
  {
#if defined(NVP_SUPPORTS_NVML)
    const NvmlMonitor::DevicePerformanceState& performanceState = m_nvmlMonitor->getDevicePerformanceState(deviceIndex);
    const NvmlMonitor::DeviceInfo&             deviceInfo       = m_nvmlMonitor->getDeviceInfo(deviceIndex);

    uint32_t generalMaxClock =
        std::max(deviceInfo.clockMaxGraphics.get(), std::max(deviceInfo.clockMaxSM.get(), deviceInfo.clockMaxVideo.get()));


    const int offset = m_nvmlMonitor->getOffset();


    std::string graphicsClockLine = fmt::format("Graphics: {}MHz", performanceState.clockGraphics.get()[offset]);
    std::string smClockLine       = fmt::format("SM: {}MHz", performanceState.clockSM.get()[offset]);
    std::string videoClockLine    = fmt::format("Video: {}MHz", performanceState.clockVideo.get()[offset]);

    static ImPlotFlags     s_plotFlags     = ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotFlags_Crosshairs;
    static ImPlotAxisFlags s_axesFlags     = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoLabel;
    static ImColor         s_graphicsColor = ImColor(0.07f, 0.9f, 0.06f, 1.0f);
    static ImColor         s_smColor       = ImColor(0.06f, 0.6f, 0.97f, 1.0f);
    static ImColor         s_videoColor    = ImColor(0.96f, 0.96f, 0.0f, 1.0f);

    ImVec2 plotSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y / 3);

    plotSize.y = std::max(plotSize.y, ImGui::GetTextLineHeight()
                                          * NvmlMonitor::DevicePerformanceState::getAllThrottleReasonList().size());

    if(ImPlot::BeginPlot("Graphics, Compute and Video clocks", plotSize, s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, "Frequency", s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);
      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, generalMaxClock);

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_graphicsColor);
      ImPlot::PlotShaded(graphicsClockLine.c_str(), performanceState.clockGraphics.get().data(),
                         (int)performanceState.clockGraphics.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);
      ImPlot::SetNextLineStyle(s_smColor);
      ImPlot::PlotLine(smClockLine.c_str(), performanceState.clockSM.get().data(),
                       (int)performanceState.clockSM.get().size(), 1.0, 0.0, 0, offset + 1);
      ImPlot::SetNextLineStyle(s_videoColor);
      ImPlot::PlotLine(videoClockLine.c_str(), performanceState.clockVideo.get().data(),
                       (int)performanceState.clockVideo.get().size(), 1.0, 0.0, 0, offset + 1);

      ImPlot::PopStyleVar();

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse       = ImPlot::GetPlotMousePos();
        int         clockOffset = (int(mouse.x) + offset) % (int)performanceState.clockGraphics.get().size();

        ImGui::BeginTooltip();
        ImGui::Text("Graphics: %dMHz", performanceState.clockGraphics.get()[clockOffset]);
        ImGui::Text("SM: %dMHz", performanceState.clockSM.get()[clockOffset]);
        ImGui::Text("Video: %dMHz", performanceState.clockVideo.get()[clockOffset]);
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }

    std::string memClockLine = fmt::format("Memory: {}MHz", performanceState.clockMem.get()[offset]);
    if(ImPlot::BeginPlot("Memory Clock", plotSize, s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, "Frequency", s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);
      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, generalMaxClock);

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_graphicsColor);
      ImPlot::PlotShaded(memClockLine.c_str(), performanceState.clockMem.get().data(),
                         (int)performanceState.clockMem.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);

      ImPlot::PopStyleVar();

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse       = ImPlot::GetPlotMousePos();
        int         clockOffset = (int(mouse.x) + offset) % (int)performanceState.clockMem.get().size();

        ImGui::BeginTooltip();
        ImGui::Text("Memory: %dMHz", performanceState.clockMem.get()[clockOffset]);
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }

    std::string throttleLine = fmt::format("Throttle reason: {}", NvmlMonitor::DevicePerformanceState::getThrottleReasonStrings(
                                                                      performanceState.throttleReasons.get()[offset])[0]);
    if(ImPlot::BeginPlot("Throttle reason", plotSize, s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, nullptr, s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);


      std::vector<double> throttleValues;
      throttleValues.resize(NvmlMonitor::DevicePerformanceState::getAllThrottleReasonList().size());
      std::vector<std::string> throttleStrings(throttleValues.size());
      std::vector<char*>       throttleCharPtr(throttleValues.size());
      uint64_t                 maxValue = 0;
      for(size_t i = 0; i < throttleValues.size(); i++)
      {
        uint64_t r         = NvmlMonitor::DevicePerformanceState::getAllThrottleReasonList()[i];
        throttleValues[i]  = static_cast<double>(r);
        throttleStrings[i] = NvmlMonitor::DevicePerformanceState::getThrottleReasonStrings(r)[0];
        throttleCharPtr[i] = throttleStrings[i].data();
        maxValue           = std::max(maxValue, r);
      }

      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, static_cast<double>(maxValue));
      ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_SymLog);
      ImPlot::SetupAxisTicks(ImAxis_Y1, throttleValues.data(), static_cast<int>(throttleValues.size()),
                             throttleCharPtr.data(), false);

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_graphicsColor);
      // Cast to unsigned long long for Linux compilation, where ImPlot functions are not instantiated with uint64_t
      ImPlot::PlotShaded(throttleLine.c_str(),
                         reinterpret_cast<const unsigned long long*>(performanceState.throttleReasons.get().data()),
                         (int)performanceState.throttleReasons.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);

      ImPlot::PopStyleVar();

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse          = ImPlot::GetPlotMousePos();
        int         throttleOffset = (int(mouse.x) + offset) % (int)performanceState.throttleReasons.get().size();

        ImGui::BeginTooltip();
        ImGui::Text("Throttle reason: %s", NvmlMonitor::DevicePerformanceState::getThrottleReasonStrings(
                                               performanceState.throttleReasons.get()[throttleOffset])[0]
                                               .c_str());
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }


#endif
  }


  void imguiDevicePowerState(uint32_t deviceIndex)
  {
#if defined(NVP_SUPPORTS_NVML)
    const NvmlMonitor::DevicePowerState& powerState = m_nvmlMonitor->getDevicePowerState(deviceIndex);

    const NvmlMonitor::DeviceInfo& info = m_nvmlMonitor->getDeviceInfo(deviceIndex);


    const int offset = m_nvmlMonitor->getOffset();


    std::string temperatureLine = fmt::format("Temperature: {}C", powerState.temperature.get()[offset]);
    std::string powerLine       = fmt::format("Power: {}W", powerState.power.get()[offset]);
    std::string fanSpeedLine    = fmt::format("Fan speed: {}%", powerState.fanSpeed.get()[offset]);

    static ImPlotFlags     s_plotFlags     = ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotFlags_Crosshairs;
    static ImPlotAxisFlags s_axesFlags     = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoLabel;
    static ImColor         s_graphicsColor = ImColor(0.07f, 0.9f, 0.06f, 1.0f);

    ImVec2 plotSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y / 3);

    // Ensure minimum height to avoid overly squished graphics
    plotSize.y = std::max(plotSize.y, ImGui::GetTextLineHeight() * 5);

    if(ImPlot::BeginPlot("Temperature", plotSize, s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, "Celsius", s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);
      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, info.tempThresholdShutdown.get());

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_graphicsColor);
      ImPlot::PlotShaded(temperatureLine.c_str(), powerState.temperature.get().data(),
                         (int)powerState.temperature.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);

      ImPlot::PopStyleVar();

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse       = ImPlot::GetPlotMousePos();
        int         mouseOffset = (int(mouse.x) + offset) % (int)powerState.temperature.get().size();
        ImGui::BeginTooltip();
        ImGui::Text("Temperature: %dC", powerState.temperature.get()[mouseOffset]);
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }

    if(ImPlot::BeginPlot("Power", plotSize, s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, "Watt", s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);
      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, info.powerLimit.get());

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_graphicsColor);
      ImPlot::PlotShaded(powerLine.c_str(), powerState.power.get().data(), (int)powerState.power.get().size(),
                         -INFINITY, 1.0, 0.0, 0, offset + 1);

      ImPlot::PopStyleVar();

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse       = ImPlot::GetPlotMousePos();
        int         mouseOffset = (int(mouse.x) + offset) % (int)powerState.power.get().size();

        ImGui::BeginTooltip();
        ImGui::Text("Power: %dW", powerState.power.get()[mouseOffset]);
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }

    if(ImPlot::BeginPlot("Fan Speed", plotSize, s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, "%%", s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);
      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, 100);

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_graphicsColor);
      ImPlot::PlotShaded(fanSpeedLine.c_str(), powerState.fanSpeed.get().data(), (int)powerState.fanSpeed.get().size(),
                         -INFINITY, 1.0, 0.0, 0, offset + 1);

      ImPlot::PopStyleVar();

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse       = ImPlot::GetPlotMousePos();
        int         mouseOffset = (int(mouse.x) + offset) % (int)powerState.fanSpeed.get().size();

        ImGui::BeginTooltip();
        ImGui::Text("Power: %dW", powerState.fanSpeed.get()[mouseOffset]);
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }
#endif
  }

  void imguiDeviceUtilization(uint32_t deviceIndex)
  {
#if defined(NVP_SUPPORTS_NVML)
    const NvmlMonitor::DeviceUtilization& utilization = m_nvmlMonitor->getDeviceUtilization(deviceIndex);

    const int offset = m_nvmlMonitor->getOffset();


    std::string gpuUtilizationLine = fmt::format("GPU: {}%", utilization.gpuUtilization.get()[offset]);
    std::string memUtilizationLine = fmt::format("Memory: {}%", utilization.memUtilization.get()[offset]);

    std::string graphicsProcessLine = fmt::format("Graphics processes: {}", utilization.graphicsProcesses.get()[offset]);
    std::string computeProcessLine = fmt::format("Compute processes: {}", utilization.computeProcesses.get()[offset]);

    static ImPlotFlags     s_plotFlags     = ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotFlags_Crosshairs;
    static ImPlotAxisFlags s_axesFlags     = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoLabel;
    static ImColor         s_graphicsColor = ImColor(0.07f, 0.9f, 0.06f, 1.0f);
    static ImColor         s_smColor       = ImColor(0.06f, 0.6f, 0.97f, 1.0f);

    ImVec2 plotSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y / 2);

    // Ensure minimum height to avoid overly squished graphics
    plotSize.y = std::max(plotSize.y, ImGui::GetTextLineHeight() * 5);

    if(ImPlot::BeginPlot("GPU and Memory Utilization", plotSize, s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, "Celsius", s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);
      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, 100);

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_graphicsColor);
      ImPlot::PlotShaded(gpuUtilizationLine.c_str(), utilization.gpuUtilization.get().data(),
                         (int)utilization.gpuUtilization.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);
      ImPlot::SetNextFillStyle(s_smColor);
      ImPlot::PlotShaded(memUtilizationLine.c_str(), utilization.memUtilization.get().data(),
                         (int)utilization.memUtilization.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);

      ImPlot::PopStyleVar();

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse       = ImPlot::GetPlotMousePos();
        int         mouseOffset = (int(mouse.x) + offset) % (int)utilization.gpuUtilization.get().size();
        ImGui::BeginTooltip();
        ImGui::Text("GPU: %d%%", utilization.gpuUtilization.get()[mouseOffset]);
        ImGui::Text("Memory: %d%%", utilization.memUtilization.get()[mouseOffset]);
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }

    if(ImPlot::BeginPlot("Graphics and Compute Processes", plotSize, s_plotFlags))
    {
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
      ImPlot::SetupAxes(nullptr, "Processes", s_axesFlags | ImPlotAxisFlags_NoDecorations, s_axesFlags);
      ImPlot::SetupAxesLimits(0, SAMPLING_NUM, 0, 100);

      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

      ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
      ImPlot::SetNextFillStyle(s_graphicsColor);
      ImPlot::PlotShaded(graphicsProcessLine.c_str(), utilization.graphicsProcesses.get().data(),
                         (int)utilization.graphicsProcesses.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);
      ImPlot::SetNextFillStyle(s_smColor);
      ImPlot::PlotShaded(computeProcessLine.c_str(), utilization.computeProcesses.get().data(),
                         (int)utilization.computeProcesses.get().size(), -INFINITY, 1.0, 0.0, 0, offset + 1);
      ImPlot::PopStyleVar();

      if(ImPlot::IsPlotHovered())
      {
        ImPlotPoint mouse       = ImPlot::GetPlotMousePos();
        int         mouseOffset = (int(mouse.x) + offset) % (int)utilization.graphicsProcesses.get().size();

        ImGui::BeginTooltip();
        ImGui::Text("Graphics: %d", utilization.graphicsProcesses.get()[mouseOffset]);
        ImGui::Text("Compute: %d", utilization.computeProcesses.get()[mouseOffset]);
        ImGui::EndTooltip();
      }

      ImPlot::EndPlot();
    }

#endif
  }

  static void tooltip(const std::string& text, ImGuiHoveredFlags flags = ImGuiHoveredFlags_DelayNormal)
  {
    if(ImGui::IsItemHovered(flags) && ImGui::BeginTooltip())
    {
      ImGui::Text("%s", text.c_str());
      ImGui::EndTooltip();
    }
  }


  void imguiClockSetup(uint32_t deviceIndex)
  {
#if defined(NVP_SUPPORTS_NVML)
    const NvmlMonitor::DeviceInfo& deviceInfo = m_nvmlMonitor->getDeviceInfo(deviceIndex);
    if(deviceInfo.supportedGraphicsClocks.isSupported && !deviceInfo.supportedGraphicsClocks.get().empty())
    {
      ImGui::Text("Supported clocks ");

      float comboWidth = ImGui::GetContentRegionAvail().x / 3.f;

      ImGui::Text("Memory");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(comboWidth);
      if(ImGui::BeginCombo(fmt::format("###DevSupportedGraphicsClocksMemCombo{}", deviceIndex).c_str(),
                           fmt::format("{}MHz", deviceInfo.supportedMemoryClocks.get()[m_selectedMemClock]).c_str()))
      {
        for(size_t i = 0; i < deviceInfo.supportedMemoryClocks.get().size(); i++)
        {
          uint32_t memClock = deviceInfo.supportedMemoryClocks.get()[i];
          bool     selected = false;
          if(ImGui::Selectable(fmt::format("{}MHz", memClock).c_str(), &selected))
          {
            m_selectedMemClock = static_cast<uint32_t>(i);
          }
        }
        ImGui::EndCombo();
      }
      ImGui::SameLine();


      ImGui::Text("Graphics");
      ImGui::SameLine();
      uint32_t activeMemClock = deviceInfo.supportedMemoryClocks.get()[m_selectedMemClock];
      auto defaultClock = deviceInfo.supportedGraphicsClocks.get().find(activeMemClock)->second[m_selectedGraphicsClock];
      ImGui::SetNextItemWidth(comboWidth);
      if(ImGui::BeginCombo(fmt::format("###DevSupportedGraphicsClocks{}", deviceIndex).c_str(),
                           fmt::format("{}MHz", defaultClock).c_str()))
      {
        auto clocks = deviceInfo.supportedGraphicsClocks.get().find(activeMemClock)->second;
        for(size_t i = 0; i < clocks.size(); i++)
        {
          bool selected = false;
          if(ImGui::Selectable(fmt::format("{}MHz", clocks[i]).c_str(), &selected))
          {
            m_selectedGraphicsClock = static_cast<uint32_t>(i);
          }
        }

        ImGui::EndCombo();
      }


      uint32_t currentSelectedMemClock = deviceInfo.supportedMemoryClocks.get()[m_selectedMemClock];
      uint32_t currentSelectedGfxClock =
          deviceInfo.supportedGraphicsClocks.get().find(activeMemClock)->second[m_selectedGraphicsClock];

      std::string nvidiaSmiMemClockLockCommand =
          fmt::format("nvidia-smi -i {} -lmc {},{}", deviceIndex, currentSelectedMemClock, currentSelectedMemClock);
      std::string nvidiaSmiGfxClockLockCommand =
          fmt::format("nvidia-smi -i {} -lgc {},{}", deviceIndex, currentSelectedGfxClock, currentSelectedGfxClock);

      ImGui::Text("NVIDIA-SMI Commands");
      ImGui::TreePush("NVIDIA-SMI Commands");

      ImGui::BeginTable(fmt::format("NVIDIA-SMI commands###NVSMICMD{}", deviceIndex).c_str(), 2,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_HighlightHoveredColumn | ImGuiTableFlags_RowBg);
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("Memory clock lock");
      ImGui::TableNextColumn();
      imguiCopyableText(nvidiaSmiMemClockLockCommand, reinterpret_cast<uint64_t>(&nvidiaSmiMemClockLockCommand));

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("Graphics clock lock");
      ImGui::TableNextColumn();
      imguiCopyableText(nvidiaSmiGfxClockLockCommand, reinterpret_cast<uint64_t>(&nvidiaSmiGfxClockLockCommand));

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("Memory clock unlock (reset to default behavior)");
      ImGui::TableNextColumn();
      std::string memoryClockResetCommand = fmt::format("nvidia-smi -i {} -rmc", deviceIndex);
      imguiCopyableText(memoryClockResetCommand, reinterpret_cast<uint64_t>(&memoryClockResetCommand));

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("Graphics clock unlock (reset to default behavior)");
      ImGui::TableNextColumn();
      std::string graphicsClockResetCommand = fmt::format("nvidia-smi -i {} -rgc", deviceIndex);
      imguiCopyableText(graphicsClockResetCommand, reinterpret_cast<uint64_t>(&graphicsClockResetCommand));

      ImGui::EndTable();
      tooltip("Copy these commands into an \nAdministrator console to setup\n the GPU clocks");

      ImGui::TreePop();
    }
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
  bool           m_showWindow{false};
  bool           m_throttleDetected{false};
  uint64_t       m_lastThrottleReason{0ull};
  nvh::Stopwatch m_throttleCooldownTimer;

  uint32_t m_selectedMemClock{0u};
  uint32_t m_selectedGraphicsClock{0u};

#if defined(NVP_SUPPORTS_NVML)
  std::unique_ptr<NvmlMonitor> m_nvmlMonitor;
  AverageCircularBuffer<float> m_avgCpu = {SAMPLING_NUM};
#endif
};


}  // namespace nvvkhl
