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

#include "context_vk.hpp"
#include <algorithm>
#include <nvh/nvprint.hpp>

namespace nvvk {

static VkResult fillFilteredNameArray(InstanceDeviceContext::NameArray&     used,
                                      const std::vector<VkLayerProperties>& properties,
                                      const ContextInfoVK::EntryArray&      requested)
{
  VkResult result = VK_SUCCESS;

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

  return result;
}

static VkResult fillFilteredNameArray(InstanceDeviceContext::NameArray&         used,
                                      const std::vector<VkExtensionProperties>& properties,
                                      const ContextInfoVK::EntryArray&          requested,
                                      std::vector<void*>&                       featureStructs)
{
  VkResult result = VK_SUCCESS;

  for(auto itr = requested.begin(); itr != requested.end(); ++itr)
  {
    bool found = false;
    for(const auto& propertie : properties)
    {
      if(strcmp(itr->name, propertie.extensionName) == 0)
      {
        found = true;
        break;
      }
    }

    if(found)
    {
      used.push_back(itr->name);
      if(itr->pFeatureStruct)
      {
        featureStructs.push_back(itr->pFeatureStruct);
      }
    }
    else if(!itr->optional)
    {
      LOGW("VK_ERROR_EXTENSION_NOT_PRESENT: %s\n", itr->name);
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
  }

  return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL InstanceDeviceContext::vulkanDebugReportCallback(VkDebugReportFlagsEXT      msgFlags,
                                                                                VkDebugReportObjectTypeEXT objType,
                                                                                uint64_t                   object,
                                                                                size_t                     location,
                                                                                int32_t                    msgCode,
                                                                                const char*                pLayerPrefix,
                                                                                const char*                pMsg,
                                                                                void*                      pUserData)
{
  const char* messageCodeString = "UNKNOWN";
  int         level             = LOGLEVEL_INFO;
  if(msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
  {
    messageCodeString = "INFO";
  }
  else if(msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
  {
    // We know that we're submitting queues without fences, ignore this warning
    if(strstr(pMsg, "vkQueueSubmit parameter, VkFence fence, is null pointer"))
    {
      return false;
    }
    level             = LOGLEVEL_WARNING;
    messageCodeString = "WARN";
  }
  else if(msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
  {
    level             = LOGLEVEL_STATS;
    messageCodeString = "PERF";
  }
  else if(msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
  {
    level             = LOGLEVEL_ERROR;
    messageCodeString = "ERROR";
  }
  else if(msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
  {
    level             = LOGLEVEL_DEBUG;
    messageCodeString = "DEBUG";
  }

  nvprintfLevel(level, "%s: [%s] Code %d : %s\n", messageCodeString, pLayerPrefix, msgCode, pMsg);
  /*
    * false indicates that layer should not bail-out of an
    * API call that had validation failures. This may mean that the
    * app dies inside the driver due to invalid parameter(s).
    * That's what would happen without validation layers, so we'll
    * keep that behavior here.
    */
  return false;
}

void InstanceDeviceContext::initDebugReport()
{
  assert(!m_debugCallback);

  m_vkCreateDebugReportCallbackEXT =
      (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
  m_vkDestroyDebugReportCallbackEXT =
      (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT");
  m_vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(m_instance, "vkDebugReportMessageEXT");

  if(m_vkCreateDebugReportCallbackEXT && m_vkDestroyDebugReportCallbackEXT && m_vkDebugReportMessageEXT)
  {
    VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
    dbgCreateInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    dbgCreateInfo.pNext       = nullptr;
    dbgCreateInfo.pfnCallback = vulkanDebugReportCallback;
    dbgCreateInfo.pUserData   = this;
    dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    VkResult result = m_vkCreateDebugReportCallbackEXT(m_instance, &dbgCreateInfo, m_allocator, &m_debugCallback);
    assert(result == VK_SUCCESS);
  }
}

void InstanceDeviceContext::initDebugMarker()
{
  m_vkDebugMarkerSetObjectTagEXT =
      (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(m_device, "vkDebugMarkerSetObjectTagEXT");
  m_vkDebugMarkerSetObjectNameEXT =
      (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(m_device, "vkDebugMarkerSetObjectNameEXT");
  m_vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerBeginEXT");
  m_vkCmdDebugMarkerEndEXT   = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerEndEXT");
  m_vkCmdDebugMarkerInsertEXT =
      (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerInsertEXT");
}
bool InstanceDeviceContext::initContext(ContextInfoVK& info, const VkAllocationCallbacks* allocator)
{
  m_allocator = allocator;
  m_apiMajor  = info.apiMajor;
  m_apiMinor  = info.apiMinor;

  VkResult result;
  {
    VkApplicationInfo applicationInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName  = info.appTitle;
    applicationInfo.pEngineName       = info.appEngine;
    applicationInfo.apiVersion        = VK_MAKE_VERSION(m_apiMajor, m_apiMinor, 0);

    std::vector<VkLayerProperties>     layerProperties;
    std::vector<VkExtensionProperties> extensionProperties;

    uint32_t count = 0;
    {
      result = vkEnumerateInstanceLayerProperties(&count, nullptr);
      assert(result == VK_SUCCESS);
      layerProperties.resize(count);
      result = vkEnumerateInstanceLayerProperties(&count, layerProperties.data());
      assert(result == VK_SUCCESS);
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
      result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
      assert(result == VK_SUCCESS);
      extensionProperties.resize(count);
      result = vkEnumerateInstanceExtensionProperties(nullptr, &count, extensionProperties.data());
      assert(result == VK_SUCCESS);
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

    std::vector<void*> featureStructs;

    if(fillFilteredNameArray(m_usedInstanceLayers, layerProperties, info.instanceLayers) != VK_SUCCESS)
    {
      return false;
    }
    if(fillFilteredNameArray(m_usedInstanceExtensions, extensionProperties, info.instanceExtensions, featureStructs) != VK_SUCCESS)
    {
      return false;
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

    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pApplicationInfo     = &applicationInfo;

    instanceCreateInfo.enabledExtensionCount   = (uint32_t)m_usedInstanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = m_usedInstanceExtensions.data();

    instanceCreateInfo.enabledLayerCount   = (uint32_t)m_usedInstanceLayers.size();
    instanceCreateInfo.ppEnabledLayerNames = m_usedInstanceLayers.data();

    result = vkCreateInstance(&instanceCreateInfo, m_allocator, &m_instance);
    if(result != VK_SUCCESS)
    {
      return false;
    }

    for(auto it = m_usedInstanceExtensions.begin(); it != m_usedInstanceExtensions.end(); ++it)
    {
      if(strcmp(*it, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
      {
        initDebugReport();
        break;
      }
    }
  }

  {
    if(info.useDeviceGroups)
    {
      uint32_t deviceGroupCount = 0;
      result                    = vkEnumeratePhysicalDeviceGroups(m_instance, &deviceGroupCount, nullptr);

      uint32_t deviceUsed = std::min(info.device, deviceGroupCount - 1);

      if(result != VK_SUCCESS || deviceGroupCount == 0)
      {
        LOGW("could not find Vulkan device group");
        deinitContext();
        return false;
      }

      std::vector<VkPhysicalDeviceGroupProperties> deviceGroups(deviceGroupCount);
      result = vkEnumeratePhysicalDeviceGroups(m_instance, &deviceGroupCount, deviceGroups.data());
      assert(result == VK_SUCCESS);

      // Pick first device group for now
      m_physicalDeviceGroup.assign(deviceGroups[deviceUsed].physicalDevices,
                                   deviceGroups[deviceUsed].physicalDevices + deviceGroups[deviceUsed].physicalDeviceCount);

      m_physicalDevice = m_physicalDeviceGroup[0];  // This assumes the first device in the device group is the main one
    }
    else
    {
      uint32_t physicalDeviceCount = 0;
      result                       = vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

      uint32_t deviceUsed = std::min(info.device, physicalDeviceCount - 1);

      if(result != VK_SUCCESS || physicalDeviceCount == 0)
      {
        LOGW("could not find Vulkan device");
        deinitContext();
        return false;
      }

      std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
      result = vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());
      assert(result == VK_SUCCESS);

      // pick first device for now
      m_physicalDevice = physicalDevices[deviceUsed];

      m_physicalDeviceGroup.assign({m_physicalDevice});
    }

    m_physicalInfo.init(m_physicalDevice, m_apiMajor, m_apiMinor);
    m_physicalInfo.physicalDeviceGroup = m_physicalDeviceGroup;

    std::vector<VkDeviceQueueCreateInfo> queues;
    std::vector<float>                   priorites;


    uint32_t queueFamilyGraphicsCompute = ~0u;
    {
      uint32_t queueFamilyPropertiesCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertiesCount, nullptr);

      std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertiesCount);
      vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties.data());

      for(uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
      {
        if(queueFamilyProperties[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
        {
          queueFamilyGraphicsCompute = i;
        }
        if(queueFamilyProperties[i].queueCount > priorites.size())
        {
          // set all priorities equal
          priorites.resize(queueFamilyProperties[i].queueCount, 1.0f);
        }
      }

      for(uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
      {
        VkDeviceQueueCreateInfo queueInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex        = i;
        queueInfo.queueCount              = queueFamilyProperties[i].queueCount;
        queueInfo.pQueuePriorities        = priorites.data();

        queues.push_back(queueInfo);
      }
    }

    if(queueFamilyGraphicsCompute == ~0u)
    {
      LOGW("could not find queue that supports graphics and compute");
      deinitContext();
      return false;
    }


    // allow all queues
    VkDeviceCreateInfo deviceCreateInfo   = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.queueCreateInfoCount = (uint32_t)queues.size();
    deviceCreateInfo.pQueueCreateInfos    = queues.data();

    // device group information
    VkDeviceGroupDeviceCreateInfo deviceGroupCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO};

    // physical device layers and extensions
    std::vector<VkLayerProperties>     layerProperties;
    std::vector<VkExtensionProperties> extensionProperties;

    uint32_t count = 0;
    result         = vkEnumerateDeviceLayerProperties(m_physicalDevice, &count, nullptr);
    assert(result == VK_SUCCESS);

    layerProperties.resize(count);
    result = vkEnumerateDeviceLayerProperties(m_physicalDevice, &count, layerProperties.data());
    assert(result == VK_SUCCESS);
    if(info.verboseAvailable)
    {
      LOGI("_________________________\n");
      LOGI("Available Device Layers :\n");
      for(auto it : layerProperties)
      {
        LOGI("%s (v. %x %x) : %s\n", it.layerName, it.specVersion, it.implementationVersion, it.description);
      }
    }

    result = vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &count, nullptr);
    assert(result == VK_SUCCESS);

    extensionProperties.resize(count);
    result = vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &count, extensionProperties.data());
    assert(result == VK_SUCCESS);
    if(info.verboseAvailable)
    {
      LOGI("\n");
      LOGI("Available Device Extensions :\n");
      for(auto it : extensionProperties)
      {
        LOGI("%s (v. %d)\n", it.extensionName, it.specVersion);
      }
    }

    std::vector<void*> featureStructs;

    if(fillFilteredNameArray(m_usedDeviceLayers, layerProperties, info.deviceLayers) != VK_SUCCESS)
    {
      deinitContext();
      return false;
    }
    if(fillFilteredNameArray(m_usedDeviceExtensions, extensionProperties, info.deviceExtensions, featureStructs) != VK_SUCCESS)
    {
      deinitContext();
      return false;
    }
    if(info.verboseUsed)
    {
      LOGI("____________________\n");
      LOGI("Used Device Layers :\n");
      for(auto it : m_usedDeviceLayers)
      {
        LOGI("%s\n", it);
      }
      LOGI("\n");
      LOGI("Used Device Extensions :\n");
      for(auto it : m_usedDeviceExtensions)
      {
        LOGI("%s\n", it);
      }
      LOGI("\n");
    }

    deviceCreateInfo.enabledExtensionCount   = (uint32_t)m_usedDeviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = m_usedDeviceExtensions.data();

    deviceCreateInfo.enabledLayerCount   = (uint32_t)m_usedDeviceLayers.size();
    deviceCreateInfo.ppEnabledLayerNames = m_usedDeviceLayers.data();

    bool v11 = m_apiMajor == 1 && m_apiMinor > 0;

    for(size_t i = 0; i < sizeof(info.keepFeatures) / sizeof(VkBool32); i++)
    {
      const auto* pKeep = (const VkBool32*)&info.keepFeatures;
      auto*       pOut  = (VkBool32*)&m_physicalInfo.features2.features;

      if(!pKeep[i])
      {
        pOut[i] = VK_FALSE;
      }
    }

    deviceCreateInfo.pEnabledFeatures = v11 ? nullptr : &m_physicalInfo.features2.features;
    deviceCreateInfo.pNext            = v11 ? &m_physicalInfo.features2 : nullptr;

    if(v11 && !featureStructs.empty())
    {

      // build up chain of all used extension features
      for(size_t i = 0; i < featureStructs.size(); i++)
      {
        auto* header  = (ContextInfoVK::ExtensionHeader*)featureStructs[i];
        header->pNext = i < featureStructs.size() - 1 ? featureStructs[i + 1] : nullptr;
      }

      // query them in full enabled state
      VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
      features2.pNext                     = featureStructs[0];
      vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);

      // pass them as part of the create chain
      // set last element to previous beginning of chain
      auto* header  = (ContextInfoVK::ExtensionHeader*)featureStructs[featureStructs.size() - 1];
      header->pNext = (void*)deviceCreateInfo.pNext;

      deviceCreateInfo.pNext = featureStructs[0];
    }

    if(info.useDeviceGroups)
    {
      deviceCreateInfo.pNext                    = &deviceGroupCreateInfo;
      deviceGroupCreateInfo.physicalDeviceCount = uint32_t(m_physicalDeviceGroup.size());
      deviceGroupCreateInfo.pPhysicalDevices    = m_physicalDeviceGroup.data();
      deviceGroupCreateInfo.pNext               = deviceGroupCreateInfo.pNext;
    }

    result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, m_allocator, &m_device);

    if(result != VK_SUCCESS)
    {
      deinitContext();
      return false;
    }

    for(auto it = m_usedDeviceExtensions.begin(); it != m_usedDeviceExtensions.end(); ++it)
    {
      if(strcmp(*it, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0)
      {
        initDebugMarker();
        break;
      }
    }
  }

  return true;
}

void InstanceDeviceContext::deinitContext()
{
  if(m_device)
  {
    vkDeviceWaitIdle(m_device);
    vkDestroyDevice(m_device, m_allocator);
    m_device = VK_NULL_HANDLE;
  }
  if(m_debugCallback)
  {
    m_vkDestroyDebugReportCallbackEXT(m_instance, m_debugCallback, m_allocator);
    m_debugCallback = VK_NULL_HANDLE;
  }
  if(m_instance)
  {
    vkDestroyInstance(m_instance, m_allocator);
    m_instance = VK_NULL_HANDLE;
  }

  m_usedInstanceExtensions.clear();
  m_usedInstanceLayers.clear();

  m_usedDeviceExtensions.clear();
  m_usedDeviceLayers.clear();

  m_vkDestroyDebugReportCallbackEXT = nullptr;
  m_vkCreateDebugReportCallbackEXT  = nullptr;
  m_vkDebugReportMessageEXT         = nullptr;

  m_vkDebugMarkerSetObjectTagEXT  = nullptr;
  m_vkDebugMarkerSetObjectNameEXT = nullptr;
  m_vkCmdDebugMarkerBeginEXT      = nullptr;
  m_vkCmdDebugMarkerEndEXT        = nullptr;
  m_vkCmdDebugMarkerInsertEXT     = nullptr;
}


bool InstanceDeviceContext::hasDeviceExtension(const char* name) const
{
  for(auto usedDeviceExtension : m_usedDeviceExtensions)
  {
    if(strcmp(name, usedDeviceExtension) == 0)
    {
      return true;
    }
  }
  return false;
}

void InstanceDeviceContext::debugReportMessageEXT(VkDebugReportFlagsEXT      flags,
                                                  VkDebugReportObjectTypeEXT objectType,
                                                  uint64_t                   object,
                                                  size_t                     location,
                                                  int32_t                    messageCode,
                                                  const char*                pLayerPrefix,
                                                  const char*                pMessage) const
{
  if(m_vkDebugReportMessageEXT)
  {
    m_vkDebugReportMessageEXT(m_instance, flags, objectType, object, location, messageCode, pLayerPrefix, pMessage);
  }
}

VkResult InstanceDeviceContext::debugMarkerSetObjectTagEXT(const VkDebugMarkerObjectTagInfoEXT* pTagInfo) const
{
  if(m_vkDebugMarkerSetObjectTagEXT)
  {
    return m_vkDebugMarkerSetObjectTagEXT(m_device, pTagInfo);
  }
  return VK_SUCCESS;
}

VkResult InstanceDeviceContext::debugMarkerSetObjectNameEXT(const VkDebugMarkerObjectNameInfoEXT* pNameInfo) const
{
  if(m_vkDebugMarkerSetObjectNameEXT)
  {
    return m_vkDebugMarkerSetObjectNameEXT(m_device, pNameInfo);
  }
  return VK_SUCCESS;
}

void InstanceDeviceContext::cmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const
{
  if(m_vkCmdDebugMarkerBeginEXT)
  {
    m_vkCmdDebugMarkerBeginEXT(commandBuffer, pMarkerInfo);
  }
}

void InstanceDeviceContext::cmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer) const
{
  if(m_vkCmdDebugMarkerEndEXT)
  {
    m_vkCmdDebugMarkerEndEXT(commandBuffer);
  }
}

void InstanceDeviceContext::cmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const
{
  if(m_vkCmdDebugMarkerInsertEXT)
  {
    m_vkCmdDebugMarkerInsertEXT(commandBuffer, pMarkerInfo);
  }
}
//////////////////////////////////////////////////////////////////////////////////////
//
// ContextInfoVK methods
//
ContextInfoVK::ContextInfoVK()
{
  memset(&keepFeatures, 1, sizeof(VkPhysicalDeviceFeatures));
  keepFeatures.robustBufferAccess = VK_FALSE;
#ifdef _DEBUG
  instanceExtensions.push_back({VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true});
  deviceExtensions.push_back({VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true});
#endif
}
void ContextInfoVK::addInstanceExtension(const char* name, bool optional)
{
  instanceExtensions.emplace_back(name, optional);
}
void ContextInfoVK::addInstanceLayer(const char* name, bool optional)
{
  instanceLayers.emplace_back(name, optional);
}
void ContextInfoVK::addDeviceExtension(const char* name, bool optional, void* pFeatureStruct)
{
  deviceExtensions.emplace_back(name, optional, pFeatureStruct);
}
void ContextInfoVK::addDeviceLayer(const char* name, bool optional)
{
  deviceLayers.emplace_back(name, optional);
}

}  // namespace nvvk
