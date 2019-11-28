/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <algorithm>
#include <nvh/nvprint.hpp>
#include <regex>

#include "context_vk.hpp"
#include "error_vk.hpp"
#include "extensions_vk.hpp"

#if _DEBUG
#include <nvvk/debug_util_vk.hpp>
static nvvk::DebugUtil s_debug;
#endif

namespace nvvk {
static std::string ObjectTypeToString(VkObjectType value)
{
  switch(value)
  {
    case VK_OBJECT_TYPE_UNKNOWN:
      return "Unknown";
    case VK_OBJECT_TYPE_INSTANCE:
      return "Instance";
    case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
      return "PhysicalDevice";
    case VK_OBJECT_TYPE_DEVICE:
      return "Device";
    case VK_OBJECT_TYPE_QUEUE:
      return "Queue";
    case VK_OBJECT_TYPE_SEMAPHORE:
      return "Semaphore";
    case VK_OBJECT_TYPE_COMMAND_BUFFER:
      return "CommandBuffer";
    case VK_OBJECT_TYPE_FENCE:
      return "Fence";
    case VK_OBJECT_TYPE_DEVICE_MEMORY:
      return "DeviceMemory";
    case VK_OBJECT_TYPE_BUFFER:
      return "Buffer";
    case VK_OBJECT_TYPE_IMAGE:
      return "Image";
    case VK_OBJECT_TYPE_EVENT:
      return "Event";
    case VK_OBJECT_TYPE_QUERY_POOL:
      return "QueryPool";
    case VK_OBJECT_TYPE_BUFFER_VIEW:
      return "BufferView";
    case VK_OBJECT_TYPE_IMAGE_VIEW:
      return "ImageView";
    case VK_OBJECT_TYPE_SHADER_MODULE:
      return "ShaderModule";
    case VK_OBJECT_TYPE_PIPELINE_CACHE:
      return "PipelineCache";
    case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
      return "PipelineLayout";
    case VK_OBJECT_TYPE_RENDER_PASS:
      return "RenderPass";
    case VK_OBJECT_TYPE_PIPELINE:
      return "Pipeline";
    case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
      return "DescriptorSetLayout";
    case VK_OBJECT_TYPE_SAMPLER:
      return "Sampler";
    case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
      return "DescriptorPool";
    case VK_OBJECT_TYPE_DESCRIPTOR_SET:
      return "DescriptorSet";
    case VK_OBJECT_TYPE_FRAMEBUFFER:
      return "Framebuffer";
    case VK_OBJECT_TYPE_COMMAND_POOL:
      return "CommandPool";
    case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:
      return "SamplerYcbcrConversion";
    case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:
      return "DescriptorUpdateTemplate";
    case VK_OBJECT_TYPE_SURFACE_KHR:
      return "SurfaceKHR";
    case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
      return "SwapchainKHR";
    case VK_OBJECT_TYPE_DISPLAY_KHR:
      return "DisplayKHR";
    case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
      return "DisplayModeKHR";
    case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
      return "DebugReportCallbackEXT";
    case VK_OBJECT_TYPE_OBJECT_TABLE_NVX:
      return "ObjectTableNVX";
    case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX:
      return "IndirectCommandsLayoutNVX";
    case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
      return "DebugUtilsMessengerEXT";
    case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:
      return "ValidationCacheEXT";
    case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV:
      return "AccelerationStructureNV";
    default:
      return "invalid";
  }
}


// Define a callback to capture the messages
VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                      VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                      void*                                       userData)
{
  int  level = LOGLEVEL_INFO;
  // repeating nvprintfLevel to help with breakpoints : so we can selectively break right after the print
  if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
  {
    nvprintfLevel(level, "VERBOSE: \n --> %s\n", callbackData->pMessageIdName, callbackData->pMessage);
  }
  else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
  {
    nvprintfLevel(level, "INFO: \n --> %s\n", callbackData->pMessageIdName, callbackData->pMessage);
  }
  else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    level = LOGLEVEL_WARNING;
    nvprintfLevel(level, "WARNING: \n --> %s\n", callbackData->pMessageIdName, callbackData->pMessage);
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    level = LOGLEVEL_ERROR;
    nvprintfLevel(level, "ERROR: %s \n --> %s", callbackData->pMessageIdName, callbackData->pMessage);
    nvprintfLevel(level, "\n"); // placeholder for breakpoint
  }
  else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
  {
    nvprintfLevel(level, "GENERAL: %s \n --> %s\n", callbackData->pMessageIdName, callbackData->pMessage);
  }
  else 
  {
    nvprintfLevel(level, "%s \n --> %s\n", callbackData->pMessageIdName, callbackData->pMessage);
  }

  if(callbackData->objectCount > 0)
  {
    for(uint32_t object = 0; object < callbackData->objectCount; ++object)
    {
      std::string otype = ObjectTypeToString(callbackData->pObjects[object].objectType);
      LOGI(" Object[%d] - Type %s, Value %p, Name \"%s\"\n", object, otype.c_str(),
           (void*)(callbackData->pObjects[object].objectHandle), callbackData->pObjects[object].pObjectName);
    }
  }
  if(callbackData->cmdBufLabelCount > 0)
  {
    for(uint32_t label = 0; label < callbackData->cmdBufLabelCount; ++label)
    {
      LOGI(" Label[%d] - %s { %f, %f, %f, %f}\n", label, callbackData->pCmdBufLabels[label].pLabelName,
           callbackData->pCmdBufLabels[label].color[0], callbackData->pCmdBufLabels[label].color[1],
           callbackData->pCmdBufLabels[label].color[2], callbackData->pCmdBufLabels[label].color[3]);
    }
  }
  // Don't bail out, but keep going.
  return VK_FALSE;
}


//--------------------------------------------------------------------------------------------------
// Create the Vulkan instance and then first compatible device based on \p info
//
bool Context::init(const ContextCreateInfo& info)
{
  if(!initInstance(info))
    return false;

  // Find all compatible devices
  auto compatibleDevices = getCompatibleDevices(info);
  if (compatibleDevices.empty())
  {
    assert(!"No compatible device found");
    return false;
  }

  // Use a compatible device
  return initDevice(compatibleDevices[info.compatibleDeviceIndex], info);
}

//--------------------------------------------------------------------------------------------------
// Create the Vulkan instance
//
bool Context::initInstance(const ContextCreateInfo& info)
{
  VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  applicationInfo.pApplicationName = info.appTitle;
  applicationInfo.pEngineName      = info.appEngine;
  applicationInfo.apiVersion       = VK_MAKE_VERSION(info.apiMajor, info.apiMinor, 0);

  uint32_t count = 0;

  {
    // Get all layers
    auto layerProperties = getInstanceLayers();

    if(fillFilteredNameArray(m_usedInstanceLayers, layerProperties, info.instanceLayers) != VK_SUCCESS)
    {
      return false;
    }

    if(info.verboseAvailable)
    {
      LOGI("___________________________\n");
      LOGI("Available Instance Layers :\n");
      for(auto it : layerProperties)
      {
        LOGI("%s (v. %x %x) : %s\n", it.layerName, it.specVersion, it.implementationVersion, it.description);
      }
    }
  }

  {
    // Get all extensions
    auto extensionProperties = getInstanceExtensions();

    std::vector<void*> featureStructs;
    if(fillFilteredNameArray(m_usedInstanceExtensions, extensionProperties, info.instanceExtensions, featureStructs) != VK_SUCCESS)
    {
      return false;
    }

    if(info.verboseAvailable)
    {
      LOGI("\n");
      LOGI("Available Instance Extensions :\n");
      for(auto it : extensionProperties)
      {
        LOGI("%s (v. %d)\n", it.extensionName, it.specVersion);
      }
    }
  }


  if(info.verboseUsed)
  {
    LOGI("______________________\n");
    LOGI("Used Instance Layers :\n");
    for(auto it : m_usedInstanceLayers)
    {
      LOGI("%s\n", it);
    }
    LOGI("\n");
    LOGI("Used Instance Extensions :\n");
    for(auto it : m_usedInstanceExtensions)
    {
      LOGI("%s\n", it);
    }
  }
  
  VkInstanceCreateInfo instanceCreateInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instanceCreateInfo.pApplicationInfo        = &applicationInfo;
  instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(m_usedInstanceExtensions.size());
  instanceCreateInfo.ppEnabledExtensionNames = m_usedInstanceExtensions.data();
  instanceCreateInfo.enabledLayerCount       = static_cast<uint32_t>(m_usedInstanceLayers.size());
  instanceCreateInfo.ppEnabledLayerNames     = m_usedInstanceLayers.data();

  NVVK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

  for(const auto& it : m_usedInstanceExtensions)
  {
    if(strcmp(it, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
    {
      initDebugReport();
      break;
    }
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
// Create Vulkan device
// \p deviceIndex is the index from the list of getPhysicalDevices/getPhysicalDeviceGroups
bool Context::initDevice(uint32_t deviceIndex, const ContextCreateInfo& info)
{
  assert(m_instance != nullptr);

  VkPhysicalDeviceGroupProperties physicalGroup;
  if(info.useDeviceGroups)
  {
    auto groups = getPhysicalDeviceGroups();
    assert(deviceIndex < static_cast<uint32_t>(groups.size()));
    physicalGroup    = groups[deviceIndex];
    m_physicalDevice = physicalGroup.physicalDevices[0];
  }
  else
  {
    auto physicalDevices = getPhysicalDevices();
    assert(deviceIndex < static_cast<uint32_t>(physicalDevices.size()));
    m_physicalDevice = physicalDevices[deviceIndex];
  }

  initPhysicalInfo(m_physicalInfo, m_physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::vector<float>                   priorities;
  std::vector<void*>                   featureStructs;

  bool queueFamilyGraphicsCompute = false;
  {
    for(auto& it : m_physicalInfo.queueProperties)
    {
      if(it.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))
      {
        queueFamilyGraphicsCompute = true;
      }
      if(it.queueCount > priorities.size())
      {
        // set all priorities equal
        priorities.resize(it.queueCount, 1.0f);
      }
    }
    for(uint32_t i = 0; i < m_physicalInfo.queueProperties.size(); ++i)
    {
      VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
      queueInfo.queueFamilyIndex = i;
      queueInfo.queueCount       = m_physicalInfo.queueProperties[i].queueCount;
      queueInfo.pQueuePriorities = priorities.data();

      queueCreateInfos.push_back(queueInfo);
    }
  }

  if(!queueFamilyGraphicsCompute)
  {
    LOGW("could not find queue that supports graphics and compute");
  }

  // allow all queues
  VkDeviceCreateInfo deviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
  deviceCreateInfo.pQueueCreateInfos    = queueCreateInfos.data();

  // physical device extensions
  auto extensionProperties = getDeviceExtensions(m_physicalDevice);

  if(info.verboseAvailable)
  {
    LOGI("_____________________________\n");
    LOGI("Available Device Extensions :\n");
    for(auto it : extensionProperties)
    {
      LOGI("%s (v. %d)\n", it.extensionName, it.specVersion);
    }
  }

  if(fillFilteredNameArray(m_usedDeviceExtensions, extensionProperties, info.deviceExtensions, featureStructs) != VK_SUCCESS)
  {
    deinit();
    return false;
  }

  if(info.verboseUsed)
  {
    LOGI("________________________\n");
    LOGI("Used Device Extensions :\n");
    for(auto it : m_usedDeviceExtensions)
    {
      LOGI("%s\n", it);
    }
    LOGI("\n");
  }

  deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(m_usedDeviceExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames = m_usedDeviceExtensions.data();

  // Vulkan >= 1.1 uses pNext to enable features, and not pEnabledFeatures
  deviceCreateInfo.pEnabledFeatures = nullptr;
  deviceCreateInfo.pNext            = &m_physicalInfo.features2;


  struct ExtensionHeader  // Helper struct to link extensions together
  {
    VkStructureType sType;
    void*           pNext;
  };

  // use the coreFeature chain to append extensions
  ExtensionHeader* lastCoreFeature = nullptr;
  if(!featureStructs.empty())
  {
    // build up chain of all used extension features
    for(size_t i = 0; i < featureStructs.size(); i++)
    {
      auto* header  = reinterpret_cast<ExtensionHeader*>(featureStructs[i]);
      header->pNext = i < featureStructs.size() - 1 ? featureStructs[i + 1] : nullptr;
    }

    // append to the end of current feature2 struct
    lastCoreFeature = (ExtensionHeader*)&m_physicalInfo.features2;
    while(lastCoreFeature->pNext != nullptr)
    {
      lastCoreFeature = (ExtensionHeader*)lastCoreFeature->pNext;
    }
    lastCoreFeature->pNext = featureStructs[0];

    // query support
    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &m_physicalInfo.features2);
  }

  // disable some features
  if(info.disableRobustBufferAccess)
  {
    m_physicalInfo.features2.features.robustBufferAccess = VK_FALSE;
  }

  // device group information
  VkDeviceGroupDeviceCreateInfo deviceGroupCreateInfo{VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO};
  if(info.useDeviceGroups)
  {
    // add ourselves to the chain
    deviceGroupCreateInfo.pNext               = deviceCreateInfo.pNext;
    deviceGroupCreateInfo.physicalDeviceCount = uint32_t(physicalGroup.physicalDeviceCount);
    deviceGroupCreateInfo.pPhysicalDevices    = physicalGroup.physicalDevices;
    deviceCreateInfo.pNext                    = &deviceGroupCreateInfo;
  }

  ExtensionHeader* deviceCreateChain = nullptr;
  if (info.deviceCreateInfoExt) {
    deviceCreateChain = (ExtensionHeader*)info.deviceCreateInfoExt;
    while(deviceCreateChain->pNext != nullptr)
    {
      deviceCreateChain = (ExtensionHeader*)deviceCreateChain->pNext;
    }
    // override last of external chain
    deviceCreateChain->pNext = (void*)deviceCreateInfo.pNext;
    deviceCreateInfo.pNext = info.deviceCreateInfoExt;
  }

  VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);

  if (deviceCreateChain)
  {
    // reset last of external chain
    deviceCreateChain->pNext = nullptr;
  }

  if(result != VK_SUCCESS)
  {
    deinit();
    return false;
  }

  // reset core feature chain
  if(lastCoreFeature)
  {
    lastCoreFeature->pNext = nullptr;
  }

  load_VK_EXTENSION_SUBSET(m_instance, vkGetInstanceProcAddr, m_device, vkGetDeviceProcAddr);

  // Now we have the device and instance, we can initialize the debug tool
#if _DEBUG
  s_debug.setup(m_device);
#endif
  // get some default queues
  uint32_t queueFamilyIndex = 0;
  for(auto& it : m_physicalInfo.queueProperties)
  {
    if((it.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))
       == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))
    {
      vkGetDeviceQueue(m_device, queueFamilyIndex, 0 /*queueIndex*/, &m_queueGCT.queue);
      m_queueGCT.familyIndex = queueFamilyIndex;
#if _DEBUG
      s_debug.setObjectName(m_queueGCT.queue, "queueGCT");
#endif
    }
    else if((it.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
    {
      vkGetDeviceQueue(m_device, queueFamilyIndex, 0 /*queueIndex*/, &m_queueT.queue);
      m_queueT.familyIndex = queueFamilyIndex;
#if _DEBUG
      s_debug.setObjectName(m_queueT.queue, "queueT");
#endif
    }
    else if((it.queueFlags & VK_QUEUE_COMPUTE_BIT))
    {
      vkGetDeviceQueue(m_device, queueFamilyIndex, 0 /*queueIndex*/, &m_queueC.queue);
      m_queueC.familyIndex = queueFamilyIndex;
#if _DEBUG
      s_debug.setObjectName(m_queueC.queue, "queueC");
#endif
    }

    queueFamilyIndex++;
  }

  return true;
}


//--------------------------------------------------------------------------------------------------
// Set the queue family index compatible with the \p surface
//
bool Context::setGCTQueueWithPresent(VkSurfaceKHR surface)
{
  VkQueueFlags bits = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;

  for(uint32_t i = 0; i < uint32_t(m_physicalInfo.queueProperties.size()); i++)
  {
    VkBool32 supportsPresent;
    vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, surface, &supportsPresent);

    if((supportsPresent == VK_TRUE) && ((m_physicalInfo.queueProperties[i].queueFlags & bits) == bits))
    {
      vkGetDeviceQueue(m_device, i, 0, &m_queueGCT.queue);
      m_queueGCT.familyIndex = i;
#if _DEBUG
      s_debug.setObjectName(m_queueGCT.queue, "queueGCT");
#endif

      return true;
    }
  }

  return false;
}

// #UNSUED
uint32_t Context::getQueueFamily(VkQueueFlags flagsSupported, VkQueueFlags flagsDisabled, VkSurfaceKHR surface)
{
  uint32_t queueFamilyIndex = 0;
  for(auto& it : m_physicalInfo.queueProperties)
  {
    VkBool32 supportsPresent = VK_TRUE;
    if(surface)
    {
      supportsPresent = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, queueFamilyIndex, surface, &supportsPresent);
    }

    if(supportsPresent && ((it.queueFlags & flagsSupported) == flagsSupported) && ((it.queueFlags & flagsDisabled) == 0))
    {
      return queueFamilyIndex;
    }

    queueFamilyIndex++;
  }

  return ~0;
}

//--------------------------------------------------------------------------------------------------
// Destructor
//
void Context::deinit()
{
  if(m_device)
  {
    VkResult  result = vkDeviceWaitIdle(m_device);
    if (nvvk::checkResult(result, __FILE__, __LINE__)) {
      exit(-1);
    }

    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
  }
  if(m_destroyDebugUtilsMessengerEXT)
  {
    // Destroy the Debug Utils Messenger
    m_destroyDebugUtilsMessengerEXT(m_instance, m_dbgMessenger, nullptr);
  }

  if(m_instance)
  {
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
  }

  m_usedInstanceExtensions.clear();
  m_usedInstanceLayers.clear();
  m_usedDeviceExtensions.clear();

  m_createDebugUtilsMessengerEXT  = nullptr;
  m_destroyDebugUtilsMessengerEXT = nullptr;
  m_dbgMessenger                  = nullptr;

  reset_VK_EXTENSION_SUBSET();
}

bool Context::hasDeviceExtension(const char* name) const
{
  for(const auto& usedDeviceExtension : m_usedDeviceExtensions)
  {
    if(strcmp(name, usedDeviceExtension) == 0)
    {
      return true;
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------------
//
//
ContextCreateInfo::ContextCreateInfo(bool bUseValidation)
{
  static VkPhysicalDeviceHostQueryResetFeaturesEXT s_physicalHostQueryResetFeaturesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT};
  deviceExtensions.push_back({VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,true, &s_physicalHostQueryResetFeaturesEXT});
#ifdef _DEBUG
  instanceExtensions.push_back({VK_EXT_DEBUG_UTILS_EXTENSION_NAME, true});
  instanceExtensions.push_back({VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true});
  deviceExtensions.push_back({VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true});
  if(bUseValidation)
    instanceLayers.push_back({"VK_LAYER_KHRONOS_validation", true});
#endif
}

void ContextCreateInfo::addInstanceExtension(const char* name, bool optional)
{
  instanceExtensions.emplace_back(name, optional);
}

void ContextCreateInfo::addInstanceLayer(const char* name, bool optional)
{
  instanceLayers.emplace_back(name, optional);
}

//--------------------------------------------------------------------------------------------------
// pFeatureStruct must be provided if it exists, it will be queried from physical device
// and then passed in this state to device create info.
//
void ContextCreateInfo::addDeviceExtension(const char* name, bool optional, void* pFeatureStruct)
{
  deviceExtensions.emplace_back(name, optional, pFeatureStruct);
}

void ContextCreateInfo::removeInstanceExtension(const char* name)
{
  for(size_t i = 0; i < instanceExtensions.size(); i++)
  {
    if(strcmp(instanceExtensions[i].name, name) == 0)
    {
      instanceExtensions.erase(instanceExtensions.begin() + i);
    }
  }
}

void ContextCreateInfo::removeInstanceLayer(const char* name)
{
  for(size_t i = 0; i < instanceLayers.size(); i++)
  {
    if(strcmp(instanceLayers[i].name, name) == 0)
    {
      instanceLayers.erase(instanceLayers.begin() + i);
    }
  }
}

void ContextCreateInfo::removeDeviceExtension(const char* name)
{
  for(size_t i = 0; i < deviceExtensions.size(); i++)
  {
    if(strcmp(deviceExtensions[i].name, name) == 0)
    {
      deviceExtensions.erase(deviceExtensions.begin() + i);
    }
  }
}

void ContextCreateInfo::setVersion(int major, int minor)
{
  assert(apiMajor >= 1 && apiMinor >= 1);
  apiMajor = major;
  apiMinor = minor;
}

VkResult Context::fillFilteredNameArray(Context::NameArray&                   used,
                                        const std::vector<VkLayerProperties>& properties,
                                        const ContextCreateInfo::EntryArray&  requested)
{
  for(auto itr = requested.begin(); itr != requested.end(); ++itr)
  {
    bool found = false;
    for(const auto& propertie : properties)
    {
      if(strcmp(itr->name, propertie.layerName) == 0)
      {
        found = true;
        break;
      }
    }

    if(!found && !itr->optional)
    {
      LOGW("VK_ERROR_EXTENSION_NOT_PRESENT: %s\n", itr->name);
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    used.push_back(itr->name);
  }

  return VK_SUCCESS;
}


VkResult Context::fillFilteredNameArray(Context::NameArray&                       used,
                                        const std::vector<VkExtensionProperties>& properties,
                                        const ContextCreateInfo::EntryArray&      requested,
                                        std::vector<void*>&                       featureStructs)
{
  for(const auto& itr : requested)
  {
    bool found = false;
    for(const auto& propertie : properties)
    {
      if(strcmp(itr.name, propertie.extensionName) == 0)
      {
        found = true;
        break;
      }
    }

    if(found)
    {
      used.push_back(itr.name);
      if(itr.pFeatureStruct)
      {
        featureStructs.push_back(itr.pFeatureStruct);
      }
    }
    else if(!itr.optional)
    {
      LOGW("VK_ERROR_EXTENSION_NOT_PRESENT: %s\n", itr.name);
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
  }

  return VK_SUCCESS;
}

void Context::initPhysicalInfo(PhysicalDeviceInfo& info, VkPhysicalDevice physicalDevice)
{
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &info.memoryProperties);
  uint32_t count;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
  info.queueProperties.resize(count);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, info.queueProperties.data());

  info.features2.sType                           = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  info.coreFeatures.multiview.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
  info.coreFeatures.t16BitStorage.sType          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
  info.coreFeatures.samplerYcbcrConversion.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES;
  info.coreFeatures.protectedMemory.sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES;
  info.coreFeatures.drawParameters.sType         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;
  info.coreFeatures.variablePointers.sType       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES;

  info.features2.pNext                           = &info.coreFeatures.multiview;
  info.coreFeatures.multiview.pNext              = &info.coreFeatures.t16BitStorage;
  info.coreFeatures.t16BitStorage.pNext          = &info.coreFeatures.samplerYcbcrConversion;
  info.coreFeatures.samplerYcbcrConversion.pNext = &info.coreFeatures.protectedMemory;
  info.coreFeatures.protectedMemory.pNext        = &info.coreFeatures.drawParameters;
  info.coreFeatures.drawParameters.pNext         = &info.coreFeatures.variablePointers;
  info.coreFeatures.variablePointers.pNext       = nullptr;

  VkPhysicalDeviceProperties2 properties2;

  properties2.sType                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  info.coreProperties.maintenance3.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
  info.coreProperties.deviceID.sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
  info.coreProperties.multiview.sType       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;
  info.coreProperties.protectedMemory.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES;
  info.coreProperties.pointClipping.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES;
  info.coreProperties.subgroup.sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;

  properties2.pNext                         = &info.coreProperties.maintenance3;
  info.coreProperties.maintenance3.pNext    = &info.coreProperties.deviceID;
  info.coreProperties.deviceID.pNext        = &info.coreProperties.multiview;
  info.coreProperties.multiview.pNext       = &info.coreProperties.protectedMemory;
  info.coreProperties.protectedMemory.pNext = &info.coreProperties.pointClipping;
  info.coreProperties.pointClipping.pNext   = &info.coreProperties.subgroup;
  info.coreProperties.subgroup.pNext        = nullptr;

  vkGetPhysicalDeviceFeatures2(physicalDevice, &info.features2);
  vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);

  info.properties = properties2.properties;
}

void Context::initDebugReport()
{
  // Debug reporting system
  // Setup our pointers to the VK_EXT_debug_utils commands
  m_createDebugUtilsMessengerEXT =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
  m_destroyDebugUtilsMessengerEXT =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
  // Create a Debug Utils Messenger that will trigger our callback for any warning
  // or error.
  if(m_createDebugUtilsMessengerEXT != nullptr)
  {
    VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info;
    dbg_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_messenger_create_info.pNext = nullptr;
    dbg_messenger_create_info.flags = 0;
    dbg_messenger_create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbg_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbg_messenger_create_info.pfnUserCallback = debugMessengerCallback;
    dbg_messenger_create_info.pUserData       = nullptr;
    VkResult result = m_createDebugUtilsMessengerEXT(m_instance, &dbg_messenger_create_info, nullptr, &m_dbgMessenger);
  }
}

//--------------------------------------------------------------------------------------------------
// Returns the list of devices or groups compatible with the mandatory extensions
//
std::vector<int> Context::getCompatibleDevices(const ContextCreateInfo& info)
{
  assert(m_instance != nullptr);
  std::vector<int>                             compatibleDevices;
  std::vector<VkPhysicalDeviceGroupProperties> groups;
  std::vector<VkPhysicalDevice>                physicalDevices;

  uint32_t nbElems;
  if(info.useDeviceGroups)
  {
    groups  = getPhysicalDeviceGroups();
    nbElems = static_cast<uint32_t>(groups.size());
  }
  else
  {
    physicalDevices = getPhysicalDevices();
    nbElems         = static_cast<uint32_t>(physicalDevices.size());
  }

  if(info.verboseCompatibleDevices)
  {
    LOGI("____________________\n");
    LOGI("Compatible Devices :\n");
  }

  uint32_t compatible = 0;
  for(uint32_t elemId = 0; elemId < nbElems; elemId++)
  {
    VkPhysicalDevice physicalDevice = info.useDeviceGroups ? groups[elemId].physicalDevices[0] : physicalDevices[elemId];

    // Note: all physical devices in a group are identical
    if(hasMandatoryExtensions(physicalDevice, info, info.verboseCompatibleDevices))
    {
      compatibleDevices.push_back(elemId);
      if(info.verboseCompatibleDevices)
      {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevice, &props);
        LOGI("%d: %s\n", compatible, props.deviceName);
        compatible++;
      }
    }
    else if (info.verboseCompatibleDevices)
    {
      VkPhysicalDeviceProperties props;
      vkGetPhysicalDeviceProperties(physicalDevice, &props);
      LOGW("Skipping physical device %s\n", props.deviceName);
    }
  }
  if(info.verboseCompatibleDevices)
  {
    LOGI("Physical devices found : ", compatible);
    if (compatible > 0)
    {
      LOGI("%d\n", compatible);
    }
    else
    {
      LOGI("OMG... NONE !!\n", compatible);
    }
  }

  return compatibleDevices;
}

//--------------------------------------------------------------------------------------------------
// Return true if all extensions in info, marked as required are available on the physicalDevice
//
bool Context::hasMandatoryExtensions(VkPhysicalDevice physicalDevice, const ContextCreateInfo& info, bool bVerbose)
{
  std::vector<VkExtensionProperties> extensionProperties;

  uint32_t count;
  NVVK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr));
  extensionProperties.resize(count);
  NVVK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensionProperties.data()));

  return checkEntryArray(extensionProperties, info.deviceExtensions, bVerbose);
}

bool Context::checkEntryArray(const std::vector<VkExtensionProperties>& properties, const ContextCreateInfo::EntryArray& requested, bool bVerbose)
{
  for(const auto& itr : requested)
  {
    bool found = false;
    for(const auto& property : properties)
    {
      if(strcmp(itr.name, property.extensionName) == 0)
      {
        found = true;
        break;
      }
    }

    if(!found && !itr.optional)
    {
      if (bVerbose)
      {
        LOGW("Could NOT locate mandatory extension '%s'\n", itr.name);
      }
      return false;
    }
  }

  return true;
}

std::vector<VkPhysicalDevice> Context::getPhysicalDevices()
{
  uint32_t                      nbElems;
  std::vector<VkPhysicalDevice> physicalDevices;
  NVVK_CHECK(vkEnumeratePhysicalDevices(m_instance, &nbElems, nullptr));
  physicalDevices.resize(nbElems);
  NVVK_CHECK(vkEnumeratePhysicalDevices(m_instance, &nbElems, physicalDevices.data()));
  return physicalDevices;
}

std::vector<VkPhysicalDeviceGroupProperties> Context::getPhysicalDeviceGroups()
{
  uint32_t                                     deviceGroupCount;
  std::vector<VkPhysicalDeviceGroupProperties> groups;
  NVVK_CHECK(vkEnumeratePhysicalDeviceGroups(m_instance, &deviceGroupCount, nullptr));
  groups.resize(deviceGroupCount);
  NVVK_CHECK(vkEnumeratePhysicalDeviceGroups(m_instance, &deviceGroupCount, groups.data()));
  return groups;
}

std::vector<VkLayerProperties> Context::getInstanceLayers()
{
  uint32_t                       count;
  std::vector<VkLayerProperties> layerProperties;
  NVVK_CHECK(vkEnumerateInstanceLayerProperties(&count, nullptr));
  layerProperties.resize(count);
  NVVK_CHECK(vkEnumerateInstanceLayerProperties(&count, layerProperties.data()));
  return layerProperties;
}

std::vector<VkExtensionProperties> Context::getInstanceExtensions()
{
  uint32_t                           count;
  std::vector<VkExtensionProperties> extensionProperties;
  NVVK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
  extensionProperties.resize(count);
  NVVK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensionProperties.data()));
  return extensionProperties;
}

std::vector<VkExtensionProperties> Context::getDeviceExtensions(VkPhysicalDevice physicalDevice)
{
  uint32_t                           count;
  std::vector<VkExtensionProperties> extensionProperties;
  NVVK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr));
  extensionProperties.resize(count);
  NVVK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensionProperties.data()));
  return extensionProperties;
}

}  // namespace nvvk
