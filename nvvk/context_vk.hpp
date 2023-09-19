/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef NV_VK_DEVICEINSTANCE_INCLUDED
#define NV_VK_DEVICEINSTANCE_INCLUDED

#include <string>    // std::string
#include <string.h>  // memcpy
#include <unordered_set>
#include <vector>
#include <functional>
#include <vulkan/vulkan_core.h>

#include "nsight_aftermath_vk.hpp"

static_assert(VK_HEADER_VERSION >= 261, "Vulkan SDK version needs to be 1.3.261.0 or greater");

namespace nvvk {
/**
To run a Vulkan application, you need to create the Vulkan instance and device.
This is done using the `nvvk::Context`, which wraps the creation of `VkInstance`
and `VkDevice`.

First, any application needs to specify how instance and device should be created:
Version, layers, instance and device extensions influence the features available.
This is done through a temporary and intermediate class that will allow you to gather
all the required conditions for the device creation.
*/

//////////////////////////////////////////////////////////////////////////
/**
# struct ContextCreateInfo

This structure allows the application to specify a set of features
that are expected for the creation of
- VkInstance
- VkDevice

It is consumed by the `nvvk::Context::init` function.

Example on how to populate information in it : 

\code{.cpp}
    nvvk::ContextCreateInfo ctxInfo;
    ctxInfo.setVersion(1, 2);
    ctxInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME, false);
    ctxInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, false);
    ctxInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME, false);

    // adding an extension with a feature struct:
    //
    VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR pipePropFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR};
    // Be aware of the lifetime of the pointer of the feature struct.
    // ctxInfo stores the pointer directly and context init functions use it for read & write access.
    ctxInfo.addDeviceExtension(VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME, true, &pipePropFeatures);

    // disabling a feature:
    //
    // This callback is called after the feature structs were filled with physical device information
    // and prior logical device creation.
    // The callback iterates over all feature structs, including those from
    // the vulkan versions.
    ctxInfo.fnDisableFeatures = [](VkStructureType sType, void *pFeatureStruct)
    {
      switch(sType){
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES:
        {
          auto* features11 = reinterpret_cast<VkPhysicalDeviceVulkan11Features>(pFeatureStruct);
          // at this point the struct is populated with what the device supports
          // and therefore it is only legal to disable features, not enable them.
          
          // let's say we wanted to disable multiview
          features11->multiView = VK_FALSE;
        }
        break;
      default:
        break;
      }
    };

\endcode

then you are ready to create initialize `nvvk::Context`

> Note: In debug builds, the extension `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` and the layer `VK_LAYER_KHRONOS_validation` are added to help finding issues early.

*/

static const VkDeviceDiagnosticsConfigFlagsNV defaultAftermathFlags =
    (VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV  // Additional information about the resource related to a GPU virtual address
     | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV  // Automatic checkpoints for all draw calls (ADD OVERHEAD)
     | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV);  // instructs the shader compiler to generate debug information (ADD OVERHEAD)


struct ContextCreateInfo
{
  // aftermathFlags != 0 will enable GPU crash dumps when Aftermath is available via SUPPORT_AFTERMATH
  // No-op when Aftermath is not available.
  ContextCreateInfo(bool bUseValidation = true, VkDeviceDiagnosticsConfigFlagsNV aftermathFlags = defaultAftermathFlags);

  void setVersion(uint32_t major, uint32_t minor);

  void addInstanceExtension(const char* name, bool optional = false);
  void addInstanceLayer(const char* name, bool optional = false);

  // Add a extension to be enabled at context creation time. If 'optional' is
  // false, context creation will fail if the extension is not supported by the
  // device. If the extension requires a feature struct, pass the initialized
  // struct to 'pFeatureStruct'. If 'version' = 0: don't care, otherwise check
  // against equality (useful for provisional exts)
  //
  // IMPORTANT: The 'pFeatureStruct' pointer will be stored and the object will
  // later be written to! Make sure the pointer is still valid when
  // Context::Init() gets called with the ContextCreateInfo object. All
  // pFeatureStruct objects will be chained together and filled out with the
  // actual device capabilities during Context::Init().
  void addDeviceExtension(const char* name, bool optional = false, void* pFeatureStruct = nullptr, uint32_t version = 0);

  void removeInstanceExtension(const char* name);
  void removeInstanceLayer(const char* name);
  void removeDeviceExtension(const char* name);

  // by default-constructor three queues are requested,
  // if you want more/different setups manipulate the requestedQueues vector
  // or use this function.
  void addRequestedQueue(VkQueueFlags flags, uint32_t count = 1, float priority = 1.0f);

  // this callback is run after extension and version related feature structs were queried for their support 
  // from the physical device and prior using them for device creation. It allows custom logic for disabling
  // certain features.
  // Be aware that enabling a feature is not legal within this function, only disabling.
  std::function<void(VkStructureType sType, void* pFeatureStruct)> fnDisableFeatures = nullptr;

  // Configure additional device creation with these variables and functions

  // use device groups
  bool useDeviceGroups = false;

  // which compatible device or device group to pick
  // only used by All-in-one Context::init(...)
  uint32_t compatibleDeviceIndex = 0;

  // instance properties
  std::string appEngine = "nvpro-sample";
  std::string appTitle  = "nvpro-sample";

  // may impact performance hence disable by default
  bool disableRobustBufferAccess = true;

  // Information printed at Context::init time
  bool verboseCompatibleDevices = true;
  bool verboseUsed              = true;  // Print what is used
  bool verboseAvailable         =        // Print what is available
#ifndef NDEBUG
      true;
#else
      false;
#endif

  // Will Enable GPU crash dumps when Aftermath is available.
  // No-op when Aftermath has not been made available via SUPPORT_AFTERMATH in CMakeLists.txt
  bool enableAftermath = true;

  struct Entry
  {
    Entry(const char* entryName, bool isOptional = false, void* pointerFeatureStruct = nullptr, uint32_t checkVersion = 0)
        : name(entryName)
        , optional(isOptional)
        , pFeatureStruct(pointerFeatureStruct)
        , version(checkVersion)
    {
    }

    std::string name;
    bool        optional{false};
    void*       pFeatureStruct{nullptr};
    uint32_t    version{0};
  };

  uint32_t apiMajor{1};
  uint32_t apiMinor{1};

  using EntryArray = std::vector<Entry>;
  EntryArray instanceLayers;
  EntryArray instanceExtensions;
  EntryArray deviceExtensions;
  void*      deviceCreateInfoExt{nullptr};
  void*      instanceCreateInfoExt{nullptr};

  struct QueueSetup
  {
    VkQueueFlags requiredFlags = 0;
    uint32_t     count         = 0;
    float        priority      = 1.0;
  };
  using QueueArray = std::vector<QueueSetup>;

  // this array defines how many queues are required for the provided queue flags
  // reset / add new entries if changes are desired
  //
  // ContextCreateInfo constructor adds 1 queue per default queue flag below
  QueueArray requestedQueues;

  // leave 0 and no default queue will be created
  VkQueueFlags defaultQueueGCT    = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
  VkQueueFlags defaultQueueT      = VK_QUEUE_TRANSFER_BIT;
  VkQueueFlags defaultQueueC      = VK_QUEUE_COMPUTE_BIT;
  float        defaultPriorityGCT = 1.0f;
  float        defaultPriorityT   = 1.0f;
  float        defaultPriorityC   = 1.0f;
};

//////////////////////////////////////////////////////////////////////////
/**
\class nvvk::Context

nvvk::Context class helps creating the Vulkan instance and to choose the logical device for the mandatory extensions. First is to fill the `ContextCreateInfo` structure, then call:

\code{.cpp}
  // Creating the Vulkan instance and device
  nvvk::ContextCreateInfo ctxInfo;
  ... see above ...

  nvvk::Context vkctx;
  vkctx.init(ctxInfo);

  // after init the ctxInfo is no longer needed
\endcode 

At this point, the class will have created the `VkInstance` and `VkDevice` according to the information passed. It will also keeps track or have query the information of:
 
* Physical Device information that you can later query : `PhysicalDeviceInfo` in which lots of `VkPhysicalDevice...` are stored
* `VkInstance` : the one instance being used for the program
* `VkPhysicalDevice` : physical device(s) used for the logical device creation. In case of more than one physical device, we have a std::vector for this purpose...
* `VkDevice` : the logical device instantiated
* `VkQueue` : By default, 3 queues are created, one per family: Graphic-Compute-Transfer, Compute and Transfer.
              For any additionnal queue, they need to be requested with `ContextCreateInfo::addRequestedQueue()`. This is creating information of the best suitable queues,
              but not creating them. To create the additional queues, 
              `Context::createQueue()` **must be call after** creating the Vulkan context.
              </br>The following queues are always created and can be directly accessed without calling createQueue :
   * `Queue m_queueGCT` : Graphics/Compute/Transfer Queue + family index
   * `Queue m_queueT` : async Transfer Queue + family index
   * `Queue m_queueC` : async Compute Queue + family index
* maintains what extensions are finally available
* implicitly hooks up the debug callback

## Choosing the device
When there are multiple devices, the `init` method is choosing the first compatible device available, but it is also possible the choose another one.
\code{.cpp}
  vkctx.initInstance(deviceInfo); 
  // Find all compatible devices
  auto compatibleDevices = vkctx.getCompatibleDevices(deviceInfo);
  assert(!compatibleDevices.empty());

  // Use first compatible device
  vkctx.initDevice(compatibleDevices[0], deviceInfo);
\endcode

## Multi-GPU

When multiple graphic cards should be used as a single device, the `ContextCreateInfo::useDeviceGroups` need to be set to `true`.
The above methods will transparently create the `VkDevice` using `VkDeviceGroupDeviceCreateInfo`.
Especially in the context of NVLink connected cards this is useful.

*/
class Context
{
public:
  Context(Context const&) = delete;
  Context& operator=(Context const&) = delete;

  Context() = default;

  // Vulkan == 1.1 used individual structs
  // Vulkan >= 1.2  have per-version structs
  struct Features11Old
  {
    VkPhysicalDeviceMultiviewFeatures    multiview{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES};
    VkPhysicalDevice16BitStorageFeatures t16BitStorage{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
    VkPhysicalDeviceSamplerYcbcrConversionFeatures samplerYcbcrConversion{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES};
    VkPhysicalDeviceProtectedMemoryFeatures protectedMemory{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES};
    VkPhysicalDeviceShaderDrawParameterFeatures drawParameters{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES};
    VkPhysicalDeviceVariablePointerFeatures variablePointers{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES};

    Features11Old()
    {
      multiview.pNext              = &t16BitStorage;
      t16BitStorage.pNext          = &samplerYcbcrConversion;
      samplerYcbcrConversion.pNext = &protectedMemory;
      protectedMemory.pNext        = &drawParameters;
      drawParameters.pNext         = &variablePointers;
      variablePointers.pNext       = nullptr;
    }

    void read(const VkPhysicalDeviceVulkan11Features& features11)
    {
      multiview.multiview                              = features11.multiview;
      multiview.multiviewGeometryShader                = features11.multiviewGeometryShader;
      multiview.multiviewTessellationShader            = features11.multiviewTessellationShader;
      t16BitStorage.storageBuffer16BitAccess           = features11.storageBuffer16BitAccess;
      t16BitStorage.storageInputOutput16               = features11.storageInputOutput16;
      t16BitStorage.storagePushConstant16              = features11.storagePushConstant16;
      t16BitStorage.uniformAndStorageBuffer16BitAccess = features11.uniformAndStorageBuffer16BitAccess;
      samplerYcbcrConversion.samplerYcbcrConversion    = features11.samplerYcbcrConversion;
      protectedMemory.protectedMemory                  = features11.protectedMemory;
      drawParameters.shaderDrawParameters              = features11.shaderDrawParameters;
      variablePointers.variablePointers                = features11.variablePointers;
      variablePointers.variablePointersStorageBuffer   = features11.variablePointersStorageBuffer;
    }

    void write(VkPhysicalDeviceVulkan11Features& features11)
    {
      features11.multiview                          = multiview.multiview;
      features11.multiviewGeometryShader            = multiview.multiviewGeometryShader;
      features11.multiviewTessellationShader        = multiview.multiviewTessellationShader;
      features11.storageBuffer16BitAccess           = t16BitStorage.storageBuffer16BitAccess;
      features11.storageInputOutput16               = t16BitStorage.storageInputOutput16;
      features11.storagePushConstant16              = t16BitStorage.storagePushConstant16;
      features11.uniformAndStorageBuffer16BitAccess = t16BitStorage.uniformAndStorageBuffer16BitAccess;
      features11.samplerYcbcrConversion             = samplerYcbcrConversion.samplerYcbcrConversion;
      features11.protectedMemory                    = protectedMemory.protectedMemory;
      features11.shaderDrawParameters               = drawParameters.shaderDrawParameters;
      features11.variablePointers                   = variablePointers.variablePointers;
      features11.variablePointersStorageBuffer      = variablePointers.variablePointersStorageBuffer;
    }
  };
  struct Properties11Old
  {
    VkPhysicalDeviceMaintenance3Properties maintenance3{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES};
    VkPhysicalDeviceIDProperties           deviceID{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
    VkPhysicalDeviceMultiviewProperties    multiview{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES};
    VkPhysicalDeviceProtectedMemoryProperties protectedMemory{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES};
    VkPhysicalDevicePointClippingProperties pointClipping{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES};
    VkPhysicalDeviceSubgroupProperties      subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};

    Properties11Old()
    {
      maintenance3.pNext    = &deviceID;
      deviceID.pNext        = &multiview;
      multiview.pNext       = &protectedMemory;
      protectedMemory.pNext = &pointClipping;
      pointClipping.pNext   = &subgroup;
      subgroup.pNext        = nullptr;
    }

    void write(VkPhysicalDeviceVulkan11Properties& properties11)
    {
      memcpy(properties11.deviceLUID, deviceID.deviceLUID, sizeof(properties11.deviceLUID));
      memcpy(properties11.deviceUUID, deviceID.deviceUUID, sizeof(properties11.deviceUUID));
      memcpy(properties11.driverUUID, deviceID.driverUUID, sizeof(properties11.driverUUID));
      properties11.deviceLUIDValid                   = deviceID.deviceLUIDValid;
      properties11.deviceNodeMask                    = deviceID.deviceNodeMask;
      properties11.subgroupSize                      = subgroup.subgroupSize;
      properties11.subgroupSupportedStages           = subgroup.supportedStages;
      properties11.subgroupSupportedOperations       = subgroup.supportedOperations;
      properties11.subgroupQuadOperationsInAllStages = subgroup.quadOperationsInAllStages;
      properties11.pointClippingBehavior             = pointClipping.pointClippingBehavior;
      properties11.maxMultiviewViewCount             = multiview.maxMultiviewViewCount;
      properties11.maxMultiviewInstanceIndex         = multiview.maxMultiviewInstanceIndex;
      properties11.protectedNoFault                  = protectedMemory.protectedNoFault;
      properties11.maxPerSetDescriptors              = maintenance3.maxPerSetDescriptors;
      properties11.maxMemoryAllocationSize           = maintenance3.maxMemoryAllocationSize;
    }
  };

  // This struct holds all core feature information for a physical device
  struct PhysicalDeviceInfo
  {
    VkPhysicalDeviceMemoryProperties     memoryProperties{};
    std::vector<VkQueueFamilyProperties> queueProperties;

    VkPhysicalDeviceFeatures         features10{};
    VkPhysicalDeviceVulkan11Features features11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};

    VkPhysicalDeviceProperties         properties10{};
    VkPhysicalDeviceVulkan11Properties properties11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
    VkPhysicalDeviceVulkan12Properties properties12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
    VkPhysicalDeviceVulkan13Properties properties13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
  };

  struct Queue
  {
    VkQueue  queue       = VK_NULL_HANDLE;
    uint32_t familyIndex = ~0U;
    uint32_t queueIndex  = ~0U;
    float    priority    = 1.0f;

    operator VkQueue() const { return queue; }
    operator uint32_t() const { return familyIndex; }
  };

  VkInstance         m_instance{VK_NULL_HANDLE};
  VkDevice           m_device{VK_NULL_HANDLE};
  VkPhysicalDevice   m_physicalDevice{VK_NULL_HANDLE};
  PhysicalDeviceInfo m_physicalInfo;
  uint32_t           m_apiMajor = 0;
  uint32_t           m_apiMinor = 0;

  // following queues are automatically created if appropriate ContextCreateInfo.defaultQueue??? is set
  // and ContextCreateInfo::requestedQueues contains a compatible config.
  Queue m_queueGCT;  // for Graphics/Compute/Transfer
  Queue m_queueT;    // for pure async Transfer Queue
  Queue m_queueC;    // for async Compute

  // additional queues must be created once through this function
  // returns new Queue and pops entry from available Queues that were requested via info.requestedQueues
  Queue createQueue(VkQueueFlags requiredFlags, const std::string& debugName, float priority = 1.0f);

  operator VkDevice() const { return m_device; }

  // All-in-one instance and device creation
  bool init(const ContextCreateInfo& info);
  void deinit();

  // Individual object creation
  bool initInstance(const ContextCreateInfo& info);
  // deviceIndex is an index either into getPhysicalDevices or getPhysicalDeviceGroups
  // depending on info.useDeviceGroups
  bool initDevice(uint32_t deviceIndex, const ContextCreateInfo& info);

  // Helpers
  std::vector<uint32_t>                        getCompatibleDevices(const ContextCreateInfo& info);
  std::vector<VkPhysicalDevice>                getPhysicalDevices();
  std::vector<VkPhysicalDeviceGroupProperties> getPhysicalDeviceGroups();
  std::vector<VkExtensionProperties>           getInstanceExtensions();
  std::vector<VkLayerProperties>               getInstanceLayers();
  std::vector<VkExtensionProperties>           getDeviceExtensions(VkPhysicalDevice physicalDevice);
  bool hasMandatoryExtensions(VkPhysicalDevice physicalDevice, const ContextCreateInfo& info, bool bVerbose);

  // Returns if GCTQueue supports present
  bool setGCTQueueWithPresent(VkSurfaceKHR surface);

  // true if the context has the optional extension activated
  bool hasDeviceExtension(const char* name) const;
  bool hasInstanceExtension(const char* name) const;

  void ignoreDebugMessage(int32_t msgID) { m_dbgIgnoreMessages.insert(msgID); }
  void setDebugSeverityFilterMask(int32_t severity) { m_dbgSeverity = severity; }


private:
  struct QueueScore
  {
    uint32_t score       = 0;  // the lower the score, the more 'specialized' it is
    uint32_t familyIndex = ~0U;
    uint32_t queueIndex  = ~0U;
    float    priority    = 1.0f;
  };
  using QueueScoreList = std::vector<QueueScore>;

  // This list is created from ContextCreateInfo::requestedQueues.
  // It contains the most specialized queues for compatible flags first.
  // Each Context::createQueue call finds a compatible item in this list
  // and removes it upon success.
  QueueScoreList m_availableQueues;

  // optional maxFamilyCounts overrides the device's max queue count per queue family
  // optional priorities overrides default priority 1.0 and must be sized physical device's queue family count * maxQueueCount
  void       initQueueList(QueueScoreList& list, const uint32_t* maxFamilyCounts, const float* priorities, uint32_t maxQueueCount) const;
  QueueScore removeQueueListItem(QueueScoreList& list, VkQueueFlags flags, float priority) const;

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                               VkDebugUtilsMessageTypeFlagsEXT        messageType,
                                                               const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                               void*                                       userData);

  std::vector<std::string> m_usedInstanceLayers;
  std::vector<std::string> m_usedInstanceExtensions;
  std::vector<std::string> m_usedDeviceExtensions;

  // New Debug system
  PFN_vkCreateDebugUtilsMessengerEXT  m_createDebugUtilsMessengerEXT  = nullptr;
  PFN_vkDestroyDebugUtilsMessengerEXT m_destroyDebugUtilsMessengerEXT = nullptr;
  VkDebugUtilsMessengerEXT            m_dbgMessenger                  = nullptr;

  std::unordered_set<int32_t> m_dbgIgnoreMessages;
  uint32_t m_dbgSeverity{VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT};

  // nSight Aftermath
  GpuCrashTracker m_gpuCrashTracker;

  void initDebugUtils();
  bool hasDebugUtils() const { return m_createDebugUtilsMessengerEXT != nullptr; }

  VkResult    fillFilteredNameArray(std::vector<std::string>&             used,
                                    const std::vector<VkLayerProperties>& properties,
                                    const ContextCreateInfo::EntryArray&  requested);
  VkResult    fillFilteredNameArray(std::vector<std::string>&                 used,
                                    const std::vector<VkExtensionProperties>& properties,
                                    const ContextCreateInfo::EntryArray&      requested,
                                    std::vector<void*>&                       featureStructs);
  bool        checkEntryArray(const std::vector<VkExtensionProperties>& properties, const ContextCreateInfo::EntryArray& requested, bool bVerbose);
  static void initPhysicalInfo(PhysicalDeviceInfo& info, VkPhysicalDevice physicalDevice, uint32_t versionMajor, uint32_t versionMinor);
};


}  // namespace nvvk

#endif
