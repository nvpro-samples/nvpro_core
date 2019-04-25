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

#include <vector>
#include <vulkan/vulkan.h>

#include "physical_vk.hpp"

namespace nvvk {

struct ContextInfoVK;

//////////////////////////////////////////////////////////////////////////
// This class maintains
// - the Vulkan instance
// - Physical Device(s)
// - Physical Info
// - the LOGICAL Device
//
// It maintains what extensions are finally available
// and allows to manage the debugging part for callbacks etc.
class InstanceDeviceContext
{
public:
  using NameArray = std::vector<const char*>;
  //
  // Public members for the application to directly access them
  //
  uint32_t m_apiMajor = 1;
  uint32_t m_apiMinor = 0;

  const VkAllocationCallbacks*  m_allocator      = nullptr;
  VkInstance                    m_instance       = VK_NULL_HANDLE;
  VkDevice                      m_device         = VK_NULL_HANDLE;
  VkPhysicalDevice              m_physicalDevice = VK_NULL_HANDLE;
  std::vector<VkPhysicalDevice> m_physicalDeviceGroup;
  PhysicalInfo                  m_physicalInfo;

  NameArray m_usedInstanceLayers;
  NameArray m_usedInstanceExtensions;
  NameArray m_usedDeviceLayers;
  NameArray m_usedDeviceExtensions;
  //
  // initContext takes what features has been requrested by the application in ContextInfoVK
  // and set-up the instance, layers, physical device(s), logical device etc.
  //
  bool initContext(ContextInfoVK &info, const VkAllocationCallbacks* allocator = nullptr);
  void deinitContext();
  bool hasDeviceExtension(const char* name) const;
  //
  // methods for Debugging information
  //
  void initDebugReport();
  void initDebugMarker();

  void debugReportMessageEXT(VkDebugReportFlagsEXT      flags,
                             VkDebugReportObjectTypeEXT objectType,
                             uint64_t                   object,
                             size_t                     location,
                             int32_t                    messageCode,
                             const char*                pLayerPrefix,
                             const char*                pMessage) const;

  VkResult debugMarkerSetObjectTagEXT(const VkDebugMarkerObjectTagInfoEXT* pTagInfo) const;
  VkResult debugMarkerSetObjectNameEXT(const VkDebugMarkerObjectNameInfoEXT* pNameInfo) const;

  void cmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const;
  void cmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer) const;
  void cmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const;

private:
  VkDebugReportCallbackEXT m_debugCallback = VK_NULL_HANDLE;

  PFN_vkCreateDebugReportCallbackEXT  m_vkCreateDebugReportCallbackEXT  = nullptr;
  PFN_vkDestroyDebugReportCallbackEXT m_vkDestroyDebugReportCallbackEXT = nullptr;
  PFN_vkDebugReportMessageEXT         m_vkDebugReportMessageEXT         = nullptr;

  PFN_vkDebugMarkerSetObjectTagEXT  m_vkDebugMarkerSetObjectTagEXT  = nullptr;
  PFN_vkDebugMarkerSetObjectNameEXT m_vkDebugMarkerSetObjectNameEXT = nullptr;
  PFN_vkCmdDebugMarkerBeginEXT      m_vkCmdDebugMarkerBeginEXT      = nullptr;
  PFN_vkCmdDebugMarkerEndEXT        m_vkCmdDebugMarkerEndEXT        = nullptr;
  PFN_vkCmdDebugMarkerInsertEXT     m_vkCmdDebugMarkerInsertEXT     = nullptr;

  static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugReportCallback(VkDebugReportFlagsEXT      msgFlags,
                                                                  VkDebugReportObjectTypeEXT objType,
                                                                  uint64_t                   object,
                                                                  size_t                     location,
                                                                  int32_t                    msgCode,
                                                                  const char*                pLayerPrefix,
                                                                  const char*                pMsg,
                                                                  void*                      pUserData);
};


//////////////////////////////////////////////////////////////////////////
//
// this structure allows the application to specify a set of features
// that are expected for the creation of
// - instance
// - physical and logical device
// Then InstanceDeviceContext::InitContext(...) will use it to query Vulkan
// for whatever is required here
//
struct ContextInfoVK
{
  ContextInfoVK();
  void addInstanceExtension(const char* name, bool optional = false);
  void addInstanceLayer(const char* name, bool optional = false);
  // pFeatureStruct is used for a version 1.1 and higher context. It will be queried from physical device
  // and then passed in this state to device create info.
  void addDeviceExtension(const char* name, bool optional = false, void* pFeatureStruct = nullptr);
  void addDeviceLayer(const char* name, bool optional = false);

  // Configure context creation with these variables and functions
  int         apiMajor  = 1;
  int         apiMinor  = 1;
  uint32_t    device    = 0;
  const char* appEngine = "nvpro-sample";
  const char* appTitle  = "nvpro-sample";

  bool useDeviceGroups  = false;
  bool verboseUsed      = true;
  bool verboseAvailable =
#ifdef _DEBUG
    true; 
#else
    false;
#endif
  // by default all device features, except robust access are enabled
  // if the device supports them. Set a feature to zero
  // to disable it.
  VkPhysicalDeviceFeatures keepFeatures;

  struct Entry
  {
    const char* name           = nullptr;
    bool        optional       = false;
    void*       pFeatureStruct = nullptr;

    Entry() {}
    Entry(const char* entryName, bool isOptional = false, void* pointerFeatureStruct = nullptr)
        : name(entryName)
        , optional(isOptional)
        , pFeatureStruct(pointerFeatureStruct)
    {
    }
  };
  using EntryArray = std::vector<Entry>;

  struct ExtensionHeader
  {
    VkStructureType sType;
    void*           pNext;
  };

  EntryArray instanceLayers;
  EntryArray instanceExtensions;
  EntryArray deviceLayers;
  EntryArray deviceExtensions;
};//struct ContextInfoVK
}  // namespace nvvk

#endif
