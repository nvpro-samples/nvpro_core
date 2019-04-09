#include "physical_vk.hpp"

namespace nvvk
{

  uint32_t PhysicalDeviceMemoryProperties_getMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& memoryProperties,
    const VkMemoryRequirements&             memReqs,
    VkFlags                                 memProps)
  {
    // Find an available memory type that satisfies the requested properties.
    for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
    {
      if ((memReqs.memoryTypeBits & (1 << memoryTypeIndex))
        && (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memProps) == memProps)
      {
        return memoryTypeIndex;
      }
    }
    return ~0;
  }

  bool PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(const VkPhysicalDeviceMemoryProperties& memoryProperties,
    const VkMemoryRequirements&             memReqs,
    VkFlags                                 memProps,
    VkMemoryAllocateInfo&                   memInfo)
  {
    memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memInfo.pNext = nullptr;

    if (!memReqs.size)
    {
      memInfo.allocationSize = 0;
      memInfo.memoryTypeIndex = ~0;
      return true;
    }

    uint32_t memoryTypeIndex = PhysicalDeviceMemoryProperties_getMemoryTypeIndex(memoryProperties, memReqs, memProps);
    if (memoryTypeIndex == ~0)
    {
      return false;
    }

    memInfo.allocationSize = memReqs.size;
    memInfo.memoryTypeIndex = memoryTypeIndex;

    return true;
  }

  bool PhysicalDeviceMemoryProperties_appendMemoryAllocationInfo(const VkPhysicalDeviceMemoryProperties& memoryProperties,
    const VkMemoryRequirements&             memReqs,
    VkFlags                                 memProps,
    VkMemoryAllocateInfo&                   memInfoAppended,
    VkDeviceSize&                           offset)
  {
    VkMemoryAllocateInfo memInfo;
    if (!PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(memoryProperties, memReqs, memProps, memInfo))
    {
      return false;
    }
    if (memInfoAppended.allocationSize == 0)
    {
      memInfoAppended = memInfo;
      offset = 0;
      return true;
    }
    else if (memInfoAppended.memoryTypeIndex != memInfo.memoryTypeIndex)
    {
      return false;
    }
    else
    {
      offset = (memInfoAppended.allocationSize + memReqs.alignment - 1) & ~(memReqs.alignment - 1);
      memInfoAppended.allocationSize = offset + memInfo.allocationSize;
      return true;
    }
  }

  //////////////////////////////////////////////////////////////////////////


  void PhysicalInfo::init(VkPhysicalDevice physicalDeviceIn, uint32_t apiMajorIn, uint32_t apiMinorIn)
  {
    physicalDevice = physicalDeviceIn;
    apiMajor = apiMajorIn;
    apiMinor = apiMinorIn;

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
    queueProperties.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queueProperties.data());

    memset(&extFeatures, 0, sizeof(extFeatures));
    memset(&extProperties, 0, sizeof(extProperties));

    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    extFeatures.multiview.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
    extFeatures.t16BitStorage.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
    extFeatures.samplerYcbcrConversion.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES;
    extFeatures.protectedMemory.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES;
    extFeatures.drawParameters.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;
    extFeatures.variablePointers.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES;

    features2.pNext = &extFeatures.multiview;
    extFeatures.multiview.pNext = &extFeatures.t16BitStorage;
    extFeatures.t16BitStorage.pNext = &extFeatures.samplerYcbcrConversion;
    extFeatures.samplerYcbcrConversion.pNext = &extFeatures.protectedMemory;
    extFeatures.protectedMemory.pNext = &extFeatures.drawParameters;
    extFeatures.drawParameters.pNext = &extFeatures.variablePointers;
    extFeatures.variablePointers.pNext = nullptr;

    VkPhysicalDeviceProperties2 properties2;

    properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    extProperties.maintenance3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
    extProperties.deviceID.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
    extProperties.multiview.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;
    extProperties.protectedMemory.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES;
    extProperties.pointClipping.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES;
    extProperties.subgroup.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;

    properties2.pNext = &extProperties.maintenance3;
    extProperties.maintenance3.pNext = &extProperties.subgroup;
    extProperties.deviceID.pNext = &extProperties.multiview;
    extProperties.multiview.pNext = &extProperties.protectedMemory;
    extProperties.protectedMemory.pNext = &extProperties.pointClipping;
    extProperties.pointClipping.pNext = &extProperties.subgroup;
    extProperties.subgroup.pNext = nullptr;

    if (apiMajor == 1 && apiMinor > 0)
    {
      vkGetPhysicalDeviceFeatures2(physicalDeviceIn, &features2);
      vkGetPhysicalDeviceProperties2(physicalDeviceIn, &properties2);
    }
    else
    {
      vkGetPhysicalDeviceProperties(physicalDevice, &properties2.properties);
      vkGetPhysicalDeviceFeatures(physicalDevice, &features2.features);
    }

    properties = properties2.properties;
  }

  bool PhysicalInfo::getOptimalDepthStencilFormat(VkFormat& depthStencilFormat) const
  {
    VkFormat depthStencilFormats[] = {
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT,
    };

    for (VkFormat format : depthStencilFormats)
    {
      VkFormatProperties formatProps;
      vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
      // Format must support depth stencil attachment for optimal tiling
      if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
      {
        depthStencilFormat = format;
        return true;
      }
    }

    return false;
  }

  uint32_t PhysicalInfo::getExclusiveQueueFamily(VkQueueFlagBits bits) const
  {
    for (uint32_t i = 0; i < uint32_t(queueProperties.size()); i++)
    {
      if ((queueProperties[i].queueFlags & bits) == bits && !(queueProperties[i].queueFlags & ~bits))
      {
        return i;
      }
    }
    return VK_QUEUE_FAMILY_IGNORED;
  }

  uint32_t PhysicalInfo::getQueueFamily(VkQueueFlags bits /*= VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT*/) const
  {
    for (uint32_t i = 0; i < uint32_t(queueProperties.size()); i++)
    {
      if ((queueProperties[i].queueFlags & bits) == bits)
      {
        return i;
      }
    }
    return VK_QUEUE_FAMILY_IGNORED;
  }

  uint32_t PhysicalInfo::getPresentQueueFamily(VkSurfaceKHR surface, VkQueueFlags bits /*=VK_QUEUE_GRAPHICS_BIT*/) const
  {
    for (uint32_t i = 0; i < uint32_t(queueProperties.size()); i++)
    {
      VkBool32 supportsPresent;
      vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent);

      if (supportsPresent && ((queueProperties[i].queueFlags & bits) == bits))
      {
        return i;
      }
    }
    return VK_QUEUE_FAMILY_IGNORED;
  }

  uint32_t PhysicalInfo::getMemoryTypeIndex(const VkMemoryRequirements& memReqs, VkFlags memProps) const
  {
    return PhysicalDeviceMemoryProperties_getMemoryTypeIndex(memoryProperties, memReqs, memProps);
  }

  bool PhysicalInfo::getMemoryAllocationInfo(const VkMemoryRequirements& memReqs, VkFlags memProps, VkMemoryAllocateInfo& memInfo) const
  {
    if (apiMajor == 1 && apiMinor >= 1 && memReqs.size > extProperties.maintenance3.maxMemoryAllocationSize) {
      return false;
    }
    return PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(memoryProperties, memReqs, memProps, memInfo);
  }

  bool PhysicalInfo::appendMemoryAllocationInfo(const VkMemoryRequirements& memReqs,
    VkFlags                     memProps,
    VkMemoryAllocateInfo&       memInfoAppended,
    VkDeviceSize&               offset) const
  {
    if (apiMajor == 1 && apiMinor >= 1 && 
      ((memInfoAppended.allocationSize + memReqs.alignment - 1) & ~(memReqs.alignment - 1)) + memReqs.size > extProperties.maintenance3.maxMemoryAllocationSize) 
    {
      return false;
    }
    return PhysicalDeviceMemoryProperties_appendMemoryAllocationInfo(memoryProperties, memReqs, memProps, memInfoAppended, offset);
  }


}

