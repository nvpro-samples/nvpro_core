/*-----------------------------------------------------------------------
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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
*/ //--------------------------------------------------------------------

#include "resources.h"

#define DECL_WININTERNAL
#include "main.h"

#define VK_USE_PLATFORM_XLIB_KHR
#include "vulkan/vulkan.h"
#ifdef USEVULKANSDK
#include "vulkan/vk_sdk_platform.h"
#endif

#include <stdio.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

#include "wininternal_win32_vk.hpp"


extern std::vector<NVPWindow *> g_windows;

template <typename T, size_t sz> inline uint32_t getArraySize(T(&t)[sz]) { return sz; }

//------------------------------------------------------------------------------
// allocator for this
//------------------------------------------------------------------------------
WINinternal* newWINinternalVK(NVPWindow *win)
{
    return new WINinternalVK(win);
}

//------------------------------------------------------------------------------
// Vulkan callback
//------------------------------------------------------------------------------
VkBool32 vulkanDbgFunc(
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  objectType,
    uint64_t                                    object,
    size_t                                      location,
    int32_t                                     messageCode,
    const char*                                 pLayerPrefix,
    const char*                                 pMessage,
    void*                                       pUserData)
{
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        if (strstr(pMessage, "Shader is not SPIR-V"))
        {
            return false;
        }
        LOGE("ERROR: [%s] Code %d : %s\n", pLayerPrefix, messageCode, pMessage);
        return false;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        //if (strstr(pMsg, "vkQueueSubmit parameter, VkFence fence, is null pointer"))
        //{
        //    return false;
        //}
        LOGW("WARNING: [%s] Code %d : %s\n", pLayerPrefix, messageCode, pMessage);
        return false;
    } else {
        return false;
    }

    return false;
}
//------------------------------------------------------------------------------
// Swapchain extension
//------------------------------------------------------------------------------
void WINinternalVK::init_swapchain_extension()
{

    VkResult res;

// Construct the surface description:
    VkXlibSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    //dpy
    createInfo.dpy = m_dpy;
    //Window
    createInfo.window = m_window;

    res = vkCreateXlibSurfaceKHR(m_instance, &createInfo, NULL, &m_surface);
    

    assert(res == VK_SUCCESS);

    // Iterate over each queue to learn whether it supports presenting:
    VkBool32 *supportsPresent =
        (VkBool32 *)malloc(m_gpu.queueProperties.size() * sizeof(VkBool32));
    for (uint32_t i = 0; i < m_gpu.queueProperties.size(); i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu.device, i, m_surface,
                                             &supportsPresent[i]);
    }

    // Search for a graphics queue and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < m_gpu.queueProperties.size(); i++) {
        if ((m_gpu.queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueNodeIndex = i;
                break;
            }
        }
    }
    free(supportsPresent);

    // Generate error if could not find a queue that supports both a graphics
    // and present
    if (graphicsQueueNodeIndex == UINT32_MAX) {
        std::cout
            << "Could not find a queue that supports both graphics and present";
        exit(-1);
    }

    m_gpu.graphics_queue_family_index = graphicsQueueNodeIndex;

}
void NVkSwapChain::init( VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface )
{
    m_surface = surface;
    m_device = device;
    m_physicalDevice = physicalDevice;

    m_currentSemaphore = 0;
    VkResult result;

    // Get the list of VkFormat's that are supported:
    uint32_t formatCount;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface,
      &formatCount, NULL);
    assert(!result);
    VkSurfaceFormatKHR *surfFormats =
      (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface,
      &formatCount, surfFormats);
    assert(!result);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
      m_surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
      assert(formatCount >= 1);
      m_surfaceFormat = surfFormats[0].format;
    }
    m_surfaceColor = surfFormats[0].colorSpace;
}

void NVkSwapChain::update(int width, int height)
{
    VkResult err;
    VkSwapchainKHR oldSwapchain = m_swapchain;

    // Check the surface capabilities and formats
    VkSurfaceCapabilitiesKHR surfCapabilities;
    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      m_physicalDevice, m_surface, &surfCapabilities);
    assert(!err);

    uint32_t presentModeCount;
    err = vkGetPhysicalDeviceSurfacePresentModesKHR(
      m_physicalDevice, m_surface, &presentModeCount, NULL);
    assert(!err);
    VkPresentModeKHR *presentModes =
      (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));
    assert(presentModes);
    err = vkGetPhysicalDeviceSurfacePresentModesKHR(
      m_physicalDevice, m_surface, &presentModeCount, presentModes);
    assert(!err);

    VkExtent2D swapchainExtent;
    // width and height are either both -1, or both not -1.
    if (surfCapabilities.currentExtent.width == (uint32_t)-1) {
      // If the surface size is undefined, the size is set to
      // the size of the images requested.
      swapchainExtent.width  = width;
      swapchainExtent.height = height;
    } else {
      // If the surface size is defined, the swap chain size must match
      swapchainExtent = surfCapabilities.currentExtent;

      //demo->width = surfCapabilities.currentExtent.width;
      //demo->height = surfCapabilities.currentExtent.height;
    }

    // If mailbox mode is available, use it, as is the lowest-latency non-
    // tearing mode.  If not, try IMMEDIATE which will usually be available,
    // and is fastest (though it tears).  If not, fall back to FIFO which is
    // always available.
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (size_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
            (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    // Determine the number of VkImage's to use in the swap chain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint32_t desiredNumberOfSwapchainImages =
      surfCapabilities.minImageCount + 2;
    if ((surfCapabilities.maxImageCount > 0) &&
      (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount)) {
        // Application must settle for fewer images than desired:
        desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfCapabilities.supportedTransforms &
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
      preTransform = surfCapabilities.currentTransform;
    }

    VkSwapchainCreateInfoKHR swapchain = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain.surface = m_surface;
    swapchain.minImageCount = desiredNumberOfSwapchainImages;
    swapchain.imageFormat = m_surfaceFormat;
    swapchain.imageColorSpace = m_surfaceColor;
    swapchain.imageExtent = swapchainExtent;
    swapchain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain.preTransform = preTransform;
    swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain.imageArrayLayers = 1;
    swapchain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;//VK_SHARING_MODE_CONCURRENT;
    swapchain.queueFamilyIndexCount = 0;
    swapchain.pQueueFamilyIndices = NULL;
    swapchain.presentMode = swapchainPresentMode;
    swapchain.oldSwapchain = oldSwapchain;
    swapchain.clipped = true;

    err = vkCreateSwapchainKHR(m_device, &swapchain, NULL,
      &m_swapchain);
    assert(!err);

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if (oldSwapchain != VK_NULL_HANDLE) {
      vkDestroySwapchainKHR(m_device, oldSwapchain, NULL);
    }

    err = vkGetSwapchainImagesKHR(m_device, m_swapchain,
      &m_swapchainImageCount, NULL);
    assert(!err);

    m_swapchainImages.resize( m_swapchainImageCount );

    err = vkGetSwapchainImagesKHR(m_device, m_swapchain,
      &m_swapchainImageCount,
      m_swapchainImages.data());
    assert(!err);
    //
    // Image views
    //
    m_swapchainImageViews.resize( m_swapchainImageCount );
    for (uint32_t i = 0; i < m_swapchainImageCount; i++){
        VkImageViewCreateInfo ci =
        {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            NULL,
            0,
            m_swapchainImages[i],
            VK_IMAGE_VIEW_TYPE_2D,
            m_surfaceFormat,
            {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
            { VK_IMAGE_ASPECT_COLOR_BIT, 0,1,0,1 }
        };
        err = vkCreateImageView(m_device, &ci, NULL, &m_swapchainImageViews[i]);
    }

    if (NULL != presentModes) {
      free(presentModes);
    }

#define USE_PRESENT_SEMAPHORES 0

    bool deleteOld = !m_swapchainSemaphores.empty();
    m_swapchainSemaphores.resize( m_swapchainImageCount * 2);
    for (uint32_t i = 0; i < m_swapchainImageCount * 2; i++){
#if USE_PRESENT_SEMAPHORES
      VkSemaphoreCreateInfo semCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
      if (deleteOld){
        vkDestroySemaphore(m_device, m_swapchainSemaphores[i], NULL);
      }
      err = vkCreateSemaphore(m_device, &semCreateInfo, NULL, &m_swapchainSemaphores[i]);
      assert(!err);
#else
        m_swapchainSemaphores[i] = NULL;
#endif
    }
    m_currentSemaphore = 0;
    m_currentImage = 0;
}

void NVkSwapChain::deinit()
{
    for (uint32_t i = 0; i < m_swapchainImageCount; i++)
    {
        vkDestroyImageView(m_device, m_swapchainImageViews[i], NULL);
    }
    m_swapchainImageViews.clear();
    m_swapchainImages.clear();
    m_swapchainImageCount = 0;
    vkDestroySwapchainKHR(m_device, m_swapchain, NULL);
    for (uint32_t i = 0; i < m_swapchainImageCount * 2; i++){
      vkDestroySemaphore(m_device, m_swapchainSemaphores[i], NULL);
    }
}

void NVkSwapChain::acquire()
{
    VkSemaphore semaphore = getActiveReadSemaphore();
    VkResult result;
    result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
      semaphore,
      (VkFence)0,
      &m_currentImage);

    assert(result == VK_SUCCESS);
}

VkSemaphore NVkSwapChain::getActiveWrittenSemaphore()
{
    return m_swapchainSemaphores[(m_currentSemaphore % m_swapchainImageCount)*2 + 1];
}

VkSemaphore NVkSwapChain::getActiveReadSemaphore()
{
    return m_swapchainSemaphores[(m_currentSemaphore % m_swapchainImageCount)*2 + 0];
}

VkImage NVkSwapChain::getActiveImage()
{
    return m_swapchainImages[m_currentImage];
}

void NVkSwapChain::present( VkQueue queue )
{
    VkResult result;
    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    VkSemaphore written = getActiveWrittenSemaphore();
    presentInfo.swapchainCount = 1;
    if (written){
      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores = &written;
    }
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_currentImage;
    
    result = vkQueuePresentKHR(queue, &presentInfo);
    assert(result == VK_SUCCESS);

    m_currentSemaphore++;
}

//------------------------------------------------------------------------------
// TODO:
// - cflags could contain the layers we want to activate
// - choice to make on queue: shall we leave it to the sample ?
//------------------------------------------------------------------------------
bool WINinternalVK::initBase(const NVPWindow::ContextFlagsVK* cflags, NVPWindow* sourcewindow)
{
    NVPWindow::ContextFlagsVK  settings;
    if (cflags){
      settings = *cflags;
    } else {
        // let's assume that if no cflags given, the sample will take care of everything
        return true;
    }

    ::VkResult result;
    ::VkApplicationInfo         appInfo         = { VK_STRUCTURE_TYPE_APPLICATION_INFO, NULL };
    ::VkInstanceCreateInfo      instanceInfo    = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, NULL };
    ::VkDeviceQueueCreateInfo   queueInfo       = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, NULL };
    int                         queueFamilyIndex= 0;
    ::VkDeviceCreateInfo        devInfo         = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, NULL };
    std::vector<char*> extension_names;
    std::vector<VkLayerProperties>      instance_layers;
    std::vector<VkPhysicalDevice>       physical_devices;
    std::vector<VkExtensionProperties>  device_extensions;
    std::vector<VkLayerProperties>      device_layers;
    std::vector<VkExtensionProperties>  instance_extensions;
    uint32_t count = 0;

    //
    // Instance layers
    //
    result = vkEnumerateInstanceLayerProperties(&count, NULL);
    assert(result == VK_SUCCESS);
    if(count > 0)
    {
        instance_layers.resize(count);
        result = vkEnumerateInstanceLayerProperties(&count, &instance_layers[0]);
        for(int i=0; i<instance_layers.size(); i++)
        {
            LOGI("%d: Layer %s: %s\n", i, instance_layers[i].layerName, instance_layers[i].description);
        }
    }
    //
    // Instance Extensions
    //
    result = vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
    assert(result == VK_SUCCESS);
    instance_extensions.resize(count);
    extension_names.resize(count);
    result = vkEnumerateInstanceExtensionProperties(NULL, &count, &instance_extensions[0]);
    for(int i=0; i<instance_extensions.size(); i++)
    {
        LOGI("%d: Extension %s\n", i, instance_extensions[i].extensionName);
        extension_names[i] = instance_extensions[i].extensionName;
    }
    #define CHECK_LAYER(l) if(settings.layers & (uint32_t)NVPWindow::ContextFlagsVK::l)\
        { instance_validation_layers.push_back(#l); }
    std::vector<char *> instance_validation_layers;
    CHECK_LAYER(VK_LAYER_LUNARG_api_dump)
    CHECK_LAYER(VK_LAYER_LUNARG_core_validation)
    CHECK_LAYER(VK_LAYER_LUNARG_device_limits)
    CHECK_LAYER(VK_LAYER_LUNARG_image)
    CHECK_LAYER(VK_LAYER_LUNARG_object_tracker)
    CHECK_LAYER(VK_LAYER_LUNARG_parameter_validation)
    CHECK_LAYER(VK_LAYER_LUNARG_screenshot)
    CHECK_LAYER(VK_LAYER_LUNARG_swapchain)
    CHECK_LAYER(VK_LAYER_GOOGLE_threading)
    CHECK_LAYER(VK_LAYER_GOOGLE_unique_objects)
    CHECK_LAYER(VK_LAYER_LUNARG_vktrace)
    CHECK_LAYER(VK_LAYER_RENDERDOC_Capture)
    CHECK_LAYER(VK_LAYER_LUNARG_standard_validation)

    static uint32_t instance_validation_layers_sz = instance_validation_layers.size();
    //
    // Instance creation
    //
    appInfo.pApplicationName = "...";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "...";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;
    // add some layers here ?
    instanceInfo.enabledLayerCount = instance_validation_layers_sz;
    instanceInfo.ppEnabledLayerNames = instance_validation_layers_sz > 0 ? &instance_validation_layers[0] : NULL;
    instanceInfo.enabledExtensionCount = (uint32_t)extension_names.size();
    instanceInfo.ppEnabledExtensionNames = &extension_names[0];

    result = vkCreateInstance(&instanceInfo, NULL, &m_instance);
    if(result == VK_ERROR_INCOMPATIBLE_DRIVER) {
        LOGE("Cannot find a compatible Vulkan installable client driver");
    } else if (result == VK_ERROR_EXTENSION_NOT_PRESENT) {
        LOGE("Cannot find a specified extension library");
    }
    if (result != VK_SUCCESS)
        return false;
    //
    // Callbacks
    //
    //if (1)
    {
        m_CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
        m_DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT");
        if (!m_CreateDebugReportCallback) {
            LOGE("GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT\n");
        }
        if (!m_DestroyDebugReportCallback) {
            LOGE("GetProcAddr: Unable to find vkDestroyDebugReportCallbackEXT\n");
        }
        m_DebugReportMessage = (PFN_vkDebugReportMessageEXT) vkGetInstanceProcAddr(m_instance, "vkDebugReportMessageEXT");
        if (!m_DebugReportMessage) {
            LOGE("GetProcAddr: Unable to find vkDebugReportMessageEXT\n");
        }
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = NULL;
        dbgCreateInfo.pfnCallback = vulkanDbgFunc;
        dbgCreateInfo.pUserData = NULL;
        dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        result = m_CreateDebugReportCallback(
                  m_instance,
                  &dbgCreateInfo,
                  NULL,
                  &m_msg_callback);
        switch (result) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            LOGE("CreateDebugReportCallback: out of host memory");
            return false;
            break;
        default:
            LOGE("CreateDebugReportCallback: unknown failure\n");
            return false;
            break;
        }
    }

    //
    // Physical device infos
    //
    result = vkEnumeratePhysicalDevices(m_instance, &count, NULL);
    physical_devices.resize(count);
    result = vkEnumeratePhysicalDevices(m_instance, &count, &physical_devices[0]);
    if (result != VK_SUCCESS) {
        return false;
    }
    LOGI("found %d Physical Devices (using device 0)\n", physical_devices.size());
    for(int j=0; j<physical_devices.size(); j++)
    {
        // redundant for displaying infos
        vkGetPhysicalDeviceProperties(physical_devices[j], &m_gpu.properties);
        LOGI("API ver. %x; driver ver. %x; VendorID %x; DeviceID %x; deviceType %x; Device Name: %s\n",
            m_gpu.properties.apiVersion, 
            m_gpu.properties.driverVersion, 
            m_gpu.properties.vendorID, 
            m_gpu.properties.deviceID, 
            m_gpu.properties.deviceType, 
            m_gpu.properties.deviceName);
        vkGetPhysicalDeviceMemoryProperties(physical_devices[j], &m_gpu.memoryProperties);
        for(uint32_t i=0; i<m_gpu.memoryProperties.memoryTypeCount; i++)
        {
            LOGI("Memory type %d: heapindex:%d %s|%s|%s|%s|%s\n", i,
                m_gpu.memoryProperties.memoryTypes[i].heapIndex,
                m_gpu.memoryProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT?"VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT":"",
                m_gpu.memoryProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT?"VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT":"",
                m_gpu.memoryProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT?"VK_MEMORY_PROPERTY_HOST_COHERENT_BIT":"",
                m_gpu.memoryProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_CACHED_BIT?"VK_MEMORY_PROPERTY_HOST_CACHED_BIT":"",
                m_gpu.memoryProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT?"VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT":""
            );
        }
        for(uint32_t i=0; i<m_gpu.memoryProperties.memoryHeapCount; i++)
        {
            LOGI("Memory heap %d: size:%uMb %s\n", i,
                m_gpu.memoryProperties.memoryHeaps[i].size/(1024*1024),
                m_gpu.memoryProperties.memoryHeaps[i].flags&VK_MEMORY_HEAP_DEVICE_LOCAL_BIT?"VK_MEMORY_HEAP_DEVICE_LOCAL_BIT":""
            );
        }
    m_gpu.memoryProperties.memoryHeaps[VK_MAX_MEMORY_HEAPS];
        vkGetPhysicalDeviceFeatures(physical_devices[j], &m_gpu.features); // too many stuff to display
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[j], &count, NULL);
        m_gpu.queueProperties.resize(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[j], &count, &m_gpu.queueProperties[0]);
        for(int i=0; i<m_gpu.queueProperties.size(); i++)
        {
            LOGI("%d: Queue count %d; flags: %s|%s|%s|%s\n", i, m_gpu.queueProperties[i].queueCount,
                (m_gpu.queueProperties[i].queueFlags&VK_QUEUE_GRAPHICS_BIT ? "VK_QUEUE_GRAPHICS_BIT":""),
                (m_gpu.queueProperties[i].queueFlags&VK_QUEUE_COMPUTE_BIT?"VK_QUEUE_COMPUTE_BIT":""),
                (m_gpu.queueProperties[i].queueFlags&VK_QUEUE_TRANSFER_BIT?"VK_QUEUE_TRANSFER_BIT":""),
                (m_gpu.queueProperties[i].queueFlags&VK_QUEUE_SPARSE_BINDING_BIT?"VK_QUEUE_SPARSE_BINDING_BIT":"")
                );
        }
        //
        // Device Layer Properties
        //
        result = vkEnumerateDeviceLayerProperties(physical_devices[j], &count, NULL);
        assert(!result);
        if(count > 0)
        {
            device_layers.resize(count);
            result = vkEnumerateDeviceLayerProperties(physical_devices[j], &count, &device_layers[0]);
            for(int i=0; i<device_layers.size(); i++)
            {
                LOGI("%d: Device layer %s: %s\n", i,device_layers[i].layerName, device_layers[i].description);
            }
        }
        result = vkEnumerateDeviceExtensionProperties(physical_devices[j], NULL, &count, NULL);
        assert(!result);
        device_extensions.resize(count);
        result = vkEnumerateDeviceExtensionProperties(physical_devices[j], NULL, &count, &device_extensions[0]);
        extension_names.resize(device_extensions.size() );
        for(int i=0; i<device_extensions.size(); i++)
        {
            LOGI("%d: available HW Device Extension: %s\n", i,device_extensions[i].extensionName);
            extension_names[i] = device_extensions[i].extensionName;
        }
    }
    //
    // Take one physical device
    //
    m_gpu.device = physical_devices[0];
    vkGetPhysicalDeviceProperties(m_gpu.device, &m_gpu.properties);
    vkGetPhysicalDeviceMemoryProperties(m_gpu.device, &m_gpu.memoryProperties);
    vkGetPhysicalDeviceFeatures(m_gpu.device, &m_gpu.features);
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu.device, &count, NULL);
    m_gpu.queueProperties.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu.device, &count, &m_gpu.queueProperties[0]);
    //
    // retain the queue type that can do graphics
    //
    for(uint32_t i=0; i<count; i++)
    {
        VkQueueFlags fl = m_gpu.queueProperties[i].queueFlags;
        LOGI("%d available queues of family %d capabilities: ", m_gpu.queueProperties[i].queueCount, i);
        if(fl & VK_QUEUE_GRAPHICS_BIT) { LOGI("GRAPHICS; "); }
        if(fl & VK_QUEUE_COMPUTE_BIT) { LOGI("COMPUTE; "); }
        if(fl & VK_QUEUE_TRANSFER_BIT) { LOGI("TRANSFER; "); }
        if(fl & VK_QUEUE_SPARSE_BINDING_BIT) { LOGI("SPARSE_BINDING; "); }
        LOGI("\n");
        if(fl & VK_QUEUE_GRAPHICS_BIT) // take the first queue that can handle graphics
        {
            queueFamilyIndex = i;
            //break;
        }
    }
    //
    // DeviceLayer Props
    //
    result = vkEnumerateDeviceLayerProperties(m_gpu.device, &count, NULL);
    assert(!result);
    if(count > 0)
    {
        device_layers.resize(count);
        result = vkEnumerateDeviceLayerProperties(m_gpu.device, &count, &device_layers[0]);
    }
    result = vkEnumerateDeviceExtensionProperties(m_gpu.device, NULL, &count, NULL);
    assert(!result);
    //
    // Device extensions
    //
    device_extensions.resize(count);
    result = vkEnumerateDeviceExtensionProperties(m_gpu.device, NULL, &count, &device_extensions[0]);
    //
    // Check if the requested extensions are available
    //
    for(int i=0; i<settings.extensions.size(); i++)
    {
        int j;
        for(j=0; j<count; j++)
        {
            if(strcmp(device_extensions[j].extensionName, settings.extensions[i].c_str()) == 0)
                break;
        }
        if(j == count)
        {
            LOGE("Failed to find needed extension %s !", settings.extensions[i].c_str());
            // Do we want to return false from initBase ?
            break;
        }
    }
    //
    // Create the device
    //
    queueInfo.queueFamilyIndex = queueFamilyIndex;
    queueInfo.queueCount =  m_gpu.queueProperties[queueFamilyIndex].queueCount;
    std::vector<float> pris;
    pris.resize(queueInfo.queueCount);
    for(int i=0; i<queueInfo.queueCount; i++)
        pris[i] = 1.0f;
    queueInfo.pQueuePriorities = &pris[0];

    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &queueInfo;
    devInfo.enabledLayerCount = instance_validation_layers_sz;
    devInfo.ppEnabledLayerNames = instance_validation_layers_sz > 0 ? &instance_validation_layers[0] : NULL;
    devInfo.enabledExtensionCount = (uint32_t)extension_names.size();
    devInfo.ppEnabledExtensionNames = &(extension_names[0]);
    result = ::vkCreateDevice(m_gpu.device, &devInfo, NULL, &m_device);
    if (result != VK_SUCCESS) {
        return false;
    }
    vkGetDeviceQueue(m_device, 0, 0, &m_queue);

    LOGOK("initialized Vulkan basis\n");

    init_swapchain_extension();
    m_swapChain.init(m_device, m_gpu.device, m_surface);
    m_swapChain.update(m_win->getWidth(), m_win->getHeight() );
    LOGOK("initialized WSI swapchain\n");

    return true;
}


int  WINinternalVK::sysExtensionSupported( const char* name )
{
    assert(!"TODO");
    return 0;
}

void WINinternalVK::swapBuffers()
{
    m_swapChain.present(m_queue);
}

void WINinternalVK::display()
{
    m_swapChain.acquire();
}

void WINinternalVK::terminate()
{
}

void WINinternalVK::reshape(int w, int h)
{
}
