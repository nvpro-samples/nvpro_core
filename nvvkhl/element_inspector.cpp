/*
 * Copyright (c) 2023, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2023 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "element_inspector.hpp"
#include "nvvk/commands_vk.hpp"
#include "shaders/dh_inspector.h"
#include <algorithm>
#include <ctype.h>
#include <glm/detail/type_half.hpp>
#include <iomanip>
#include <regex>
#include "imgui.h"
#include <chrono>
#include "imgui_internal.h"
#include "nvh/parallel_work.hpp"
#include "imgui/imgui_icon.h"

// Time during which a selected row will flash, in ms
#define SELECTED_FLASH_DURATION 800.0
// Number of times the selected row flashes
#define SELECTED_FLASH_COUNT 3
// Half-size of the area covevered by the magnifying glass when hovering an image, in pixels
#define ZOOM_HALF_SIZE 3.0f
// Size of the buttons for images and buffers
#define SQUARE_BUTTON_SIZE 64.f

#define VALUE_FLAG_INTERNAL 0x2

// Number of threads used when filtering buffer contents
#define FILTER_THREAD_COUNT std::thread::hardware_concurrency() / 2

// Maximum number of entries in a buffer for which filtering will be automatically updated
// Above this threshold the user has to click on the "Apply" button to apply the filter to preserve
// interactivity
#define FILTER_AUTO_UPDATE_THRESHOLD 1024 * 1024
namespace nvvkhl {

static const ImVec4 highlightColor = ImVec4(118.f / 255.f, 185.f / 255.f, 0.f, 1.f);

static bool isActiveButtonPushed = false;
void        imguiPushActiveButtonStyle(bool active)
{
  static const ImVec4 selectedColor = highlightColor;
  static const ImVec4 hoveredColor = ImVec4(selectedColor.x * 1.2f, selectedColor.y * 1.2f, selectedColor.z * 1.2f, 1.f);
  if(active)
  {
    ImGui::PushStyleColor(ImGuiCol_Button, selectedColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
    isActiveButtonPushed = true;
  }
}

static void imguiPopActiveButtonStyle()
{
  if(isActiveButtonPushed)
  {
    ImGui::PopStyleColor(2);
    isActiveButtonPushed = false;
  }
}

static bool checkFormatFlag(const std::vector<ElementInspector::ValueFormat>& format)
{
  for(auto& f : format)
  {
    if(f.flags >= ElementInspector::eValueFormatFlagCount)
    {
      LOGE("ValueFormat::flags is invalid\n");
      assert(false);
      return false;
    }
  }
  return true;
}

static void sanitizeExtent(glm::uvec3& extent)
{
  extent = glm::max({1, 1, 1}, extent);
}


static void tooltip(const std::string& text, ImGuiHoveredFlags flags = ImGuiHoveredFlags_DelayNormal)
{
  if(ImGui::IsItemHovered(flags) && ImGui::BeginTooltip())
  {
    ImGui::Text(text.c_str());
    ImGui::EndTooltip();
  }
}

void ElementInspector::onAttach(nvvkhl::Application* app)
{
  m_app = app;

  VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.magFilter    = VK_FILTER_NEAREST;
  samplerInfo.minFilter    = VK_FILTER_NEAREST;

  vkCreateSampler(m_app->getDevice(), &samplerInfo, nullptr, &m_sampler);
  m_isAttached = true;
}

void ElementInspector::onDetach()
{

  if(!m_isAttached)
  {
    return;
  }

  NVVK_CHECK(vkDeviceWaitIdle(m_app->getDevice()));

  deinit();
  m_isAttached = false;
}

void ElementInspector::onUIRender()
{
  if(!m_isAttached)
  {
    return;
  }
  m_childIndex = 1;

  ImGui::Begin("Inspector");

  {
    imguiPushActiveButtonStyle(m_settings.isPaused);

    ImGuiH::getIconicFont()->Scale *= 2;
    ImGui::PushFont(ImGuiH::getIconicFont());

    if(ImGui::Button(m_settings.isPaused ? ImGuiH::icon_media_pause : ImGuiH::icon_media_play,
                     {SQUARE_BUTTON_SIZE / 2, SQUARE_BUTTON_SIZE / 2}))
    {
      m_settings.isPaused = !m_settings.isPaused;
    }
    ImGui::PopFont();
    ImGuiH::getIconicFont()->Scale /= 2;
    imguiPopActiveButtonStyle();
    tooltip("Pause inspection, effectively freezing the displayed values");

    if(!m_inspectedComputeVariables.empty())
    {
      imguiPopActiveButtonStyle();
      ImGui::SameLine();
      imguiPushActiveButtonStyle(m_settings.showInactiveBlocks);
      if(ImGui::Button("Show inactive\n blocks", {SQUARE_BUTTON_SIZE * 2, SQUARE_BUTTON_SIZE / 2}))
      {
        m_settings.showInactiveBlocks = !m_settings.showInactiveBlocks;
      }
      imguiPopActiveButtonStyle();
      tooltip("If enabled, blocks for which no inspection is enabled will be shown with inactive buttons");
    }
    ImGui::SameLine();
    if(m_isFilterTimeout)
    {
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 0.f, 0.f, 1.f));
      ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "Filter timeout in seconds");
    }
    else
    {
      ImGui::Text("Filter timeout in seconds");
    }
    tooltip("If filtering a column takes more than the specified time, an error will be logged and filtering will be cancelled");

    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::GetFontSize() * 4);
    ImGui::InputFloat("##FilterTimeout", &m_settings.filterTimeoutInSeconds, 0.f, 0.f, "%.1f");
    ImGui::PopItemWidth();

    if(m_isFilterTimeout)
    {
      ImGui::PopStyleColor();
    }
  }

  if(m_settings.isPaused)
  {
    ImGui::PushStyleColor(ImGuiCol_Border, highlightColor);
  }

  ImGui::BeginDisabled(m_inspectedImages.empty());
  if(ImGui::CollapsingHeader("Images", ImGuiTreeNodeFlags_DefaultOpen))
  {

    ImGui::TreePush("###");

    ImVec2 imageDisplaySize = ImGui::GetContentRegionAvail();
    imageDisplaySize.x /= m_inspectedImages.size();

    for(size_t imageIndex = 0; imageIndex < m_inspectedImages.size(); imageIndex++)
    {
      if(imageIndex > 0)
      {
        ImGui::SameLine();
      }

      ImGui::BeginGroup();

      imguiPushActiveButtonStyle(m_inspectedImages[imageIndex].show);

      if(ImGui::ImageButton(fmt::format("##ImageButton{}", imageIndex).c_str(),
                            m_inspectedImages[imageIndex].imguiImage, ImVec2(SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE)))
      {
        m_inspectedImages[imageIndex].show = !m_inspectedImages[imageIndex].show;
      }
      imguiPopActiveButtonStyle();
      ImGui::Text(m_inspectedImages[imageIndex].name.c_str());


      ImGui::EndGroup();
    }

    bool firstImage = true;
    for(size_t imageIndex = 0; imageIndex < m_inspectedImages.size(); imageIndex++)
    {

      if(m_inspectedImages[imageIndex].show)
      {
        if(firstImage)
        {
          firstImage = false;
        }
        else
        {
          ImGui::SameLine();
        }
        imGuiImage(imageIndex, imageDisplaySize);
      }
    }
    ImGui::TreePop();
  }
  ImGui::EndDisabled();


  ImGui::BeginDisabled(m_inspectedBuffers.empty());
  if(ImGui::CollapsingHeader("Buffers", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush("###");
    ImVec2 widgetSize = ImGui::GetContentRegionAvail();
    widgetSize.x /= m_inspectedBuffers.size();

    for(size_t bufferIndex = 0; bufferIndex < m_inspectedBuffers.size(); bufferIndex++)
    {
      if(bufferIndex > 0)
      {
        ImGui::SameLine();
      }

      ImGui::BeginDisabled(!m_inspectedBuffers[bufferIndex].isInspected);

      imguiPushActiveButtonStyle(m_inspectedBuffers[bufferIndex].show);
      if(ImGui::Button(m_inspectedBuffers[bufferIndex].isAllocated ? m_inspectedBuffers[bufferIndex].name.c_str() : "",
                       ImVec2(SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE)))
      {
        m_inspectedBuffers[bufferIndex].show = !m_inspectedBuffers[bufferIndex].show;
      }

      imguiPopActiveButtonStyle();

      if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && m_inspectedBuffers[bufferIndex].isInspected)
      {
        if(ImGui::BeginTooltip())
        {
          std::stringstream sss;
          sss << m_inspectedBuffers[bufferIndex].name << ": " << m_inspectedBuffers[bufferIndex].comment << std::endl;
          if(m_inspectedBuffers[bufferIndex].format.size() > 1)
          {
            sss << "struct" << std::endl << "{" << std::endl;
            for(size_t i = 0; i < m_inspectedBuffers[bufferIndex].format.size(); i++)
            {
              auto& f = m_inspectedBuffers[bufferIndex].format[i];

              sss << "  " << valueFormatTypeToString(f).c_str() << " " << f.name << std::endl;
            }
            sss << "}";
            if(m_inspectedBuffers[bufferIndex].entryCount > 1)
            {
              sss << "[]";
            }
            sss << std::endl;
          }
          else
          {
            const auto& f = m_inspectedBuffers[bufferIndex].format[0];
            sss << "  " << valueFormatTypeToString(f).c_str() << " " << f.name;
            if(m_inspectedBuffers[bufferIndex].entryCount > 1)
            {
              sss << "[]";
            }
            sss << std::endl;
          }

          ImGui::Text(sss.str().c_str());
          ImGui::EndTooltip();
        }
      }
      ImGui::EndDisabled();
    }


    for(size_t bufferIndex = 0; bufferIndex < m_inspectedBuffers.size(); bufferIndex++)
    {
      imGuiBuffer(m_inspectedBuffers[bufferIndex], ~0u, {m_inspectedBuffers[bufferIndex].entryCount, 1, 1}, true);
    }
    ImGui::TreePop();
  }
  ImGui::EndDisabled();

  ImGui::BeginDisabled(m_inspectedComputeVariables.empty());
  if(ImGui::CollapsingHeader("Compute variables", ImGuiTreeNodeFlags_DefaultOpen))
  {
    if(!m_inspectedComputeVariables.empty())
    {
      ImGui::TreePush("###");
      ImVec2 widgetSize = ImGui::GetContentRegionAvail();
      widgetSize.x /= m_inspectedComputeVariables.size();

      for(size_t i = 0; i < m_inspectedComputeVariables.size(); i++)
      {
        if(i > 0)
        {
          ImGui::SameLine();
        }
        imGuiComputeVariable(i);
      }
      ImGui::TreePop();
    }
  }
  ImGui::EndDisabled();

  ImGui::BeginDisabled(m_inspectedFragmentVariables.empty());
  if(ImGui::CollapsingHeader("Fragment variables", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush("###");
    for(size_t i = 0; i < m_inspectedFragmentVariables.size(); i++)
    {
      if(i > 0)
      {
        ImGui::SameLine();
      }
      m_inspectedFragmentVariables[i].show = true;
      imGuiBuffer(m_inspectedFragmentVariables[i], ~0u,
                  {m_inspectedFragmentVariables[i].maxFragment.x - m_inspectedFragmentVariables[i].minFragment.x + 1,
                   m_inspectedFragmentVariables[i].maxFragment.y - m_inspectedFragmentVariables[i].minFragment.y + 1, 1},
                  true, {m_inspectedFragmentVariables[i].minFragment, 1});
    }
    ImGui::TreePop();
  }
  ImGui::EndDisabled();

  ImGui::BeginDisabled(m_inspectedCustomVariables.empty());
  if(ImGui::CollapsingHeader("Custom variables", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush("###");
    for(size_t i = 0; i < m_inspectedCustomVariables.size(); i++)
    {
      if(i > 0)
      {
        ImGui::SameLine();
      }
      m_inspectedCustomVariables[i].show = true;
      imGuiBuffer(m_inspectedCustomVariables[i], ~0u,
                  {m_inspectedCustomVariables[i].maxCoord.x - m_inspectedCustomVariables[i].minCoord.x + 1,
                   m_inspectedCustomVariables[i].maxCoord.y - m_inspectedCustomVariables[i].minCoord.y + 1,
                   m_inspectedCustomVariables[i].maxCoord.z - m_inspectedCustomVariables[i].minCoord.z + 1},
                  true, m_inspectedCustomVariables[i].minCoord);
    }
    ImGui::TreePop();
  }
  ImGui::EndDisabled();


  if(m_settings.isPaused)
  {
    ImGui::PopStyleColor();
  }
  ImGui::End();
}

void ElementInspector::imGuiComputeVariable(uint32_t i)
{
  auto& computeVar = m_inspectedComputeVariables[i];

  ImGui::TextDisabled(computeVar.name.c_str());
  if(!computeVar.comment.empty())
  {
    ImGui::TextDisabled(computeVar.comment.c_str());
  }
  if(computeVar.isAllocated)
  {

    const uint8_t* bufferContent = (uint8_t*)m_alloc->map(computeVar.hostBuffer);

    imGuiGrid(i, computeVar, bufferContent);

    m_alloc->unmap(computeVar.hostBuffer);
  }
}

void ElementInspector::imGuiImage(uint32_t imageIndex, ImVec2& imageSize)
{
  auto& img = m_inspectedImages[imageIndex];
  if(!img.isInspected)
  {
    return;
  }

  float  imageAspect  = float(img.createInfo.extent.width) / float(img.createInfo.extent.height);
  float  regionAspect = imageSize.x / imageSize.y;
  ImVec2 localSize;
  localSize.x = imageSize.x;
  localSize.y = imageSize.y * regionAspect / imageAspect;

  ImGui::BeginChild(m_childIndex++, localSize, true, ImGuiWindowFlags_ChildWindow);

  ImGui::TextDisabled(img.name.c_str());
  if(!img.comment.empty())
  {
    ImGui::TextDisabled(img.comment.c_str());
  }
  ImGui::SameLine();
  if(ImGui::Button(img.tableView ? "Image view" : "Table view"))
  {
    img.tableView = !img.tableView;
  }
  if(img.isAllocated)
  {
    if(img.tableView)
    {
      img.show = true;
      imGuiBuffer(img, img.selectedPixelIndex,
                  {img.createInfo.extent.width, img.createInfo.extent.height, img.createInfo.extent.depth}, true);
      img.selectedPixelIndex = ~0u;
    }
    else
    {
      ImVec2 regionSize = ImGui::GetContentRegionAvail();
      ImGui::Image(img.imguiImage, regionSize);
      const auto offset = ImGui::GetItemRectMin();

      // Render zoomed in image at the position of the cursor.
      if(ImGui::IsItemHovered())
      {
        if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
          const auto cursor = ImGui::GetIO().MousePos;

          ImVec2     delta       = {cursor.x - offset.x, cursor.y - offset.y};
          ImVec2     imageSize   = ImVec2(img.createInfo.extent.width, img.createInfo.extent.height);
          const auto center      = imageSize * delta / regionSize;
          uint32_t   pixelIndex  = (uint32_t)center.x + imageSize.x * (uint32_t)center.y;
          img.selectedPixelIndex = pixelIndex;
          img.tableView          = true;
        }

        if(ImGui::BeginTooltip())
        {
          const auto cursor = ImGui::GetIO().MousePos;

          ImVec2 delta     = {cursor.x - offset.x, cursor.y - offset.y};
          ImVec2 imageSize = ImVec2(img.createInfo.extent.width, img.createInfo.extent.height);
          auto   center    = imageSize * delta / regionSize;

          center.x              = floor(center.x);
          center.y              = floor(center.y);
          ImVec2     zoomRegion = ImVec2(ZOOM_HALF_SIZE, ZOOM_HALF_SIZE);
          const auto uv0        = (center - zoomRegion) / imageSize;
          const auto uv1        = (center + zoomRegion + ImVec2(1, 1)) / imageSize;


          ImVec2 pixelCoord = imageSize * delta;
          ImGui::Text("(%d, %d)", (int32_t)center.x, (int32_t)center.y);

          const uint8_t* contents   = static_cast<uint8_t*>(m_alloc->map(img.hostBuffer));
          uint32_t       pixelIndex = (uint32_t)center.x + imageSize.x * (uint32_t)center.y;

          contents += pixelIndex * formatSizeInBytes(img.format);
          ImGui::Text(bufferEntryToString(contents, img.format).c_str());
          m_alloc->unmap(img.hostBuffer);
          ImVec2 currentPos = ImGui::GetCursorPos();
          ImGui::Image(img.imguiImage, ImVec2(SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE), uv0, uv1);


          float pixelSize = SQUARE_BUTTON_SIZE / (2 * ZOOM_HALF_SIZE + 1);
          currentPos.x += ZOOM_HALF_SIZE * pixelSize;
          currentPos.y += ZOOM_HALF_SIZE * pixelSize;

          ImGui::SetCursorPos(currentPos);

          ImGui::PushStyleColor(ImGuiCol_Border, highlightColor);
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
          ImGui::Button(" ", ImVec2(SQUARE_BUTTON_SIZE / (2 * ZOOM_HALF_SIZE + 1), SQUARE_BUTTON_SIZE / (2 * ZOOM_HALF_SIZE + 1)));
          ImGui::PopStyleColor(2);
          ImGui::EndTooltip();
        }
      }
    }
  }
  ImGui::EndChild();
}

uint32_t ElementInspector::imGuiBufferContents(InspectedBuffer& buf,
                                               const uint8_t*   contents,
                                               uint32_t         begin,
                                               uint32_t         end,
                                               uint32_t         entrySizeInBytes,
                                               glm::uvec3       extent,
                                               uint32_t         previousFilteredOut,
                                               uint32_t         scrollToItem /*=~0u*/,
                                               glm::uvec3       coordDisplayOffset /*= {0,0}*/
)
{
  sanitizeExtent(extent);
  uint32_t filteredOut = previousFilteredOut;
  for(uint32_t i = begin; i < end; i++)
  {
    uint32_t sourceBufferEntryIndex = i + buf.viewMin + filteredOut;
    uint32_t hostBufferEntryIndex   = sourceBufferEntryIndex - buf.offsetInEntries;

    const uint8_t* entryData = contents + entrySizeInBytes * hostBufferEntryIndex;

    while(!buf.filter.filterPasses(entryData) && hostBufferEntryIndex < buf.entryCount)
    {
      filteredOut++;
      sourceBufferEntryIndex++;
      hostBufferEntryIndex++;
      entryData += entrySizeInBytes;
    }

    if(filteredOut == buf.entryCount)
    {
      ImGui::TableNextRow();
      for(size_t i = 0; i < buf.format.size(); i++)
      {
        ImGui::TableNextColumn();
        ImGui::TextDisabled("Not found");
      }
      break;
    }

    if(i == scrollToItem)
    {
      ImGui::SetScrollHereY();
    }

    if(hostBufferEntryIndex == buf.entryCount || sourceBufferEntryIndex > buf.viewMax)
    {
      ImGui::TableNextRow();
      for(size_t i = 0; i < buf.format.size(); i++)
      {
        ImGui::TableNextColumn();
        ImGui::TextDisabled("");
      }
      break;
    }
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      if(buf.selectedRow != ~0u && buf.selectedRow == sourceBufferEntryIndex)
      {
        uint32_t flash = uint32_t((buf.selectedFlashTimer.elapsed() / SELECTED_FLASH_DURATION) * (2 * SELECTED_FLASH_COUNT + 1));
        if(flash % 2 == 0)
        {
          ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::ColorConvertFloat4ToU32(highlightColor));
        }
      }
      if(extent.y == 1 && extent.z == 1)
      {
        ImGui::TextDisabled("%d", sourceBufferEntryIndex + coordDisplayOffset.x);
      }
      else
      {
        if(extent.z == 1)
        {
          ImGui::TextDisabled("(%d, %d)", coordDisplayOffset.x + (sourceBufferEntryIndex) % extent.x,
                              coordDisplayOffset.y + (sourceBufferEntryIndex) / extent.x);
        }
        else
        {
          ImGui::TextDisabled("(%d, %d, %d)", coordDisplayOffset.x + (sourceBufferEntryIndex) % extent.x,
                              coordDisplayOffset.y + ((sourceBufferEntryIndex) / extent.x) % extent.y,
                              coordDisplayOffset.z + (sourceBufferEntryIndex / (extent.x * extent.y)));
        }
      }

      imGuiColumns(entryData, buf.format);

      if(buf.selectedRow == sourceBufferEntryIndex)
      {
        if(buf.selectedFlashTimer.elapsed() > SELECTED_FLASH_DURATION)
        {
          buf.selectedRow = ~0u;
        }
      }
    }
  }
  return filteredOut;
}


void ElementInspector::imGuiBuffer(InspectedBuffer& buf,
                                   uint32_t         topItem /*= ~0u*/,
                                   glm::uvec3       extent,
                                   bool             defaultOpen /*= false*/,
                                   glm::uvec3       coordDisplayOffset /*= {0,0}*/)
{
  sanitizeExtent(extent);
  if(buf.isAllocated && buf.isInspected && buf.show
     && ImGui::CollapsingHeader(buf.name.c_str(), defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0))
  {

    bool is1d = extent.y == 1 && extent.z == 1;
    if(!buf.comment.empty())
    {
      ImGui::TextDisabled(buf.comment.c_str());
    }
    if(buf.entryCount > 1)
    {
      //  Range choice only for 1D buffers
      if(is1d)
      {
        ImGui::Text("Display Range");
        tooltip("Reduce the range of displayed values");
        ImGui::SameLine();

        int32_t rangeMin = buf.viewMin + coordDisplayOffset.x;
        int32_t rangeMax = buf.viewMax + coordDisplayOffset.x;
        ImGui::DragIntRange2("###", &rangeMin, &rangeMax, 1.f, buf.offsetInEntries + coordDisplayOffset.x,
                             buf.offsetInEntries + coordDisplayOffset.x + buf.entryCount - 1);

        if(rangeMin >= buf.viewMin)
        {
          buf.viewMin = rangeMin - coordDisplayOffset.x;
        }
        if(rangeMax >= buf.viewMin)
        {
          buf.viewMax = rangeMax - coordDisplayOffset.x;
        }
      }

      if(ImGui::BeginPopup("Go to entry", ImGuiWindowFlags_Popup))
      {
        int32_t line = buf.selectedRow;

        if(is1d)
        {
          ImGui::SetKeyboardFocusHere();
          int32_t inputLine = line + coordDisplayOffset.x;
          ImGui::InputInt("###", &inputLine);
          if(inputLine < coordDisplayOffset.x)
          {
            line = 0;
          }
          else
          {
            line = inputLine - coordDisplayOffset.x;
          }
        }
        else
        {
          if(extent.z == 1)  // 2D
          {
            int32_t coord[2] = {static_cast<int32_t>(coordDisplayOffset.x) + line % int32_t(extent.x),
                                static_cast<int32_t>(coordDisplayOffset.y) + line / int32_t(extent.x)};
            ImGui::InputInt2("Coordinates", coord);
            if(coord[0] < coordDisplayOffset.x)
            {
              coord[0] = 0;
            }
            else
            {
              coord[0] -= coordDisplayOffset.x;
            }
            if(coord[1] < coordDisplayOffset.y)
            {
              coord[1] = 0;
            }
            else
            {
              coord[1] -= coordDisplayOffset.y;
            }
            line = coord[0] + coord[1] * extent.x;
          }
          else  // 3D
          {
            int32_t coord[3] = {
                static_cast<int32_t>(coordDisplayOffset.x) + line % int32_t(extent.x),
                static_cast<int32_t>(coordDisplayOffset.y) + (line / int32_t(extent.x)) % int32_t(extent.y),
                static_cast<int32_t>(coordDisplayOffset.z) + (line / int32_t(extent.x * extent.y)),
            };
            ImGui::InputInt3("Coordinates", coord);
            if(coord[0] < coordDisplayOffset.x)
            {
              coord[0] = 0;
            }
            else
            {
              coord[0] -= coordDisplayOffset.x;
            }

            if(coord[1] < coordDisplayOffset.y)
            {
              coord[1] = 0;
            }
            else
            {
              coord[1] -= coordDisplayOffset.y;
            }

            if(coord[2] < coordDisplayOffset.z)
            {
              coord[2] = 0;
            }
            else
            {
              coord[2] -= coordDisplayOffset.z;
            }


            line = coord[0] + extent.x * (coord[1] + coord[2] * extent.y);
          }
        }
        buf.selectedRow = line;
        if(ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeyPadEnter))
        {
          topItem = line;
          ImGui::CloseCurrentPopup();
        }
        if(ImGui::Button("OK"))
        {
          topItem = line;
          ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel"))
        {
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }
      if(ImGui::Button("Go to..."))
      {
        ImGui::OpenPopup("Go to entry");
      }
      tooltip("Jump to a specific entry");
    }

    uint32_t visibleColumns = 0;
    for(auto& f : buf.format)
    {
      if(f.flags == ElementInspector::eVisible)
      {
        visibleColumns++;
      }
    }

    ImGui::BeginTable(buf.name.c_str(), visibleColumns + 1, ImGuiTableFlags_Borders | ImGuiTableFlags_HighlightHoveredColumn);
    ImGui::TableHeadersRow();


    size_t entrySizeInBytes = formatSizeInBytes(buf.format);
    ImGui::TableNextColumn();

    ImGui::TextDisabled("Index");
    for(size_t i = 0; i < buf.format.size(); i++)
    {

      if(buf.format[i].flags != VALUE_FLAG_INTERNAL && buf.format[i].flags == ElementInspector::eVisible)
      {
        ImGui::TableNextColumn();
        ImGui::Text(valueFormatToString(buf.format[i]).c_str());
        imguiPushActiveButtonStyle(buf.format[i].hexDisplay);
        if(ImGui::Button(fmt::format("Hex##Buffer{}{}", buf.name, i).c_str()))
        {
          buf.format[i].hexDisplay = !buf.format[i].hexDisplay;
        }
        imguiPopActiveButtonStyle();
        tooltip("Display values as hexadecimal");
        ImGui::SameLine();
        imguiPushActiveButtonStyle(buf.filter.hasFilter[i]);
        if(ImGui::Button(fmt::format("Filter##Buffer{}{}", buf.name, i).c_str()))
        {
          buf.filter.hasFilter[i] = !buf.filter.hasFilter[i];
        }
        tooltip("Filter the values in the column");
        imguiPopActiveButtonStyle();
      }
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    ImGui::BeginDisabled(buf.filter.dataMax == buf.filter.requestedDataMax
                         && buf.filter.dataMin == buf.filter.requestedDataMin && !m_isFilterTimeout);
    if(buf.entryCount < FILTER_AUTO_UPDATE_THRESHOLD || ImGui::Button(fmt::format("Apply##FilterBuffer{}", buf.name).c_str()))
    {
      buf.filter.dataMin  = buf.filter.requestedDataMin;
      buf.filter.dataMax  = buf.filter.requestedDataMax;
      buf.filteredEntries = ~0u;
    }
    ImGui::EndDisabled();


    const uint8_t* bufferContent = (uint8_t*)m_alloc->map(buf.hostBuffer);
    uint32_t       filteredOut   = 0u;
    uint32_t       viewed        = 0u;

    std::atomic_uint32_t viewSize = 0;
    if(buf.filteredEntries == ~0u)
    {
      if(buf.filter.hasAnyFilter())
      {

        nvh::Stopwatch timeoutTimer;
        m_isFilterTimeout = false;
        std::atomic_bool isTimeout{false};
        nvh::parallel_batches(
            buf.viewMax - buf.viewMin + 1,
            [&](uint64_t i, uint32_t threadIdx) {
              if(timeoutTimer.elapsed() > 1000.f * m_settings.filterTimeoutInSeconds)
              {

                if(!isTimeout.exchange(true))
                {
                  LOGE("Inspector filter timeout - consider reducing the buffer filtering range or increasing timeout (at the expense of interactivity)\n");
                  ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "Filter timeout");
                  m_isFilterTimeout = true;
                }
                return;
              }

              const uint8_t* entryData = bufferContent + entrySizeInBytes * (i + buf.viewMin);
              if(buf.filter.filterPasses(entryData))
              {
                viewSize++;
              }
            },
            FILTER_THREAD_COUNT);
      }
      else
      {
        viewSize = buf.viewMax - buf.viewMin + 1;
      }
      buf.filteredEntries = viewSize;
    }
    else
    {
      viewSize = buf.filteredEntries;
    }

    buf.filter.imguiFilterColumns();

    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    if(topItem == ~0u)
    {
      ImGuiListClipper clipper;

      if(viewSize == 0)
      {
        ImGui::TableNextRow();
        for(size_t i = 0; i < visibleColumns + 1; i++)
        {
          ImGui::TableNextColumn();
          ImGui::TextDisabled("Not found");
        }
      }
      else
      {
        clipper.Begin(viewSize);

        while(clipper.Step())
        {
          filteredOut = imGuiBufferContents(buf, bufferContent, clipper.DisplayStart, clipper.DisplayEnd,
                                            entrySizeInBytes, extent, filteredOut, ~0u, coordDisplayOffset);
        }

        clipper.End();
      }
    }
    else
    {

      filteredOut = imGuiBufferContents(buf, bufferContent, 0, buf.entryCount, entrySizeInBytes, extent, filteredOut,
                                        topItem, coordDisplayOffset);
      buf.selectedRow = topItem;
      buf.selectedFlashTimer.reset();
    }
    m_alloc->unmap(buf.hostBuffer);
    ImGui::EndTable();
  }
}


glm::uvec3 ElementInspector::getThreadInvocationId(uint32_t                   absoluteBlockIndex,
                                                   uint32_t                   warpIndex,
                                                   uint32_t                   localInvocationId,
                                                   InspectedComputeVariables& v)
{
  glm::uvec3 blockStart = getBlockIndex(absoluteBlockIndex, v) * v.blockSize;

  glm::uvec3 warpStart    = {};
  glm::uvec3 threadInWarp = {};
  if(v.blockSize.y == 1 && v.blockSize.z == 1)
  {
    warpStart.x    = warpIndex * WARP_SIZE;
    threadInWarp.x = localInvocationId;
  }
  else
  {
    glm::uvec3 warpsPerBlock = {};
    warpsPerBlock.x          = v.blockSize.x / WARP_2D_SIZE_X;
    warpsPerBlock.y          = v.blockSize.y / WARP_2D_SIZE_Y;
    warpsPerBlock.z          = v.blockSize.z / WARP_2D_SIZE_Z;

    glm::uvec3 warpCoord = {};
    warpCoord.z          = warpIndex % warpsPerBlock.z;
    warpCoord.x          = (warpIndex / warpsPerBlock.z) % warpsPerBlock.x;
    warpCoord.y          = (warpIndex / (warpsPerBlock.z * warpsPerBlock.x));

    warpStart.x = warpCoord.x * WARP_2D_SIZE_X;
    warpStart.y = warpCoord.y * WARP_2D_SIZE_Y;
    warpStart.z = warpCoord.z * WARP_2D_SIZE_Z;

    threadInWarp.x = localInvocationId % WARP_2D_SIZE_X;
    threadInWarp.y = (localInvocationId / WARP_2D_SIZE_X) % WARP_2D_SIZE_Y;
    threadInWarp.z = (localInvocationId / (WARP_2D_SIZE_X * WARP_2D_SIZE_Y));
  }

  return blockStart + warpStart + threadInWarp;
}

std::string ElementInspector::multiDimUvec3ToString(const glm::uvec3 v, bool forceMultiDim /*= false*/)
{
  if(!forceMultiDim && v.y <= 1 && v.z <= 1)
  {
    return "";
  }
  if(!forceMultiDim && v.z <= 1)
  {
    return fmt::format("({}, {})", v.x, v.y);
  }
  return fmt::format("({}, {}, {})", v.x, v.y, v.z);
}

void ElementInspector::createInspectedBuffer(InspectedBuffer&                inspectedBuffer,
                                             VkBuffer                        sourceBuffer,
                                             const std::string&              name,
                                             const std::vector<ValueFormat>& format,
                                             uint32_t                        entryCount,
                                             const std::string&              comment /*= ""*/,
                                             uint32_t                        offsetInEntries /*= 0u*/,
                                             uint32_t                        viewMin /*= 0u*/,
                                             uint32_t                        viewMax /*= ~0u*/)
{
  uint32_t sizeInBytes = formatSizeInBytes(format) * entryCount;

  inspectedBuffer.hostBuffer  = m_alloc->createBuffer(sizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  inspectedBuffer.format      = format;
  inspectedBuffer.entryCount  = entryCount;
  inspectedBuffer.isAllocated = true;
  inspectedBuffer.name        = name;
  inspectedBuffer.comment     = comment;
  inspectedBuffer.filter.create();


  inspectedBuffer.viewMin         = viewMin;
  inspectedBuffer.viewMax         = std::min(viewMax, entryCount - 1);
  inspectedBuffer.offsetInEntries = offsetInEntries;
  inspectedBuffer.sourceBuffer    = sourceBuffer;
  inspectedBuffer.filteredEntries = ~0u;
}

void ElementInspector::destroyInspectedBuffer(InspectedBuffer& inspectedBuffer)
{
  m_alloc->destroy(inspectedBuffer.hostBuffer);
  inspectedBuffer.entryCount  = 0u;
  inspectedBuffer.isAllocated = false;
  inspectedBuffer.filter.destroy();
}


uint32_t getCapturedBlockIndex(uint32_t absoluteBlockIndex, const glm::uvec3& gridSize, const glm::uvec3& minBlock, const glm::uvec3& maxBlock)
{

  uint32_t x = absoluteBlockIndex % gridSize.x;
  absoluteBlockIndex /= gridSize.x;
  uint32_t y = absoluteBlockIndex % gridSize.y;
  absoluteBlockIndex /= gridSize.y;
  uint32_t z = absoluteBlockIndex % gridSize.z;

  if(x < minBlock.x || x > maxBlock.x || y < minBlock.y || y > maxBlock.y || z < minBlock.z || z > maxBlock.z)
  {
    return ~0u;
  }

  glm::vec3 inspectionSize = maxBlock - minBlock + glm::uvec3(1, 1, 1);
  x -= minBlock.x;
  y -= minBlock.y;
  z -= minBlock.z;

  return x + inspectionSize.x * (y + inspectionSize.y * z);
}

void ElementInspector::imGuiGrid(uint32_t index, InspectedComputeVariables& computeVar, const uint8_t* contents)
{
  if(computeVar.gridSizeInBlocks.x == 0 || computeVar.gridSizeInBlocks.y == 0 || computeVar.gridSizeInBlocks.z == 0)
  {
    LOGE("Inspector: Invalid grid size for compute variable inspection (%d, %d, %d)\n", computeVar.gridSizeInBlocks.x,
         computeVar.gridSizeInBlocks.y, computeVar.gridSizeInBlocks.z);
    return;
  }

  if(computeVar.blockSize.x == 0 || computeVar.blockSize.y == 0 || computeVar.blockSize.z == 0)
  {
    LOGE("Inspector: Invalid block size for compute variable inspection (%d, %d, %d)\n", computeVar.blockSize.x,
         computeVar.blockSize.y, computeVar.blockSize.z);
    return;
  }


  size_t                entrySizeInBytes = formatSizeInBytes(computeVar.format);
  std::vector<uint32_t> shownBlocks;
  bool                  isFirstShownBlock = true;
  float                 cursorX           = 0.f;
  float                 itemSpacing       = ImGui::GetStyle().ItemSpacing.x;
  float                 regionMax         = ImGui::GetContentRegionMax().x;

  uint32_t gridTotalBlockCount =
      computeVar.gridSizeInBlocks.x * computeVar.gridSizeInBlocks.y * computeVar.gridSizeInBlocks.z;
  uint32_t minBlockIndex =
      computeVar.minBlock.x
      + computeVar.gridSizeInBlocks.x * (computeVar.minBlock.y + computeVar.gridSizeInBlocks.y * computeVar.minBlock.z);
  uint32_t maxBlockIndex =
      computeVar.maxBlock.x
      + computeVar.gridSizeInBlocks.x * (computeVar.maxBlock.y + computeVar.gridSizeInBlocks.y * computeVar.maxBlock.z);
  uint32_t inspectedWarpsPerBlock = computeVar.maxWarpInBlock - computeVar.minWarpInBlock + 1;


  for(uint32_t absoluteBlockIndex = 0; absoluteBlockIndex < gridTotalBlockCount; absoluteBlockIndex++)
  {
    uint32_t inspectedBlockIndex =
        getCapturedBlockIndex(absoluteBlockIndex, computeVar.gridSizeInBlocks, computeVar.minBlock, computeVar.maxBlock);
    bool isBlockShown = false;

    bool isBlockDisabled = (inspectedBlockIndex == ~0u);
    if(isBlockDisabled && !m_settings.showInactiveBlocks)
    {
      continue;
    }
    if(!isFirstShownBlock)
    {
      if(cursorX + SQUARE_BUTTON_SIZE + itemSpacing < regionMax)
        ImGui::SameLine();
      else
        cursorX = 0.f;
    }

    cursorX += itemSpacing + SQUARE_BUTTON_SIZE;
    isFirstShownBlock = false;
    ImGui::BeginDisabled(isBlockDisabled);

    ImGui::BeginGroup();


    bool hasAll = !isBlockDisabled;
    bool hasAny = false;
    if(hasAll)
    {
      for(uint32_t absoluteWarpIndex = computeVar.minWarpInBlock; absoluteWarpIndex <= computeVar.maxWarpInBlock; absoluteWarpIndex++)
      {
        uint32_t index = inspectedBlockIndex * inspectedWarpsPerBlock + absoluteWarpIndex - computeVar.minWarpInBlock;
        hasAll         = hasAll && computeVar.showWarps[index];
        hasAny         = hasAny || computeVar.showWarps[index];
      }
    }
    imguiPushActiveButtonStyle(hasAll);

    if(ImGui::Button(fmt::format("Block {}\n{}##{}", absoluteBlockIndex,
                                 multiDimUvec3ToString(getBlockIndex(absoluteBlockIndex, computeVar)), index)
                         .c_str(),
                     ImVec2(SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE)))
    {
      for(uint32_t absoluteWarpIndex = computeVar.minWarpInBlock; absoluteWarpIndex <= computeVar.maxWarpInBlock; absoluteWarpIndex++)
      {
        uint32_t inspectedWarpIndex = absoluteWarpIndex - computeVar.minWarpInBlock;
        uint32_t index              = inspectedBlockIndex * inspectedWarpsPerBlock + inspectedWarpIndex;
        computeVar.showWarps[index] = !hasAny;
      }
    }
    imguiPopActiveButtonStyle();


    ImGui::TreePush("###");

    for(uint32_t absoluteWarpIndex = 0;
        absoluteWarpIndex < (computeVar.blockSize.x * computeVar.blockSize.y * computeVar.blockSize.z + WARP_SIZE - 1) / WARP_SIZE;
        absoluteWarpIndex++)
    {
      uint32_t inspectedWarpIndex = absoluteWarpIndex - computeVar.minWarpInBlock;
      bool     isDisabled         = isBlockDisabled || absoluteWarpIndex < computeVar.minWarpInBlock
                        || absoluteWarpIndex > computeVar.maxWarpInBlock;
      ImGui::BeginDisabled(isDisabled);

      std::string warpName = fmt::format("Warp {}##{}Block{}", absoluteWarpIndex, index, absoluteBlockIndex);

      bool     isClicked = false;
      uint32_t index     = 0u;
      bool     isShown   = false;
      if(!isDisabled)
      {
        index   = inspectedBlockIndex * inspectedWarpsPerBlock + inspectedWarpIndex;
        isShown = computeVar.showWarps[index];
        if(isShown)
        {
          isBlockShown = true;
          imguiPushActiveButtonStyle(true);
          isClicked = ImGui::Button(warpName.c_str(), ImVec2(48.f, 32.f));
          imguiPopActiveButtonStyle();
        }
      }
      if(!isShown)
      {
        isClicked = isClicked || ImGui::Button(warpName.c_str(), ImVec2(48.f, 32.f));
      }

      if(isClicked)
      {
        computeVar.showWarps[index] = !computeVar.showWarps[index];
      }
      ImGui::EndDisabled();
    }
    ImGui::TreePop();
    ImGui::EndGroup();

    ImGui::EndDisabled();
    if(isBlockShown)
    {
      shownBlocks.push_back(absoluteBlockIndex);
    }
  }
  ImGui::SliderInt("Blocks per row###", &computeVar.blocksPerRow, 1, maxBlockIndex - minBlockIndex + 1);
  if(!shownBlocks.empty())
  {
    bool r = ImGui::BeginTable(fmt::format("Grid {}", index).c_str(), computeVar.blocksPerRow);

    uint32_t         counter = 0;
    ImGuiListClipper clipper;

    clipper.Begin((shownBlocks.size() + computeVar.blocksPerRow - 1) / computeVar.blocksPerRow);

    while(clipper.Step())
    {
      for(uint32_t i = clipper.DisplayStart; i < uint32_t(clipper.DisplayEnd); i++)
      {
        for(uint32_t b = 0; b < computeVar.blocksPerRow; b++)
        {
          if(counter % computeVar.blocksPerRow == 0)
          {
            ImGui::TableNextRow();
          }
          ImGui::TableNextColumn();

          if((i * computeVar.blocksPerRow + b) < shownBlocks.size())
          {
            uint32_t displayBlockIndex = shownBlocks[i * computeVar.blocksPerRow + b];
            imGuiBlock(displayBlockIndex, computeVar,
                       contents + (displayBlockIndex - minBlockIndex) * inspectedWarpsPerBlock * WARP_SIZE * entrySizeInBytes);
            counter++;
          }
        }
      }
    }
    clipper.End();


    ImGui::EndTable();
  }
}


void ElementInspector::imGuiBlock(uint32_t absoluteBlockIndex, InspectedComputeVariables& computeVar, const uint8_t* contents)
{
  ImGui::Text(fmt::format("Block {} {}", absoluteBlockIndex, multiDimUvec3ToString(getBlockIndex(absoluteBlockIndex, computeVar)))
                  .c_str());

  uint32_t warpsPerBlock = (computeVar.blockSize.x * computeVar.blockSize.y * computeVar.blockSize.z) / WARP_SIZE;

  uint32_t warpCount = computeVar.maxWarpInBlock - computeVar.minWarpInBlock + 1;

  uint32_t minBlockIndex =
      computeVar.minBlock.x
      + computeVar.gridSizeInBlocks.x * (computeVar.minBlock.y + computeVar.gridSizeInBlocks.y * computeVar.minBlock.z);

  uint32_t inspectedBlockIndex =
      getCapturedBlockIndex(absoluteBlockIndex, computeVar.gridSizeInBlocks, computeVar.minBlock, computeVar.maxBlock);


  uint32_t inspectedWarpBeginIndex = inspectedBlockIndex * warpCount;
  uint32_t visibleWarpCount        = 0u;
  for(uint32_t i = 0; i < warpCount; i++)
  {
    if(computeVar.showWarps[inspectedWarpBeginIndex + i])
    {
      visibleWarpCount++;
    }
  }
  if(visibleWarpCount == 0u)
  {
    return;
  }

  ImGui::BeginTable(fmt::format("Block {}{}", absoluteBlockIndex, m_childIndex++).c_str(), visibleWarpCount,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable);
  size_t   entrySizeInBytes = formatSizeInBytes(computeVar.format);
  uint32_t warpSizeInBytes  = entrySizeInBytes * WARP_SIZE;

  uint32_t baseGlobalThreadIndex = absoluteBlockIndex * warpsPerBlock * WARP_SIZE;
  ImGui::TableNextRow();
  for(uint32_t absoluteWarpIndexInBlock = computeVar.minWarpInBlock;
      absoluteWarpIndexInBlock <= computeVar.maxWarpInBlock; absoluteWarpIndexInBlock++)
  {
    uint32_t inspectedWarpIndex = absoluteWarpIndexInBlock - computeVar.minWarpInBlock;
    if(computeVar.showWarps[inspectedWarpBeginIndex + inspectedWarpIndex])
    {
      ImGui::TableNextColumn();
      imGuiWarp(absoluteBlockIndex, baseGlobalThreadIndex, absoluteWarpIndexInBlock,
                contents + inspectedWarpIndex * warpSizeInBytes, computeVar);
    }
  }

  ImGui::EndTable();
}


inline uint32_t wangHash(uint32_t seed)
{
  seed = (seed ^ 61) ^ (seed >> 16);
  seed *= 9;
  seed = seed ^ (seed >> 4);
  seed *= 0x27d4eb2d;
  seed = seed ^ (seed >> 15);
  return seed;
}


template <typename T>
inline uint32_t colorFromValue(const T& v)
{
  const size_t          sizeInBytes = sizeof(T);
  const size_t          sizeInU32   = (sizeInBytes + 3) / 4;
  std::vector<uint32_t> u32Value(sizeInU32);

  uint8_t*       u32Bytes   = reinterpret_cast<uint8_t*>(u32Value.data());
  const uint8_t* inputBytes = reinterpret_cast<const uint8_t*>(&v);
  bool           isZero     = true;
  for(size_t i = 0; i < sizeInBytes; i++)
  {
    u32Bytes[i] = inputBytes[i];
    isZero      = isZero && (inputBytes[i] == 0);
  }

  if(isZero)
  {
    ImVec4 res{};
    return ImGui::ColorConvertFloat4ToU32(res);
  }

  uint32_t hashValue = 0;
  for(size_t i = 0; i < sizeInU32; i++)
  {
    hashValue = wangHash(hashValue + u32Value[i]);
  }

  float  value = float(hashValue) / float(~0u);
  ImVec4 res;
  res.x = value * highlightColor.x;
  res.y = value * highlightColor.y;
  res.z = value * highlightColor.z;
  res.w = 1.f;

  return ImGui::ColorConvertFloat4ToU32(res);
}

void ElementInspector::imGuiColumns(const uint8_t* contents, std::vector<ValueFormat>& format)
{

  const uint8_t* current = contents;

  if(format.empty())
  {
    return;
  }


  for(size_t i = 0; i < format.size(); i++)
  {

    std::stringstream sss;
    if(format[i].hexDisplay)
    {
      sss << "0x" << std::uppercase << std::setfill('0') << std::setw(valueFormatSizeInBytes(format[i]) * 2) << std::hex;
    }
    else
    {
      sss << std::fixed << std::setprecision(5);
    }
    if(format[i].flags == ElementInspector::eVisible)
    {
      ImGui::TableNextColumn();
    }
    ImU32 backgroundColor = {};
    switch(format[i].type)
    {
      case eUint8:
        backgroundColor = colorFromValue(*(const uint8_t*)current);
        sss << uint32_t(*(const uint8_t*)current);
        break;
      case eInt8:
        backgroundColor = colorFromValue(*(const int8_t*)current);
        sss << uint32_t(*(const int8_t*)current);
        break;
      case eUint16:
        backgroundColor = colorFromValue(*(const uint16_t*)current);
        sss << *(const uint16_t*)current;
        break;
      case eInt16:
        backgroundColor = colorFromValue(*(const int16_t*)current);
        sss << *(const int16_t*)current;
        break;
      case eFloat16:
        backgroundColor = colorFromValue(*(const uint16_t*)current);
        if(format[i].hexDisplay)
        {
          sss << *(const uint16_t*)current;
        }
        else
        {
          sss << glm::detail::toFloat32(*(const glm::detail::hdata*)current);
        }
        break;
      case eUint32:
        backgroundColor = colorFromValue(*(const uint32_t*)current);
        sss << *(const uint32_t*)current;
        break;
      case eInt32:
        backgroundColor = colorFromValue(*(const int32_t*)current);
        sss << *(const int32_t*)current;
        break;
      case eFloat32:
        backgroundColor = colorFromValue(*(const uint32_t*)current);
        if(format[i].hexDisplay)
        {
          sss << *(const uint32_t*)current;
        }
        else
        {
          sss << *(const float*)current;
        }
        break;
      case eInt64:
        backgroundColor = colorFromValue(*(const int64_t*)current);
        sss << *(const int64_t*)current;
        break;
      case eUint64:
        backgroundColor = colorFromValue(*(const uint32_t*)current);
        sss << *(const uint64_t*)current;
        break;
    }

    if(format[i].flags != VALUE_FLAG_INTERNAL && format[i].flags == ElementInspector::eVisible)
    {
      ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, backgroundColor);
      ImGui::Text(sss.str().c_str());
    }
    current += valueFormatSizeInBytes(format[i]);
  }
}

void ElementInspector::imGuiWarp(uint32_t                   absoluteBlockIndex,
                                 uint32_t                   baseGlobalThreadIndex,
                                 uint32_t                   index,
                                 const uint8_t*             contents,
                                 InspectedComputeVariables& var)
{

  auto& format = var.format;
  ImGui::Text(fmt::format("Warp {}", index).c_str());

  ImGui::BeginTable(fmt::format("Warp {}{}", index, m_childIndex++).c_str(), 2 + format.size(),
                    ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable);
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::Text("Global Index");
  ImGui::TableNextColumn();
  ImGui::Text("Local Index");

  size_t entrySizeInBytes = formatSizeInBytes(format);
  for(size_t i = 0; i < format.size(); i++)
  {

    ImGui::TableNextColumn();
    ImGui::Text(valueFormatToString(format[i]).c_str());
    imguiPushActiveButtonStyle(format[i].hexDisplay);
    if(ImGui::Button(fmt::format("Hex##{}", i).c_str()))
    {
      format[i].hexDisplay = !format[i].hexDisplay;
    }

    imguiPopActiveButtonStyle();
  }

  const uint8_t* current   = contents;
  size_t         entrySize = formatSizeInBytes(format);
  for(uint32_t i = 0; i < WARP_SIZE; i++)
  {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    glm::uvec3 globalInvocationId = getThreadInvocationId(absoluteBlockIndex, index, i, var);

    ImGui::TextDisabled("%d %s", baseGlobalThreadIndex + index * WARP_SIZE + i,
                        multiDimUvec3ToString(globalInvocationId, var.blockSize.y > 1 || var.blockSize.z > 1).c_str());
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%d", i);

    imGuiColumns(current, format);
    current += entrySize;
  }

  ImGui::EndTable();
}


void ElementInspector::onUIMenu()
{
  if(!m_isAttached)
  {
    return;
  }
  if(ImGui::BeginMenu("File"))
  {
    if(ImGui::MenuItem("Exit", "Ctrl+Q"))
      m_app->close();
    ImGui::EndMenu();
  }
  if(ImGui::IsKeyPressed(ImGuiKey_Q) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
    m_app->close();
}

void ElementInspector::init(const InitInfo& info)
{
  if(!m_isAttached)
  {
    return;
  }
  m_alloc = info.allocator;
  m_inspectedImages.resize(info.imageCount);
  m_inspectedBuffers.resize(info.bufferCount);
  m_inspectedComputeVariables.resize(info.computeCount);
  m_inspectedFragmentVariables.resize(info.fragmentCount);
  m_inspectedCustomVariables.resize(info.customCount);
}


void ElementInspector::deinit()
{
  if(!m_isAttached)
  {
    return;
  }
  for(size_t i = 0; i < m_inspectedImages.size(); i++)
  {
    deinitImageInspection(i);
  }

  for(size_t i = 0; i < m_inspectedBuffers.size(); i++)
  {
    deinitBufferInspection(i);
  }

  for(size_t i = 0; i < m_inspectedComputeVariables.size(); i++)
  {
    deinitComputeInspection(i);
  }

  for(size_t i = 0; i < m_inspectedFragmentVariables.size(); i++)
  {
    deinitFragmentInspection(i);
  }

  for(size_t i = 0; i < m_inspectedCustomVariables.size(); i++)
  {
    deinitCustomInspection(i);
  }

  vkDestroySampler(m_app->getDevice(), m_sampler, nullptr);
  m_sampler = VK_NULL_HANDLE;
}

void ElementInspector::initImageInspection(uint32_t index, const ImageInspectionInfo& info)
{
  if(!m_isAttached)
  {
    return;
  }
  checkFormatFlag(info.format);
  InspectedImage& inspectedImage = m_inspectedImages[index];

  inspectedImage.sourceImage = info.sourceImage;
  if(inspectedImage.isAllocated)
  {
    inspectedImage.isAllocated = false;
    m_alloc->destroy(inspectedImage.image);
    m_alloc->destroy(inspectedImage.hostBuffer);
    vkDestroyImageView(m_app->getDevice(), inspectedImage.view, nullptr);
  }

  createInspectedBuffer(inspectedImage, VK_NULL_HANDLE, info.name, info.format,
                        info.createInfo.extent.width * info.createInfo.extent.height, info.comment);
  inspectedImage.image      = m_alloc->createImage(info.createInfo);
  inspectedImage.createInfo = info.createInfo;

  {
    nvvk::ScopeCommandBuffer cmd(m_app->getDevice(), m_app->getQueueGCT().familyIndex);
    nvvk::cmdBarrierImageLayout(cmd, inspectedImage.image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }

  VkImageViewCreateInfo viewCreateInfo = nvvk::makeImageViewCreateInfo(inspectedImage.image.image, info.createInfo);

  vkCreateImageView(m_app->getDevice(), &viewCreateInfo, nullptr, &inspectedImage.view);

  ImGui_ImplVulkan_RemoveTexture(inspectedImage.imguiImage);
  inspectedImage.imguiImage = ImGui_ImplVulkan_AddTexture(m_sampler, inspectedImage.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void ElementInspector::deinitImageInspection(uint32_t index)
{
  if(!m_isAttached)
  {
    return;
  }
  InspectedImage& inspectedImage = m_inspectedImages[index];
  if(!inspectedImage.isAllocated)
  {
    return;
  }
  destroyInspectedBuffer(inspectedImage);
  vkDestroyImageView(m_app->getDevice(), inspectedImage.view, nullptr);
  m_alloc->destroy(inspectedImage.image);

  ImGui_ImplVulkan_RemoveTexture(inspectedImage.imguiImage);
  inspectedImage.isAllocated = false;
}

void ElementInspector::initBufferInspection(uint32_t index, const BufferInspectionInfo& info)
{
  if(!m_isAttached)
  {
    return;
  }
  checkFormatFlag(info.format);
  createInspectedBuffer(m_inspectedBuffers[index], info.sourceBuffer, info.name, info.format, info.entryCount,
                        info.comment, info.minEntry, info.viewMin, info.viewMax);
}

void ElementInspector::deinitBufferInspection(uint32_t index)
{
  if(!m_isAttached)
  {
    return;
  }
  destroyInspectedBuffer(m_inspectedBuffers[index]);
}

void ElementInspector::inspectImage(VkCommandBuffer cmd, uint32_t index, VkImageLayout currentLayout)
{
  if(!m_isAttached)
  {
    return;
  }
  if(m_settings.isPaused)
  {
    return;
  }

  InspectedImage& internalImg = m_inspectedImages[index];
  internalImg.filteredEntries = ~0u;
  assert(internalImg.isAllocated && "Capture of invalid image requested");

  VkImageCopy cpy{};
  cpy.srcSubresource.layerCount = 1;
  cpy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  cpy.dstSubresource.layerCount = 1;
  cpy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

  cpy.extent = internalImg.createInfo.extent;

  nvvk::cmdBarrierImageLayout(cmd, internalImg.sourceImage, currentLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  nvvk::cmdBarrierImageLayout(cmd, internalImg.image.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  vkCmdCopyImage(cmd, internalImg.sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, internalImg.image.image,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cpy);

  nvvk::cmdBarrierImageLayout(cmd, internalImg.sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, currentLayout);
  nvvk::cmdBarrierImageLayout(cmd, internalImg.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  nvvk::cmdBarrierImageLayout(cmd, internalImg.image.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  VkBufferImageCopy bcpy{};
  bcpy.imageExtent                 = internalImg.createInfo.extent;
  bcpy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bcpy.imageSubresource.layerCount = 1;

  vkCmdCopyImageToBuffer(cmd, internalImg.image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         internalImg.hostBuffer.buffer, 1, &bcpy);

  nvvk::cmdBarrierImageLayout(cmd, internalImg.image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  internalImg.isInspected = true;
}

size_t ElementInspector::formatSizeInBytes(const std::vector<ValueFormat>& format)
{
  size_t result = 0;
  for(auto& c : format)
  {
    result += valueFormatSizeInBytes(c);
  }
  return result;
}

uint32_t ElementInspector::valueFormatSizeInBytes(ValueFormat v)
{
  switch(v.type)
  {
    case eUint8:
    case eInt8:
      return 1;
    case eUint16:
    case eInt16:
    case eFloat16:
      return 2;
    case eUint32:
    case eInt32:
    case eFloat32:
      return 4;
    case eInt64:
    case eUint64:
      return 8;
  }
  return 0;
}

std::string formatType(const std::string& str)
{
  std::string res;
  res.resize(str.size() - 1);
  std::transform(str.begin() + 1, str.end(), res.begin(), ::tolower);
  return res;
}

std::string ElementInspector::valueFormatToString(ValueFormat v)
{
  return std::string(v.name + " (" + valueFormatTypeToString(v) + ")");
}


std::string ElementInspector::valueFormatTypeToString(ValueFormat v)
{
#define TYPE_TO_STRING(x_)                                                                                             \
  case x_:                                                                                                             \
    return std::string(formatType(std::string(#x_)))
  switch(v.type)
  {
    TYPE_TO_STRING(eUint8);
    TYPE_TO_STRING(eUint16);
    TYPE_TO_STRING(eUint32);
    TYPE_TO_STRING(eUint64);
    TYPE_TO_STRING(eInt8);
    TYPE_TO_STRING(eInt16);
    TYPE_TO_STRING(eInt32);
    TYPE_TO_STRING(eInt64);
    TYPE_TO_STRING(eFloat16);
    TYPE_TO_STRING(eFloat32);
  }
#undef TYPE_TO_STRING
  return "";
}


std::string ElementInspector::bufferEntryToString(const uint8_t* contents, const std::vector<ValueFormat> format)
{
  const uint8_t*    current = contents;
  std::stringstream sss;
  for(size_t i = 0; i < format.size(); i++)
  {

    if(format[i].hexDisplay)
    {
      sss << "0x" << std::uppercase << std::setfill('0') << std::setw(valueFormatSizeInBytes(format[i]) * 2) << std::hex;
    }
    else
    {
      sss << std::dec;
    }

    switch(format[i].type)
    {
      case eUint8:
        sss << uint32_t(*(const uint8_t*)current);
        break;
      case eInt8:
        sss << uint32_t(*(const int8_t*)current);
        break;
      case eUint16:
        sss << *(const uint16_t*)current;
        break;
      case eInt16:
        sss << *(const int16_t*)current;
        break;
      case eFloat16:
        if(format[i].hexDisplay)
        {
          sss << *(const uint16_t*)current;
        }
        else
        {
          sss << glm::detail::toFloat32(*(const glm::detail::hdata*)current);
        }
        break;
      case eUint32:
        sss << *(const uint32_t*)current;
        break;
      case eInt32:
        sss << *(const int32_t*)current;
        break;
      case eFloat32:
        sss << *(const float*)current;
        break;
      case eInt64:
        sss << *(const int64_t*)current;
        break;
      case eUint64:
        sss << *(const uint64_t*)current;
        break;
    }
    if(i < format.size() - 1)
    {
      sss << ", ";
    }
    current += valueFormatSizeInBytes(format[i]);
  }
  return sss.str();
}

void ElementInspector::inspectBuffer(VkCommandBuffer cmd, uint32_t index)
{
  if(!m_isAttached)
  {
    return;
  }
  if(m_settings.isPaused)
  {
    return;
  }

  InspectedBuffer& internalBuffer = m_inspectedBuffers[index];

  internalBuffer.filteredEntries = ~0u;

  size_t entrySize = formatSizeInBytes(internalBuffer.format);

  VkBufferCopy bcpy{};
  bcpy.size      = entrySize * internalBuffer.entryCount;
  bcpy.srcOffset = entrySize * internalBuffer.offsetInEntries;
  {
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags srcStage{};

    mb.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
  }
  vkCmdCopyBuffer(cmd, internalBuffer.sourceBuffer, internalBuffer.hostBuffer.buffer, 1, &bcpy);

  {
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags srcStage{};

    mb.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
  }
  internalBuffer.isInspected = true;
}


void ElementInspector::initComputeInspection(uint32_t index, const ComputeInspectionInfo& info)
{
  if(!m_isAttached)
  {
    return;
  }
  checkFormatFlag(info.format);
  InspectedComputeVariables& var = m_inspectedComputeVariables[index];

  var.blocksPerRow = info.uiBlocksPerRow;
  assert(info.uiBlocksPerRow > 0);
  var.blockSize        = info.blockSize;
  var.gridSizeInBlocks = info.gridSizeInBlocks;

  var.minBlock       = info.minBlock;
  var.maxBlock.x     = std::min(var.gridSizeInBlocks.x - 1, info.maxBlock.x);
  var.maxBlock.y     = std::min(var.gridSizeInBlocks.y - 1, info.maxBlock.y);
  var.maxBlock.z     = std::min(var.gridSizeInBlocks.z - 1, info.maxBlock.z);
  var.minWarpInBlock = info.minWarp;
  var.maxWarpInBlock =
      std::min(((info.blockSize.x * info.blockSize.y * info.blockSize.z + WARP_SIZE - 1) / WARP_SIZE) - 1, info.maxWarp);


  if(var.deviceBuffer.buffer)
  {
    m_alloc->destroy(var.deviceBuffer);
    m_alloc->destroy(var.hostBuffer);
    m_alloc->destroy(var.metadata);
  }

  var.format = info.format;


  uint32_t u32PerThread = formatSizeInBytes(var.format);
  assert(u32PerThread % 4 == 0 && "Format must be 32-bit aligned");
  u32PerThread /= 4;

  var.u32PerThread = u32PerThread;


  assert(var.maxBlock.x >= var.minBlock.x && var.maxBlock.y >= var.minBlock.y && var.maxBlock.z >= var.minBlock.z);
  assert(var.maxWarpInBlock >= var.minWarpInBlock);

  glm::uvec3 inspectedBlocks = {var.maxBlock.x - var.minBlock.x + 1, var.maxBlock.y - var.minBlock.y + 1,
                                var.maxBlock.z - var.minBlock.z + 1};

  uint32_t blockCount       = inspectedBlocks.x * inspectedBlocks.y * inspectedBlocks.z;
  uint32_t warpCountInBlock = var.maxWarpInBlock - var.minWarpInBlock + 1;

  uint32_t entryCount = blockCount * warpCountInBlock * WARP_SIZE;
  uint32_t bufferSize = entryCount * u32PerThread * sizeof(uint32_t);

  var.showWarps.resize(blockCount * (var.maxWarpInBlock - var.minWarpInBlock + 1), false);

  var.deviceBuffer = m_alloc->createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  createInspectedBuffer(var, var.deviceBuffer.buffer, info.name, info.format, entryCount, info.comment);
  {
    nvvk::ScopeCommandBuffer                              cmd(m_app->getDevice(), m_app->getQueueGCT().familyIndex);
    std::vector<nvvkhl_shaders::InspectorComputeMetadata> metadata(1);
    metadata[0].u32PerThread   = var.u32PerThread;
    metadata[0].minBlock       = var.minBlock;
    metadata[0].maxBlock       = var.maxBlock;
    metadata[0].minWarpInBlock = var.minWarpInBlock;
    metadata[0].maxWarpInBlock = var.maxWarpInBlock;
    var.metadata = m_alloc->createBuffer(cmd, metadata, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  }
}

void ElementInspector::deinitComputeInspection(uint32_t index)
{
  if(!m_isAttached)
  {
    return;
  }
  InspectedComputeVariables& inspectedComputeVariable = m_inspectedComputeVariables[index];
  destroyInspectedBuffer(inspectedComputeVariable);
  m_alloc->destroy(inspectedComputeVariable.deviceBuffer);
  m_alloc->destroy(inspectedComputeVariable.metadata);
}

void ElementInspector::inspectComputeVariables(VkCommandBuffer cmd, uint32_t index)
{
  if(!m_isAttached)
  {
    return;
  }
  if(m_settings.isPaused)
  {
    return;
  }


  InspectedComputeVariables& var = m_inspectedComputeVariables[index];
  var.filteredEntries            = ~0u;
  VkBufferCopy bcpy{};
  glm::uvec3   inspectedBlocks = {var.maxBlock.x - var.minBlock.x + 1, var.maxBlock.y - var.minBlock.y + 1,
                                  var.maxBlock.z - var.minBlock.z + 1};

  uint32_t blockCount = inspectedBlocks.x * inspectedBlocks.y * inspectedBlocks.z;
  bcpy.size = WARP_SIZE * (var.maxWarpInBlock - var.minWarpInBlock + 1) * blockCount * var.u32PerThread * sizeof(uint32_t);

  {
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags srcStage{};

    mb.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
  }

  vkCmdCopyBuffer(cmd, var.deviceBuffer.buffer, var.hostBuffer.buffer, 1, &bcpy);

  {
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags srcStage{};

    mb.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
  }
  var.isInspected = true;
}

VkBuffer ElementInspector::getComputeInspectionBuffer(uint32_t index)
{
  if(!m_isAttached)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedComputeVariables[index].deviceBuffer.buffer;
}

VkBuffer ElementInspector::getComputeMetadataBuffer(uint32_t index)
{
  if(!m_isAttached)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedComputeVariables[index].metadata.buffer;
}

void ElementInspector::initCustomInspection(uint32_t index, const CustomInspectionInfo& info)
{
  if(!m_isAttached)
  {
    return;
  }
  checkFormatFlag(info.format);
  InspectedCustomVariables& var = m_inspectedCustomVariables[index];


  var.extent = info.extent;

  var.minCoord   = info.minCoord;
  var.maxCoord.x = std::min(var.extent.x - 1, info.maxCoord.x);
  var.maxCoord.y = std::min(var.extent.y - 1, info.maxCoord.y);
  var.maxCoord.z = std::min(var.extent.z - 1, info.maxCoord.z);


  if(var.deviceBuffer.buffer)
  {
    m_alloc->destroy(var.deviceBuffer);
    m_alloc->destroy(var.hostBuffer);
    m_alloc->destroy(var.metadata);
  }

  var.format = info.format;


  uint32_t u32PerThread = formatSizeInBytes(var.format);
  assert(u32PerThread % 4 == 0 && "Format must be 32-bit aligned");
  u32PerThread /= 4;

  var.u32PerThread = u32PerThread;


  assert(var.maxCoord.x >= var.minCoord.x && var.maxCoord.y >= var.minCoord.y && var.maxCoord.z >= var.minCoord.z);

  glm::uvec3 inspectedValues = {var.maxCoord.x - var.minCoord.x + 1, var.maxCoord.y - var.minCoord.y + 1,
                                var.maxCoord.z - var.minCoord.z + 1};

  uint32_t entryCount = inspectedValues.x * inspectedValues.y * inspectedValues.z;
  uint32_t bufferSize = entryCount * u32PerThread * sizeof(uint32_t);


  var.deviceBuffer = m_alloc->createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  createInspectedBuffer(var, var.deviceBuffer.buffer, info.name, info.format, entryCount, info.comment);
  {
    nvvk::ScopeCommandBuffer                             cmd(m_app->getDevice(), m_app->getQueueGCT().familyIndex);
    std::vector<nvvkhl_shaders::InspectorCustomMetadata> metadata(1);
    metadata[0].u32PerThread = var.u32PerThread;
    metadata[0].minCoord     = var.minCoord;
    metadata[0].maxCoord     = var.maxCoord;
    metadata[0].extent       = var.extent;
    var.metadata = m_alloc->createBuffer(cmd, metadata, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  }
}

void ElementInspector::deinitCustomInspection(uint32_t index)
{
  if(!m_isAttached)
  {
    return;
  }
  InspectedCustomVariables& inspectedCustomVariable = m_inspectedCustomVariables[index];
  destroyInspectedBuffer(inspectedCustomVariable);
  m_alloc->destroy(inspectedCustomVariable.deviceBuffer);
  m_alloc->destroy(inspectedCustomVariable.metadata);
}

void ElementInspector::inspectCustomVariables(VkCommandBuffer cmd, uint32_t index)
{
  if(!m_isAttached)
  {
    return;
  }
  if(m_settings.isPaused)
  {
    return;
  }


  InspectedCustomVariables& var = m_inspectedCustomVariables[index];
  var.filteredEntries           = ~0u;
  VkBufferCopy bcpy{};
  glm::uvec3   inspectedValues = {var.maxCoord.x - var.minCoord.x + 1, var.maxCoord.y - var.minCoord.y + 1,
                                  var.maxCoord.z - var.minCoord.z + 1};

  uint32_t entryCount = inspectedValues.x * inspectedValues.y * inspectedValues.z;
  bcpy.size           = entryCount * var.u32PerThread * sizeof(uint32_t);

  {
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags srcStage{};

    mb.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
  }

  vkCmdCopyBuffer(cmd, var.deviceBuffer.buffer, var.hostBuffer.buffer, 1, &bcpy);

  {
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags srcStage{};

    mb.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
  }
  var.isInspected = true;
}

VkBuffer ElementInspector::getCustomInspectionBuffer(uint32_t index)
{
  if(!m_isAttached)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedCustomVariables[index].deviceBuffer.buffer;
}

VkBuffer ElementInspector::getCustomMetadataBuffer(uint32_t index)
{
  if(!m_isAttached)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedCustomVariables[index].metadata.buffer;
}


void ElementInspector::initFragmentInspection(uint32_t index, const FragmentInspectionInfo& info)
{
  if(!m_isAttached)
  {
    return;
  }
  checkFormatFlag(info.format);
  InspectedFragmentVariables& var = m_inspectedFragmentVariables[index];

  var.renderSize  = info.renderSize;
  var.minFragment = info.minFragment;
  var.maxFragment = info.maxFragment;

  if(var.deviceBuffer.buffer)
  {
    m_alloc->destroy(var.deviceBuffer);
    m_alloc->destroy(var.hostBuffer);
    m_alloc->destroy(var.metadata);
  }

  var.format.clear();
  for(size_t i = 0; i < info.format.size(); i++)
  {

    var.format.push_back(info.format[i]);
    uint32_t valueSize = valueFormatSizeInBytes(info.format[i]);
    while(valueSize < 4)
    {
      var.format.push_back({eUint8, "Pad", false, VALUE_FLAG_INTERNAL});
      valueSize += sizeof(uint8_t);
    }
    var.format.push_back({eFloat32, "Z", false, VALUE_FLAG_INTERNAL});
  }

  uint32_t u32PerThread = formatSizeInBytes(var.format);
  assert(u32PerThread % 4 == 0 && "Format must be 32-bit aligned");
  u32PerThread /= 4;

  var.u32PerThread = u32PerThread;


  assert(var.maxFragment.x >= var.minFragment.x && var.maxFragment.y >= var.minFragment.y);

  glm::uvec2 inspectedFragments = {var.maxFragment.x - var.minFragment.x + 1, var.maxFragment.y - var.minFragment.y + 1};

  uint32_t fragmentCount = inspectedFragments.x * inspectedFragments.y;

  uint32_t bufferSize = fragmentCount * u32PerThread * sizeof(uint32_t);


  var.deviceBuffer = m_alloc->createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  createInspectedBuffer(var, var.deviceBuffer.buffer, info.name, var.format, fragmentCount, info.comment);
  {
    nvvk::ScopeCommandBuffer                               cmd(m_app->getDevice(), m_app->getQueueGCT().familyIndex);
    std::vector<nvvkhl_shaders::InspectorFragmentMetadata> metadata(1);
    metadata[0].u32PerThread = var.u32PerThread;
    metadata[0].minFragment  = var.minFragment;
    metadata[0].maxFragment  = var.maxFragment;
    metadata[0].renderSize   = var.renderSize;

    var.metadata = m_alloc->createBuffer(cmd, metadata, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  }
}

void ElementInspector::deinitFragmentInspection(uint32_t index)
{
  if(!m_isAttached)
  {
    return;
  }
  InspectedFragmentVariables& inspectedFragmentVariable = m_inspectedFragmentVariables[index];
  destroyInspectedBuffer(inspectedFragmentVariable);
  m_alloc->destroy(inspectedFragmentVariable.deviceBuffer);
  m_alloc->destroy(inspectedFragmentVariable.metadata);
}


void ElementInspector::clearFragmentVariables(VkCommandBuffer cmd, uint32_t index)
{
  if(!m_isAttached)
  {
    return;
  }
  if(m_settings.isPaused)
  {
    return;
  }


  InspectedFragmentVariables& var = m_inspectedFragmentVariables[index];

  vkCmdFillBuffer(cmd, var.deviceBuffer.buffer, 0, VK_WHOLE_SIZE, 0.f);
  {
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags srcStage{};

    mb.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
  }
}

void ElementInspector::inspectFragmentVariables(VkCommandBuffer cmd, uint32_t index)
{
  if(!m_isAttached)
  {
    return;
  }
  if(m_settings.isPaused)
  {
    return;
  }


  InspectedFragmentVariables& var = m_inspectedFragmentVariables[index];
  var.filteredEntries             = ~0u;
  VkBufferCopy bcpy{};

  glm::uvec2 inspectedFragments = var.maxFragment - var.minFragment + glm::uvec2(1, 1);

  uint32_t fragmentCount = inspectedFragments.x * inspectedFragments.y;

  bcpy.size = fragmentCount * var.u32PerThread * sizeof(uint32_t);

  {
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags srcStage{};

    mb.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
  }

  vkCmdCopyBuffer(cmd, var.deviceBuffer.buffer, var.hostBuffer.buffer, 1, &bcpy);

  {
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags srcStage{};

    mb.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
  }
  var.isInspected = true;
}

void ElementInspector::updateMinMaxFragmentInspection(VkCommandBuffer cmd, uint32_t index, const glm::uvec2& minFragment, const glm::uvec2& maxFragment)
{
  if(!m_isAttached)
  {
    return;
  }
  if(m_settings.isPaused)
  {
    return;
  }


  InspectedFragmentVariables& var = m_inspectedFragmentVariables[index];

  if((maxFragment - minFragment) != (var.maxFragment - var.minFragment))
  {
    LOGE("New min to max range must be the same as the previous min to max range\n");
    return;
  }

  var.minFragment = minFragment;
  var.maxFragment = maxFragment;

  nvvkhl_shaders::InspectorFragmentMetadata metadata;
  metadata.u32PerThread = var.u32PerThread;
  metadata.minFragment  = var.minFragment;
  metadata.maxFragment  = var.maxFragment;
  metadata.renderSize   = var.renderSize;

  vkCmdUpdateBuffer(cmd, var.metadata.buffer, 0, sizeof(nvvkhl_shaders::InspectorFragmentMetadata), &metadata);

  VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
  VkPipelineStageFlags srcStage{};

  mb.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

  srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

  vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
}

VkBuffer ElementInspector::getFragmentInspectionBuffer(uint32_t index)
{
  if(!m_isAttached)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedFragmentVariables[index].deviceBuffer.buffer;
}

VkBuffer ElementInspector::getFragmentMetadataBuffer(uint32_t index)
{
  if(!m_isAttached)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedFragmentVariables[index].metadata.buffer;
}

void ElementInspector::Filter::create()
{
  uint32_t formatSize = formatSizeInBytes(format);
  dataMin.resize(formatSize);
  dataMax.resize(formatSize);
  requestedDataMin.resize(formatSize);
  requestedDataMax.resize(formatSize);
  hasFilter.resize(format.size(), false);
}

bool ElementInspector::Filter::imguiFilterColumns()
{
  bool     hasChanged     = false;
  uint8_t* currentDataMin = requestedDataMin.data();
  uint8_t* currentDataMax = requestedDataMax.data();
  for(size_t i = 0; i < format.size(); i++)
  {
    if(format[i].flags == ElementInspector::eHidden || format[i].flags == VALUE_FLAG_INTERNAL)
    {
      continue;
    }
    ImGui::TableNextColumn();
    bool v = hasFilter[i];

    ImGui::BeginDisabled(!hasFilter[i]);
    if(hasFilter[i])
    {
      ImGui::Text("Min");
      ImGui::SameLine();
      if(imguiInputValue(format[i], currentDataMin))
      {
        hasChanged = true;
      }
      ImGui::Text("Max");
      ImGui::SameLine();
      if(imguiInputValue(format[i], currentDataMax))
      {
        hasChanged = true;
      }
    }
    ImGui::EndDisabled();
    currentDataMin += valueFormatSizeInBytes(format[i]);
    currentDataMax += valueFormatSizeInBytes(format[i]);
  }
  return hasChanged;
}

bool ElementInspector::Filter::filterPasses(const uint8_t* data)
{
  const uint8_t* currentData = data;
  const uint8_t* currentMin  = dataMin.data();
  const uint8_t* currentMax  = dataMax.data();
  for(size_t i = 0; i < format.size(); i++)
  {
    if(hasFilter[i])
    {

      switch(format[i].type)
      {
        case eUint8:
          if(!passes<uint8_t>(currentData, currentMin, currentMax))
          {
            return false;
          }
          break;
        case eInt8:
          if(!passes<int8_t>(currentData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eUint16:
          if(!passes<uint16_t>(currentData, currentMin, currentMax))
          {
            return false;
          }
          break;
        case eInt16:
          if(!passes<int16_t>(currentData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eFloat16: {
          float currentDataFloat = glm::detail::toFloat32(*(const glm::detail::hdata*)currentData);
          float currentMinFloat  = glm::detail::toFloat32(*(const glm::detail::hdata*)currentMin);
          float currentMaxFloat  = glm::detail::toFloat32(*(const glm::detail::hdata*)currentMax);
          if(!passes<float>(reinterpret_cast<uint8_t*>(&currentDataFloat), reinterpret_cast<uint8_t*>(&currentMinFloat),
                            reinterpret_cast<uint8_t*>(&currentMaxFloat)))
          {
            return false;
          }
        }
        break;
        case eUint32:
          if(!passes<uint32_t>(currentData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eInt32:
          if(!passes<int32_t>(currentData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eFloat32:
          if(!passes<float>(currentData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eInt64:
          if(!passes<int64_t>(currentData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eUint64:
          if(!passes<uint64_t>(currentData, currentMin, currentMax))
          {
            return false;
          }
          break;
      }
    }
    uint32_t sizeInBytes = valueFormatSizeInBytes(format[i]);
    currentData += sizeInBytes;
    currentMin += sizeInBytes;
    currentMax += sizeInBytes;
  }
  return true;
}
}  // namespace nvvkhl