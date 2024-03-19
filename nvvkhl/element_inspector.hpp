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


/** @DOC_START

# class nvvkhl::ElementInspector

--------------------------------------------------------------------------------------------------
 This element is used to facilitate GPU debugging by inspection of:
  - Image contents
  - Buffer contents
  - Variables in compute shaders
  - Variables in fragment shaders

  IMPORTANT NOTE: if used in a multi threaded environment synchronization needs to be performed
  externally by the application. 

 Basic usage:
 ------------------------------------------------------------------------------------------------
 ###                 INITIALIZATION
 ------------------------------------------------------------------------------------------------
 
 Create the element as a global variable, and add it to the applications
 ```cpp
 std::shared_ptr<ElementInspector> g_inspectorElement = std::make_shared<ElementInspector>();

 void main(...)
 {
   ...
   app->addElement(g_inspectorElement);
   ...
  }
 ```
 Upon attachment of the main app element, initialize the Inspector and specify the number of
 buffers, images, compute shader variables and fragment shader variables that it will need to
 inspect
 ```cpp
 void onAttach(nvvkhl::Application* app) override
 {
   ...
    ElementInspector::InitInfo initInfo{};
    initInfo.allocator     = m_alloc.get();
    initInfo.imageCount    = imageInspectionCount;
    initInfo.bufferCount   = bufferInspectionCount;
    initInfo.computeCount  = computeInspectionCount;
    initInfo.fragmentCount = fragmentInspectionCount;
    initInfo.customCount   = customInspectionCount;
    initInfo.device = m_app->getDevice();
    initInfo.graphicsQueueFamilyIndex = m_app->getQueueGCT().familyIndex;

    g_inspectorElement->init(initInfo);
   ...
  }
  ```

 ------------------------------------------------------------------------------------------------
 ###                 BUFFER INSPECTION
 ------------------------------------------------------------------------------------------------
 
 Each inspection needs to be initialized before use:
 Inspect a buffer of size bufferSize, where each entry contains 5 values. The buffer format specifies the data
 structure of the entries. The following format is the equivalent of
 ```cpp
  // struct
  // {
  //   uint32_t counterU32;
  //   float    counterF32;
  //   int16_t  anI16Value;
  //   uint16_t myU16;
  //   int32_t  anI32;
  // };
 ```

 ```cpp
 bufferFormat    = std::vector<ElementInspector::ValueFormat>(5);
 bufferFormat[0] = {ElementInspector::eUint32, "counterU32"};
 bufferFormat[1] = {ElementInspector::eFloat32, "counterF32"};
 bufferFormat[2] = {ElementInspector::eInt16, "anI16Value"};
 bufferFormat[3] = {ElementInspector::eUint16, "myU16"};
 bufferFormat[4] = {ElementInspector::eInt32, "anI32"};
 ElementInspector::BufferInspectionInfo info{};
 info.entryCount   = bufferSize;
 info.format       = bufferFormat;
 info.name         = "myBuffer";
 info.sourceBuffer = m_buffer.buffer;
 g_inspectorElement->initBufferInspection(0, info);
 ```

 When the inspection is desired, simply add it to the current command buffer. Required barriers are added internally.
 IMPORTANT: the buffer MUST have been created with the VK_BUFFER_USAGE_TRANSFER_SRC_BIT flag
 ```cpp
 g_inspectorElement->inspectBuffer(cmd, 0);
 ```

 ------------------------------------------------------------------------------------------------
 ###                 IMAGE INSPECTION
 ------------------------------------------------------------------------------------------------
 
 Inspection of the image stored in m_texture, with format RGBA32F. Other formats can be specified using the syntax
 above
 ```cpp
 ElementInspector::ImageInspectionInfo info{};
 info.createInfo  = create_info;
 info.format      = g_inspectorElement->formatRGBA32();
 info.name        = "MyImageInspection";
 info.sourceImage = m_texture.image;
 g_inspectorElement->initImageInspection(0, info);
 ```

 When the inspection is desired, simply add it to the current command buffer. Required barriers are added internally.
 ```cpp
 g_inspectorElement->inspectImage(cmd, 0, imageCurrentLayout);
 ```

 ------------------------------------------------------------------------------------------------
 ###                 COMPUTE SHADER VARIABLE INSPECTION
 ------------------------------------------------------------------------------------------------
 
 Inspect a compute shader variable for a given 3D grid and block size (use 1 for unused dimensions). This mode applies
 to shaders where invocation IDs (e.g. gl_LocalInvocationID) are defined, such as compute, mesh and task shaders.
 Since grids may contain many threads capturing a variable for all threads
 may incur large memory consumption and performance loss. The blocks to inspect, and the warps within those blocks can
 be specified using inspectedMin/MaxBlocks and inspectedMin/MaxWarp. 
 ```cpp
 computeInspectionFormat    = std::vector<ElementInspector::ValueFormat>(...); 
 ElementInspector::ComputeInspectionInfo info{}; info.blockSize = blockSize;
 
 // Create a 4-component vector format where each component is a uint32_t. The components will be labeled myVec4u.x,
 // myVec4u.y, myVec4u.z, myVec4u.w in the UI 
 info.format           = ElementInspector::formatVector4(eUint32, "myVec4u"); 
 info.gridSizeInBlocks = gridSize; 
 info.minBlock         = inspectedMinBlock; 
 info.maxBlock         = inspectedMaxBlock; 
 info.minWarp          = inspectedMinWarp; 
 info.maxWarp          = inspectedMaxWarp; 
 info.name             = "My Compute Inspection"; 
 g_inspectorElement->initComputeInspection(0, info);
 ```

 To allow variable inspection two buffers need to be made available to the target shader:
 m_computeShader.updateBufferBinding(eThreadInspection, g_inspectorElement->getComputeInspectionBuffer(0));
 m_computeShader.updateBufferBinding(eThreadMetadata, g_inspectorElement->getComputeMetadataBuffer(0));

 The shader code needs to indicate include the Inspector header along with preprocessor variables to set the
 inspection mode to Compute, and indicate the binding points for the buffers: 
 ```cpp
 #define INSPECTOR_MODE_COMPUTE 
 #define INSPECTOR_DESCRIPTOR_SET 0 
 #define INSPECTOR_INSPECTION_DATA_BINDING 1 
 #define INSPECTOR_METADATA_BINDING 2 
 #include "dh_inspector.h"
 ```

 The inspection of a variable is then done as follows. For alignment purposes the inspection is done with a 32-bit
 granularity. The shader is responsible for packing the inspected variables in 32-bit uint words. Those will be
 unpacked within the Inspector for display according to the specified format. 
 ```cpp
 uint32_t myVariable = myFunction(...);
 inspect32BitValue(0, myVariable);
 ```

 The inspection is triggered on the host side right after the compute shader invocation:
 ```cpp
 m_computeShader.dispatchBlocks(cmd, computGridSize, &constants);
 
 g_inspectorElement->inspectComputeVariables(cmd, 0);
 ```

 ------------------------------------------------------------------------------------------------
 ###                 FRAGMENT SHADER VARIABLE INSPECTION
 ------------------------------------------------------------------------------------------------
 
 Inspect a fragment shader variable for a given output image resolution. Since the image may have high resolution
 capturing a variable for all threads may incur large memory consumption and performance loss. The bounding box of the
 fragments to inspect can be specified using inspectedMin/MaxCoord. 
 IMPORTANT: Overlapping geometry may trigger
 several fragment shader invocations for a given pixel. The inspection will only store the value of the foremost
 fragment (with the lowest gl_FragCoord.z). 
 ```cpp
 fragmentInspectionFormat    = std::vector<ElementInspector::ValueFormat>(...); 
 FragmentInspectionInfo info{}; 
 info.name        = "My Fragment Inspection";
 info.format      = fragmentInspectionFormat;
 info.renderSize  = imageSize;
 info.minFragment = inspectedMinCoord;
 info.maxFragment = inspectedMaxCoord;
 g_inspectorElement->initFragmentInspection(0, info);
 ```

 To allow variable inspection two storage buffers need to be declared in the pipeline layout and made available
 as follows:
 ```cpp
 std::vector<VkWriteDescriptorSet> writes;
 
 const VkDescriptorBufferInfo inspectorInspection{
    g_inspectorElement->getFragmentInspectionBuffer(0), 
    0, 
    VK_WHOLE_SIZE}; 
 writes.emplace_back(m_dset->makeWrite(0, 1, &inspectorInspection)); 
 const VkDescriptorBufferInfo inspectorMetadata{
    g_inspectorElement->getFragmentMetadataBuffer(0), 
    0, 
    VK_WHOLE_SIZE};
 writes.emplace_back(m_dset->makeWrite(0, 2, &inspectorMetadata));

 vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
 ```

 The shader code needs to indicate include the Inspector header along with preprocessor variables to set the
 inspection mode to Fragment, and indicate the binding points for the buffers: 
 ```cpp
 #define INSPECTOR_MODE_FRAGMENT 
 #define INSPECTOR_DESCRIPTOR_SET 0 
 #define INSPECTOR_INSPECTION_DATA_BINDING 1 
 #define INSPECTOR_METADATA_BINDING 2 
 #include "dh_inspector.h"
 ```

 The inspection of a variable is then done as follows. For alignment purposes the inspection is done with a 32-bit
 granularity. The shader is responsible for packing the inspected variables in 32-bit uint words. Those will be
 unpacked within the Inspector for display according to the specified format. 
 ```cpp
 uint32_t myVariable = myFunction(...);
 inspect32BitValue(0, myVariable);
 ```

 The inspection data for a pixel will only be written if a fragment actually covers that pixel. To avoid ghosting
 where no fragments are rendered it is useful to clear the inspection data before rendering:
 ```cpp
 g_inspectorElement->clearFragmentVariables(cmd, 0);
 vkCmdBeginRendering(...);
 ```

 The inspection is triggered on the host side right after rendering:
 ```cpp
 vkCmdEndRendering(cmd);
 g_inspectorElement->inspectFragmentVariables(cmd, 0);
 ```

 ------------------------------------------------------------------------------------------------
 ###                 CUSTOM SHADER VARIABLE INSPECTION
 ------------------------------------------------------------------------------------------------
 
 In case some in-shader data has to be inspected in other shader types, or not on a once-per-thread basis, the custom
 inspection mode can be used. This mode allows the user to specify the overall size of the generated data as well as
 an effective inspection window. This mode may be used in conjunction with the COMPUTE and FRAGMENT modes.
 std::vector<ElementInspector::ValueFormat> customCaptureFormat;
 ```cpp
 ...
 ElementInspector::CustomInspectionInfo info{};
 info.extent   = totalInspectionSize;
 info.format   = customCaptureFormat;
 info.minCoord = inspectionWindowMin;
 info.maxCoord = inspectionWindowMax;
 info.name     = "My Custom Capture";
 g_inspectorElement->initCustomInspection(0, info);
 ```

 To allow variable inspection two buffers need to be made available to the target pipeline:
 ```cpp
 std::vector<VkWriteDescriptorSet> writes;
 const VkDescriptorBufferInfo      inspectorInspection{
    g_inspectorElement->getCustomInspectionBuffer(0), 
    0,
    VK_WHOLE_SIZE}; 
 writes.emplace_back(m_dset->makeWrite(0, 1, &inspectorInspection)); 
 const VkDescriptorBufferInfo inspectorMetadata{
    g_inspectorElement->getCustomMetadataBuffer(0), 
    0, 
    VK_WHOLE_SIZE};
 writes.emplace_back(m_dset->makeWrite(0, 2, &inspectorMetadata));

 vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
 ```

 The shader code needs to indicate include the Inspector header along with preprocessor variables to set the
 inspection mode to Fragment, and indicate the binding points for the buffers: 
 ```cpp
 #define INSPECTOR_MODE_CUSTOM 
 #define INSPECTOR_DESCRIPTOR_SET 0 
 #define INSPECTOR_CUSTOM_INSPECTION_DATA_BINDING 1 
 #define INSPECTOR_CUSTOM_METADATA_BINDING 2 
 #include "dh_inspector.h"
 ```
 The inspection of a variable is then done as follows. For alignment purposes the inspection is done with a 32-bit
 granularity. The shader is responsible for packing the inspected variables in 32-bit uint words. Those will be
 unpacked within the Inspector for display according to the specified format. 
 ```cpp
 uint32_t myVariable = myFunction(...);
 inspectCustom32BitValue(0, myCoordinates, myVariable);
 ```

 The inspection is triggered on the host side right after running the pipeline:
 ```cpp
 g_inspectorElement->inspectCustomVariables(cmd, 0);
 ```
@DOC_END */

namespace nvvkhl {

class ElementInspectorInternal;

class ElementInspector : public nvvkhl::IAppElement
{
public:
  ElementInspector();
  ~ElementInspector() override;

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

    eU8Vec2,
    eU8Vec3,
    eU8Vec4,

    eU16Vec2,
    eU16Vec3,
    eU16Vec4,

    eU32Vec2,
    eU32Vec3,
    eU32Vec4,

    eU64Vec2,
    eU64Vec3,
    eU64Vec4,


    eS8Vec2,
    eS8Vec3,
    eS8Vec4,

    eS16Vec2,
    eS16Vec3,
    eS16Vec4,

    eS32Vec2,
    eS32Vec3,
    eS32Vec4,

    eS64Vec2,
    eS64Vec3,
    eS64Vec4,

    eF16Vec2,
    eF16Vec3,
    eF16Vec4,

    eF32Vec2,
    eF32Vec3,
    eF32Vec4,


    eU8Mat2x2,
    eU8Mat2x3,
    eU8Mat2x4,
    eU16Mat2x2,
    eU16Mat2x3,
    eU16Mat2x4,
    eU32Mat2x2,
    eU32Mat2x3,
    eU32Mat2x4,
    eU64Mat2x2,
    eU64Mat2x3,
    eU64Mat2x4,
    eU8Mat3x2,
    eU8Mat3x3,
    eU8Mat3x4,
    eU16Mat3x2,
    eU16Mat3x3,
    eU16Mat3x4,
    eU32Mat3x2,
    eU32Mat3x3,
    eU32Mat3x4,
    eU64Mat3x2,
    eU64Mat3x3,
    eU64Mat3x4,
    eU8Mat4x2,
    eU8Mat4x3,
    eU8Mat4x4,
    eU16Mat4x2,
    eU16Mat4x3,
    eU16Mat4x4,
    eU32Mat4x2,
    eU32Mat4x3,
    eU32Mat4x4,
    eU64Mat4x2,
    eU64Mat4x3,
    eU64Mat4x4,

    eS8Mat2x2,
    eS8Mat2x3,
    eS8Mat2x4,
    eS16Mat2x2,
    eS16Mat2x3,
    eS16Mat2x4,
    eS32Mat2x2,
    eS32Mat2x3,
    eS32Mat2x4,
    eS64Mat2x2,
    eS64Mat2x3,
    eS64Mat2x4,
    eS8Mat3x2,
    eS8Mat3x3,
    eS8Mat3x4,
    eS16Mat3x2,
    eS16Mat3x3,
    eS16Mat3x4,
    eS32Mat3x2,
    eS32Mat3x3,
    eS32Mat3x4,
    eS64Mat3x2,
    eS64Mat3x3,
    eS64Mat3x4,
    eS8Mat4x2,
    eS8Mat4x3,
    eS8Mat4x4,
    eS16Mat4x2,
    eS16Mat4x3,
    eS16Mat4x4,
    eS32Mat4x2,
    eS32Mat4x3,
    eS32Mat4x4,
    eS64Mat4x2,
    eS64Mat4x3,
    eS64Mat4x4,

    eF16Mat2x2,
    eF16Mat2x3,
    eF16Mat2x4,
    eF32Mat2x2,
    eF32Mat2x3,
    eF32Mat2x4,
    eF16Mat3x2,
    eF16Mat3x3,
    eF16Mat3x4,
    eF32Mat3x2,
    eF32Mat3x3,
    eF32Mat3x4,
    eF16Mat4x2,
    eF16Mat4x3,
    eF16Mat4x4,
    eF32Mat4x2,
    eF32Mat4x3,
    eF32Mat4x4,


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
    ValueType   type{eUint32};
    std::string name;
    bool        hexDisplay{false};
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
    VkDevice                 device{VK_NULL_HANDLE};
    uint32_t                 graphicsQueueFamilyIndex{~0u};
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
  bool updateImageFormat(uint32_t index, const std::vector<nvvkhl::ElementInspector::ValueFormat>& newFormat);
  void inspectImage(VkCommandBuffer cmd, uint32_t index, VkImageLayout currentLayout);

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
  void inspectBuffer(VkCommandBuffer cmd, uint32_t index);
  bool updateBufferFormat(uint32_t index, const std::vector<nvvkhl::ElementInspector::ValueFormat>& newFormat);

  static void appendStructToFormat(std::vector<ValueFormat>&       format,
                                   const std::vector<ValueFormat>& addedStruct,
                                   const std::string               addedStructName,
                                   bool                            forceHidden = false);

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
  bool updateComputeFormat(uint32_t index, const std::vector<nvvkhl::ElementInspector::ValueFormat>& newFormat);

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

  bool     updateCustomFormat(uint32_t index, const std::vector<ValueFormat>& newFormat);
  VkBuffer getCustomInspectionBuffer(uint32_t index);
  VkBuffer getCustomMetadataBuffer(uint32_t index);


  struct FragmentInspectionInfo
  {
    std::string              name;
    std::string              comment;
    std::vector<ValueFormat> format;
    glm::uvec2               renderSize{0, 0};
    glm::uvec2               minFragment{0, 0};
    glm::uvec2               maxFragment{~0u, ~0u};
  };

  void initFragmentInspection(uint32_t index, const FragmentInspectionInfo& info);
  void deinitFragmentInspection(uint32_t index);
  void updateMinMaxFragmentInspection(VkCommandBuffer cmd, uint32_t index, const glm::uvec2& minFragment, const glm::uvec2& maxFragment);

  void clearFragmentVariables(VkCommandBuffer cmd, uint32_t index);
  void inspectFragmentVariables(VkCommandBuffer cmd, uint32_t index);


  bool     updateFragmentFormat(uint32_t index, const std::vector<ValueFormat>& newFormat);
  VkBuffer getFragmentInspectionBuffer(uint32_t index);
  VkBuffer getFragmentMetadataBuffer(uint32_t index);


  static std::vector<ValueFormat> formatRGBA8(const std::string& name = "");
  static std::vector<ValueFormat> formatRGBA32(const std::string& name = "");
  static std::vector<ValueFormat> formatVector4(ValueType type, const std::string& name);
  static std::vector<ValueFormat> formatVector3(ValueType type, const std::string& name);
  static std::vector<ValueFormat> formatVector2(ValueType type, const std::string& name);
  static std::vector<ValueFormat> formatValue(ValueType type, const std::string& name);
  static std::vector<ValueFormat> formatInt32(const std::string& name = "value");
  static std::vector<ValueFormat> formatUint32(const std::string& name = "value");
  static std::vector<ValueFormat> formatFloat32(const std::string& name = "value");

private:
  ElementInspectorInternal* m_internals{nullptr};
};
}  // namespace nvvkhl