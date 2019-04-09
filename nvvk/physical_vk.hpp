

#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace nvvk
{
  // returns ~0 if not found
  uint32_t PhysicalDeviceMemoryProperties_getMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& memoryProperties,
    const VkMemoryRequirements&             memReqs,
    VkFlags                                 memProps);

  // returns false on incompatibility
  bool PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(const VkPhysicalDeviceMemoryProperties& memoryProperties,
    const VkMemoryRequirements&             memReqs,
    VkFlags                                 memProps,
    VkMemoryAllocateInfo&                   memInfo);

  bool PhysicalDeviceMemoryProperties_appendMemoryAllocationInfo(const VkPhysicalDeviceMemoryProperties& memoryProperties,
    const VkMemoryRequirements&             memReqs,
    VkFlags                                 memProps,
    VkMemoryAllocateInfo&                   memInfoAppended,
    VkDeviceSize&                           offset);

  struct PhysicalInfo
  {
    /*
      Here we store typical information about the pyhsical device for easy access.
      It is also particularly useful for Vulkan instances beyond version 1.0
    */


    uint32_t                             apiMajor = 0;
    uint32_t                             apiMinor = 0;
    VkPhysicalDevice                     physicalDevice = {};
    std::vector<VkPhysicalDevice>        physicalDeviceGroup = {};
    VkPhysicalDeviceMemoryProperties     memoryProperties = {};
    VkPhysicalDeviceProperties           properties = {};  // copy of properties2.properties (backwards compatibility)
    VkPhysicalDeviceFeatures2            features2 = {};
    std::vector<VkQueueFamilyProperties> queueProperties;

    // vulkan 1.1
    struct Features
    {
      VkPhysicalDeviceMultiviewFeatures              multiview = {};
      VkPhysicalDevice16BitStorageFeatures           t16BitStorage = {};
      VkPhysicalDeviceSamplerYcbcrConversionFeatures samplerYcbcrConversion = {};
      VkPhysicalDeviceProtectedMemoryFeatures        protectedMemory = {};
      VkPhysicalDeviceShaderDrawParameterFeatures    drawParameters = {};
      VkPhysicalDeviceVariablePointerFeatures        variablePointers = {};
    };
    struct Properties
    {
      VkPhysicalDeviceMaintenance3Properties    maintenance3 = {};
      VkPhysicalDeviceIDProperties              deviceID = {};
      VkPhysicalDeviceMultiviewProperties       multiview = {};
      VkPhysicalDeviceProtectedMemoryProperties protectedMemory = {};
      VkPhysicalDevicePointClippingProperties   pointClipping = {};
      VkPhysicalDeviceSubgroupProperties        subgroup = {};
    };

    Features   extFeatures;
    Properties extProperties;

    void init(VkPhysicalDevice physicalDeviceIn, uint32_t apiMajorIn = 1, uint32_t apiMinorIn = 0);

    bool     getOptimalDepthStencilFormat(VkFormat& depthStencilFormat) const;
    uint32_t getExclusiveQueueFamily(VkQueueFlagBits bits) const;
    uint32_t getQueueFamily(VkQueueFlags bits = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT) const;
    uint32_t getPresentQueueFamily(VkSurfaceKHR surface, VkQueueFlags bits = VK_QUEUE_GRAPHICS_BIT) const;

    // returns ~0 if not found
    uint32_t getMemoryTypeIndex(const VkMemoryRequirements& memReqs, VkFlags memProps) const;
    bool getMemoryAllocationInfo(const VkMemoryRequirements& memReqs, VkFlags memProps, VkMemoryAllocateInfo& memInfo) const;
    bool appendMemoryAllocationInfo(const VkMemoryRequirements& memReqs, VkFlags memProps, VkMemoryAllocateInfo& memInfoAppended, VkDeviceSize& offset) const;

    PhysicalInfo() {}
    PhysicalInfo(VkPhysicalDevice physical, uint32_t apiMajorIn = 1, uint32_t apiMinorIn = 0)
    {
      init(physical, apiMajorIn, apiMinorIn);
    }
  };
}

