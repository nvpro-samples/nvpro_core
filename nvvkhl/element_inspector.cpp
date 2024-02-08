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
#include "nvpsystem.hpp"
#include "nvh/fileoperations.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"


// Time during which a selected row will flash, in ms
#define SELECTED_FLASH_DURATION 800.0
// Number of times the selected row flashes
#define SELECTED_FLASH_COUNT 3
// Half-size of the area covered by the magnifying glass when hovering an image, in pixels
#define ZOOM_HALF_SIZE 3.0f
// Size of the buttons for images and buffers
#define SQUARE_BUTTON_SIZE 64.f

#define VALUE_FLAG_INTERNAL 0x2

// Number of threads used when filtering buffer contents
#define FILTER_THREAD_COUNT std::thread::hardware_concurrency() / 2


#define MEDIUM_SQUARE_BUTTON_SIZE ImVec2(SQUARE_BUTTON_SIZE / 2, SQUARE_BUTTON_SIZE / 2)
#define LARGE_SQUARE_BUTTON_SIZE ImVec2(SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE)


// Maximum number of entries in a buffer for which filtering will be automatically updated
// Above this threshold the user has to click on the "Apply" button to apply the filter to preserve
// interactivity
#define FILTER_AUTO_UPDATE_THRESHOLD 1024ULL * 1024ULL * 1024ULL * 10ULL

namespace nvvkhl {

template <typename T>
std::string createCopyName(const std::string& originalName, const std::vector<T>& existingOriginals, const std::vector<T>& copies);


class ElementInspectorInternal
{
public:
  enum ValueType
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

  struct ValueFormat
  {
    ValueType   type{eUint32};
    std::string name;
    bool        hexDisplay{false};
    // One of ValueFormatFlagBits
    ElementInspector::ValueFormatFlag flags{ElementInspector::eVisible};
  };


  static inline void addVectorComponent(std::vector<ElementInspectorInternal::ValueFormat>& res,
                                        ElementInspector::ValueType                         minType,
                                        ElementInspector::ValueType                         type,
                                        const ElementInspectorInternal::ValueFormat&        src)

  {
    static const std::string suffixes[] = {".x", ".y", ".z", ".w"};

    uint32_t componentCount = 2 + type - minType;
    for(uint32_t i = 0; i < componentCount; i++)
    {
      ElementInspectorInternal::ValueFormat component = src;
      component.name += suffixes[i];
      res.push_back(component);
    }
  }

  static inline void addMatrixComponent(std::vector<ElementInspectorInternal::ValueFormat>& res,
                                        const ElementInspectorInternal::ValueFormat&        src,
                                        const glm::uvec2&                                   dims)
  {
    static const std::string suffixes[] = {"x", "y", "z", "w"};

    for(uint32_t y = 0; y < dims.y; y++)
    {
      for(uint32_t x = 0; x < dims.x; x++)
      {
        ElementInspectorInternal::ValueFormat component = src;
        component.name += "." + suffixes[y] + suffixes[x];
        res.push_back(component);
      }
    }
  }

#define VEC_TO_INTERNAL(minType_, maxType_, internalType_)                                                             \
  if(f.type <= maxType_)                                                                                               \
  {                                                                                                                    \
    internal.type = internalType_;                                                                                     \
    addVectorComponent(res, minType_, f.type, internal);                                                               \
    continue;                                                                                                          \
  }

#define MAT_TO_INTERNAL_CASE(type_, internalType_, dims_)                                                              \
  case type_: {                                                                                                        \
    internal.type = internalType_;                                                                                     \
    addMatrixComponent(res, internal, dims_);                                                                          \
    break;                                                                                                             \
  }

  static std::vector<ElementInspectorInternal::ValueFormat> toInternalFormat(const std::vector<ElementInspector::ValueFormat>& format)
  {
    std::vector<ElementInspectorInternal::ValueFormat> res;

    for(auto& f : format)
    {
      ElementInspectorInternal::ValueFormat internal;
      internal.flags          = f.flags;
      internal.hexDisplay     = f.hexDisplay;
      internal.name           = f.name;
      uint32_t componentCount = 1;

      if(f.type <= ElementInspector::eFloat32)
      {
        componentCount = 1;
        internal.type  = *reinterpret_cast<const ValueType*>(&f.type);
        res.push_back(internal);
        continue;
      }

      VEC_TO_INTERNAL(ElementInspector::eU8Vec2, ElementInspector::eU8Vec4, eUint8);
      VEC_TO_INTERNAL(ElementInspector::eU16Vec2, ElementInspector::eU16Vec4, eUint16);
      VEC_TO_INTERNAL(ElementInspector::eU32Vec2, ElementInspector::eU32Vec4, eUint32);
      VEC_TO_INTERNAL(ElementInspector::eU64Vec2, ElementInspector::eU64Vec4, eUint64);

      VEC_TO_INTERNAL(ElementInspector::eS8Vec2, ElementInspector::eS8Vec4, eInt8);
      VEC_TO_INTERNAL(ElementInspector::eS16Vec2, ElementInspector::eS16Vec4, eInt16);
      VEC_TO_INTERNAL(ElementInspector::eS32Vec2, ElementInspector::eS32Vec4, eInt32);
      VEC_TO_INTERNAL(ElementInspector::eS64Vec2, ElementInspector::eS64Vec4, eInt64);

      VEC_TO_INTERNAL(ElementInspector::eF16Vec2, ElementInspector::eF16Vec4, eFloat16);
      VEC_TO_INTERNAL(ElementInspector::eF32Vec2, ElementInspector::eF32Vec4, eFloat32);

      switch(f.type)
      {
        MAT_TO_INTERNAL_CASE(ElementInspector::eU8Mat2x2, eUint8, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU8Mat2x3, eUint8, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU8Mat2x4, eUint8, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU16Mat2x2, eUint16, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU16Mat2x3, eUint16, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU16Mat2x4, eUint16, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU32Mat2x2, eUint32, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU32Mat2x3, eUint32, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU32Mat2x4, eUint32, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU64Mat2x2, eUint64, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU64Mat2x3, eUint64, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU64Mat2x4, eUint64, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU8Mat3x2, eUint8, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU8Mat3x3, eUint8, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU8Mat3x4, eUint8, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU16Mat3x2, eUint16, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU16Mat3x3, eUint16, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU16Mat3x4, eUint16, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU32Mat3x2, eUint32, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU32Mat3x3, eUint32, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU32Mat3x4, eUint32, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU64Mat3x2, eUint64, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU64Mat3x3, eUint64, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU64Mat3x4, eUint64, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU8Mat4x2, eUint8, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU8Mat4x3, eUint8, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU8Mat4x4, eUint8, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU16Mat4x2, eUint16, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU16Mat4x3, eUint16, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU16Mat4x4, eUint16, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU32Mat4x2, eUint32, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU32Mat4x3, eUint32, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU32Mat4x4, eUint32, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU64Mat4x2, eUint64, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU64Mat4x3, eUint64, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eU64Mat4x4, eUint64, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS8Mat2x2, eInt8, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS8Mat2x3, eInt8, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS8Mat2x4, eInt8, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS16Mat2x2, eInt16, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS16Mat2x3, eInt16, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS16Mat2x4, eInt16, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS32Mat2x2, eInt32, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS32Mat2x3, eInt32, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS32Mat2x4, eInt32, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS64Mat2x2, eInt64, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS64Mat2x3, eInt64, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS64Mat2x4, eInt64, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS8Mat3x2, eInt8, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS8Mat3x3, eInt8, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS8Mat3x4, eInt8, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS16Mat3x2, eInt16, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS16Mat3x3, eInt16, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS16Mat3x4, eInt16, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS32Mat3x2, eInt32, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS32Mat3x3, eInt32, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS32Mat3x4, eInt32, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS64Mat3x2, eInt64, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS64Mat3x3, eInt64, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS64Mat3x4, eInt64, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS8Mat4x2, eInt8, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS8Mat4x3, eInt8, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS8Mat4x4, eInt8, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS16Mat4x2, eInt16, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS16Mat4x3, eInt16, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS16Mat4x4, eInt16, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS32Mat4x2, eInt32, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS32Mat4x3, eInt32, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS32Mat4x4, eInt32, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS64Mat4x2, eInt64, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS64Mat4x3, eInt64, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eS64Mat4x4, eInt64, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF16Mat2x2, eFloat16, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF16Mat2x3, eFloat16, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF16Mat2x4, eFloat16, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF32Mat2x2, eFloat32, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF32Mat2x3, eFloat32, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF32Mat2x4, eFloat32, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF16Mat3x2, eFloat16, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF16Mat3x3, eFloat16, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF16Mat3x4, eFloat16, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF32Mat3x2, eFloat32, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF32Mat3x3, eFloat32, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF32Mat3x4, eFloat32, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF16Mat4x2, eFloat16, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF16Mat4x3, eFloat16, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF16Mat4x4, eFloat16, glm::uvec2(2, 4));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF32Mat4x2, eFloat32, glm::uvec2(2, 2));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF32Mat4x3, eFloat32, glm::uvec2(2, 3));
        MAT_TO_INTERNAL_CASE(ElementInspector::eF32Mat4x4, eFloat32, glm::uvec2(2, 4));
      }
    }
    return res;
  }


  void init(const ElementInspector::InitInfo& info);
  void deinit();

  void initImageInspection(uint32_t index, const ElementInspector::ImageInspectionInfo& info);
  void deinitImageInspection(uint32_t index, bool isCopy);
  bool updateImageFormat(uint32_t index, const std::vector<nvvkhl::ElementInspectorInternal::ValueFormat>& newFormat);
  void inspectImage(VkCommandBuffer cmd, uint32_t index, VkImageLayout currentLayout);

  void initBufferInspection(uint32_t index, const ElementInspector::BufferInspectionInfo& info);
  void deinitBufferInspection(uint32_t index);

  void inspectBuffer(VkCommandBuffer cmd, uint32_t index);
  bool updateBufferFormat(uint32_t index, const std::vector<nvvkhl::ElementInspectorInternal::ValueFormat>& newFormat);

  void initComputeInspection(uint32_t index, const ElementInspector::ComputeInspectionInfo& info);
  void deinitComputeInspection(uint32_t index);
  void inspectComputeVariables(VkCommandBuffer cmd, uint32_t index);
  bool updateComputeFormat(uint32_t index, const std::vector<nvvkhl::ElementInspectorInternal::ValueFormat>& newFormat);

  VkBuffer getComputeInspectionBuffer(uint32_t index);
  VkBuffer getComputeMetadataBuffer(uint32_t index);


  void initCustomInspection(uint32_t index, const ElementInspector::CustomInspectionInfo& info);
  void deinitCustomInspection(uint32_t index);
  void inspectCustomVariables(VkCommandBuffer cmd, uint32_t index);

  bool     updateCustomFormat(uint32_t index, const std::vector<ElementInspectorInternal::ValueFormat>& newFormat);
  VkBuffer getCustomInspectionBuffer(uint32_t index);
  VkBuffer getCustomMetadataBuffer(uint32_t index);

  void initFragmentInspection(uint32_t index, const ElementInspector::FragmentInspectionInfo& info);

  void deinitFragmentInspection(uint32_t index);

  void clearFragmentVariables(VkCommandBuffer cmd, uint32_t index);
  void inspectFragmentVariables(VkCommandBuffer cmd, uint32_t index);

  void updateMinMaxFragmentInspection(VkCommandBuffer cmd, uint32_t index, const glm::uvec2& minFragment, const glm::uvec2& maxFragment);

  bool     updateFragmentFormat(uint32_t index, const std::vector<ElementInspectorInternal::ValueFormat>& newFormat);
  VkBuffer getFragmentInspectionBuffer(uint32_t index);
  VkBuffer getFragmentMetadataBuffer(uint32_t index);


  nvvk::ResourceAllocator* m_alloc{nullptr};

  struct Settings
  {
    bool  isPauseRequested{false};
    bool  isPaused{false};
    bool  showInactiveBlocks{false};
    float filterTimeoutInSeconds{2.f};
    bool  inspectNextFrame{false};
  };

  Settings m_settings;
  bool     m_isFilterTimeout{false};

  struct InspectedBuffer;

  // Minimalistic bit set encapsulation, preferred to stl containers
  // for perf in debug mode
  struct FastBitSet
  {
    FastBitSet() = default;
    FastBitSet(const FastBitSet& src)
    {
      resize(src.size(), false);
      memcpy(data, src.data, sizeInBytes);
    }

    ~FastBitSet() { delete[] data; }

    FastBitSet& operator=(const FastBitSet& src)
    {
      if(this == &src)
      {
        return *this;
      }
      resize(src.size(), false);
      memcpy(data, src.data, sizeInBytes);
      return *this;
    }

    void resize(size_t newSize, bool v)
    {
      if(data)
      {
        free(data);
      }
      sizeInBytes = (static_cast<uint32_t>(newSize) + 7) / 8;
      data        = (uint8_t*)malloc(sizeInBytes);
      if(data != nullptr)
      {
        memset(data, v ? 0xFF : 0x0, sizeInBytes);
      }
    }

    inline uint32_t size() const { return sizeInBytes * 8; }

    inline bool any() const
    {
      for(uint32_t i = 0; i < sizeInBytes; i++)
      {
        if(data[i] != 0)
        {
          return true;
        }
      }
      return false;
    }

    inline bool operator[](uint32_t i) const
    {
      uint32_t byteIndex   = i / 8;
      uint32_t indexInByte = i % 8;
      return ((data[byteIndex] & (1 << indexInByte)) != 0);
    }

    inline void set(uint32_t i, bool v)
    {
      uint32_t byteIndex   = i / 8;
      uint32_t indexInByte = i % 8;

      uint8_t byteMod = 0x0;
      if(v)
      {
        byteMod = (1 << indexInByte);
      }

      data[byteIndex] = (data[byteIndex] & ~(1 << indexInByte)) | byteMod;
    }

    uint8_t* data{nullptr};
    uint32_t sizeInBytes{0u};
  };

  struct Filter
  {
    std::vector<uint8_t> dataMin;
    std::vector<uint8_t> dataMax;

    std::vector<uint8_t> requestedDataMin;
    std::vector<uint8_t> requestedDataMax;
    bool                 updateRequested{false};

    FastBitSet hasFilter;

    void create(const ElementInspectorInternal::InspectedBuffer& b);
    void destroy() {}
    bool imguiFilterColumns(const ElementInspectorInternal::InspectedBuffer& b);

    inline bool hasAnyFilter() const { return hasFilter.any(); }

    template <typename T>
    bool passes(const uint8_t* mappedData, const uint8_t* snapshotData, const uint8_t* minValue, const uint8_t* maxValue)
    {
      auto typedMappedData   = reinterpret_cast<const T*>(mappedData);
      auto typedSnapshotData = reinterpret_cast<const T*>(snapshotData);
      auto typedMin          = reinterpret_cast<const T*>(minValue);
      auto typedMax          = reinterpret_cast<const T*>(maxValue);

      bool isPassing = true;
      if(mappedData != nullptr)
      {
        isPassing = *typedMappedData >= *typedMin && *typedMappedData <= *typedMax;
      }
      if(isPassing && snapshotData != nullptr)
      {
        isPassing = *typedSnapshotData >= *typedMin && *typedSnapshotData <= *typedMax;
      }

      return isPassing;
    }

    inline bool filterPasses(const InspectedBuffer& buffer, uint32_t offset);
  };

  static inline uint32_t valueFormatSizeInBytes(const ElementInspectorInternal::ValueFormat& v)
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

  static inline uint32_t computeFormatSizeInBytes(const std::vector<ElementInspectorInternal::ValueFormat>& format)
  {

    uint32_t result = 0;
    for(auto& c : format)
    {
      result += valueFormatSizeInBytes(c);
    }
    return result;
  }


  template <typename T>
  static inline bool imguiInputScalar(const std::string& name, uint8_t* data, ImGuiDataType dataType)
  {
    T* current = reinterpret_cast<T*>(data);
    T  backup  = (*current);

    ImGui::InputScalar(fmt::format("##InputScalar{}{}", name, reinterpret_cast<void*>(data)).c_str(), dataType, current);
    bool changed = (*current != backup);
    return changed;
  }

  static inline bool imguiInputValue(const ElementInspectorInternal::ValueFormat& format, uint8_t* data)
  {

    switch(format.type)
    {
      case eUint8:
        return imguiInputScalar<uint8_t>(format.name, data, ImGuiDataType_U8);
        break;
      case eInt8:
        return imguiInputScalar<int8_t>(format.name, data, ImGuiDataType_S8);
        break;
      case eUint16:
        return imguiInputScalar<uint16_t>(format.name, data, ImGuiDataType_U16);
        break;
      case eInt16:
        return imguiInputScalar<int16_t>(format.name, data, ImGuiDataType_S16);
        break;
      case eFloat16: {
        auto  hVal            = reinterpret_cast<glm::detail::hdata*>(data);
        float currentValFloat = glm::detail::toFloat32(*hVal);
        bool hasChanged = imguiInputScalar<float>(format.name, reinterpret_cast<uint8_t*>(&currentValFloat), ImGuiDataType_Float);
        *hVal = glm::detail::toFloat16(currentValFloat);
        return hasChanged;
      }
      case eUint32:
        return imguiInputScalar<uint32_t>(format.name, data, ImGuiDataType_U32);
        break;
      case eInt32:
        return imguiInputScalar<int32_t>(format.name, data, ImGuiDataType_S32);
        break;
      case eFloat32:
        return imguiInputScalar<float>(format.name, data, ImGuiDataType_Float);
        break;
      case eInt64:
        return imguiInputScalar<int64_t>(format.name, data, ImGuiDataType_S64);
        break;
      case eUint64:
        return imguiInputScalar<uint64_t>(format.name, data, ImGuiDataType_U64);
        break;
    }
    return false;
  }

  struct InspectedBuffer
  {
    VkBuffer                                           sourceBuffer{VK_NULL_HANDLE};
    uint32_t                                           selectedRow{~0u};
    nvh::Stopwatch                                     selectedFlashTimer;
    int32_t                                            viewMin{0};
    int32_t                                            viewMax{INT_MAX};
    uint32_t                                           offsetInEntries{0u};
    bool                                               isAllocated{false};
    bool                                               isInspected{false};
    std::vector<ElementInspectorInternal::ValueFormat> format;
    std::string                                        name;
    std::string                                        comment;
    Filter                                             filter;
    nvvk::Buffer                                       hostBuffer;
    uint32_t                                           entryCount{0u};
    bool                                               show{false};
    uint32_t                                           filteredEntries{~0u};
    std::vector<uint8_t>                               snapshotContents;
    bool                                               snapshotRequested{false};
    bool                                               showSnapshot{false};
    uint8_t*                                           mappedContents{nullptr};
    bool                                               showOnlyDiffToSnapshot{false};
    bool                                               showDynamic{true};
    bool                                               isCopy{false};
    bool                                               hasSnapshotContents{false};
    bool                                               saveSnapshotToFileRequested{false};
    std::string                                        snapshotFileName;
    uint32_t                                           formatSizeInBytes{0u};
    bool                                               breakOnFilterPass{false};
  };

  void createInspectedBuffer(InspectedBuffer&                                          inspectedBuffer,
                             VkBuffer                                                  sourceBuffer,
                             const std::string&                                        name,
                             const std::vector<ElementInspectorInternal::ValueFormat>& format,
                             uint32_t                                                  entryCount,
                             const std::string&                                        comment         = "",
                             uint32_t                                                  offsetInEntries = 0u,
                             uint32_t                                                  viewMin         = 0u,
                             uint32_t                                                  viewMax         = ~0u);

  void destroyInspectedBuffer(InspectedBuffer& inspectedBuffer);


  std::vector<InspectedBuffer> m_inspectedBuffers;

  std::vector<InspectedBuffer> m_inspectedBuffersCopies;

  struct InspectedImage : public InspectedBuffer
  {
    bool              tableView{false};
    VkImageCreateInfo createInfo{};
    VkImageView       view{VK_NULL_HANDLE};
    nvvk::Image       image;
    VkDescriptorSet   imguiImage{VK_NULL_HANDLE};

    VkImage                  sourceImage{VK_NULL_HANDLE};
    uint32_t                 selectedPixelIndex{~0u};
    nvvk::ResourceAllocator* allocator;
    VkDevice                 device;
    uint32_t                 graphicsQueueFamilyIndex;
    VkSampler                sampler;

    InspectedImage& operator=(const InspectedImage& rhs)
    {
      InspectedBuffer::operator=(rhs);
      tableView                = rhs.tableView;
      createInfo               = rhs.createInfo;
      allocator                = rhs.allocator;
      device                   = rhs.device;
      graphicsQueueFamilyIndex = rhs.graphicsQueueFamilyIndex;
      sampler                  = rhs.sampler;


      {
        nvvk::ScopeCommandBuffer cmd(device, graphicsQueueFamilyIndex);

        image = allocator->createImage(cmd, rhs.snapshotContents.size(), rhs.snapshotContents.data(), createInfo);
      }

      VkImageViewCreateInfo viewCreateInfo = nvvk::makeImageViewCreateInfo(image.image, createInfo);

      vkCreateImageView(device, &viewCreateInfo, nullptr, &view);

      ImGui_ImplVulkan_RemoveTexture(imguiImage);
      imguiImage = ImGui_ImplVulkan_AddTexture(sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      return *this;
    }
  };

  std::vector<InspectedImage> m_inspectedImages;
  std::vector<InspectedImage> m_inspectedImagesCopies;


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
  std::vector<InspectedComputeVariables> m_inspectedComputeVariablesCopies;

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
  std::vector<InspectedCustomVariables> m_inspectedCustomVariablesCopies;


  struct InspectedFragmentVariables : public InspectedBuffer
  {
    nvvk::Buffer deviceBuffer;
    glm::uvec2   renderSize{};
    uint32_t     u32PerThread{0};
    nvvk::Buffer metadata;
    glm::uvec2   minFragment{};
    glm::uvec2   maxFragment{};
    bool         isCleared{false};
  };
  std::vector<InspectedFragmentVariables> m_inspectedFragmentVariables;
  std::vector<InspectedFragmentVariables> m_inspectedFragmentVariablesCopies;


  VkDevice  m_device{VK_NULL_HANDLE};
  uint32_t  m_graphicsQueueFamilyIndex{~0u};
  VkSampler m_sampler{VK_NULL_HANDLE};

  uint32_t m_childIndex = 0u;

  void imGuiGrid(uint32_t index, InspectedComputeVariables& computeVar);
  void imGuiBlock(uint32_t index, InspectedComputeVariables& computeVar, uint32_t offsetInBytes);
  void imGuiColumns(const InspectedBuffer& buffer, uint32_t offsetInBytes);
  void imGuiWarp(uint32_t absoluteBlockIndex, uint32_t baseGlobalThreadIndex, uint32_t index, uint32_t offsetInBytes, InspectedComputeVariables& var);


  std::string valueFormatToString(const ElementInspectorInternal::ValueFormat& v);

  std::string valueFormatTypeToString(const ElementInspectorInternal::ValueFormat& v);
  void bufferEntryToLdrPixel(const uint8_t* contents, const std::vector<ElementInspectorInternal::ValueFormat> format, uint8_t* dst);
  std::string bufferEntryToString(const uint8_t* contents, const std::vector<ElementInspectorInternal::ValueFormat> format);

  void imGuiComputeVariable(uint32_t i, bool isCopy);

  void imGuiImage(uint32_t imageIndex, ImVec2& imageSize, bool isCopy);


  void imGuiCustomVariable(uint32_t i, bool isCopy)
  {
    auto& var = isCopy ? m_inspectedCustomVariablesCopies[i] : m_inspectedCustomVariables[i];
    imGuiBuffer(var, ~0u,
                {var.maxCoord.x - var.minCoord.x + 1, var.maxCoord.y - var.minCoord.y + 1, var.maxCoord.z - var.minCoord.z + 1},
                true, var.minCoord);
  }

  void imGuiFragmentVariable(uint32_t i, bool isCopy)
  {
    auto& var = isCopy ? m_inspectedFragmentVariablesCopies[i] : m_inspectedFragmentVariables[i];

    imGuiBuffer(var, ~0u, {var.maxFragment.x - var.minFragment.x + 1, var.maxFragment.y - var.minFragment.y + 1, 1},
                true, {var.minFragment, 1});
  }

  uint32_t imGuiBufferContents(InspectedBuffer& buf,
                               uint32_t         begin,
                               uint32_t         end,
                               uint32_t         entrySizeInBytes,
                               glm::uvec3       extent,
                               uint32_t         previousFilteredOut,
                               uint32_t         scrollToItem       = ~0u,
                               glm::uvec3       coordDisplayOffset = {0, 0, 0});


  void imGuiBuffer(InspectedBuffer& buf,
                   uint32_t         topItem               = ~0u,
                   glm::uvec3       extent                = {1, 1, 1},
                   bool             defaultOpen           = false,
                   glm::uvec3       coordDisplayOffset    = {0, 0, 0},
                   bool             showCollapsibleHeader = true);

  void saveBufferSnapshotToFile(InspectedBuffer& buf);

  void saveImageSnapshotToFile(InspectedImage& img);


  void saveCsvHeader(std::ofstream& out, const std::vector<ElementInspectorInternal::ValueFormat>& format);
  void saveCsvEntry(std::ofstream& out, const InspectedBuffer& buf, size_t index);

  inline glm::uvec3 getBlockIndex(uint32_t absoluteBlockIndex, InspectedComputeVariables& v)
  {
    glm::uvec3 res;
    res.x = absoluteBlockIndex % v.gridSizeInBlocks.x;
    res.y = (absoluteBlockIndex / v.gridSizeInBlocks.x) % v.gridSizeInBlocks.y;
    res.z = (absoluteBlockIndex / (v.gridSizeInBlocks.x * v.gridSizeInBlocks.z));
    return res;
  }

  glm::uvec3 getThreadInvocationId(uint32_t absoluteBlockIndex, uint32_t warpIndex, uint32_t localInvocationId, InspectedComputeVariables& v);

  std::string multiDimUvec3ToString(const glm::uvec3 v, bool forceMultiDim = false);

  inline bool checkInitialized()
  {
    if(!m_isInitialized)
    {
      if(!m_unitializedMessageIssued)
      {
        LOGW("ElementInspected was not initialized - subsequent calls to Inspector will be ignored\n");
        m_unitializedMessageIssued = true;
      }
      return false;
    }
    return true;
  }


  bool m_isAttached{false};
  bool m_isInitialized{false};
  bool m_showWindow{true};
  bool m_unitializedMessageIssued{false};

  void requestFullSnapshot()
  {
    for(auto& element : m_inspectedImages)
    {
      element.snapshotRequested = true;
      element.showSnapshot      = true;
    }

    for(auto& element : m_inspectedBuffers)
    {
      element.snapshotRequested = true;
      element.showSnapshot      = true;
    }

    for(auto& element : m_inspectedComputeVariables)
    {
      element.snapshotRequested = true;
      element.showSnapshot      = true;
    }
    for(auto& element : m_inspectedFragmentVariables)
    {
      element.snapshotRequested = true;
      element.showSnapshot      = true;
    }
    for(auto& element : m_inspectedCustomVariables)
    {
      element.snapshotRequested = true;
      element.showSnapshot      = true;
    }
  }
  template <typename T>
  void setSnapshotFileNames(std::vector<T>& elements, const std::string& filename)
  {
    for(auto& element : elements)
    {
      if(element.hasSnapshotContents)
      {
        element.snapshotFileName            = filename + "_" + element.name + ".csv";
        element.saveSnapshotToFileRequested = true;
      }
    }
  }

  void requestFullSnapshotSave()
  {

    auto filename = NVPSystem::windowSaveFileDialog(m_app->getWindowHandle(), "Save CSV", "csv (.csv)|*.csv");


    if(filename.empty())
    {
      return;
    }


    std::string lowerCase = filename;
    std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(), [](unsigned char c) { return std::tolower(c); });


    if(!nvh::endsWith(lowerCase, ".csv"))
    {
      filename += ".csv";
    }

    std::string pathWithoutExtension = filename.substr(0, filename.length() - 4);

    setSnapshotFileNames(m_inspectedImages, pathWithoutExtension);
    setSnapshotFileNames(m_inspectedImagesCopies, pathWithoutExtension);

    setSnapshotFileNames(m_inspectedBuffers, pathWithoutExtension);
    setSnapshotFileNames(m_inspectedBuffersCopies, pathWithoutExtension);
    setSnapshotFileNames(m_inspectedComputeVariables, pathWithoutExtension);
    setSnapshotFileNames(m_inspectedComputeVariablesCopies, pathWithoutExtension);

    setSnapshotFileNames(m_inspectedFragmentVariables, pathWithoutExtension);
    setSnapshotFileNames(m_inspectedFragmentVariablesCopies, pathWithoutExtension);

    setSnapshotFileNames(m_inspectedCustomVariables, pathWithoutExtension);
    setSnapshotFileNames(m_inspectedCustomVariablesCopies, pathWithoutExtension);
  }


  nvvkhl::Application* m_app;

  bool hasSnapshotContent()
  {
    for(auto& element : m_inspectedImages)
    {
      if(element.hasSnapshotContents)
      {
        return true;
      }
    }

    for(auto& element : m_inspectedBuffers)
    {
      if(element.hasSnapshotContents)
      {
        return true;
      }
    }

    for(auto& element : m_inspectedComputeVariables)
    {
      if(element.hasSnapshotContents)
      {
        return true;
      }
    }
    for(auto& element : m_inspectedFragmentVariables)
    {
      if(element.hasSnapshotContents)
      {
        return true;
      }
    }
    for(auto& element : m_inspectedCustomVariables)
    {
      if(element.hasSnapshotContents)
      {
        return true;
      }
    }
    return false;
  }
};


static void memoryBarrier(VkCommandBuffer cmd)
{
  VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
  VkPipelineStageFlags srcStage{};

  mb.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

  srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

  vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
}

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


template <typename T>
bool updateFormat(T& inspection, const std::vector<nvvkhl::ElementInspectorInternal::ValueFormat>& newFormat)
{
  uint32_t originalSize = inspection.formatSizeInBytes;
  uint32_t newSize      = nvvkhl::ElementInspectorInternal::computeFormatSizeInBytes(newFormat);
  if(originalSize != newSize)
  {
    LOGE("Cannot update inspection format with size (%d) different from original (%d)\n", newSize, originalSize);
    assert(false);
    return false;
  }
  inspection.format = newFormat;
  return true;
}

ElementInspector::ElementInspector()
    : m_internals(new ElementInspectorInternal)
{
}

ElementInspector::~ElementInspector()
{
  delete m_internals;
}


void ElementInspector::onAttach(nvvkhl::Application* app)
{
  m_internals->m_isAttached = true;
  m_internals->m_app        = app;
}

void ElementInspector::onDetach()
{

  if(!m_internals->m_isAttached)
  {
    return;
  }

  if(!m_internals->checkInitialized())
  {
    return;
  }

  NVVK_CHECK(vkDeviceWaitIdle(m_internals->m_device));

  deinit();
  m_internals->m_isAttached = false;
}


template <typename T>
void imguiCopy(nvvkhl::Application* app, T& src, const std::vector<T>& existingOriginals, std::vector<T>& copies)
{
  static bool buttonClicked = false;
  static char textStr[1024];


  ImGui::PushFont(ImGuiH::getIconicFont());

  if(ImGui::Button(fmt::format("{}###Snapshot{}", ImGuiH::icon_camera_slr, src.name).c_str()))
  {
    src.showSnapshot      = true;
    src.snapshotRequested = true;
  }
  ImGui::PopFont();


  tooltip("Take a snapshot of the current values");


  if(ImGui::BeginPopup(fmt::format("Copy to...##{}", src.name).c_str(), ImGuiWindowFlags_Popup))
  {
    std::string copyName = createCopyName(src.name, existingOriginals, copies);
    if(buttonClicked)
    {
      buttonClicked = false;
      strcpy(textStr, copyName.c_str());
      ImGui::SetKeyboardFocusHere();
    }
    // Create a name that does not already exist
    ImGui::InputText(fmt::format("##Copy{}", src.name).c_str(), textStr, 1024);

    // Make sure the user-provided name does not exist, adding "- copy" if necessary
    copyName = createCopyName(std::string(textStr), existingOriginals, copies);


    bool isEnterPressed = ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);

    T copyBuffer = {};
    if(isEnterPressed || (ImGui::Button(fmt::format("OK##Copy{}", src.name).c_str())))
    {
      copyBuffer                        = src;
      copyBuffer.showDynamic            = false;
      copyBuffer.isCopy                 = true;
      copyBuffer.showSnapshot           = true;
      copyBuffer.showOnlyDiffToSnapshot = false;
      copyBuffer.name                   = copyName;
      copyBuffer.hostBuffer             = {};

      copies.push_back(copyBuffer);
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if(ImGui::Button(fmt::format("Cancel##Copy{}", src.name).c_str()))
    {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }


  ImGui::SameLine();
  ImGui::PushFont(ImGuiH::getIconicFont());

  ImGui::BeginDisabled(!src.hasSnapshotContents);

  if(ImGui::Button(fmt::format("{}###{}", ImGuiH::icon_clipboard, src.name).c_str()))
  {
    buttonClicked = true;
    ImGui::OpenPopup(fmt::format("Copy to...##{}", src.name).c_str());
  }
  ImGui::PopFont();
  tooltip("Copy the snapshot values into another element");


  ImGui::SameLine();
  ImGui::PushFont(ImGuiH::getIconicFont());

  if(ImGui::Button(fmt::format("{}###SnapshotSaveToFile{}", ImGuiH::icon_file, src.name).c_str()))
  {

    if(src.snapshotFileName.empty())
    {
      src.snapshotFileName = NVPSystem::windowSaveFileDialog(app->getWindowHandle(), "Save CSV", "CSV(.csv)|*.csv");
    }
    if(!src.snapshotFileName.empty())
    {

      std::string lowerCase = src.snapshotFileName;
      std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(),
                     [](unsigned char c) { return std::tolower(c); });


      if(!nvh::endsWith(lowerCase, ".csv"))
      {
        src.snapshotFileName += ".csv";
      }

      src.saveSnapshotToFileRequested = true;
    }
  }
  ImGui::PopFont();
  tooltip("Save the snapshot values to file");

  ImGui::EndDisabled();
}


static inline bool imguiButton(const std::string& text,
                               const std::string& tooltipText,
                               bool               useActiveButtonStyle,
                               bool               isActive,
                               bool               useIconic,
                               bool               enlargeFont,
                               ImVec2             buttonSize = ImVec2(0, 0))
{
  if(useActiveButtonStyle)
  {
    imguiPushActiveButtonStyle(isActive);
  }
  if(enlargeFont)
  {
    ImGuiH::getIconicFont()->Scale *= 2;
  }
  if(useIconic)
  {
    ImGui::PushFont(ImGuiH::getIconicFont());
  }
  bool res = ImGui::Button(text.c_str(), buttonSize);

  if(useActiveButtonStyle)
  {
    imguiPopActiveButtonStyle();
  }

  if(enlargeFont)
  {
    ImGuiH::getIconicFont()->Scale /= 2;
  }

  if(useIconic)
  {
    ImGui::PopFont();
  }
  if(!tooltipText.empty())
  {
    tooltip(tooltipText);
  }
  return res;
}


template <typename T>
void imguiElementShowButtons(std::vector<T>& elements, std::vector<T>& elementsCopies)
{
  for(size_t elementIndex = 0; elementIndex < elements.size(); elementIndex++)
  {
    if(elementIndex > 0)
    {
      ImGui::SameLine();
    }

    ImGui::BeginDisabled(!elements[elementIndex].isInspected);

    if(imguiButton(elements[elementIndex].isAllocated ? elements[elementIndex].name.c_str() : "",
                   elements[elementIndex].name, true, elements[elementIndex].show, false, false, LARGE_SQUARE_BUTTON_SIZE))
    {
      elements[elementIndex].show = !elements[elementIndex].show;
    }
    ImGui::EndDisabled();
  }
  if(!elementsCopies.empty())
  {
    ImGui::Separator();
    ImGui::TextDisabled("Snapshots");

    for(size_t elementIndex = 0; elementIndex < elementsCopies.size(); elementIndex++)
    {
      if(elementIndex > 0)
      {
        ImGui::SameLine();
      }

      ImGui::BeginDisabled(!elementsCopies[elementIndex].isInspected);

      imguiPushActiveButtonStyle(elementsCopies[elementIndex].show);
      if(ImGui::Button(elementsCopies[elementIndex].isAllocated ? elementsCopies[elementIndex].name.c_str() : "",
                       ImVec2(SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE)))
      {
        elementsCopies[elementIndex].show = !elementsCopies[elementIndex].show;
      }
      if(ImGui::IsItemHovered())
      {
        if(ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
        {
          elementsCopies.erase(elementsCopies.begin() + elementIndex);
          imguiPopActiveButtonStyle();
          ImGui::EndDisabled();
          break;
        }
      }
      tooltip(elementsCopies[elementIndex].name);

      imguiPopActiveButtonStyle();
      ImGui::EndDisabled();
    }
  }
}


template <typename T>
void imguiElementShowImageButtons(std::vector<T>& elements, std::vector<T>& elementsCopies)
{
  for(size_t elementIndex = 0; elementIndex < elements.size(); elementIndex++)
  {
    if(elementIndex > 0)
    {
      ImGui::SameLine();
    }

    ImGui::BeginDisabled(!elements[elementIndex].isInspected);
    imguiPushActiveButtonStyle(elements[elementIndex].show);
    if(ImGui::ImageButton(fmt::format("##ImageButton{}", elementIndex).c_str(), elements[elementIndex].imguiImage,
                          ImVec2(SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE)))
    {
      elements[elementIndex].show = !elements[elementIndex].show;
    }
    tooltip(elements[elementIndex].name);
    imguiPopActiveButtonStyle();
    ImGui::EndDisabled();
  }
  if(!elementsCopies.empty())
  {
    ImGui::Separator();
    ImGui::TextDisabled("Snapshots");

    for(size_t elementIndex = 0; elementIndex < elementsCopies.size(); elementIndex++)
    {
      if(elementIndex > 0)
      {
        ImGui::SameLine();
      }

      ImGui::BeginDisabled(!elementsCopies[elementIndex].isInspected);

      imguiPushActiveButtonStyle(elementsCopies[elementIndex].show);
      if(ImGui::ImageButton(fmt::format("##ImageButtonCopy{}", elementIndex).c_str(),
                            elementsCopies[elementIndex].imguiImage, ImVec2(SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE)))
      {
        elementsCopies[elementIndex].show = !elementsCopies[elementIndex].show;
      }
      if(ImGui::IsItemHovered())
      {
        if(ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
        {
          elementsCopies.erase(elementsCopies.begin() + elementIndex);
          imguiPopActiveButtonStyle();
          ImGui::EndDisabled();
          break;
        }
      }
      tooltip(elementsCopies[elementIndex].name);

      imguiPopActiveButtonStyle();
      ImGui::EndDisabled();
    }
  }
}


void ElementInspector::onUIRender()
{
  if(!m_internals->m_isAttached)
  {
    return;
  }
  if(!m_internals->m_showWindow)
  {
    return;
  }
  if(!m_internals->checkInitialized())
  {
    return;
  }

  m_internals->m_childIndex = 1;

  ImGui::Begin("Inspector", &m_internals->m_showWindow);

  {
    if(m_internals->m_settings.inspectNextFrame)
    {
      m_internals->m_settings.isPaused         = true;
      m_internals->m_settings.inspectNextFrame = false;
    }


    if(imguiButton(m_internals->m_settings.isPaused ? ImGuiH::icon_media_pause : ImGuiH::icon_media_play,
                   "Pause inspection, effectively freezing the displayed values", true,
                   m_internals->m_settings.isPaused, true, true, MEDIUM_SQUARE_BUTTON_SIZE))
    {
      m_internals->m_settings.isPaused = !m_internals->m_settings.isPaused;
    }

    ImGui::SameLine();


    ImGui::BeginDisabled(!m_internals->m_settings.isPaused);
    if(imguiButton(ImGuiH::icon_media_step_forward, "Capture the next frame", false, false, true, true, MEDIUM_SQUARE_BUTTON_SIZE))

    {
      m_internals->m_settings.inspectNextFrame = true;
      m_internals->m_settings.isPaused         = false;
    }
    ImGui::EndDisabled();


    ImGui::SameLine();
    if(imguiButton(ImGuiH::icon_camera_slr, "Take a snapshot of all inspection elements", false, false, true, true, MEDIUM_SQUARE_BUTTON_SIZE))
    {
      m_internals->requestFullSnapshot();
    }


    ImGui::SameLine();
    ImGui::BeginDisabled(!m_internals->hasSnapshotContent());
    if(imguiButton(ImGuiH::icon_file, "Save all snapshots to files", false, false, true, true, MEDIUM_SQUARE_BUTTON_SIZE))
    {
      m_internals->requestFullSnapshotSave();
    }

    ImGui::EndDisabled();

    if(!m_internals->m_inspectedComputeVariables.empty())
    {
      ImGui::SameLine();
      if(imguiButton("Show inactive\n blocks",
                     "If enabled, blocks for which no inspection is enabled will be shown with inactive buttons", true,
                     m_internals->m_settings.showInactiveBlocks, false, false, {SQUARE_BUTTON_SIZE * 2, SQUARE_BUTTON_SIZE / 2}))
      {
        m_internals->m_settings.showInactiveBlocks = !m_internals->m_settings.showInactiveBlocks;
      }
    }
    ImGui::SameLine();

    if(m_internals->m_isFilterTimeout)
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
    ImGui::InputFloat("##FilterTimeout", &m_internals->m_settings.filterTimeoutInSeconds, 0.f, 0.f, "%.1f");
    ImGui::PopItemWidth();

    if(m_internals->m_isFilterTimeout)
    {
      ImGui::PopStyleColor();
    }
  }

  if(m_internals->m_settings.isPaused)
  {
    ImGui::PushStyleColor(ImGuiCol_Border, highlightColor);
  }

  ImGui::BeginDisabled(m_internals->m_inspectedImages.empty());
  if(ImGui::CollapsingHeader("Images", ImGuiTreeNodeFlags_DefaultOpen))
  {

    ImGui::TreePush("###");

    imguiElementShowImageButtons(m_internals->m_inspectedImages, m_internals->m_inspectedImagesCopies);

    ImVec2 imageDisplaySize = ImGui::GetContentRegionAvail();
    imageDisplaySize.x /= m_internals->m_inspectedImages.size();

    bool firstImage = true;
    for(size_t imageIndex = 0; imageIndex < m_internals->m_inspectedImages.size(); imageIndex++)
    {

      imguiCopy(m_internals->m_app, m_internals->m_inspectedImages[imageIndex], m_internals->m_inspectedImages,
                m_internals->m_inspectedImagesCopies);
      ImGui::SameLine();

      m_internals->imGuiImage(static_cast<uint32_t>(imageIndex), imageDisplaySize, false);
    }

    if(!m_internals->m_inspectedImagesCopies.empty())
    {
      ImGui::Separator();
      for(size_t imageIndex = 0; imageIndex < m_internals->m_inspectedImagesCopies.size(); imageIndex++)
      {
        if(m_internals->m_inspectedImagesCopies[imageIndex].show)
        {
          m_internals->imGuiImage(static_cast<uint32_t>(imageIndex), imageDisplaySize, true);
        }
      }
    }
    ImGui::TreePop();
  }
  ImGui::EndDisabled();


  ImGui::BeginDisabled(m_internals->m_inspectedBuffers.empty());
  if(ImGui::CollapsingHeader("Buffers", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush("###");

    imguiElementShowButtons(m_internals->m_inspectedBuffers, m_internals->m_inspectedBuffersCopies);
    for(size_t bufferIndex = 0; bufferIndex < m_internals->m_inspectedBuffers.size(); bufferIndex++)
    {
      if(m_internals->m_inspectedBuffers[bufferIndex].show)
      {
        imguiCopy(m_internals->m_app, m_internals->m_inspectedBuffers[bufferIndex], m_internals->m_inspectedBuffers,
                  m_internals->m_inspectedBuffersCopies);
        ImGui::SameLine();
      }

      m_internals->imGuiBuffer(m_internals->m_inspectedBuffers[bufferIndex], ~0u,
                               {m_internals->m_inspectedBuffers[bufferIndex].entryCount, 1, 1}, true);
    }

    if(!m_internals->m_inspectedBuffersCopies.empty())
    {
      ImGui::Separator();
      for(size_t bufferIndex = 0; bufferIndex < m_internals->m_inspectedBuffersCopies.size(); bufferIndex++)
      {
        m_internals->imGuiBuffer(m_internals->m_inspectedBuffersCopies[bufferIndex], ~0u,
                                 {m_internals->m_inspectedBuffersCopies[bufferIndex].entryCount, 1, 1}, true);
      }
    }
    ImGui::TreePop();
  }
  ImGui::EndDisabled();


  ImGui::BeginDisabled(m_internals->m_inspectedComputeVariables.empty());
  if(ImGui::CollapsingHeader("Compute variables", ImGuiTreeNodeFlags_DefaultOpen))
  {
    if(!m_internals->m_inspectedComputeVariables.empty())
    {
      ImGui::TreePush("###");

      imguiElementShowButtons(m_internals->m_inspectedComputeVariables, m_internals->m_inspectedComputeVariablesCopies);

      for(size_t i = 0; i < m_internals->m_inspectedComputeVariables.size(); i++)
      {
        if(m_internals->m_inspectedComputeVariables[i].show)
        {
          imguiCopy(m_internals->m_app, m_internals->m_inspectedComputeVariables[i],
                    m_internals->m_inspectedComputeVariables, m_internals->m_inspectedComputeVariablesCopies);
          ImGui::SameLine();
        }

        m_internals->imGuiComputeVariable(static_cast<uint32_t>(i), false);
      }

      if(!m_internals->m_inspectedComputeVariablesCopies.empty())
      {
        ImGui::Separator();
        for(size_t i = 0; i < m_internals->m_inspectedComputeVariablesCopies.size(); i++)
        {
          m_internals->imGuiComputeVariable(static_cast<uint32_t>(i), true);
        }
      }


      ImGui::TreePop();
    }
  }
  ImGui::EndDisabled();


  ImGui::BeginDisabled(m_internals->m_inspectedFragmentVariables.empty());
  if(ImGui::CollapsingHeader("Fragment variables", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush("###");

    imguiElementShowButtons(m_internals->m_inspectedFragmentVariables, m_internals->m_inspectedFragmentVariablesCopies);

    for(size_t i = 0; i < m_internals->m_inspectedFragmentVariables.size(); i++)
    {
      if(m_internals->m_inspectedFragmentVariables[i].show)
      {
        imguiCopy(m_internals->m_app, m_internals->m_inspectedFragmentVariables[i],
                  m_internals->m_inspectedFragmentVariables, m_internals->m_inspectedFragmentVariablesCopies);
        ImGui::SameLine();
      }

      m_internals->imGuiFragmentVariable(static_cast<uint32_t>(i), false);
    }

    if(!m_internals->m_inspectedFragmentVariablesCopies.empty())
    {
      ImGui::Separator();
      for(size_t i = 0; i < m_internals->m_inspectedFragmentVariablesCopies.size(); i++)
      {
        m_internals->imGuiFragmentVariable(static_cast<uint32_t>(i), true);
      }
    }


    ImGui::TreePop();
  }
  ImGui::EndDisabled();

  ImGui::BeginDisabled(m_internals->m_inspectedCustomVariables.empty());
  if(ImGui::CollapsingHeader("Custom variables", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush("###");
    imguiElementShowButtons(m_internals->m_inspectedCustomVariables, m_internals->m_inspectedCustomVariablesCopies);

    for(size_t i = 0; i < m_internals->m_inspectedCustomVariables.size(); i++)
    {
      if(m_internals->m_inspectedCustomVariables[i].show)
      {
        imguiCopy(m_internals->m_app, m_internals->m_inspectedCustomVariables[i],
                  m_internals->m_inspectedCustomVariables, m_internals->m_inspectedCustomVariablesCopies);
        ImGui::SameLine();
      }

      m_internals->imGuiCustomVariable(static_cast<uint32_t>(i), false);
    }

    if(!m_internals->m_inspectedCustomVariablesCopies.empty())
    {
      ImGui::Separator();
      for(size_t i = 0; i < m_internals->m_inspectedCustomVariablesCopies.size(); i++)
      {
        m_internals->imGuiCustomVariable(static_cast<uint32_t>(i), true);
      }
    }

    ImGui::TreePop();
  }
  ImGui::EndDisabled();


  if(m_internals->m_settings.isPaused)
  {
    ImGui::PopStyleColor();
  }

  if(m_internals->m_settings.isPauseRequested)
  {
    m_internals->m_settings.isPaused         = true;
    m_internals->m_settings.isPauseRequested = false;
  }

  ImGui::End();
}

void ElementInspectorInternal::imGuiComputeVariable(uint32_t i, bool isCopy)
{
  auto& computeVar = isCopy ? (m_inspectedComputeVariablesCopies[i]) : (m_inspectedComputeVariables[i]);

  if(computeVar.saveSnapshotToFileRequested)
  {
    saveBufferSnapshotToFile(computeVar);
  }


  if(computeVar.snapshotRequested)
  {
    computeVar.mappedContents = (uint8_t*)m_alloc->map(computeVar.hostBuffer);
    memcpy(computeVar.snapshotContents.data(), computeVar.mappedContents, computeVar.entryCount * computeVar.formatSizeInBytes);
    computeVar.snapshotRequested   = false;
    computeVar.hasSnapshotContents = true;
    m_alloc->unmap(computeVar.hostBuffer);
  }


  if(!computeVar.show)
  {
    return;
  }
  if(ImGui::CollapsingHeader(computeVar.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
  {

    if(!computeVar.comment.empty())
    {
      ImGui::TextDisabled(computeVar.comment.c_str());
    }
    if(computeVar.isAllocated)
    {

      computeVar.mappedContents = (uint8_t*)m_alloc->map(computeVar.hostBuffer);
      imGuiGrid(i, computeVar);

      m_alloc->unmap(computeVar.hostBuffer);
      computeVar.mappedContents = nullptr;
    }
  }
}


void ElementInspectorInternal::imGuiImage(uint32_t imageIndex, ImVec2& imageSize, bool isCopy)
{
  auto& img = isCopy ? m_inspectedImagesCopies[imageIndex] : m_inspectedImages[imageIndex];
  if(!img.isInspected)
  {
    return;
  }

  if(img.snapshotRequested)
  {
    uint32_t entrySizeInBytes = img.formatSizeInBytes;
    img.mappedContents        = (uint8_t*)m_alloc->map(img.hostBuffer);
    memcpy(img.snapshotContents.data(), img.mappedContents, img.entryCount * entrySizeInBytes);
    img.snapshotRequested   = false;
    img.hasSnapshotContents = true;
    m_alloc->unmap(img.hostBuffer);
  }

  if(img.saveSnapshotToFileRequested)
  {
    saveImageSnapshotToFile(img);
  }

  if(!img.show)
  {
    return;
  }
  float  imageAspect  = float(img.createInfo.extent.width) / float(img.createInfo.extent.height);
  float  regionAspect = imageSize.x / imageSize.y;
  ImVec2 localSize;
  localSize.x = imageSize.x;
  localSize.y = imageSize.y * regionAspect / imageAspect;

  if(ImGui::CollapsingHeader(img.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
  {


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
                    {img.createInfo.extent.width, img.createInfo.extent.height, img.createInfo.extent.depth}, true,
                    {0u, 0u, 0u}, false);
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

            ImVec2 delta = {cursor.x - offset.x, cursor.y - offset.y};
            ImVec2 imageSize =
                ImVec2(static_cast<float>(img.createInfo.extent.width), static_cast<float>(img.createInfo.extent.height));
            const auto center = imageSize * delta / regionSize;
            uint32_t   pixelIndex =
                static_cast<uint32_t>(center.x) + static_cast<uint32_t>(imageSize.x) * static_cast<uint32_t>(center.y);
            img.selectedPixelIndex = pixelIndex;
            img.tableView          = true;
          }

          if(ImGui::BeginTooltip())
          {
            const auto cursor = ImGui::GetIO().MousePos;

            ImVec2 delta = {cursor.x - offset.x, cursor.y - offset.y};
            ImVec2 imageSize =
                ImVec2(static_cast<float>(img.createInfo.extent.width), static_cast<float>(img.createInfo.extent.height));
            auto center = imageSize * delta / regionSize;

            center.x              = floor(center.x);
            center.y              = floor(center.y);
            ImVec2     zoomRegion = ImVec2(ZOOM_HALF_SIZE, ZOOM_HALF_SIZE);
            const auto uv0        = (center - zoomRegion) / imageSize;
            const auto uv1        = (center + zoomRegion + ImVec2(1, 1)) / imageSize;


            ImVec2 pixelCoord = imageSize * delta;
            ImGui::Text("(%d, %d)", (int32_t)center.x, (int32_t)center.y);

            const uint8_t* contents = static_cast<uint8_t*>(m_alloc->map(img.hostBuffer));
            uint32_t       pixelIndex =
                static_cast<uint32_t>(center.x) + static_cast<uint32_t>(imageSize.x) * static_cast<uint32_t>(center.y);

            contents += pixelIndex * img.formatSizeInBytes;
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
  }
}

uint32_t ElementInspectorInternal::imGuiBufferContents(InspectedBuffer& buf,
                                                       uint32_t         begin,
                                                       uint32_t         end,
                                                       uint32_t         entrySizeInBytes,
                                                       glm::uvec3       extent,
                                                       uint32_t         previousFilteredOut,
                                                       uint32_t         scrollToItem /*= ~0u*/,
                                                       glm::uvec3       coordDisplayOffset) /*= {*/
{
  sanitizeExtent(extent);
  uint32_t filteredOut = previousFilteredOut;
  for(uint32_t i = begin; i < end; i++)
  {
    uint32_t sourceBufferEntryIndex = i + buf.viewMin + filteredOut;
    uint32_t hostBufferEntryIndex   = sourceBufferEntryIndex - buf.offsetInEntries;

    uint32_t entryOffset = entrySizeInBytes * hostBufferEntryIndex;

    while(hostBufferEntryIndex < buf.entryCount && !buf.filter.filterPasses(buf, entryOffset))
    {
      filteredOut++;
      sourceBufferEntryIndex++;
      hostBufferEntryIndex++;
      entryOffset += entrySizeInBytes;
    }

    size_t columnCount = buf.format.size() * ((buf.showDynamic ? 1 : 0) + (buf.showSnapshot ? 1 : 0));
    if(filteredOut == buf.entryCount)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextDisabled("Not found");

      for(size_t i = 0; i < buf.format.size(); i++)
      {
        if((buf.format[i].flags & ElementInspector::eHidden) == 0)
        {
          if(buf.showSnapshot)
          {
            ImGui::TableNextColumn();
            ImGui::TextDisabled("Not found");
          }
          if(buf.showDynamic)
          {
            ImGui::TableNextColumn();
            ImGui::TextDisabled("Not found");
          }
        }
      }
      break;
    }

    if(buf.breakOnFilterPass)
    {
      m_settings.isPauseRequested = true;
    }

    if(i == scrollToItem)
    {
      ImGui::SetScrollHereY();
    }

    if(hostBufferEntryIndex == buf.entryCount || static_cast<int32_t>(sourceBufferEntryIndex) > buf.viewMax)
    {
      ImGui::TableNextRow();
      for(size_t i = 0; i < columnCount /* buf.format.size()*/; i++)
      {
        ImGui::TableNextColumn();
        ImGui::TextDisabled("");
      }
      break;
    }
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      // If a specific scrolling is not requested, display the contents
      if(scrollToItem == ~0u)
      {
        if(buf.selectedRow != ~0u && buf.selectedRow == sourceBufferEntryIndex)
        {
          uint32_t flash =
              uint32_t((buf.selectedFlashTimer.elapsed() / SELECTED_FLASH_DURATION) * (2 * SELECTED_FLASH_COUNT + 1));
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

        imGuiColumns(buf, entryOffset);
      }
      else
      {
        // If a specific scrolling is requested, display placeholder for performance. Actual display of values will be done
        // in the next frame, where the high performance clipper will be used to quickly show the relevant entries
        ImGui::Text("");
      }
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


template <typename T>
std::string createCopyName(const std::string& originalName, const std::vector<T>& existingOriginals, const std::vector<T>& copies)
{
  std::string newName  = originalName;
  bool        finished = false;
  while(!finished)
  {
    finished = true;
    for(auto& b : existingOriginals)
    {
      if(b.name.compare(newName) == 0)
      {
        newName  = newName + " - Copy";
        finished = false;
        break;
      }
    }
    if(finished)
    {
      for(auto& b : copies)
      {
        if(b.name.compare(newName) == 0)
        {
          newName  = newName + " - Copy";
          finished = false;
          break;
        }
      }
    }
  }
  return newName;
}

void ElementInspectorInternal::imGuiBuffer(InspectedBuffer& buf,
                                           uint32_t         topItem /*= ~0u*/,
                                           glm::uvec3       extent,
                                           bool             defaultOpen /*= false*/,
                                           glm::uvec3       coordDisplayOffset /*= {0,0}*/,
                                           bool             showCollapsibleHeader /*= true*/)
{
  if(buf.saveSnapshotToFileRequested)
  {
    saveBufferSnapshotToFile(buf);
  }


  if(buf.snapshotRequested && buf.isAllocated && buf.isInspected)
  {
    uint32_t entrySizeInBytes = buf.formatSizeInBytes;
    buf.mappedContents        = (uint8_t*)m_alloc->map(buf.hostBuffer);
    memcpy(buf.snapshotContents.data(), buf.mappedContents, buf.entryCount * entrySizeInBytes);
    buf.snapshotRequested   = false;
    buf.hasSnapshotContents = true;
    m_alloc->unmap(buf.hostBuffer);
  }


  sanitizeExtent(extent);
  if(buf.isAllocated && buf.isInspected && buf.show
     && (!showCollapsibleHeader || ImGui::CollapsingHeader(buf.name.c_str(), defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0)))
  {

    bool is1d = extent.y == 1 && extent.z == 1;
    if(!buf.comment.empty())
    {
      ImGui::TextDisabled(buf.comment.c_str());
    }

    if(!buf.isCopy)
    {
      ImGui::BeginDisabled(!buf.hasSnapshotContents);
      imguiPushActiveButtonStyle(buf.showSnapshot);
      if(ImGui::Button(fmt::format("Show snapshot values##Buffer{}", buf.name).c_str()))
      {
        buf.showSnapshot = !buf.showSnapshot;
        if(!buf.showSnapshot)
        {
          buf.showOnlyDiffToSnapshot = false;
        }
      }
      imguiPopActiveButtonStyle();
      ImGui::EndDisabled();

      ImGui::SameLine();
      ImGui::BeginDisabled(!buf.showSnapshot);
      imguiPushActiveButtonStyle(buf.showOnlyDiffToSnapshot);
      if(ImGui::Button(fmt::format("Only show differences from snapshot##Buffer{}", buf.name).c_str()))
      {
        buf.showOnlyDiffToSnapshot = !buf.showOnlyDiffToSnapshot;
      }
      imguiPopActiveButtonStyle();
      ImGui::EndDisabled();

      ImGui::SameLine();
      imguiPushActiveButtonStyle(buf.breakOnFilterPass);
      if(ImGui::Button(fmt::format("Pause on filter pass##Buffer{}", buf.name).c_str()))
      {
        buf.breakOnFilterPass = !buf.breakOnFilterPass;
      }
      imguiPopActiveButtonStyle();
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
        ImGui::DragIntRange2(fmt::format("###Range1D{}", buf.name).c_str(), &rangeMin, &rangeMax, 1.f,
                             buf.offsetInEntries + coordDisplayOffset.x,
                             buf.offsetInEntries + coordDisplayOffset.x + buf.entryCount - 1);

        if(rangeMin >= static_cast<int32_t>(coordDisplayOffset.x))
        {
          buf.viewMin = rangeMin - coordDisplayOffset.x;
        }
        if(rangeMax >= static_cast<int32_t>(coordDisplayOffset.x))
        {
          buf.viewMax = rangeMax - coordDisplayOffset.x;
        }
      }

      buf.viewMax = std::max(buf.viewMax, buf.viewMin);

      if(ImGui::BeginPopup("Go to entry", ImGuiWindowFlags_Popup))
      {
        int32_t line = buf.selectedRow;

        if(is1d)
        {
          ImGui::SetKeyboardFocusHere();
          int32_t inputLine = line + coordDisplayOffset.x;
          ImGui::InputInt("###", &inputLine);
          if(inputLine < static_cast<int32_t>(coordDisplayOffset.x))
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
            if(coord[0] < static_cast<int32_t>(coordDisplayOffset.x))
            {
              coord[0] = 0;
            }
            else
            {
              coord[0] -= coordDisplayOffset.x;
            }
            if(coord[1] < static_cast<int32_t>(coordDisplayOffset.y))
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
            if(coord[0] < static_cast<int32_t>(coordDisplayOffset.x))
            {
              coord[0] = 0;
            }
            else
            {
              coord[0] -= coordDisplayOffset.x;
            }

            if(coord[1] < static_cast<int32_t>(coordDisplayOffset.y))
            {
              coord[1] = 0;
            }
            else
            {
              coord[1] -= coordDisplayOffset.y;
            }

            if(coord[2] < static_cast<int32_t>(coordDisplayOffset.z))
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
        if(ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
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
        visibleColumns += (buf.showDynamic ? 1 : 0) + (buf.showSnapshot ? 1 : 0);
      }
    }

    ImGui::BeginChild(fmt::format("tableChild##{}", buf.name).c_str(), ImVec2(0, 0), ImGuiChildFlags_ResizeY);

    bool r = ImGui::BeginTable(fmt::format("{}##BufferTable", buf.name).c_str(), visibleColumns + 1,
                               ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable
                                   | ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingFixedFit,
                               ImVec2(ImGui::GetContentRegionAvail().x, std::max(200.f, ImGui::GetContentRegionAvail().y)));
    if(!r)
    {
      ImGui::EndChild();
      return;
    }

    ImGui::TableSetupScrollFreeze(0, 2);


    uint32_t entrySizeInBytes = buf.formatSizeInBytes;
    ImGui::TableNextColumn();

    ImGui::TextDisabled("Index");
    for(size_t i = 0; i < buf.format.size(); i++)
    {

      if(buf.format[i].flags != VALUE_FLAG_INTERNAL && buf.format[i].flags == ElementInspector::eVisible)
      {

        if(buf.showSnapshot)
        {
          ImGui::TableNextColumn();
          ImGui::Text((std::string("SNAPSHOT\n") + valueFormatToString(buf.format[i])).c_str());
          tooltip(valueFormatTypeToString(buf.format[i]));
          imguiPushActiveButtonStyle(buf.format[i].hexDisplay);
          if(ImGui::Button(fmt::format("Hex##Buffer{}{}", buf.name, i).c_str()))
          {
            buf.format[i].hexDisplay = !buf.format[i].hexDisplay;
          }
          imguiPopActiveButtonStyle();
          tooltip("Display values as hexadecimal");
          ImGui::SameLine();
          imguiPushActiveButtonStyle(buf.filter.hasFilter[static_cast<uint32_t>(i)]);
          if(ImGui::Button(fmt::format("Filter##Buffer{}{}", buf.name, i).c_str()))
          {
            buf.filter.hasFilter.set(static_cast<uint32_t>(i), !buf.filter.hasFilter[static_cast<uint32_t>(i)]);
          }
          tooltip("Filter the values in the column");
          imguiPopActiveButtonStyle();
        }
        if(buf.showDynamic)
        {
          ImGui::TableNextColumn();
          ImGui::Text(valueFormatToString(buf.format[i]).c_str());
          tooltip(valueFormatTypeToString(buf.format[i]));
          imguiPushActiveButtonStyle(buf.format[i].hexDisplay);
          if(ImGui::Button(fmt::format("Hex##Buffer{}{}", buf.name, i).c_str()))
          {
            buf.format[i].hexDisplay = !buf.format[i].hexDisplay;
          }
          imguiPopActiveButtonStyle();
          tooltip("Display values as hexadecimal");
          ImGui::SameLine();
          imguiPushActiveButtonStyle(buf.filter.hasFilter[static_cast<uint32_t>(i)]);
          if(ImGui::Button(fmt::format("Filter##Buffer{}{}", buf.name, i).c_str()))
          {
            buf.filter.hasFilter.set(static_cast<uint32_t>(i), !buf.filter.hasFilter[static_cast<uint32_t>(i)]);
          }
          tooltip("Filter the values in the column");
          imguiPopActiveButtonStyle();
        }
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

    if(!buf.isCopy)
    {
      buf.mappedContents = (uint8_t*)m_alloc->map(buf.hostBuffer);
    }
    uint32_t filteredOut = 0u;
    uint32_t viewed      = 0u;

    std::atomic_uint32_t viewSize = 0;
    if(buf.filteredEntries == ~0u)
    {
      if(buf.filter.hasAnyFilter() || buf.showOnlyDiffToSnapshot)
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


              if(buf.filter.filterPasses(buf, entrySizeInBytes * (static_cast<uint32_t>(i) + buf.viewMin)))
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

    buf.filter.imguiFilterColumns(buf);

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
          uint32_t startPoint = clipper.DisplayStart;
          uint32_t endPoint   = clipper.DisplayEnd;
          filteredOut = imGuiBufferContents(buf, startPoint, endPoint, entrySizeInBytes, extent, filteredOut, ~0u, coordDisplayOffset);
          if(filteredOut + clipper.DisplayStart >= buf.entryCount)
          {
            break;
          }
        }

        clipper.End();
      }
    }
    else
    {

      filteredOut = imGuiBufferContents(buf, 0, buf.entryCount, entrySizeInBytes, extent, filteredOut, topItem, coordDisplayOffset);
      buf.selectedRow = topItem;
      buf.selectedFlashTimer.reset();
    }
    if(!buf.isCopy)
    {
      m_alloc->unmap(buf.hostBuffer);
    }
    buf.mappedContents = nullptr;
    ImGui::EndTable();
    ImGui::EndChild();
  }
}

void ElementInspectorInternal::saveBufferSnapshotToFile(InspectedBuffer& buf)
{
  if(!buf.saveSnapshotToFileRequested)
  {
    return;
  }

  if(!buf.hasSnapshotContents)
  {
    return;
  }

  std::ofstream outStream;
  outStream.open(buf.snapshotFileName);

  saveCsvHeader(outStream, buf.format);
  for(size_t i = 0; i < buf.entryCount; i++)
  {
    saveCsvEntry(outStream, buf, i);
  }
  outStream.close();
  buf.saveSnapshotToFileRequested = false;
  buf.snapshotFileName.clear();
}


void ElementInspectorInternal::saveImageSnapshotToFile(InspectedImage& img)
{
  if(img.snapshotRequested)
  {
    uint32_t entrySizeInBytes = img.formatSizeInBytes;
    img.mappedContents        = (uint8_t*)m_alloc->map(img.hostBuffer);
    memcpy(img.snapshotContents.data(), img.mappedContents, img.entryCount * entrySizeInBytes);
    img.snapshotRequested   = false;
    img.hasSnapshotContents = true;
    m_alloc->unmap(img.hostBuffer);
  }

  if(img.saveSnapshotToFileRequested)
  {
    size_t components = img.format.size();
    if(components <= 4)
    {
      uint32_t             entrySizeInBytes = img.formatSizeInBytes;
      std::vector<uint8_t> ldrData(img.entryCount * components);
      uint8_t*             srcData = img.snapshotContents.data();
      uint8_t*             dstData = ldrData.data();
      for(uint32_t i = 0; i < img.entryCount; i++)
      {
        bufferEntryToLdrPixel(srcData, img.format, dstData);
        srcData += entrySizeInBytes;
        dstData += components;
      }

      stbi_write_png((img.snapshotFileName.substr(0, img.snapshotFileName.length() - 4) + ".png").c_str(),
                     img.createInfo.extent.width, img.createInfo.extent.height, static_cast<uint32_t>(components),
                     ldrData.data(), 0);
    }
    saveBufferSnapshotToFile(img);
  }
}

void ElementInspectorInternal::saveCsvHeader(std::ofstream& out, const std::vector<ElementInspectorInternal::ValueFormat>& format)
{
  for(size_t i = 0; i < format.size(); i++)
  {
    out << valueFormatToString(format[i]);
    if(i < (format.size() - 1))
    {
      out << ", ";
    }
  }
  out << std::endl;
}

void ElementInspectorInternal::saveCsvEntry(std::ofstream& out, const InspectedBuffer& buf, size_t index)
{

  uint32_t entrySizeInBytes = buf.formatSizeInBytes;

  out << bufferEntryToString(buf.snapshotContents.data() + index * entrySizeInBytes, buf.format);
  out << std::endl;
}

glm::uvec3 ElementInspectorInternal::getThreadInvocationId(uint32_t                   absoluteBlockIndex,
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

std::string ElementInspectorInternal::multiDimUvec3ToString(const glm::uvec3 v, bool forceMultiDim /*= false*/)
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

void ElementInspectorInternal::createInspectedBuffer(InspectedBuffer&   inspectedBuffer,
                                                     VkBuffer           sourceBuffer,
                                                     const std::string& name,
                                                     const std::vector<ElementInspectorInternal::ValueFormat>& format,
                                                     uint32_t           entryCount,
                                                     const std::string& comment /*= ""*/,
                                                     uint32_t           offsetInEntries /*= 0u*/,
                                                     uint32_t           viewMin /*= 0u*/,
                                                     uint32_t           viewMax /*= ~0u*/)
{
  inspectedBuffer.formatSizeInBytes = computeFormatSizeInBytes(format);
  uint32_t sizeInBytes              = inspectedBuffer.formatSizeInBytes * entryCount;

  inspectedBuffer.hostBuffer  = m_alloc->createBuffer(sizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  inspectedBuffer.format      = format;
  inspectedBuffer.entryCount  = entryCount;
  inspectedBuffer.isAllocated = true;
  inspectedBuffer.name        = name;
  inspectedBuffer.comment     = comment;
  inspectedBuffer.filter.create(inspectedBuffer);


  inspectedBuffer.viewMin         = viewMin;
  inspectedBuffer.viewMax         = std::min(viewMax, entryCount - 1);
  inspectedBuffer.offsetInEntries = offsetInEntries;
  inspectedBuffer.sourceBuffer    = sourceBuffer;
  inspectedBuffer.filteredEntries = ~0u;

  uint32_t entrySizeInBytes = inspectedBuffer.formatSizeInBytes;
  inspectedBuffer.snapshotContents.resize(inspectedBuffer.entryCount * entrySizeInBytes);
  memset(inspectedBuffer.snapshotContents.data(), 0, inspectedBuffer.entryCount * entrySizeInBytes);
}

void ElementInspectorInternal::destroyInspectedBuffer(InspectedBuffer& inspectedBuffer)
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

  glm::uvec3 inspectionSize = maxBlock - minBlock + glm::uvec3(1, 1, 1);
  x -= minBlock.x;
  y -= minBlock.y;
  z -= minBlock.z;

  return x + inspectionSize.x * (y + inspectionSize.y * z);
}

void ElementInspectorInternal::imGuiGrid(uint32_t index, InspectedComputeVariables& computeVar)
{
  const uint8_t* contents = computeVar.mappedContents;
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

  if(!computeVar.isCopy)
  {
    imguiPushActiveButtonStyle(computeVar.showSnapshot);
    if(ImGui::Button(fmt::format("Show snapshot values##Buffer{}", computeVar.name).c_str()))
    {
      computeVar.showSnapshot = !computeVar.showSnapshot;
      if(!computeVar.showSnapshot)
      {
        computeVar.showOnlyDiffToSnapshot = false;
      }
    }
    imguiPopActiveButtonStyle();
    ImGui::SameLine();
    ImGui::BeginDisabled(!computeVar.showSnapshot);
    imguiPushActiveButtonStyle(computeVar.showOnlyDiffToSnapshot);
    if(ImGui::Button(fmt::format("Only show differences from snapshot##Buffer{}", computeVar.name).c_str()))
    {
      computeVar.showOnlyDiffToSnapshot = !computeVar.showOnlyDiffToSnapshot;
    }

    imguiPopActiveButtonStyle();
    ImGui::EndDisabled();
  }

  size_t                entrySizeInBytes = computeVar.formatSizeInBytes;
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

    clipper.Begin((static_cast<int32_t>(shownBlocks.size()) + computeVar.blocksPerRow - 1) / computeVar.blocksPerRow);

    while(clipper.Step())
    {
      for(uint32_t i = clipper.DisplayStart; i < uint32_t(clipper.DisplayEnd); i++)
      {
        for(int32_t b = 0; b < computeVar.blocksPerRow; b++)
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
                       (displayBlockIndex - minBlockIndex) * inspectedWarpsPerBlock * WARP_SIZE
                           * static_cast<uint32_t>(entrySizeInBytes));
            counter++;
          }
        }
      }
    }
    clipper.End();


    ImGui::EndTable();
  }
}


void ElementInspectorInternal::imGuiBlock(uint32_t absoluteBlockIndex, InspectedComputeVariables& computeVar, uint32_t offsetInBytes)
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
  uint32_t entrySizeInBytes = computeVar.formatSizeInBytes;
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
                offsetInBytes + inspectedWarpIndex * warpSizeInBytes, computeVar);
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

static inline void imguiBufferCell(const uint8_t*                               content,
                                   const ElementInspectorInternal::ValueFormat& format,
                                   std::string&                                 outString,
                                   ImU32&                                       outBackgroundColor)
{
  std::stringstream sss;
  if(format.hexDisplay)
  {
    sss << "0x" << std::uppercase << std::setfill('0')
        << std::setw(ElementInspectorInternal::valueFormatSizeInBytes(format) * 2) << std::hex;
  }
  else
  {
    sss << std::fixed << std::setprecision(5);
  }
  if(format.flags == ElementInspector::eVisible)
  {
    ImGui::TableNextColumn();
  }
  outBackgroundColor = {};
  switch(format.type)
  {
    case ElementInspectorInternal::eUint8:
      outBackgroundColor = colorFromValue(*(const uint8_t*)content);
      sss << uint32_t(*(const uint8_t*)content);
      break;
    case ElementInspectorInternal::eInt8:
      outBackgroundColor = colorFromValue(*(const int8_t*)content);
      sss << uint32_t(*(const int8_t*)content);
      break;
    case ElementInspectorInternal::eUint16:
      outBackgroundColor = colorFromValue(*(const uint16_t*)content);
      sss << *(const uint16_t*)content;
      break;
    case ElementInspectorInternal::eInt16:
      outBackgroundColor = colorFromValue(*(const int16_t*)content);
      sss << *(const int16_t*)content;
      break;
    case ElementInspectorInternal::eFloat16:
      outBackgroundColor = colorFromValue(*(const uint16_t*)content);
      if(format.hexDisplay)
      {
        sss << *(const uint16_t*)content;
      }
      else
      {
        sss << glm::detail::toFloat32(*(const glm::detail::hdata*)content);
      }
      break;
    case ElementInspectorInternal::eUint32:
      outBackgroundColor = colorFromValue(*(const uint32_t*)content);
      sss << *(const uint32_t*)content;
      break;
    case ElementInspectorInternal::eInt32:
      outBackgroundColor = colorFromValue(*(const int32_t*)content);
      sss << *(const int32_t*)content;
      break;
    case ElementInspectorInternal::eFloat32:
      outBackgroundColor = colorFromValue(*(const uint32_t*)content);
      if(format.hexDisplay)
      {
        sss << *(const uint32_t*)content;
      }
      else
      {
        sss << *(const float*)content;
      }
      break;
    case ElementInspectorInternal::eInt64:
      outBackgroundColor = colorFromValue(*(const int64_t*)content);
      sss << *(const int64_t*)content;
      break;
    case ElementInspectorInternal::eUint64:
      outBackgroundColor = colorFromValue(*(const uint32_t*)content);
      sss << *(const uint64_t*)content;
      break;
  }
  outString = sss.str();
}

void ElementInspectorInternal::imGuiColumns(const InspectedBuffer& buffer, uint32_t offsetInBytes)
{

  uint32_t currentOffset = offsetInBytes;
  auto&    format        = buffer.format;

  if(format.empty())
  {
    return;
  }


  for(size_t i = 0; i < format.size(); i++)
  {
    if(format[i].flags != VALUE_FLAG_INTERNAL && format[i].flags == ElementInspector::eVisible)
    {
      if(buffer.showSnapshot)
      {
        std::string cellText;
        ImU32       backgroundColor = {};
        imguiBufferCell(buffer.snapshotContents.data() + currentOffset, format[i], cellText, backgroundColor);

        ImGui::TablePushBackgroundChannel();
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, backgroundColor);
        ImGui::Text(cellText.c_str());
        ImGui::TablePopBackgroundChannel();
      }
      if(buffer.showDynamic)
      {
        std::string cellText;
        ImU32       backgroundColor = {};
        imguiBufferCell(buffer.mappedContents + currentOffset, format[i], cellText, backgroundColor);

        ImGui::TablePushBackgroundChannel();
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, backgroundColor);
        ImGui::Text(cellText.c_str());
        ImGui::TablePopBackgroundChannel();
      }
    }
    currentOffset += valueFormatSizeInBytes(format[i]);
  }
}

void ElementInspectorInternal::imGuiWarp(uint32_t                   absoluteBlockIndex,
                                         uint32_t                   baseGlobalThreadIndex,
                                         uint32_t                   index,
                                         uint32_t                   offsetInBytes,
                                         InspectedComputeVariables& var)
{
  const uint8_t* contents         = var.mappedContents + offsetInBytes;
  const uint8_t* snapshotContents = var.snapshotContents.data() + offsetInBytes;
  auto&          format           = var.format;
  ImGui::Text(fmt::format("Warp {}", index).c_str());

  ImGui::BeginTable(fmt::format("Warp {}{}", index, m_childIndex++).c_str(),
                    2 + (static_cast<int32_t>(format.size()) * ((var.showDynamic ? 1 : 0) + (var.showSnapshot ? 1 : 0))),
                    ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable);
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::Text("Global Index");
  ImGui::TableNextColumn();
  ImGui::Text("Local Index");

  size_t entrySizeInBytes = var.formatSizeInBytes;
  for(size_t i = 0; i < format.size(); i++)
  {
    if(var.showSnapshot)
    {
      ImGui::TableNextColumn();
      ImGui::Text((std::string("SNAPSHOT\n") + valueFormatToString(format[i])).c_str());
      imguiPushActiveButtonStyle(format[i].hexDisplay);
      if(ImGui::Button(fmt::format("Hex##{}", i).c_str()))
      {
        format[i].hexDisplay = !format[i].hexDisplay;
      }

      imguiPopActiveButtonStyle();
    }
    if(var.showDynamic)
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
  }

  uint32_t offset          = 0;
  size_t   entrySize       = var.formatSizeInBytes;
  bool     isAnyEntryShown = false;
  for(uint32_t i = 0; i < WARP_SIZE; i++)
  {
    if(var.showOnlyDiffToSnapshot)
    {
      if(memcmp(snapshotContents + offset, contents + offset, entrySizeInBytes) == 0)
        continue;
    }
    isAnyEntryShown = true;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    glm::uvec3 globalInvocationId = getThreadInvocationId(absoluteBlockIndex, index, i, var);

    ImGui::TextDisabled("%d %s", baseGlobalThreadIndex + index * WARP_SIZE + i,
                        multiDimUvec3ToString(globalInvocationId, var.blockSize.y > 1 || var.blockSize.z > 1).c_str());
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%d", i);

    imGuiColumns(var, offsetInBytes + offset);
    offset += static_cast<uint32_t>(entrySize);
  }

  if(!isAnyEntryShown)
  {
    ImGui::TableNextRow();
    size_t tableWidth = 2 + var.format.size() * ((var.showDynamic ? 1 : 0) + (var.showSnapshot ? 1 : 0));
    for(size_t i = 0; i < tableWidth; i++)
    {
      ImGui::TableNextColumn();
      ImGui::TextDisabled("Not found");
    }
  }

  ImGui::EndTable();
}


void ElementInspector::onUIMenu()
{
  if(!m_internals->m_isAttached || !m_internals->m_isInitialized)
  {
    return;
  }

  if(ImGui::BeginMenu("View"))
  {
    ImGui::MenuItem("Inspector", nullptr, &m_internals->m_showWindow);
    ImGui::EndMenu();
  }
}

void ElementInspector::init(const InitInfo& info)
{
  m_internals->init(info);
}

void ElementInspectorInternal::init(const ElementInspector::InitInfo& info)
{
  m_alloc = info.allocator;
  if(m_alloc == nullptr)
  {
    LOGE("ElementInspector: allocator cannot be nullptr\n");
    assert(false);
    return;
  }
  if(info.device == VK_NULL_HANDLE)
  {
    LOGE("ElementInspector: device cannot be VK_NULL_HANDLE\n");
    assert(false);
    return;
  }
  if(info.graphicsQueueFamilyIndex == ~0u)
  {
    LOGE("ElementInspector: graphicsQueueFamilyIndex cannot be ~0u\n");
    assert(false);
    return;
  }
  m_device                   = info.device;
  m_graphicsQueueFamilyIndex = info.graphicsQueueFamilyIndex;

  m_inspectedImages.resize(info.imageCount);
  m_inspectedBuffers.resize(info.bufferCount);
  m_inspectedComputeVariables.resize(info.computeCount);
  m_inspectedFragmentVariables.resize(info.fragmentCount);
  m_inspectedCustomVariables.resize(info.customCount);
  VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.magFilter    = VK_FILTER_NEAREST;
  samplerInfo.minFilter    = VK_FILTER_NEAREST;
  vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler);

  m_isInitialized = true;
}

void ElementInspector::deinit()
{
  m_internals->deinit();
}


void ElementInspectorInternal::deinit()
{

  for(size_t i = 0; i < m_inspectedImages.size(); i++)
  {
    deinitImageInspection(static_cast<uint32_t>(i), false);
  }
  for(size_t i = 0; i < m_inspectedImagesCopies.size(); i++)
  {
    deinitImageInspection(static_cast<uint32_t>(i), true);
  }


  for(size_t i = 0; i < m_inspectedBuffers.size(); i++)
  {
    deinitBufferInspection(static_cast<uint32_t>(i));
  }

  for(size_t i = 0; i < m_inspectedComputeVariables.size(); i++)
  {
    deinitComputeInspection(static_cast<uint32_t>(i));
  }

  for(size_t i = 0; i < m_inspectedFragmentVariables.size(); i++)
  {
    deinitFragmentInspection(static_cast<uint32_t>(i));
  }

  for(size_t i = 0; i < m_inspectedCustomVariables.size(); i++)
  {
    deinitCustomInspection(static_cast<uint32_t>(i));
  }

  vkDestroySampler(m_device, m_sampler, nullptr);
  m_sampler       = VK_NULL_HANDLE;
  m_isInitialized = false;
}


void ElementInspector::initImageInspection(uint32_t index, const ImageInspectionInfo& info)
{
  m_internals->initImageInspection(index, info);
}

void ElementInspectorInternal::initImageInspection(uint32_t index, const ElementInspector::ImageInspectionInfo& info)
{
  if(!checkInitialized())
  {
    return;
  }

  checkFormatFlag(info.format);
  ElementInspectorInternal::InspectedImage& inspectedImage = m_inspectedImages[index];


  inspectedImage.allocator = m_alloc;

  inspectedImage.device                   = m_device;
  inspectedImage.graphicsQueueFamilyIndex = m_graphicsQueueFamilyIndex;
  inspectedImage.sampler                  = m_sampler;


  inspectedImage.sourceImage = info.sourceImage;
  if(inspectedImage.isAllocated)
  {
    inspectedImage.isAllocated = false;
    m_alloc->destroy(inspectedImage.image);
    m_alloc->destroy(inspectedImage.hostBuffer);
    vkDestroyImageView(m_device, inspectedImage.view, nullptr);
  }

  createInspectedBuffer(inspectedImage, VK_NULL_HANDLE, info.name, toInternalFormat(info.format),
                        info.createInfo.extent.width * info.createInfo.extent.height, info.comment);
  inspectedImage.image      = m_alloc->createImage(info.createInfo);
  inspectedImage.createInfo = info.createInfo;

  {
    nvvk::ScopeCommandBuffer cmd(m_device, m_graphicsQueueFamilyIndex);
    nvvk::cmdBarrierImageLayout(cmd, inspectedImage.image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkClearColorValue       clearColor{0.f, 0.f, 0.f, 0.f};
    VkImageSubresourceRange range;
    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseArrayLayer = 0;
    range.baseMipLevel   = 0;
    range.layerCount     = 1;
    range.levelCount     = 1;
    vkCmdClearColorImage(cmd, inspectedImage.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
    nvvk::cmdBarrierImageLayout(cmd, inspectedImage.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }

  VkImageViewCreateInfo viewCreateInfo = nvvk::makeImageViewCreateInfo(inspectedImage.image.image, info.createInfo);

  vkCreateImageView(m_device, &viewCreateInfo, nullptr, &inspectedImage.view);

  ImGui_ImplVulkan_RemoveTexture(inspectedImage.imguiImage);
  inspectedImage.imguiImage = ImGui_ImplVulkan_AddTexture(m_sampler, inspectedImage.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void ElementInspector::deinitImageInspection(uint32_t index)
{
  m_internals->deinitImageInspection(index, false);
}


void ElementInspectorInternal::deinitImageInspection(uint32_t index, bool isCopy)
{
  if(!checkInitialized())
  {
    return;
  }

  InspectedImage& inspectedImage = isCopy ? m_inspectedImagesCopies[index] : m_inspectedImages[index];
  if(!inspectedImage.isAllocated)
  {
    return;
  }
  destroyInspectedBuffer(inspectedImage);
  vkDestroyImageView(m_device, inspectedImage.view, nullptr);
  m_alloc->destroy(inspectedImage.image);

  ImGui_ImplVulkan_RemoveTexture(inspectedImage.imguiImage);
  inspectedImage.isAllocated = false;
}


void ElementInspector::initBufferInspection(uint32_t index, const BufferInspectionInfo& info)
{
  m_internals->initBufferInspection(index, info);
}

void ElementInspectorInternal::initBufferInspection(uint32_t index, const ElementInspector::BufferInspectionInfo& info)
{
  if(!checkInitialized())
  {
    return;
  }
  checkFormatFlag(info.format);
  createInspectedBuffer(m_inspectedBuffers[index], info.sourceBuffer, info.name, toInternalFormat(info.format),
                        info.entryCount, info.comment, info.minEntry, info.viewMin, info.viewMax);
}

void ElementInspector::deinitBufferInspection(uint32_t index)
{
  m_internals->deinitBufferInspection(index);
}

void ElementInspectorInternal::deinitBufferInspection(uint32_t index)
{
  if(!checkInitialized())
  {
    return;
  }
  destroyInspectedBuffer(m_inspectedBuffers[index]);
}


bool ElementInspector::updateImageFormat(uint32_t index, const std::vector<nvvkhl::ElementInspector::ValueFormat>& newFormat)
{
  return m_internals->updateImageFormat(index, ElementInspectorInternal::toInternalFormat(newFormat));
}

bool ElementInspectorInternal::updateImageFormat(uint32_t index,
                                                 const std::vector<nvvkhl::ElementInspectorInternal::ValueFormat>& newFormat)
{
  if(!m_isInitialized)
  {
    return false;
  }
  InspectedImage& img = m_inspectedImages[index];
  return updateFormat(img, newFormat);
}


void ElementInspector::inspectImage(VkCommandBuffer cmd, uint32_t index, VkImageLayout currentLayout)
{
  m_internals->inspectImage(cmd, index, currentLayout);
}

void ElementInspectorInternal::inspectImage(VkCommandBuffer cmd, uint32_t index, VkImageLayout currentLayout)
{
  if(!m_isAttached || !m_isInitialized)
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


std::string formatType(const std::string& str)
{
  std::string res;
  res.resize(str.size() - 1);
  std::transform(str.begin() + 1, str.end(), res.begin(), ::tolower);
  return res;
}

std::string ElementInspectorInternal::valueFormatToString(const ElementInspectorInternal::ValueFormat& v)
{
  return v.name;
}


std::string ElementInspectorInternal::valueFormatTypeToString(const ElementInspectorInternal::ValueFormat& v)
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

void ElementInspectorInternal::bufferEntryToLdrPixel(const uint8_t*                                           contents,
                                                     const std::vector<ElementInspectorInternal::ValueFormat> format,
                                                     uint8_t* destination)
{

  const uint8_t* current = contents;
  for(size_t i = 0; i < format.size(); i++)
  {
    switch(format[i].type)
    {
      case eUint8:
        destination[i] = *reinterpret_cast<const uint8_t*>(current);
        break;
      case eInt8:
        destination[i] = *reinterpret_cast<const int8_t*>(current);
        break;
      case eUint16:
        destination[i] = static_cast<uint8_t>(*reinterpret_cast<const uint16_t*>(current));
        break;
      case eInt16:
        destination[i] = static_cast<uint8_t>(*reinterpret_cast<const int16_t*>(current));

        break;
      case eFloat16:
        destination[i] = static_cast<uint8_t>(glm::detail::toFloat32(*(const glm::detail::hdata*)current) * 255.f);
        break;
      case eUint32:
        destination[i] = *reinterpret_cast<const uint32_t*>(current);
        break;
      case eInt32:
        destination[i] = *reinterpret_cast<const int32_t*>(current);
        break;
      case eFloat32:
        destination[i] = static_cast<uint8_t>(*reinterpret_cast<const float*>(current) * 255.f);
        break;
      case eInt64:
        destination[i] = static_cast<uint8_t>(*reinterpret_cast<const int64_t*>(current));
        break;
      case eUint64:
        destination[i] = static_cast<uint8_t>(*reinterpret_cast<const uint64_t*>(current));
        break;
    }
    current += valueFormatSizeInBytes(format[i]);
  }
}


std::string ElementInspectorInternal::bufferEntryToString(const uint8_t* contents,
                                                          const std::vector<ElementInspectorInternal::ValueFormat> format)
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
  m_internals->inspectBuffer(cmd, index);
}

void ElementInspectorInternal::inspectBuffer(VkCommandBuffer cmd, uint32_t index)
{
  if(!m_isAttached || !m_isInitialized)
  {
    return;
  }
  if(m_settings.isPaused)
  {
    return;
  }
  InspectedBuffer& internalBuffer = m_inspectedBuffers[index];

  internalBuffer.filteredEntries = ~0u;

  size_t entrySize = internalBuffer.formatSizeInBytes;

  VkBufferCopy bcpy{};
  bcpy.size      = entrySize * internalBuffer.entryCount;
  bcpy.srcOffset = entrySize * internalBuffer.offsetInEntries;
  memoryBarrier(cmd);
  vkCmdCopyBuffer(cmd, internalBuffer.sourceBuffer, internalBuffer.hostBuffer.buffer, 1, &bcpy);
  memoryBarrier(cmd);
  internalBuffer.isInspected = true;
}


bool ElementInspector::updateBufferFormat(uint32_t index, const std::vector<ValueFormat>& newFormat)
{
  return m_internals->updateBufferFormat(index, ElementInspectorInternal::toInternalFormat(newFormat));
}

bool ElementInspectorInternal::updateBufferFormat(uint32_t index, const std::vector<ElementInspectorInternal::ValueFormat>& newFormat)
{
  if(!m_isInitialized)
  {
    return false;
  }
  InspectedBuffer& buffer = m_inspectedBuffers[index];
  return updateFormat(buffer, newFormat);
}


void ElementInspector::appendStructToFormat(std::vector<ValueFormat>&       format,
                                            const std::vector<ValueFormat>& addedStruct,
                                            const std::string               addedStructName,
                                            bool                            forceHidden /*= false*/)
{
  size_t originalSize = format.size();
  format.insert(format.end(), addedStruct.begin(), addedStruct.end());
  for(size_t i = originalSize; i < format.size(); i++)
  {
    if(addedStruct.size() == 1)
    {
      // Discard the input field name and only keep the struct name if the added struct has only one field
      format[i].name = fmt::format("{}", addedStructName);
    }
    else
    {
      format[i].name = fmt::format("{}.{}", addedStructName, format[i].name);
    }
    if(forceHidden)
    {
      format[i].flags = ElementInspector::eHidden;
    }
  }
}


void ElementInspector::initComputeInspection(uint32_t index, const ComputeInspectionInfo& info)
{
  m_internals->initComputeInspection(index, info);
}

void ElementInspectorInternal::initComputeInspection(uint32_t index, const ElementInspector::ComputeInspectionInfo& info)
{
  if(!checkInitialized())
  {
    return;
  }
  checkFormatFlag(info.format);
  InspectedComputeVariables& var = m_inspectedComputeVariables[index];

  var.blocksPerRow = info.uiBlocksPerRow;
  assert(info.uiBlocksPerRow > 0);
  var.blockSize = info.blockSize;


  uint32_t threadsPerBlock = var.blockSize.x * var.blockSize.y * var.blockSize.z;
  if(threadsPerBlock % WARP_SIZE != 0)
  {
    LOGE("Thread count per block (%d) is not a multiple of warp size (%d) - undefined behavior\n", threadsPerBlock, WARP_SIZE);
    assert(false);
  }

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

  var.format = toInternalFormat(info.format);


  uint32_t u32PerThread = computeFormatSizeInBytes(var.format);
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

  createInspectedBuffer(var, var.deviceBuffer.buffer, info.name, var.format, entryCount, info.comment);
  {
    nvvk::ScopeCommandBuffer                              cmd(m_device, m_graphicsQueueFamilyIndex);
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
  m_internals->deinitComputeInspection(index);
}

void ElementInspectorInternal::deinitComputeInspection(uint32_t index)
{
  if(!checkInitialized())
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
  m_internals->inspectComputeVariables(cmd, index);
}

void ElementInspectorInternal::inspectComputeVariables(VkCommandBuffer cmd, uint32_t index)
{
  if(!m_isAttached || !m_isInitialized)
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

  memoryBarrier(cmd);

  vkCmdCopyBuffer(cmd, var.deviceBuffer.buffer, var.hostBuffer.buffer, 1, &bcpy);

  memoryBarrier(cmd);
  var.isInspected = true;
}

bool ElementInspector::updateComputeFormat(uint32_t index, const std::vector<ValueFormat>& newFormat)
{
  return m_internals->updateComputeFormat(index, ElementInspectorInternal::toInternalFormat(newFormat));
}


bool ElementInspectorInternal::updateComputeFormat(uint32_t index, const std::vector<ElementInspectorInternal::ValueFormat>& newFormat)
{
  if(!m_isInitialized)
  {
    return false;
  }
  InspectedComputeVariables& var = m_inspectedComputeVariables[index];
  return updateFormat(var, newFormat);
}


VkBuffer ElementInspector::getComputeInspectionBuffer(uint32_t index)
{
  return m_internals->getComputeInspectionBuffer(index);
}

VkBuffer ElementInspectorInternal::getComputeInspectionBuffer(uint32_t index)
{
  if(!m_isInitialized)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedComputeVariables[index].deviceBuffer.buffer;
}

VkBuffer ElementInspector::getComputeMetadataBuffer(uint32_t index)
{
  return m_internals->getComputeMetadataBuffer(index);
}

VkBuffer ElementInspectorInternal::getComputeMetadataBuffer(uint32_t index)
{
  if(!m_isInitialized)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedComputeVariables[index].metadata.buffer;
}


void ElementInspector::initCustomInspection(uint32_t index, const CustomInspectionInfo& info)
{
  m_internals->initCustomInspection(index, info);
}

void ElementInspectorInternal::initCustomInspection(uint32_t index, const ElementInspector::CustomInspectionInfo& info)
{
  if(!checkInitialized())
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

  var.format = toInternalFormat(info.format);


  uint32_t u32PerThread = computeFormatSizeInBytes(var.format);
  assert(u32PerThread % 4 == 0 && "Format must be 32-bit aligned");
  u32PerThread /= 4;

  var.u32PerThread = u32PerThread;


  assert(var.maxCoord.x >= var.minCoord.x && var.maxCoord.y >= var.minCoord.y && var.maxCoord.z >= var.minCoord.z);

  glm::uvec3 inspectedValues = {var.maxCoord.x - var.minCoord.x + 1, var.maxCoord.y - var.minCoord.y + 1,
                                var.maxCoord.z - var.minCoord.z + 1};

  uint32_t entryCount = inspectedValues.x * inspectedValues.y * inspectedValues.z;
  uint32_t bufferSize = entryCount * u32PerThread * sizeof(uint32_t);


  var.deviceBuffer = m_alloc->createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  createInspectedBuffer(var, var.deviceBuffer.buffer, info.name, var.format, entryCount, info.comment);
  {
    nvvk::ScopeCommandBuffer                             cmd(m_device, m_graphicsQueueFamilyIndex);
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
  m_internals->deinitCustomInspection(index);
}

void ElementInspectorInternal::deinitCustomInspection(uint32_t index)
{
  if(!checkInitialized())
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
  m_internals->inspectCustomVariables(cmd, index);
}

void ElementInspectorInternal::inspectCustomVariables(VkCommandBuffer cmd, uint32_t index)
{
  if(!m_isAttached || !m_isInitialized)
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
  memoryBarrier(cmd);

  vkCmdCopyBuffer(cmd, var.deviceBuffer.buffer, var.hostBuffer.buffer, 1, &bcpy);

  memoryBarrier(cmd);

  var.isInspected = true;
}

bool ElementInspector::updateCustomFormat(uint32_t index, const std::vector<ValueFormat>& newFormat)
{
  return m_internals->updateCustomFormat(index, ElementInspectorInternal::toInternalFormat(newFormat));
}

bool ElementInspectorInternal::updateCustomFormat(uint32_t index, const std::vector<ElementInspectorInternal::ValueFormat>& newFormat)
{
  if(!m_isInitialized)
  {
    return false;
  }
  InspectedCustomVariables& var = m_inspectedCustomVariables[index];
  return updateFormat(var, newFormat);
}

VkBuffer ElementInspector::getCustomInspectionBuffer(uint32_t index)
{
  return m_internals->getCustomInspectionBuffer(index);
}


VkBuffer ElementInspectorInternal::getCustomInspectionBuffer(uint32_t index)
{
  if(!m_isInitialized)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedCustomVariables[index].deviceBuffer.buffer;
}

VkBuffer ElementInspector::getCustomMetadataBuffer(uint32_t index)
{
  return m_internals->getCustomMetadataBuffer(index);
}

VkBuffer ElementInspectorInternal::getCustomMetadataBuffer(uint32_t index)
{
  if(!m_isInitialized)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedCustomVariables[index].metadata.buffer;
}


void ElementInspector::initFragmentInspection(uint32_t index, const FragmentInspectionInfo& info)
{
  m_internals->initFragmentInspection(index, info);
}

void ElementInspectorInternal::initFragmentInspection(uint32_t index, const ElementInspector::FragmentInspectionInfo& info)
{
  if(!checkInitialized())
  {
    return;
  }

  checkFormatFlag(info.format);
  InspectedFragmentVariables& var = m_inspectedFragmentVariables[index];

  var.renderSize  = info.renderSize;
  var.minFragment = glm::clamp(info.minFragment, {0, 0}, info.renderSize);
  var.maxFragment = glm::clamp(info.maxFragment, {0, 0}, info.renderSize);

  if(var.deviceBuffer.buffer)
  {
    m_alloc->destroy(var.deviceBuffer);
    m_alloc->destroy(var.hostBuffer);
    m_alloc->destroy(var.metadata);
  }

  var.format.clear();
  for(size_t i = 0; i < info.format.size(); i++)
  {
    ValueFormat f = toInternalFormat({info.format[i]})[0];
    var.format.push_back(f);
    uint32_t valueSize = valueFormatSizeInBytes(f);
    while(valueSize < 4)
    {
      var.format.push_back({eUint8, "Pad", false, VALUE_FLAG_INTERNAL});
      valueSize += sizeof(uint8_t);
    }
    var.format.push_back({eFloat32, "Z", false, VALUE_FLAG_INTERNAL});
  }

  uint32_t u32PerThread = computeFormatSizeInBytes(var.format);
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
    nvvk::ScopeCommandBuffer                               cmd(m_device, m_graphicsQueueFamilyIndex);
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
  m_internals->deinitFragmentInspection(index);
}

void ElementInspectorInternal::deinitFragmentInspection(uint32_t index)
{
  if(!checkInitialized())
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
  m_internals->clearFragmentVariables(cmd, index);
}

void ElementInspectorInternal::clearFragmentVariables(VkCommandBuffer cmd, uint32_t index)
{
  if(!m_isAttached || !m_isInitialized)
  {
    return;
  }
  if(m_settings.isPaused)
  {
    return;
  }

  InspectedFragmentVariables& var = m_inspectedFragmentVariables[index];

  vkCmdFillBuffer(cmd, var.deviceBuffer.buffer, 0, VK_WHOLE_SIZE, 0u);
  memoryBarrier(cmd);
  var.isCleared = true;
}

void ElementInspector::inspectFragmentVariables(VkCommandBuffer cmd, uint32_t index)
{
  m_internals->inspectFragmentVariables(cmd, index);
}

void ElementInspectorInternal::inspectFragmentVariables(VkCommandBuffer cmd, uint32_t index)
{
  if(!m_isAttached || !m_isInitialized)
  {
    return;
  }
  if(m_settings.isPaused)
  {
    return;
  }

  InspectedFragmentVariables& var = m_inspectedFragmentVariables[index];

  if(!var.isCleared)
  {
    LOGW("Fragment shader variable inspection triggered without prior call to clearFragmentVariables()\n");
  }

  var.filteredEntries = ~0u;
  VkBufferCopy bcpy{};

  glm::uvec2 inspectedFragments = var.maxFragment - var.minFragment + glm::uvec2(1, 1);

  uint32_t fragmentCount = inspectedFragments.x * inspectedFragments.y;

  bcpy.size = fragmentCount * var.u32PerThread * sizeof(uint32_t);

  memoryBarrier(cmd);

  vkCmdCopyBuffer(cmd, var.deviceBuffer.buffer, var.hostBuffer.buffer, 1, &bcpy);

  memoryBarrier(cmd);

  var.isInspected = true;
  var.isCleared   = false;
}

void ElementInspector::updateMinMaxFragmentInspection(VkCommandBuffer cmd, uint32_t index, const glm::uvec2& minFragment, const glm::uvec2& maxFragment)
{
  m_internals->updateMinMaxFragmentInspection(cmd, index, minFragment, maxFragment);
}

void ElementInspectorInternal::updateMinMaxFragmentInspection(VkCommandBuffer   cmd,
                                                              uint32_t          index,
                                                              const glm::uvec2& minFragment,
                                                              const glm::uvec2& maxFragment)
{
  if(!m_isAttached || !m_isInitialized)
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

  memoryBarrier(cmd);
}


bool ElementInspector::updateFragmentFormat(uint32_t index, const std::vector<ValueFormat>& newFormat)
{
  return m_internals->updateFragmentFormat(index, ElementInspectorInternal::toInternalFormat(newFormat));
}

bool ElementInspectorInternal::updateFragmentFormat(uint32_t index, const std::vector<ElementInspectorInternal::ValueFormat>& newFormat)
{
  if(!m_isInitialized)
  {
    return false;
  }
  InspectedFragmentVariables& var = m_inspectedFragmentVariables[index];
  return updateFormat(var, newFormat);
}

VkBuffer ElementInspector::getFragmentInspectionBuffer(uint32_t index)
{
  return m_internals->getFragmentInspectionBuffer(index);
}

VkBuffer ElementInspectorInternal::getFragmentInspectionBuffer(uint32_t index)
{
  if(!m_isInitialized)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedFragmentVariables[index].deviceBuffer.buffer;
}

VkBuffer ElementInspector::getFragmentMetadataBuffer(uint32_t index)
{
  return m_internals->getFragmentMetadataBuffer(index);
}

VkBuffer ElementInspectorInternal::getFragmentMetadataBuffer(uint32_t index)
{
  if(!m_isInitialized)
  {
    return VK_NULL_HANDLE;
  }
  return m_inspectedFragmentVariables[index].metadata.buffer;
}

std::vector<nvvkhl::ElementInspector::ValueFormat> ElementInspector::formatRGBA8(const std::string& name /*= ""*/)
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

std::vector<nvvkhl::ElementInspector::ValueFormat> ElementInspector::formatRGBA32(const std::string& name /*= ""*/)
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

std::vector<nvvkhl::ElementInspector::ValueFormat> ElementInspector::formatVector4(ValueType type, const std::string& name)
{
  if(name == "")
  {
    return {{type, "x"}, {type, "y"}, {type, "z"}, {type, "w"}};
  }
  else
  {
    return {
        {type, name + ".x"},
        {type, name + ".y"},
        {type, name + ".z"},
        {type, name + ".w"},
    };
  }
}

std::vector<nvvkhl::ElementInspector::ValueFormat> ElementInspector::formatVector3(ValueType type, const std::string& name)
{
  if(name == "")
  {
    return {{type, "x"}, {type, "y"}, {type, "z"}};
  }
  else
  {
    return {{type, name + ".x"}, {type, name + ".y"}, {type, name + ".z"}};
  }
}

std::vector<nvvkhl::ElementInspector::ValueFormat> ElementInspector::formatVector2(ValueType type, const std::string& name)
{
  if(name == "")
  {
    return {{type, "x"}, {type, "y"}};
  }
  else
  {
    return {{type, name + ".x"}, {type, name + ".y"}};
  }
}

std::vector<nvvkhl::ElementInspector::ValueFormat> ElementInspector::formatValue(ValueType type, const std::string& name)
{
  return {{type, name}};
}

std::vector<nvvkhl::ElementInspector::ValueFormat> ElementInspector::formatInt32(const std::string& name /*= "value"*/)
{
  return formatValue(eInt32, name);
}

std::vector<nvvkhl::ElementInspector::ValueFormat> ElementInspector::formatUint32(const std::string& name /*= "value"*/)
{
  return formatValue(eUint32, name);
}

std::vector<nvvkhl::ElementInspector::ValueFormat> ElementInspector::formatFloat32(const std::string& name /*= "value"*/)
{
  return formatValue(eFloat32, name);
}

void ElementInspectorInternal::Filter::create(const ElementInspectorInternal::InspectedBuffer& b)
{
  uint32_t formatSize = b.formatSizeInBytes;
  dataMin.resize(formatSize);
  dataMax.resize(formatSize);
  requestedDataMin.resize(formatSize);
  requestedDataMax.resize(formatSize);
  hasFilter.resize(b.format.size(), false);
}


template <typename T>
void sanitizeFilterMax(uint8_t* minValue, uint8_t* maxValue)
{
  auto typedMin = reinterpret_cast<T*>(minValue);
  auto typedMax = reinterpret_cast<T*>(maxValue);
  *typedMax     = std::max(*typedMin, *typedMax);
}


void sanitizeFilterBounds(const std::vector<ElementInspectorInternal::ValueFormat>& format, uint8_t* filterMin, uint8_t* filterMax)
{

  uint8_t* currentMin = filterMin;
  uint8_t* currentMax = filterMax;
  for(size_t i = 0; i < format.size(); i++)
  {
    switch(format[i].type)
    {
      case ElementInspectorInternal::eUint8:
        sanitizeFilterMax<uint8_t>(currentMin, currentMax);
        break;
      case ElementInspectorInternal::eInt8:
        sanitizeFilterMax<int8_t>(currentMin, currentMax);
        break;
      case ElementInspectorInternal::eUint16:
        sanitizeFilterMax<uint16_t>(currentMin, currentMax);
        break;
      case ElementInspectorInternal::eInt16:
        sanitizeFilterMax<int16_t>(currentMin, currentMax);
        break;
      case ElementInspectorInternal::eFloat16: {

        auto hMin = reinterpret_cast<glm::detail::hdata*>(currentMin);
        auto hMax = reinterpret_cast<glm::detail::hdata*>(currentMax);

        float currentMinFloat = glm::detail::toFloat32(*hMin);
        float currentMaxFloat = glm::detail::toFloat32(*hMax);
        sanitizeFilterMax<float>(reinterpret_cast<uint8_t*>(&currentMinFloat), reinterpret_cast<uint8_t*>(&currentMaxFloat));
        *hMin = glm::detail::toFloat16(currentMinFloat);
        *hMax = glm::detail::toFloat16(currentMaxFloat);
      }
      break;
      case ElementInspectorInternal::eUint32:
        sanitizeFilterMax<uint32_t>(currentMin, currentMax);
        break;
      case ElementInspectorInternal::eInt32:
        sanitizeFilterMax<int32_t>(currentMin, currentMax);
        break;
      case ElementInspectorInternal::eFloat32:
        sanitizeFilterMax<float>(currentMin, currentMax);
        break;
      case ElementInspectorInternal::eInt64:
        sanitizeFilterMax<int64_t>(currentMin, currentMax);
        break;
      case ElementInspectorInternal::eUint64:
        sanitizeFilterMax<uint64_t>(currentMin, currentMax);
        break;
    }

    uint32_t sizeInBytes = ElementInspectorInternal::valueFormatSizeInBytes(format[i]);
    currentMin += sizeInBytes;
    currentMax += sizeInBytes;
  }
}


bool ElementInspectorInternal::Filter::imguiFilterColumns(const ElementInspectorInternal::InspectedBuffer& b)
{
  const auto& format         = b.format;
  bool        hasChanged     = false;
  uint8_t*    currentDataMin = requestedDataMin.data();
  uint8_t*    currentDataMax = requestedDataMax.data();
  for(size_t i = 0; i < format.size(); i++)
  {
    if(format[i].flags == ElementInspector::eHidden || format[i].flags == VALUE_FLAG_INTERNAL)
    {
      currentDataMin += valueFormatSizeInBytes(format[i]);
      currentDataMax += valueFormatSizeInBytes(format[i]);
      continue;
    }
    ImGui::TableNextColumn();
    bool v = hasFilter[static_cast<uint32_t>(i)];

    ImGui::BeginDisabled(!hasFilter[static_cast<uint32_t>(i)]);
    if(hasFilter[static_cast<uint32_t>(i)])
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
  if(hasChanged)
  {
    sanitizeFilterBounds(format, requestedDataMin.data(), requestedDataMax.data());
  }
  return hasChanged;
}

bool ElementInspectorInternal::Filter::filterPasses(const InspectedBuffer& buffer, uint32_t offset)
{
  const uint8_t* mappedData   = buffer.isCopy ? nullptr : (buffer.mappedContents + offset);
  const uint8_t* snapshotData = buffer.hasSnapshotContents ? (buffer.snapshotContents.data() + offset) : nullptr;

  if(buffer.showOnlyDiffToSnapshot && mappedData != nullptr && snapshotData != nullptr)
  {
    bool     diffFromRef = false;
    uint32_t formatSize  = buffer.formatSizeInBytes;
    diffFromRef          = (memcmp(mappedData, snapshotData, formatSize) != 0);
    if(!diffFromRef)
    {
      return false;
    }
  }

  if(!buffer.filter.hasAnyFilter())
  {
    return true;
  }

  const uint8_t* currentMin = dataMin.data();
  const uint8_t* currentMax = dataMax.data();
  for(size_t i = 0; i < buffer.format.size(); i++)
  {
    if(hasFilter[static_cast<uint32_t>(i)])
    {

      switch(buffer.format[i].type)
      {
        case eUint8:
          if(!passes<uint8_t>(mappedData, snapshotData, currentMin, currentMax))
          {
            return false;
          }
          break;
        case eInt8:
          if(!passes<int8_t>(mappedData, snapshotData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eUint16:
          if(!passes<uint16_t>(mappedData, snapshotData, currentMin, currentMax))
          {
            return false;
          }
          break;
        case eInt16:
          if(!passes<int16_t>(mappedData, snapshotData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eFloat16: {
          float currentDataFloat = mappedData ? (glm::detail::toFloat32(*(const glm::detail::hdata*)mappedData)) : 0.f;
          float snapshotDataFloat = snapshotData ? (glm::detail::toFloat32(*(const glm::detail::hdata*)snapshotData)) : 0.f;
          float currentMinFloat = glm::detail::toFloat32(*(const glm::detail::hdata*)currentMin);
          float currentMaxFloat = glm::detail::toFloat32(*(const glm::detail::hdata*)currentMax);
          if(!passes<float>(mappedData ? (reinterpret_cast<uint8_t*>(&currentDataFloat)) : nullptr,
                            snapshotData ? (reinterpret_cast<uint8_t*>(&snapshotDataFloat)) : nullptr,
                            reinterpret_cast<uint8_t*>(&currentMinFloat), reinterpret_cast<uint8_t*>(&currentMaxFloat)))
          {
            return false;
          }
        }
        break;
        case eUint32:
          if(!passes<uint32_t>(mappedData, snapshotData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eInt32:
          if(!passes<int32_t>(mappedData, snapshotData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eFloat32:
          if(!passes<float>(mappedData, snapshotData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eInt64:
          if(!passes<int64_t>(mappedData, snapshotData, currentMin, currentMax))
          {
            return false;
          }

          break;
        case eUint64:
          if(!passes<uint64_t>(mappedData, snapshotData, currentMin, currentMax))
          {
            return false;
          }
          break;
      }
    }
    uint32_t sizeInBytes = valueFormatSizeInBytes(buffer.format[i]);
    mappedData += sizeInBytes;
    currentMin += sizeInBytes;
    currentMax += sizeInBytes;
  }
  return true;
}
}  // namespace nvvkhl