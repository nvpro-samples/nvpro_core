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

#include <windows.h>

#include <algorithm>
#include <cassert>
#include <nvh/nvprint.hpp>
#include <regex>

#include "context_vkpp.hpp"
#include "utilities_vkpp.hpp"

// See: https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;
static_assert(VK_HEADER_VERSION >= 126, "Vulkan version need 1.1.126.0 or greater");

namespace nvvkpp {

// Define a callback to capture the messages
VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                      VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                      void*                                       userData)
{

  int level = LOGLEVEL_INFO;
  if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    level = LOGLEVEL_WARNING;
  }
  else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    level = LOGLEVEL_ERROR;
  }

  std::string prefix = vk::to_string(vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity));
  nvprintfLevel(level, "%s: %s \n --> %s\n", prefix.c_str(), callbackData->pMessageIdName, callbackData->pMessage);

  if(callbackData->objectCount > 0)
  {
    for(uint32_t object = 0; object < callbackData->objectCount; ++object)
    {
      std::string otype = vk::to_string(vk::ObjectType(callbackData->pObjects[object].objectType));
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
bool Context::init(const ContextCreateInfo& contextInfo)
{
  if(!initInstance(contextInfo))
    return false;

  // Find all compatible devices
  auto compatibleDevices = getCompatibleDevices(contextInfo);
  assert(!compatibleDevices.empty());

  // Use a compatible device
  return initDevice(compatibleDevices[contextInfo.compatibleDeviceIndex], contextInfo);
}

//--------------------------------------------------------------------------------------------------
// Create the Vulkan instance
//
bool Context::initInstance(const ContextCreateInfo& info)
{
  vk::DynamicLoader         dl;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
      dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
  VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

  if(info.verboseAvailable)
    LOGI("Initializing Vulkan Instance\n");

  std::vector<void*> dummy;

  // Get all layers
  auto layerProperties = vk::enumerateInstanceLayerProperties();
  if(fillFilteredNameArray(m_usedInstanceLayers, layerProperties, info.instanceLayers) != VK_SUCCESS)
  {
    LOGE("Error: Required layer not present\n");
    return false;
  }

  // Get all extensions
  auto extensionProperties = vk::enumerateInstanceExtensionProperties();
  if(fillFilteredNameArray(m_usedInstanceExtensions, extensionProperties, info.instanceExtensions, dummy) != VK_SUCCESS)
  {
    LOGE("Error: No extensions properties\n");
    return false;
  }

  if(info.verboseAvailable)
  {
    printAllLayers();
    printAllExtensions();
  }

  if(info.verboseUsed)
  {
    printLayersExtensionsUsed();
  }

  vk::ApplicationInfo applicationInfo;
  applicationInfo.pApplicationName = info.appTitle;
  applicationInfo.pEngineName      = info.appEngine;
  applicationInfo.apiVersion       = VK_MAKE_VERSION(info.apiMajor, info.apiMinor, 0);

  vk::InstanceCreateInfo instanceCreateInfo;
  instanceCreateInfo.pApplicationInfo        = &applicationInfo;
  instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(m_usedInstanceExtensions.size());
  instanceCreateInfo.ppEnabledExtensionNames = m_usedInstanceExtensions.data();
  instanceCreateInfo.enabledLayerCount       = static_cast<uint32_t>(m_usedInstanceLayers.size());
  instanceCreateInfo.ppEnabledLayerNames     = m_usedInstanceLayers.data();

#ifdef _DEBUG
  //std::vector<vk::ValidationFeatureEnableEXT> vfe{vk::ValidationFeatureEnableEXT::eGpuAssisted,
  //                                                vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot,
  //                                                vk::ValidationFeatureEnableEXT::eBestPractices};
  //vk::ValidationFeaturesEXT                   validationFeatures;
  //validationFeatures.setEnabledValidationFeatureCount(static_cast<uint32_t>(vfe.size()));
  //validationFeatures.setPEnabledValidationFeatures(vfe.data());
  //instanceCreateInfo.setPNext(&validationFeatures);
#endif  // _DEBUG

  if(info.verboseAvailable)
    LOGI("Creating Vulkan instance\n");
  m_instance = vk::createInstance(instanceCreateInfo);

  if(!m_instance)
  {
    LOGE("Error: failed creating the instance\n");
    return false;
  }


  // Initializing dynamic Vulkan functions
  if(info.verboseAvailable)
    LOGI("Initializing Vulkan extra functions\n");
  VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

  // If one of the extension has Debug, initialize the Debug callback
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

void Context::printAllLayers()
{
  LOGI("___________________________\n");
  LOGI("Available Instance Layers :\n");
  for(const auto& it : vk::enumerateInstanceLayerProperties())
  {
    LOGI("%s (v. %x %x) : %s\n", it.layerName, it.specVersion, it.implementationVersion, it.description);
  }
}

//--------------------------------------------------------------------------------------------------
// Create Vulkan device
// \p deviceIndex is the index from the list of getPhysicalDevices/getPhysicalDeviceGroups
bool Context::initDevice(uint32_t deviceIndex, const ContextCreateInfo& info)
{
  assert(m_instance);

  vk::PhysicalDeviceGroupProperties physicalGroup;

  if(info.useDeviceGroups)
  {
    auto groups = m_instance.enumeratePhysicalDeviceGroups();
    assert(deviceIndex < static_cast<uint32_t>(groups.size()));
    physicalGroup    = groups[deviceIndex];
    m_physicalDevice = physicalGroup.physicalDevices[0];
  }
  else
  {
    auto physicalDevices = m_instance.enumeratePhysicalDevices();
    assert(deviceIndex < static_cast<uint32_t>(physicalDevices.size()));
    m_physicalDevice = physicalDevices[deviceIndex];
  }

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::vector<float>                     priorities;
  std::vector<void*>                     featureStructs;

  bool queueFamilyGraphicsCompute = false;
  auto queueFamilyProperties      = m_physicalDevice.getQueueFamilyProperties();
  {
    for(auto& it : queueFamilyProperties)
    {

      if(it.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer))
      {
        queueFamilyGraphicsCompute = true;
      }
      if(it.queueCount > priorities.size())
      {
        // set all priorities equal
        priorities.resize(it.queueCount, 1.0f);
      }
    }

    for(uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
    {
      vk::DeviceQueueCreateInfo queueInfo;
      queueInfo.queueFamilyIndex = i;
      queueInfo.queueCount       = queueFamilyProperties[i].queueCount;
      queueInfo.pQueuePriorities = priorities.data();

      queueCreateInfos.push_back(queueInfo);
    }
  }

  if(!queueFamilyGraphicsCompute)
  {
    LOGW("could not find queue that supports graphics and compute");
  }

  // allow all queues
  vk::DeviceCreateInfo deviceCreateInfo;
  deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
  deviceCreateInfo.pQueueCreateInfos    = queueCreateInfos.data();

  // physical device extensions
  auto extensionProperties = m_physicalDevice.enumerateDeviceExtensionProperties();

  if(info.verboseAvailable)
  {
    LOGI("_____________________________\n");
    LOGI("Available Device Extensions :\n");
    for(auto it : extensionProperties)
    {
      LOGI("%s (v. %d)\n", it.extensionName, it.specVersion);
    }
  }

  m_usedDeviceExtensions.clear();
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
  vk::PhysicalDeviceFeatures2 enabledFeatures2{};  //
  deviceCreateInfo.pEnabledFeatures = nullptr;
  deviceCreateInfo.pNext            = &enabledFeatures2;


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
    lastCoreFeature = (ExtensionHeader*)&enabledFeatures2;
    while(lastCoreFeature->pNext != nullptr)
    {
      lastCoreFeature = (ExtensionHeader*)lastCoreFeature->pNext;
    }
    lastCoreFeature->pNext = featureStructs[0];
  }
  // query support
  // Request core features (warning otherwise)
  enabledFeatures2.features = m_physicalDevice.getFeatures();

  // Enabling features
  m_physicalDevice.getFeatures2(&enabledFeatures2);

  // disable some features
  if(info.disableRobustBufferAccess)
  {
    enabledFeatures2.features.robustBufferAccess = VK_FALSE;
  }

  // device group information
  vk::DeviceGroupDeviceCreateInfo deviceGroupCreateInfo;
  if(info.useDeviceGroups)
  {
    // add ourselves to the chain
    deviceGroupCreateInfo.pNext               = deviceCreateInfo.pNext;
    deviceGroupCreateInfo.physicalDeviceCount = uint32_t(physicalGroup.physicalDeviceCount);
    deviceGroupCreateInfo.pPhysicalDevices    = physicalGroup.physicalDevices;
    deviceCreateInfo.pNext                    = &deviceGroupCreateInfo;
  }

  m_device = m_physicalDevice.createDevice(deviceCreateInfo);
  if(!m_device)
  {
    deinit();
    return false;
  }

  // reset core feature chain
  if(lastCoreFeature)
  {
    lastCoreFeature->pNext = nullptr;
  }


  // Initialize function pointers
  VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);


  // Now we have the device and instance, we can initialize the debug tool
  // get some default queues
  uint32_t queueFamilyIndex = 0;
  for(auto& it : queueFamilyProperties)
  {
    auto gct = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;
    if((it.queueFlags & gct) == gct)
    {
      m_queueGCT.queue       = m_device.getQueue(queueFamilyIndex, 0);
      m_queueGCT.familyIndex = queueFamilyIndex;
#if _DEBUG
      m_device.setDebugUtilsObjectNameEXT({vk::ObjectType::eQueue, (uint64_t)(VkQueue)m_queueGCT.queue, "queueGCT"});
#endif
    }
    else if((it.queueFlags & vk::QueueFlagBits::eTransfer) == vk::QueueFlagBits::eTransfer)
    {
      m_queueT.queue       = m_device.getQueue(queueFamilyIndex, 0);
      m_queueT.familyIndex = queueFamilyIndex;
#if _DEBUG
      m_device.setDebugUtilsObjectNameEXT({vk::ObjectType::eQueue, (uint64_t)(VkQueue)m_queueT.queue, "queueT"});
#endif
    }
    else if((it.queueFlags & vk::QueueFlagBits::eCompute))
    {
      m_queueC.queue       = m_device.getQueue(queueFamilyIndex, 0);
      m_queueC.familyIndex = queueFamilyIndex;
#if _DEBUG
      m_device.setDebugUtilsObjectNameEXT({vk::ObjectType::eQueue, (uint64_t)(VkQueue)m_queueC.queue, "queueC"});
#endif
    }

    queueFamilyIndex++;
  }


  return true;
}


//--------------------------------------------------------------------------------------------------
// Set the queue family index compatible with the \p surface
//
bool Context::setGCTQueueWithPresent(vk::SurfaceKHR surface)
{
  vk::QueueFlags bits = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;

  auto queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();
  for(uint32_t i = 0; i < uint32_t(queueFamilyProperties.size()); i++)
  {
    VkBool32 supportsPresent = m_physicalDevice.getSurfaceSupportKHR(i, surface);

    if((supportsPresent == VK_TRUE) && ((queueFamilyProperties[i].queueFlags & bits) == bits))
    {
      m_queueGCT.queue       = m_device.getQueue(i, 0);
      m_queueGCT.familyIndex = i;
#if _DEBUG
      m_device.setDebugUtilsObjectNameEXT({vk::ObjectType::eQueue, (uint64_t)(VkQueue)m_queueGCT.queue, "queueGCT"});
#endif

      return true;
    }
  }

  return false;
}

//--------------------------------------------------------------------------------------------------
// Destructor
//
void Context::deinit()
{
  if(m_device)
  {
    m_device.waitIdle();
    m_device.destroy();
    m_device = vk::Device();
  }
  if(m_dbgMessenger)
  {
    // Destroy the Debug Utils Messenger
    m_instance.destroyDebugUtilsMessengerEXT(m_dbgMessenger);
  }

  if(m_instance)
  {
    m_instance.destroy();
    m_instance = vk::Instance();
  }

  m_usedInstanceExtensions.clear();
  m_usedInstanceLayers.clear();
  m_usedDeviceExtensions.clear();

  m_createDebugUtilsMessengerEXT  = nullptr;
  m_destroyDebugUtilsMessengerEXT = nullptr;
  m_dbgMessenger                  = nullptr;
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
  static VkPhysicalDeviceHostQueryResetFeaturesEXT s_physicalHostQueryResetFeaturesEXT = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT};
  deviceExtensions.push_back({VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME, true, &s_physicalHostQueryResetFeaturesEXT});
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

VkResult Context::fillFilteredNameArray(Context::NameArray&                     used,
                                        const std::vector<vk::LayerProperties>& properties,
                                        const ContextCreateInfo::EntryArray&    requested)
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

    if(found)
      used.push_back(itr->name);
  }

  return VK_SUCCESS;
}


VkResult Context::fillFilteredNameArray(Context::NameArray&                         used,
                                        const std::vector<vk::ExtensionProperties>& properties,
                                        const ContextCreateInfo::EntryArray&        requested,
                                        std::vector<void*>&                         featureStructs)
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

void Context::initDebugReport()
{
  // Debug reporting system
  vk::DebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info;

  dbg_messenger_create_info.messageSeverity =
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
  dbg_messenger_create_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                                          | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                                          | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
  dbg_messenger_create_info.pfnUserCallback = debugMessengerCallback;

  m_dbgMessenger = m_instance.createDebugUtilsMessengerEXT(dbg_messenger_create_info);
  if(!m_dbgMessenger)
  {
    LOGE("Error: Failed creating Debug Messenger\n");
  }
}

//--------------------------------------------------------------------------------------------------
// Returns the list of devices or groups compatible with the mandatory extensions
//
std::vector<int> Context::getCompatibleDevices(const ContextCreateInfo& info)
{
  assert(m_instance);
  std::vector<int>                               compatibleDevices;
  std::vector<vk::PhysicalDeviceGroupProperties> groups;
  std::vector<vk::PhysicalDevice>                physicalDevices;

  uint32_t nbElems;
  if(info.useDeviceGroups)
  {
    groups  = m_instance.enumeratePhysicalDeviceGroups();
    nbElems = static_cast<uint32_t>(groups.size());
  }
  else
  {
    physicalDevices = m_instance.enumeratePhysicalDevices();
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
    vk::PhysicalDevice physicalDevice = info.useDeviceGroups ? groups[elemId].physicalDevices[0] : physicalDevices[elemId];

    // Note: all physical devices in a group are identical
    if(hasMandatoryExtensions(physicalDevice, info))
    {
      compatibleDevices.push_back(elemId);
      if(info.verboseCompatibleDevices)
      {
        vk::PhysicalDeviceProperties props = physicalDevice.getProperties();
        LOGI("%d: %s\n", compatible, props.deviceName);
        compatible++;
      }
    }
  }
  if(info.verboseCompatibleDevices)
  {
    LOGI("\n");
  }

  if(compatibleDevices.empty())
  {
    LOGE("ERROR: There are no compatible cards! \n");
    for(uint32_t elemId = 0; elemId < nbElems; elemId++)
    {
      vk::PhysicalDevice pd = info.useDeviceGroups ? groups[elemId].physicalDevices[0] : physicalDevices[elemId];
      LOGI("Card: %s \n", pd.getProperties().deviceName);
    }
  }

  return compatibleDevices;
}

//--------------------------------------------------------------------------------------------------
// Return true if all extensions in info, marked as required are available on the physicalDevice
//
bool Context::hasMandatoryExtensions(vk::PhysicalDevice physicalDevice, const ContextCreateInfo& info)
{
  std::vector<vk::ExtensionProperties> extensionProperties = physicalDevice.enumerateDeviceExtensionProperties();
  return checkEntryArray(extensionProperties, info.deviceExtensions);
}

bool Context::checkEntryArray(const std::vector<vk::ExtensionProperties>& properties, const ContextCreateInfo::EntryArray& requested)
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
      return false;
    }
  }

  return true;
}

void Context::printAllExtensions()
{
  LOGI("\n");
  LOGI("Available Instance Extensions :\n");
  for(const auto& it : vk::enumerateInstanceExtensionProperties())
  {
    LOGI("%s (v. %d)\n", it.extensionName, it.specVersion);
  }
}

void Context::printLayersExtensionsUsed()
{
  LOGI("______________________\n");
  LOGI("Used Instance Layers :\n");
  for(const auto& it : m_usedInstanceLayers)
  {
    LOGI("%s\n", it);
  }
  LOGI("\n");
  LOGI("Used Instance Extensions :\n");
  for(const auto& it : m_usedInstanceExtensions)
  {
    LOGI("%s\n", it);
  }
}

}  // namespace nvvkpp
