/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2018-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include "extensions_vk.hpp"
#include "nvh/nvprint.hpp"

/* NVVK_GENERATE_VERSION_INFO */
// Generated using Vulkan 261
/* NVVK_GENERATE_VERSION_INFO */

/* clang-format off */

/* NVVK_GENERATE_STATIC_PFN */
#if defined(VK_AMD_buffer_marker)
static PFN_vkCmdWriteBufferMarkerAMD pfn_vkCmdWriteBufferMarkerAMD= 0;
#endif /* VK_AMD_buffer_marker */
#if defined(VK_AMD_display_native_hdr)
static PFN_vkSetLocalDimmingAMD pfn_vkSetLocalDimmingAMD= 0;
#endif /* VK_AMD_display_native_hdr */
#if defined(VK_AMD_draw_indirect_count)
static PFN_vkCmdDrawIndexedIndirectCountAMD pfn_vkCmdDrawIndexedIndirectCountAMD= 0;
static PFN_vkCmdDrawIndirectCountAMD pfn_vkCmdDrawIndirectCountAMD= 0;
#endif /* VK_AMD_draw_indirect_count */
#if defined(VK_AMD_shader_info)
static PFN_vkGetShaderInfoAMD pfn_vkGetShaderInfoAMD= 0;
#endif /* VK_AMD_shader_info */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
static PFN_vkGetAndroidHardwareBufferPropertiesANDROID pfn_vkGetAndroidHardwareBufferPropertiesANDROID= 0;
static PFN_vkGetMemoryAndroidHardwareBufferANDROID pfn_vkGetMemoryAndroidHardwareBufferANDROID= 0;
#endif /* VK_ANDROID_external_memory_android_hardware_buffer */
#if defined(VK_EXT_acquire_drm_display)
static PFN_vkAcquireDrmDisplayEXT pfn_vkAcquireDrmDisplayEXT= 0;
static PFN_vkGetDrmDisplayEXT pfn_vkGetDrmDisplayEXT= 0;
#endif /* VK_EXT_acquire_drm_display */
#if defined(VK_EXT_acquire_xlib_display)
static PFN_vkAcquireXlibDisplayEXT pfn_vkAcquireXlibDisplayEXT= 0;
static PFN_vkGetRandROutputDisplayEXT pfn_vkGetRandROutputDisplayEXT= 0;
#endif /* VK_EXT_acquire_xlib_display */
#if defined(VK_EXT_attachment_feedback_loop_dynamic_state)
static PFN_vkCmdSetAttachmentFeedbackLoopEnableEXT pfn_vkCmdSetAttachmentFeedbackLoopEnableEXT= 0;
#endif /* VK_EXT_attachment_feedback_loop_dynamic_state */
#if defined(VK_EXT_buffer_device_address)
static PFN_vkGetBufferDeviceAddressEXT pfn_vkGetBufferDeviceAddressEXT= 0;
#endif /* VK_EXT_buffer_device_address */
#if defined(VK_EXT_calibrated_timestamps)
static PFN_vkGetCalibratedTimestampsEXT pfn_vkGetCalibratedTimestampsEXT= 0;
static PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT pfn_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT= 0;
#endif /* VK_EXT_calibrated_timestamps */
#if defined(VK_EXT_color_write_enable)
static PFN_vkCmdSetColorWriteEnableEXT pfn_vkCmdSetColorWriteEnableEXT= 0;
#endif /* VK_EXT_color_write_enable */
#if defined(VK_EXT_conditional_rendering)
static PFN_vkCmdBeginConditionalRenderingEXT pfn_vkCmdBeginConditionalRenderingEXT= 0;
static PFN_vkCmdEndConditionalRenderingEXT pfn_vkCmdEndConditionalRenderingEXT= 0;
#endif /* VK_EXT_conditional_rendering */
#if defined(VK_EXT_debug_marker)
static PFN_vkCmdDebugMarkerBeginEXT pfn_vkCmdDebugMarkerBeginEXT= 0;
static PFN_vkCmdDebugMarkerEndEXT pfn_vkCmdDebugMarkerEndEXT= 0;
static PFN_vkCmdDebugMarkerInsertEXT pfn_vkCmdDebugMarkerInsertEXT= 0;
static PFN_vkDebugMarkerSetObjectNameEXT pfn_vkDebugMarkerSetObjectNameEXT= 0;
static PFN_vkDebugMarkerSetObjectTagEXT pfn_vkDebugMarkerSetObjectTagEXT= 0;
#endif /* VK_EXT_debug_marker */
#if defined(VK_EXT_debug_report)
static PFN_vkCreateDebugReportCallbackEXT pfn_vkCreateDebugReportCallbackEXT= 0;
static PFN_vkDebugReportMessageEXT pfn_vkDebugReportMessageEXT= 0;
static PFN_vkDestroyDebugReportCallbackEXT pfn_vkDestroyDebugReportCallbackEXT= 0;
#endif /* VK_EXT_debug_report */
#if defined(VK_EXT_debug_utils)
static PFN_vkCmdBeginDebugUtilsLabelEXT pfn_vkCmdBeginDebugUtilsLabelEXT= 0;
static PFN_vkCmdEndDebugUtilsLabelEXT pfn_vkCmdEndDebugUtilsLabelEXT= 0;
static PFN_vkCmdInsertDebugUtilsLabelEXT pfn_vkCmdInsertDebugUtilsLabelEXT= 0;
static PFN_vkCreateDebugUtilsMessengerEXT pfn_vkCreateDebugUtilsMessengerEXT= 0;
static PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT= 0;
static PFN_vkQueueBeginDebugUtilsLabelEXT pfn_vkQueueBeginDebugUtilsLabelEXT= 0;
static PFN_vkQueueEndDebugUtilsLabelEXT pfn_vkQueueEndDebugUtilsLabelEXT= 0;
static PFN_vkQueueInsertDebugUtilsLabelEXT pfn_vkQueueInsertDebugUtilsLabelEXT= 0;
static PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT= 0;
static PFN_vkSetDebugUtilsObjectTagEXT pfn_vkSetDebugUtilsObjectTagEXT= 0;
static PFN_vkSubmitDebugUtilsMessageEXT pfn_vkSubmitDebugUtilsMessageEXT= 0;
#endif /* VK_EXT_debug_utils */
#if defined(VK_EXT_depth_bias_control)
static PFN_vkCmdSetDepthBias2EXT pfn_vkCmdSetDepthBias2EXT= 0;
#endif /* VK_EXT_depth_bias_control */
#if defined(VK_EXT_descriptor_buffer)
static PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT pfn_vkCmdBindDescriptorBufferEmbeddedSamplersEXT= 0;
static PFN_vkCmdBindDescriptorBuffersEXT pfn_vkCmdBindDescriptorBuffersEXT= 0;
static PFN_vkCmdSetDescriptorBufferOffsetsEXT pfn_vkCmdSetDescriptorBufferOffsetsEXT= 0;
static PFN_vkGetBufferOpaqueCaptureDescriptorDataEXT pfn_vkGetBufferOpaqueCaptureDescriptorDataEXT= 0;
static PFN_vkGetDescriptorEXT pfn_vkGetDescriptorEXT= 0;
static PFN_vkGetDescriptorSetLayoutBindingOffsetEXT pfn_vkGetDescriptorSetLayoutBindingOffsetEXT= 0;
static PFN_vkGetDescriptorSetLayoutSizeEXT pfn_vkGetDescriptorSetLayoutSizeEXT= 0;
static PFN_vkGetImageOpaqueCaptureDescriptorDataEXT pfn_vkGetImageOpaqueCaptureDescriptorDataEXT= 0;
static PFN_vkGetImageViewOpaqueCaptureDescriptorDataEXT pfn_vkGetImageViewOpaqueCaptureDescriptorDataEXT= 0;
static PFN_vkGetSamplerOpaqueCaptureDescriptorDataEXT pfn_vkGetSamplerOpaqueCaptureDescriptorDataEXT= 0;
#endif /* VK_EXT_descriptor_buffer */
#if defined(VK_EXT_descriptor_buffer) && (defined(VK_KHR_acceleration_structure) || defined(VK_NV_ray_tracing))
static PFN_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT pfn_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT= 0;
#endif /* VK_EXT_descriptor_buffer && (VK_KHR_acceleration_structure || VK_NV_ray_tracing) */
#if defined(VK_EXT_device_fault)
static PFN_vkGetDeviceFaultInfoEXT pfn_vkGetDeviceFaultInfoEXT= 0;
#endif /* VK_EXT_device_fault */
#if defined(VK_EXT_direct_mode_display)
static PFN_vkReleaseDisplayEXT pfn_vkReleaseDisplayEXT= 0;
#endif /* VK_EXT_direct_mode_display */
#if defined(VK_EXT_directfb_surface)
static PFN_vkCreateDirectFBSurfaceEXT pfn_vkCreateDirectFBSurfaceEXT= 0;
static PFN_vkGetPhysicalDeviceDirectFBPresentationSupportEXT pfn_vkGetPhysicalDeviceDirectFBPresentationSupportEXT= 0;
#endif /* VK_EXT_directfb_surface */
#if defined(VK_EXT_discard_rectangles)
static PFN_vkCmdSetDiscardRectangleEXT pfn_vkCmdSetDiscardRectangleEXT= 0;
#endif /* VK_EXT_discard_rectangles */
#if defined(VK_EXT_discard_rectangles) && VK_EXT_DISCARD_RECTANGLES_SPEC_VERSION >= 2
static PFN_vkCmdSetDiscardRectangleEnableEXT pfn_vkCmdSetDiscardRectangleEnableEXT= 0;
static PFN_vkCmdSetDiscardRectangleModeEXT pfn_vkCmdSetDiscardRectangleModeEXT= 0;
#endif /* VK_EXT_discard_rectangles && VK_EXT_DISCARD_RECTANGLES_SPEC_VERSION >= 2 */
#if defined(VK_EXT_display_control)
static PFN_vkDisplayPowerControlEXT pfn_vkDisplayPowerControlEXT= 0;
static PFN_vkGetSwapchainCounterEXT pfn_vkGetSwapchainCounterEXT= 0;
static PFN_vkRegisterDeviceEventEXT pfn_vkRegisterDeviceEventEXT= 0;
static PFN_vkRegisterDisplayEventEXT pfn_vkRegisterDisplayEventEXT= 0;
#endif /* VK_EXT_display_control */
#if defined(VK_EXT_display_surface_counter)
static PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT pfn_vkGetPhysicalDeviceSurfaceCapabilities2EXT= 0;
#endif /* VK_EXT_display_surface_counter */
#if defined(VK_EXT_external_memory_host)
static PFN_vkGetMemoryHostPointerPropertiesEXT pfn_vkGetMemoryHostPointerPropertiesEXT= 0;
#endif /* VK_EXT_external_memory_host */
#if defined(VK_EXT_full_screen_exclusive)
static PFN_vkAcquireFullScreenExclusiveModeEXT pfn_vkAcquireFullScreenExclusiveModeEXT= 0;
static PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT pfn_vkGetPhysicalDeviceSurfacePresentModes2EXT= 0;
static PFN_vkReleaseFullScreenExclusiveModeEXT pfn_vkReleaseFullScreenExclusiveModeEXT= 0;
#endif /* VK_EXT_full_screen_exclusive */
#if defined(VK_EXT_hdr_metadata)
static PFN_vkSetHdrMetadataEXT pfn_vkSetHdrMetadataEXT= 0;
#endif /* VK_EXT_hdr_metadata */
#if defined(VK_EXT_headless_surface)
static PFN_vkCreateHeadlessSurfaceEXT pfn_vkCreateHeadlessSurfaceEXT= 0;
#endif /* VK_EXT_headless_surface */
#if defined(VK_EXT_host_image_copy)
static PFN_vkCopyImageToImageEXT pfn_vkCopyImageToImageEXT= 0;
static PFN_vkCopyImageToMemoryEXT pfn_vkCopyImageToMemoryEXT= 0;
static PFN_vkCopyMemoryToImageEXT pfn_vkCopyMemoryToImageEXT= 0;
static PFN_vkTransitionImageLayoutEXT pfn_vkTransitionImageLayoutEXT= 0;
#endif /* VK_EXT_host_image_copy */
#if defined(VK_EXT_host_query_reset)
static PFN_vkResetQueryPoolEXT pfn_vkResetQueryPoolEXT= 0;
#endif /* VK_EXT_host_query_reset */
#if defined(VK_EXT_image_drm_format_modifier)
static PFN_vkGetImageDrmFormatModifierPropertiesEXT pfn_vkGetImageDrmFormatModifierPropertiesEXT= 0;
#endif /* VK_EXT_image_drm_format_modifier */
#if defined(VK_EXT_line_rasterization)
static PFN_vkCmdSetLineStippleEXT pfn_vkCmdSetLineStippleEXT= 0;
#endif /* VK_EXT_line_rasterization */
#if defined(VK_EXT_mesh_shader)
static PFN_vkCmdDrawMeshTasksEXT pfn_vkCmdDrawMeshTasksEXT= 0;
static PFN_vkCmdDrawMeshTasksIndirectCountEXT pfn_vkCmdDrawMeshTasksIndirectCountEXT= 0;
static PFN_vkCmdDrawMeshTasksIndirectEXT pfn_vkCmdDrawMeshTasksIndirectEXT= 0;
#endif /* VK_EXT_mesh_shader */
#if defined(VK_EXT_metal_objects)
static PFN_vkExportMetalObjectsEXT pfn_vkExportMetalObjectsEXT= 0;
#endif /* VK_EXT_metal_objects */
#if defined(VK_EXT_metal_surface)
static PFN_vkCreateMetalSurfaceEXT pfn_vkCreateMetalSurfaceEXT= 0;
#endif /* VK_EXT_metal_surface */
#if defined(VK_EXT_multi_draw)
static PFN_vkCmdDrawMultiEXT pfn_vkCmdDrawMultiEXT= 0;
static PFN_vkCmdDrawMultiIndexedEXT pfn_vkCmdDrawMultiIndexedEXT= 0;
#endif /* VK_EXT_multi_draw */
#if defined(VK_EXT_opacity_micromap)
static PFN_vkBuildMicromapsEXT pfn_vkBuildMicromapsEXT= 0;
static PFN_vkCmdBuildMicromapsEXT pfn_vkCmdBuildMicromapsEXT= 0;
static PFN_vkCmdCopyMemoryToMicromapEXT pfn_vkCmdCopyMemoryToMicromapEXT= 0;
static PFN_vkCmdCopyMicromapEXT pfn_vkCmdCopyMicromapEXT= 0;
static PFN_vkCmdCopyMicromapToMemoryEXT pfn_vkCmdCopyMicromapToMemoryEXT= 0;
static PFN_vkCmdWriteMicromapsPropertiesEXT pfn_vkCmdWriteMicromapsPropertiesEXT= 0;
static PFN_vkCopyMemoryToMicromapEXT pfn_vkCopyMemoryToMicromapEXT= 0;
static PFN_vkCopyMicromapEXT pfn_vkCopyMicromapEXT= 0;
static PFN_vkCopyMicromapToMemoryEXT pfn_vkCopyMicromapToMemoryEXT= 0;
static PFN_vkCreateMicromapEXT pfn_vkCreateMicromapEXT= 0;
static PFN_vkDestroyMicromapEXT pfn_vkDestroyMicromapEXT= 0;
static PFN_vkGetDeviceMicromapCompatibilityEXT pfn_vkGetDeviceMicromapCompatibilityEXT= 0;
static PFN_vkGetMicromapBuildSizesEXT pfn_vkGetMicromapBuildSizesEXT= 0;
static PFN_vkWriteMicromapsPropertiesEXT pfn_vkWriteMicromapsPropertiesEXT= 0;
#endif /* VK_EXT_opacity_micromap */
#if defined(VK_EXT_pageable_device_local_memory)
static PFN_vkSetDeviceMemoryPriorityEXT pfn_vkSetDeviceMemoryPriorityEXT= 0;
#endif /* VK_EXT_pageable_device_local_memory */
#if defined(VK_EXT_pipeline_properties)
static PFN_vkGetPipelinePropertiesEXT pfn_vkGetPipelinePropertiesEXT= 0;
#endif /* VK_EXT_pipeline_properties */
#if defined(VK_EXT_private_data)
static PFN_vkCreatePrivateDataSlotEXT pfn_vkCreatePrivateDataSlotEXT= 0;
static PFN_vkDestroyPrivateDataSlotEXT pfn_vkDestroyPrivateDataSlotEXT= 0;
static PFN_vkGetPrivateDataEXT pfn_vkGetPrivateDataEXT= 0;
static PFN_vkSetPrivateDataEXT pfn_vkSetPrivateDataEXT= 0;
#endif /* VK_EXT_private_data */
#if defined(VK_EXT_sample_locations)
static PFN_vkCmdSetSampleLocationsEXT pfn_vkCmdSetSampleLocationsEXT= 0;
static PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT pfn_vkGetPhysicalDeviceMultisamplePropertiesEXT= 0;
#endif /* VK_EXT_sample_locations */
#if defined(VK_EXT_shader_module_identifier)
static PFN_vkGetShaderModuleCreateInfoIdentifierEXT pfn_vkGetShaderModuleCreateInfoIdentifierEXT= 0;
static PFN_vkGetShaderModuleIdentifierEXT pfn_vkGetShaderModuleIdentifierEXT= 0;
#endif /* VK_EXT_shader_module_identifier */
#if defined(VK_EXT_shader_object)
static PFN_vkCmdBindShadersEXT pfn_vkCmdBindShadersEXT= 0;
static PFN_vkCreateShadersEXT pfn_vkCreateShadersEXT= 0;
static PFN_vkDestroyShaderEXT pfn_vkDestroyShaderEXT= 0;
static PFN_vkGetShaderBinaryDataEXT pfn_vkGetShaderBinaryDataEXT= 0;
#endif /* VK_EXT_shader_object */
#if defined(VK_EXT_swapchain_maintenance1)
static PFN_vkReleaseSwapchainImagesEXT pfn_vkReleaseSwapchainImagesEXT= 0;
#endif /* VK_EXT_swapchain_maintenance1 */
#if defined(VK_EXT_tooling_info)
static PFN_vkGetPhysicalDeviceToolPropertiesEXT pfn_vkGetPhysicalDeviceToolPropertiesEXT= 0;
#endif /* VK_EXT_tooling_info */
#if defined(VK_EXT_transform_feedback)
static PFN_vkCmdBeginQueryIndexedEXT pfn_vkCmdBeginQueryIndexedEXT= 0;
static PFN_vkCmdBeginTransformFeedbackEXT pfn_vkCmdBeginTransformFeedbackEXT= 0;
static PFN_vkCmdBindTransformFeedbackBuffersEXT pfn_vkCmdBindTransformFeedbackBuffersEXT= 0;
static PFN_vkCmdDrawIndirectByteCountEXT pfn_vkCmdDrawIndirectByteCountEXT= 0;
static PFN_vkCmdEndQueryIndexedEXT pfn_vkCmdEndQueryIndexedEXT= 0;
static PFN_vkCmdEndTransformFeedbackEXT pfn_vkCmdEndTransformFeedbackEXT= 0;
#endif /* VK_EXT_transform_feedback */
#if defined(VK_EXT_validation_cache)
static PFN_vkCreateValidationCacheEXT pfn_vkCreateValidationCacheEXT= 0;
static PFN_vkDestroyValidationCacheEXT pfn_vkDestroyValidationCacheEXT= 0;
static PFN_vkGetValidationCacheDataEXT pfn_vkGetValidationCacheDataEXT= 0;
static PFN_vkMergeValidationCachesEXT pfn_vkMergeValidationCachesEXT= 0;
#endif /* VK_EXT_validation_cache */
#if defined(VK_FUCHSIA_buffer_collection)
static PFN_vkCreateBufferCollectionFUCHSIA pfn_vkCreateBufferCollectionFUCHSIA= 0;
static PFN_vkDestroyBufferCollectionFUCHSIA pfn_vkDestroyBufferCollectionFUCHSIA= 0;
static PFN_vkGetBufferCollectionPropertiesFUCHSIA pfn_vkGetBufferCollectionPropertiesFUCHSIA= 0;
static PFN_vkSetBufferCollectionBufferConstraintsFUCHSIA pfn_vkSetBufferCollectionBufferConstraintsFUCHSIA= 0;
static PFN_vkSetBufferCollectionImageConstraintsFUCHSIA pfn_vkSetBufferCollectionImageConstraintsFUCHSIA= 0;
#endif /* VK_FUCHSIA_buffer_collection */
#if defined(VK_FUCHSIA_external_memory)
static PFN_vkGetMemoryZirconHandleFUCHSIA pfn_vkGetMemoryZirconHandleFUCHSIA= 0;
static PFN_vkGetMemoryZirconHandlePropertiesFUCHSIA pfn_vkGetMemoryZirconHandlePropertiesFUCHSIA= 0;
#endif /* VK_FUCHSIA_external_memory */
#if defined(VK_FUCHSIA_external_semaphore)
static PFN_vkGetSemaphoreZirconHandleFUCHSIA pfn_vkGetSemaphoreZirconHandleFUCHSIA= 0;
static PFN_vkImportSemaphoreZirconHandleFUCHSIA pfn_vkImportSemaphoreZirconHandleFUCHSIA= 0;
#endif /* VK_FUCHSIA_external_semaphore */
#if defined(VK_FUCHSIA_imagepipe_surface)
static PFN_vkCreateImagePipeSurfaceFUCHSIA pfn_vkCreateImagePipeSurfaceFUCHSIA= 0;
#endif /* VK_FUCHSIA_imagepipe_surface */
#if defined(VK_GGP_stream_descriptor_surface)
static PFN_vkCreateStreamDescriptorSurfaceGGP pfn_vkCreateStreamDescriptorSurfaceGGP= 0;
#endif /* VK_GGP_stream_descriptor_surface */
#if defined(VK_GOOGLE_display_timing)
static PFN_vkGetPastPresentationTimingGOOGLE pfn_vkGetPastPresentationTimingGOOGLE= 0;
static PFN_vkGetRefreshCycleDurationGOOGLE pfn_vkGetRefreshCycleDurationGOOGLE= 0;
#endif /* VK_GOOGLE_display_timing */
#if defined(VK_HUAWEI_cluster_culling_shader)
static PFN_vkCmdDrawClusterHUAWEI pfn_vkCmdDrawClusterHUAWEI= 0;
static PFN_vkCmdDrawClusterIndirectHUAWEI pfn_vkCmdDrawClusterIndirectHUAWEI= 0;
#endif /* VK_HUAWEI_cluster_culling_shader */
#if defined(VK_HUAWEI_invocation_mask)
static PFN_vkCmdBindInvocationMaskHUAWEI pfn_vkCmdBindInvocationMaskHUAWEI= 0;
#endif /* VK_HUAWEI_invocation_mask */
#if defined(VK_HUAWEI_subpass_shading)
static PFN_vkCmdSubpassShadingHUAWEI pfn_vkCmdSubpassShadingHUAWEI= 0;
static PFN_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI pfn_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI= 0;
#endif /* VK_HUAWEI_subpass_shading */
#if defined(VK_INTEL_performance_query)
static PFN_vkAcquirePerformanceConfigurationINTEL pfn_vkAcquirePerformanceConfigurationINTEL= 0;
static PFN_vkCmdSetPerformanceMarkerINTEL pfn_vkCmdSetPerformanceMarkerINTEL= 0;
static PFN_vkCmdSetPerformanceOverrideINTEL pfn_vkCmdSetPerformanceOverrideINTEL= 0;
static PFN_vkCmdSetPerformanceStreamMarkerINTEL pfn_vkCmdSetPerformanceStreamMarkerINTEL= 0;
static PFN_vkGetPerformanceParameterINTEL pfn_vkGetPerformanceParameterINTEL= 0;
static PFN_vkInitializePerformanceApiINTEL pfn_vkInitializePerformanceApiINTEL= 0;
static PFN_vkQueueSetPerformanceConfigurationINTEL pfn_vkQueueSetPerformanceConfigurationINTEL= 0;
static PFN_vkReleasePerformanceConfigurationINTEL pfn_vkReleasePerformanceConfigurationINTEL= 0;
static PFN_vkUninitializePerformanceApiINTEL pfn_vkUninitializePerformanceApiINTEL= 0;
#endif /* VK_INTEL_performance_query */
#if defined(VK_KHR_acceleration_structure)
static PFN_vkBuildAccelerationStructuresKHR pfn_vkBuildAccelerationStructuresKHR= 0;
static PFN_vkCmdBuildAccelerationStructuresIndirectKHR pfn_vkCmdBuildAccelerationStructuresIndirectKHR= 0;
static PFN_vkCmdBuildAccelerationStructuresKHR pfn_vkCmdBuildAccelerationStructuresKHR= 0;
static PFN_vkCmdCopyAccelerationStructureKHR pfn_vkCmdCopyAccelerationStructureKHR= 0;
static PFN_vkCmdCopyAccelerationStructureToMemoryKHR pfn_vkCmdCopyAccelerationStructureToMemoryKHR= 0;
static PFN_vkCmdCopyMemoryToAccelerationStructureKHR pfn_vkCmdCopyMemoryToAccelerationStructureKHR= 0;
static PFN_vkCmdWriteAccelerationStructuresPropertiesKHR pfn_vkCmdWriteAccelerationStructuresPropertiesKHR= 0;
static PFN_vkCopyAccelerationStructureKHR pfn_vkCopyAccelerationStructureKHR= 0;
static PFN_vkCopyAccelerationStructureToMemoryKHR pfn_vkCopyAccelerationStructureToMemoryKHR= 0;
static PFN_vkCopyMemoryToAccelerationStructureKHR pfn_vkCopyMemoryToAccelerationStructureKHR= 0;
static PFN_vkCreateAccelerationStructureKHR pfn_vkCreateAccelerationStructureKHR= 0;
static PFN_vkDestroyAccelerationStructureKHR pfn_vkDestroyAccelerationStructureKHR= 0;
static PFN_vkGetAccelerationStructureBuildSizesKHR pfn_vkGetAccelerationStructureBuildSizesKHR= 0;
static PFN_vkGetAccelerationStructureDeviceAddressKHR pfn_vkGetAccelerationStructureDeviceAddressKHR= 0;
static PFN_vkGetDeviceAccelerationStructureCompatibilityKHR pfn_vkGetDeviceAccelerationStructureCompatibilityKHR= 0;
static PFN_vkWriteAccelerationStructuresPropertiesKHR pfn_vkWriteAccelerationStructuresPropertiesKHR= 0;
#endif /* VK_KHR_acceleration_structure */
#if defined(VK_KHR_android_surface)
static PFN_vkCreateAndroidSurfaceKHR pfn_vkCreateAndroidSurfaceKHR= 0;
#endif /* VK_KHR_android_surface */
#if defined(VK_KHR_bind_memory2)
static PFN_vkBindBufferMemory2KHR pfn_vkBindBufferMemory2KHR= 0;
static PFN_vkBindImageMemory2KHR pfn_vkBindImageMemory2KHR= 0;
#endif /* VK_KHR_bind_memory2 */
#if defined(VK_KHR_buffer_device_address)
static PFN_vkGetBufferDeviceAddressKHR pfn_vkGetBufferDeviceAddressKHR= 0;
static PFN_vkGetBufferOpaqueCaptureAddressKHR pfn_vkGetBufferOpaqueCaptureAddressKHR= 0;
static PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR pfn_vkGetDeviceMemoryOpaqueCaptureAddressKHR= 0;
#endif /* VK_KHR_buffer_device_address */
#if defined(VK_KHR_cooperative_matrix)
static PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR pfn_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR= 0;
#endif /* VK_KHR_cooperative_matrix */
#if defined(VK_KHR_copy_commands2)
static PFN_vkCmdBlitImage2KHR pfn_vkCmdBlitImage2KHR= 0;
static PFN_vkCmdCopyBuffer2KHR pfn_vkCmdCopyBuffer2KHR= 0;
static PFN_vkCmdCopyBufferToImage2KHR pfn_vkCmdCopyBufferToImage2KHR= 0;
static PFN_vkCmdCopyImage2KHR pfn_vkCmdCopyImage2KHR= 0;
static PFN_vkCmdCopyImageToBuffer2KHR pfn_vkCmdCopyImageToBuffer2KHR= 0;
static PFN_vkCmdResolveImage2KHR pfn_vkCmdResolveImage2KHR= 0;
#endif /* VK_KHR_copy_commands2 */
#if defined(VK_KHR_create_renderpass2)
static PFN_vkCmdBeginRenderPass2KHR pfn_vkCmdBeginRenderPass2KHR= 0;
static PFN_vkCmdEndRenderPass2KHR pfn_vkCmdEndRenderPass2KHR= 0;
static PFN_vkCmdNextSubpass2KHR pfn_vkCmdNextSubpass2KHR= 0;
static PFN_vkCreateRenderPass2KHR pfn_vkCreateRenderPass2KHR= 0;
#endif /* VK_KHR_create_renderpass2 */
#if defined(VK_KHR_deferred_host_operations)
static PFN_vkCreateDeferredOperationKHR pfn_vkCreateDeferredOperationKHR= 0;
static PFN_vkDeferredOperationJoinKHR pfn_vkDeferredOperationJoinKHR= 0;
static PFN_vkDestroyDeferredOperationKHR pfn_vkDestroyDeferredOperationKHR= 0;
static PFN_vkGetDeferredOperationMaxConcurrencyKHR pfn_vkGetDeferredOperationMaxConcurrencyKHR= 0;
static PFN_vkGetDeferredOperationResultKHR pfn_vkGetDeferredOperationResultKHR= 0;
#endif /* VK_KHR_deferred_host_operations */
#if defined(VK_KHR_descriptor_update_template)
static PFN_vkCreateDescriptorUpdateTemplateKHR pfn_vkCreateDescriptorUpdateTemplateKHR= 0;
static PFN_vkDestroyDescriptorUpdateTemplateKHR pfn_vkDestroyDescriptorUpdateTemplateKHR= 0;
static PFN_vkUpdateDescriptorSetWithTemplateKHR pfn_vkUpdateDescriptorSetWithTemplateKHR= 0;
#endif /* VK_KHR_descriptor_update_template */
#if defined(VK_KHR_device_group)
static PFN_vkCmdDispatchBaseKHR pfn_vkCmdDispatchBaseKHR= 0;
static PFN_vkCmdSetDeviceMaskKHR pfn_vkCmdSetDeviceMaskKHR= 0;
static PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR pfn_vkGetDeviceGroupPeerMemoryFeaturesKHR= 0;
#endif /* VK_KHR_device_group */
#if defined(VK_KHR_device_group_creation)
static PFN_vkEnumeratePhysicalDeviceGroupsKHR pfn_vkEnumeratePhysicalDeviceGroupsKHR= 0;
#endif /* VK_KHR_device_group_creation */
#if defined(VK_KHR_draw_indirect_count)
static PFN_vkCmdDrawIndexedIndirectCountKHR pfn_vkCmdDrawIndexedIndirectCountKHR= 0;
static PFN_vkCmdDrawIndirectCountKHR pfn_vkCmdDrawIndirectCountKHR= 0;
#endif /* VK_KHR_draw_indirect_count */
#if defined(VK_KHR_dynamic_rendering)
static PFN_vkCmdBeginRenderingKHR pfn_vkCmdBeginRenderingKHR= 0;
static PFN_vkCmdEndRenderingKHR pfn_vkCmdEndRenderingKHR= 0;
#endif /* VK_KHR_dynamic_rendering */
#if defined(VK_KHR_external_fence_capabilities)
static PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR pfn_vkGetPhysicalDeviceExternalFencePropertiesKHR= 0;
#endif /* VK_KHR_external_fence_capabilities */
#if defined(VK_KHR_external_fence_fd)
static PFN_vkGetFenceFdKHR pfn_vkGetFenceFdKHR= 0;
static PFN_vkImportFenceFdKHR pfn_vkImportFenceFdKHR= 0;
#endif /* VK_KHR_external_fence_fd */
#if defined(VK_KHR_external_fence_win32)
static PFN_vkGetFenceWin32HandleKHR pfn_vkGetFenceWin32HandleKHR= 0;
static PFN_vkImportFenceWin32HandleKHR pfn_vkImportFenceWin32HandleKHR= 0;
#endif /* VK_KHR_external_fence_win32 */
#if defined(VK_KHR_external_memory_capabilities)
static PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR pfn_vkGetPhysicalDeviceExternalBufferPropertiesKHR= 0;
#endif /* VK_KHR_external_memory_capabilities */
#if defined(VK_KHR_external_memory_fd)
static PFN_vkGetMemoryFdKHR pfn_vkGetMemoryFdKHR= 0;
static PFN_vkGetMemoryFdPropertiesKHR pfn_vkGetMemoryFdPropertiesKHR= 0;
#endif /* VK_KHR_external_memory_fd */
#if defined(VK_KHR_external_memory_win32)
static PFN_vkGetMemoryWin32HandleKHR pfn_vkGetMemoryWin32HandleKHR= 0;
static PFN_vkGetMemoryWin32HandlePropertiesKHR pfn_vkGetMemoryWin32HandlePropertiesKHR= 0;
#endif /* VK_KHR_external_memory_win32 */
#if defined(VK_KHR_external_semaphore_capabilities)
static PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR pfn_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR= 0;
#endif /* VK_KHR_external_semaphore_capabilities */
#if defined(VK_KHR_external_semaphore_fd)
static PFN_vkGetSemaphoreFdKHR pfn_vkGetSemaphoreFdKHR= 0;
static PFN_vkImportSemaphoreFdKHR pfn_vkImportSemaphoreFdKHR= 0;
#endif /* VK_KHR_external_semaphore_fd */
#if defined(VK_KHR_external_semaphore_win32)
static PFN_vkGetSemaphoreWin32HandleKHR pfn_vkGetSemaphoreWin32HandleKHR= 0;
static PFN_vkImportSemaphoreWin32HandleKHR pfn_vkImportSemaphoreWin32HandleKHR= 0;
#endif /* VK_KHR_external_semaphore_win32 */
#if defined(VK_KHR_fragment_shading_rate)
static PFN_vkCmdSetFragmentShadingRateKHR pfn_vkCmdSetFragmentShadingRateKHR= 0;
static PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR pfn_vkGetPhysicalDeviceFragmentShadingRatesKHR= 0;
#endif /* VK_KHR_fragment_shading_rate */
#if defined(VK_KHR_get_memory_requirements2)
static PFN_vkGetBufferMemoryRequirements2KHR pfn_vkGetBufferMemoryRequirements2KHR= 0;
static PFN_vkGetImageMemoryRequirements2KHR pfn_vkGetImageMemoryRequirements2KHR= 0;
static PFN_vkGetImageSparseMemoryRequirements2KHR pfn_vkGetImageSparseMemoryRequirements2KHR= 0;
#endif /* VK_KHR_get_memory_requirements2 */
#if defined(VK_KHR_get_physical_device_properties2)
static PFN_vkGetPhysicalDeviceFeatures2KHR pfn_vkGetPhysicalDeviceFeatures2KHR= 0;
static PFN_vkGetPhysicalDeviceFormatProperties2KHR pfn_vkGetPhysicalDeviceFormatProperties2KHR= 0;
static PFN_vkGetPhysicalDeviceImageFormatProperties2KHR pfn_vkGetPhysicalDeviceImageFormatProperties2KHR= 0;
static PFN_vkGetPhysicalDeviceMemoryProperties2KHR pfn_vkGetPhysicalDeviceMemoryProperties2KHR= 0;
static PFN_vkGetPhysicalDeviceProperties2KHR pfn_vkGetPhysicalDeviceProperties2KHR= 0;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR pfn_vkGetPhysicalDeviceQueueFamilyProperties2KHR= 0;
static PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR pfn_vkGetPhysicalDeviceSparseImageFormatProperties2KHR= 0;
#endif /* VK_KHR_get_physical_device_properties2 */
#if defined(VK_KHR_maintenance1)
static PFN_vkTrimCommandPoolKHR pfn_vkTrimCommandPoolKHR= 0;
#endif /* VK_KHR_maintenance1 */
#if defined(VK_KHR_maintenance3)
static PFN_vkGetDescriptorSetLayoutSupportKHR pfn_vkGetDescriptorSetLayoutSupportKHR= 0;
#endif /* VK_KHR_maintenance3 */
#if defined(VK_KHR_maintenance4)
static PFN_vkGetDeviceBufferMemoryRequirementsKHR pfn_vkGetDeviceBufferMemoryRequirementsKHR= 0;
static PFN_vkGetDeviceImageMemoryRequirementsKHR pfn_vkGetDeviceImageMemoryRequirementsKHR= 0;
static PFN_vkGetDeviceImageSparseMemoryRequirementsKHR pfn_vkGetDeviceImageSparseMemoryRequirementsKHR= 0;
#endif /* VK_KHR_maintenance4 */
#if defined(VK_KHR_maintenance5)
static PFN_vkCmdBindIndexBuffer2KHR pfn_vkCmdBindIndexBuffer2KHR= 0;
static PFN_vkGetDeviceImageSubresourceLayoutKHR pfn_vkGetDeviceImageSubresourceLayoutKHR= 0;
static PFN_vkGetImageSubresourceLayout2KHR pfn_vkGetImageSubresourceLayout2KHR= 0;
static PFN_vkGetRenderingAreaGranularityKHR pfn_vkGetRenderingAreaGranularityKHR= 0;
#endif /* VK_KHR_maintenance5 */
#if defined(VK_KHR_map_memory2)
static PFN_vkMapMemory2KHR pfn_vkMapMemory2KHR= 0;
static PFN_vkUnmapMemory2KHR pfn_vkUnmapMemory2KHR= 0;
#endif /* VK_KHR_map_memory2 */
#if defined(VK_KHR_performance_query)
static PFN_vkAcquireProfilingLockKHR pfn_vkAcquireProfilingLockKHR= 0;
static PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR pfn_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR= 0;
static PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR pfn_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR= 0;
static PFN_vkReleaseProfilingLockKHR pfn_vkReleaseProfilingLockKHR= 0;
#endif /* VK_KHR_performance_query */
#if defined(VK_KHR_pipeline_executable_properties)
static PFN_vkGetPipelineExecutableInternalRepresentationsKHR pfn_vkGetPipelineExecutableInternalRepresentationsKHR= 0;
static PFN_vkGetPipelineExecutablePropertiesKHR pfn_vkGetPipelineExecutablePropertiesKHR= 0;
static PFN_vkGetPipelineExecutableStatisticsKHR pfn_vkGetPipelineExecutableStatisticsKHR= 0;
#endif /* VK_KHR_pipeline_executable_properties */
#if defined(VK_KHR_present_wait)
static PFN_vkWaitForPresentKHR pfn_vkWaitForPresentKHR= 0;
#endif /* VK_KHR_present_wait */
#if defined(VK_KHR_push_descriptor)
static PFN_vkCmdPushDescriptorSetKHR pfn_vkCmdPushDescriptorSetKHR= 0;
#endif /* VK_KHR_push_descriptor */
#if defined(VK_KHR_ray_tracing_maintenance1) && defined(VK_KHR_ray_tracing_pipeline)
static PFN_vkCmdTraceRaysIndirect2KHR pfn_vkCmdTraceRaysIndirect2KHR= 0;
#endif /* VK_KHR_ray_tracing_maintenance1 && VK_KHR_ray_tracing_pipeline */
#if defined(VK_KHR_ray_tracing_pipeline)
static PFN_vkCmdSetRayTracingPipelineStackSizeKHR pfn_vkCmdSetRayTracingPipelineStackSizeKHR= 0;
static PFN_vkCmdTraceRaysIndirectKHR pfn_vkCmdTraceRaysIndirectKHR= 0;
static PFN_vkCmdTraceRaysKHR pfn_vkCmdTraceRaysKHR= 0;
static PFN_vkCreateRayTracingPipelinesKHR pfn_vkCreateRayTracingPipelinesKHR= 0;
static PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR= 0;
static PFN_vkGetRayTracingShaderGroupHandlesKHR pfn_vkGetRayTracingShaderGroupHandlesKHR= 0;
static PFN_vkGetRayTracingShaderGroupStackSizeKHR pfn_vkGetRayTracingShaderGroupStackSizeKHR= 0;
#endif /* VK_KHR_ray_tracing_pipeline */
#if defined(VK_KHR_sampler_ycbcr_conversion)
static PFN_vkCreateSamplerYcbcrConversionKHR pfn_vkCreateSamplerYcbcrConversionKHR= 0;
static PFN_vkDestroySamplerYcbcrConversionKHR pfn_vkDestroySamplerYcbcrConversionKHR= 0;
#endif /* VK_KHR_sampler_ycbcr_conversion */
#if defined(VK_KHR_shared_presentable_image)
static PFN_vkGetSwapchainStatusKHR pfn_vkGetSwapchainStatusKHR= 0;
#endif /* VK_KHR_shared_presentable_image */
#if defined(VK_KHR_synchronization2)
static PFN_vkCmdPipelineBarrier2KHR pfn_vkCmdPipelineBarrier2KHR= 0;
static PFN_vkCmdResetEvent2KHR pfn_vkCmdResetEvent2KHR= 0;
static PFN_vkCmdSetEvent2KHR pfn_vkCmdSetEvent2KHR= 0;
static PFN_vkCmdWaitEvents2KHR pfn_vkCmdWaitEvents2KHR= 0;
static PFN_vkCmdWriteTimestamp2KHR pfn_vkCmdWriteTimestamp2KHR= 0;
static PFN_vkQueueSubmit2KHR pfn_vkQueueSubmit2KHR= 0;
#endif /* VK_KHR_synchronization2 */
#if defined(VK_KHR_synchronization2) && defined(VK_AMD_buffer_marker)
static PFN_vkCmdWriteBufferMarker2AMD pfn_vkCmdWriteBufferMarker2AMD= 0;
#endif /* VK_KHR_synchronization2 && VK_AMD_buffer_marker */
#if defined(VK_KHR_synchronization2) && defined(VK_NV_device_diagnostic_checkpoints)
static PFN_vkGetQueueCheckpointData2NV pfn_vkGetQueueCheckpointData2NV= 0;
#endif /* VK_KHR_synchronization2 && VK_NV_device_diagnostic_checkpoints */
#if defined(VK_KHR_timeline_semaphore)
static PFN_vkGetSemaphoreCounterValueKHR pfn_vkGetSemaphoreCounterValueKHR= 0;
static PFN_vkSignalSemaphoreKHR pfn_vkSignalSemaphoreKHR= 0;
static PFN_vkWaitSemaphoresKHR pfn_vkWaitSemaphoresKHR= 0;
#endif /* VK_KHR_timeline_semaphore */
#if defined(VK_KHR_video_decode_queue)
static PFN_vkCmdDecodeVideoKHR pfn_vkCmdDecodeVideoKHR= 0;
#endif /* VK_KHR_video_decode_queue */
#if defined(VK_KHR_video_queue)
static PFN_vkBindVideoSessionMemoryKHR pfn_vkBindVideoSessionMemoryKHR= 0;
static PFN_vkCmdBeginVideoCodingKHR pfn_vkCmdBeginVideoCodingKHR= 0;
static PFN_vkCmdControlVideoCodingKHR pfn_vkCmdControlVideoCodingKHR= 0;
static PFN_vkCmdEndVideoCodingKHR pfn_vkCmdEndVideoCodingKHR= 0;
static PFN_vkCreateVideoSessionKHR pfn_vkCreateVideoSessionKHR= 0;
static PFN_vkCreateVideoSessionParametersKHR pfn_vkCreateVideoSessionParametersKHR= 0;
static PFN_vkDestroyVideoSessionKHR pfn_vkDestroyVideoSessionKHR= 0;
static PFN_vkDestroyVideoSessionParametersKHR pfn_vkDestroyVideoSessionParametersKHR= 0;
static PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR pfn_vkGetPhysicalDeviceVideoCapabilitiesKHR= 0;
static PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR pfn_vkGetPhysicalDeviceVideoFormatPropertiesKHR= 0;
static PFN_vkGetVideoSessionMemoryRequirementsKHR pfn_vkGetVideoSessionMemoryRequirementsKHR= 0;
static PFN_vkUpdateVideoSessionParametersKHR pfn_vkUpdateVideoSessionParametersKHR= 0;
#endif /* VK_KHR_video_queue */
#if defined(VK_MVK_ios_surface)
static PFN_vkCreateIOSSurfaceMVK pfn_vkCreateIOSSurfaceMVK= 0;
#endif /* VK_MVK_ios_surface */
#if defined(VK_MVK_macos_surface)
static PFN_vkCreateMacOSSurfaceMVK pfn_vkCreateMacOSSurfaceMVK= 0;
#endif /* VK_MVK_macos_surface */
#if defined(VK_NN_vi_surface)
static PFN_vkCreateViSurfaceNN pfn_vkCreateViSurfaceNN= 0;
#endif /* VK_NN_vi_surface */
#if defined(VK_NVX_binary_import)
static PFN_vkCmdCuLaunchKernelNVX pfn_vkCmdCuLaunchKernelNVX= 0;
static PFN_vkCreateCuFunctionNVX pfn_vkCreateCuFunctionNVX= 0;
static PFN_vkCreateCuModuleNVX pfn_vkCreateCuModuleNVX= 0;
static PFN_vkDestroyCuFunctionNVX pfn_vkDestroyCuFunctionNVX= 0;
static PFN_vkDestroyCuModuleNVX pfn_vkDestroyCuModuleNVX= 0;
#endif /* VK_NVX_binary_import */
#if defined(VK_NVX_image_view_handle)
static PFN_vkGetImageViewAddressNVX pfn_vkGetImageViewAddressNVX= 0;
static PFN_vkGetImageViewHandleNVX pfn_vkGetImageViewHandleNVX= 0;
#endif /* VK_NVX_image_view_handle */
#if defined(VK_NV_acquire_winrt_display)
static PFN_vkAcquireWinrtDisplayNV pfn_vkAcquireWinrtDisplayNV= 0;
static PFN_vkGetWinrtDisplayNV pfn_vkGetWinrtDisplayNV= 0;
#endif /* VK_NV_acquire_winrt_display */
#if defined(VK_NV_clip_space_w_scaling)
static PFN_vkCmdSetViewportWScalingNV pfn_vkCmdSetViewportWScalingNV= 0;
#endif /* VK_NV_clip_space_w_scaling */
#if defined(VK_NV_cooperative_matrix)
static PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV pfn_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV= 0;
#endif /* VK_NV_cooperative_matrix */
#if defined(VK_NV_copy_memory_indirect)
static PFN_vkCmdCopyMemoryIndirectNV pfn_vkCmdCopyMemoryIndirectNV= 0;
static PFN_vkCmdCopyMemoryToImageIndirectNV pfn_vkCmdCopyMemoryToImageIndirectNV= 0;
#endif /* VK_NV_copy_memory_indirect */
#if defined(VK_NV_coverage_reduction_mode)
static PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV pfn_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV= 0;
#endif /* VK_NV_coverage_reduction_mode */
#if defined(VK_NV_device_diagnostic_checkpoints)
static PFN_vkCmdSetCheckpointNV pfn_vkCmdSetCheckpointNV= 0;
static PFN_vkGetQueueCheckpointDataNV pfn_vkGetQueueCheckpointDataNV= 0;
#endif /* VK_NV_device_diagnostic_checkpoints */
#if defined(VK_NV_device_generated_commands)
static PFN_vkCmdBindPipelineShaderGroupNV pfn_vkCmdBindPipelineShaderGroupNV= 0;
static PFN_vkCmdExecuteGeneratedCommandsNV pfn_vkCmdExecuteGeneratedCommandsNV= 0;
static PFN_vkCmdPreprocessGeneratedCommandsNV pfn_vkCmdPreprocessGeneratedCommandsNV= 0;
static PFN_vkCreateIndirectCommandsLayoutNV pfn_vkCreateIndirectCommandsLayoutNV= 0;
static PFN_vkDestroyIndirectCommandsLayoutNV pfn_vkDestroyIndirectCommandsLayoutNV= 0;
static PFN_vkGetGeneratedCommandsMemoryRequirementsNV pfn_vkGetGeneratedCommandsMemoryRequirementsNV= 0;
#endif /* VK_NV_device_generated_commands */
#if defined(VK_NV_device_generated_commands_compute)
static PFN_vkCmdUpdatePipelineIndirectBufferNV pfn_vkCmdUpdatePipelineIndirectBufferNV= 0;
static PFN_vkGetPipelineIndirectDeviceAddressNV pfn_vkGetPipelineIndirectDeviceAddressNV= 0;
static PFN_vkGetPipelineIndirectMemoryRequirementsNV pfn_vkGetPipelineIndirectMemoryRequirementsNV= 0;
#endif /* VK_NV_device_generated_commands_compute */
#if defined(VK_NV_external_memory_capabilities)
static PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV pfn_vkGetPhysicalDeviceExternalImageFormatPropertiesNV= 0;
#endif /* VK_NV_external_memory_capabilities */
#if defined(VK_NV_external_memory_rdma)
static PFN_vkGetMemoryRemoteAddressNV pfn_vkGetMemoryRemoteAddressNV= 0;
#endif /* VK_NV_external_memory_rdma */
#if defined(VK_NV_external_memory_win32)
static PFN_vkGetMemoryWin32HandleNV pfn_vkGetMemoryWin32HandleNV= 0;
#endif /* VK_NV_external_memory_win32 */
#if defined(VK_NV_fragment_shading_rate_enums)
static PFN_vkCmdSetFragmentShadingRateEnumNV pfn_vkCmdSetFragmentShadingRateEnumNV= 0;
#endif /* VK_NV_fragment_shading_rate_enums */
#if defined(VK_NV_memory_decompression)
static PFN_vkCmdDecompressMemoryIndirectCountNV pfn_vkCmdDecompressMemoryIndirectCountNV= 0;
static PFN_vkCmdDecompressMemoryNV pfn_vkCmdDecompressMemoryNV= 0;
#endif /* VK_NV_memory_decompression */
#if defined(VK_NV_mesh_shader)
static PFN_vkCmdDrawMeshTasksIndirectCountNV pfn_vkCmdDrawMeshTasksIndirectCountNV= 0;
static PFN_vkCmdDrawMeshTasksIndirectNV pfn_vkCmdDrawMeshTasksIndirectNV= 0;
static PFN_vkCmdDrawMeshTasksNV pfn_vkCmdDrawMeshTasksNV= 0;
#endif /* VK_NV_mesh_shader */
#if defined(VK_NV_optical_flow)
static PFN_vkBindOpticalFlowSessionImageNV pfn_vkBindOpticalFlowSessionImageNV= 0;
static PFN_vkCmdOpticalFlowExecuteNV pfn_vkCmdOpticalFlowExecuteNV= 0;
static PFN_vkCreateOpticalFlowSessionNV pfn_vkCreateOpticalFlowSessionNV= 0;
static PFN_vkDestroyOpticalFlowSessionNV pfn_vkDestroyOpticalFlowSessionNV= 0;
static PFN_vkGetPhysicalDeviceOpticalFlowImageFormatsNV pfn_vkGetPhysicalDeviceOpticalFlowImageFormatsNV= 0;
#endif /* VK_NV_optical_flow */
#if defined(VK_NV_ray_tracing)
static PFN_vkBindAccelerationStructureMemoryNV pfn_vkBindAccelerationStructureMemoryNV= 0;
static PFN_vkCmdBuildAccelerationStructureNV pfn_vkCmdBuildAccelerationStructureNV= 0;
static PFN_vkCmdCopyAccelerationStructureNV pfn_vkCmdCopyAccelerationStructureNV= 0;
static PFN_vkCmdTraceRaysNV pfn_vkCmdTraceRaysNV= 0;
static PFN_vkCmdWriteAccelerationStructuresPropertiesNV pfn_vkCmdWriteAccelerationStructuresPropertiesNV= 0;
static PFN_vkCompileDeferredNV pfn_vkCompileDeferredNV= 0;
static PFN_vkCreateAccelerationStructureNV pfn_vkCreateAccelerationStructureNV= 0;
static PFN_vkCreateRayTracingPipelinesNV pfn_vkCreateRayTracingPipelinesNV= 0;
static PFN_vkDestroyAccelerationStructureNV pfn_vkDestroyAccelerationStructureNV= 0;
static PFN_vkGetAccelerationStructureHandleNV pfn_vkGetAccelerationStructureHandleNV= 0;
static PFN_vkGetAccelerationStructureMemoryRequirementsNV pfn_vkGetAccelerationStructureMemoryRequirementsNV= 0;
static PFN_vkGetRayTracingShaderGroupHandlesNV pfn_vkGetRayTracingShaderGroupHandlesNV= 0;
#endif /* VK_NV_ray_tracing */
#if defined(VK_NV_scissor_exclusive) && VK_NV_SCISSOR_EXCLUSIVE_SPEC_VERSION >= 2
static PFN_vkCmdSetExclusiveScissorEnableNV pfn_vkCmdSetExclusiveScissorEnableNV= 0;
#endif /* VK_NV_scissor_exclusive && VK_NV_SCISSOR_EXCLUSIVE_SPEC_VERSION >= 2 */
#if defined(VK_NV_scissor_exclusive)
static PFN_vkCmdSetExclusiveScissorNV pfn_vkCmdSetExclusiveScissorNV= 0;
#endif /* VK_NV_scissor_exclusive */
#if defined(VK_NV_shading_rate_image)
static PFN_vkCmdBindShadingRateImageNV pfn_vkCmdBindShadingRateImageNV= 0;
static PFN_vkCmdSetCoarseSampleOrderNV pfn_vkCmdSetCoarseSampleOrderNV= 0;
static PFN_vkCmdSetViewportShadingRatePaletteNV pfn_vkCmdSetViewportShadingRatePaletteNV= 0;
#endif /* VK_NV_shading_rate_image */
#if defined(VK_QCOM_tile_properties)
static PFN_vkGetDynamicRenderingTilePropertiesQCOM pfn_vkGetDynamicRenderingTilePropertiesQCOM= 0;
static PFN_vkGetFramebufferTilePropertiesQCOM pfn_vkGetFramebufferTilePropertiesQCOM= 0;
#endif /* VK_QCOM_tile_properties */
#if defined(VK_QNX_external_memory_screen_buffer)
static PFN_vkGetScreenBufferPropertiesQNX pfn_vkGetScreenBufferPropertiesQNX= 0;
#endif /* VK_QNX_external_memory_screen_buffer */
#if defined(VK_QNX_screen_surface)
static PFN_vkCreateScreenSurfaceQNX pfn_vkCreateScreenSurfaceQNX= 0;
static PFN_vkGetPhysicalDeviceScreenPresentationSupportQNX pfn_vkGetPhysicalDeviceScreenPresentationSupportQNX= 0;
#endif /* VK_QNX_screen_surface */
#if defined(VK_VALVE_descriptor_set_host_mapping)
static PFN_vkGetDescriptorSetHostMappingVALVE pfn_vkGetDescriptorSetHostMappingVALVE= 0;
static PFN_vkGetDescriptorSetLayoutHostMappingInfoVALVE pfn_vkGetDescriptorSetLayoutHostMappingInfoVALVE= 0;
#endif /* VK_VALVE_descriptor_set_host_mapping */
#if defined(VK_EXT_extended_dynamic_state) || defined(VK_EXT_shader_object)
static PFN_vkCmdBindVertexBuffers2EXT pfn_vkCmdBindVertexBuffers2EXT= 0;
static PFN_vkCmdSetCullModeEXT pfn_vkCmdSetCullModeEXT= 0;
static PFN_vkCmdSetDepthBoundsTestEnableEXT pfn_vkCmdSetDepthBoundsTestEnableEXT= 0;
static PFN_vkCmdSetDepthCompareOpEXT pfn_vkCmdSetDepthCompareOpEXT= 0;
static PFN_vkCmdSetDepthTestEnableEXT pfn_vkCmdSetDepthTestEnableEXT= 0;
static PFN_vkCmdSetDepthWriteEnableEXT pfn_vkCmdSetDepthWriteEnableEXT= 0;
static PFN_vkCmdSetFrontFaceEXT pfn_vkCmdSetFrontFaceEXT= 0;
static PFN_vkCmdSetPrimitiveTopologyEXT pfn_vkCmdSetPrimitiveTopologyEXT= 0;
static PFN_vkCmdSetScissorWithCountEXT pfn_vkCmdSetScissorWithCountEXT= 0;
static PFN_vkCmdSetStencilOpEXT pfn_vkCmdSetStencilOpEXT= 0;
static PFN_vkCmdSetStencilTestEnableEXT pfn_vkCmdSetStencilTestEnableEXT= 0;
static PFN_vkCmdSetViewportWithCountEXT pfn_vkCmdSetViewportWithCountEXT= 0;
#endif /* VK_EXT_extended_dynamic_state || VK_EXT_shader_object */
#if defined(VK_EXT_extended_dynamic_state2) || defined(VK_EXT_shader_object)
static PFN_vkCmdSetDepthBiasEnableEXT pfn_vkCmdSetDepthBiasEnableEXT= 0;
static PFN_vkCmdSetLogicOpEXT pfn_vkCmdSetLogicOpEXT= 0;
static PFN_vkCmdSetPatchControlPointsEXT pfn_vkCmdSetPatchControlPointsEXT= 0;
static PFN_vkCmdSetPrimitiveRestartEnableEXT pfn_vkCmdSetPrimitiveRestartEnableEXT= 0;
static PFN_vkCmdSetRasterizerDiscardEnableEXT pfn_vkCmdSetRasterizerDiscardEnableEXT= 0;
#endif /* VK_EXT_extended_dynamic_state2 || VK_EXT_shader_object */
#if defined(VK_EXT_extended_dynamic_state3) || defined(VK_EXT_shader_object)
static PFN_vkCmdSetAlphaToCoverageEnableEXT pfn_vkCmdSetAlphaToCoverageEnableEXT= 0;
static PFN_vkCmdSetAlphaToOneEnableEXT pfn_vkCmdSetAlphaToOneEnableEXT= 0;
static PFN_vkCmdSetColorBlendAdvancedEXT pfn_vkCmdSetColorBlendAdvancedEXT= 0;
static PFN_vkCmdSetColorBlendEnableEXT pfn_vkCmdSetColorBlendEnableEXT= 0;
static PFN_vkCmdSetColorBlendEquationEXT pfn_vkCmdSetColorBlendEquationEXT= 0;
static PFN_vkCmdSetColorWriteMaskEXT pfn_vkCmdSetColorWriteMaskEXT= 0;
static PFN_vkCmdSetConservativeRasterizationModeEXT pfn_vkCmdSetConservativeRasterizationModeEXT= 0;
static PFN_vkCmdSetDepthClampEnableEXT pfn_vkCmdSetDepthClampEnableEXT= 0;
static PFN_vkCmdSetDepthClipEnableEXT pfn_vkCmdSetDepthClipEnableEXT= 0;
static PFN_vkCmdSetDepthClipNegativeOneToOneEXT pfn_vkCmdSetDepthClipNegativeOneToOneEXT= 0;
static PFN_vkCmdSetExtraPrimitiveOverestimationSizeEXT pfn_vkCmdSetExtraPrimitiveOverestimationSizeEXT= 0;
static PFN_vkCmdSetLineRasterizationModeEXT pfn_vkCmdSetLineRasterizationModeEXT= 0;
static PFN_vkCmdSetLineStippleEnableEXT pfn_vkCmdSetLineStippleEnableEXT= 0;
static PFN_vkCmdSetLogicOpEnableEXT pfn_vkCmdSetLogicOpEnableEXT= 0;
static PFN_vkCmdSetPolygonModeEXT pfn_vkCmdSetPolygonModeEXT= 0;
static PFN_vkCmdSetProvokingVertexModeEXT pfn_vkCmdSetProvokingVertexModeEXT= 0;
static PFN_vkCmdSetRasterizationSamplesEXT pfn_vkCmdSetRasterizationSamplesEXT= 0;
static PFN_vkCmdSetRasterizationStreamEXT pfn_vkCmdSetRasterizationStreamEXT= 0;
static PFN_vkCmdSetSampleLocationsEnableEXT pfn_vkCmdSetSampleLocationsEnableEXT= 0;
static PFN_vkCmdSetSampleMaskEXT pfn_vkCmdSetSampleMaskEXT= 0;
static PFN_vkCmdSetTessellationDomainOriginEXT pfn_vkCmdSetTessellationDomainOriginEXT= 0;
#endif /* VK_EXT_extended_dynamic_state3 || VK_EXT_shader_object */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_clip_space_w_scaling) || defined(VK_EXT_shader_object) && defined(VK_NV_clip_space_w_scaling)
static PFN_vkCmdSetViewportWScalingEnableNV pfn_vkCmdSetViewportWScalingEnableNV= 0;
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_clip_space_w_scaling || VK_EXT_shader_object && VK_NV_clip_space_w_scaling */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_viewport_swizzle) || defined(VK_EXT_shader_object) && defined(VK_NV_viewport_swizzle)
static PFN_vkCmdSetViewportSwizzleNV pfn_vkCmdSetViewportSwizzleNV= 0;
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_viewport_swizzle || VK_EXT_shader_object && VK_NV_viewport_swizzle */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_fragment_coverage_to_color) || defined(VK_EXT_shader_object) && defined(VK_NV_fragment_coverage_to_color)
static PFN_vkCmdSetCoverageToColorEnableNV pfn_vkCmdSetCoverageToColorEnableNV= 0;
static PFN_vkCmdSetCoverageToColorLocationNV pfn_vkCmdSetCoverageToColorLocationNV= 0;
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_fragment_coverage_to_color || VK_EXT_shader_object && VK_NV_fragment_coverage_to_color */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_framebuffer_mixed_samples) || defined(VK_EXT_shader_object) && defined(VK_NV_framebuffer_mixed_samples)
static PFN_vkCmdSetCoverageModulationModeNV pfn_vkCmdSetCoverageModulationModeNV= 0;
static PFN_vkCmdSetCoverageModulationTableEnableNV pfn_vkCmdSetCoverageModulationTableEnableNV= 0;
static PFN_vkCmdSetCoverageModulationTableNV pfn_vkCmdSetCoverageModulationTableNV= 0;
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_framebuffer_mixed_samples || VK_EXT_shader_object && VK_NV_framebuffer_mixed_samples */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_shading_rate_image) || defined(VK_EXT_shader_object) && defined(VK_NV_shading_rate_image)
static PFN_vkCmdSetShadingRateImageEnableNV pfn_vkCmdSetShadingRateImageEnableNV= 0;
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_shading_rate_image || VK_EXT_shader_object && VK_NV_shading_rate_image */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_representative_fragment_test) || defined(VK_EXT_shader_object) && defined(VK_NV_representative_fragment_test)
static PFN_vkCmdSetRepresentativeFragmentTestEnableNV pfn_vkCmdSetRepresentativeFragmentTestEnableNV= 0;
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_representative_fragment_test || VK_EXT_shader_object && VK_NV_representative_fragment_test */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_coverage_reduction_mode) || defined(VK_EXT_shader_object) && defined(VK_NV_coverage_reduction_mode)
static PFN_vkCmdSetCoverageReductionModeNV pfn_vkCmdSetCoverageReductionModeNV= 0;
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_coverage_reduction_mode || VK_EXT_shader_object && VK_NV_coverage_reduction_mode */
#if defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group) || defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)
static PFN_vkGetDeviceGroupSurfacePresentModes2EXT pfn_vkGetDeviceGroupSurfacePresentModes2EXT= 0;
#endif /* VK_EXT_full_screen_exclusive && VK_KHR_device_group || VK_EXT_full_screen_exclusive && VK_VERSION_1_1 */
#if defined(VK_EXT_host_image_copy) || defined(VK_EXT_image_compression_control)
static PFN_vkGetImageSubresourceLayout2EXT pfn_vkGetImageSubresourceLayout2EXT= 0;
#endif /* VK_EXT_host_image_copy || VK_EXT_image_compression_control */
#if defined(VK_EXT_shader_object) || defined(VK_EXT_vertex_input_dynamic_state)
static PFN_vkCmdSetVertexInputEXT pfn_vkCmdSetVertexInputEXT= 0;
#endif /* VK_EXT_shader_object || VK_EXT_vertex_input_dynamic_state */
#if defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor) || defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1) || defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template)
static PFN_vkCmdPushDescriptorSetWithTemplateKHR pfn_vkCmdPushDescriptorSetWithTemplateKHR= 0;
#endif /* VK_KHR_descriptor_update_template && VK_KHR_push_descriptor || VK_KHR_push_descriptor && VK_VERSION_1_1 || VK_KHR_push_descriptor && VK_KHR_descriptor_update_template */
#if defined(VK_KHR_device_group) && defined(VK_KHR_surface) || defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)
static PFN_vkGetDeviceGroupPresentCapabilitiesKHR pfn_vkGetDeviceGroupPresentCapabilitiesKHR= 0;
static PFN_vkGetDeviceGroupSurfacePresentModesKHR pfn_vkGetDeviceGroupSurfacePresentModesKHR= 0;
static PFN_vkGetPhysicalDevicePresentRectanglesKHR pfn_vkGetPhysicalDevicePresentRectanglesKHR= 0;
#endif /* VK_KHR_device_group && VK_KHR_surface || VK_KHR_swapchain && VK_VERSION_1_1 */
#if defined(VK_KHR_device_group) && defined(VK_KHR_swapchain) || defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)
static PFN_vkAcquireNextImage2KHR pfn_vkAcquireNextImage2KHR= 0;
#endif /* VK_KHR_device_group && VK_KHR_swapchain || VK_KHR_swapchain && VK_VERSION_1_1 */
/* NVVK_GENERATE_STATIC_PFN */


/* NVVK_GENERATE_DECLARE */
#if defined(VK_AMD_buffer_marker)
VKAPI_ATTR void VKAPI_CALL vkCmdWriteBufferMarkerAMD(
	VkCommandBuffer commandBuffer, 
	VkPipelineStageFlagBits pipelineStage, 
	VkBuffer dstBuffer, 
	VkDeviceSize dstOffset, 
	uint32_t marker) 
{ 
  pfn_vkCmdWriteBufferMarkerAMD(commandBuffer, pipelineStage, dstBuffer, dstOffset, marker); 
}
#endif /* VK_AMD_buffer_marker */
#if defined(VK_AMD_display_native_hdr)
VKAPI_ATTR void VKAPI_CALL vkSetLocalDimmingAMD(
	VkDevice device, 
	VkSwapchainKHR swapChain, 
	VkBool32 localDimmingEnable) 
{ 
  pfn_vkSetLocalDimmingAMD(device, swapChain, localDimmingEnable); 
}
#endif /* VK_AMD_display_native_hdr */
#if defined(VK_AMD_draw_indirect_count)
VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirectCountAMD(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset, 
	VkBuffer countBuffer, 
	VkDeviceSize countBufferOffset, 
	uint32_t maxDrawCount, 
	uint32_t stride) 
{ 
  pfn_vkCmdDrawIndexedIndirectCountAMD(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectCountAMD(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset, 
	VkBuffer countBuffer, 
	VkDeviceSize countBufferOffset, 
	uint32_t maxDrawCount, 
	uint32_t stride) 
{ 
  pfn_vkCmdDrawIndirectCountAMD(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride); 
}
#endif /* VK_AMD_draw_indirect_count */
#if defined(VK_AMD_shader_info)
VKAPI_ATTR VkResult VKAPI_CALL vkGetShaderInfoAMD(
	VkDevice device, 
	VkPipeline pipeline, 
	VkShaderStageFlagBits shaderStage, 
	VkShaderInfoTypeAMD infoType, 
	size_t* pInfoSize, 
	void* pInfo) 
{ 
  return pfn_vkGetShaderInfoAMD(device, pipeline, shaderStage, infoType, pInfoSize, pInfo); 
}
#endif /* VK_AMD_shader_info */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
VKAPI_ATTR VkResult VKAPI_CALL vkGetAndroidHardwareBufferPropertiesANDROID(
	VkDevice device, 
	const struct AHardwareBuffer* buffer, 
	VkAndroidHardwareBufferPropertiesANDROID* pProperties) 
{ 
  return pfn_vkGetAndroidHardwareBufferPropertiesANDROID(device, buffer, pProperties); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryAndroidHardwareBufferANDROID(
	VkDevice device, 
	const VkMemoryGetAndroidHardwareBufferInfoANDROID* pInfo, 
	struct AHardwareBuffer** pBuffer) 
{ 
  return pfn_vkGetMemoryAndroidHardwareBufferANDROID(device, pInfo, pBuffer); 
}
#endif /* VK_ANDROID_external_memory_android_hardware_buffer */
#if defined(VK_EXT_acquire_drm_display)
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireDrmDisplayEXT(
	VkPhysicalDevice physicalDevice, 
	int32_t drmFd, 
	VkDisplayKHR display) 
{ 
  return pfn_vkAcquireDrmDisplayEXT(physicalDevice, drmFd, display); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetDrmDisplayEXT(
	VkPhysicalDevice physicalDevice, 
	int32_t drmFd, 
	uint32_t connectorId, 
	VkDisplayKHR* display) 
{ 
  return pfn_vkGetDrmDisplayEXT(physicalDevice, drmFd, connectorId, display); 
}
#endif /* VK_EXT_acquire_drm_display */
#if defined(VK_EXT_acquire_xlib_display)
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireXlibDisplayEXT(
	VkPhysicalDevice physicalDevice, 
	Display* dpy, 
	VkDisplayKHR display) 
{ 
  return pfn_vkAcquireXlibDisplayEXT(physicalDevice, dpy, display); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetRandROutputDisplayEXT(
	VkPhysicalDevice physicalDevice, 
	Display* dpy, 
	RROutput rrOutput, 
	VkDisplayKHR* pDisplay) 
{ 
  return pfn_vkGetRandROutputDisplayEXT(physicalDevice, dpy, rrOutput, pDisplay); 
}
#endif /* VK_EXT_acquire_xlib_display */
#if defined(VK_EXT_attachment_feedback_loop_dynamic_state)
VKAPI_ATTR void VKAPI_CALL vkCmdSetAttachmentFeedbackLoopEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkImageAspectFlags aspectMask) 
{ 
  pfn_vkCmdSetAttachmentFeedbackLoopEnableEXT(commandBuffer, aspectMask); 
}
#endif /* VK_EXT_attachment_feedback_loop_dynamic_state */
#if defined(VK_EXT_buffer_device_address)
VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetBufferDeviceAddressEXT(
	VkDevice device, 
	const VkBufferDeviceAddressInfo* pInfo) 
{ 
  return pfn_vkGetBufferDeviceAddressEXT(device, pInfo); 
}
#endif /* VK_EXT_buffer_device_address */
#if defined(VK_EXT_calibrated_timestamps)
VKAPI_ATTR VkResult VKAPI_CALL vkGetCalibratedTimestampsEXT(
	VkDevice device, 
	uint32_t timestampCount, 
	const VkCalibratedTimestampInfoEXT* pTimestampInfos, 
	uint64_t* pTimestamps, 
	uint64_t* pMaxDeviation) 
{ 
  return pfn_vkGetCalibratedTimestampsEXT(device, timestampCount, pTimestampInfos, pTimestamps, pMaxDeviation); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(
	VkPhysicalDevice physicalDevice, 
	uint32_t* pTimeDomainCount, 
	VkTimeDomainEXT* pTimeDomains) 
{ 
  return pfn_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(physicalDevice, pTimeDomainCount, pTimeDomains); 
}
#endif /* VK_EXT_calibrated_timestamps */
#if defined(VK_EXT_color_write_enable)
VKAPI_ATTR void VKAPI_CALL vkCmdSetColorWriteEnableEXT(
	VkCommandBuffer       commandBuffer, 
	uint32_t                                attachmentCount, 
	const VkBool32*   pColorWriteEnables) 
{ 
  pfn_vkCmdSetColorWriteEnableEXT(commandBuffer, attachmentCount, pColorWriteEnables); 
}
#endif /* VK_EXT_color_write_enable */
#if defined(VK_EXT_conditional_rendering)
VKAPI_ATTR void VKAPI_CALL vkCmdBeginConditionalRenderingEXT(
	VkCommandBuffer commandBuffer, 
	const VkConditionalRenderingBeginInfoEXT* pConditionalRenderingBegin) 
{ 
  pfn_vkCmdBeginConditionalRenderingEXT(commandBuffer, pConditionalRenderingBegin); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdEndConditionalRenderingEXT(
	VkCommandBuffer commandBuffer) 
{ 
  pfn_vkCmdEndConditionalRenderingEXT(commandBuffer); 
}
#endif /* VK_EXT_conditional_rendering */
#if defined(VK_EXT_debug_marker)
VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerBeginEXT(
	VkCommandBuffer commandBuffer, 
	const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) 
{ 
  pfn_vkCmdDebugMarkerBeginEXT(commandBuffer, pMarkerInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerEndEXT(
	VkCommandBuffer commandBuffer) 
{ 
  pfn_vkCmdDebugMarkerEndEXT(commandBuffer); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerInsertEXT(
	VkCommandBuffer commandBuffer, 
	const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) 
{ 
  pfn_vkCmdDebugMarkerInsertEXT(commandBuffer, pMarkerInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectNameEXT(
	VkDevice device, 
	const VkDebugMarkerObjectNameInfoEXT* pNameInfo) 
{ 
  return pfn_vkDebugMarkerSetObjectNameEXT(device, pNameInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectTagEXT(
	VkDevice device, 
	const VkDebugMarkerObjectTagInfoEXT* pTagInfo) 
{ 
  return pfn_vkDebugMarkerSetObjectTagEXT(device, pTagInfo); 
}
#endif /* VK_EXT_debug_marker */
#if defined(VK_EXT_debug_report)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
	VkInstance instance, 
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugReportCallbackEXT* pCallback) 
{ 
  return pfn_vkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback); 
}
VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(
	VkInstance instance, 
	VkDebugReportFlagsEXT flags, 
	VkDebugReportObjectTypeEXT objectType, 
	uint64_t object, 
	size_t location, 
	int32_t messageCode, 
	const char* pLayerPrefix, 
	const char* pMessage) 
{ 
  pfn_vkDebugReportMessageEXT(instance, flags, objectType, object, location, messageCode, pLayerPrefix, pMessage); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
	VkInstance instance, 
	VkDebugReportCallbackEXT callback, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyDebugReportCallbackEXT(instance, callback, pAllocator); 
}
#endif /* VK_EXT_debug_report */
#if defined(VK_EXT_debug_utils)
VKAPI_ATTR void VKAPI_CALL vkCmdBeginDebugUtilsLabelEXT(
	VkCommandBuffer commandBuffer, 
	const VkDebugUtilsLabelEXT* pLabelInfo) 
{ 
  pfn_vkCmdBeginDebugUtilsLabelEXT(commandBuffer, pLabelInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdEndDebugUtilsLabelEXT(
	VkCommandBuffer commandBuffer) 
{ 
  pfn_vkCmdEndDebugUtilsLabelEXT(commandBuffer); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdInsertDebugUtilsLabelEXT(
	VkCommandBuffer commandBuffer, 
	const VkDebugUtilsLabelEXT* pLabelInfo) 
{ 
  pfn_vkCmdInsertDebugUtilsLabelEXT(commandBuffer, pLabelInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
	VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugUtilsMessengerEXT* pMessenger) 
{ 
  return pfn_vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
	VkInstance instance, 
	VkDebugUtilsMessengerEXT messenger, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator); 
}
VKAPI_ATTR void VKAPI_CALL vkQueueBeginDebugUtilsLabelEXT(
	VkQueue queue, 
	const VkDebugUtilsLabelEXT* pLabelInfo) 
{ 
  pfn_vkQueueBeginDebugUtilsLabelEXT(queue, pLabelInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkQueueEndDebugUtilsLabelEXT(
	VkQueue queue) 
{ 
  pfn_vkQueueEndDebugUtilsLabelEXT(queue); 
}
VKAPI_ATTR void VKAPI_CALL vkQueueInsertDebugUtilsLabelEXT(
	VkQueue queue, 
	const VkDebugUtilsLabelEXT* pLabelInfo) 
{ 
  pfn_vkQueueInsertDebugUtilsLabelEXT(queue, pLabelInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(
	VkDevice device, 
	const VkDebugUtilsObjectNameInfoEXT* pNameInfo) 
{ 
  return pfn_vkSetDebugUtilsObjectNameEXT(device, pNameInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectTagEXT(
	VkDevice device, 
	const VkDebugUtilsObjectTagInfoEXT* pTagInfo) 
{ 
  return pfn_vkSetDebugUtilsObjectTagEXT(device, pTagInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkSubmitDebugUtilsMessageEXT(
	VkInstance instance, 
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
	VkDebugUtilsMessageTypeFlagsEXT messageTypes, 
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData) 
{ 
  pfn_vkSubmitDebugUtilsMessageEXT(instance, messageSeverity, messageTypes, pCallbackData); 
}
#endif /* VK_EXT_debug_utils */
#if defined(VK_EXT_depth_bias_control)
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias2EXT(
	VkCommandBuffer commandBuffer, 
	const VkDepthBiasInfoEXT*         pDepthBiasInfo) 
{ 
  pfn_vkCmdSetDepthBias2EXT(commandBuffer, pDepthBiasInfo); 
}
#endif /* VK_EXT_depth_bias_control */
#if defined(VK_EXT_descriptor_buffer)
VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorBufferEmbeddedSamplersEXT(
	VkCommandBuffer commandBuffer, 
	VkPipelineBindPoint pipelineBindPoint, 
	VkPipelineLayout layout, 
	uint32_t set) 
{ 
  pfn_vkCmdBindDescriptorBufferEmbeddedSamplersEXT(commandBuffer, pipelineBindPoint, layout, set); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorBuffersEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t bufferCount, 
	const VkDescriptorBufferBindingInfoEXT* pBindingInfos) 
{ 
  pfn_vkCmdBindDescriptorBuffersEXT(commandBuffer, bufferCount, pBindingInfos); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDescriptorBufferOffsetsEXT(
	VkCommandBuffer commandBuffer, 
	VkPipelineBindPoint pipelineBindPoint, 
	VkPipelineLayout layout, 
	uint32_t firstSet, 
	uint32_t setCount, 
	const uint32_t* pBufferIndices, 
	const VkDeviceSize* pOffsets) 
{ 
  pfn_vkCmdSetDescriptorBufferOffsetsEXT(commandBuffer, pipelineBindPoint, layout, firstSet, setCount, pBufferIndices, pOffsets); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetBufferOpaqueCaptureDescriptorDataEXT(
	VkDevice device, 
	const VkBufferCaptureDescriptorDataInfoEXT* pInfo, 
	void* pData) 
{ 
  return pfn_vkGetBufferOpaqueCaptureDescriptorDataEXT(device, pInfo, pData); 
}
VKAPI_ATTR void VKAPI_CALL vkGetDescriptorEXT(
	VkDevice device, 
	const VkDescriptorGetInfoEXT* pDescriptorInfo, 
	size_t dataSize, 
	void* pDescriptor) 
{ 
  pfn_vkGetDescriptorEXT(device, pDescriptorInfo, dataSize, pDescriptor); 
}
VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutBindingOffsetEXT(
	VkDevice device, 
	VkDescriptorSetLayout layout, 
	uint32_t binding, 
	VkDeviceSize* pOffset) 
{ 
  pfn_vkGetDescriptorSetLayoutBindingOffsetEXT(device, layout, binding, pOffset); 
}
VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutSizeEXT(
	VkDevice device, 
	VkDescriptorSetLayout layout, 
	VkDeviceSize* pLayoutSizeInBytes) 
{ 
  pfn_vkGetDescriptorSetLayoutSizeEXT(device, layout, pLayoutSizeInBytes); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetImageOpaqueCaptureDescriptorDataEXT(
	VkDevice device, 
	const VkImageCaptureDescriptorDataInfoEXT* pInfo, 
	void* pData) 
{ 
  return pfn_vkGetImageOpaqueCaptureDescriptorDataEXT(device, pInfo, pData); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetImageViewOpaqueCaptureDescriptorDataEXT(
	VkDevice device, 
	const VkImageViewCaptureDescriptorDataInfoEXT* pInfo, 
	void* pData) 
{ 
  return pfn_vkGetImageViewOpaqueCaptureDescriptorDataEXT(device, pInfo, pData); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetSamplerOpaqueCaptureDescriptorDataEXT(
	VkDevice device, 
	const VkSamplerCaptureDescriptorDataInfoEXT* pInfo, 
	void* pData) 
{ 
  return pfn_vkGetSamplerOpaqueCaptureDescriptorDataEXT(device, pInfo, pData); 
}
#endif /* VK_EXT_descriptor_buffer */
#if defined(VK_EXT_descriptor_buffer) && (defined(VK_KHR_acceleration_structure) || defined(VK_NV_ray_tracing))
VKAPI_ATTR VkResult VKAPI_CALL vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT(
	VkDevice device, 
	const VkAccelerationStructureCaptureDescriptorDataInfoEXT* pInfo, 
	void* pData) 
{ 
  return pfn_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT(device, pInfo, pData); 
}
#endif /* VK_EXT_descriptor_buffer && (VK_KHR_acceleration_structure || VK_NV_ray_tracing) */
#if defined(VK_EXT_device_fault)
VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceFaultInfoEXT(
	VkDevice device, 
	VkDeviceFaultCountsEXT* pFaultCounts, 
	VkDeviceFaultInfoEXT* pFaultInfo) 
{ 
  return pfn_vkGetDeviceFaultInfoEXT(device, pFaultCounts, pFaultInfo); 
}
#endif /* VK_EXT_device_fault */
#if defined(VK_EXT_direct_mode_display)
VKAPI_ATTR VkResult VKAPI_CALL vkReleaseDisplayEXT(
	VkPhysicalDevice physicalDevice, 
	VkDisplayKHR display) 
{ 
  return pfn_vkReleaseDisplayEXT(physicalDevice, display); 
}
#endif /* VK_EXT_direct_mode_display */
#if defined(VK_EXT_directfb_surface)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDirectFBSurfaceEXT(
	VkInstance instance, 
	const VkDirectFBSurfaceCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkSurfaceKHR* pSurface) 
{ 
  return pfn_vkCreateDirectFBSurfaceEXT(instance, pCreateInfo, pAllocator, pSurface); 
}
VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceDirectFBPresentationSupportEXT(
	VkPhysicalDevice physicalDevice, 
	uint32_t queueFamilyIndex, 
	IDirectFB* dfb) 
{ 
  return pfn_vkGetPhysicalDeviceDirectFBPresentationSupportEXT(physicalDevice, queueFamilyIndex, dfb); 
}
#endif /* VK_EXT_directfb_surface */
#if defined(VK_EXT_discard_rectangles)
VKAPI_ATTR void VKAPI_CALL vkCmdSetDiscardRectangleEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t firstDiscardRectangle, 
	uint32_t discardRectangleCount, 
	const VkRect2D* pDiscardRectangles) 
{ 
  pfn_vkCmdSetDiscardRectangleEXT(commandBuffer, firstDiscardRectangle, discardRectangleCount, pDiscardRectangles); 
}
#endif /* VK_EXT_discard_rectangles */
#if defined(VK_EXT_discard_rectangles) && VK_EXT_DISCARD_RECTANGLES_SPEC_VERSION >= 2
VKAPI_ATTR void VKAPI_CALL vkCmdSetDiscardRectangleEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 discardRectangleEnable) 
{ 
  pfn_vkCmdSetDiscardRectangleEnableEXT(commandBuffer, discardRectangleEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDiscardRectangleModeEXT(
	VkCommandBuffer commandBuffer, 
	VkDiscardRectangleModeEXT discardRectangleMode) 
{ 
  pfn_vkCmdSetDiscardRectangleModeEXT(commandBuffer, discardRectangleMode); 
}
#endif /* VK_EXT_discard_rectangles && VK_EXT_DISCARD_RECTANGLES_SPEC_VERSION >= 2 */
#if defined(VK_EXT_display_control)
VKAPI_ATTR VkResult VKAPI_CALL vkDisplayPowerControlEXT(
	VkDevice device, 
	VkDisplayKHR display, 
	const VkDisplayPowerInfoEXT* pDisplayPowerInfo) 
{ 
  return pfn_vkDisplayPowerControlEXT(device, display, pDisplayPowerInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainCounterEXT(
	VkDevice device, 
	VkSwapchainKHR swapchain, 
	VkSurfaceCounterFlagBitsEXT counter, 
	uint64_t* pCounterValue) 
{ 
  return pfn_vkGetSwapchainCounterEXT(device, swapchain, counter, pCounterValue); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkRegisterDeviceEventEXT(
	VkDevice device, 
	const VkDeviceEventInfoEXT* pDeviceEventInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkFence* pFence) 
{ 
  return pfn_vkRegisterDeviceEventEXT(device, pDeviceEventInfo, pAllocator, pFence); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkRegisterDisplayEventEXT(
	VkDevice device, 
	VkDisplayKHR display, 
	const VkDisplayEventInfoEXT* pDisplayEventInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkFence* pFence) 
{ 
  return pfn_vkRegisterDisplayEventEXT(device, display, pDisplayEventInfo, pAllocator, pFence); 
}
#endif /* VK_EXT_display_control */
#if defined(VK_EXT_display_surface_counter)
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilities2EXT(
	VkPhysicalDevice physicalDevice, 
	VkSurfaceKHR surface, 
	VkSurfaceCapabilities2EXT* pSurfaceCapabilities) 
{ 
  return pfn_vkGetPhysicalDeviceSurfaceCapabilities2EXT(physicalDevice, surface, pSurfaceCapabilities); 
}
#endif /* VK_EXT_display_surface_counter */
#if defined(VK_EXT_external_memory_host)
VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryHostPointerPropertiesEXT(
	VkDevice device, 
	VkExternalMemoryHandleTypeFlagBits handleType, 
	const void* pHostPointer, 
	VkMemoryHostPointerPropertiesEXT* pMemoryHostPointerProperties) 
{ 
  return pfn_vkGetMemoryHostPointerPropertiesEXT(device, handleType, pHostPointer, pMemoryHostPointerProperties); 
}
#endif /* VK_EXT_external_memory_host */
#if defined(VK_EXT_full_screen_exclusive)
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireFullScreenExclusiveModeEXT(
	VkDevice device, 
	VkSwapchainKHR swapchain) 
{ 
  return pfn_vkAcquireFullScreenExclusiveModeEXT(device, swapchain); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModes2EXT(
	VkPhysicalDevice physicalDevice, 
	const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, 
	uint32_t* pPresentModeCount, 
	VkPresentModeKHR* pPresentModes) 
{ 
  return pfn_vkGetPhysicalDeviceSurfacePresentModes2EXT(physicalDevice, pSurfaceInfo, pPresentModeCount, pPresentModes); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkReleaseFullScreenExclusiveModeEXT(
	VkDevice device, 
	VkSwapchainKHR swapchain) 
{ 
  return pfn_vkReleaseFullScreenExclusiveModeEXT(device, swapchain); 
}
#endif /* VK_EXT_full_screen_exclusive */
#if defined(VK_EXT_hdr_metadata)
VKAPI_ATTR void VKAPI_CALL vkSetHdrMetadataEXT(
	VkDevice device, 
	uint32_t swapchainCount, 
	const VkSwapchainKHR* pSwapchains, 
	const VkHdrMetadataEXT* pMetadata) 
{ 
  pfn_vkSetHdrMetadataEXT(device, swapchainCount, pSwapchains, pMetadata); 
}
#endif /* VK_EXT_hdr_metadata */
#if defined(VK_EXT_headless_surface)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateHeadlessSurfaceEXT(
	VkInstance instance, 
	const VkHeadlessSurfaceCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkSurfaceKHR* pSurface) 
{ 
  return pfn_vkCreateHeadlessSurfaceEXT(instance, pCreateInfo, pAllocator, pSurface); 
}
#endif /* VK_EXT_headless_surface */
#if defined(VK_EXT_host_image_copy)
VKAPI_ATTR VkResult VKAPI_CALL vkCopyImageToImageEXT(
	VkDevice device, 
	const VkCopyImageToImageInfoEXT* pCopyImageToImageInfo) 
{ 
  return pfn_vkCopyImageToImageEXT(device, pCopyImageToImageInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyImageToMemoryEXT(
	VkDevice device, 
	const VkCopyImageToMemoryInfoEXT* pCopyImageToMemoryInfo) 
{ 
  return pfn_vkCopyImageToMemoryEXT(device, pCopyImageToMemoryInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyMemoryToImageEXT(
	VkDevice device, 
	const VkCopyMemoryToImageInfoEXT* pCopyMemoryToImageInfo) 
{ 
  return pfn_vkCopyMemoryToImageEXT(device, pCopyMemoryToImageInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkTransitionImageLayoutEXT(
	VkDevice device, 
	uint32_t transitionCount, 
	const VkHostImageLayoutTransitionInfoEXT* pTransitions) 
{ 
  return pfn_vkTransitionImageLayoutEXT(device, transitionCount, pTransitions); 
}
#endif /* VK_EXT_host_image_copy */
#if defined(VK_EXT_host_query_reset)
VKAPI_ATTR void VKAPI_CALL vkResetQueryPoolEXT(
	VkDevice device, 
	VkQueryPool queryPool, 
	uint32_t firstQuery, 
	uint32_t queryCount) 
{ 
  pfn_vkResetQueryPoolEXT(device, queryPool, firstQuery, queryCount); 
}
#endif /* VK_EXT_host_query_reset */
#if defined(VK_EXT_image_drm_format_modifier)
VKAPI_ATTR VkResult VKAPI_CALL vkGetImageDrmFormatModifierPropertiesEXT(
	VkDevice device, 
	VkImage image, 
	VkImageDrmFormatModifierPropertiesEXT* pProperties) 
{ 
  return pfn_vkGetImageDrmFormatModifierPropertiesEXT(device, image, pProperties); 
}
#endif /* VK_EXT_image_drm_format_modifier */
#if defined(VK_EXT_line_rasterization)
VKAPI_ATTR void VKAPI_CALL vkCmdSetLineStippleEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t lineStippleFactor, 
	uint16_t lineStipplePattern) 
{ 
  pfn_vkCmdSetLineStippleEXT(commandBuffer, lineStippleFactor, lineStipplePattern); 
}
#endif /* VK_EXT_line_rasterization */
#if defined(VK_EXT_mesh_shader)
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t groupCountX, 
	uint32_t groupCountY, 
	uint32_t groupCountZ) 
{ 
  pfn_vkCmdDrawMeshTasksEXT(commandBuffer, groupCountX, groupCountY, groupCountZ); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectCountEXT(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset, 
	VkBuffer countBuffer, 
	VkDeviceSize countBufferOffset, 
	uint32_t maxDrawCount, 
	uint32_t stride) 
{ 
  pfn_vkCmdDrawMeshTasksIndirectCountEXT(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectEXT(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset, 
	uint32_t drawCount, 
	uint32_t stride) 
{ 
  pfn_vkCmdDrawMeshTasksIndirectEXT(commandBuffer, buffer, offset, drawCount, stride); 
}
#endif /* VK_EXT_mesh_shader */
#if defined(VK_EXT_metal_objects)
VKAPI_ATTR void VKAPI_CALL vkExportMetalObjectsEXT(
	VkDevice device, 
	VkExportMetalObjectsInfoEXT* pMetalObjectsInfo) 
{ 
  pfn_vkExportMetalObjectsEXT(device, pMetalObjectsInfo); 
}
#endif /* VK_EXT_metal_objects */
#if defined(VK_EXT_metal_surface)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateMetalSurfaceEXT(
	VkInstance instance, 
	const VkMetalSurfaceCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkSurfaceKHR* pSurface) 
{ 
  return pfn_vkCreateMetalSurfaceEXT(instance, pCreateInfo, pAllocator, pSurface); 
}
#endif /* VK_EXT_metal_surface */
#if defined(VK_EXT_multi_draw)
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMultiEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t drawCount, 
	const VkMultiDrawInfoEXT* pVertexInfo, 
	uint32_t instanceCount, 
	uint32_t firstInstance, 
	uint32_t stride) 
{ 
  pfn_vkCmdDrawMultiEXT(commandBuffer, drawCount, pVertexInfo, instanceCount, firstInstance, stride); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMultiIndexedEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t drawCount, 
	const VkMultiDrawIndexedInfoEXT* pIndexInfo, 
	uint32_t instanceCount, 
	uint32_t firstInstance, 
	uint32_t stride, 
	const int32_t* pVertexOffset) 
{ 
  pfn_vkCmdDrawMultiIndexedEXT(commandBuffer, drawCount, pIndexInfo, instanceCount, firstInstance, stride, pVertexOffset); 
}
#endif /* VK_EXT_multi_draw */
#if defined(VK_EXT_opacity_micromap)
VKAPI_ATTR VkResult VKAPI_CALL vkBuildMicromapsEXT(
	VkDevice                                           device, 
	VkDeferredOperationKHR deferredOperation, 
	uint32_t infoCount, 
	const VkMicromapBuildInfoEXT* pInfos) 
{ 
  return pfn_vkBuildMicromapsEXT(device, deferredOperation, infoCount, pInfos); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdBuildMicromapsEXT(
	VkCommandBuffer                                    commandBuffer, 
	uint32_t infoCount, 
	const VkMicromapBuildInfoEXT* pInfos) 
{ 
  pfn_vkCmdBuildMicromapsEXT(commandBuffer, infoCount, pInfos); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyMemoryToMicromapEXT(
	VkCommandBuffer commandBuffer, 
	const VkCopyMemoryToMicromapInfoEXT* pInfo) 
{ 
  pfn_vkCmdCopyMemoryToMicromapEXT(commandBuffer, pInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyMicromapEXT(
	VkCommandBuffer commandBuffer, 
	const VkCopyMicromapInfoEXT* pInfo) 
{ 
  pfn_vkCmdCopyMicromapEXT(commandBuffer, pInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyMicromapToMemoryEXT(
	VkCommandBuffer commandBuffer, 
	const VkCopyMicromapToMemoryInfoEXT* pInfo) 
{ 
  pfn_vkCmdCopyMicromapToMemoryEXT(commandBuffer, pInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdWriteMicromapsPropertiesEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t micromapCount, 
	const VkMicromapEXT* pMicromaps, 
	VkQueryType queryType, 
	VkQueryPool queryPool, 
	uint32_t firstQuery) 
{ 
  pfn_vkCmdWriteMicromapsPropertiesEXT(commandBuffer, micromapCount, pMicromaps, queryType, queryPool, firstQuery); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyMemoryToMicromapEXT(
	VkDevice device, 
	VkDeferredOperationKHR deferredOperation, 
	const VkCopyMemoryToMicromapInfoEXT* pInfo) 
{ 
  return pfn_vkCopyMemoryToMicromapEXT(device, deferredOperation, pInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyMicromapEXT(
	VkDevice device, 
	VkDeferredOperationKHR deferredOperation, 
	const VkCopyMicromapInfoEXT* pInfo) 
{ 
  return pfn_vkCopyMicromapEXT(device, deferredOperation, pInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyMicromapToMemoryEXT(
	VkDevice device, 
	VkDeferredOperationKHR deferredOperation, 
	const VkCopyMicromapToMemoryInfoEXT* pInfo) 
{ 
  return pfn_vkCopyMicromapToMemoryEXT(device, deferredOperation, pInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateMicromapEXT(
	VkDevice                                           device, 
	const VkMicromapCreateInfoEXT*        pCreateInfo, 
	const VkAllocationCallbacks*       pAllocator, 
	VkMicromapEXT*                        pMicromap) 
{ 
  return pfn_vkCreateMicromapEXT(device, pCreateInfo, pAllocator, pMicromap); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyMicromapEXT(
	VkDevice device, 
	VkMicromapEXT micromap, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyMicromapEXT(device, micromap, pAllocator); 
}
VKAPI_ATTR void VKAPI_CALL vkGetDeviceMicromapCompatibilityEXT(
	VkDevice device, 
	const VkMicromapVersionInfoEXT* pVersionInfo, 
	VkAccelerationStructureCompatibilityKHR* pCompatibility) 
{ 
  pfn_vkGetDeviceMicromapCompatibilityEXT(device, pVersionInfo, pCompatibility); 
}
VKAPI_ATTR void VKAPI_CALL vkGetMicromapBuildSizesEXT(
	VkDevice                                            device, 
	VkAccelerationStructureBuildTypeKHR                 buildType, 
	const VkMicromapBuildInfoEXT*  pBuildInfo, 
	VkMicromapBuildSizesInfoEXT*           pSizeInfo) 
{ 
  pfn_vkGetMicromapBuildSizesEXT(device, buildType, pBuildInfo, pSizeInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkWriteMicromapsPropertiesEXT(
	VkDevice device, 
	uint32_t micromapCount, 
	const VkMicromapEXT* pMicromaps, 
	VkQueryType  queryType, 
	size_t       dataSize, 
	void* pData, 
	size_t stride) 
{ 
  return pfn_vkWriteMicromapsPropertiesEXT(device, micromapCount, pMicromaps, queryType, dataSize, pData, stride); 
}
#endif /* VK_EXT_opacity_micromap */
#if defined(VK_EXT_pageable_device_local_memory)
VKAPI_ATTR void VKAPI_CALL vkSetDeviceMemoryPriorityEXT(
	VkDevice       device, 
	VkDeviceMemory memory, 
	float          priority) 
{ 
  pfn_vkSetDeviceMemoryPriorityEXT(device, memory, priority); 
}
#endif /* VK_EXT_pageable_device_local_memory */
#if defined(VK_EXT_pipeline_properties)
VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelinePropertiesEXT(
	VkDevice device, 
	const VkPipelineInfoEXT* pPipelineInfo, 
	VkBaseOutStructure* pPipelineProperties) 
{ 
  return pfn_vkGetPipelinePropertiesEXT(device, pPipelineInfo, pPipelineProperties); 
}
#endif /* VK_EXT_pipeline_properties */
#if defined(VK_EXT_private_data)
VKAPI_ATTR VkResult VKAPI_CALL vkCreatePrivateDataSlotEXT(
	VkDevice device, 
	const VkPrivateDataSlotCreateInfo* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkPrivateDataSlot* pPrivateDataSlot) 
{ 
  return pfn_vkCreatePrivateDataSlotEXT(device, pCreateInfo, pAllocator, pPrivateDataSlot); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyPrivateDataSlotEXT(
	VkDevice device, 
	VkPrivateDataSlot privateDataSlot, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyPrivateDataSlotEXT(device, privateDataSlot, pAllocator); 
}
VKAPI_ATTR void VKAPI_CALL vkGetPrivateDataEXT(
	VkDevice device, 
	VkObjectType objectType, 
	uint64_t objectHandle, 
	VkPrivateDataSlot privateDataSlot, 
	uint64_t* pData) 
{ 
  pfn_vkGetPrivateDataEXT(device, objectType, objectHandle, privateDataSlot, pData); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkSetPrivateDataEXT(
	VkDevice device, 
	VkObjectType objectType, 
	uint64_t objectHandle, 
	VkPrivateDataSlot privateDataSlot, 
	uint64_t data) 
{ 
  return pfn_vkSetPrivateDataEXT(device, objectType, objectHandle, privateDataSlot, data); 
}
#endif /* VK_EXT_private_data */
#if defined(VK_EXT_sample_locations)
VKAPI_ATTR void VKAPI_CALL vkCmdSetSampleLocationsEXT(
	VkCommandBuffer commandBuffer, 
	const VkSampleLocationsInfoEXT* pSampleLocationsInfo) 
{ 
  pfn_vkCmdSetSampleLocationsEXT(commandBuffer, pSampleLocationsInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMultisamplePropertiesEXT(
	VkPhysicalDevice physicalDevice, 
	VkSampleCountFlagBits samples, 
	VkMultisamplePropertiesEXT* pMultisampleProperties) 
{ 
  pfn_vkGetPhysicalDeviceMultisamplePropertiesEXT(physicalDevice, samples, pMultisampleProperties); 
}
#endif /* VK_EXT_sample_locations */
#if defined(VK_EXT_shader_module_identifier)
VKAPI_ATTR void VKAPI_CALL vkGetShaderModuleCreateInfoIdentifierEXT(
	VkDevice device, 
	const VkShaderModuleCreateInfo* pCreateInfo, 
	VkShaderModuleIdentifierEXT* pIdentifier) 
{ 
  pfn_vkGetShaderModuleCreateInfoIdentifierEXT(device, pCreateInfo, pIdentifier); 
}
VKAPI_ATTR void VKAPI_CALL vkGetShaderModuleIdentifierEXT(
	VkDevice device, 
	VkShaderModule shaderModule, 
	VkShaderModuleIdentifierEXT* pIdentifier) 
{ 
  pfn_vkGetShaderModuleIdentifierEXT(device, shaderModule, pIdentifier); 
}
#endif /* VK_EXT_shader_module_identifier */
#if defined(VK_EXT_shader_object)
VKAPI_ATTR void VKAPI_CALL vkCmdBindShadersEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t stageCount, 
	const VkShaderStageFlagBits* pStages, 
	const VkShaderEXT* pShaders) 
{ 
  pfn_vkCmdBindShadersEXT(commandBuffer, stageCount, pStages, pShaders); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateShadersEXT(
	VkDevice device, 
	uint32_t createInfoCount, 
	const VkShaderCreateInfoEXT* pCreateInfos, 
	const VkAllocationCallbacks* pAllocator, 
	VkShaderEXT* pShaders) 
{ 
  return pfn_vkCreateShadersEXT(device, createInfoCount, pCreateInfos, pAllocator, pShaders); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyShaderEXT(
	VkDevice device, 
	VkShaderEXT shader, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyShaderEXT(device, shader, pAllocator); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetShaderBinaryDataEXT(
	VkDevice device, 
	VkShaderEXT shader, 
	size_t* pDataSize, 
	void* pData) 
{ 
  return pfn_vkGetShaderBinaryDataEXT(device, shader, pDataSize, pData); 
}
#endif /* VK_EXT_shader_object */
#if defined(VK_EXT_swapchain_maintenance1)
VKAPI_ATTR VkResult VKAPI_CALL vkReleaseSwapchainImagesEXT(
	VkDevice device, 
	const VkReleaseSwapchainImagesInfoEXT* pReleaseInfo) 
{ 
  return pfn_vkReleaseSwapchainImagesEXT(device, pReleaseInfo); 
}
#endif /* VK_EXT_swapchain_maintenance1 */
#if defined(VK_EXT_tooling_info)
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceToolPropertiesEXT(
	VkPhysicalDevice physicalDevice, 
	uint32_t* pToolCount, 
	VkPhysicalDeviceToolProperties* pToolProperties) 
{ 
  return pfn_vkGetPhysicalDeviceToolPropertiesEXT(physicalDevice, pToolCount, pToolProperties); 
}
#endif /* VK_EXT_tooling_info */
#if defined(VK_EXT_transform_feedback)
VKAPI_ATTR void VKAPI_CALL vkCmdBeginQueryIndexedEXT(
	VkCommandBuffer commandBuffer, 
	VkQueryPool queryPool, 
	uint32_t query, 
	VkQueryControlFlags flags, 
	uint32_t index) 
{ 
  pfn_vkCmdBeginQueryIndexedEXT(commandBuffer, queryPool, query, flags, index); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdBeginTransformFeedbackEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t firstCounterBuffer, 
	uint32_t counterBufferCount, 
	const VkBuffer* pCounterBuffers, 
	const VkDeviceSize* pCounterBufferOffsets) 
{ 
  pfn_vkCmdBeginTransformFeedbackEXT(commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdBindTransformFeedbackBuffersEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t firstBinding, 
	uint32_t bindingCount, 
	const VkBuffer* pBuffers, 
	const VkDeviceSize* pOffsets, 
	const VkDeviceSize* pSizes) 
{ 
  pfn_vkCmdBindTransformFeedbackBuffersEXT(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectByteCountEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t instanceCount, 
	uint32_t firstInstance, 
	VkBuffer counterBuffer, 
	VkDeviceSize counterBufferOffset, 
	uint32_t counterOffset, 
	uint32_t vertexStride) 
{ 
  pfn_vkCmdDrawIndirectByteCountEXT(commandBuffer, instanceCount, firstInstance, counterBuffer, counterBufferOffset, counterOffset, vertexStride); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdEndQueryIndexedEXT(
	VkCommandBuffer commandBuffer, 
	VkQueryPool queryPool, 
	uint32_t query, 
	uint32_t index) 
{ 
  pfn_vkCmdEndQueryIndexedEXT(commandBuffer, queryPool, query, index); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdEndTransformFeedbackEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t firstCounterBuffer, 
	uint32_t counterBufferCount, 
	const VkBuffer* pCounterBuffers, 
	const VkDeviceSize* pCounterBufferOffsets) 
{ 
  pfn_vkCmdEndTransformFeedbackEXT(commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets); 
}
#endif /* VK_EXT_transform_feedback */
#if defined(VK_EXT_validation_cache)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateValidationCacheEXT(
	VkDevice device, 
	const VkValidationCacheCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkValidationCacheEXT* pValidationCache) 
{ 
  return pfn_vkCreateValidationCacheEXT(device, pCreateInfo, pAllocator, pValidationCache); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyValidationCacheEXT(
	VkDevice device, 
	VkValidationCacheEXT validationCache, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyValidationCacheEXT(device, validationCache, pAllocator); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetValidationCacheDataEXT(
	VkDevice device, 
	VkValidationCacheEXT validationCache, 
	size_t* pDataSize, 
	void* pData) 
{ 
  return pfn_vkGetValidationCacheDataEXT(device, validationCache, pDataSize, pData); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkMergeValidationCachesEXT(
	VkDevice device, 
	VkValidationCacheEXT dstCache, 
	uint32_t srcCacheCount, 
	const VkValidationCacheEXT* pSrcCaches) 
{ 
  return pfn_vkMergeValidationCachesEXT(device, dstCache, srcCacheCount, pSrcCaches); 
}
#endif /* VK_EXT_validation_cache */
#if defined(VK_FUCHSIA_buffer_collection)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferCollectionFUCHSIA(
	VkDevice device, 
	const VkBufferCollectionCreateInfoFUCHSIA* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkBufferCollectionFUCHSIA* pCollection) 
{ 
  return pfn_vkCreateBufferCollectionFUCHSIA(device, pCreateInfo, pAllocator, pCollection); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyBufferCollectionFUCHSIA(
	VkDevice device, 
	VkBufferCollectionFUCHSIA collection, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyBufferCollectionFUCHSIA(device, collection, pAllocator); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetBufferCollectionPropertiesFUCHSIA(
	VkDevice device, 
	VkBufferCollectionFUCHSIA collection, 
	VkBufferCollectionPropertiesFUCHSIA* pProperties) 
{ 
  return pfn_vkGetBufferCollectionPropertiesFUCHSIA(device, collection, pProperties); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkSetBufferCollectionBufferConstraintsFUCHSIA(
	VkDevice device, 
	VkBufferCollectionFUCHSIA collection, 
	const VkBufferConstraintsInfoFUCHSIA* pBufferConstraintsInfo) 
{ 
  return pfn_vkSetBufferCollectionBufferConstraintsFUCHSIA(device, collection, pBufferConstraintsInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkSetBufferCollectionImageConstraintsFUCHSIA(
	VkDevice device, 
	VkBufferCollectionFUCHSIA collection, 
	const VkImageConstraintsInfoFUCHSIA* pImageConstraintsInfo) 
{ 
  return pfn_vkSetBufferCollectionImageConstraintsFUCHSIA(device, collection, pImageConstraintsInfo); 
}
#endif /* VK_FUCHSIA_buffer_collection */
#if defined(VK_FUCHSIA_external_memory)
VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryZirconHandleFUCHSIA(
	VkDevice device, 
	const VkMemoryGetZirconHandleInfoFUCHSIA* pGetZirconHandleInfo, 
	zx_handle_t* pZirconHandle) 
{ 
  return pfn_vkGetMemoryZirconHandleFUCHSIA(device, pGetZirconHandleInfo, pZirconHandle); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryZirconHandlePropertiesFUCHSIA(
	VkDevice device, 
	VkExternalMemoryHandleTypeFlagBits handleType, 
	zx_handle_t zirconHandle, 
	VkMemoryZirconHandlePropertiesFUCHSIA* pMemoryZirconHandleProperties) 
{ 
  return pfn_vkGetMemoryZirconHandlePropertiesFUCHSIA(device, handleType, zirconHandle, pMemoryZirconHandleProperties); 
}
#endif /* VK_FUCHSIA_external_memory */
#if defined(VK_FUCHSIA_external_semaphore)
VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreZirconHandleFUCHSIA(
	VkDevice device, 
	const VkSemaphoreGetZirconHandleInfoFUCHSIA* pGetZirconHandleInfo, 
	zx_handle_t* pZirconHandle) 
{ 
  return pfn_vkGetSemaphoreZirconHandleFUCHSIA(device, pGetZirconHandleInfo, pZirconHandle); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkImportSemaphoreZirconHandleFUCHSIA(
	VkDevice device, 
	const VkImportSemaphoreZirconHandleInfoFUCHSIA* pImportSemaphoreZirconHandleInfo) 
{ 
  return pfn_vkImportSemaphoreZirconHandleFUCHSIA(device, pImportSemaphoreZirconHandleInfo); 
}
#endif /* VK_FUCHSIA_external_semaphore */
#if defined(VK_FUCHSIA_imagepipe_surface)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateImagePipeSurfaceFUCHSIA(
	VkInstance instance, 
	const VkImagePipeSurfaceCreateInfoFUCHSIA* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkSurfaceKHR* pSurface) 
{ 
  return pfn_vkCreateImagePipeSurfaceFUCHSIA(instance, pCreateInfo, pAllocator, pSurface); 
}
#endif /* VK_FUCHSIA_imagepipe_surface */
#if defined(VK_GGP_stream_descriptor_surface)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateStreamDescriptorSurfaceGGP(
	VkInstance instance, 
	const VkStreamDescriptorSurfaceCreateInfoGGP* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkSurfaceKHR* pSurface) 
{ 
  return pfn_vkCreateStreamDescriptorSurfaceGGP(instance, pCreateInfo, pAllocator, pSurface); 
}
#endif /* VK_GGP_stream_descriptor_surface */
#if defined(VK_GOOGLE_display_timing)
VKAPI_ATTR VkResult VKAPI_CALL vkGetPastPresentationTimingGOOGLE(
	VkDevice device, 
	VkSwapchainKHR swapchain, 
	uint32_t* pPresentationTimingCount, 
	VkPastPresentationTimingGOOGLE* pPresentationTimings) 
{ 
  return pfn_vkGetPastPresentationTimingGOOGLE(device, swapchain, pPresentationTimingCount, pPresentationTimings); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetRefreshCycleDurationGOOGLE(
	VkDevice device, 
	VkSwapchainKHR swapchain, 
	VkRefreshCycleDurationGOOGLE* pDisplayTimingProperties) 
{ 
  return pfn_vkGetRefreshCycleDurationGOOGLE(device, swapchain, pDisplayTimingProperties); 
}
#endif /* VK_GOOGLE_display_timing */
#if defined(VK_HUAWEI_cluster_culling_shader)
VKAPI_ATTR void VKAPI_CALL vkCmdDrawClusterHUAWEI(
	VkCommandBuffer commandBuffer, 
	uint32_t groupCountX, 
	uint32_t groupCountY, 
	uint32_t groupCountZ) 
{ 
  pfn_vkCmdDrawClusterHUAWEI(commandBuffer, groupCountX, groupCountY, groupCountZ); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawClusterIndirectHUAWEI(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset) 
{ 
  pfn_vkCmdDrawClusterIndirectHUAWEI(commandBuffer, buffer, offset); 
}
#endif /* VK_HUAWEI_cluster_culling_shader */
#if defined(VK_HUAWEI_invocation_mask)
VKAPI_ATTR void VKAPI_CALL vkCmdBindInvocationMaskHUAWEI(
	VkCommandBuffer commandBuffer, 
	VkImageView imageView, 
	VkImageLayout imageLayout) 
{ 
  pfn_vkCmdBindInvocationMaskHUAWEI(commandBuffer, imageView, imageLayout); 
}
#endif /* VK_HUAWEI_invocation_mask */
#if defined(VK_HUAWEI_subpass_shading)
VKAPI_ATTR void VKAPI_CALL vkCmdSubpassShadingHUAWEI(
	VkCommandBuffer commandBuffer) 
{ 
  pfn_vkCmdSubpassShadingHUAWEI(commandBuffer); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(
	VkDevice device, 
	VkRenderPass renderpass, 
	VkExtent2D* pMaxWorkgroupSize) 
{ 
  return pfn_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(device, renderpass, pMaxWorkgroupSize); 
}
#endif /* VK_HUAWEI_subpass_shading */
#if defined(VK_INTEL_performance_query)
VKAPI_ATTR VkResult VKAPI_CALL vkAcquirePerformanceConfigurationINTEL(
	VkDevice device, 
	const VkPerformanceConfigurationAcquireInfoINTEL* pAcquireInfo, 
	VkPerformanceConfigurationINTEL* pConfiguration) 
{ 
  return pfn_vkAcquirePerformanceConfigurationINTEL(device, pAcquireInfo, pConfiguration); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCmdSetPerformanceMarkerINTEL(
	VkCommandBuffer commandBuffer, 
	const VkPerformanceMarkerInfoINTEL* pMarkerInfo) 
{ 
  return pfn_vkCmdSetPerformanceMarkerINTEL(commandBuffer, pMarkerInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCmdSetPerformanceOverrideINTEL(
	VkCommandBuffer commandBuffer, 
	const VkPerformanceOverrideInfoINTEL* pOverrideInfo) 
{ 
  return pfn_vkCmdSetPerformanceOverrideINTEL(commandBuffer, pOverrideInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCmdSetPerformanceStreamMarkerINTEL(
	VkCommandBuffer commandBuffer, 
	const VkPerformanceStreamMarkerInfoINTEL* pMarkerInfo) 
{ 
  return pfn_vkCmdSetPerformanceStreamMarkerINTEL(commandBuffer, pMarkerInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPerformanceParameterINTEL(
	VkDevice device, 
	VkPerformanceParameterTypeINTEL parameter, 
	VkPerformanceValueINTEL* pValue) 
{ 
  return pfn_vkGetPerformanceParameterINTEL(device, parameter, pValue); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkInitializePerformanceApiINTEL(
	VkDevice device, 
	const VkInitializePerformanceApiInfoINTEL* pInitializeInfo) 
{ 
  return pfn_vkInitializePerformanceApiINTEL(device, pInitializeInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkQueueSetPerformanceConfigurationINTEL(
	VkQueue queue, 
	VkPerformanceConfigurationINTEL configuration) 
{ 
  return pfn_vkQueueSetPerformanceConfigurationINTEL(queue, configuration); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkReleasePerformanceConfigurationINTEL(
	VkDevice device, 
	VkPerformanceConfigurationINTEL configuration) 
{ 
  return pfn_vkReleasePerformanceConfigurationINTEL(device, configuration); 
}
VKAPI_ATTR void VKAPI_CALL vkUninitializePerformanceApiINTEL(
	VkDevice device) 
{ 
  pfn_vkUninitializePerformanceApiINTEL(device); 
}
#endif /* VK_INTEL_performance_query */
#if defined(VK_KHR_acceleration_structure)
VKAPI_ATTR VkResult VKAPI_CALL vkBuildAccelerationStructuresKHR(
	VkDevice                                           device, 
	VkDeferredOperationKHR deferredOperation, 
	uint32_t infoCount, 
	const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, 
	const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) 
{ 
  return pfn_vkBuildAccelerationStructuresKHR(device, deferredOperation, infoCount, pInfos, ppBuildRangeInfos); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresIndirectKHR(
	VkCommandBuffer                  commandBuffer, 
	uint32_t                                           infoCount, 
	const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, 
	const VkDeviceAddress*             pIndirectDeviceAddresses, 
	const uint32_t*                    pIndirectStrides, 
	const uint32_t* const*             ppMaxPrimitiveCounts) 
{ 
  pfn_vkCmdBuildAccelerationStructuresIndirectKHR(commandBuffer, infoCount, pInfos, pIndirectDeviceAddresses, pIndirectStrides, ppMaxPrimitiveCounts); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresKHR(
	VkCommandBuffer                                    commandBuffer, 
	uint32_t infoCount, 
	const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, 
	const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) 
{ 
  pfn_vkCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureKHR(
	VkCommandBuffer commandBuffer, 
	const VkCopyAccelerationStructureInfoKHR* pInfo) 
{ 
  pfn_vkCmdCopyAccelerationStructureKHR(commandBuffer, pInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureToMemoryKHR(
	VkCommandBuffer commandBuffer, 
	const VkCopyAccelerationStructureToMemoryInfoKHR* pInfo) 
{ 
  pfn_vkCmdCopyAccelerationStructureToMemoryKHR(commandBuffer, pInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyMemoryToAccelerationStructureKHR(
	VkCommandBuffer commandBuffer, 
	const VkCopyMemoryToAccelerationStructureInfoKHR* pInfo) 
{ 
  pfn_vkCmdCopyMemoryToAccelerationStructureKHR(commandBuffer, pInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesKHR(
	VkCommandBuffer commandBuffer, 
	uint32_t accelerationStructureCount, 
	const VkAccelerationStructureKHR* pAccelerationStructures, 
	VkQueryType queryType, 
	VkQueryPool queryPool, 
	uint32_t firstQuery) 
{ 
  pfn_vkCmdWriteAccelerationStructuresPropertiesKHR(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyAccelerationStructureKHR(
	VkDevice device, 
	VkDeferredOperationKHR deferredOperation, 
	const VkCopyAccelerationStructureInfoKHR* pInfo) 
{ 
  return pfn_vkCopyAccelerationStructureKHR(device, deferredOperation, pInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyAccelerationStructureToMemoryKHR(
	VkDevice device, 
	VkDeferredOperationKHR deferredOperation, 
	const VkCopyAccelerationStructureToMemoryInfoKHR* pInfo) 
{ 
  return pfn_vkCopyAccelerationStructureToMemoryKHR(device, deferredOperation, pInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyMemoryToAccelerationStructureKHR(
	VkDevice device, 
	VkDeferredOperationKHR deferredOperation, 
	const VkCopyMemoryToAccelerationStructureInfoKHR* pInfo) 
{ 
  return pfn_vkCopyMemoryToAccelerationStructureKHR(device, deferredOperation, pInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR(
	VkDevice                                           device, 
	const VkAccelerationStructureCreateInfoKHR*        pCreateInfo, 
	const VkAllocationCallbacks*       pAllocator, 
	VkAccelerationStructureKHR*                        pAccelerationStructure) 
{ 
  return pfn_vkCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR(
	VkDevice device, 
	VkAccelerationStructureKHR accelerationStructure, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator); 
}
VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureBuildSizesKHR(
	VkDevice                                            device, 
	VkAccelerationStructureBuildTypeKHR                 buildType, 
	const VkAccelerationStructureBuildGeometryInfoKHR*  pBuildInfo, 
	const uint32_t*  pMaxPrimitiveCounts, 
	VkAccelerationStructureBuildSizesInfoKHR*           pSizeInfo) 
{ 
  pfn_vkGetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo); 
}
VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetAccelerationStructureDeviceAddressKHR(
	VkDevice device, 
	const VkAccelerationStructureDeviceAddressInfoKHR* pInfo) 
{ 
  return pfn_vkGetAccelerationStructureDeviceAddressKHR(device, pInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkGetDeviceAccelerationStructureCompatibilityKHR(
	VkDevice device, 
	const VkAccelerationStructureVersionInfoKHR* pVersionInfo, 
	VkAccelerationStructureCompatibilityKHR* pCompatibility) 
{ 
  pfn_vkGetDeviceAccelerationStructureCompatibilityKHR(device, pVersionInfo, pCompatibility); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkWriteAccelerationStructuresPropertiesKHR(
	VkDevice device, 
	uint32_t accelerationStructureCount, 
	const VkAccelerationStructureKHR* pAccelerationStructures, 
	VkQueryType  queryType, 
	size_t       dataSize, 
	void* pData, 
	size_t stride) 
{ 
  return pfn_vkWriteAccelerationStructuresPropertiesKHR(device, accelerationStructureCount, pAccelerationStructures, queryType, dataSize, pData, stride); 
}
#endif /* VK_KHR_acceleration_structure */
#if defined(VK_KHR_android_surface)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateAndroidSurfaceKHR(
	VkInstance instance, 
	const VkAndroidSurfaceCreateInfoKHR* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkSurfaceKHR* pSurface) 
{ 
  return pfn_vkCreateAndroidSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface); 
}
#endif /* VK_KHR_android_surface */
#if defined(VK_KHR_bind_memory2)
VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory2KHR(
	VkDevice device, 
	uint32_t bindInfoCount, 
	const VkBindBufferMemoryInfo* pBindInfos) 
{ 
  return pfn_vkBindBufferMemory2KHR(device, bindInfoCount, pBindInfos); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory2KHR(
	VkDevice device, 
	uint32_t bindInfoCount, 
	const VkBindImageMemoryInfo* pBindInfos) 
{ 
  return pfn_vkBindImageMemory2KHR(device, bindInfoCount, pBindInfos); 
}
#endif /* VK_KHR_bind_memory2 */
#if defined(VK_KHR_buffer_device_address)
VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetBufferDeviceAddressKHR(
	VkDevice device, 
	const VkBufferDeviceAddressInfo* pInfo) 
{ 
  return pfn_vkGetBufferDeviceAddressKHR(device, pInfo); 
}
VKAPI_ATTR uint64_t VKAPI_CALL vkGetBufferOpaqueCaptureAddressKHR(
	VkDevice device, 
	const VkBufferDeviceAddressInfo* pInfo) 
{ 
  return pfn_vkGetBufferOpaqueCaptureAddressKHR(device, pInfo); 
}
VKAPI_ATTR uint64_t VKAPI_CALL vkGetDeviceMemoryOpaqueCaptureAddressKHR(
	VkDevice device, 
	const VkDeviceMemoryOpaqueCaptureAddressInfo* pInfo) 
{ 
  return pfn_vkGetDeviceMemoryOpaqueCaptureAddressKHR(device, pInfo); 
}
#endif /* VK_KHR_buffer_device_address */
#if defined(VK_KHR_cooperative_matrix)
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(
	VkPhysicalDevice physicalDevice, 
	uint32_t* pPropertyCount, 
	VkCooperativeMatrixPropertiesKHR* pProperties) 
{ 
  return pfn_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(physicalDevice, pPropertyCount, pProperties); 
}
#endif /* VK_KHR_cooperative_matrix */
#if defined(VK_KHR_copy_commands2)
VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage2KHR(
	VkCommandBuffer commandBuffer, 
	const VkBlitImageInfo2* pBlitImageInfo) 
{ 
  pfn_vkCmdBlitImage2KHR(commandBuffer, pBlitImageInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer2KHR(
	VkCommandBuffer commandBuffer, 
	const VkCopyBufferInfo2* pCopyBufferInfo) 
{ 
  pfn_vkCmdCopyBuffer2KHR(commandBuffer, pCopyBufferInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage2KHR(
	VkCommandBuffer commandBuffer, 
	const VkCopyBufferToImageInfo2* pCopyBufferToImageInfo) 
{ 
  pfn_vkCmdCopyBufferToImage2KHR(commandBuffer, pCopyBufferToImageInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage2KHR(
	VkCommandBuffer commandBuffer, 
	const VkCopyImageInfo2* pCopyImageInfo) 
{ 
  pfn_vkCmdCopyImage2KHR(commandBuffer, pCopyImageInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer2KHR(
	VkCommandBuffer commandBuffer, 
	const VkCopyImageToBufferInfo2* pCopyImageToBufferInfo) 
{ 
  pfn_vkCmdCopyImageToBuffer2KHR(commandBuffer, pCopyImageToBufferInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage2KHR(
	VkCommandBuffer commandBuffer, 
	const VkResolveImageInfo2* pResolveImageInfo) 
{ 
  pfn_vkCmdResolveImage2KHR(commandBuffer, pResolveImageInfo); 
}
#endif /* VK_KHR_copy_commands2 */
#if defined(VK_KHR_create_renderpass2)
VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass2KHR(
	VkCommandBuffer commandBuffer, 
	const VkRenderPassBeginInfo*      pRenderPassBegin, 
	const VkSubpassBeginInfo*      pSubpassBeginInfo) 
{ 
  pfn_vkCmdBeginRenderPass2KHR(commandBuffer, pRenderPassBegin, pSubpassBeginInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass2KHR(
	VkCommandBuffer commandBuffer, 
	const VkSubpassEndInfo*        pSubpassEndInfo) 
{ 
  pfn_vkCmdEndRenderPass2KHR(commandBuffer, pSubpassEndInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass2KHR(
	VkCommandBuffer commandBuffer, 
	const VkSubpassBeginInfo*      pSubpassBeginInfo, 
	const VkSubpassEndInfo*        pSubpassEndInfo) 
{ 
  pfn_vkCmdNextSubpass2KHR(commandBuffer, pSubpassBeginInfo, pSubpassEndInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass2KHR(
	VkDevice device, 
	const VkRenderPassCreateInfo2* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkRenderPass* pRenderPass) 
{ 
  return pfn_vkCreateRenderPass2KHR(device, pCreateInfo, pAllocator, pRenderPass); 
}
#endif /* VK_KHR_create_renderpass2 */
#if defined(VK_KHR_deferred_host_operations)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDeferredOperationKHR(
	VkDevice device, 
	const VkAllocationCallbacks* pAllocator, 
	VkDeferredOperationKHR* pDeferredOperation) 
{ 
  return pfn_vkCreateDeferredOperationKHR(device, pAllocator, pDeferredOperation); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkDeferredOperationJoinKHR(
	VkDevice device, 
	VkDeferredOperationKHR operation) 
{ 
  return pfn_vkDeferredOperationJoinKHR(device, operation); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyDeferredOperationKHR(
	VkDevice device, 
	VkDeferredOperationKHR operation, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyDeferredOperationKHR(device, operation, pAllocator); 
}
VKAPI_ATTR uint32_t VKAPI_CALL vkGetDeferredOperationMaxConcurrencyKHR(
	VkDevice device, 
	VkDeferredOperationKHR operation) 
{ 
  return pfn_vkGetDeferredOperationMaxConcurrencyKHR(device, operation); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetDeferredOperationResultKHR(
	VkDevice device, 
	VkDeferredOperationKHR operation) 
{ 
  return pfn_vkGetDeferredOperationResultKHR(device, operation); 
}
#endif /* VK_KHR_deferred_host_operations */
#if defined(VK_KHR_descriptor_update_template)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorUpdateTemplateKHR(
	VkDevice device, 
	const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) 
{ 
  return pfn_vkCreateDescriptorUpdateTemplateKHR(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorUpdateTemplateKHR(
	VkDevice device, 
	VkDescriptorUpdateTemplate descriptorUpdateTemplate, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyDescriptorUpdateTemplateKHR(device, descriptorUpdateTemplate, pAllocator); 
}
VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSetWithTemplateKHR(
	VkDevice device, 
	VkDescriptorSet descriptorSet, 
	VkDescriptorUpdateTemplate descriptorUpdateTemplate, 
	const void* pData) 
{ 
  pfn_vkUpdateDescriptorSetWithTemplateKHR(device, descriptorSet, descriptorUpdateTemplate, pData); 
}
#endif /* VK_KHR_descriptor_update_template */
#if defined(VK_KHR_device_group)
VKAPI_ATTR void VKAPI_CALL vkCmdDispatchBaseKHR(
	VkCommandBuffer commandBuffer, 
	uint32_t baseGroupX, 
	uint32_t baseGroupY, 
	uint32_t baseGroupZ, 
	uint32_t groupCountX, 
	uint32_t groupCountY, 
	uint32_t groupCountZ) 
{ 
  pfn_vkCmdDispatchBaseKHR(commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDeviceMaskKHR(
	VkCommandBuffer commandBuffer, 
	uint32_t deviceMask) 
{ 
  pfn_vkCmdSetDeviceMaskKHR(commandBuffer, deviceMask); 
}
VKAPI_ATTR void VKAPI_CALL vkGetDeviceGroupPeerMemoryFeaturesKHR(
	VkDevice device, 
	uint32_t heapIndex, 
	uint32_t localDeviceIndex, 
	uint32_t remoteDeviceIndex, 
	VkPeerMemoryFeatureFlags* pPeerMemoryFeatures) 
{ 
  pfn_vkGetDeviceGroupPeerMemoryFeaturesKHR(device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures); 
}
#endif /* VK_KHR_device_group */
#if defined(VK_KHR_device_group_creation)
VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceGroupsKHR(
	VkInstance instance, 
	uint32_t* pPhysicalDeviceGroupCount, 
	VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) 
{ 
  return pfn_vkEnumeratePhysicalDeviceGroupsKHR(instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties); 
}
#endif /* VK_KHR_device_group_creation */
#if defined(VK_KHR_draw_indirect_count)
VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirectCountKHR(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset, 
	VkBuffer countBuffer, 
	VkDeviceSize countBufferOffset, 
	uint32_t maxDrawCount, 
	uint32_t stride) 
{ 
  pfn_vkCmdDrawIndexedIndirectCountKHR(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectCountKHR(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset, 
	VkBuffer countBuffer, 
	VkDeviceSize countBufferOffset, 
	uint32_t maxDrawCount, 
	uint32_t stride) 
{ 
  pfn_vkCmdDrawIndirectCountKHR(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride); 
}
#endif /* VK_KHR_draw_indirect_count */
#if defined(VK_KHR_dynamic_rendering)
VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderingKHR(
	VkCommandBuffer                   commandBuffer, 
	const VkRenderingInfo*                              pRenderingInfo) 
{ 
  pfn_vkCmdBeginRenderingKHR(commandBuffer, pRenderingInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderingKHR(
	VkCommandBuffer                   commandBuffer) 
{ 
  pfn_vkCmdEndRenderingKHR(commandBuffer); 
}
#endif /* VK_KHR_dynamic_rendering */
#if defined(VK_KHR_external_fence_capabilities)
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalFencePropertiesKHR(
	VkPhysicalDevice physicalDevice, 
	const VkPhysicalDeviceExternalFenceInfo* pExternalFenceInfo, 
	VkExternalFenceProperties* pExternalFenceProperties) 
{ 
  pfn_vkGetPhysicalDeviceExternalFencePropertiesKHR(physicalDevice, pExternalFenceInfo, pExternalFenceProperties); 
}
#endif /* VK_KHR_external_fence_capabilities */
#if defined(VK_KHR_external_fence_fd)
VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceFdKHR(
	VkDevice device, 
	const VkFenceGetFdInfoKHR* pGetFdInfo, 
	int* pFd) 
{ 
  return pfn_vkGetFenceFdKHR(device, pGetFdInfo, pFd); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkImportFenceFdKHR(
	VkDevice device, 
	const VkImportFenceFdInfoKHR* pImportFenceFdInfo) 
{ 
  return pfn_vkImportFenceFdKHR(device, pImportFenceFdInfo); 
}
#endif /* VK_KHR_external_fence_fd */
#if defined(VK_KHR_external_fence_win32)
VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceWin32HandleKHR(
	VkDevice device, 
	const VkFenceGetWin32HandleInfoKHR* pGetWin32HandleInfo, 
	HANDLE* pHandle) 
{ 
  return pfn_vkGetFenceWin32HandleKHR(device, pGetWin32HandleInfo, pHandle); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkImportFenceWin32HandleKHR(
	VkDevice device, 
	const VkImportFenceWin32HandleInfoKHR* pImportFenceWin32HandleInfo) 
{ 
  return pfn_vkImportFenceWin32HandleKHR(device, pImportFenceWin32HandleInfo); 
}
#endif /* VK_KHR_external_fence_win32 */
#if defined(VK_KHR_external_memory_capabilities)
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalBufferPropertiesKHR(
	VkPhysicalDevice physicalDevice, 
	const VkPhysicalDeviceExternalBufferInfo* pExternalBufferInfo, 
	VkExternalBufferProperties* pExternalBufferProperties) 
{ 
  pfn_vkGetPhysicalDeviceExternalBufferPropertiesKHR(physicalDevice, pExternalBufferInfo, pExternalBufferProperties); 
}
#endif /* VK_KHR_external_memory_capabilities */
#if defined(VK_KHR_external_memory_fd)
VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryFdKHR(
	VkDevice device, 
	const VkMemoryGetFdInfoKHR* pGetFdInfo, 
	int* pFd) 
{ 
  return pfn_vkGetMemoryFdKHR(device, pGetFdInfo, pFd); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryFdPropertiesKHR(
	VkDevice device, 
	VkExternalMemoryHandleTypeFlagBits handleType, 
	int fd, 
	VkMemoryFdPropertiesKHR* pMemoryFdProperties) 
{ 
  return pfn_vkGetMemoryFdPropertiesKHR(device, handleType, fd, pMemoryFdProperties); 
}
#endif /* VK_KHR_external_memory_fd */
#if defined(VK_KHR_external_memory_win32)
VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandleKHR(
	VkDevice device, 
	const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, 
	HANDLE* pHandle) 
{ 
  return pfn_vkGetMemoryWin32HandleKHR(device, pGetWin32HandleInfo, pHandle); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandlePropertiesKHR(
	VkDevice device, 
	VkExternalMemoryHandleTypeFlagBits handleType, 
	HANDLE handle, 
	VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties) 
{ 
  return pfn_vkGetMemoryWin32HandlePropertiesKHR(device, handleType, handle, pMemoryWin32HandleProperties); 
}
#endif /* VK_KHR_external_memory_win32 */
#if defined(VK_KHR_external_semaphore_capabilities)
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(
	VkPhysicalDevice physicalDevice, 
	const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo, 
	VkExternalSemaphoreProperties* pExternalSemaphoreProperties) 
{ 
  pfn_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties); 
}
#endif /* VK_KHR_external_semaphore_capabilities */
#if defined(VK_KHR_external_semaphore_fd)
VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreFdKHR(
	VkDevice device, 
	const VkSemaphoreGetFdInfoKHR* pGetFdInfo, 
	int* pFd) 
{ 
  return pfn_vkGetSemaphoreFdKHR(device, pGetFdInfo, pFd); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkImportSemaphoreFdKHR(
	VkDevice device, 
	const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) 
{ 
  return pfn_vkImportSemaphoreFdKHR(device, pImportSemaphoreFdInfo); 
}
#endif /* VK_KHR_external_semaphore_fd */
#if defined(VK_KHR_external_semaphore_win32)
VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreWin32HandleKHR(
	VkDevice device, 
	const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo, 
	HANDLE* pHandle) 
{ 
  return pfn_vkGetSemaphoreWin32HandleKHR(device, pGetWin32HandleInfo, pHandle); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkImportSemaphoreWin32HandleKHR(
	VkDevice device, 
	const VkImportSemaphoreWin32HandleInfoKHR* pImportSemaphoreWin32HandleInfo) 
{ 
  return pfn_vkImportSemaphoreWin32HandleKHR(device, pImportSemaphoreWin32HandleInfo); 
}
#endif /* VK_KHR_external_semaphore_win32 */
#if defined(VK_KHR_fragment_shading_rate)
VKAPI_ATTR void VKAPI_CALL vkCmdSetFragmentShadingRateKHR(
	VkCommandBuffer           commandBuffer, 
	const VkExtent2D*                           pFragmentSize, 
	const VkFragmentShadingRateCombinerOpKHR    combinerOps[2]) 
{ 
  pfn_vkCmdSetFragmentShadingRateKHR(commandBuffer, pFragmentSize, combinerOps); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceFragmentShadingRatesKHR(
	VkPhysicalDevice physicalDevice, 
	uint32_t* pFragmentShadingRateCount, 
	VkPhysicalDeviceFragmentShadingRateKHR* pFragmentShadingRates) 
{ 
  return pfn_vkGetPhysicalDeviceFragmentShadingRatesKHR(physicalDevice, pFragmentShadingRateCount, pFragmentShadingRates); 
}
#endif /* VK_KHR_fragment_shading_rate */
#if defined(VK_KHR_get_memory_requirements2)
VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements2KHR(
	VkDevice device, 
	const VkBufferMemoryRequirementsInfo2* pInfo, 
	VkMemoryRequirements2* pMemoryRequirements) 
{ 
  pfn_vkGetBufferMemoryRequirements2KHR(device, pInfo, pMemoryRequirements); 
}
VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements2KHR(
	VkDevice device, 
	const VkImageMemoryRequirementsInfo2* pInfo, 
	VkMemoryRequirements2* pMemoryRequirements) 
{ 
  pfn_vkGetImageMemoryRequirements2KHR(device, pInfo, pMemoryRequirements); 
}
VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements2KHR(
	VkDevice device, 
	const VkImageSparseMemoryRequirementsInfo2* pInfo, 
	uint32_t* pSparseMemoryRequirementCount, 
	VkSparseImageMemoryRequirements2* pSparseMemoryRequirements) 
{ 
  pfn_vkGetImageSparseMemoryRequirements2KHR(device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements); 
}
#endif /* VK_KHR_get_memory_requirements2 */
#if defined(VK_KHR_get_physical_device_properties2)
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2KHR(
	VkPhysicalDevice physicalDevice, 
	VkPhysicalDeviceFeatures2* pFeatures) 
{ 
  pfn_vkGetPhysicalDeviceFeatures2KHR(physicalDevice, pFeatures); 
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2KHR(
	VkPhysicalDevice physicalDevice, 
	VkFormat format, 
	VkFormatProperties2* pFormatProperties) 
{ 
  pfn_vkGetPhysicalDeviceFormatProperties2KHR(physicalDevice, format, pFormatProperties); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2KHR(
	VkPhysicalDevice physicalDevice, 
	const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo, 
	VkImageFormatProperties2* pImageFormatProperties) 
{ 
  return pfn_vkGetPhysicalDeviceImageFormatProperties2KHR(physicalDevice, pImageFormatInfo, pImageFormatProperties); 
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties2KHR(
	VkPhysicalDevice physicalDevice, 
	VkPhysicalDeviceMemoryProperties2* pMemoryProperties) 
{ 
  pfn_vkGetPhysicalDeviceMemoryProperties2KHR(physicalDevice, pMemoryProperties); 
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2KHR(
	VkPhysicalDevice physicalDevice, 
	VkPhysicalDeviceProperties2* pProperties) 
{ 
  pfn_vkGetPhysicalDeviceProperties2KHR(physicalDevice, pProperties); 
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2KHR(
	VkPhysicalDevice physicalDevice, 
	uint32_t* pQueueFamilyPropertyCount, 
	VkQueueFamilyProperties2* pQueueFamilyProperties) 
{ 
  pfn_vkGetPhysicalDeviceQueueFamilyProperties2KHR(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties); 
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties2KHR(
	VkPhysicalDevice physicalDevice, 
	const VkPhysicalDeviceSparseImageFormatInfo2* pFormatInfo, 
	uint32_t* pPropertyCount, 
	VkSparseImageFormatProperties2* pProperties) 
{ 
  pfn_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(physicalDevice, pFormatInfo, pPropertyCount, pProperties); 
}
#endif /* VK_KHR_get_physical_device_properties2 */
#if defined(VK_KHR_maintenance1)
VKAPI_ATTR void VKAPI_CALL vkTrimCommandPoolKHR(
	VkDevice device, 
	VkCommandPool commandPool, 
	VkCommandPoolTrimFlags flags) 
{ 
  pfn_vkTrimCommandPoolKHR(device, commandPool, flags); 
}
#endif /* VK_KHR_maintenance1 */
#if defined(VK_KHR_maintenance3)
VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutSupportKHR(
	VkDevice device, 
	const VkDescriptorSetLayoutCreateInfo* pCreateInfo, 
	VkDescriptorSetLayoutSupport* pSupport) 
{ 
  pfn_vkGetDescriptorSetLayoutSupportKHR(device, pCreateInfo, pSupport); 
}
#endif /* VK_KHR_maintenance3 */
#if defined(VK_KHR_maintenance4)
VKAPI_ATTR void VKAPI_CALL vkGetDeviceBufferMemoryRequirementsKHR(
	VkDevice device, 
	const VkDeviceBufferMemoryRequirements* pInfo, 
	VkMemoryRequirements2* pMemoryRequirements) 
{ 
  pfn_vkGetDeviceBufferMemoryRequirementsKHR(device, pInfo, pMemoryRequirements); 
}
VKAPI_ATTR void VKAPI_CALL vkGetDeviceImageMemoryRequirementsKHR(
	VkDevice device, 
	const VkDeviceImageMemoryRequirements* pInfo, 
	VkMemoryRequirements2* pMemoryRequirements) 
{ 
  pfn_vkGetDeviceImageMemoryRequirementsKHR(device, pInfo, pMemoryRequirements); 
}
VKAPI_ATTR void VKAPI_CALL vkGetDeviceImageSparseMemoryRequirementsKHR(
	VkDevice device, 
	const VkDeviceImageMemoryRequirements* pInfo, 
	uint32_t* pSparseMemoryRequirementCount, 
	VkSparseImageMemoryRequirements2* pSparseMemoryRequirements) 
{ 
  pfn_vkGetDeviceImageSparseMemoryRequirementsKHR(device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements); 
}
#endif /* VK_KHR_maintenance4 */
#if defined(VK_KHR_maintenance5)
VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer2KHR(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset, 
	VkDeviceSize size, 
	VkIndexType indexType) 
{ 
  pfn_vkCmdBindIndexBuffer2KHR(commandBuffer, buffer, offset, size, indexType); 
}
VKAPI_ATTR void VKAPI_CALL vkGetDeviceImageSubresourceLayoutKHR(
	VkDevice device, 
	const VkDeviceImageSubresourceInfoKHR* pInfo, 
	VkSubresourceLayout2KHR* pLayout) 
{ 
  pfn_vkGetDeviceImageSubresourceLayoutKHR(device, pInfo, pLayout); 
}
VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout2KHR(
	VkDevice device, 
	VkImage image, 
	const VkImageSubresource2KHR* pSubresource, 
	VkSubresourceLayout2KHR* pLayout) 
{ 
  pfn_vkGetImageSubresourceLayout2KHR(device, image, pSubresource, pLayout); 
}
VKAPI_ATTR void VKAPI_CALL vkGetRenderingAreaGranularityKHR(
	VkDevice device, 
	const VkRenderingAreaInfoKHR* pRenderingAreaInfo, 
	VkExtent2D* pGranularity) 
{ 
  pfn_vkGetRenderingAreaGranularityKHR(device, pRenderingAreaInfo, pGranularity); 
}
#endif /* VK_KHR_maintenance5 */
#if defined(VK_KHR_map_memory2)
VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory2KHR(
	VkDevice device, 
	const VkMemoryMapInfoKHR* pMemoryMapInfo, 
	void** ppData) 
{ 
  return pfn_vkMapMemory2KHR(device, pMemoryMapInfo, ppData); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkUnmapMemory2KHR(
	VkDevice device, 
	const VkMemoryUnmapInfoKHR* pMemoryUnmapInfo) 
{ 
  return pfn_vkUnmapMemory2KHR(device, pMemoryUnmapInfo); 
}
#endif /* VK_KHR_map_memory2 */
#if defined(VK_KHR_performance_query)
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireProfilingLockKHR(
	VkDevice device, 
	const VkAcquireProfilingLockInfoKHR* pInfo) 
{ 
  return pfn_vkAcquireProfilingLockKHR(device, pInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(
	VkPhysicalDevice physicalDevice, 
	uint32_t queueFamilyIndex, 
	uint32_t* pCounterCount, 
	VkPerformanceCounterKHR* pCounters, 
	VkPerformanceCounterDescriptionKHR* pCounterDescriptions) 
{ 
  return pfn_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(physicalDevice, queueFamilyIndex, pCounterCount, pCounters, pCounterDescriptions); 
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(
	VkPhysicalDevice physicalDevice, 
	const VkQueryPoolPerformanceCreateInfoKHR* pPerformanceQueryCreateInfo, 
	uint32_t* pNumPasses) 
{ 
  pfn_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(physicalDevice, pPerformanceQueryCreateInfo, pNumPasses); 
}
VKAPI_ATTR void VKAPI_CALL vkReleaseProfilingLockKHR(
	VkDevice device) 
{ 
  pfn_vkReleaseProfilingLockKHR(device); 
}
#endif /* VK_KHR_performance_query */
#if defined(VK_KHR_pipeline_executable_properties)
VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineExecutableInternalRepresentationsKHR(
	VkDevice                        device, 
	const VkPipelineExecutableInfoKHR*  pExecutableInfo, 
	uint32_t* pInternalRepresentationCount, 
	VkPipelineExecutableInternalRepresentationKHR* pInternalRepresentations) 
{ 
  return pfn_vkGetPipelineExecutableInternalRepresentationsKHR(device, pExecutableInfo, pInternalRepresentationCount, pInternalRepresentations); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineExecutablePropertiesKHR(
	VkDevice                        device, 
	const VkPipelineInfoKHR*        pPipelineInfo, 
	uint32_t* pExecutableCount, 
	VkPipelineExecutablePropertiesKHR* pProperties) 
{ 
  return pfn_vkGetPipelineExecutablePropertiesKHR(device, pPipelineInfo, pExecutableCount, pProperties); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineExecutableStatisticsKHR(
	VkDevice                        device, 
	const VkPipelineExecutableInfoKHR*  pExecutableInfo, 
	uint32_t* pStatisticCount, 
	VkPipelineExecutableStatisticKHR* pStatistics) 
{ 
  return pfn_vkGetPipelineExecutableStatisticsKHR(device, pExecutableInfo, pStatisticCount, pStatistics); 
}
#endif /* VK_KHR_pipeline_executable_properties */
#if defined(VK_KHR_present_wait)
VKAPI_ATTR VkResult VKAPI_CALL vkWaitForPresentKHR(
	VkDevice device, 
	VkSwapchainKHR swapchain, 
	uint64_t presentId, 
	uint64_t timeout) 
{ 
  return pfn_vkWaitForPresentKHR(device, swapchain, presentId, timeout); 
}
#endif /* VK_KHR_present_wait */
#if defined(VK_KHR_push_descriptor)
VKAPI_ATTR void VKAPI_CALL vkCmdPushDescriptorSetKHR(
	VkCommandBuffer commandBuffer, 
	VkPipelineBindPoint pipelineBindPoint, 
	VkPipelineLayout layout, 
	uint32_t set, 
	uint32_t descriptorWriteCount, 
	const VkWriteDescriptorSet* pDescriptorWrites) 
{ 
  pfn_vkCmdPushDescriptorSetKHR(commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites); 
}
#endif /* VK_KHR_push_descriptor */
#if defined(VK_KHR_ray_tracing_maintenance1) && defined(VK_KHR_ray_tracing_pipeline)
VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysIndirect2KHR(
	VkCommandBuffer commandBuffer, 
	VkDeviceAddress indirectDeviceAddress) 
{ 
  pfn_vkCmdTraceRaysIndirect2KHR(commandBuffer, indirectDeviceAddress); 
}
#endif /* VK_KHR_ray_tracing_maintenance1 && VK_KHR_ray_tracing_pipeline */
#if defined(VK_KHR_ray_tracing_pipeline)
VKAPI_ATTR void VKAPI_CALL vkCmdSetRayTracingPipelineStackSizeKHR(
	VkCommandBuffer commandBuffer, 
	uint32_t pipelineStackSize) 
{ 
  pfn_vkCmdSetRayTracingPipelineStackSizeKHR(commandBuffer, pipelineStackSize); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysIndirectKHR(
	VkCommandBuffer commandBuffer, 
	const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, 
	VkDeviceAddress indirectDeviceAddress) 
{ 
  pfn_vkCmdTraceRaysIndirectKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, indirectDeviceAddress); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysKHR(
	VkCommandBuffer commandBuffer, 
	const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, 
	uint32_t width, 
	uint32_t height, 
	uint32_t depth) 
{ 
  pfn_vkCmdTraceRaysKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR(
	VkDevice device, 
	VkDeferredOperationKHR deferredOperation, 
	VkPipelineCache pipelineCache, 
	uint32_t createInfoCount, 
	const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, 
	const VkAllocationCallbacks* pAllocator, 
	VkPipeline* pPipelines) 
{ 
  return pfn_vkCreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(
	VkDevice device, 
	VkPipeline pipeline, 
	uint32_t firstGroup, 
	uint32_t groupCount, 
	size_t dataSize, 
	void* pData) 
{ 
  return pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesKHR(
	VkDevice device, 
	VkPipeline pipeline, 
	uint32_t firstGroup, 
	uint32_t groupCount, 
	size_t dataSize, 
	void* pData) 
{ 
  return pfn_vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData); 
}
VKAPI_ATTR VkDeviceSize VKAPI_CALL vkGetRayTracingShaderGroupStackSizeKHR(
	VkDevice device, 
	VkPipeline pipeline, 
	uint32_t group, 
	VkShaderGroupShaderKHR groupShader) 
{ 
  return pfn_vkGetRayTracingShaderGroupStackSizeKHR(device, pipeline, group, groupShader); 
}
#endif /* VK_KHR_ray_tracing_pipeline */
#if defined(VK_KHR_sampler_ycbcr_conversion)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateSamplerYcbcrConversionKHR(
	VkDevice device, 
	const VkSamplerYcbcrConversionCreateInfo* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkSamplerYcbcrConversion* pYcbcrConversion) 
{ 
  return pfn_vkCreateSamplerYcbcrConversionKHR(device, pCreateInfo, pAllocator, pYcbcrConversion); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroySamplerYcbcrConversionKHR(
	VkDevice device, 
	VkSamplerYcbcrConversion ycbcrConversion, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroySamplerYcbcrConversionKHR(device, ycbcrConversion, pAllocator); 
}
#endif /* VK_KHR_sampler_ycbcr_conversion */
#if defined(VK_KHR_shared_presentable_image)
VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainStatusKHR(
	VkDevice device, 
	VkSwapchainKHR swapchain) 
{ 
  return pfn_vkGetSwapchainStatusKHR(device, swapchain); 
}
#endif /* VK_KHR_shared_presentable_image */
#if defined(VK_KHR_synchronization2)
VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier2KHR(
	VkCommandBuffer                   commandBuffer, 
	const VkDependencyInfo*                             pDependencyInfo) 
{ 
  pfn_vkCmdPipelineBarrier2KHR(commandBuffer, pDependencyInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent2KHR(
	VkCommandBuffer                   commandBuffer, 
	VkEvent                                             event, 
	VkPipelineStageFlags2               stageMask) 
{ 
  pfn_vkCmdResetEvent2KHR(commandBuffer, event, stageMask); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent2KHR(
	VkCommandBuffer                   commandBuffer, 
	VkEvent                                             event, 
	const VkDependencyInfo*                             pDependencyInfo) 
{ 
  pfn_vkCmdSetEvent2KHR(commandBuffer, event, pDependencyInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents2KHR(
	VkCommandBuffer                   commandBuffer, 
	uint32_t                                            eventCount, 
	const VkEvent*                     pEvents, 
	const VkDependencyInfo*            pDependencyInfos) 
{ 
  pfn_vkCmdWaitEvents2KHR(commandBuffer, eventCount, pEvents, pDependencyInfos); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp2KHR(
	VkCommandBuffer                   commandBuffer, 
	VkPipelineStageFlags2               stage, 
	VkQueryPool                                         queryPool, 
	uint32_t                                            query) 
{ 
  pfn_vkCmdWriteTimestamp2KHR(commandBuffer, stage, queryPool, query); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit2KHR(
	VkQueue                           queue, 
	uint32_t                            submitCount, 
	const VkSubmitInfo2*              pSubmits, 
	VkFence           fence) 
{ 
  return pfn_vkQueueSubmit2KHR(queue, submitCount, pSubmits, fence); 
}
#endif /* VK_KHR_synchronization2 */
#if defined(VK_KHR_synchronization2) && defined(VK_AMD_buffer_marker)
VKAPI_ATTR void VKAPI_CALL vkCmdWriteBufferMarker2AMD(
	VkCommandBuffer                   commandBuffer, 
	VkPipelineStageFlags2               stage, 
	VkBuffer                                            dstBuffer, 
	VkDeviceSize                                        dstOffset, 
	uint32_t                                            marker) 
{ 
  pfn_vkCmdWriteBufferMarker2AMD(commandBuffer, stage, dstBuffer, dstOffset, marker); 
}
#endif /* VK_KHR_synchronization2 && VK_AMD_buffer_marker */
#if defined(VK_KHR_synchronization2) && defined(VK_NV_device_diagnostic_checkpoints)
VKAPI_ATTR void VKAPI_CALL vkGetQueueCheckpointData2NV(
	VkQueue queue, 
	uint32_t* pCheckpointDataCount, 
	VkCheckpointData2NV* pCheckpointData) 
{ 
  pfn_vkGetQueueCheckpointData2NV(queue, pCheckpointDataCount, pCheckpointData); 
}
#endif /* VK_KHR_synchronization2 && VK_NV_device_diagnostic_checkpoints */
#if defined(VK_KHR_timeline_semaphore)
VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreCounterValueKHR(
	VkDevice device, 
	VkSemaphore semaphore, 
	uint64_t* pValue) 
{ 
  return pfn_vkGetSemaphoreCounterValueKHR(device, semaphore, pValue); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkSignalSemaphoreKHR(
	VkDevice device, 
	const VkSemaphoreSignalInfo* pSignalInfo) 
{ 
  return pfn_vkSignalSemaphoreKHR(device, pSignalInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkWaitSemaphoresKHR(
	VkDevice device, 
	const VkSemaphoreWaitInfo* pWaitInfo, 
	uint64_t timeout) 
{ 
  return pfn_vkWaitSemaphoresKHR(device, pWaitInfo, timeout); 
}
#endif /* VK_KHR_timeline_semaphore */
#if defined(VK_KHR_video_decode_queue)
VKAPI_ATTR void VKAPI_CALL vkCmdDecodeVideoKHR(
	VkCommandBuffer commandBuffer, 
	const VkVideoDecodeInfoKHR* pDecodeInfo) 
{ 
  pfn_vkCmdDecodeVideoKHR(commandBuffer, pDecodeInfo); 
}
#endif /* VK_KHR_video_decode_queue */
#if defined(VK_KHR_video_queue)
VKAPI_ATTR VkResult VKAPI_CALL vkBindVideoSessionMemoryKHR(
	VkDevice device, 
	VkVideoSessionKHR videoSession, 
	uint32_t bindSessionMemoryInfoCount, 
	const VkBindVideoSessionMemoryInfoKHR* pBindSessionMemoryInfos) 
{ 
  return pfn_vkBindVideoSessionMemoryKHR(device, videoSession, bindSessionMemoryInfoCount, pBindSessionMemoryInfos); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdBeginVideoCodingKHR(
	VkCommandBuffer commandBuffer, 
	const VkVideoBeginCodingInfoKHR* pBeginInfo) 
{ 
  pfn_vkCmdBeginVideoCodingKHR(commandBuffer, pBeginInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdControlVideoCodingKHR(
	VkCommandBuffer commandBuffer, 
	const VkVideoCodingControlInfoKHR* pCodingControlInfo) 
{ 
  pfn_vkCmdControlVideoCodingKHR(commandBuffer, pCodingControlInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdEndVideoCodingKHR(
	VkCommandBuffer commandBuffer, 
	const VkVideoEndCodingInfoKHR* pEndCodingInfo) 
{ 
  pfn_vkCmdEndVideoCodingKHR(commandBuffer, pEndCodingInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateVideoSessionKHR(
	VkDevice device, 
	const VkVideoSessionCreateInfoKHR* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkVideoSessionKHR* pVideoSession) 
{ 
  return pfn_vkCreateVideoSessionKHR(device, pCreateInfo, pAllocator, pVideoSession); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateVideoSessionParametersKHR(
	VkDevice device, 
	const VkVideoSessionParametersCreateInfoKHR* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkVideoSessionParametersKHR* pVideoSessionParameters) 
{ 
  return pfn_vkCreateVideoSessionParametersKHR(device, pCreateInfo, pAllocator, pVideoSessionParameters); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyVideoSessionKHR(
	VkDevice device, 
	VkVideoSessionKHR videoSession, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyVideoSessionKHR(device, videoSession, pAllocator); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyVideoSessionParametersKHR(
	VkDevice device, 
	VkVideoSessionParametersKHR videoSessionParameters, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyVideoSessionParametersKHR(device, videoSessionParameters, pAllocator); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceVideoCapabilitiesKHR(
	VkPhysicalDevice physicalDevice, 
	const VkVideoProfileInfoKHR* pVideoProfile, 
	VkVideoCapabilitiesKHR* pCapabilities) 
{ 
  return pfn_vkGetPhysicalDeviceVideoCapabilitiesKHR(physicalDevice, pVideoProfile, pCapabilities); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceVideoFormatPropertiesKHR(
	VkPhysicalDevice physicalDevice, 
	const VkPhysicalDeviceVideoFormatInfoKHR* pVideoFormatInfo, 
	uint32_t* pVideoFormatPropertyCount, 
	VkVideoFormatPropertiesKHR* pVideoFormatProperties) 
{ 
  return pfn_vkGetPhysicalDeviceVideoFormatPropertiesKHR(physicalDevice, pVideoFormatInfo, pVideoFormatPropertyCount, pVideoFormatProperties); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetVideoSessionMemoryRequirementsKHR(
	VkDevice device, 
	VkVideoSessionKHR videoSession, 
	uint32_t* pMemoryRequirementsCount, 
	VkVideoSessionMemoryRequirementsKHR* pMemoryRequirements) 
{ 
  return pfn_vkGetVideoSessionMemoryRequirementsKHR(device, videoSession, pMemoryRequirementsCount, pMemoryRequirements); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkUpdateVideoSessionParametersKHR(
	VkDevice device, 
	VkVideoSessionParametersKHR videoSessionParameters, 
	const VkVideoSessionParametersUpdateInfoKHR* pUpdateInfo) 
{ 
  return pfn_vkUpdateVideoSessionParametersKHR(device, videoSessionParameters, pUpdateInfo); 
}
#endif /* VK_KHR_video_queue */
#if defined(VK_MVK_ios_surface)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateIOSSurfaceMVK(
	VkInstance instance, 
	const VkIOSSurfaceCreateInfoMVK* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkSurfaceKHR* pSurface) 
{ 
  return pfn_vkCreateIOSSurfaceMVK(instance, pCreateInfo, pAllocator, pSurface); 
}
#endif /* VK_MVK_ios_surface */
#if defined(VK_MVK_macos_surface)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateMacOSSurfaceMVK(
	VkInstance instance, 
	const VkMacOSSurfaceCreateInfoMVK* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkSurfaceKHR* pSurface) 
{ 
  return pfn_vkCreateMacOSSurfaceMVK(instance, pCreateInfo, pAllocator, pSurface); 
}
#endif /* VK_MVK_macos_surface */
#if defined(VK_NN_vi_surface)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateViSurfaceNN(
	VkInstance instance, 
	const VkViSurfaceCreateInfoNN* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkSurfaceKHR* pSurface) 
{ 
  return pfn_vkCreateViSurfaceNN(instance, pCreateInfo, pAllocator, pSurface); 
}
#endif /* VK_NN_vi_surface */
#if defined(VK_NVX_binary_import)
VKAPI_ATTR void VKAPI_CALL vkCmdCuLaunchKernelNVX(
	VkCommandBuffer commandBuffer, 
	const VkCuLaunchInfoNVX* pLaunchInfo) 
{ 
  pfn_vkCmdCuLaunchKernelNVX(commandBuffer, pLaunchInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateCuFunctionNVX(
	VkDevice device, 
	const VkCuFunctionCreateInfoNVX* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkCuFunctionNVX* pFunction) 
{ 
  return pfn_vkCreateCuFunctionNVX(device, pCreateInfo, pAllocator, pFunction); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateCuModuleNVX(
	VkDevice device, 
	const VkCuModuleCreateInfoNVX* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkCuModuleNVX* pModule) 
{ 
  return pfn_vkCreateCuModuleNVX(device, pCreateInfo, pAllocator, pModule); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyCuFunctionNVX(
	VkDevice device, 
	VkCuFunctionNVX function, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyCuFunctionNVX(device, function, pAllocator); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyCuModuleNVX(
	VkDevice device, 
	VkCuModuleNVX module, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyCuModuleNVX(device, module, pAllocator); 
}
#endif /* VK_NVX_binary_import */
#if defined(VK_NVX_image_view_handle)
VKAPI_ATTR VkResult VKAPI_CALL vkGetImageViewAddressNVX(
	VkDevice device, 
	VkImageView imageView, 
	VkImageViewAddressPropertiesNVX* pProperties) 
{ 
  return pfn_vkGetImageViewAddressNVX(device, imageView, pProperties); 
}
VKAPI_ATTR uint32_t VKAPI_CALL vkGetImageViewHandleNVX(
	VkDevice device, 
	const VkImageViewHandleInfoNVX* pInfo) 
{ 
  return pfn_vkGetImageViewHandleNVX(device, pInfo); 
}
#endif /* VK_NVX_image_view_handle */
#if defined(VK_NV_acquire_winrt_display)
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireWinrtDisplayNV(
	VkPhysicalDevice physicalDevice, 
	VkDisplayKHR display) 
{ 
  return pfn_vkAcquireWinrtDisplayNV(physicalDevice, display); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetWinrtDisplayNV(
	VkPhysicalDevice physicalDevice, 
	uint32_t deviceRelativeId, 
	VkDisplayKHR* pDisplay) 
{ 
  return pfn_vkGetWinrtDisplayNV(physicalDevice, deviceRelativeId, pDisplay); 
}
#endif /* VK_NV_acquire_winrt_display */
#if defined(VK_NV_clip_space_w_scaling)
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWScalingNV(
	VkCommandBuffer commandBuffer, 
	uint32_t firstViewport, 
	uint32_t viewportCount, 
	const VkViewportWScalingNV* pViewportWScalings) 
{ 
  pfn_vkCmdSetViewportWScalingNV(commandBuffer, firstViewport, viewportCount, pViewportWScalings); 
}
#endif /* VK_NV_clip_space_w_scaling */
#if defined(VK_NV_cooperative_matrix)
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(
	VkPhysicalDevice physicalDevice, 
	uint32_t* pPropertyCount, 
	VkCooperativeMatrixPropertiesNV* pProperties) 
{ 
  return pfn_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(physicalDevice, pPropertyCount, pProperties); 
}
#endif /* VK_NV_cooperative_matrix */
#if defined(VK_NV_copy_memory_indirect)
VKAPI_ATTR void VKAPI_CALL vkCmdCopyMemoryIndirectNV(
	VkCommandBuffer commandBuffer, 
	VkDeviceAddress copyBufferAddress, 
	uint32_t copyCount, 
	uint32_t stride) 
{ 
  pfn_vkCmdCopyMemoryIndirectNV(commandBuffer, copyBufferAddress, copyCount, stride); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyMemoryToImageIndirectNV(
	VkCommandBuffer commandBuffer, 
	VkDeviceAddress copyBufferAddress, 
	uint32_t copyCount, 
	uint32_t stride, 
	VkImage dstImage, 
	VkImageLayout dstImageLayout, 
	const VkImageSubresourceLayers* pImageSubresources) 
{ 
  pfn_vkCmdCopyMemoryToImageIndirectNV(commandBuffer, copyBufferAddress, copyCount, stride, dstImage, dstImageLayout, pImageSubresources); 
}
#endif /* VK_NV_copy_memory_indirect */
#if defined(VK_NV_coverage_reduction_mode)
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(
	VkPhysicalDevice physicalDevice, 
	uint32_t* pCombinationCount, 
	VkFramebufferMixedSamplesCombinationNV* pCombinations) 
{ 
  return pfn_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(physicalDevice, pCombinationCount, pCombinations); 
}
#endif /* VK_NV_coverage_reduction_mode */
#if defined(VK_NV_device_diagnostic_checkpoints)
VKAPI_ATTR void VKAPI_CALL vkCmdSetCheckpointNV(
	VkCommandBuffer commandBuffer, 
	const void* pCheckpointMarker) 
{ 
  pfn_vkCmdSetCheckpointNV(commandBuffer, pCheckpointMarker); 
}
VKAPI_ATTR void VKAPI_CALL vkGetQueueCheckpointDataNV(
	VkQueue queue, 
	uint32_t* pCheckpointDataCount, 
	VkCheckpointDataNV* pCheckpointData) 
{ 
  pfn_vkGetQueueCheckpointDataNV(queue, pCheckpointDataCount, pCheckpointData); 
}
#endif /* VK_NV_device_diagnostic_checkpoints */
#if defined(VK_NV_device_generated_commands)
VKAPI_ATTR void VKAPI_CALL vkCmdBindPipelineShaderGroupNV(
	VkCommandBuffer commandBuffer, 
	VkPipelineBindPoint pipelineBindPoint, 
	VkPipeline pipeline, 
	uint32_t groupIndex) 
{ 
  pfn_vkCmdBindPipelineShaderGroupNV(commandBuffer, pipelineBindPoint, pipeline, groupIndex); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdExecuteGeneratedCommandsNV(
	VkCommandBuffer commandBuffer, 
	VkBool32 isPreprocessed, 
	const VkGeneratedCommandsInfoNV* pGeneratedCommandsInfo) 
{ 
  pfn_vkCmdExecuteGeneratedCommandsNV(commandBuffer, isPreprocessed, pGeneratedCommandsInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdPreprocessGeneratedCommandsNV(
	VkCommandBuffer commandBuffer, 
	const VkGeneratedCommandsInfoNV* pGeneratedCommandsInfo) 
{ 
  pfn_vkCmdPreprocessGeneratedCommandsNV(commandBuffer, pGeneratedCommandsInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateIndirectCommandsLayoutNV(
	VkDevice device, 
	const VkIndirectCommandsLayoutCreateInfoNV* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkIndirectCommandsLayoutNV* pIndirectCommandsLayout) 
{ 
  return pfn_vkCreateIndirectCommandsLayoutNV(device, pCreateInfo, pAllocator, pIndirectCommandsLayout); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyIndirectCommandsLayoutNV(
	VkDevice device, 
	VkIndirectCommandsLayoutNV indirectCommandsLayout, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyIndirectCommandsLayoutNV(device, indirectCommandsLayout, pAllocator); 
}
VKAPI_ATTR void VKAPI_CALL vkGetGeneratedCommandsMemoryRequirementsNV(
	VkDevice device, 
	const VkGeneratedCommandsMemoryRequirementsInfoNV* pInfo, 
	VkMemoryRequirements2* pMemoryRequirements) 
{ 
  pfn_vkGetGeneratedCommandsMemoryRequirementsNV(device, pInfo, pMemoryRequirements); 
}
#endif /* VK_NV_device_generated_commands */
#if defined(VK_NV_device_generated_commands_compute)
VKAPI_ATTR void VKAPI_CALL vkCmdUpdatePipelineIndirectBufferNV(
	VkCommandBuffer commandBuffer, 
	VkPipelineBindPoint           pipelineBindPoint, 
	VkPipeline                    pipeline) 
{ 
  pfn_vkCmdUpdatePipelineIndirectBufferNV(commandBuffer, pipelineBindPoint, pipeline); 
}
VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetPipelineIndirectDeviceAddressNV(
	VkDevice device, 
	const VkPipelineIndirectDeviceAddressInfoNV* pInfo) 
{ 
  return pfn_vkGetPipelineIndirectDeviceAddressNV(device, pInfo); 
}
VKAPI_ATTR void VKAPI_CALL vkGetPipelineIndirectMemoryRequirementsNV(
	VkDevice device, 
	const VkComputePipelineCreateInfo* pCreateInfo, 
	VkMemoryRequirements2* pMemoryRequirements) 
{ 
  pfn_vkGetPipelineIndirectMemoryRequirementsNV(device, pCreateInfo, pMemoryRequirements); 
}
#endif /* VK_NV_device_generated_commands_compute */
#if defined(VK_NV_external_memory_capabilities)
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceExternalImageFormatPropertiesNV(
	VkPhysicalDevice physicalDevice, 
	VkFormat format, 
	VkImageType type, 
	VkImageTiling tiling, 
	VkImageUsageFlags usage, 
	VkImageCreateFlags flags, 
	VkExternalMemoryHandleTypeFlagsNV externalHandleType, 
	VkExternalImageFormatPropertiesNV* pExternalImageFormatProperties) 
{ 
  return pfn_vkGetPhysicalDeviceExternalImageFormatPropertiesNV(physicalDevice, format, type, tiling, usage, flags, externalHandleType, pExternalImageFormatProperties); 
}
#endif /* VK_NV_external_memory_capabilities */
#if defined(VK_NV_external_memory_rdma)
VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryRemoteAddressNV(
	VkDevice device, 
	const VkMemoryGetRemoteAddressInfoNV* pMemoryGetRemoteAddressInfo, 
	VkRemoteAddressNV* pAddress) 
{ 
  return pfn_vkGetMemoryRemoteAddressNV(device, pMemoryGetRemoteAddressInfo, pAddress); 
}
#endif /* VK_NV_external_memory_rdma */
#if defined(VK_NV_external_memory_win32)
VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandleNV(
	VkDevice device, 
	VkDeviceMemory memory, 
	VkExternalMemoryHandleTypeFlagsNV handleType, 
	HANDLE* pHandle) 
{ 
  return pfn_vkGetMemoryWin32HandleNV(device, memory, handleType, pHandle); 
}
#endif /* VK_NV_external_memory_win32 */
#if defined(VK_NV_fragment_shading_rate_enums)
VKAPI_ATTR void VKAPI_CALL vkCmdSetFragmentShadingRateEnumNV(
	VkCommandBuffer           commandBuffer, 
	VkFragmentShadingRateNV                     shadingRate, 
	const VkFragmentShadingRateCombinerOpKHR    combinerOps[2]) 
{ 
  pfn_vkCmdSetFragmentShadingRateEnumNV(commandBuffer, shadingRate, combinerOps); 
}
#endif /* VK_NV_fragment_shading_rate_enums */
#if defined(VK_NV_memory_decompression)
VKAPI_ATTR void VKAPI_CALL vkCmdDecompressMemoryIndirectCountNV(
	VkCommandBuffer commandBuffer, 
	VkDeviceAddress indirectCommandsAddress, 
	VkDeviceAddress indirectCommandsCountAddress, 
	uint32_t stride) 
{ 
  pfn_vkCmdDecompressMemoryIndirectCountNV(commandBuffer, indirectCommandsAddress, indirectCommandsCountAddress, stride); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDecompressMemoryNV(
	VkCommandBuffer commandBuffer, 
	uint32_t decompressRegionCount, 
	const VkDecompressMemoryRegionNV* pDecompressMemoryRegions) 
{ 
  pfn_vkCmdDecompressMemoryNV(commandBuffer, decompressRegionCount, pDecompressMemoryRegions); 
}
#endif /* VK_NV_memory_decompression */
#if defined(VK_NV_mesh_shader)
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectCountNV(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset, 
	VkBuffer countBuffer, 
	VkDeviceSize countBufferOffset, 
	uint32_t maxDrawCount, 
	uint32_t stride) 
{ 
  pfn_vkCmdDrawMeshTasksIndirectCountNV(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectNV(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset, 
	uint32_t drawCount, 
	uint32_t stride) 
{ 
  pfn_vkCmdDrawMeshTasksIndirectNV(commandBuffer, buffer, offset, drawCount, stride); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksNV(
	VkCommandBuffer commandBuffer, 
	uint32_t taskCount, 
	uint32_t firstTask) 
{ 
  pfn_vkCmdDrawMeshTasksNV(commandBuffer, taskCount, firstTask); 
}
#endif /* VK_NV_mesh_shader */
#if defined(VK_NV_optical_flow)
VKAPI_ATTR VkResult VKAPI_CALL vkBindOpticalFlowSessionImageNV(
	VkDevice device, 
	VkOpticalFlowSessionNV session, 
	VkOpticalFlowSessionBindingPointNV bindingPoint, 
	VkImageView view, 
	VkImageLayout layout) 
{ 
  return pfn_vkBindOpticalFlowSessionImageNV(device, session, bindingPoint, view, layout); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdOpticalFlowExecuteNV(
	VkCommandBuffer commandBuffer, 
	VkOpticalFlowSessionNV session, 
	const VkOpticalFlowExecuteInfoNV* pExecuteInfo) 
{ 
  pfn_vkCmdOpticalFlowExecuteNV(commandBuffer, session, pExecuteInfo); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateOpticalFlowSessionNV(
	VkDevice device, 
	const VkOpticalFlowSessionCreateInfoNV* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkOpticalFlowSessionNV* pSession) 
{ 
  return pfn_vkCreateOpticalFlowSessionNV(device, pCreateInfo, pAllocator, pSession); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyOpticalFlowSessionNV(
	VkDevice device, 
	VkOpticalFlowSessionNV session, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyOpticalFlowSessionNV(device, session, pAllocator); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceOpticalFlowImageFormatsNV(
	VkPhysicalDevice physicalDevice, 
	const VkOpticalFlowImageFormatInfoNV* pOpticalFlowImageFormatInfo, 
	uint32_t* pFormatCount, 
	VkOpticalFlowImageFormatPropertiesNV* pImageFormatProperties) 
{ 
  return pfn_vkGetPhysicalDeviceOpticalFlowImageFormatsNV(physicalDevice, pOpticalFlowImageFormatInfo, pFormatCount, pImageFormatProperties); 
}
#endif /* VK_NV_optical_flow */
#if defined(VK_NV_ray_tracing)
VKAPI_ATTR VkResult VKAPI_CALL vkBindAccelerationStructureMemoryNV(
	VkDevice device, 
	uint32_t bindInfoCount, 
	const VkBindAccelerationStructureMemoryInfoNV* pBindInfos) 
{ 
  return pfn_vkBindAccelerationStructureMemoryNV(device, bindInfoCount, pBindInfos); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructureNV(
	VkCommandBuffer commandBuffer, 
	const VkAccelerationStructureInfoNV* pInfo, 
	VkBuffer instanceData, 
	VkDeviceSize instanceOffset, 
	VkBool32 update, 
	VkAccelerationStructureNV dst, 
	VkAccelerationStructureNV src, 
	VkBuffer scratch, 
	VkDeviceSize scratchOffset) 
{ 
  pfn_vkCmdBuildAccelerationStructureNV(commandBuffer, pInfo, instanceData, instanceOffset, update, dst, src, scratch, scratchOffset); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureNV(
	VkCommandBuffer commandBuffer, 
	VkAccelerationStructureNV dst, 
	VkAccelerationStructureNV src, 
	VkCopyAccelerationStructureModeKHR mode) 
{ 
  pfn_vkCmdCopyAccelerationStructureNV(commandBuffer, dst, src, mode); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysNV(
	VkCommandBuffer commandBuffer, 
	VkBuffer raygenShaderBindingTableBuffer, 
	VkDeviceSize raygenShaderBindingOffset, 
	VkBuffer missShaderBindingTableBuffer, 
	VkDeviceSize missShaderBindingOffset, 
	VkDeviceSize missShaderBindingStride, 
	VkBuffer hitShaderBindingTableBuffer, 
	VkDeviceSize hitShaderBindingOffset, 
	VkDeviceSize hitShaderBindingStride, 
	VkBuffer callableShaderBindingTableBuffer, 
	VkDeviceSize callableShaderBindingOffset, 
	VkDeviceSize callableShaderBindingStride, 
	uint32_t width, 
	uint32_t height, 
	uint32_t depth) 
{ 
  pfn_vkCmdTraceRaysNV(commandBuffer, raygenShaderBindingTableBuffer, raygenShaderBindingOffset, missShaderBindingTableBuffer, missShaderBindingOffset, missShaderBindingStride, hitShaderBindingTableBuffer, hitShaderBindingOffset, hitShaderBindingStride, callableShaderBindingTableBuffer, callableShaderBindingOffset, callableShaderBindingStride, width, height, depth); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesNV(
	VkCommandBuffer commandBuffer, 
	uint32_t accelerationStructureCount, 
	const VkAccelerationStructureNV* pAccelerationStructures, 
	VkQueryType queryType, 
	VkQueryPool queryPool, 
	uint32_t firstQuery) 
{ 
  pfn_vkCmdWriteAccelerationStructuresPropertiesNV(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCompileDeferredNV(
	VkDevice device, 
	VkPipeline pipeline, 
	uint32_t shader) 
{ 
  return pfn_vkCompileDeferredNV(device, pipeline, shader); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureNV(
	VkDevice device, 
	const VkAccelerationStructureCreateInfoNV* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkAccelerationStructureNV* pAccelerationStructure) 
{ 
  return pfn_vkCreateAccelerationStructureNV(device, pCreateInfo, pAllocator, pAccelerationStructure); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesNV(
	VkDevice device, 
	VkPipelineCache pipelineCache, 
	uint32_t createInfoCount, 
	const VkRayTracingPipelineCreateInfoNV* pCreateInfos, 
	const VkAllocationCallbacks* pAllocator, 
	VkPipeline* pPipelines) 
{ 
  return pfn_vkCreateRayTracingPipelinesNV(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines); 
}
VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureNV(
	VkDevice device, 
	VkAccelerationStructureNV accelerationStructure, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyAccelerationStructureNV(device, accelerationStructure, pAllocator); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetAccelerationStructureHandleNV(
	VkDevice device, 
	VkAccelerationStructureNV accelerationStructure, 
	size_t dataSize, 
	void* pData) 
{ 
  return pfn_vkGetAccelerationStructureHandleNV(device, accelerationStructure, dataSize, pData); 
}
VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureMemoryRequirementsNV(
	VkDevice device, 
	const VkAccelerationStructureMemoryRequirementsInfoNV* pInfo, 
	VkMemoryRequirements2KHR* pMemoryRequirements) 
{ 
  pfn_vkGetAccelerationStructureMemoryRequirementsNV(device, pInfo, pMemoryRequirements); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesNV(
	VkDevice device, 
	VkPipeline pipeline, 
	uint32_t firstGroup, 
	uint32_t groupCount, 
	size_t dataSize, 
	void* pData) 
{ 
  return pfn_vkGetRayTracingShaderGroupHandlesNV(device, pipeline, firstGroup, groupCount, dataSize, pData); 
}
#endif /* VK_NV_ray_tracing */
#if defined(VK_NV_scissor_exclusive) && VK_NV_SCISSOR_EXCLUSIVE_SPEC_VERSION >= 2
VKAPI_ATTR void VKAPI_CALL vkCmdSetExclusiveScissorEnableNV(
	VkCommandBuffer commandBuffer, 
	uint32_t firstExclusiveScissor, 
	uint32_t exclusiveScissorCount, 
	const VkBool32* pExclusiveScissorEnables) 
{ 
  pfn_vkCmdSetExclusiveScissorEnableNV(commandBuffer, firstExclusiveScissor, exclusiveScissorCount, pExclusiveScissorEnables); 
}
#endif /* VK_NV_scissor_exclusive && VK_NV_SCISSOR_EXCLUSIVE_SPEC_VERSION >= 2 */
#if defined(VK_NV_scissor_exclusive)
VKAPI_ATTR void VKAPI_CALL vkCmdSetExclusiveScissorNV(
	VkCommandBuffer commandBuffer, 
	uint32_t firstExclusiveScissor, 
	uint32_t exclusiveScissorCount, 
	const VkRect2D* pExclusiveScissors) 
{ 
  pfn_vkCmdSetExclusiveScissorNV(commandBuffer, firstExclusiveScissor, exclusiveScissorCount, pExclusiveScissors); 
}
#endif /* VK_NV_scissor_exclusive */
#if defined(VK_NV_shading_rate_image)
VKAPI_ATTR void VKAPI_CALL vkCmdBindShadingRateImageNV(
	VkCommandBuffer commandBuffer, 
	VkImageView imageView, 
	VkImageLayout imageLayout) 
{ 
  pfn_vkCmdBindShadingRateImageNV(commandBuffer, imageView, imageLayout); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetCoarseSampleOrderNV(
	VkCommandBuffer commandBuffer, 
	VkCoarseSampleOrderTypeNV sampleOrderType, 
	uint32_t customSampleOrderCount, 
	const VkCoarseSampleOrderCustomNV* pCustomSampleOrders) 
{ 
  pfn_vkCmdSetCoarseSampleOrderNV(commandBuffer, sampleOrderType, customSampleOrderCount, pCustomSampleOrders); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportShadingRatePaletteNV(
	VkCommandBuffer commandBuffer, 
	uint32_t firstViewport, 
	uint32_t viewportCount, 
	const VkShadingRatePaletteNV* pShadingRatePalettes) 
{ 
  pfn_vkCmdSetViewportShadingRatePaletteNV(commandBuffer, firstViewport, viewportCount, pShadingRatePalettes); 
}
#endif /* VK_NV_shading_rate_image */
#if defined(VK_QCOM_tile_properties)
VKAPI_ATTR VkResult VKAPI_CALL vkGetDynamicRenderingTilePropertiesQCOM(
	VkDevice device, 
	const VkRenderingInfo* pRenderingInfo, 
	VkTilePropertiesQCOM* pProperties) 
{ 
  return pfn_vkGetDynamicRenderingTilePropertiesQCOM(device, pRenderingInfo, pProperties); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetFramebufferTilePropertiesQCOM(
	VkDevice device, 
	VkFramebuffer framebuffer, 
	uint32_t* pPropertiesCount, 
	VkTilePropertiesQCOM* pProperties) 
{ 
  return pfn_vkGetFramebufferTilePropertiesQCOM(device, framebuffer, pPropertiesCount, pProperties); 
}
#endif /* VK_QCOM_tile_properties */
#if defined(VK_QNX_external_memory_screen_buffer)
VKAPI_ATTR VkResult VKAPI_CALL vkGetScreenBufferPropertiesQNX(
	VkDevice device, 
	const struct _screen_buffer* buffer, 
	VkScreenBufferPropertiesQNX* pProperties) 
{ 
  return pfn_vkGetScreenBufferPropertiesQNX(device, buffer, pProperties); 
}
#endif /* VK_QNX_external_memory_screen_buffer */
#if defined(VK_QNX_screen_surface)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateScreenSurfaceQNX(
	VkInstance instance, 
	const VkScreenSurfaceCreateInfoQNX* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkSurfaceKHR* pSurface) 
{ 
  return pfn_vkCreateScreenSurfaceQNX(instance, pCreateInfo, pAllocator, pSurface); 
}
VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceScreenPresentationSupportQNX(
	VkPhysicalDevice physicalDevice, 
	uint32_t queueFamilyIndex, 
	struct _screen_window* window) 
{ 
  return pfn_vkGetPhysicalDeviceScreenPresentationSupportQNX(physicalDevice, queueFamilyIndex, window); 
}
#endif /* VK_QNX_screen_surface */
#if defined(VK_VALVE_descriptor_set_host_mapping)
VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetHostMappingVALVE(
	VkDevice device, 
	VkDescriptorSet descriptorSet, 
	void** ppData) 
{ 
  pfn_vkGetDescriptorSetHostMappingVALVE(device, descriptorSet, ppData); 
}
VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutHostMappingInfoVALVE(
	VkDevice device, 
	const VkDescriptorSetBindingReferenceVALVE* pBindingReference, 
	VkDescriptorSetLayoutHostMappingInfoVALVE* pHostMapping) 
{ 
  pfn_vkGetDescriptorSetLayoutHostMappingInfoVALVE(device, pBindingReference, pHostMapping); 
}
#endif /* VK_VALVE_descriptor_set_host_mapping */
#if defined(VK_EXT_extended_dynamic_state) || defined(VK_EXT_shader_object)
VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers2EXT(
	VkCommandBuffer commandBuffer, 
	uint32_t firstBinding, 
	uint32_t bindingCount, 
	const VkBuffer* pBuffers, 
	const VkDeviceSize* pOffsets, 
	const VkDeviceSize* pSizes, 
	const VkDeviceSize* pStrides) 
{ 
  pfn_vkCmdBindVertexBuffers2EXT(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetCullModeEXT(
	VkCommandBuffer commandBuffer, 
	VkCullModeFlags cullMode) 
{ 
  pfn_vkCmdSetCullModeEXT(commandBuffer, cullMode); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBoundsTestEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 depthBoundsTestEnable) 
{ 
  pfn_vkCmdSetDepthBoundsTestEnableEXT(commandBuffer, depthBoundsTestEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthCompareOpEXT(
	VkCommandBuffer commandBuffer, 
	VkCompareOp depthCompareOp) 
{ 
  pfn_vkCmdSetDepthCompareOpEXT(commandBuffer, depthCompareOp); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthTestEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 depthTestEnable) 
{ 
  pfn_vkCmdSetDepthTestEnableEXT(commandBuffer, depthTestEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthWriteEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 depthWriteEnable) 
{ 
  pfn_vkCmdSetDepthWriteEnableEXT(commandBuffer, depthWriteEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetFrontFaceEXT(
	VkCommandBuffer commandBuffer, 
	VkFrontFace frontFace) 
{ 
  pfn_vkCmdSetFrontFaceEXT(commandBuffer, frontFace); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetPrimitiveTopologyEXT(
	VkCommandBuffer commandBuffer, 
	VkPrimitiveTopology primitiveTopology) 
{ 
  pfn_vkCmdSetPrimitiveTopologyEXT(commandBuffer, primitiveTopology); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetScissorWithCountEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t scissorCount, 
	const VkRect2D* pScissors) 
{ 
  pfn_vkCmdSetScissorWithCountEXT(commandBuffer, scissorCount, pScissors); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilOpEXT(
	VkCommandBuffer commandBuffer, 
	VkStencilFaceFlags faceMask, 
	VkStencilOp failOp, 
	VkStencilOp passOp, 
	VkStencilOp depthFailOp, 
	VkCompareOp compareOp) 
{ 
  pfn_vkCmdSetStencilOpEXT(commandBuffer, faceMask, failOp, passOp, depthFailOp, compareOp); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilTestEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 stencilTestEnable) 
{ 
  pfn_vkCmdSetStencilTestEnableEXT(commandBuffer, stencilTestEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWithCountEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t viewportCount, 
	const VkViewport* pViewports) 
{ 
  pfn_vkCmdSetViewportWithCountEXT(commandBuffer, viewportCount, pViewports); 
}
#endif /* VK_EXT_extended_dynamic_state || VK_EXT_shader_object */
#if defined(VK_EXT_extended_dynamic_state2) || defined(VK_EXT_shader_object)
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBiasEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 depthBiasEnable) 
{ 
  pfn_vkCmdSetDepthBiasEnableEXT(commandBuffer, depthBiasEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetLogicOpEXT(
	VkCommandBuffer commandBuffer, 
	VkLogicOp logicOp) 
{ 
  pfn_vkCmdSetLogicOpEXT(commandBuffer, logicOp); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetPatchControlPointsEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t patchControlPoints) 
{ 
  pfn_vkCmdSetPatchControlPointsEXT(commandBuffer, patchControlPoints); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetPrimitiveRestartEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 primitiveRestartEnable) 
{ 
  pfn_vkCmdSetPrimitiveRestartEnableEXT(commandBuffer, primitiveRestartEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetRasterizerDiscardEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 rasterizerDiscardEnable) 
{ 
  pfn_vkCmdSetRasterizerDiscardEnableEXT(commandBuffer, rasterizerDiscardEnable); 
}
#endif /* VK_EXT_extended_dynamic_state2 || VK_EXT_shader_object */
#if defined(VK_EXT_extended_dynamic_state3) || defined(VK_EXT_shader_object)
VKAPI_ATTR void VKAPI_CALL vkCmdSetAlphaToCoverageEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 alphaToCoverageEnable) 
{ 
  pfn_vkCmdSetAlphaToCoverageEnableEXT(commandBuffer, alphaToCoverageEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetAlphaToOneEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 alphaToOneEnable) 
{ 
  pfn_vkCmdSetAlphaToOneEnableEXT(commandBuffer, alphaToOneEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetColorBlendAdvancedEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t firstAttachment, 
	uint32_t attachmentCount, 
	const VkColorBlendAdvancedEXT* pColorBlendAdvanced) 
{ 
  pfn_vkCmdSetColorBlendAdvancedEXT(commandBuffer, firstAttachment, attachmentCount, pColorBlendAdvanced); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetColorBlendEnableEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t firstAttachment, 
	uint32_t attachmentCount, 
	const VkBool32* pColorBlendEnables) 
{ 
  pfn_vkCmdSetColorBlendEnableEXT(commandBuffer, firstAttachment, attachmentCount, pColorBlendEnables); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetColorBlendEquationEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t firstAttachment, 
	uint32_t attachmentCount, 
	const VkColorBlendEquationEXT* pColorBlendEquations) 
{ 
  pfn_vkCmdSetColorBlendEquationEXT(commandBuffer, firstAttachment, attachmentCount, pColorBlendEquations); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetColorWriteMaskEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t firstAttachment, 
	uint32_t attachmentCount, 
	const VkColorComponentFlags* pColorWriteMasks) 
{ 
  pfn_vkCmdSetColorWriteMaskEXT(commandBuffer, firstAttachment, attachmentCount, pColorWriteMasks); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetConservativeRasterizationModeEXT(
	VkCommandBuffer commandBuffer, 
	VkConservativeRasterizationModeEXT conservativeRasterizationMode) 
{ 
  pfn_vkCmdSetConservativeRasterizationModeEXT(commandBuffer, conservativeRasterizationMode); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthClampEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 depthClampEnable) 
{ 
  pfn_vkCmdSetDepthClampEnableEXT(commandBuffer, depthClampEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthClipEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 depthClipEnable) 
{ 
  pfn_vkCmdSetDepthClipEnableEXT(commandBuffer, depthClipEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthClipNegativeOneToOneEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 negativeOneToOne) 
{ 
  pfn_vkCmdSetDepthClipNegativeOneToOneEXT(commandBuffer, negativeOneToOne); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetExtraPrimitiveOverestimationSizeEXT(
	VkCommandBuffer commandBuffer, 
	float extraPrimitiveOverestimationSize) 
{ 
  pfn_vkCmdSetExtraPrimitiveOverestimationSizeEXT(commandBuffer, extraPrimitiveOverestimationSize); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetLineRasterizationModeEXT(
	VkCommandBuffer commandBuffer, 
	VkLineRasterizationModeEXT lineRasterizationMode) 
{ 
  pfn_vkCmdSetLineRasterizationModeEXT(commandBuffer, lineRasterizationMode); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetLineStippleEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 stippledLineEnable) 
{ 
  pfn_vkCmdSetLineStippleEnableEXT(commandBuffer, stippledLineEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetLogicOpEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 logicOpEnable) 
{ 
  pfn_vkCmdSetLogicOpEnableEXT(commandBuffer, logicOpEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetPolygonModeEXT(
	VkCommandBuffer commandBuffer, 
	VkPolygonMode polygonMode) 
{ 
  pfn_vkCmdSetPolygonModeEXT(commandBuffer, polygonMode); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetProvokingVertexModeEXT(
	VkCommandBuffer commandBuffer, 
	VkProvokingVertexModeEXT provokingVertexMode) 
{ 
  pfn_vkCmdSetProvokingVertexModeEXT(commandBuffer, provokingVertexMode); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetRasterizationSamplesEXT(
	VkCommandBuffer commandBuffer, 
	VkSampleCountFlagBits  rasterizationSamples) 
{ 
  pfn_vkCmdSetRasterizationSamplesEXT(commandBuffer, rasterizationSamples); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetRasterizationStreamEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t rasterizationStream) 
{ 
  pfn_vkCmdSetRasterizationStreamEXT(commandBuffer, rasterizationStream); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetSampleLocationsEnableEXT(
	VkCommandBuffer commandBuffer, 
	VkBool32 sampleLocationsEnable) 
{ 
  pfn_vkCmdSetSampleLocationsEnableEXT(commandBuffer, sampleLocationsEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetSampleMaskEXT(
	VkCommandBuffer commandBuffer, 
	VkSampleCountFlagBits  samples, 
	const VkSampleMask*    pSampleMask) 
{ 
  pfn_vkCmdSetSampleMaskEXT(commandBuffer, samples, pSampleMask); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetTessellationDomainOriginEXT(
	VkCommandBuffer commandBuffer, 
	VkTessellationDomainOrigin domainOrigin) 
{ 
  pfn_vkCmdSetTessellationDomainOriginEXT(commandBuffer, domainOrigin); 
}
#endif /* VK_EXT_extended_dynamic_state3 || VK_EXT_shader_object */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_clip_space_w_scaling) || defined(VK_EXT_shader_object) && defined(VK_NV_clip_space_w_scaling)
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWScalingEnableNV(
	VkCommandBuffer commandBuffer, 
	VkBool32 viewportWScalingEnable) 
{ 
  pfn_vkCmdSetViewportWScalingEnableNV(commandBuffer, viewportWScalingEnable); 
}
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_clip_space_w_scaling || VK_EXT_shader_object && VK_NV_clip_space_w_scaling */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_viewport_swizzle) || defined(VK_EXT_shader_object) && defined(VK_NV_viewport_swizzle)
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportSwizzleNV(
	VkCommandBuffer commandBuffer, 
	uint32_t firstViewport, 
	uint32_t viewportCount, 
	const VkViewportSwizzleNV* pViewportSwizzles) 
{ 
  pfn_vkCmdSetViewportSwizzleNV(commandBuffer, firstViewport, viewportCount, pViewportSwizzles); 
}
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_viewport_swizzle || VK_EXT_shader_object && VK_NV_viewport_swizzle */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_fragment_coverage_to_color) || defined(VK_EXT_shader_object) && defined(VK_NV_fragment_coverage_to_color)
VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageToColorEnableNV(
	VkCommandBuffer commandBuffer, 
	VkBool32 coverageToColorEnable) 
{ 
  pfn_vkCmdSetCoverageToColorEnableNV(commandBuffer, coverageToColorEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageToColorLocationNV(
	VkCommandBuffer commandBuffer, 
	uint32_t coverageToColorLocation) 
{ 
  pfn_vkCmdSetCoverageToColorLocationNV(commandBuffer, coverageToColorLocation); 
}
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_fragment_coverage_to_color || VK_EXT_shader_object && VK_NV_fragment_coverage_to_color */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_framebuffer_mixed_samples) || defined(VK_EXT_shader_object) && defined(VK_NV_framebuffer_mixed_samples)
VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageModulationModeNV(
	VkCommandBuffer commandBuffer, 
	VkCoverageModulationModeNV coverageModulationMode) 
{ 
  pfn_vkCmdSetCoverageModulationModeNV(commandBuffer, coverageModulationMode); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageModulationTableEnableNV(
	VkCommandBuffer commandBuffer, 
	VkBool32 coverageModulationTableEnable) 
{ 
  pfn_vkCmdSetCoverageModulationTableEnableNV(commandBuffer, coverageModulationTableEnable); 
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageModulationTableNV(
	VkCommandBuffer commandBuffer, 
	uint32_t coverageModulationTableCount, 
	const float* pCoverageModulationTable) 
{ 
  pfn_vkCmdSetCoverageModulationTableNV(commandBuffer, coverageModulationTableCount, pCoverageModulationTable); 
}
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_framebuffer_mixed_samples || VK_EXT_shader_object && VK_NV_framebuffer_mixed_samples */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_shading_rate_image) || defined(VK_EXT_shader_object) && defined(VK_NV_shading_rate_image)
VKAPI_ATTR void VKAPI_CALL vkCmdSetShadingRateImageEnableNV(
	VkCommandBuffer commandBuffer, 
	VkBool32 shadingRateImageEnable) 
{ 
  pfn_vkCmdSetShadingRateImageEnableNV(commandBuffer, shadingRateImageEnable); 
}
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_shading_rate_image || VK_EXT_shader_object && VK_NV_shading_rate_image */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_representative_fragment_test) || defined(VK_EXT_shader_object) && defined(VK_NV_representative_fragment_test)
VKAPI_ATTR void VKAPI_CALL vkCmdSetRepresentativeFragmentTestEnableNV(
	VkCommandBuffer commandBuffer, 
	VkBool32 representativeFragmentTestEnable) 
{ 
  pfn_vkCmdSetRepresentativeFragmentTestEnableNV(commandBuffer, representativeFragmentTestEnable); 
}
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_representative_fragment_test || VK_EXT_shader_object && VK_NV_representative_fragment_test */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_coverage_reduction_mode) || defined(VK_EXT_shader_object) && defined(VK_NV_coverage_reduction_mode)
VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageReductionModeNV(
	VkCommandBuffer commandBuffer, 
	VkCoverageReductionModeNV coverageReductionMode) 
{ 
  pfn_vkCmdSetCoverageReductionModeNV(commandBuffer, coverageReductionMode); 
}
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_coverage_reduction_mode || VK_EXT_shader_object && VK_NV_coverage_reduction_mode */
#if defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group) || defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)
VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceGroupSurfacePresentModes2EXT(
	VkDevice device, 
	const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, 
	VkDeviceGroupPresentModeFlagsKHR* pModes) 
{ 
  return pfn_vkGetDeviceGroupSurfacePresentModes2EXT(device, pSurfaceInfo, pModes); 
}
#endif /* VK_EXT_full_screen_exclusive && VK_KHR_device_group || VK_EXT_full_screen_exclusive && VK_VERSION_1_1 */
#if defined(VK_EXT_host_image_copy) || defined(VK_EXT_image_compression_control)
VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout2EXT(
	VkDevice device, 
	VkImage image, 
	const VkImageSubresource2KHR* pSubresource, 
	VkSubresourceLayout2KHR* pLayout) 
{ 
  pfn_vkGetImageSubresourceLayout2EXT(device, image, pSubresource, pLayout); 
}
#endif /* VK_EXT_host_image_copy || VK_EXT_image_compression_control */
#if defined(VK_EXT_shader_object) || defined(VK_EXT_vertex_input_dynamic_state)
VKAPI_ATTR void VKAPI_CALL vkCmdSetVertexInputEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t vertexBindingDescriptionCount, 
	const VkVertexInputBindingDescription2EXT* pVertexBindingDescriptions, 
	uint32_t vertexAttributeDescriptionCount, 
	const VkVertexInputAttributeDescription2EXT* pVertexAttributeDescriptions) 
{ 
  pfn_vkCmdSetVertexInputEXT(commandBuffer, vertexBindingDescriptionCount, pVertexBindingDescriptions, vertexAttributeDescriptionCount, pVertexAttributeDescriptions); 
}
#endif /* VK_EXT_shader_object || VK_EXT_vertex_input_dynamic_state */
#if defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor) || defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1) || defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template)
VKAPI_ATTR void VKAPI_CALL vkCmdPushDescriptorSetWithTemplateKHR(
	VkCommandBuffer commandBuffer, 
	VkDescriptorUpdateTemplate descriptorUpdateTemplate, 
	VkPipelineLayout layout, 
	uint32_t set, 
	const void* pData) 
{ 
  pfn_vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, descriptorUpdateTemplate, layout, set, pData); 
}
#endif /* VK_KHR_descriptor_update_template && VK_KHR_push_descriptor || VK_KHR_push_descriptor && VK_VERSION_1_1 || VK_KHR_push_descriptor && VK_KHR_descriptor_update_template */
#if defined(VK_KHR_device_group) && defined(VK_KHR_surface) || defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)
VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceGroupPresentCapabilitiesKHR(
	VkDevice device, 
	VkDeviceGroupPresentCapabilitiesKHR* pDeviceGroupPresentCapabilities) 
{ 
  return pfn_vkGetDeviceGroupPresentCapabilitiesKHR(device, pDeviceGroupPresentCapabilities); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceGroupSurfacePresentModesKHR(
	VkDevice device, 
	VkSurfaceKHR surface, 
	VkDeviceGroupPresentModeFlagsKHR* pModes) 
{ 
  return pfn_vkGetDeviceGroupSurfacePresentModesKHR(device, surface, pModes); 
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDevicePresentRectanglesKHR(
	VkPhysicalDevice physicalDevice, 
	VkSurfaceKHR surface, 
	uint32_t* pRectCount, 
	VkRect2D* pRects) 
{ 
  return pfn_vkGetPhysicalDevicePresentRectanglesKHR(physicalDevice, surface, pRectCount, pRects); 
}
#endif /* VK_KHR_device_group && VK_KHR_surface || VK_KHR_swapchain && VK_VERSION_1_1 */
#if defined(VK_KHR_device_group) && defined(VK_KHR_swapchain) || defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImage2KHR(
	VkDevice device, 
	const VkAcquireNextImageInfoKHR* pAcquireInfo, 
	uint32_t* pImageIndex) 
{ 
  return pfn_vkAcquireNextImage2KHR(device, pAcquireInfo, pImageIndex); 
}
#endif /* VK_KHR_device_group && VK_KHR_swapchain || VK_KHR_swapchain && VK_VERSION_1_1 */
/* NVVK_GENERATE_DECLARE */


/* super load/reset */
void load_VK_EXTENSIONS(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr) {
/* NVVK_GENERATE_LOAD_PROC */
#if defined(VK_AMD_buffer_marker)
  pfn_vkCmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD)getDeviceProcAddr(device, "vkCmdWriteBufferMarkerAMD");
#endif /* VK_AMD_buffer_marker */
#if defined(VK_AMD_display_native_hdr)
  pfn_vkSetLocalDimmingAMD = (PFN_vkSetLocalDimmingAMD)getDeviceProcAddr(device, "vkSetLocalDimmingAMD");
#endif /* VK_AMD_display_native_hdr */
#if defined(VK_AMD_draw_indirect_count)
  pfn_vkCmdDrawIndexedIndirectCountAMD = (PFN_vkCmdDrawIndexedIndirectCountAMD)getDeviceProcAddr(device, "vkCmdDrawIndexedIndirectCountAMD");
  pfn_vkCmdDrawIndirectCountAMD = (PFN_vkCmdDrawIndirectCountAMD)getDeviceProcAddr(device, "vkCmdDrawIndirectCountAMD");
#endif /* VK_AMD_draw_indirect_count */
#if defined(VK_AMD_shader_info)
  pfn_vkGetShaderInfoAMD = (PFN_vkGetShaderInfoAMD)getDeviceProcAddr(device, "vkGetShaderInfoAMD");
#endif /* VK_AMD_shader_info */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
  pfn_vkGetAndroidHardwareBufferPropertiesANDROID = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)getDeviceProcAddr(device, "vkGetAndroidHardwareBufferPropertiesANDROID");
  pfn_vkGetMemoryAndroidHardwareBufferANDROID = (PFN_vkGetMemoryAndroidHardwareBufferANDROID)getDeviceProcAddr(device, "vkGetMemoryAndroidHardwareBufferANDROID");
#endif /* VK_ANDROID_external_memory_android_hardware_buffer */
#if defined(VK_EXT_acquire_drm_display)
  pfn_vkAcquireDrmDisplayEXT = (PFN_vkAcquireDrmDisplayEXT)getInstanceProcAddr(instance, "vkAcquireDrmDisplayEXT");
  pfn_vkGetDrmDisplayEXT = (PFN_vkGetDrmDisplayEXT)getInstanceProcAddr(instance, "vkGetDrmDisplayEXT");
#endif /* VK_EXT_acquire_drm_display */
#if defined(VK_EXT_acquire_xlib_display)
  pfn_vkAcquireXlibDisplayEXT = (PFN_vkAcquireXlibDisplayEXT)getInstanceProcAddr(instance, "vkAcquireXlibDisplayEXT");
  pfn_vkGetRandROutputDisplayEXT = (PFN_vkGetRandROutputDisplayEXT)getInstanceProcAddr(instance, "vkGetRandROutputDisplayEXT");
#endif /* VK_EXT_acquire_xlib_display */
#if defined(VK_EXT_attachment_feedback_loop_dynamic_state)
  pfn_vkCmdSetAttachmentFeedbackLoopEnableEXT = (PFN_vkCmdSetAttachmentFeedbackLoopEnableEXT)getDeviceProcAddr(device, "vkCmdSetAttachmentFeedbackLoopEnableEXT");
#endif /* VK_EXT_attachment_feedback_loop_dynamic_state */
#if defined(VK_EXT_buffer_device_address)
  pfn_vkGetBufferDeviceAddressEXT = (PFN_vkGetBufferDeviceAddressEXT)getDeviceProcAddr(device, "vkGetBufferDeviceAddressEXT");
#endif /* VK_EXT_buffer_device_address */
#if defined(VK_EXT_calibrated_timestamps)
  pfn_vkGetCalibratedTimestampsEXT = (PFN_vkGetCalibratedTimestampsEXT)getDeviceProcAddr(device, "vkGetCalibratedTimestampsEXT");
  pfn_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT = (PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)getInstanceProcAddr(instance, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT");
#endif /* VK_EXT_calibrated_timestamps */
#if defined(VK_EXT_color_write_enable)
  pfn_vkCmdSetColorWriteEnableEXT = (PFN_vkCmdSetColorWriteEnableEXT)getDeviceProcAddr(device, "vkCmdSetColorWriteEnableEXT");
#endif /* VK_EXT_color_write_enable */
#if defined(VK_EXT_conditional_rendering)
  pfn_vkCmdBeginConditionalRenderingEXT = (PFN_vkCmdBeginConditionalRenderingEXT)getDeviceProcAddr(device, "vkCmdBeginConditionalRenderingEXT");
  pfn_vkCmdEndConditionalRenderingEXT = (PFN_vkCmdEndConditionalRenderingEXT)getDeviceProcAddr(device, "vkCmdEndConditionalRenderingEXT");
#endif /* VK_EXT_conditional_rendering */
#if defined(VK_EXT_debug_marker)
  pfn_vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)getDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
  pfn_vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)getDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
  pfn_vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)getDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
  pfn_vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)getDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
  pfn_vkDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)getDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
#endif /* VK_EXT_debug_marker */
#if defined(VK_EXT_debug_report)
  pfn_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)getInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
  pfn_vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)getInstanceProcAddr(instance, "vkDebugReportMessageEXT");
  pfn_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)getInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
#endif /* VK_EXT_debug_report */
#if defined(VK_EXT_debug_utils)
  pfn_vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)getInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
  pfn_vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)getInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
  pfn_vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)getInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
  pfn_vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)getInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  pfn_vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)getInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  pfn_vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)getInstanceProcAddr(instance, "vkQueueBeginDebugUtilsLabelEXT");
  pfn_vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)getInstanceProcAddr(instance, "vkQueueEndDebugUtilsLabelEXT");
  pfn_vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)getInstanceProcAddr(instance, "vkQueueInsertDebugUtilsLabelEXT");
  pfn_vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)getInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
  pfn_vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)getInstanceProcAddr(instance, "vkSetDebugUtilsObjectTagEXT");
  pfn_vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)getInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");
#endif /* VK_EXT_debug_utils */
#if defined(VK_EXT_depth_bias_control)
  pfn_vkCmdSetDepthBias2EXT = (PFN_vkCmdSetDepthBias2EXT)getDeviceProcAddr(device, "vkCmdSetDepthBias2EXT");
#endif /* VK_EXT_depth_bias_control */
#if defined(VK_EXT_descriptor_buffer)
  pfn_vkCmdBindDescriptorBufferEmbeddedSamplersEXT = (PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT)getDeviceProcAddr(device, "vkCmdBindDescriptorBufferEmbeddedSamplersEXT");
  pfn_vkCmdBindDescriptorBuffersEXT = (PFN_vkCmdBindDescriptorBuffersEXT)getDeviceProcAddr(device, "vkCmdBindDescriptorBuffersEXT");
  pfn_vkCmdSetDescriptorBufferOffsetsEXT = (PFN_vkCmdSetDescriptorBufferOffsetsEXT)getDeviceProcAddr(device, "vkCmdSetDescriptorBufferOffsetsEXT");
  pfn_vkGetBufferOpaqueCaptureDescriptorDataEXT = (PFN_vkGetBufferOpaqueCaptureDescriptorDataEXT)getDeviceProcAddr(device, "vkGetBufferOpaqueCaptureDescriptorDataEXT");
  pfn_vkGetDescriptorEXT = (PFN_vkGetDescriptorEXT)getDeviceProcAddr(device, "vkGetDescriptorEXT");
  pfn_vkGetDescriptorSetLayoutBindingOffsetEXT = (PFN_vkGetDescriptorSetLayoutBindingOffsetEXT)getDeviceProcAddr(device, "vkGetDescriptorSetLayoutBindingOffsetEXT");
  pfn_vkGetDescriptorSetLayoutSizeEXT = (PFN_vkGetDescriptorSetLayoutSizeEXT)getDeviceProcAddr(device, "vkGetDescriptorSetLayoutSizeEXT");
  pfn_vkGetImageOpaqueCaptureDescriptorDataEXT = (PFN_vkGetImageOpaqueCaptureDescriptorDataEXT)getDeviceProcAddr(device, "vkGetImageOpaqueCaptureDescriptorDataEXT");
  pfn_vkGetImageViewOpaqueCaptureDescriptorDataEXT = (PFN_vkGetImageViewOpaqueCaptureDescriptorDataEXT)getDeviceProcAddr(device, "vkGetImageViewOpaqueCaptureDescriptorDataEXT");
  pfn_vkGetSamplerOpaqueCaptureDescriptorDataEXT = (PFN_vkGetSamplerOpaqueCaptureDescriptorDataEXT)getDeviceProcAddr(device, "vkGetSamplerOpaqueCaptureDescriptorDataEXT");
#endif /* VK_EXT_descriptor_buffer */
#if defined(VK_EXT_descriptor_buffer) && (defined(VK_KHR_acceleration_structure) || defined(VK_NV_ray_tracing))
  pfn_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT = (PFN_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT)getDeviceProcAddr(device, "vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT");
#endif /* VK_EXT_descriptor_buffer && (VK_KHR_acceleration_structure || VK_NV_ray_tracing) */
#if defined(VK_EXT_device_fault)
  pfn_vkGetDeviceFaultInfoEXT = (PFN_vkGetDeviceFaultInfoEXT)getDeviceProcAddr(device, "vkGetDeviceFaultInfoEXT");
#endif /* VK_EXT_device_fault */
#if defined(VK_EXT_direct_mode_display)
  pfn_vkReleaseDisplayEXT = (PFN_vkReleaseDisplayEXT)getInstanceProcAddr(instance, "vkReleaseDisplayEXT");
#endif /* VK_EXT_direct_mode_display */
#if defined(VK_EXT_directfb_surface)
  pfn_vkCreateDirectFBSurfaceEXT = (PFN_vkCreateDirectFBSurfaceEXT)getInstanceProcAddr(instance, "vkCreateDirectFBSurfaceEXT");
  pfn_vkGetPhysicalDeviceDirectFBPresentationSupportEXT = (PFN_vkGetPhysicalDeviceDirectFBPresentationSupportEXT)getInstanceProcAddr(instance, "vkGetPhysicalDeviceDirectFBPresentationSupportEXT");
#endif /* VK_EXT_directfb_surface */
#if defined(VK_EXT_discard_rectangles)
  pfn_vkCmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT)getDeviceProcAddr(device, "vkCmdSetDiscardRectangleEXT");
#endif /* VK_EXT_discard_rectangles */
#if defined(VK_EXT_discard_rectangles) && VK_EXT_DISCARD_RECTANGLES_SPEC_VERSION >= 2
  pfn_vkCmdSetDiscardRectangleEnableEXT = (PFN_vkCmdSetDiscardRectangleEnableEXT)getDeviceProcAddr(device, "vkCmdSetDiscardRectangleEnableEXT");
  pfn_vkCmdSetDiscardRectangleModeEXT = (PFN_vkCmdSetDiscardRectangleModeEXT)getDeviceProcAddr(device, "vkCmdSetDiscardRectangleModeEXT");
#endif /* VK_EXT_discard_rectangles && VK_EXT_DISCARD_RECTANGLES_SPEC_VERSION >= 2 */
#if defined(VK_EXT_display_control)
  pfn_vkDisplayPowerControlEXT = (PFN_vkDisplayPowerControlEXT)getDeviceProcAddr(device, "vkDisplayPowerControlEXT");
  pfn_vkGetSwapchainCounterEXT = (PFN_vkGetSwapchainCounterEXT)getDeviceProcAddr(device, "vkGetSwapchainCounterEXT");
  pfn_vkRegisterDeviceEventEXT = (PFN_vkRegisterDeviceEventEXT)getDeviceProcAddr(device, "vkRegisterDeviceEventEXT");
  pfn_vkRegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT)getDeviceProcAddr(device, "vkRegisterDisplayEventEXT");
#endif /* VK_EXT_display_control */
#if defined(VK_EXT_display_surface_counter)
  pfn_vkGetPhysicalDeviceSurfaceCapabilities2EXT = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT)getInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilities2EXT");
#endif /* VK_EXT_display_surface_counter */
#if defined(VK_EXT_external_memory_host)
  pfn_vkGetMemoryHostPointerPropertiesEXT = (PFN_vkGetMemoryHostPointerPropertiesEXT)getDeviceProcAddr(device, "vkGetMemoryHostPointerPropertiesEXT");
#endif /* VK_EXT_external_memory_host */
#if defined(VK_EXT_full_screen_exclusive)
  pfn_vkAcquireFullScreenExclusiveModeEXT = (PFN_vkAcquireFullScreenExclusiveModeEXT)getDeviceProcAddr(device, "vkAcquireFullScreenExclusiveModeEXT");
  pfn_vkGetPhysicalDeviceSurfacePresentModes2EXT = (PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT)getInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModes2EXT");
  pfn_vkReleaseFullScreenExclusiveModeEXT = (PFN_vkReleaseFullScreenExclusiveModeEXT)getDeviceProcAddr(device, "vkReleaseFullScreenExclusiveModeEXT");
#endif /* VK_EXT_full_screen_exclusive */
#if defined(VK_EXT_hdr_metadata)
  pfn_vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)getDeviceProcAddr(device, "vkSetHdrMetadataEXT");
#endif /* VK_EXT_hdr_metadata */
#if defined(VK_EXT_headless_surface)
  pfn_vkCreateHeadlessSurfaceEXT = (PFN_vkCreateHeadlessSurfaceEXT)getInstanceProcAddr(instance, "vkCreateHeadlessSurfaceEXT");
#endif /* VK_EXT_headless_surface */
#if defined(VK_EXT_host_image_copy)
  pfn_vkCopyImageToImageEXT = (PFN_vkCopyImageToImageEXT)getDeviceProcAddr(device, "vkCopyImageToImageEXT");
  pfn_vkCopyImageToMemoryEXT = (PFN_vkCopyImageToMemoryEXT)getDeviceProcAddr(device, "vkCopyImageToMemoryEXT");
  pfn_vkCopyMemoryToImageEXT = (PFN_vkCopyMemoryToImageEXT)getDeviceProcAddr(device, "vkCopyMemoryToImageEXT");
  pfn_vkTransitionImageLayoutEXT = (PFN_vkTransitionImageLayoutEXT)getDeviceProcAddr(device, "vkTransitionImageLayoutEXT");
#endif /* VK_EXT_host_image_copy */
#if defined(VK_EXT_host_query_reset)
  pfn_vkResetQueryPoolEXT = (PFN_vkResetQueryPoolEXT)getDeviceProcAddr(device, "vkResetQueryPoolEXT");
#endif /* VK_EXT_host_query_reset */
#if defined(VK_EXT_image_drm_format_modifier)
  pfn_vkGetImageDrmFormatModifierPropertiesEXT = (PFN_vkGetImageDrmFormatModifierPropertiesEXT)getDeviceProcAddr(device, "vkGetImageDrmFormatModifierPropertiesEXT");
#endif /* VK_EXT_image_drm_format_modifier */
#if defined(VK_EXT_line_rasterization)
  pfn_vkCmdSetLineStippleEXT = (PFN_vkCmdSetLineStippleEXT)getDeviceProcAddr(device, "vkCmdSetLineStippleEXT");
#endif /* VK_EXT_line_rasterization */
#if defined(VK_EXT_mesh_shader)
  pfn_vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)getDeviceProcAddr(device, "vkCmdDrawMeshTasksEXT");
  pfn_vkCmdDrawMeshTasksIndirectCountEXT = (PFN_vkCmdDrawMeshTasksIndirectCountEXT)getDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectCountEXT");
  pfn_vkCmdDrawMeshTasksIndirectEXT = (PFN_vkCmdDrawMeshTasksIndirectEXT)getDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectEXT");
#endif /* VK_EXT_mesh_shader */
#if defined(VK_EXT_metal_objects)
  pfn_vkExportMetalObjectsEXT = (PFN_vkExportMetalObjectsEXT)getDeviceProcAddr(device, "vkExportMetalObjectsEXT");
#endif /* VK_EXT_metal_objects */
#if defined(VK_EXT_metal_surface)
  pfn_vkCreateMetalSurfaceEXT = (PFN_vkCreateMetalSurfaceEXT)getInstanceProcAddr(instance, "vkCreateMetalSurfaceEXT");
#endif /* VK_EXT_metal_surface */
#if defined(VK_EXT_multi_draw)
  pfn_vkCmdDrawMultiEXT = (PFN_vkCmdDrawMultiEXT)getDeviceProcAddr(device, "vkCmdDrawMultiEXT");
  pfn_vkCmdDrawMultiIndexedEXT = (PFN_vkCmdDrawMultiIndexedEXT)getDeviceProcAddr(device, "vkCmdDrawMultiIndexedEXT");
#endif /* VK_EXT_multi_draw */
#if defined(VK_EXT_opacity_micromap)
  pfn_vkBuildMicromapsEXT = (PFN_vkBuildMicromapsEXT)getDeviceProcAddr(device, "vkBuildMicromapsEXT");
  pfn_vkCmdBuildMicromapsEXT = (PFN_vkCmdBuildMicromapsEXT)getDeviceProcAddr(device, "vkCmdBuildMicromapsEXT");
  pfn_vkCmdCopyMemoryToMicromapEXT = (PFN_vkCmdCopyMemoryToMicromapEXT)getDeviceProcAddr(device, "vkCmdCopyMemoryToMicromapEXT");
  pfn_vkCmdCopyMicromapEXT = (PFN_vkCmdCopyMicromapEXT)getDeviceProcAddr(device, "vkCmdCopyMicromapEXT");
  pfn_vkCmdCopyMicromapToMemoryEXT = (PFN_vkCmdCopyMicromapToMemoryEXT)getDeviceProcAddr(device, "vkCmdCopyMicromapToMemoryEXT");
  pfn_vkCmdWriteMicromapsPropertiesEXT = (PFN_vkCmdWriteMicromapsPropertiesEXT)getDeviceProcAddr(device, "vkCmdWriteMicromapsPropertiesEXT");
  pfn_vkCopyMemoryToMicromapEXT = (PFN_vkCopyMemoryToMicromapEXT)getDeviceProcAddr(device, "vkCopyMemoryToMicromapEXT");
  pfn_vkCopyMicromapEXT = (PFN_vkCopyMicromapEXT)getDeviceProcAddr(device, "vkCopyMicromapEXT");
  pfn_vkCopyMicromapToMemoryEXT = (PFN_vkCopyMicromapToMemoryEXT)getDeviceProcAddr(device, "vkCopyMicromapToMemoryEXT");
  pfn_vkCreateMicromapEXT = (PFN_vkCreateMicromapEXT)getDeviceProcAddr(device, "vkCreateMicromapEXT");
  pfn_vkDestroyMicromapEXT = (PFN_vkDestroyMicromapEXT)getDeviceProcAddr(device, "vkDestroyMicromapEXT");
  pfn_vkGetDeviceMicromapCompatibilityEXT = (PFN_vkGetDeviceMicromapCompatibilityEXT)getDeviceProcAddr(device, "vkGetDeviceMicromapCompatibilityEXT");
  pfn_vkGetMicromapBuildSizesEXT = (PFN_vkGetMicromapBuildSizesEXT)getDeviceProcAddr(device, "vkGetMicromapBuildSizesEXT");
  pfn_vkWriteMicromapsPropertiesEXT = (PFN_vkWriteMicromapsPropertiesEXT)getDeviceProcAddr(device, "vkWriteMicromapsPropertiesEXT");
#endif /* VK_EXT_opacity_micromap */
#if defined(VK_EXT_pageable_device_local_memory)
  pfn_vkSetDeviceMemoryPriorityEXT = (PFN_vkSetDeviceMemoryPriorityEXT)getDeviceProcAddr(device, "vkSetDeviceMemoryPriorityEXT");
#endif /* VK_EXT_pageable_device_local_memory */
#if defined(VK_EXT_pipeline_properties)
  pfn_vkGetPipelinePropertiesEXT = (PFN_vkGetPipelinePropertiesEXT)getDeviceProcAddr(device, "vkGetPipelinePropertiesEXT");
#endif /* VK_EXT_pipeline_properties */
#if defined(VK_EXT_private_data)
  pfn_vkCreatePrivateDataSlotEXT = (PFN_vkCreatePrivateDataSlotEXT)getDeviceProcAddr(device, "vkCreatePrivateDataSlotEXT");
  pfn_vkDestroyPrivateDataSlotEXT = (PFN_vkDestroyPrivateDataSlotEXT)getDeviceProcAddr(device, "vkDestroyPrivateDataSlotEXT");
  pfn_vkGetPrivateDataEXT = (PFN_vkGetPrivateDataEXT)getDeviceProcAddr(device, "vkGetPrivateDataEXT");
  pfn_vkSetPrivateDataEXT = (PFN_vkSetPrivateDataEXT)getDeviceProcAddr(device, "vkSetPrivateDataEXT");
#endif /* VK_EXT_private_data */
#if defined(VK_EXT_sample_locations)
  pfn_vkCmdSetSampleLocationsEXT = (PFN_vkCmdSetSampleLocationsEXT)getDeviceProcAddr(device, "vkCmdSetSampleLocationsEXT");
  pfn_vkGetPhysicalDeviceMultisamplePropertiesEXT = (PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT)getInstanceProcAddr(instance, "vkGetPhysicalDeviceMultisamplePropertiesEXT");
#endif /* VK_EXT_sample_locations */
#if defined(VK_EXT_shader_module_identifier)
  pfn_vkGetShaderModuleCreateInfoIdentifierEXT = (PFN_vkGetShaderModuleCreateInfoIdentifierEXT)getDeviceProcAddr(device, "vkGetShaderModuleCreateInfoIdentifierEXT");
  pfn_vkGetShaderModuleIdentifierEXT = (PFN_vkGetShaderModuleIdentifierEXT)getDeviceProcAddr(device, "vkGetShaderModuleIdentifierEXT");
#endif /* VK_EXT_shader_module_identifier */
#if defined(VK_EXT_shader_object)
  pfn_vkCmdBindShadersEXT = (PFN_vkCmdBindShadersEXT)getDeviceProcAddr(device, "vkCmdBindShadersEXT");
  pfn_vkCreateShadersEXT = (PFN_vkCreateShadersEXT)getDeviceProcAddr(device, "vkCreateShadersEXT");
  pfn_vkDestroyShaderEXT = (PFN_vkDestroyShaderEXT)getDeviceProcAddr(device, "vkDestroyShaderEXT");
  pfn_vkGetShaderBinaryDataEXT = (PFN_vkGetShaderBinaryDataEXT)getDeviceProcAddr(device, "vkGetShaderBinaryDataEXT");
#endif /* VK_EXT_shader_object */
#if defined(VK_EXT_swapchain_maintenance1)
  pfn_vkReleaseSwapchainImagesEXT = (PFN_vkReleaseSwapchainImagesEXT)getDeviceProcAddr(device, "vkReleaseSwapchainImagesEXT");
#endif /* VK_EXT_swapchain_maintenance1 */
#if defined(VK_EXT_tooling_info)
  pfn_vkGetPhysicalDeviceToolPropertiesEXT = (PFN_vkGetPhysicalDeviceToolPropertiesEXT)getInstanceProcAddr(instance, "vkGetPhysicalDeviceToolPropertiesEXT");
#endif /* VK_EXT_tooling_info */
#if defined(VK_EXT_transform_feedback)
  pfn_vkCmdBeginQueryIndexedEXT = (PFN_vkCmdBeginQueryIndexedEXT)getDeviceProcAddr(device, "vkCmdBeginQueryIndexedEXT");
  pfn_vkCmdBeginTransformFeedbackEXT = (PFN_vkCmdBeginTransformFeedbackEXT)getDeviceProcAddr(device, "vkCmdBeginTransformFeedbackEXT");
  pfn_vkCmdBindTransformFeedbackBuffersEXT = (PFN_vkCmdBindTransformFeedbackBuffersEXT)getDeviceProcAddr(device, "vkCmdBindTransformFeedbackBuffersEXT");
  pfn_vkCmdDrawIndirectByteCountEXT = (PFN_vkCmdDrawIndirectByteCountEXT)getDeviceProcAddr(device, "vkCmdDrawIndirectByteCountEXT");
  pfn_vkCmdEndQueryIndexedEXT = (PFN_vkCmdEndQueryIndexedEXT)getDeviceProcAddr(device, "vkCmdEndQueryIndexedEXT");
  pfn_vkCmdEndTransformFeedbackEXT = (PFN_vkCmdEndTransformFeedbackEXT)getDeviceProcAddr(device, "vkCmdEndTransformFeedbackEXT");
#endif /* VK_EXT_transform_feedback */
#if defined(VK_EXT_validation_cache)
  pfn_vkCreateValidationCacheEXT = (PFN_vkCreateValidationCacheEXT)getDeviceProcAddr(device, "vkCreateValidationCacheEXT");
  pfn_vkDestroyValidationCacheEXT = (PFN_vkDestroyValidationCacheEXT)getDeviceProcAddr(device, "vkDestroyValidationCacheEXT");
  pfn_vkGetValidationCacheDataEXT = (PFN_vkGetValidationCacheDataEXT)getDeviceProcAddr(device, "vkGetValidationCacheDataEXT");
  pfn_vkMergeValidationCachesEXT = (PFN_vkMergeValidationCachesEXT)getDeviceProcAddr(device, "vkMergeValidationCachesEXT");
#endif /* VK_EXT_validation_cache */
#if defined(VK_FUCHSIA_buffer_collection)
  pfn_vkCreateBufferCollectionFUCHSIA = (PFN_vkCreateBufferCollectionFUCHSIA)getDeviceProcAddr(device, "vkCreateBufferCollectionFUCHSIA");
  pfn_vkDestroyBufferCollectionFUCHSIA = (PFN_vkDestroyBufferCollectionFUCHSIA)getDeviceProcAddr(device, "vkDestroyBufferCollectionFUCHSIA");
  pfn_vkGetBufferCollectionPropertiesFUCHSIA = (PFN_vkGetBufferCollectionPropertiesFUCHSIA)getDeviceProcAddr(device, "vkGetBufferCollectionPropertiesFUCHSIA");
  pfn_vkSetBufferCollectionBufferConstraintsFUCHSIA = (PFN_vkSetBufferCollectionBufferConstraintsFUCHSIA)getDeviceProcAddr(device, "vkSetBufferCollectionBufferConstraintsFUCHSIA");
  pfn_vkSetBufferCollectionImageConstraintsFUCHSIA = (PFN_vkSetBufferCollectionImageConstraintsFUCHSIA)getDeviceProcAddr(device, "vkSetBufferCollectionImageConstraintsFUCHSIA");
#endif /* VK_FUCHSIA_buffer_collection */
#if defined(VK_FUCHSIA_external_memory)
  pfn_vkGetMemoryZirconHandleFUCHSIA = (PFN_vkGetMemoryZirconHandleFUCHSIA)getDeviceProcAddr(device, "vkGetMemoryZirconHandleFUCHSIA");
  pfn_vkGetMemoryZirconHandlePropertiesFUCHSIA = (PFN_vkGetMemoryZirconHandlePropertiesFUCHSIA)getDeviceProcAddr(device, "vkGetMemoryZirconHandlePropertiesFUCHSIA");
#endif /* VK_FUCHSIA_external_memory */
#if defined(VK_FUCHSIA_external_semaphore)
  pfn_vkGetSemaphoreZirconHandleFUCHSIA = (PFN_vkGetSemaphoreZirconHandleFUCHSIA)getDeviceProcAddr(device, "vkGetSemaphoreZirconHandleFUCHSIA");
  pfn_vkImportSemaphoreZirconHandleFUCHSIA = (PFN_vkImportSemaphoreZirconHandleFUCHSIA)getDeviceProcAddr(device, "vkImportSemaphoreZirconHandleFUCHSIA");
#endif /* VK_FUCHSIA_external_semaphore */
#if defined(VK_FUCHSIA_imagepipe_surface)
  pfn_vkCreateImagePipeSurfaceFUCHSIA = (PFN_vkCreateImagePipeSurfaceFUCHSIA)getInstanceProcAddr(instance, "vkCreateImagePipeSurfaceFUCHSIA");
#endif /* VK_FUCHSIA_imagepipe_surface */
#if defined(VK_GGP_stream_descriptor_surface)
  pfn_vkCreateStreamDescriptorSurfaceGGP = (PFN_vkCreateStreamDescriptorSurfaceGGP)getInstanceProcAddr(instance, "vkCreateStreamDescriptorSurfaceGGP");
#endif /* VK_GGP_stream_descriptor_surface */
#if defined(VK_GOOGLE_display_timing)
  pfn_vkGetPastPresentationTimingGOOGLE = (PFN_vkGetPastPresentationTimingGOOGLE)getDeviceProcAddr(device, "vkGetPastPresentationTimingGOOGLE");
  pfn_vkGetRefreshCycleDurationGOOGLE = (PFN_vkGetRefreshCycleDurationGOOGLE)getDeviceProcAddr(device, "vkGetRefreshCycleDurationGOOGLE");
#endif /* VK_GOOGLE_display_timing */
#if defined(VK_HUAWEI_cluster_culling_shader)
  pfn_vkCmdDrawClusterHUAWEI = (PFN_vkCmdDrawClusterHUAWEI)getDeviceProcAddr(device, "vkCmdDrawClusterHUAWEI");
  pfn_vkCmdDrawClusterIndirectHUAWEI = (PFN_vkCmdDrawClusterIndirectHUAWEI)getDeviceProcAddr(device, "vkCmdDrawClusterIndirectHUAWEI");
#endif /* VK_HUAWEI_cluster_culling_shader */
#if defined(VK_HUAWEI_invocation_mask)
  pfn_vkCmdBindInvocationMaskHUAWEI = (PFN_vkCmdBindInvocationMaskHUAWEI)getDeviceProcAddr(device, "vkCmdBindInvocationMaskHUAWEI");
#endif /* VK_HUAWEI_invocation_mask */
#if defined(VK_HUAWEI_subpass_shading)
  pfn_vkCmdSubpassShadingHUAWEI = (PFN_vkCmdSubpassShadingHUAWEI)getDeviceProcAddr(device, "vkCmdSubpassShadingHUAWEI");
  pfn_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI = (PFN_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI)getDeviceProcAddr(device, "vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI");
#endif /* VK_HUAWEI_subpass_shading */
#if defined(VK_INTEL_performance_query)
  pfn_vkAcquirePerformanceConfigurationINTEL = (PFN_vkAcquirePerformanceConfigurationINTEL)getDeviceProcAddr(device, "vkAcquirePerformanceConfigurationINTEL");
  pfn_vkCmdSetPerformanceMarkerINTEL = (PFN_vkCmdSetPerformanceMarkerINTEL)getDeviceProcAddr(device, "vkCmdSetPerformanceMarkerINTEL");
  pfn_vkCmdSetPerformanceOverrideINTEL = (PFN_vkCmdSetPerformanceOverrideINTEL)getDeviceProcAddr(device, "vkCmdSetPerformanceOverrideINTEL");
  pfn_vkCmdSetPerformanceStreamMarkerINTEL = (PFN_vkCmdSetPerformanceStreamMarkerINTEL)getDeviceProcAddr(device, "vkCmdSetPerformanceStreamMarkerINTEL");
  pfn_vkGetPerformanceParameterINTEL = (PFN_vkGetPerformanceParameterINTEL)getDeviceProcAddr(device, "vkGetPerformanceParameterINTEL");
  pfn_vkInitializePerformanceApiINTEL = (PFN_vkInitializePerformanceApiINTEL)getDeviceProcAddr(device, "vkInitializePerformanceApiINTEL");
  pfn_vkQueueSetPerformanceConfigurationINTEL = (PFN_vkQueueSetPerformanceConfigurationINTEL)getDeviceProcAddr(device, "vkQueueSetPerformanceConfigurationINTEL");
  pfn_vkReleasePerformanceConfigurationINTEL = (PFN_vkReleasePerformanceConfigurationINTEL)getDeviceProcAddr(device, "vkReleasePerformanceConfigurationINTEL");
  pfn_vkUninitializePerformanceApiINTEL = (PFN_vkUninitializePerformanceApiINTEL)getDeviceProcAddr(device, "vkUninitializePerformanceApiINTEL");
#endif /* VK_INTEL_performance_query */
#if defined(VK_KHR_acceleration_structure)
  pfn_vkBuildAccelerationStructuresKHR = (PFN_vkBuildAccelerationStructuresKHR)getDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR");
  pfn_vkCmdBuildAccelerationStructuresIndirectKHR = (PFN_vkCmdBuildAccelerationStructuresIndirectKHR)getDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresIndirectKHR");
  pfn_vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)getDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
  pfn_vkCmdCopyAccelerationStructureKHR = (PFN_vkCmdCopyAccelerationStructureKHR)getDeviceProcAddr(device, "vkCmdCopyAccelerationStructureKHR");
  pfn_vkCmdCopyAccelerationStructureToMemoryKHR = (PFN_vkCmdCopyAccelerationStructureToMemoryKHR)getDeviceProcAddr(device, "vkCmdCopyAccelerationStructureToMemoryKHR");
  pfn_vkCmdCopyMemoryToAccelerationStructureKHR = (PFN_vkCmdCopyMemoryToAccelerationStructureKHR)getDeviceProcAddr(device, "vkCmdCopyMemoryToAccelerationStructureKHR");
  pfn_vkCmdWriteAccelerationStructuresPropertiesKHR = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)getDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesKHR");
  pfn_vkCopyAccelerationStructureKHR = (PFN_vkCopyAccelerationStructureKHR)getDeviceProcAddr(device, "vkCopyAccelerationStructureKHR");
  pfn_vkCopyAccelerationStructureToMemoryKHR = (PFN_vkCopyAccelerationStructureToMemoryKHR)getDeviceProcAddr(device, "vkCopyAccelerationStructureToMemoryKHR");
  pfn_vkCopyMemoryToAccelerationStructureKHR = (PFN_vkCopyMemoryToAccelerationStructureKHR)getDeviceProcAddr(device, "vkCopyMemoryToAccelerationStructureKHR");
  pfn_vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)getDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
  pfn_vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)getDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
  pfn_vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)getDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
  pfn_vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)getDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
  pfn_vkGetDeviceAccelerationStructureCompatibilityKHR = (PFN_vkGetDeviceAccelerationStructureCompatibilityKHR)getDeviceProcAddr(device, "vkGetDeviceAccelerationStructureCompatibilityKHR");
  pfn_vkWriteAccelerationStructuresPropertiesKHR = (PFN_vkWriteAccelerationStructuresPropertiesKHR)getDeviceProcAddr(device, "vkWriteAccelerationStructuresPropertiesKHR");
#endif /* VK_KHR_acceleration_structure */
#if defined(VK_KHR_android_surface)
  pfn_vkCreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)getInstanceProcAddr(instance, "vkCreateAndroidSurfaceKHR");
#endif /* VK_KHR_android_surface */
#if defined(VK_KHR_bind_memory2)
  pfn_vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)getDeviceProcAddr(device, "vkBindBufferMemory2KHR");
  pfn_vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR)getDeviceProcAddr(device, "vkBindImageMemory2KHR");
#endif /* VK_KHR_bind_memory2 */
#if defined(VK_KHR_buffer_device_address)
  pfn_vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)getDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR");
  pfn_vkGetBufferOpaqueCaptureAddressKHR = (PFN_vkGetBufferOpaqueCaptureAddressKHR)getDeviceProcAddr(device, "vkGetBufferOpaqueCaptureAddressKHR");
  pfn_vkGetDeviceMemoryOpaqueCaptureAddressKHR = (PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR)getDeviceProcAddr(device, "vkGetDeviceMemoryOpaqueCaptureAddressKHR");
#endif /* VK_KHR_buffer_device_address */
#if defined(VK_KHR_cooperative_matrix)
  pfn_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR = (PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR");
#endif /* VK_KHR_cooperative_matrix */
#if defined(VK_KHR_copy_commands2)
  pfn_vkCmdBlitImage2KHR = (PFN_vkCmdBlitImage2KHR)getDeviceProcAddr(device, "vkCmdBlitImage2KHR");
  pfn_vkCmdCopyBuffer2KHR = (PFN_vkCmdCopyBuffer2KHR)getDeviceProcAddr(device, "vkCmdCopyBuffer2KHR");
  pfn_vkCmdCopyBufferToImage2KHR = (PFN_vkCmdCopyBufferToImage2KHR)getDeviceProcAddr(device, "vkCmdCopyBufferToImage2KHR");
  pfn_vkCmdCopyImage2KHR = (PFN_vkCmdCopyImage2KHR)getDeviceProcAddr(device, "vkCmdCopyImage2KHR");
  pfn_vkCmdCopyImageToBuffer2KHR = (PFN_vkCmdCopyImageToBuffer2KHR)getDeviceProcAddr(device, "vkCmdCopyImageToBuffer2KHR");
  pfn_vkCmdResolveImage2KHR = (PFN_vkCmdResolveImage2KHR)getDeviceProcAddr(device, "vkCmdResolveImage2KHR");
#endif /* VK_KHR_copy_commands2 */
#if defined(VK_KHR_create_renderpass2)
  pfn_vkCmdBeginRenderPass2KHR = (PFN_vkCmdBeginRenderPass2KHR)getDeviceProcAddr(device, "vkCmdBeginRenderPass2KHR");
  pfn_vkCmdEndRenderPass2KHR = (PFN_vkCmdEndRenderPass2KHR)getDeviceProcAddr(device, "vkCmdEndRenderPass2KHR");
  pfn_vkCmdNextSubpass2KHR = (PFN_vkCmdNextSubpass2KHR)getDeviceProcAddr(device, "vkCmdNextSubpass2KHR");
  pfn_vkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR)getDeviceProcAddr(device, "vkCreateRenderPass2KHR");
#endif /* VK_KHR_create_renderpass2 */
#if defined(VK_KHR_deferred_host_operations)
  pfn_vkCreateDeferredOperationKHR = (PFN_vkCreateDeferredOperationKHR)getDeviceProcAddr(device, "vkCreateDeferredOperationKHR");
  pfn_vkDeferredOperationJoinKHR = (PFN_vkDeferredOperationJoinKHR)getDeviceProcAddr(device, "vkDeferredOperationJoinKHR");
  pfn_vkDestroyDeferredOperationKHR = (PFN_vkDestroyDeferredOperationKHR)getDeviceProcAddr(device, "vkDestroyDeferredOperationKHR");
  pfn_vkGetDeferredOperationMaxConcurrencyKHR = (PFN_vkGetDeferredOperationMaxConcurrencyKHR)getDeviceProcAddr(device, "vkGetDeferredOperationMaxConcurrencyKHR");
  pfn_vkGetDeferredOperationResultKHR = (PFN_vkGetDeferredOperationResultKHR)getDeviceProcAddr(device, "vkGetDeferredOperationResultKHR");
#endif /* VK_KHR_deferred_host_operations */
#if defined(VK_KHR_descriptor_update_template)
  pfn_vkCreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR)getDeviceProcAddr(device, "vkCreateDescriptorUpdateTemplateKHR");
  pfn_vkDestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR)getDeviceProcAddr(device, "vkDestroyDescriptorUpdateTemplateKHR");
  pfn_vkUpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR)getDeviceProcAddr(device, "vkUpdateDescriptorSetWithTemplateKHR");
#endif /* VK_KHR_descriptor_update_template */
#if defined(VK_KHR_device_group)
  pfn_vkCmdDispatchBaseKHR = (PFN_vkCmdDispatchBaseKHR)getDeviceProcAddr(device, "vkCmdDispatchBaseKHR");
  pfn_vkCmdSetDeviceMaskKHR = (PFN_vkCmdSetDeviceMaskKHR)getDeviceProcAddr(device, "vkCmdSetDeviceMaskKHR");
  pfn_vkGetDeviceGroupPeerMemoryFeaturesKHR = (PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR)getDeviceProcAddr(device, "vkGetDeviceGroupPeerMemoryFeaturesKHR");
#endif /* VK_KHR_device_group */
#if defined(VK_KHR_device_group_creation)
  pfn_vkEnumeratePhysicalDeviceGroupsKHR = (PFN_vkEnumeratePhysicalDeviceGroupsKHR)getInstanceProcAddr(instance, "vkEnumeratePhysicalDeviceGroupsKHR");
#endif /* VK_KHR_device_group_creation */
#if defined(VK_KHR_draw_indirect_count)
  pfn_vkCmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR)getDeviceProcAddr(device, "vkCmdDrawIndexedIndirectCountKHR");
  pfn_vkCmdDrawIndirectCountKHR = (PFN_vkCmdDrawIndirectCountKHR)getDeviceProcAddr(device, "vkCmdDrawIndirectCountKHR");
#endif /* VK_KHR_draw_indirect_count */
#if defined(VK_KHR_dynamic_rendering)
  pfn_vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)getDeviceProcAddr(device, "vkCmdBeginRenderingKHR");
  pfn_vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)getDeviceProcAddr(device, "vkCmdEndRenderingKHR");
#endif /* VK_KHR_dynamic_rendering */
#if defined(VK_KHR_external_fence_capabilities)
  pfn_vkGetPhysicalDeviceExternalFencePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceExternalFencePropertiesKHR");
#endif /* VK_KHR_external_fence_capabilities */
#if defined(VK_KHR_external_fence_fd)
  pfn_vkGetFenceFdKHR = (PFN_vkGetFenceFdKHR)getDeviceProcAddr(device, "vkGetFenceFdKHR");
  pfn_vkImportFenceFdKHR = (PFN_vkImportFenceFdKHR)getDeviceProcAddr(device, "vkImportFenceFdKHR");
#endif /* VK_KHR_external_fence_fd */
#if defined(VK_KHR_external_fence_win32)
  pfn_vkGetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR)getDeviceProcAddr(device, "vkGetFenceWin32HandleKHR");
  pfn_vkImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR)getDeviceProcAddr(device, "vkImportFenceWin32HandleKHR");
#endif /* VK_KHR_external_fence_win32 */
#if defined(VK_KHR_external_memory_capabilities)
  pfn_vkGetPhysicalDeviceExternalBufferPropertiesKHR = (PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceExternalBufferPropertiesKHR");
#endif /* VK_KHR_external_memory_capabilities */
#if defined(VK_KHR_external_memory_fd)
  pfn_vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)getDeviceProcAddr(device, "vkGetMemoryFdKHR");
  pfn_vkGetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR)getDeviceProcAddr(device, "vkGetMemoryFdPropertiesKHR");
#endif /* VK_KHR_external_memory_fd */
#if defined(VK_KHR_external_memory_win32)
  pfn_vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)getDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR");
  pfn_vkGetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR)getDeviceProcAddr(device, "vkGetMemoryWin32HandlePropertiesKHR");
#endif /* VK_KHR_external_memory_win32 */
#if defined(VK_KHR_external_semaphore_capabilities)
  pfn_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
#endif /* VK_KHR_external_semaphore_capabilities */
#if defined(VK_KHR_external_semaphore_fd)
  pfn_vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)getDeviceProcAddr(device, "vkGetSemaphoreFdKHR");
  pfn_vkImportSemaphoreFdKHR = (PFN_vkImportSemaphoreFdKHR)getDeviceProcAddr(device, "vkImportSemaphoreFdKHR");
#endif /* VK_KHR_external_semaphore_fd */
#if defined(VK_KHR_external_semaphore_win32)
  pfn_vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)getDeviceProcAddr(device, "vkGetSemaphoreWin32HandleKHR");
  pfn_vkImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR)getDeviceProcAddr(device, "vkImportSemaphoreWin32HandleKHR");
#endif /* VK_KHR_external_semaphore_win32 */
#if defined(VK_KHR_fragment_shading_rate)
  pfn_vkCmdSetFragmentShadingRateKHR = (PFN_vkCmdSetFragmentShadingRateKHR)getDeviceProcAddr(device, "vkCmdSetFragmentShadingRateKHR");
  pfn_vkGetPhysicalDeviceFragmentShadingRatesKHR = (PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceFragmentShadingRatesKHR");
#endif /* VK_KHR_fragment_shading_rate */
#if defined(VK_KHR_get_memory_requirements2)
  pfn_vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR)getDeviceProcAddr(device, "vkGetBufferMemoryRequirements2KHR");
  pfn_vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR)getDeviceProcAddr(device, "vkGetImageMemoryRequirements2KHR");
  pfn_vkGetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR)getDeviceProcAddr(device, "vkGetImageSparseMemoryRequirements2KHR");
#endif /* VK_KHR_get_memory_requirements2 */
#if defined(VK_KHR_get_physical_device_properties2)
  pfn_vkGetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2KHR");
  pfn_vkGetPhysicalDeviceFormatProperties2KHR = (PFN_vkGetPhysicalDeviceFormatProperties2KHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties2KHR");
  pfn_vkGetPhysicalDeviceImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties2KHR");
  pfn_vkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties2KHR");
  pfn_vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR");
  pfn_vkGetPhysicalDeviceQueueFamilyProperties2KHR = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
  pfn_vkGetPhysicalDeviceSparseImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceSparseImageFormatProperties2KHR");
#endif /* VK_KHR_get_physical_device_properties2 */
#if defined(VK_KHR_maintenance1)
  pfn_vkTrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR)getDeviceProcAddr(device, "vkTrimCommandPoolKHR");
#endif /* VK_KHR_maintenance1 */
#if defined(VK_KHR_maintenance3)
  pfn_vkGetDescriptorSetLayoutSupportKHR = (PFN_vkGetDescriptorSetLayoutSupportKHR)getDeviceProcAddr(device, "vkGetDescriptorSetLayoutSupportKHR");
#endif /* VK_KHR_maintenance3 */
#if defined(VK_KHR_maintenance4)
  pfn_vkGetDeviceBufferMemoryRequirementsKHR = (PFN_vkGetDeviceBufferMemoryRequirementsKHR)getDeviceProcAddr(device, "vkGetDeviceBufferMemoryRequirementsKHR");
  pfn_vkGetDeviceImageMemoryRequirementsKHR = (PFN_vkGetDeviceImageMemoryRequirementsKHR)getDeviceProcAddr(device, "vkGetDeviceImageMemoryRequirementsKHR");
  pfn_vkGetDeviceImageSparseMemoryRequirementsKHR = (PFN_vkGetDeviceImageSparseMemoryRequirementsKHR)getDeviceProcAddr(device, "vkGetDeviceImageSparseMemoryRequirementsKHR");
#endif /* VK_KHR_maintenance4 */
#if defined(VK_KHR_maintenance5)
  pfn_vkCmdBindIndexBuffer2KHR = (PFN_vkCmdBindIndexBuffer2KHR)getDeviceProcAddr(device, "vkCmdBindIndexBuffer2KHR");
  pfn_vkGetDeviceImageSubresourceLayoutKHR = (PFN_vkGetDeviceImageSubresourceLayoutKHR)getDeviceProcAddr(device, "vkGetDeviceImageSubresourceLayoutKHR");
  pfn_vkGetImageSubresourceLayout2KHR = (PFN_vkGetImageSubresourceLayout2KHR)getDeviceProcAddr(device, "vkGetImageSubresourceLayout2KHR");
  pfn_vkGetRenderingAreaGranularityKHR = (PFN_vkGetRenderingAreaGranularityKHR)getDeviceProcAddr(device, "vkGetRenderingAreaGranularityKHR");
#endif /* VK_KHR_maintenance5 */
#if defined(VK_KHR_map_memory2)
  pfn_vkMapMemory2KHR = (PFN_vkMapMemory2KHR)getDeviceProcAddr(device, "vkMapMemory2KHR");
  pfn_vkUnmapMemory2KHR = (PFN_vkUnmapMemory2KHR)getDeviceProcAddr(device, "vkUnmapMemory2KHR");
#endif /* VK_KHR_map_memory2 */
#if defined(VK_KHR_performance_query)
  pfn_vkAcquireProfilingLockKHR = (PFN_vkAcquireProfilingLockKHR)getDeviceProcAddr(device, "vkAcquireProfilingLockKHR");
  pfn_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR = (PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR)getInstanceProcAddr(instance, "vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR");
  pfn_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR = (PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR");
  pfn_vkReleaseProfilingLockKHR = (PFN_vkReleaseProfilingLockKHR)getDeviceProcAddr(device, "vkReleaseProfilingLockKHR");
#endif /* VK_KHR_performance_query */
#if defined(VK_KHR_pipeline_executable_properties)
  pfn_vkGetPipelineExecutableInternalRepresentationsKHR = (PFN_vkGetPipelineExecutableInternalRepresentationsKHR)getDeviceProcAddr(device, "vkGetPipelineExecutableInternalRepresentationsKHR");
  pfn_vkGetPipelineExecutablePropertiesKHR = (PFN_vkGetPipelineExecutablePropertiesKHR)getDeviceProcAddr(device, "vkGetPipelineExecutablePropertiesKHR");
  pfn_vkGetPipelineExecutableStatisticsKHR = (PFN_vkGetPipelineExecutableStatisticsKHR)getDeviceProcAddr(device, "vkGetPipelineExecutableStatisticsKHR");
#endif /* VK_KHR_pipeline_executable_properties */
#if defined(VK_KHR_present_wait)
  pfn_vkWaitForPresentKHR = (PFN_vkWaitForPresentKHR)getDeviceProcAddr(device, "vkWaitForPresentKHR");
#endif /* VK_KHR_present_wait */
#if defined(VK_KHR_push_descriptor)
  pfn_vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)getDeviceProcAddr(device, "vkCmdPushDescriptorSetKHR");
#endif /* VK_KHR_push_descriptor */
#if defined(VK_KHR_ray_tracing_maintenance1) && defined(VK_KHR_ray_tracing_pipeline)
  pfn_vkCmdTraceRaysIndirect2KHR = (PFN_vkCmdTraceRaysIndirect2KHR)getDeviceProcAddr(device, "vkCmdTraceRaysIndirect2KHR");
#endif /* VK_KHR_ray_tracing_maintenance1 && VK_KHR_ray_tracing_pipeline */
#if defined(VK_KHR_ray_tracing_pipeline)
  pfn_vkCmdSetRayTracingPipelineStackSizeKHR = (PFN_vkCmdSetRayTracingPipelineStackSizeKHR)getDeviceProcAddr(device, "vkCmdSetRayTracingPipelineStackSizeKHR");
  pfn_vkCmdTraceRaysIndirectKHR = (PFN_vkCmdTraceRaysIndirectKHR)getDeviceProcAddr(device, "vkCmdTraceRaysIndirectKHR");
  pfn_vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)getDeviceProcAddr(device, "vkCmdTraceRaysKHR");
  pfn_vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)getDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
  pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = (PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)getDeviceProcAddr(device, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR");
  pfn_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)getDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
  pfn_vkGetRayTracingShaderGroupStackSizeKHR = (PFN_vkGetRayTracingShaderGroupStackSizeKHR)getDeviceProcAddr(device, "vkGetRayTracingShaderGroupStackSizeKHR");
#endif /* VK_KHR_ray_tracing_pipeline */
#if defined(VK_KHR_sampler_ycbcr_conversion)
  pfn_vkCreateSamplerYcbcrConversionKHR = (PFN_vkCreateSamplerYcbcrConversionKHR)getDeviceProcAddr(device, "vkCreateSamplerYcbcrConversionKHR");
  pfn_vkDestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR)getDeviceProcAddr(device, "vkDestroySamplerYcbcrConversionKHR");
#endif /* VK_KHR_sampler_ycbcr_conversion */
#if defined(VK_KHR_shared_presentable_image)
  pfn_vkGetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR)getDeviceProcAddr(device, "vkGetSwapchainStatusKHR");
#endif /* VK_KHR_shared_presentable_image */
#if defined(VK_KHR_synchronization2)
  pfn_vkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)getDeviceProcAddr(device, "vkCmdPipelineBarrier2KHR");
  pfn_vkCmdResetEvent2KHR = (PFN_vkCmdResetEvent2KHR)getDeviceProcAddr(device, "vkCmdResetEvent2KHR");
  pfn_vkCmdSetEvent2KHR = (PFN_vkCmdSetEvent2KHR)getDeviceProcAddr(device, "vkCmdSetEvent2KHR");
  pfn_vkCmdWaitEvents2KHR = (PFN_vkCmdWaitEvents2KHR)getDeviceProcAddr(device, "vkCmdWaitEvents2KHR");
  pfn_vkCmdWriteTimestamp2KHR = (PFN_vkCmdWriteTimestamp2KHR)getDeviceProcAddr(device, "vkCmdWriteTimestamp2KHR");
  pfn_vkQueueSubmit2KHR = (PFN_vkQueueSubmit2KHR)getDeviceProcAddr(device, "vkQueueSubmit2KHR");
#endif /* VK_KHR_synchronization2 */
#if defined(VK_KHR_synchronization2) && defined(VK_AMD_buffer_marker)
  pfn_vkCmdWriteBufferMarker2AMD = (PFN_vkCmdWriteBufferMarker2AMD)getDeviceProcAddr(device, "vkCmdWriteBufferMarker2AMD");
#endif /* VK_KHR_synchronization2 && VK_AMD_buffer_marker */
#if defined(VK_KHR_synchronization2) && defined(VK_NV_device_diagnostic_checkpoints)
  pfn_vkGetQueueCheckpointData2NV = (PFN_vkGetQueueCheckpointData2NV)getDeviceProcAddr(device, "vkGetQueueCheckpointData2NV");
#endif /* VK_KHR_synchronization2 && VK_NV_device_diagnostic_checkpoints */
#if defined(VK_KHR_timeline_semaphore)
  pfn_vkGetSemaphoreCounterValueKHR = (PFN_vkGetSemaphoreCounterValueKHR)getDeviceProcAddr(device, "vkGetSemaphoreCounterValueKHR");
  pfn_vkSignalSemaphoreKHR = (PFN_vkSignalSemaphoreKHR)getDeviceProcAddr(device, "vkSignalSemaphoreKHR");
  pfn_vkWaitSemaphoresKHR = (PFN_vkWaitSemaphoresKHR)getDeviceProcAddr(device, "vkWaitSemaphoresKHR");
#endif /* VK_KHR_timeline_semaphore */
#if defined(VK_KHR_video_decode_queue)
  pfn_vkCmdDecodeVideoKHR = (PFN_vkCmdDecodeVideoKHR)getDeviceProcAddr(device, "vkCmdDecodeVideoKHR");
#endif /* VK_KHR_video_decode_queue */
#if defined(VK_KHR_video_queue)
  pfn_vkBindVideoSessionMemoryKHR = (PFN_vkBindVideoSessionMemoryKHR)getDeviceProcAddr(device, "vkBindVideoSessionMemoryKHR");
  pfn_vkCmdBeginVideoCodingKHR = (PFN_vkCmdBeginVideoCodingKHR)getDeviceProcAddr(device, "vkCmdBeginVideoCodingKHR");
  pfn_vkCmdControlVideoCodingKHR = (PFN_vkCmdControlVideoCodingKHR)getDeviceProcAddr(device, "vkCmdControlVideoCodingKHR");
  pfn_vkCmdEndVideoCodingKHR = (PFN_vkCmdEndVideoCodingKHR)getDeviceProcAddr(device, "vkCmdEndVideoCodingKHR");
  pfn_vkCreateVideoSessionKHR = (PFN_vkCreateVideoSessionKHR)getDeviceProcAddr(device, "vkCreateVideoSessionKHR");
  pfn_vkCreateVideoSessionParametersKHR = (PFN_vkCreateVideoSessionParametersKHR)getDeviceProcAddr(device, "vkCreateVideoSessionParametersKHR");
  pfn_vkDestroyVideoSessionKHR = (PFN_vkDestroyVideoSessionKHR)getDeviceProcAddr(device, "vkDestroyVideoSessionKHR");
  pfn_vkDestroyVideoSessionParametersKHR = (PFN_vkDestroyVideoSessionParametersKHR)getDeviceProcAddr(device, "vkDestroyVideoSessionParametersKHR");
  pfn_vkGetPhysicalDeviceVideoCapabilitiesKHR = (PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceVideoCapabilitiesKHR");
  pfn_vkGetPhysicalDeviceVideoFormatPropertiesKHR = (PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceVideoFormatPropertiesKHR");
  pfn_vkGetVideoSessionMemoryRequirementsKHR = (PFN_vkGetVideoSessionMemoryRequirementsKHR)getDeviceProcAddr(device, "vkGetVideoSessionMemoryRequirementsKHR");
  pfn_vkUpdateVideoSessionParametersKHR = (PFN_vkUpdateVideoSessionParametersKHR)getDeviceProcAddr(device, "vkUpdateVideoSessionParametersKHR");
#endif /* VK_KHR_video_queue */
#if defined(VK_MVK_ios_surface)
  pfn_vkCreateIOSSurfaceMVK = (PFN_vkCreateIOSSurfaceMVK)getInstanceProcAddr(instance, "vkCreateIOSSurfaceMVK");
#endif /* VK_MVK_ios_surface */
#if defined(VK_MVK_macos_surface)
  pfn_vkCreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK)getInstanceProcAddr(instance, "vkCreateMacOSSurfaceMVK");
#endif /* VK_MVK_macos_surface */
#if defined(VK_NN_vi_surface)
  pfn_vkCreateViSurfaceNN = (PFN_vkCreateViSurfaceNN)getInstanceProcAddr(instance, "vkCreateViSurfaceNN");
#endif /* VK_NN_vi_surface */
#if defined(VK_NVX_binary_import)
  pfn_vkCmdCuLaunchKernelNVX = (PFN_vkCmdCuLaunchKernelNVX)getDeviceProcAddr(device, "vkCmdCuLaunchKernelNVX");
  pfn_vkCreateCuFunctionNVX = (PFN_vkCreateCuFunctionNVX)getDeviceProcAddr(device, "vkCreateCuFunctionNVX");
  pfn_vkCreateCuModuleNVX = (PFN_vkCreateCuModuleNVX)getDeviceProcAddr(device, "vkCreateCuModuleNVX");
  pfn_vkDestroyCuFunctionNVX = (PFN_vkDestroyCuFunctionNVX)getDeviceProcAddr(device, "vkDestroyCuFunctionNVX");
  pfn_vkDestroyCuModuleNVX = (PFN_vkDestroyCuModuleNVX)getDeviceProcAddr(device, "vkDestroyCuModuleNVX");
#endif /* VK_NVX_binary_import */
#if defined(VK_NVX_image_view_handle)
  pfn_vkGetImageViewAddressNVX = (PFN_vkGetImageViewAddressNVX)getDeviceProcAddr(device, "vkGetImageViewAddressNVX");
  pfn_vkGetImageViewHandleNVX = (PFN_vkGetImageViewHandleNVX)getDeviceProcAddr(device, "vkGetImageViewHandleNVX");
#endif /* VK_NVX_image_view_handle */
#if defined(VK_NV_acquire_winrt_display)
  pfn_vkAcquireWinrtDisplayNV = (PFN_vkAcquireWinrtDisplayNV)getInstanceProcAddr(instance, "vkAcquireWinrtDisplayNV");
  pfn_vkGetWinrtDisplayNV = (PFN_vkGetWinrtDisplayNV)getInstanceProcAddr(instance, "vkGetWinrtDisplayNV");
#endif /* VK_NV_acquire_winrt_display */
#if defined(VK_NV_clip_space_w_scaling)
  pfn_vkCmdSetViewportWScalingNV = (PFN_vkCmdSetViewportWScalingNV)getDeviceProcAddr(device, "vkCmdSetViewportWScalingNV");
#endif /* VK_NV_clip_space_w_scaling */
#if defined(VK_NV_cooperative_matrix)
  pfn_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV = (PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV)getInstanceProcAddr(instance, "vkGetPhysicalDeviceCooperativeMatrixPropertiesNV");
#endif /* VK_NV_cooperative_matrix */
#if defined(VK_NV_copy_memory_indirect)
  pfn_vkCmdCopyMemoryIndirectNV = (PFN_vkCmdCopyMemoryIndirectNV)getDeviceProcAddr(device, "vkCmdCopyMemoryIndirectNV");
  pfn_vkCmdCopyMemoryToImageIndirectNV = (PFN_vkCmdCopyMemoryToImageIndirectNV)getDeviceProcAddr(device, "vkCmdCopyMemoryToImageIndirectNV");
#endif /* VK_NV_copy_memory_indirect */
#if defined(VK_NV_coverage_reduction_mode)
  pfn_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV = (PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV)getInstanceProcAddr(instance, "vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV");
#endif /* VK_NV_coverage_reduction_mode */
#if defined(VK_NV_device_diagnostic_checkpoints)
  pfn_vkCmdSetCheckpointNV = (PFN_vkCmdSetCheckpointNV)getDeviceProcAddr(device, "vkCmdSetCheckpointNV");
  pfn_vkGetQueueCheckpointDataNV = (PFN_vkGetQueueCheckpointDataNV)getDeviceProcAddr(device, "vkGetQueueCheckpointDataNV");
#endif /* VK_NV_device_diagnostic_checkpoints */
#if defined(VK_NV_device_generated_commands)
  pfn_vkCmdBindPipelineShaderGroupNV = (PFN_vkCmdBindPipelineShaderGroupNV)getDeviceProcAddr(device, "vkCmdBindPipelineShaderGroupNV");
  pfn_vkCmdExecuteGeneratedCommandsNV = (PFN_vkCmdExecuteGeneratedCommandsNV)getDeviceProcAddr(device, "vkCmdExecuteGeneratedCommandsNV");
  pfn_vkCmdPreprocessGeneratedCommandsNV = (PFN_vkCmdPreprocessGeneratedCommandsNV)getDeviceProcAddr(device, "vkCmdPreprocessGeneratedCommandsNV");
  pfn_vkCreateIndirectCommandsLayoutNV = (PFN_vkCreateIndirectCommandsLayoutNV)getDeviceProcAddr(device, "vkCreateIndirectCommandsLayoutNV");
  pfn_vkDestroyIndirectCommandsLayoutNV = (PFN_vkDestroyIndirectCommandsLayoutNV)getDeviceProcAddr(device, "vkDestroyIndirectCommandsLayoutNV");
  pfn_vkGetGeneratedCommandsMemoryRequirementsNV = (PFN_vkGetGeneratedCommandsMemoryRequirementsNV)getDeviceProcAddr(device, "vkGetGeneratedCommandsMemoryRequirementsNV");
#endif /* VK_NV_device_generated_commands */
#if defined(VK_NV_device_generated_commands_compute)
  pfn_vkCmdUpdatePipelineIndirectBufferNV = (PFN_vkCmdUpdatePipelineIndirectBufferNV)getDeviceProcAddr(device, "vkCmdUpdatePipelineIndirectBufferNV");
  pfn_vkGetPipelineIndirectDeviceAddressNV = (PFN_vkGetPipelineIndirectDeviceAddressNV)getDeviceProcAddr(device, "vkGetPipelineIndirectDeviceAddressNV");
  pfn_vkGetPipelineIndirectMemoryRequirementsNV = (PFN_vkGetPipelineIndirectMemoryRequirementsNV)getDeviceProcAddr(device, "vkGetPipelineIndirectMemoryRequirementsNV");
#endif /* VK_NV_device_generated_commands_compute */
#if defined(VK_NV_external_memory_capabilities)
  pfn_vkGetPhysicalDeviceExternalImageFormatPropertiesNV = (PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV)getInstanceProcAddr(instance, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV");
#endif /* VK_NV_external_memory_capabilities */
#if defined(VK_NV_external_memory_rdma)
  pfn_vkGetMemoryRemoteAddressNV = (PFN_vkGetMemoryRemoteAddressNV)getDeviceProcAddr(device, "vkGetMemoryRemoteAddressNV");
#endif /* VK_NV_external_memory_rdma */
#if defined(VK_NV_external_memory_win32)
  pfn_vkGetMemoryWin32HandleNV = (PFN_vkGetMemoryWin32HandleNV)getDeviceProcAddr(device, "vkGetMemoryWin32HandleNV");
#endif /* VK_NV_external_memory_win32 */
#if defined(VK_NV_fragment_shading_rate_enums)
  pfn_vkCmdSetFragmentShadingRateEnumNV = (PFN_vkCmdSetFragmentShadingRateEnumNV)getDeviceProcAddr(device, "vkCmdSetFragmentShadingRateEnumNV");
#endif /* VK_NV_fragment_shading_rate_enums */
#if defined(VK_NV_memory_decompression)
  pfn_vkCmdDecompressMemoryIndirectCountNV = (PFN_vkCmdDecompressMemoryIndirectCountNV)getDeviceProcAddr(device, "vkCmdDecompressMemoryIndirectCountNV");
  pfn_vkCmdDecompressMemoryNV = (PFN_vkCmdDecompressMemoryNV)getDeviceProcAddr(device, "vkCmdDecompressMemoryNV");
#endif /* VK_NV_memory_decompression */
#if defined(VK_NV_mesh_shader)
  pfn_vkCmdDrawMeshTasksIndirectCountNV = (PFN_vkCmdDrawMeshTasksIndirectCountNV)getDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectCountNV");
  pfn_vkCmdDrawMeshTasksIndirectNV = (PFN_vkCmdDrawMeshTasksIndirectNV)getDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectNV");
  pfn_vkCmdDrawMeshTasksNV = (PFN_vkCmdDrawMeshTasksNV)getDeviceProcAddr(device, "vkCmdDrawMeshTasksNV");
#endif /* VK_NV_mesh_shader */
#if defined(VK_NV_optical_flow)
  pfn_vkBindOpticalFlowSessionImageNV = (PFN_vkBindOpticalFlowSessionImageNV)getDeviceProcAddr(device, "vkBindOpticalFlowSessionImageNV");
  pfn_vkCmdOpticalFlowExecuteNV = (PFN_vkCmdOpticalFlowExecuteNV)getDeviceProcAddr(device, "vkCmdOpticalFlowExecuteNV");
  pfn_vkCreateOpticalFlowSessionNV = (PFN_vkCreateOpticalFlowSessionNV)getDeviceProcAddr(device, "vkCreateOpticalFlowSessionNV");
  pfn_vkDestroyOpticalFlowSessionNV = (PFN_vkDestroyOpticalFlowSessionNV)getDeviceProcAddr(device, "vkDestroyOpticalFlowSessionNV");
  pfn_vkGetPhysicalDeviceOpticalFlowImageFormatsNV = (PFN_vkGetPhysicalDeviceOpticalFlowImageFormatsNV)getInstanceProcAddr(instance, "vkGetPhysicalDeviceOpticalFlowImageFormatsNV");
#endif /* VK_NV_optical_flow */
#if defined(VK_NV_ray_tracing)
  pfn_vkBindAccelerationStructureMemoryNV = (PFN_vkBindAccelerationStructureMemoryNV)getDeviceProcAddr(device, "vkBindAccelerationStructureMemoryNV");
  pfn_vkCmdBuildAccelerationStructureNV = (PFN_vkCmdBuildAccelerationStructureNV)getDeviceProcAddr(device, "vkCmdBuildAccelerationStructureNV");
  pfn_vkCmdCopyAccelerationStructureNV = (PFN_vkCmdCopyAccelerationStructureNV)getDeviceProcAddr(device, "vkCmdCopyAccelerationStructureNV");
  pfn_vkCmdTraceRaysNV = (PFN_vkCmdTraceRaysNV)getDeviceProcAddr(device, "vkCmdTraceRaysNV");
  pfn_vkCmdWriteAccelerationStructuresPropertiesNV = (PFN_vkCmdWriteAccelerationStructuresPropertiesNV)getDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesNV");
  pfn_vkCompileDeferredNV = (PFN_vkCompileDeferredNV)getDeviceProcAddr(device, "vkCompileDeferredNV");
  pfn_vkCreateAccelerationStructureNV = (PFN_vkCreateAccelerationStructureNV)getDeviceProcAddr(device, "vkCreateAccelerationStructureNV");
  pfn_vkCreateRayTracingPipelinesNV = (PFN_vkCreateRayTracingPipelinesNV)getDeviceProcAddr(device, "vkCreateRayTracingPipelinesNV");
  pfn_vkDestroyAccelerationStructureNV = (PFN_vkDestroyAccelerationStructureNV)getDeviceProcAddr(device, "vkDestroyAccelerationStructureNV");
  pfn_vkGetAccelerationStructureHandleNV = (PFN_vkGetAccelerationStructureHandleNV)getDeviceProcAddr(device, "vkGetAccelerationStructureHandleNV");
  pfn_vkGetAccelerationStructureMemoryRequirementsNV = (PFN_vkGetAccelerationStructureMemoryRequirementsNV)getDeviceProcAddr(device, "vkGetAccelerationStructureMemoryRequirementsNV");
  pfn_vkGetRayTracingShaderGroupHandlesNV = (PFN_vkGetRayTracingShaderGroupHandlesNV)getDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesNV");
#endif /* VK_NV_ray_tracing */
#if defined(VK_NV_scissor_exclusive) && VK_NV_SCISSOR_EXCLUSIVE_SPEC_VERSION >= 2
  pfn_vkCmdSetExclusiveScissorEnableNV = (PFN_vkCmdSetExclusiveScissorEnableNV)getDeviceProcAddr(device, "vkCmdSetExclusiveScissorEnableNV");
#endif /* VK_NV_scissor_exclusive && VK_NV_SCISSOR_EXCLUSIVE_SPEC_VERSION >= 2 */
#if defined(VK_NV_scissor_exclusive)
  pfn_vkCmdSetExclusiveScissorNV = (PFN_vkCmdSetExclusiveScissorNV)getDeviceProcAddr(device, "vkCmdSetExclusiveScissorNV");
#endif /* VK_NV_scissor_exclusive */
#if defined(VK_NV_shading_rate_image)
  pfn_vkCmdBindShadingRateImageNV = (PFN_vkCmdBindShadingRateImageNV)getDeviceProcAddr(device, "vkCmdBindShadingRateImageNV");
  pfn_vkCmdSetCoarseSampleOrderNV = (PFN_vkCmdSetCoarseSampleOrderNV)getDeviceProcAddr(device, "vkCmdSetCoarseSampleOrderNV");
  pfn_vkCmdSetViewportShadingRatePaletteNV = (PFN_vkCmdSetViewportShadingRatePaletteNV)getDeviceProcAddr(device, "vkCmdSetViewportShadingRatePaletteNV");
#endif /* VK_NV_shading_rate_image */
#if defined(VK_QCOM_tile_properties)
  pfn_vkGetDynamicRenderingTilePropertiesQCOM = (PFN_vkGetDynamicRenderingTilePropertiesQCOM)getDeviceProcAddr(device, "vkGetDynamicRenderingTilePropertiesQCOM");
  pfn_vkGetFramebufferTilePropertiesQCOM = (PFN_vkGetFramebufferTilePropertiesQCOM)getDeviceProcAddr(device, "vkGetFramebufferTilePropertiesQCOM");
#endif /* VK_QCOM_tile_properties */
#if defined(VK_QNX_external_memory_screen_buffer)
  pfn_vkGetScreenBufferPropertiesQNX = (PFN_vkGetScreenBufferPropertiesQNX)getDeviceProcAddr(device, "vkGetScreenBufferPropertiesQNX");
#endif /* VK_QNX_external_memory_screen_buffer */
#if defined(VK_QNX_screen_surface)
  pfn_vkCreateScreenSurfaceQNX = (PFN_vkCreateScreenSurfaceQNX)getInstanceProcAddr(instance, "vkCreateScreenSurfaceQNX");
  pfn_vkGetPhysicalDeviceScreenPresentationSupportQNX = (PFN_vkGetPhysicalDeviceScreenPresentationSupportQNX)getInstanceProcAddr(instance, "vkGetPhysicalDeviceScreenPresentationSupportQNX");
#endif /* VK_QNX_screen_surface */
#if defined(VK_VALVE_descriptor_set_host_mapping)
  pfn_vkGetDescriptorSetHostMappingVALVE = (PFN_vkGetDescriptorSetHostMappingVALVE)getDeviceProcAddr(device, "vkGetDescriptorSetHostMappingVALVE");
  pfn_vkGetDescriptorSetLayoutHostMappingInfoVALVE = (PFN_vkGetDescriptorSetLayoutHostMappingInfoVALVE)getDeviceProcAddr(device, "vkGetDescriptorSetLayoutHostMappingInfoVALVE");
#endif /* VK_VALVE_descriptor_set_host_mapping */
#if defined(VK_EXT_extended_dynamic_state) || defined(VK_EXT_shader_object)
  pfn_vkCmdBindVertexBuffers2EXT = (PFN_vkCmdBindVertexBuffers2EXT)getDeviceProcAddr(device, "vkCmdBindVertexBuffers2EXT");
  pfn_vkCmdSetCullModeEXT = (PFN_vkCmdSetCullModeEXT)getDeviceProcAddr(device, "vkCmdSetCullModeEXT");
  pfn_vkCmdSetDepthBoundsTestEnableEXT = (PFN_vkCmdSetDepthBoundsTestEnableEXT)getDeviceProcAddr(device, "vkCmdSetDepthBoundsTestEnableEXT");
  pfn_vkCmdSetDepthCompareOpEXT = (PFN_vkCmdSetDepthCompareOpEXT)getDeviceProcAddr(device, "vkCmdSetDepthCompareOpEXT");
  pfn_vkCmdSetDepthTestEnableEXT = (PFN_vkCmdSetDepthTestEnableEXT)getDeviceProcAddr(device, "vkCmdSetDepthTestEnableEXT");
  pfn_vkCmdSetDepthWriteEnableEXT = (PFN_vkCmdSetDepthWriteEnableEXT)getDeviceProcAddr(device, "vkCmdSetDepthWriteEnableEXT");
  pfn_vkCmdSetFrontFaceEXT = (PFN_vkCmdSetFrontFaceEXT)getDeviceProcAddr(device, "vkCmdSetFrontFaceEXT");
  pfn_vkCmdSetPrimitiveTopologyEXT = (PFN_vkCmdSetPrimitiveTopologyEXT)getDeviceProcAddr(device, "vkCmdSetPrimitiveTopologyEXT");
  pfn_vkCmdSetScissorWithCountEXT = (PFN_vkCmdSetScissorWithCountEXT)getDeviceProcAddr(device, "vkCmdSetScissorWithCountEXT");
  pfn_vkCmdSetStencilOpEXT = (PFN_vkCmdSetStencilOpEXT)getDeviceProcAddr(device, "vkCmdSetStencilOpEXT");
  pfn_vkCmdSetStencilTestEnableEXT = (PFN_vkCmdSetStencilTestEnableEXT)getDeviceProcAddr(device, "vkCmdSetStencilTestEnableEXT");
  pfn_vkCmdSetViewportWithCountEXT = (PFN_vkCmdSetViewportWithCountEXT)getDeviceProcAddr(device, "vkCmdSetViewportWithCountEXT");
#endif /* VK_EXT_extended_dynamic_state || VK_EXT_shader_object */
#if defined(VK_EXT_extended_dynamic_state2) || defined(VK_EXT_shader_object)
  pfn_vkCmdSetDepthBiasEnableEXT = (PFN_vkCmdSetDepthBiasEnableEXT)getDeviceProcAddr(device, "vkCmdSetDepthBiasEnableEXT");
  pfn_vkCmdSetLogicOpEXT = (PFN_vkCmdSetLogicOpEXT)getDeviceProcAddr(device, "vkCmdSetLogicOpEXT");
  pfn_vkCmdSetPatchControlPointsEXT = (PFN_vkCmdSetPatchControlPointsEXT)getDeviceProcAddr(device, "vkCmdSetPatchControlPointsEXT");
  pfn_vkCmdSetPrimitiveRestartEnableEXT = (PFN_vkCmdSetPrimitiveRestartEnableEXT)getDeviceProcAddr(device, "vkCmdSetPrimitiveRestartEnableEXT");
  pfn_vkCmdSetRasterizerDiscardEnableEXT = (PFN_vkCmdSetRasterizerDiscardEnableEXT)getDeviceProcAddr(device, "vkCmdSetRasterizerDiscardEnableEXT");
#endif /* VK_EXT_extended_dynamic_state2 || VK_EXT_shader_object */
#if defined(VK_EXT_extended_dynamic_state3) || defined(VK_EXT_shader_object)
  pfn_vkCmdSetAlphaToCoverageEnableEXT = (PFN_vkCmdSetAlphaToCoverageEnableEXT)getDeviceProcAddr(device, "vkCmdSetAlphaToCoverageEnableEXT");
  pfn_vkCmdSetAlphaToOneEnableEXT = (PFN_vkCmdSetAlphaToOneEnableEXT)getDeviceProcAddr(device, "vkCmdSetAlphaToOneEnableEXT");
  pfn_vkCmdSetColorBlendAdvancedEXT = (PFN_vkCmdSetColorBlendAdvancedEXT)getDeviceProcAddr(device, "vkCmdSetColorBlendAdvancedEXT");
  pfn_vkCmdSetColorBlendEnableEXT = (PFN_vkCmdSetColorBlendEnableEXT)getDeviceProcAddr(device, "vkCmdSetColorBlendEnableEXT");
  pfn_vkCmdSetColorBlendEquationEXT = (PFN_vkCmdSetColorBlendEquationEXT)getDeviceProcAddr(device, "vkCmdSetColorBlendEquationEXT");
  pfn_vkCmdSetColorWriteMaskEXT = (PFN_vkCmdSetColorWriteMaskEXT)getDeviceProcAddr(device, "vkCmdSetColorWriteMaskEXT");
  pfn_vkCmdSetConservativeRasterizationModeEXT = (PFN_vkCmdSetConservativeRasterizationModeEXT)getDeviceProcAddr(device, "vkCmdSetConservativeRasterizationModeEXT");
  pfn_vkCmdSetDepthClampEnableEXT = (PFN_vkCmdSetDepthClampEnableEXT)getDeviceProcAddr(device, "vkCmdSetDepthClampEnableEXT");
  pfn_vkCmdSetDepthClipEnableEXT = (PFN_vkCmdSetDepthClipEnableEXT)getDeviceProcAddr(device, "vkCmdSetDepthClipEnableEXT");
  pfn_vkCmdSetDepthClipNegativeOneToOneEXT = (PFN_vkCmdSetDepthClipNegativeOneToOneEXT)getDeviceProcAddr(device, "vkCmdSetDepthClipNegativeOneToOneEXT");
  pfn_vkCmdSetExtraPrimitiveOverestimationSizeEXT = (PFN_vkCmdSetExtraPrimitiveOverestimationSizeEXT)getDeviceProcAddr(device, "vkCmdSetExtraPrimitiveOverestimationSizeEXT");
  pfn_vkCmdSetLineRasterizationModeEXT = (PFN_vkCmdSetLineRasterizationModeEXT)getDeviceProcAddr(device, "vkCmdSetLineRasterizationModeEXT");
  pfn_vkCmdSetLineStippleEnableEXT = (PFN_vkCmdSetLineStippleEnableEXT)getDeviceProcAddr(device, "vkCmdSetLineStippleEnableEXT");
  pfn_vkCmdSetLogicOpEnableEXT = (PFN_vkCmdSetLogicOpEnableEXT)getDeviceProcAddr(device, "vkCmdSetLogicOpEnableEXT");
  pfn_vkCmdSetPolygonModeEXT = (PFN_vkCmdSetPolygonModeEXT)getDeviceProcAddr(device, "vkCmdSetPolygonModeEXT");
  pfn_vkCmdSetProvokingVertexModeEXT = (PFN_vkCmdSetProvokingVertexModeEXT)getDeviceProcAddr(device, "vkCmdSetProvokingVertexModeEXT");
  pfn_vkCmdSetRasterizationSamplesEXT = (PFN_vkCmdSetRasterizationSamplesEXT)getDeviceProcAddr(device, "vkCmdSetRasterizationSamplesEXT");
  pfn_vkCmdSetRasterizationStreamEXT = (PFN_vkCmdSetRasterizationStreamEXT)getDeviceProcAddr(device, "vkCmdSetRasterizationStreamEXT");
  pfn_vkCmdSetSampleLocationsEnableEXT = (PFN_vkCmdSetSampleLocationsEnableEXT)getDeviceProcAddr(device, "vkCmdSetSampleLocationsEnableEXT");
  pfn_vkCmdSetSampleMaskEXT = (PFN_vkCmdSetSampleMaskEXT)getDeviceProcAddr(device, "vkCmdSetSampleMaskEXT");
  pfn_vkCmdSetTessellationDomainOriginEXT = (PFN_vkCmdSetTessellationDomainOriginEXT)getDeviceProcAddr(device, "vkCmdSetTessellationDomainOriginEXT");
#endif /* VK_EXT_extended_dynamic_state3 || VK_EXT_shader_object */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_clip_space_w_scaling) || defined(VK_EXT_shader_object) && defined(VK_NV_clip_space_w_scaling)
  pfn_vkCmdSetViewportWScalingEnableNV = (PFN_vkCmdSetViewportWScalingEnableNV)getDeviceProcAddr(device, "vkCmdSetViewportWScalingEnableNV");
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_clip_space_w_scaling || VK_EXT_shader_object && VK_NV_clip_space_w_scaling */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_viewport_swizzle) || defined(VK_EXT_shader_object) && defined(VK_NV_viewport_swizzle)
  pfn_vkCmdSetViewportSwizzleNV = (PFN_vkCmdSetViewportSwizzleNV)getDeviceProcAddr(device, "vkCmdSetViewportSwizzleNV");
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_viewport_swizzle || VK_EXT_shader_object && VK_NV_viewport_swizzle */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_fragment_coverage_to_color) || defined(VK_EXT_shader_object) && defined(VK_NV_fragment_coverage_to_color)
  pfn_vkCmdSetCoverageToColorEnableNV = (PFN_vkCmdSetCoverageToColorEnableNV)getDeviceProcAddr(device, "vkCmdSetCoverageToColorEnableNV");
  pfn_vkCmdSetCoverageToColorLocationNV = (PFN_vkCmdSetCoverageToColorLocationNV)getDeviceProcAddr(device, "vkCmdSetCoverageToColorLocationNV");
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_fragment_coverage_to_color || VK_EXT_shader_object && VK_NV_fragment_coverage_to_color */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_framebuffer_mixed_samples) || defined(VK_EXT_shader_object) && defined(VK_NV_framebuffer_mixed_samples)
  pfn_vkCmdSetCoverageModulationModeNV = (PFN_vkCmdSetCoverageModulationModeNV)getDeviceProcAddr(device, "vkCmdSetCoverageModulationModeNV");
  pfn_vkCmdSetCoverageModulationTableEnableNV = (PFN_vkCmdSetCoverageModulationTableEnableNV)getDeviceProcAddr(device, "vkCmdSetCoverageModulationTableEnableNV");
  pfn_vkCmdSetCoverageModulationTableNV = (PFN_vkCmdSetCoverageModulationTableNV)getDeviceProcAddr(device, "vkCmdSetCoverageModulationTableNV");
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_framebuffer_mixed_samples || VK_EXT_shader_object && VK_NV_framebuffer_mixed_samples */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_shading_rate_image) || defined(VK_EXT_shader_object) && defined(VK_NV_shading_rate_image)
  pfn_vkCmdSetShadingRateImageEnableNV = (PFN_vkCmdSetShadingRateImageEnableNV)getDeviceProcAddr(device, "vkCmdSetShadingRateImageEnableNV");
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_shading_rate_image || VK_EXT_shader_object && VK_NV_shading_rate_image */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_representative_fragment_test) || defined(VK_EXT_shader_object) && defined(VK_NV_representative_fragment_test)
  pfn_vkCmdSetRepresentativeFragmentTestEnableNV = (PFN_vkCmdSetRepresentativeFragmentTestEnableNV)getDeviceProcAddr(device, "vkCmdSetRepresentativeFragmentTestEnableNV");
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_representative_fragment_test || VK_EXT_shader_object && VK_NV_representative_fragment_test */
#if defined(VK_EXT_extended_dynamic_state3) && defined(VK_NV_coverage_reduction_mode) || defined(VK_EXT_shader_object) && defined(VK_NV_coverage_reduction_mode)
  pfn_vkCmdSetCoverageReductionModeNV = (PFN_vkCmdSetCoverageReductionModeNV)getDeviceProcAddr(device, "vkCmdSetCoverageReductionModeNV");
#endif /* VK_EXT_extended_dynamic_state3 && VK_NV_coverage_reduction_mode || VK_EXT_shader_object && VK_NV_coverage_reduction_mode */
#if defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group) || defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)
  pfn_vkGetDeviceGroupSurfacePresentModes2EXT = (PFN_vkGetDeviceGroupSurfacePresentModes2EXT)getDeviceProcAddr(device, "vkGetDeviceGroupSurfacePresentModes2EXT");
#endif /* VK_EXT_full_screen_exclusive && VK_KHR_device_group || VK_EXT_full_screen_exclusive && VK_VERSION_1_1 */
#if defined(VK_EXT_host_image_copy) || defined(VK_EXT_image_compression_control)
  pfn_vkGetImageSubresourceLayout2EXT = (PFN_vkGetImageSubresourceLayout2EXT)getDeviceProcAddr(device, "vkGetImageSubresourceLayout2EXT");
#endif /* VK_EXT_host_image_copy || VK_EXT_image_compression_control */
#if defined(VK_EXT_shader_object) || defined(VK_EXT_vertex_input_dynamic_state)
  pfn_vkCmdSetVertexInputEXT = (PFN_vkCmdSetVertexInputEXT)getDeviceProcAddr(device, "vkCmdSetVertexInputEXT");
#endif /* VK_EXT_shader_object || VK_EXT_vertex_input_dynamic_state */
#if defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor) || defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1) || defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template)
  pfn_vkCmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)getDeviceProcAddr(device, "vkCmdPushDescriptorSetWithTemplateKHR");
#endif /* VK_KHR_descriptor_update_template && VK_KHR_push_descriptor || VK_KHR_push_descriptor && VK_VERSION_1_1 || VK_KHR_push_descriptor && VK_KHR_descriptor_update_template */
#if defined(VK_KHR_device_group) && defined(VK_KHR_surface) || defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)
  pfn_vkGetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR)getDeviceProcAddr(device, "vkGetDeviceGroupPresentCapabilitiesKHR");
  pfn_vkGetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR)getDeviceProcAddr(device, "vkGetDeviceGroupSurfacePresentModesKHR");
  pfn_vkGetPhysicalDevicePresentRectanglesKHR = (PFN_vkGetPhysicalDevicePresentRectanglesKHR)getInstanceProcAddr(instance, "vkGetPhysicalDevicePresentRectanglesKHR");
#endif /* VK_KHR_device_group && VK_KHR_surface || VK_KHR_swapchain && VK_VERSION_1_1 */
#if defined(VK_KHR_device_group) && defined(VK_KHR_swapchain) || defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)
  pfn_vkAcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR)getDeviceProcAddr(device, "vkAcquireNextImage2KHR");
#endif /* VK_KHR_device_group && VK_KHR_swapchain || VK_KHR_swapchain && VK_VERSION_1_1 */
/* NVVK_GENERATE_LOAD_PROC */
}


/* clang-format on */

