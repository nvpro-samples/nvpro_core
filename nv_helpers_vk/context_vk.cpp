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

#include <nv_helpers/nvprint.hpp>
#include "context_vk.hpp"
#include <algorithm>

namespace nv_helpers_vk {

#define CHECK_VK_RESULT() assert(result == VK_SUCCESS)

  VKAPI_ATTR VkBool32 VKAPI_CALL InstanceDeviceContext::vulkanDebugReportCallback(
    VkDebugReportFlagsEXT               msgFlags,
    VkDebugReportObjectTypeEXT          objType,
    uint64_t                            object,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData)
  {
    const char* messageCodeString = "UNKNOWN";
    bool isError = false;
    if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
    {
      messageCodeString = "INFO";
    }
    else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
      // We know that we're submitting queues without fences, ignore this warning
      if (strstr(pMsg, "vkQueueSubmit parameter, VkFence fence, is null pointer")) {
        return false;
      }

      messageCodeString = "WARN";
    }
    else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
      messageCodeString = "PERF";
    }
    else if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
      messageCodeString = "ERROR";
      isError = true;
    }
    else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
    {
      messageCodeString = "DEBUG";
    }

    char *message = (char *)malloc(strlen(pMsg) + 100);
    sprintf(message, "%s: [%s] Code %d : %s", messageCodeString, pLayerPrefix, msgCode, pMsg);

    LOGW("%s\n", message);

    free(message);

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
      (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
        m_instance, "vkCreateDebugReportCallbackEXT");
    m_vkDestroyDebugReportCallbackEXT =
      (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
        m_instance, "vkDestroyDebugReportCallbackEXT");
    m_vkDebugReportMessageEXT =
      (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(
        m_instance, "vkDebugReportMessageEXT");

    if (m_vkCreateDebugReportCallbackEXT && m_vkDestroyDebugReportCallbackEXT && m_vkDebugReportMessageEXT) {
      VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
      dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
      dbgCreateInfo.pNext = NULL;
      dbgCreateInfo.pfnCallback = vulkanDebugReportCallback;
      dbgCreateInfo.pUserData = this;
      dbgCreateInfo.flags =
        VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
      VkResult result = m_vkCreateDebugReportCallbackEXT(m_instance, &dbgCreateInfo, m_allocator,
        &m_debugCallback);
      CHECK_VK_RESULT();
    }
  }

  void InstanceDeviceContext::initDebugMarker()
  {
    m_vkDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(m_device, "vkDebugMarkerSetObjectTagEXT");
    m_vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(m_device, "vkDebugMarkerSetObjectNameEXT");
    m_vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerBeginEXT");
    m_vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerEndEXT");
    m_vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerInsertEXT");
  }


  void InstanceDeviceContext::deinitContext()
  {
    if (m_device) {
      vkDeviceWaitIdle(m_device);
      vkDestroyDevice(m_device, m_allocator);
      m_device = VK_NULL_HANDLE;
    }
    if (m_debugCallback) {
      m_vkDestroyDebugReportCallbackEXT(m_instance, m_debugCallback, m_allocator);
      m_debugCallback = VK_NULL_HANDLE;
    }
    if (m_instance) {
      vkDestroyInstance(m_instance, m_allocator);
      m_instance = VK_NULL_HANDLE;
    }

    m_usedInstanceExtensions.clear();
    m_usedInstanceLayers.clear();

    m_usedDeviceExtensions.clear();
    m_usedDeviceLayers.clear();

    m_vkDestroyDebugReportCallbackEXT = nullptr;
    m_vkCreateDebugReportCallbackEXT = nullptr;
    m_vkDebugReportMessageEXT = nullptr;

    m_vkDebugMarkerSetObjectTagEXT = nullptr;
    m_vkDebugMarkerSetObjectNameEXT = nullptr;
    m_vkCmdDebugMarkerBeginEXT = nullptr;
    m_vkCmdDebugMarkerEndEXT = nullptr;
    m_vkCmdDebugMarkerInsertEXT = nullptr;
  }

  
  bool InstanceDeviceContext::hasDeviceExtension(const char* name) const
  {
    for (auto it = m_usedDeviceExtensions.begin(); it != m_usedDeviceExtensions.end(); ++it) {
      if (strcmp(name, *it) == 0) {
        return true;
      }
    }
    return false;
  }

  void InstanceDeviceContext::debugReportMessageEXT(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage) const
  {
    if (m_vkDebugReportMessageEXT) {
      m_vkDebugReportMessageEXT(m_instance, flags, objectType, object, location, messageCode, pLayerPrefix, pMessage);
    }
  }

  VkResult InstanceDeviceContext::debugMarkerSetObjectTagEXT(const VkDebugMarkerObjectTagInfoEXT* pTagInfo) const
  {
    if (m_vkDebugMarkerSetObjectTagEXT) {
      return m_vkDebugMarkerSetObjectTagEXT(m_device, pTagInfo);
    }
    return VK_SUCCESS;
  }

  VkResult InstanceDeviceContext::debugMarkerSetObjectNameEXT(const VkDebugMarkerObjectNameInfoEXT* pNameInfo) const
  {
    if (m_vkDebugMarkerSetObjectNameEXT) {
      return m_vkDebugMarkerSetObjectNameEXT(m_device, pNameInfo);
    }
    return VK_SUCCESS;
  }

  void InstanceDeviceContext::cmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const
  {
    if (m_vkCmdDebugMarkerBeginEXT) {
      m_vkCmdDebugMarkerBeginEXT(commandBuffer, pMarkerInfo);
    }
  }

  void InstanceDeviceContext::cmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer) const
  {
    if (m_vkCmdDebugMarkerEndEXT) {
      m_vkCmdDebugMarkerEndEXT(commandBuffer);
    }
  }

  void InstanceDeviceContext::cmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const
  {
    if (m_vkCmdDebugMarkerInsertEXT) {
      m_vkCmdDebugMarkerInsertEXT(commandBuffer, pMarkerInfo);
    }
  }
  
  static VkResult fillFilteredNameArray(InstanceDeviceContext::NameArray& used, const std::vector<VkLayerProperties> &properties, const BasicContextInfo::EntryArray& requested)
  {
    VkResult result = VK_SUCCESS;

    for (auto itr = requested.begin(); itr != requested.end(); ++itr) {
      bool found = false;
      for (auto itprop = properties.begin(); itprop != properties.end(); ++itprop) {
        if (strcmp(itr->name, itprop->layerName) == 0) {
          found = true;
          break;
        }
      }

      if (!found && !itr->optional) {
        LOGW("VK_ERROR_EXTENSION_NOT_PRESENT: %s\n", itr->name);
        return VK_ERROR_EXTENSION_NOT_PRESENT;
      }

      used.push_back(itr->name);
    }

    return result;
  }

  static VkResult fillFilteredNameArray(InstanceDeviceContext::NameArray& used, const std::vector<VkExtensionProperties> &properties, const BasicContextInfo::EntryArray& requested, std::vector<void*>& featureStructs)
  {
    VkResult result = VK_SUCCESS;

    for (auto itr = requested.begin(); itr != requested.end(); ++itr) {
      bool found = false;
      for (auto itprop = properties.begin(); itprop != properties.end(); ++itprop) {
        if (strcmp(itr->name, itprop->extensionName) == 0) {
          found = true;
          break;
        }
      }

      if (found) {
        used.push_back(itr->name);
        if (itr->pFeatureStruct) {
          featureStructs.push_back(itr->pFeatureStruct);
        }
      }
      else if (!itr->optional) {
        LOGW("VK_ERROR_EXTENSION_NOT_PRESENT: %s\n", itr->name);
        return VK_ERROR_EXTENSION_NOT_PRESENT;
      }
    }

    return result;
  }

  bool BasicContextInfo::initDeviceContext(InstanceDeviceContext& ctx, const VkAllocationCallbacks* allocator) const
  {
    ctx.m_allocator = allocator;
    ctx.m_apiMajor = apiMajor;
    ctx.m_apiMinor = apiMinor;

    VkResult result;
    {
      VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
      applicationInfo.pApplicationName = appTitle;
      applicationInfo.pEngineName = appEngine;
      applicationInfo.apiVersion = VK_MAKE_VERSION(apiMajor, apiMinor, 0);

      std::vector<VkLayerProperties> layerProperties;
      std::vector<VkExtensionProperties> extensionProperties;

      uint32_t count = 0;
      result = vkEnumerateInstanceLayerProperties(&count, NULL);
      CHECK_VK_RESULT();

      layerProperties.resize(count);
      result = vkEnumerateInstanceLayerProperties(&count, layerProperties.data());
      CHECK_VK_RESULT();

      result = vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
      CHECK_VK_RESULT();

      extensionProperties.resize(count);
      result = vkEnumerateInstanceExtensionProperties(NULL, &count, extensionProperties.data());
      CHECK_VK_RESULT();


      std::vector<void*> featureStructs;

      if (fillFilteredNameArray(ctx.m_usedInstanceLayers, layerProperties, instanceLayers) != VK_SUCCESS) {
        return false;
      }
      if (fillFilteredNameArray(ctx.m_usedInstanceExtensions, extensionProperties, instanceExtensions, featureStructs) != VK_SUCCESS) {
        return false;
      }

      VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
      instanceCreateInfo.pApplicationInfo = &applicationInfo;

      instanceCreateInfo.enabledExtensionCount = (uint32_t)ctx.m_usedInstanceExtensions.size();
      instanceCreateInfo.ppEnabledExtensionNames = ctx.m_usedInstanceExtensions.data();

      instanceCreateInfo.enabledLayerCount = (uint32_t)ctx.m_usedInstanceLayers.size();
      instanceCreateInfo.ppEnabledLayerNames = ctx.m_usedInstanceLayers.data();

      result = vkCreateInstance(&instanceCreateInfo, ctx.m_allocator, &ctx.m_instance);
      if (result != VK_SUCCESS) {
        return false;
      }

      for (auto it = ctx.m_usedInstanceExtensions.begin(); it != ctx.m_usedInstanceExtensions.end(); ++it) {
        if (strcmp(*it, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
          ctx.initDebugReport();
          break;
        }
      }
    }

    {
      if (useDeviceGroups)
      {
        uint32_t deviceGroupCount = 0;
        result = vkEnumeratePhysicalDeviceGroups(ctx.m_instance, &deviceGroupCount, nullptr);

        uint32_t deviceUsed = std::min(device, deviceGroupCount - 1);

        if (result != VK_SUCCESS || deviceGroupCount == 0) {
          LOGW("could not find Vulkan device group");
          ctx.deinitContext();
          return false;
        }

        std::vector<VkPhysicalDeviceGroupProperties> deviceGroups(deviceGroupCount);
        result = vkEnumeratePhysicalDeviceGroups(ctx.m_instance, &deviceGroupCount, deviceGroups.data());
        CHECK_VK_RESULT();

        // Pick first device group for now
        ctx.m_physicalDeviceGroup.assign(
          deviceGroups[deviceUsed].physicalDevices,
          deviceGroups[deviceUsed].physicalDevices + deviceGroups[deviceUsed].physicalDeviceCount);

        ctx.m_physicalDevice = ctx.m_physicalDeviceGroup[0]; // This assumes the first device in the device group is the main one
      }
      else
      {
        uint32_t physicalDeviceCount = 0;
        result = vkEnumeratePhysicalDevices(ctx.m_instance, &physicalDeviceCount, nullptr);

        uint32_t deviceUsed = std::min(device, physicalDeviceCount - 1);

        if (result != VK_SUCCESS || physicalDeviceCount == 0) {
          LOGW("could not find Vulkan device");
          ctx.deinitContext();
          return false;
        }

        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        result = vkEnumeratePhysicalDevices(ctx.m_instance, &physicalDeviceCount, physicalDevices.data());
        CHECK_VK_RESULT();

        // pick first device for now
        ctx.m_physicalDevice = physicalDevices[deviceUsed];

        ctx.m_physicalDeviceGroup.assign({ ctx.m_physicalDevice });
      }

      ctx.m_physicalInfo.init(ctx.m_physicalDevice, ctx.m_apiMajor, ctx.m_apiMinor);
      ctx.m_physicalInfo.physicalDeviceGroup = ctx.m_physicalDeviceGroup;

      std::vector<VkDeviceQueueCreateInfo>  queues;
      std::vector<float>                    priorites;


      uint32_t queueFamilyGraphicsCompute = ~0u;
      {
        uint32_t queueFamilyPropertiesCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(ctx.m_physicalDevice, &queueFamilyPropertiesCount, NULL);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertiesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(ctx.m_physicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties.data());

        for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
          if (queueFamilyProperties[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
            queueFamilyGraphicsCompute = i;
          }
          if (queueFamilyProperties[i].queueCount > priorites.size()) {
            // set all priorities equal
            priorites.resize(queueFamilyProperties[i].queueCount, 1.0f);
          }
        }

        for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
          VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
          queueInfo.queueFamilyIndex = i;
          queueInfo.queueCount = queueFamilyProperties[i].queueCount;
          queueInfo.pQueuePriorities = priorites.data();

          queues.push_back(queueInfo);
        }
      }

      if (queueFamilyGraphicsCompute == ~0u) {
        LOGW("could not find queue that supports graphics and compute");
        ctx.deinitContext();
        return false;
      }


      // allow all queues
      VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
      deviceCreateInfo.queueCreateInfoCount = (uint32_t)queues.size();
      deviceCreateInfo.pQueueCreateInfos = queues.data();

      // device group information
      VkDeviceGroupDeviceCreateInfo deviceGroupCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO };

      // physical device layers and extensions
      std::vector<VkLayerProperties> layerProperties;
      std::vector<VkExtensionProperties> extensionProperties;

      uint32_t count = 0;
      result = vkEnumerateDeviceLayerProperties(ctx.m_physicalDevice, &count, NULL);
      CHECK_VK_RESULT();

      layerProperties.resize(count);
      result = vkEnumerateDeviceLayerProperties(ctx.m_physicalDevice, &count, layerProperties.data());
      CHECK_VK_RESULT();

      result = vkEnumerateDeviceExtensionProperties(ctx.m_physicalDevice, NULL, &count, NULL);
      CHECK_VK_RESULT();

      extensionProperties.resize(count);
      result = vkEnumerateDeviceExtensionProperties(ctx.m_physicalDevice, NULL, &count, extensionProperties.data());
      CHECK_VK_RESULT();

      std::vector<void*> featureStructs;

      if (fillFilteredNameArray(ctx.m_usedDeviceLayers, layerProperties, deviceLayers) != VK_SUCCESS) {
        ctx.deinitContext();
        return false;
      }
      if (fillFilteredNameArray(ctx.m_usedDeviceExtensions, extensionProperties, deviceExtensions, featureStructs) != VK_SUCCESS) {
        ctx.deinitContext();
        return false;
      }

      deviceCreateInfo.enabledExtensionCount = (uint32_t)ctx.m_usedDeviceExtensions.size();
      deviceCreateInfo.ppEnabledExtensionNames = ctx.m_usedDeviceExtensions.data();

      deviceCreateInfo.enabledLayerCount = (uint32_t)ctx.m_usedDeviceLayers.size();
      deviceCreateInfo.ppEnabledLayerNames = ctx.m_usedDeviceLayers.data();

      bool v11 = apiMajor == 1 && apiMinor > 0;

      for (size_t i = 0; i < sizeof(keepFeatures) / sizeof(VkBool32); i++) {
        const VkBool32* pKeep  = (const VkBool32*)&keepFeatures;
        VkBool32* pOut = (VkBool32*)&ctx.m_physicalInfo.features2.features;

        if (!pKeep[i]) {
          pOut[i] = VK_FALSE;
        }
      }

      deviceCreateInfo.pEnabledFeatures = v11 ? nullptr : &ctx.m_physicalInfo.features2.features;
      deviceCreateInfo.pNext = v11 ? &ctx.m_physicalInfo.features2 : nullptr;

      if (v11 && !featureStructs.empty()) {

        // build up chain of all used extension features
        for (size_t i = 0; i < featureStructs.size(); i++) {
          ExtensionHeader* header = (ExtensionHeader*)featureStructs[i];
          header->pNext = i < featureStructs.size() - 1 ? featureStructs[i + 1] : nullptr;
        }

        // query them in full enabled state
        VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        features2.pNext = featureStructs[0];
        vkGetPhysicalDeviceFeatures2(ctx.m_physicalDevice, &features2);

        // pass them as part of the create chain
        // set last element to previous beginning of chain
        ExtensionHeader* header = (ExtensionHeader*)featureStructs[featureStructs.size()-1];
        header->pNext = (void*)deviceCreateInfo.pNext;

        deviceCreateInfo.pNext = featureStructs[0];
      }

      if (useDeviceGroups)
      {
        deviceCreateInfo.pNext = &deviceGroupCreateInfo;
        deviceGroupCreateInfo.physicalDeviceCount = uint32_t(ctx.m_physicalDeviceGroup.size());
        deviceGroupCreateInfo.pPhysicalDevices = ctx.m_physicalDeviceGroup.data();
        deviceGroupCreateInfo.pNext = deviceGroupCreateInfo.pNext;
      }

      result = vkCreateDevice(ctx.m_physicalDevice, &deviceCreateInfo, ctx.m_allocator, &ctx.m_device);

      if (result != VK_SUCCESS) {
        ctx.deinitContext();
        return false;
      }

      for (auto it = ctx.m_usedDeviceExtensions.begin(); it != ctx.m_usedDeviceExtensions.end(); ++it) {
        if (strcmp(*it, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
          ctx.initDebugMarker();
          break;
        }
      }
    }

    return true;
  }
}
