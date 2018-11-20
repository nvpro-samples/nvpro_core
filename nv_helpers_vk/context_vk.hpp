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

#ifndef NV_VK_CONTEXT_INCLUDED
#define NV_VK_CONTEXT_INCLUDED

#include <vulkan/vulkan.h>
#include <vector>

#include "base_vk.hpp"

namespace nv_helpers_vk {

  class InstanceDeviceContext {
  public:
    typedef std::vector<const char*>  NameArray;

    uint32_t                      m_apiMajor = 1;
    uint32_t                      m_apiMinor = 0;

    const VkAllocationCallbacks*  m_allocator = nullptr;
    VkInstance                    m_instance = VK_NULL_HANDLE;
    VkDevice                      m_device = VK_NULL_HANDLE;
    VkPhysicalDevice              m_physicalDevice = VK_NULL_HANDLE;
    std::vector<VkPhysicalDevice> m_physicalDeviceGroup;
    PhysicalInfo                  m_physicalInfo;

    NameArray                     m_usedInstanceLayers;
    NameArray                     m_usedInstanceExtensions;
    NameArray                     m_usedDeviceLayers;
    NameArray                     m_usedDeviceExtensions;

    void deinitContext();

    void initDebugReport();
    void initDebugMarker();

    bool hasDeviceExtension(const char* name) const;

    void debugReportMessageEXT(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode,
      const char* pLayerPrefix, const char* pMessage) const;

    VkResult debugMarkerSetObjectTagEXT (const VkDebugMarkerObjectTagInfoEXT*  pTagInfo) const;
    VkResult debugMarkerSetObjectNameEXT(const VkDebugMarkerObjectNameInfoEXT* pNameInfo) const;

    void cmdDebugMarkerBeginEXT (VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const;
    void cmdDebugMarkerEndEXT   (VkCommandBuffer commandBuffer) const;
    void cmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const;

  private:
    VkDebugReportCallbackEXT              m_debugCallback = VK_NULL_HANDLE;

    PFN_vkCreateDebugReportCallbackEXT    m_vkCreateDebugReportCallbackEXT = nullptr;
    PFN_vkDestroyDebugReportCallbackEXT   m_vkDestroyDebugReportCallbackEXT = nullptr;
    PFN_vkDebugReportMessageEXT           m_vkDebugReportMessageEXT = nullptr;

    PFN_vkDebugMarkerSetObjectTagEXT      m_vkDebugMarkerSetObjectTagEXT = nullptr;
    PFN_vkDebugMarkerSetObjectNameEXT     m_vkDebugMarkerSetObjectNameEXT = nullptr;
    PFN_vkCmdDebugMarkerBeginEXT          m_vkCmdDebugMarkerBeginEXT = nullptr;
    PFN_vkCmdDebugMarkerEndEXT            m_vkCmdDebugMarkerEndEXT = nullptr;
    PFN_vkCmdDebugMarkerInsertEXT         m_vkCmdDebugMarkerInsertEXT = nullptr;

    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugReportCallback(
      VkDebugReportFlagsEXT               msgFlags,
      VkDebugReportObjectTypeEXT          objType,
      uint64_t                            object,
      size_t                              location,
      int32_t                             msgCode,
      const char*                         pLayerPrefix,
      const char*                         pMsg,
      void*                               pUserData);
  };


  struct BasicContextInfo {

    //////////////////////////////////////////////////////////////////////////
    // Configure context creation with these variables and functions

    int           apiMajor = 1;
    int           apiMinor = 1;
    uint32_t      device = 0;
    const char*   appEngine = "nvpro-sample";
    const char*   appTitle = "nvpro-sample";

    bool          useDeviceGroups = false;

    // by default all device features, except robust access are enabled
    // if the device supports them. Set a feature to zero
    // to disable it.
    VkPhysicalDeviceFeatures    keepFeatures;

    void addInstanceExtension(const char* name, bool optional = false) {
      instanceExtensions.push_back(Entry(name, optional));
    }
    void addInstanceLayer(const char* name, bool optional = false) {
      instanceLayers.push_back(Entry(name, optional));
    }

    // pFeatureStruct is used for a version 1.1 and higher context. It will be queried from physical device 
    // and then passed in this state to device create info.
    void addDeviceExtension(const char* name, bool optional = false, void* pFeatureStruct = nullptr) {
      deviceExtensions.push_back(Entry(name, optional, pFeatureStruct));
    }
    void addDeviceLayer(const char* name, bool optional = false) {
      deviceLayers.push_back(Entry(name, optional));
    }

    //////////////////////////////////////////////////////////////////////////

    BasicContextInfo()
    {
      memset(&keepFeatures, 1, sizeof(VkPhysicalDeviceFeatures));
      keepFeatures.robustBufferAccess = VK_FALSE;
#ifdef _DEBUG
      instanceExtensions.push_back({ VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true });
      deviceExtensions.push_back({ VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true });
#endif
    }

    bool initDeviceContext(InstanceDeviceContext& ctx, const VkAllocationCallbacks* allocator = nullptr) const;

    struct Entry {
      const char* name = nullptr;
      bool        optional = false;
      void*       pFeatureStruct = nullptr;

      Entry() {}
      Entry(const char* entryName, bool isOptional = false, void* pointerFeatureStruct = nullptr) : 
        name(entryName), 
        optional(isOptional), 
        pFeatureStruct(pointerFeatureStruct){}
    };
    typedef std::vector<Entry>  EntryArray;

  private:

    struct ExtensionHeader {
      VkStructureType sType;
      void*           pNext;
    };

    EntryArray    instanceLayers;
    EntryArray    instanceExtensions;
    EntryArray    deviceLayers;
    EntryArray    deviceExtensions;
  };
}

#endif
