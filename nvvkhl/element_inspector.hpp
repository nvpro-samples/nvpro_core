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

#include "nvvkhl/application.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "nvvk/images_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"
#include "nvvkhl/alloc_vma.hpp"
#include "nvvkhl/application.hpp"
#include <iosfwd>

#include <sstream>
#include "nvvk/descriptorsets_vk.hpp"
#include "nvh/timesampler.hpp"
#include "nvvk/commands_vk.hpp"

//--------------------------------------------------------------------------------------------------
// This element is used to facilitate GPU debugging by inspection of:
//  - Image contents
//  - Buffer contents
//  - Variables in compute shaders
//  - Variables in fragment shaders
//
// Basic usage:
//
// Create the element as a global variable, and add it to the applications
// std::shared_ptr<ElementInspector> g_inspectorElement = std::make_shared<ElementInspector>();
//
// void main(...)
// {
//   ...
//   app->addElement(g_inspectorElement);
//   ...
//  }
//
// Upon attachment of the main app element, initialize the Inspector and specify the number of
// buffers, images, compute shader variables and fragment shader variables that it will need to
// inspect
// void onAttach(nvvkhl::Application* app) override
// {
//   ...
//
//   g_inspectorElement->init(m_alloc,  imageInspectionCount, bufferInspectionCount, computeInspectionCount, fragmentInspectionCount);
//   ...
//  }
//
// Each inspection needs to be initialized before use:
// Inspect a buffer of size bufferSize, where each entry contains 5 values. The buffer format specifies the data structure of the entries.
// The following format is the equivalent of
//  // struct
//  // {
//  //   uint32_t counterU32;
//  //   float    counterF32;
//  //   int16_t  anI16Value;
//  //   uint16_t myU16;
//  //   int32_t  anI32;
//  // };
// bufferFormat    = std::vector<ElementInspector::ValueFormat>(5);
// bufferFormat[0] = {ElementInspector::eUint32, "counterU32"};
// bufferFormat[1] = {ElementInspector::eFloat32, "counterF32"};
// bufferFormat[2] = {ElementInspector::eInt16, "anI16Value"};
// bufferFormat[3] = {ElementInspector::eUint16, "myU16"};
// bufferFormat[4] = {ElementInspector::eInt32, "anI32"};
// ElementInspector::BufferCaptureInfo info{};
// info.entryCount   = bufferSize;
// info.format       = bufferFormat;
// info.name         = "myBuffer";
// info.sourceBuffer = m_buffer.buffer;
// g_inspectorElement->initBufferInspection(0, info);
//
// When the inspection is desired, simply add it to the current command buffer. Required barriers are added internally.
// g_inspectorElement->inspectBuffer(0, cmd);
//
// Inspection of the image stored in m_texture, with format RGBA32F. Other formats can be specified using the syntax above
// ElementInspector::ImageInspectionInfo info{};
// info.createInfo  = create_info;
// info.format      = s_inspectorElement->formatRGBA32();
// info.name        = "MyImageInspection";
// info.sourceImage = m_texture.image;
// g_inspectorElement->initImageInspection(0, info);
//
// When the inspection is desired, simply add it to the current command buffer. Required barriers are added internally.
// g_inspectorElement->inspectImage(0, cmd, imageCurrentLayout);
//
// Inspect a compute shader variable for a given 3D grid and block size (use 1 for unused dimensions). This mode applies to shaders where invocation IDs (e.g. gl_LocalInvocationID) are defined.
// Since grids may contain many threads capturing a variable for all threads
// may incur large memory consumption and performance loss. The blocks to inspect, and the warps within those blocks can be specified using inspectedMin/MaxBlocks and inspectedMin/MaxWarp.
// computeInspectionFormat    = std::vector<ElementInspector::ValueFormat>(...);
// ElementInspector::ComputeInspectionInfo info{};
// info.blockSize        = blockSize;
// info.format           = threadInspectionFormat;
// info.gridSizeInBlocks = gridSize;
// info.minBlock         = inspectedMinBlock;
// info.maxBlock         = inspectedMaxBlock;
// info.minWarp          = inspectedMinWarp;
// info.maxWarp          = inspectedMaxWarp;
// info.name             = "My Compute Inspection";
// g_inspectorElement->initComputeInspection(0, info);
//
// To allow variable inspection two buffers need to be made available to the target shader:
// m_computeShader.updateBufferBinding(eThreadInspection, g_inspectorElement->getComputeInspectionBuffer(0));
// m_computeShader.updateBufferBinding(eThreadMetadata, g_inspectorElement->getComputeMetadataBuffer(0));
//
// The shader code needs to indicate include the Inspector header along with preprocessor variables to set the inspection mode to Compute, and indicate the binding points for the buffers:
// #define INSPECTOR_MODE_COMPUTE
// #define INSPECTOR_SET 0
// #define INSPECTOR_INSPECTION_DATA_BINDING 1
// #define INSPECTOR_METADATA_BINDING 2
// #include "dh_inspector.h"
//
// The inspection of a variable is then done as follows. For alignment purposes the inspection is done with a 32-bit granularity. The shader is responsible for packing the inspected variables
// in 32-bit uint words. Those will be unpacked within the Inspector for display according to the specified format.
// uint32_t myVariable = myFunction(...);
// inspect32BitValue(0, myVariable);
//
// The inspection is triggered on the host side right after the compute shader invocation:
// m_computeShader.dispatchBlocks(cmd, computGridSize, &constants);
//
// g_inspectorElement->inspectComputeVariables(0, cmd);
//
// Inspect a fragment shader variable for a given output image resolution. Since the image may have high resolution capturing a variable for all threads
// may incur large memory consumption and performance loss. The bounding box of the fragments to inspect can be specified using inspectedMin/MaxCoord.
// IMPORTANT: Overlapping geometry may trigger several fragment shader invocations for a given pixel. The inspection will only store the value of the foremost fragment (with the
// lowest gl_FragCoord.z).
// fragmentInspectionFormat    = std::vector<ElementInspector::ValueFormat>(...);
// FragmentInspectionInfo info{};
// info.name = "My Fragment Inspection";
// info.format = fragmentInspectionFormat;
// info.renderSize = imageSize;
// info.minFragment = inspectedMinCoord;
// info.maxFragment = inspectedMaxCoord;
// g_inspectorElement->initFragmentInspection(0, info);
//
// To allow variable inspection two buffers need to be made available to the target pipeline:
// std::vector<VkWriteDescriptorSet> writes;
// const VkDescriptorBufferInfo      inspectorInspection{g_inspectorElement->getFragmentInspectionBuffer(0), 0, VK_WHOLE_SIZE};
// writes.emplace_back(m_dset->makeWrite(0, 1, &inspectorInspection));
// const VkDescriptorBufferInfo inspectorMetadata{g_inspectorElement->getFragmentMetadataBuffer(0), 0, VK_WHOLE_SIZE};
// writes.emplace_back(m_dset->makeWrite(0, 2, &inspectorMetadata));
//
// vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
// //
// The shader code needs to indicate include the Inspector header along with preprocessor variables to set the inspection mode to Fragment, and indicate the binding points for the buffers:
// #define INSPECTOR_MODE_FRAGMENT
// #define INSPECTOR_SET 0
// #define INSPECTOR_INSPECTION_DATA_BINDING 1
// #define INSPECTOR_METADATA_BINDING 2
// #include "dh_inspector.h"
//
// The inspection of a variable is then done as follows. For alignment purposes the inspection is done with a 32-bit granularity. The shader is responsible for packing the inspected variables
// in 32-bit uint words. Those will be unpacked within the Inspector for display according to the specified format.
// uint32_t myVariable = myFunction(...);
// inspect32BitValue(0, myVariable);
//
// The inspection data for a pixel will only be written if a fragment actually covers that pixel. To avoid ghosting where no fragments are rendered it is useful to clear the inspection data
// before rendering:
// g_inspectorElement->clearFragmentVariables(0, cmd);
// vkCmdBeginRendering(...);
//
// The inspection is triggered on the host side right after rendering:
// vkCmdEndRendering(cmd);
// g_inspectorElement->inspectFragmentVariables(0, cmd);
//
class ElementInspector : public nvvkhl::IAppElement
{
public:
  ElementInspector()           = default;
  ~ElementInspector() override = default;

  enum BufferValueType
  {
    eUint8,
    eUint16,
    eUint32,
    eUint64,
    eInt8,
    eInt16,
    eInt32,
    eInt64,
    eFloat16,
    eFloat32,
  };

  enum ValueFormatFlagBits
  {
    eVisible = 0,
    eHidden  = 1,
    eValueFormatFlagCount
  };

  typedef uint32_t ValueFormatFlag;

  struct ValueFormat
  {
    BufferValueType type{eUint32};
    std::string     name;
    bool            hexDisplay{false};
    // One of ValueFormatFlagBits
    ValueFormatFlag flags{eVisible};
  };


  void onAttach(nvvkhl::Application* app) override;

  void onDetach() override;

  void onUIRender() override;


  // Called if showMenu is true
  void onUIMenu() override;

  struct InitInfo
  {
    nvvk::ResourceAllocator* allocator{nullptr};
    uint32_t                 imageCount{0u};
    uint32_t                 bufferCount{0u};
    uint32_t                 computeCount{0u};
    uint32_t                 fragmentCount{0u};
    uint32_t                 customCount{0u};
  };

  void init(const InitInfo& info);

  void deinit();

  struct ImageInspectionInfo
  {
    std::string              name;
    std::string              comment;
    VkImageCreateInfo        createInfo{};
    VkImage                  sourceImage{VK_NULL_HANDLE};
    std::vector<ValueFormat> format;
  };

  void initImageInspection(uint32_t index, const ImageInspectionInfo& info);

  void deinitImageInspection(uint32_t index);

  struct BufferInspectionInfo
  {
    std::string              name;
    std::string              comment;
    VkBuffer                 sourceBuffer{VK_NULL_HANDLE};
    std::vector<ValueFormat> format;
    uint32_t                 entryCount{0u};
    uint32_t                 minEntry{0u};
    uint32_t                 viewMin{0};
    uint32_t                 viewMax{~0u};
  };

  void initBufferInspection(uint32_t index, const BufferInspectionInfo& info);
  void deinitBufferInspection(uint32_t index);


  void inspectImage(VkCommandBuffer cmd, uint32_t index, VkImageLayout currentLayout);


  static size_t formatSizeInBytes(const std::vector<ValueFormat>& format);
  void          inspectBuffer(VkCommandBuffer cmd, uint32_t index);


  static inline void appendStructToFormat(std::vector<ValueFormat>&       format,
                                          const std::vector<ValueFormat>& addedStruct,
                                          const std::string               addedStructName,
                                          bool                            forceHidden = false)
  {
    size_t originalSize = format.size();
    format.insert(format.end(), addedStruct.begin(), addedStruct.end());
    for(size_t i = originalSize; i < format.size(); i++)
    {
      format[i].name = fmt::format("{}.{}", addedStructName, format[i].name);
      if(forceHidden)
      {
        format[i].flags = ElementInspector::eHidden;
      }
    }
  }

  struct ComputeInspectionInfo
  {
    std::string              name;
    std::string              comment;
    std::vector<ValueFormat> format;
    glm::uvec3               gridSizeInBlocks{0, 0, 0};
    glm::uvec3               blockSize{0, 0, 0};
    glm::uvec3               minBlock{0, 0, 0};
    glm::uvec3               maxBlock{~0u, ~0u, ~0u};
    uint32_t                 minWarp{0u};
    uint32_t                 maxWarp{~0u};
    int32_t                  uiBlocksPerRow{1};
  };

  void initComputeInspection(uint32_t index, const ComputeInspectionInfo& info);

  void deinitComputeInspection(uint32_t index);

  void inspectComputeVariables(VkCommandBuffer cmd, uint32_t index);

  VkBuffer getComputeInspectionBuffer(uint32_t index);
  VkBuffer getComputeMetadataBuffer(uint32_t index);


  struct CustomInspectionInfo
  {
    std::string              name;
    std::string              comment;
    std::vector<ValueFormat> format;
    glm::uvec3               extent{0, 0, 0};
    glm::uvec3               minCoord{0u, 0u, 0u};
    glm::uvec3               maxCoord{~0u, ~0u, ~0u};
  };

  void initCustomInspection(uint32_t index, const CustomInspectionInfo& info);
  void deinitCustomInspection(uint32_t index);
  void inspectCustomVariables(VkCommandBuffer cmd, uint32_t index);

  VkBuffer getCustomInspectionBuffer(uint32_t index);
  VkBuffer getCustomMetadataBuffer(uint32_t index);


  struct FragmentInspectionInfo
  {
    std::string              name;
    std::string              comment;
    std::vector<ValueFormat> format;
    glm::uvec2               renderSize{0, 0};
    ;
    glm::uvec2 minFragment{0, 0};
    glm::uvec2 maxFragment{~0u, ~0u};
  };

  void initFragmentInspection(uint32_t index, const FragmentInspectionInfo& info);

  void deinitFragmentInspection(uint32_t index);

  void clearFragmentVariables(VkCommandBuffer cmd, uint32_t index);
  void inspectFragmentVariables(VkCommandBuffer cmd, uint32_t index);

  VkBuffer getFragmentInspectionBuffer(uint32_t index);
  VkBuffer getFragmentMetadataBuffer(uint32_t index);


  static inline std::vector<ValueFormat> formatRGBA8(const std::string& name = "")
  {
    if(name.empty())
    {
      return {
          {eUint8, "r"},
          {eUint8, "g"},
          {eUint8, "b"},
          {eUint8, "a"},
      };
    }
    else
    {
      return {
          {eUint8, name + ".r"},
          {eUint8, name + ".g"},
          {eUint8, name + ".b"},
          {eUint8, name + ".a"},
      };
    }
  }

  static inline std::vector<ValueFormat> formatRGBA32(const std::string& name = "")
  {
    if(name.empty())
    {
      return {
          {eFloat32, "r"},
          {eFloat32, "g"},
          {eFloat32, "b"},
          {eFloat32, "a"},
      };
    }
    else
    {
      return {
          {eFloat32, name + ".r"},
          {eFloat32, name + ".g"},
          {eFloat32, name + ".b"},
          {eFloat32, name + ".a"},
      };
    }
  }

  static inline std::vector<ValueFormat> formatVector4(BufferValueType type, const std::string& name)
  {
    return {
        {type, name + ".x"},
        {type, name + ".y"},
        {type, name + ".z"},
        {type, name + ".w"},
    };
  }

  static inline std::vector<ValueFormat> formatVector2(BufferValueType type, const std::string& name)
  {
    return {{type, name + ".x"}, {type, name + ".y"}};
  }

  static inline std::vector<ValueFormat> formatValue(BufferValueType type, const std::string& name)
  {
    return {{type, name}};
  }

  static inline std::vector<ValueFormat> formatUint32(const std::string& name = "value")
  {
    return formatValue(eUint32, name);
  }
  static inline std::vector<ValueFormat> formatFloat32(const std::string& name = "value")
  {
    return formatValue(eFloat32, name);
  }


private:
  nvvk::ResourceAllocator* m_alloc;

  struct Settings
  {
    bool  isPaused{false};
    bool  showInactiveBlocks{false};
    float filterTimeoutInSeconds{2.f};
  };

  Settings m_settings;
  bool     m_isFilterTimeout{false};

  struct Filter
  {
    const std::vector<ValueFormat>& format;
    std::vector<uint8_t>            dataMin;
    std::vector<uint8_t>            dataMax;

    std::vector<uint8_t> requestedDataMin;
    std::vector<uint8_t> requestedDataMax;
    bool                 updateRequested{false};

    std::vector<bool> hasFilter;

    Filter(const std::vector<ValueFormat>& f)
        : format(f)
    {
    }

    void create();
    void destroy() {}
    bool imguiFilterColumns();

    bool hasAnyFilter()
    {
      for(bool f : hasFilter)
      {
        if(f)
        {
          return true;
        }
      }
      return false;
    }

    template <typename T>
    bool passes(const uint8_t* data, const uint8_t* minValue, const uint8_t* maxValue)
    {
      auto typedData = reinterpret_cast<const T*>(data);
      auto typedMin  = reinterpret_cast<const T*>(minValue);
      auto typedMax  = reinterpret_cast<const T*>(maxValue);
      bool isPassing = *typedData >= *typedMin && *typedData <= *typedMax;

      return isPassing;
    }

    bool filterPasses(const uint8_t* data);
  };


  template <typename T>
  static bool imguiInputInt(const std::string& name, uint8_t* data)
  {
    T*      current  = reinterpret_cast<T*>(data);
    int32_t i32Value = uint32_t(*current);

    std::stringstream sss;
    sss << "##inputInt" << name << (void*)data;

    ImGui::InputInt(sss.str().c_str(), &i32Value);
    bool changed = (*current != T(i32Value));
    *current     = T(i32Value);
    return changed;
  }


  static bool imguiInputFloat(const std::string& name, uint8_t* data)
  {
    float*            current = reinterpret_cast<float*>(data);
    std::stringstream sss;
    sss << "##inputFloat" << name << (void*)data;

    float backup = *current;
    ImGui::InputFloat(sss.str().c_str(), current);
    bool changed = (backup != *current);
    return changed;
  }

  static bool imguiInputValue(const ValueFormat& format, uint8_t* data)
  {

    switch(format.type)
    {
      case eUint8:
        return imguiInputInt<uint8_t>(format.name, data);
        break;
      case eInt8:
        return imguiInputInt<int8_t>(format.name, data);
        break;
      case eUint16:
        return imguiInputInt<uint16_t>(format.name, data);
        break;
      case eInt16:
        return imguiInputInt<int16_t>(format.name, data);
        break;
      case eFloat16:
        ImGui::TextDisabled("Unsupported");
        return false;
        break;
      case eUint32:
        return imguiInputInt<uint32_t>(format.name, data);
        break;
      case eInt32:
        return imguiInputInt<int32_t>(format.name, data);
        break;
      case eFloat32:
        return imguiInputFloat(format.name, data);
        break;
      case eInt64:
        ImGui::TextDisabled("Unsupported");
        return false;
        break;
      case eUint64:
        ImGui::TextDisabled("Unsupported");
        return false;
        break;
    }
    return false;
  }

  struct InspectedBuffer
  {
    VkBuffer                 sourceBuffer{VK_NULL_HANDLE};
    uint32_t                 selectedRow{~0u};
    nvh::Stopwatch           selectedFlashTimer;
    int32_t                  viewMin{0};
    int32_t                  viewMax{INT_MAX};
    uint32_t                 offsetInEntries{0u};
    bool                     isAllocated{false};
    bool                     isInspected{false};
    std::vector<ValueFormat> format;
    std::string              name;
    std::string              comment;
    Filter                   filter{Filter(format)};
    nvvk::Buffer             hostBuffer;
    uint32_t                 entryCount{0u};
    bool                     show{false};
    uint32_t                 filteredEntries{~0u};
  };

  void createInspectedBuffer(InspectedBuffer&                inspectedBuffer,
                             VkBuffer                        sourceBuffer,
                             const std::string&              name,
                             const std::vector<ValueFormat>& format,
                             uint32_t                        entryCount,
                             const std::string&              comment         = "",
                             uint32_t                        offsetInEntries = 0u,
                             uint32_t                        viewMin         = 0u,
                             uint32_t                        viewMax         = ~0u);

  void destroyInspectedBuffer(InspectedBuffer& inspectedBuffer);


  std::vector<InspectedBuffer> m_inspectedBuffers;
  struct InspectedImage : public InspectedBuffer
  {
    bool              tableView{false};
    VkImageView       view{VK_NULL_HANDLE};
    nvvk::Image       image;
    VkImageCreateInfo createInfo{};
    VkDescriptorSet   imguiImage{VK_NULL_HANDLE};
    VkImage           sourceImage{VK_NULL_HANDLE};
    uint32_t          selectedPixelIndex{};
  };

  std::vector<InspectedImage> m_inspectedImages;


  struct InspectedComputeVariables : public InspectedBuffer
  {
    nvvk::Buffer      deviceBuffer;
    glm::uvec3        gridSizeInBlocks{};
    glm::uvec3        blockSize{};
    uint32_t          u32PerThread{0u};
    nvvk::Buffer      metadata;
    glm::uvec3        minBlock{};
    glm::uvec3        maxBlock{};
    uint32_t          minWarpInBlock{0u};
    uint32_t          maxWarpInBlock{0u};
    int32_t           blocksPerRow{0};
    std::vector<bool> showWarps;
  };
  std::vector<InspectedComputeVariables> m_inspectedComputeVariables;

  struct InspectedCustomVariables : public InspectedBuffer
  {
    nvvk::Buffer deviceBuffer;
    glm::uvec3   extent{};
    uint32_t     u32PerThread{0u};
    nvvk::Buffer metadata;
    glm::uvec3   minCoord{};
    glm::uvec3   maxCoord{};
  };
  std::vector<InspectedCustomVariables> m_inspectedCustomVariables;


  struct InspectedFragmentVariables : public InspectedBuffer
  {
    nvvk::Buffer deviceBuffer;
    glm::uvec2   renderSize{};
    uint32_t     u32PerThread{0};
    nvvk::Buffer metadata;
    glm::uvec2   minFragment{};
    glm::uvec2   maxFragment{};
  };
  std::vector<InspectedFragmentVariables> m_inspectedFragmentVariables;


  nvvkhl::Application* m_app{nullptr};
  VkSampler            m_sampler{VK_NULL_HANDLE};

  uint32_t m_childIndex = 0u;

  void imGuiGrid(uint32_t index, InspectedComputeVariables& computeVar, const uint8_t* contents);
  void imGuiBlock(uint32_t index, InspectedComputeVariables& computeVar, const uint8_t* contents);
  void imGuiColumns(const uint8_t* contents, std::vector<ValueFormat>& format);
  void imGuiWarp(uint32_t absoluteBlockIndex, uint32_t baseGlobalThreadIndex, uint32_t index, const uint8_t* contents, InspectedComputeVariables& var);

  static uint32_t valueFormatSizeInBytes(ValueFormat v);

  std::string valueFormatToString(ValueFormat v);

  std::string valueFormatTypeToString(ValueFormat v);
  std::string bufferEntryToString(const uint8_t* contents, const std::vector<ValueFormat> format);

  void imGuiComputeVariable(uint32_t i);

  void imGuiImage(uint32_t imageIndex, ImVec2& imageSize);

  uint32_t imGuiBufferContents(InspectedBuffer& buf,
                               const uint8_t*   contents,
                               uint32_t         begin,
                               uint32_t         end,
                               uint32_t         entrySizeInBytes,
                               glm::uvec3       extent,
                               uint32_t         previousFilteredOut,
                               uint32_t         scrollToItem       = ~0u,
                               glm::uvec3       coordDisplayOffset = {0, 0, 0});

  void imGuiBuffer(InspectedBuffer& buf,
                   uint32_t         topItem            = ~0u,
                   glm::uvec3       extent             = {1, 1, 1},
                   bool             defaultOpen        = false,
                   glm::uvec3       coordDisplayOffset = {0, 0, 0});

  inline glm::uvec3 getBlockIndex(uint32_t absoluteBlockIndex, ElementInspector::InspectedComputeVariables& v)
  {
    glm::uvec3 res;
    res.x = absoluteBlockIndex % v.gridSizeInBlocks.x;
    res.y = (absoluteBlockIndex / v.gridSizeInBlocks.x) % v.gridSizeInBlocks.y;
    res.z = (absoluteBlockIndex / (v.gridSizeInBlocks.x * v.gridSizeInBlocks.z));
    return res;
  }

  glm::uvec3 getThreadInvocationId(uint32_t                                     absoluteBlockIndex,
                                   uint32_t                                     warpIndex,
                                   uint32_t                                     localInvocationId,
                                   ElementInspector::InspectedComputeVariables& v);

  std::string multiDimUvec3ToString(const glm::uvec3 v, bool forceMultiDim = false);
};