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


#include <algorithm>
#include <nvh/nvprint.hpp>
#include <regex>

#include "context_vk.hpp"
#include "error_vk.hpp"
#include "extensions_vk.hpp"

#include <nvvk/debug_util_vk.hpp>
#include <nvp/perproject_globals.hpp>

namespace nvvk {

// Would be used in Context::debugMessengerCallback.
#if 0
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
#if VK_NV_device_generated_commands
    case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV:
      return "IndirectCommandsLayoutNV";
#endif
    case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
      return "DebugUtilsMessengerEXT";
    case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:
      return "ValidationCacheEXT";
#if VK_NV_ray_tracing
    case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV:
      return "AccelerationStructureNV";
#endif
    default:
      return "invalid";
  }
}
#endif

// Define a callback to capture the messages
VKAPI_ATTR VkBool32 VKAPI_CALL Context::debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                               VkDebugUtilsMessageTypeFlagsEXT        messageType,
                                                               const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                               void*                                       userData)
{
  Context* ctx = reinterpret_cast<Context*>(userData);

  if(ctx->m_dbgIgnoreMessages.find(callbackData->messageIdNumber) != ctx->m_dbgIgnoreMessages.end())
    return VK_FALSE;

  // Check for severity: default ERROR and WARNING
  if((ctx->m_dbgSeverity & messageSeverity) != messageSeverity)
    return VK_FALSE;

  int level = LOGLEVEL_INFO;
  // repeating nvprintfLevel to help with breakpoints : so we can selectively break right after the print
  if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
  {
    nvprintfLevel(level, "VERBOSE: %s \n --> %s\n", callbackData->pMessageIdName, callbackData->pMessage);
  }
  else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
  {
    nvprintfLevel(level, "INFO: %s \n --> %s\n", callbackData->pMessageIdName, callbackData->pMessage);
  }
  else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    level = LOGLEVEL_WARNING;
    nvprintfLevel(level, "WARNING: %s \n --> %s\n", callbackData->pMessageIdName, callbackData->pMessage);
  }
  else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    level = LOGLEVEL_ERROR;
    nvprintfLevel(level, "ERROR: %s \n --> %s\n", callbackData->pMessageIdName, callbackData->pMessage);
  }
  else if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
  {
    nvprintfLevel(level, "GENERAL: %s \n --> %s\n", callbackData->pMessageIdName, callbackData->pMessage);
  }
  else
  {
    nvprintfLevel(level, "%s \n --> %s\n", callbackData->pMessageIdName, callbackData->pMessage);
  }

  // this seems redundant with the info already in callbackData->pMessage
#if 0
  
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
#endif
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
  if(compatibleDevices.empty())
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
  // #Aftermath - Initialization
  if(::isAftermathAvailable() && info.enableAftermath)
  {
    m_gpuCrashTracker.initialize();
  }

  VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  applicationInfo.pApplicationName = info.appTitle.c_str();
  applicationInfo.pEngineName      = info.appEngine.c_str();
  applicationInfo.apiVersion       = VK_MAKE_VERSION(info.apiMajor, info.apiMinor, 0);

  m_apiMajor = info.apiMajor;
  m_apiMinor = info.apiMinor;

  if(info.verboseUsed)
  {
    uint32_t version;
    VkResult result = vkEnumerateInstanceVersion(&version);
    NVVK_CHECK(result);
    LOGI("_______________\n");
    LOGI("Vulkan Version:\n");
    LOGI(" - available:  %d.%d.%d\n", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
    LOGI(" - requesting: %d.%d.%d\n", info.apiMajor, info.apiMinor, 0);
  }

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
        LOGI("%s (v. %d.%d.%d %x) : %s\n", it.layerName, VK_VERSION_MAJOR(it.specVersion),
             VK_VERSION_MINOR(it.specVersion), VK_VERSION_PATCH(it.specVersion), it.implementationVersion, it.description);
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
    for(const auto& it : m_usedInstanceLayers)
    {
      LOGI("%s\n", it.c_str());
    }
    LOGI("\n");
    LOGI("Used Instance Extensions :\n");
    for(const auto& it : m_usedInstanceExtensions)
    {
      LOGI("%s\n", it.c_str());
    }
  }


  std::vector<const char*> usedInstanceLayers;
  std::vector<const char*> usedInstanceExtensions;
  for(const auto& it : m_usedInstanceExtensions)
  {
    usedInstanceExtensions.push_back(it.c_str());
  }
  for(const auto& it : m_usedInstanceLayers)
  {
    usedInstanceLayers.push_back(it.c_str());
  }

  VkInstanceCreateInfo instanceCreateInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instanceCreateInfo.pApplicationInfo        = &applicationInfo;
  instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(usedInstanceExtensions.size());
  instanceCreateInfo.ppEnabledExtensionNames = usedInstanceExtensions.data();
  instanceCreateInfo.enabledLayerCount       = static_cast<uint32_t>(usedInstanceLayers.size());
  instanceCreateInfo.ppEnabledLayerNames     = usedInstanceLayers.data();
  instanceCreateInfo.pNext                   = info.instanceCreateInfoExt;

  NVVK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

  for(const auto& it : usedInstanceExtensions)
  {
    if(strcmp(it, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
    {
      initDebugUtils();
      break;
    }
  }

  return true;
}

void Context::initQueueList(QueueScoreList& list, const uint32_t* maxFamilyCounts, const float* priorities, uint32_t maxQueueCount) const
{
  for(uint32_t qF = 0; qF < m_physicalInfo.queueProperties.size(); ++qF)
  {
    const auto& queueFamily = m_physicalInfo.queueProperties[qF];
    QueueScore  score{0, qF, 0, 1.0f};

    for(uint32_t i = 0; i < 32; i++)
    {
      if(queueFamily.queueFlags & (1 << i))
      {
        score.score++;
      }
    }
    for(uint32_t qI = 0; qI < (maxFamilyCounts ? maxFamilyCounts[qF] : queueFamily.queueCount); ++qI)
    {
      score.queueIndex = qI;

      if(priorities)
      {
        score.priority = priorities[qF * maxQueueCount + qI];
      }

      list.emplace_back(score);
    }
  }

  // Sort the queues for specialization, highest specialization has lowest score
  std::sort(list.begin(), list.end(), [](const QueueScore& lhs, const QueueScore& rhs) {
    if(lhs.score < rhs.score)
      return true;
    if(lhs.score > rhs.score)
      return false;
    if(lhs.priority > rhs.priority)
      return true;
    if(lhs.priority < rhs.priority)
      return false;
    return lhs.queueIndex < rhs.queueIndex;
  });
}

nvvk::Context::QueueScore Context::removeQueueListItem(QueueScoreList& list, VkQueueFlags needFlags, float priority) const
{
  for(uint32_t q = 0; q < list.size(); ++q)
  {
    const QueueScore& score  = list[q];
    auto&             family = m_physicalInfo.queueProperties[score.familyIndex];
    if((family.queueFlags & needFlags) == needFlags && score.priority == priority)
    {
      QueueScore queue  = score;
      queue.familyIndex = score.familyIndex;
      queue.queueIndex  = score.queueIndex;
      // we used this queue
      list.erase(list.begin() + q);
      return queue;
    }
  }

  return {};
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

  initPhysicalInfo(m_physicalInfo, m_physicalDevice, info.apiMajor, info.apiMinor);


  //////////////////////////////////////////////////////////////////////////
  // queue setup

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::vector<float>                   priorities;

  {
    QueueScoreList queueScoresTemp;

    uint32_t maxQueueCount = 0;

    for(auto& it : m_physicalInfo.queueProperties)
    {
      maxQueueCount = std::max(maxQueueCount, it.queueCount);
    }
    // priorities is sized to easily address enough slots for each family
    priorities.resize(m_physicalInfo.queueProperties.size() * maxQueueCount);

    // init list with all maximum queue counts
    initQueueList(queueScoresTemp, nullptr, nullptr, 0);

    // figure out how many queues we need per family
    std::vector<uint32_t> queueFamilyCounts(m_physicalInfo.queueProperties.size(), 0);

    for(auto& it : info.requestedQueues)
    {
      // handle each request individually
      for(uint32_t i = 0; i < it.count; i++)
      {
        // in this pass we don't care about the real priority yet, queueList is initialized with 1.0f
        QueueScore queue = removeQueueListItem(queueScoresTemp, it.requiredFlags, 1.0f);
        if(!queue.score)
        {
          // there were not enough queues left supporting the required flags
          LOGE("could not setup requested queue configuration\n");
          return false;
        }

        priorities[queue.familyIndex * maxQueueCount + queueFamilyCounts[queue.familyIndex]] = it.priority;
        queueFamilyCounts[queue.familyIndex]++;
      }
    }

    // init requested families with appropriate family count
    for(uint32_t i = 0; i < m_physicalInfo.queueProperties.size(); ++i)
    {
      if(queueFamilyCounts[i])
      {
        VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex = i;
        queueInfo.queueCount       = queueFamilyCounts[i];
        queueInfo.pQueuePriorities = priorities.data() + (i * maxQueueCount);

        queueCreateInfos.push_back(queueInfo);
      }
    }

    // setup available queues, now with actual requested counts and available priorities
    initQueueList(m_availableQueues, queueFamilyCounts.data(), priorities.data(), maxQueueCount);
  }

  VkDeviceCreateInfo deviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
  deviceCreateInfo.pQueueCreateInfos    = queueCreateInfos.data();

  //////////////////////////////////////////////////////////////////////////
  // version features and physical device extensions

  VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
  Features11Old             features11old;
  std::vector<void*>        featureStructs;

  features2.features = m_physicalInfo.features10;
  features11old.read(m_physicalInfo.features11);

  if(info.apiMajor == 1 && info.apiMinor == 1)
  {
    features2.pNext = &features11old.multiview;
  }

  if(info.apiMajor == 1 && info.apiMinor >= 2)
  {
    features2.pNext                 = &m_physicalInfo.features11;
    m_physicalInfo.features11.pNext = &m_physicalInfo.features12;
    m_physicalInfo.features12.pNext = nullptr;
  }

  if(info.apiMajor == 1 && info.apiMinor >= 3)
  {
    m_physicalInfo.features12.pNext = &m_physicalInfo.features13;
    m_physicalInfo.features13.pNext = nullptr;
  }

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
    for(const auto& it : m_usedDeviceExtensions)
    {
      LOGI("%s\n", it.c_str());
    }
    LOGI("\n");
  }

  struct ExtensionHeader  // Helper struct to link extensions together
  {
    VkStructureType sType;
    void*           pNext;
  };

  // use the features2 chain to append extensions
  if(!featureStructs.empty())
  {
    // build up chain of all used extension features
    for(size_t i = 0; i < featureStructs.size(); i++)
    {
      auto* header  = reinterpret_cast<ExtensionHeader*>(featureStructs[i]);
      header->pNext = i < featureStructs.size() - 1 ? featureStructs[i + 1] : nullptr;
    }

    // append to the end of current feature2 struct chain
    ExtensionHeader* lastCoreFeature = (ExtensionHeader*)&features2;
    while(lastCoreFeature->pNext != nullptr)
    {
      lastCoreFeature = (ExtensionHeader*)lastCoreFeature->pNext;
    }
    lastCoreFeature->pNext = featureStructs[0];

    // query support
    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);
  }

  // run user callback to disable features
  if(info.fnDisableFeatures)
  {
    ExtensionHeader* featurePtr = (ExtensionHeader*)&features2;
    while(featurePtr)
    {
      info.fnDisableFeatures(featurePtr->sType, featurePtr);
      featurePtr = (ExtensionHeader*)featurePtr->pNext;
    }
  }

  // disable this feature through info directly
  if(info.disableRobustBufferAccess)
  {
    features2.features.robustBufferAccess = VK_FALSE;
  }


  std::vector<const char*> usedDeviceExtensions;
  for(const auto& it : m_usedDeviceExtensions)
  {
    usedDeviceExtensions.push_back(it.c_str());
  }

  deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(usedDeviceExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames = usedDeviceExtensions.data();

  // Vulkan >= 1.1 uses pNext to enable features, and not pEnabledFeatures
  deviceCreateInfo.pEnabledFeatures = nullptr;
  deviceCreateInfo.pNext            = &features2;

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
  if(info.deviceCreateInfoExt)
  {
    deviceCreateChain = (ExtensionHeader*)info.deviceCreateInfoExt;
    while(deviceCreateChain->pNext != nullptr)
    {
      deviceCreateChain = (ExtensionHeader*)deviceCreateChain->pNext;
    }
    // override last of external chain
    deviceCreateChain->pNext = (void*)deviceCreateInfo.pNext;
    deviceCreateInfo.pNext   = info.deviceCreateInfoExt;
  }

  VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);

  if(deviceCreateChain)
  {
    // reset last of external chain
    deviceCreateChain->pNext = nullptr;
  }

  if(result != VK_SUCCESS)
  {
    deinit();
    return false;
  }

  load_VK_EXTENSIONS(m_instance, vkGetInstanceProcAddr, m_device, vkGetDeviceProcAddr);

  nvvk::DebugUtil::setEnabled(hasDebugUtils());

  if(hasDeviceExtension(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME)
     || hasDeviceExtension(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME))
  {
    LOGW(
        "\n-------------------------------------------------------------------"
        "\nWARNING: Aftermath extensions enabled. This may affect performance."
        "\n-------------------------------------------------------------------\n\n");
  }
  else if(isAftermathAvailable() && info.enableAftermath)
  {
    LOGW(
        "\n--------------------------------------------------------------"
        "\nWARNING: Attempted to enable Aftermath extensions, but failed."
        "\n" VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME " or\n " VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME
        " not enabled or missing."
        "\n--------------------------------------------------------------\n\n");
  }

  m_queueGCT = createQueue(info.defaultQueueGCT, "queueGCT", info.defaultPriorityGCT);
  m_queueC   = createQueue(info.defaultQueueC, "queueC", info.defaultPriorityC);
  m_queueT   = createQueue(info.defaultQueueT, "queueT", info.defaultPriorityT);

  return true;
}

nvvk::Context::Queue Context::createQueue(VkQueueFlags requiredFlags, const std::string& debugName, float priority)
{
  if(!requiredFlags || m_availableQueues.empty())
  {
    return Queue();
  }

  QueueScore score = removeQueueListItem(m_availableQueues, requiredFlags, priority);
  if(!score.score)
  {
    return Queue();
  }

  nvvk::DebugUtil debugUtil(m_device);
  Queue           queue;
  queue.familyIndex = score.familyIndex;
  queue.queueIndex  = score.queueIndex;
  queue.priority    = score.priority;
  vkGetDeviceQueue(m_device, queue.familyIndex, queue.queueIndex, &queue.queue);
  debugUtil.setObjectName(queue.queue, debugName);

  return queue;
}

//--------------------------------------------------------------------------------------------------
// returns if GCTQueue supports present \p surface
//
bool Context::setGCTQueueWithPresent(VkSurfaceKHR surface)
{
  VkBool32 supportsPresent;
  vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, m_queueGCT.familyIndex, surface, &supportsPresent);
  return supportsPresent == VK_TRUE;
}

//--------------------------------------------------------------------------------------------------
// Destructor
//
void Context::deinit()
{
  if(m_device)
  {
    VkResult result = vkDeviceWaitIdle(m_device);
    if(nvvk::checkResult(result, __FILE__, __LINE__))
    {
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

  nvvk::DebugUtil::setEnabled(false);
}

bool Context::hasDeviceExtension(const char* name) const
{
  for(const auto& it : m_usedDeviceExtensions)
  {
    if(strcmp(name, it.c_str()) == 0)
    {
      return true;
    }
  }
  return false;
}

bool Context::hasInstanceExtension(const char* name) const
{
  for(const auto& it : m_usedInstanceExtensions)
  {
    if(strcmp(name, it.c_str()) == 0)
    {
      return true;
    }
  }
  return false;
}


//--------------------------------------------------------------------------------------------------
//
//
ContextCreateInfo::ContextCreateInfo(bool bUseValidation, VkDeviceDiagnosticsConfigFlagsNV aftermathFlags)
{
  if(defaultQueueGCT)
  {
    requestedQueues.push_back({defaultQueueGCT, 1, defaultPriorityGCT});
  }
  if(defaultQueueT)
  {
    requestedQueues.push_back({defaultQueueT, 1, defaultPriorityT});
  }
  if(defaultQueueC)
  {
    requestedQueues.push_back({defaultQueueC, 1, defaultPriorityC});
  }

#ifndef NDEBUG
  instanceExtensions.push_back({VK_EXT_DEBUG_UTILS_EXTENSION_NAME, true});
  if(bUseValidation)
    instanceLayers.push_back({"VK_LAYER_KHRONOS_validation", true});
#endif

  this->enableAftermath = !!aftermathFlags;
  if(isAftermathAvailable() && this->enableAftermath)
  {
    // #Aftermath
    // Set up device creation info for Aftermath feature flag configuration.

    // BEWARE: we need to use static here, because addDeviceExtension doesn't make a copy of aftermathInfo.
    // The pointer to aftermathInfo MUST be valid until initDevice() time!!!
    static VkDeviceDiagnosticsConfigCreateInfoNV aftermathInfo{VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV};
    aftermathInfo.flags = aftermathFlags;
    // Enable NV_device_diagnostic_checkpoints extension to be able to use Aftermath event markers.
    addDeviceExtension(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME, true);
    // Enable NV_device_diagnostics_config extension to configure Aftermath features.
    addDeviceExtension(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME, true, &aftermathInfo);
  }
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
// pFeatureStruct must be provided if a feature structure exists for the given extension.
// The *pFeatureStruct struct will be filled out by querying the physical device.
// The filled struct will then be passed to device create info.
//
void ContextCreateInfo::addDeviceExtension(const char* name, bool optional, void* pFeatureStruct, uint32_t version)
{
  deviceExtensions.emplace_back(name, optional, pFeatureStruct, version);
}

void ContextCreateInfo::removeInstanceExtension(const char* name)
{
  for(size_t i = 0; i < instanceExtensions.size(); i++)
  {
    if(strcmp(instanceExtensions[i].name.c_str(), name) == 0)
    {
      instanceExtensions.erase(instanceExtensions.begin() + i);
    }
  }
}

void ContextCreateInfo::removeInstanceLayer(const char* name)
{
  for(size_t i = 0; i < instanceLayers.size(); i++)
  {
    if(strcmp(instanceLayers[i].name.c_str(), name) == 0)
    {
      instanceLayers.erase(instanceLayers.begin() + i);
    }
  }
}

void ContextCreateInfo::removeDeviceExtension(const char* name)
{
  for(size_t i = 0; i < deviceExtensions.size(); i++)
  {
    if(strcmp(deviceExtensions[i].name.c_str(), name) == 0)
    {
      deviceExtensions.erase(deviceExtensions.begin() + i);
    }
  }
}

void ContextCreateInfo::addRequestedQueue(VkQueueFlags flags, uint32_t count /*= 1*/, float priority /*= 1.0f*/)
{
  requestedQueues.push_back({flags, count, priority});
}

void ContextCreateInfo::setVersion(uint32_t major, uint32_t minor)
{
  assert(apiMajor == 1 && apiMinor >= 1);
  apiMajor = major;
  apiMinor = minor;
}

VkResult Context::fillFilteredNameArray(std::vector<std::string>&             used,
                                        const std::vector<VkLayerProperties>& properties,
                                        const ContextCreateInfo::EntryArray&  requested)
{
  for(const auto& itr : requested)
  {
    bool found = false;
    for(const auto& property : properties)
    {
      if(strcmp(itr.name.c_str(), property.layerName) == 0)
      {
        found = true;
        break;
      }
    }

    if(found)
    {
      used.push_back(itr.name);
    }
    else if(itr.optional == false)
    {
      LOGE("Requiered layer not found: %s\n", itr.name.c_str());
      return VK_ERROR_LAYER_NOT_PRESENT;
    }
  }

  return VK_SUCCESS;
}  // namespace nvvk


VkResult Context::fillFilteredNameArray(std::vector<std::string>&                 used,
                                        const std::vector<VkExtensionProperties>& properties,
                                        const ContextCreateInfo::EntryArray&      requested,
                                        std::vector<void*>&                       featureStructs)
{
  for(const auto& itr : requested)
  {
    bool found = false;
    for(const auto& property : properties)
    {
      if(strcmp(itr.name.c_str(), property.extensionName) == 0 && (itr.version == 0 || itr.version == property.specVersion))
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
      LOGW("VK_ERROR_EXTENSION_NOT_PRESENT: %s - %d\n", itr.name.c_str(), itr.version);
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
  }

  return VK_SUCCESS;
}

void Context::initPhysicalInfo(PhysicalDeviceInfo& info, VkPhysicalDevice physicalDevice, uint32_t versionMajor, uint32_t versionMinor)
{
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &info.memoryProperties);
  uint32_t count;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
  info.queueProperties.resize(count);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, info.queueProperties.data());

  // for queries and device creation
  VkPhysicalDeviceFeatures2   features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
  VkPhysicalDeviceProperties2 properties2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  Properties11Old             properties11old;
  Features11Old               features11old;

  if(versionMajor == 1 && versionMinor == 1)
  {
    features2.pNext   = &features11old.multiview;
    properties2.pNext = &properties11old.maintenance3;
  }
  else if(versionMajor == 1 && versionMinor >= 2)
  {
    features2.pNext       = &info.features11;
    info.features11.pNext = &info.features12;
    info.features12.pNext = nullptr;

    info.properties12.driverID                     = VK_DRIVER_ID_NVIDIA_PROPRIETARY;
    info.properties12.supportedDepthResolveModes   = VK_RESOLVE_MODE_MAX_BIT;
    info.properties12.supportedStencilResolveModes = VK_RESOLVE_MODE_MAX_BIT;

    properties2.pNext       = &info.properties11;
    info.properties11.pNext = &info.properties12;
    info.properties12.pNext = nullptr;
  }

  if(versionMajor == 1 && versionMinor >= 3)
  {
    info.features12.pNext   = &info.features13;
    info.features13.pNext   = nullptr;
    info.properties12.pNext = &info.properties13;
    info.properties13.pNext = nullptr;
  }

  vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
  vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);

  info.properties10 = properties2.properties;
  info.features10   = features2.features;

  if(versionMajor == 1 && versionMinor == 1)
  {
    properties11old.write(info.properties11);
    features11old.write(info.features11);
  }
}

void Context::initDebugUtils()
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
    VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    dbg_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT       // For debug printf
                                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT  // GPU info, bug
                                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;   // Invalid usage
    dbg_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT            // Other
                                            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT       // Violation of spec
                                            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;     // Non-optimal use
    dbg_messenger_create_info.pfnUserCallback = debugMessengerCallback;
    dbg_messenger_create_info.pUserData       = this;
    NVVK_CHECK(m_createDebugUtilsMessengerEXT(m_instance, &dbg_messenger_create_info, nullptr, &m_dbgMessenger));
  }
}

//--------------------------------------------------------------------------------------------------
// Returns the list of devices or groups compatible with the mandatory extensions
//
std::vector<uint32_t> Context::getCompatibleDevices(const ContextCreateInfo& info)
{
  assert(m_instance != nullptr);

  std::vector<std::pair<bool, uint32_t>>       compatibleDevices;
  std::vector<uint32_t>                        sortedDevices;
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

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);

    bool discreteGpu = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    // Note: all physical devices in a group are identical
    if(hasMandatoryExtensions(physicalDevice, info, info.verboseCompatibleDevices))
    {
      compatibleDevices.emplace_back(discreteGpu, elemId);
      if(info.verboseCompatibleDevices)
      {

        LOGI("%d: %s\n", compatible, props.deviceName);
        compatible++;
      }
    }
    else if(info.verboseCompatibleDevices)
    {
      LOGI("Skipping physical device %s\n", props.deviceName);
    }
  }
  if(info.verboseCompatibleDevices)
  {
    LOGI("Physical devices found : ");
    if(compatible > 0)
    {
      LOGI("%d\n", compatible);
    }
    else
    {
      LOGE("OMG... NONE !!\n");
    }
  }

  // Sorting by discrete GPU
  std::sort(compatibleDevices.begin(), compatibleDevices.end(), [](auto a, auto b) { return a.first > b.first; });
  sortedDevices.reserve(compatibleDevices.size());
  for(const auto& d : compatibleDevices)
    sortedDevices.push_back(d.second);

  return sortedDevices;
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
  extensionProperties.resize(std::min(extensionProperties.size(), size_t(count)));

  return checkEntryArray(extensionProperties, info.deviceExtensions, bVerbose);
}

bool Context::checkEntryArray(const std::vector<VkExtensionProperties>& properties, const ContextCreateInfo::EntryArray& requested, bool bVerbose)
{
  for(const auto& itr : requested)
  {
    bool found = false;
    for(const auto& property : properties)
    {
      if(strcmp(itr.name.c_str(), property.extensionName) == 0)
      {
        found = true;
        break;
      }
    }

    if(!found && !itr.optional)
    {
      if(bVerbose)
      {
        LOGW("Could NOT locate mandatory extension '%s'\n", itr.name.c_str());
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
  layerProperties.resize(std::min(layerProperties.size(), size_t(count)));
  return layerProperties;
}

std::vector<VkExtensionProperties> Context::getInstanceExtensions()
{
  uint32_t                           count;
  std::vector<VkExtensionProperties> extensionProperties;
  NVVK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
  extensionProperties.resize(count);
  NVVK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensionProperties.data()));
  extensionProperties.resize(std::min(extensionProperties.size(), size_t(count)));
  return extensionProperties;
}

std::vector<VkExtensionProperties> Context::getDeviceExtensions(VkPhysicalDevice physicalDevice)
{
  uint32_t                           count;
  std::vector<VkExtensionProperties> extensionProperties;
  NVVK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr));
  extensionProperties.resize(count);
  NVVK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensionProperties.data()));
  extensionProperties.resize(std::min(extensionProperties.size(), size_t(count)));
  return extensionProperties;
}

}  // namespace nvvk
