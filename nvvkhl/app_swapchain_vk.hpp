/*
 * Copyright (c) 2022-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>

#include "app_utils.hpp"

namespace nvvkhl {


//--- Swapchain ------------------------------------------------------------------------------------------------------------
/*--
 * Swapchain: The swapchain is responsible for presenting rendered images to the screen.
 * It consists of multiple images (frames) that are cycled through for rendering and display.
 * The swapchain is created with a surface and optional vSync setting, with the
 * window size determined during its setup.
 * "Frames in flight" refers to the number of images being processed concurrently (e.g., double buffering = 2, triple buffering = 3).
 * vSync enabled (FIFO mode) uses double buffering, while disabling vSync  (MAILBOX mode) uses triple buffering.
 *
 * The "current frame" is the frame currently being processed.
 * The "next image index" points to the swapchain image that will be rendered next, which might differ from the current frame's index.
 * If the window is resized or certain conditions are met, the swapchain needs to be recreated (`needRebuild` flag).
-*/
class AppSwapchain
{
public:
  AppSwapchain() = default;
  ~AppSwapchain();

  void        requestRebuild();
  bool        needRebuilding() const;
  VkImage     getNextImage() const;
  VkImageView getNextImageView() const;
  VkFormat    getImageFormat() const;
  uint32_t    getMaxFramesInFlight() const;
  // Wait for image to be available
  VkSemaphore getImageAvailableSemaphore() const;
  // Signal when rendering is finished
  VkSemaphore getRenderFinishedSemaphore() const;

  // Initialize the swapchain with the provided context and surface, then we can create and re-create it
  void init(VkPhysicalDevice physicalDevice, VkDevice device, const nvvkhl::QueueInfo& queue, VkSurfaceKHR surface, VkCommandPool cmdPool);

  // Destroy internal resources and reset its initial state
  void deinit();

  /*--
   * Create the swapchain using the provided context, surface, and vSync option. The actual window size is returned.
   * Queries the GPU capabilities, selects the best surface format and present mode, and creates the swapchain accordingly.
  -*/
  VkExtent2D initResources(bool vSync = true);

  /*--
   * Recreate the swapchain, typically after a window resize or when it becomes invalid.
   * This waits for all rendering to be finished before destroying the old swapchain and creating a new one.
  -*/
  VkExtent2D reinitResources(bool vSync = true);

  /*--
   * Destroy the swapchain and its associated resources.
   * This function is also called when the swapchain needs to be recreated.
  -*/
  void deinitResources();

  /*--
   * Prepares the command buffer for recording rendering commands.
   * This function handles synchronization with the previous frame and acquires the next image from the swapchain.
   * The command buffer is reset, ready for new rendering commands.
  -*/
  void acquireNextImage(VkDevice device);

  /*--
   * Presents the rendered image to the screen.
   * The semaphore ensures that the image is presented only after rendering is complete.
   * Advances to the next frame in the cycle.
  -*/
  void presentFrame(VkQueue queue);

private:
  // Represents an image within the swapchain that can be rendered to.
  struct Image
  {
    VkImage     image{};      // Image to render to
    VkImageView imageView{};  // Image view to access the image
  };
  /*--
   * Resources associated with each frame being processed.
   * Each frame has its own set of resources, mainly synchronization primitives
  -*/
  struct FrameResources
  {
    VkSemaphore imageAvailableSemaphore{};  // Signals when the image is ready for rendering
    VkSemaphore renderFinishedSemaphore{};  // Signals when rendering is finished
  };

  // We choose the format that is the most common, and that is supported by* the physical device.
  VkSurfaceFormat2KHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormat2KHR>& availableFormats) const;

  /*--
   * The present mode is chosen based on the vSync option
   * The FIFO mode is the most common, and is used when vSync is enabled.
   * The MAILBOX mode is used when vSync is disabled, and is the best mode for triple buffering.
   * The IMMEDIATE mode is used when vSync is disabled, and is the best mode for low latency.
  -*/
  VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vSync = true);

private:
  VkPhysicalDevice  m_physicalDevice{};  // The physical device (GPU)
  VkDevice          m_device{};          // The logical device (interface to the physical device)
  nvvkhl::QueueInfo m_queue{};           // The queue used to submit command buffers to the GPU
  VkSwapchainKHR    m_swapChain{};       // The swapchain
  VkFormat          m_imageFormat{};     // The format of the swapchain images
  VkSurfaceKHR      m_surface{};         // The surface to present images to
  VkCommandPool     m_cmdPool{};         // The command pool for the swapchain

  std::vector<Image>          m_nextImages{};
  std::vector<FrameResources> m_frameResources{};
  uint32_t                    m_currentFrame   = 0;
  uint32_t                    m_nextImageIndex = 0;
  bool                        m_needRebuild    = false;

  uint32_t m_maxFramesInFlight = 3;  // Best for pretty much all cases
};

}  // namespace nvvkhl