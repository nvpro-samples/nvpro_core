/*
 * Copyright (c) 2014-2023, NVIDIA CORPORATION.  All rights reserved.
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

#pragma once

#include <set>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "nvh/cameramanipulator.hpp"
#include "nvh/timesampler.hpp"
#include "nvvk/swapchain_vk.hpp"

struct GLFWwindow;

namespace nvvkhl {

/**
\class nvvkhl::AppBase

nvvkhl::AppBaseVk is the same as nvvkhl::AppBaseVk but makes use of the Vulkan C++ API (`vulkan.hpp`).

*/

class AppBase
{
public:
  AppBase()          = default;
  virtual ~AppBase() = default;

  virtual void onResize(int /*w*/, int /*h*/);
  ;  // To implement when the size of the window change

  //--------------------------------------------------------------------------------------------------
  // Setup the low level Vulkan for various operations
  //
  virtual void setup(const vk::Instance& instance, const vk::Device& device, const vk::PhysicalDevice& physicalDevice, uint32_t graphicsQueueIndex);

  //--------------------------------------------------------------------------------------------------
  // To call on exit
  //
  virtual void destroy();


  //--------------------------------------------------------------------------------------------------
  // Return the surface "screen" for the display
  //
  VkSurfaceKHR getVkSurface(const vk::Instance& instance, GLFWwindow* window);


  //--------------------------------------------------------------------------------------------------
  // Creating the surface for rendering
  //
  virtual void createSwapchain(const vk::SurfaceKHR& surface,
                               uint32_t              width,
                               uint32_t              height,
                               vk::Format            colorFormat = vk::Format::eB8G8R8A8Unorm,
                               vk::Format            depthFormat = vk::Format::eUndefined,
                               bool                  vsync       = false);

  //--------------------------------------------------------------------------------------------------
  // Create the framebuffers in which the image will be rendered
  // - Swapchain need to be created before calling this
  //
  virtual void createFrameBuffers();

  //--------------------------------------------------------------------------------------------------
  // Creating a default render pass, very simple one.
  // Other examples will mostly override this one.
  //
  virtual void createRenderPass();

  //--------------------------------------------------------------------------------------------------
  // Creating an image to be used as depth buffer
  //
  virtual void createDepthBuffer();

  //--------------------------------------------------------------------------------------------------
  // Convenient function to call before rendering
  //
  void prepareFrame();

  //--------------------------------------------------------------------------------------------------
  // Convenient function to call for submitting the rendering command
  //
  virtual void submitFrame();


  //--------------------------------------------------------------------------------------------------
  // When the pipeline is set for using dynamic, this becomes useful
  //
  void setViewport(const vk::CommandBuffer& cmdBuf);

  //--------------------------------------------------------------------------------------------------
  // Window callback when the it is resized
  // - Destroy allocated frames, then rebuild them with the new size
  // - Call onResize() of the derived class
  //
  virtual void onFramebufferSize(int w, int h);


  //--------------------------------------------------------------------------------------------------
  // Window callback when the mouse move
  // - Handling ImGui and a default camera
  //
  virtual void onMouseMotion(int x, int y);

  //--------------------------------------------------------------------------------------------------
  // Window callback when a special key gets hit
  // - Handling ImGui and a default camera
  //
  virtual void onKeyboard(int key, int /*scancode*/, int action, int mods);

  //--------------------------------------------------------------------------------------------------
  // Window callback when a key gets hit
  //
  virtual void onKeyboardChar(unsigned char key);

  //--------------------------------------------------------------------------------------------------
  // Window callback when the mouse button is pressed
  // - Handling ImGui and a default camera
  //
  virtual void onMouseButton(int button, int action, int mods);

  //--------------------------------------------------------------------------------------------------
  // Window callback when the mouse wheel is modified
  // - Handling ImGui and a default camera
  //
  virtual void onMouseWheel(int delta);

  virtual void onFileDrop(const char* filename);

  //--------------------------------------------------------------------------------------------------
  // Initialization of the GUI
  // - Need to be call after the device creation
  //
  void initGUI(uint32_t subpassID = 0);

  //--------------------------------------------------------------------------------------------------
  // Fit the camera to the Bounding box
  //
  void fitCamera(const nvmath::vec3f& boxMin, const nvmath::vec3f& boxMax, bool instantFit = true);

  // Return true if the window is minimized
  bool isMinimized(bool doSleeping = true);

  void setTitle(const std::string& title);

  // GLFW Callback setup
  void        setupGlfwCallbacks(GLFWwindow* window);
  static void framebuffersize_cb(GLFWwindow* window, int w, int h);
  static void mousebutton_cb(GLFWwindow* window, int button, int action, int mods);
  static void cursorpos_cb(GLFWwindow* window, double x, double y);
  static void scroll_cb(GLFWwindow* window, double x, double y);
  static void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods);
  static void char_cb(GLFWwindow* window, unsigned int key);
  static void drop_cb(GLFWwindow* window, int count, const char** paths);
  // GLFW Callback end

  // Set if Nvlink will be used
  void useNvlink(bool useNvlink);

  //--------------------------------------------------------------------------------------------------
  // Getters
  vk::Instance                          getInstance();
  vk::Device                            getDevice();
  vk::PhysicalDevice                    getPhysicalDevice();
  vk::Queue                             getQueue();
  uint32_t                              getQueueFamily();
  vk::CommandPool                       getCommandPool();
  vk::RenderPass                        getRenderPass();
  vk::Extent2D                          getSize();
  vk::PipelineCache                     getPipelineCache();
  vk::SurfaceKHR                        getSurface();
  const std::vector<vk::Framebuffer>&   getFramebuffers();
  const std::vector<vk::CommandBuffer>& getCommandBuffers();
  uint32_t                              getCurFrame() const;
  vk::Format                            getColorFormat() const;
  vk::Format                            getDepthFormat() const;
  bool                                  showGui();

protected:
  uint32_t getMemoryType(uint32_t typeBits, const vk::MemoryPropertyFlags& properties) const;

  // Showing help
  void uiDisplayHelp();

  //--------------------------------------------------------------------------------------------------
  // Called every frame to translate currently pressed keys into camera movement
  //
  void updateCamera();

  //--------------------------------------------------------------------------------------------------

  // Vulkan low level
  vk::Instance       m_instance;
  vk::Device         m_device;
  vk::SurfaceKHR     m_surface;
  vk::PhysicalDevice m_physicalDevice;
  vk::Queue          m_queue;
  uint32_t           m_graphicsQueueIndex{VK_QUEUE_FAMILY_IGNORED};
  vk::CommandPool    m_cmdPool;
  vk::DescriptorPool m_imguiDescPool;

  // Drawing/Surface
  nvvk::SwapChain                m_swapChain;
  std::vector<vk::Framebuffer>   m_framebuffers;      // All framebuffers, correspond to the Swapchain
  std::vector<vk::CommandBuffer> m_commandBuffers;    // Command buffer per nb element in Swapchain
  std::vector<vk::Fence>         m_waitFences;        // Fences per nb element in Swapchain
  vk::Image                      m_depthImage;        // Depth/Stencil
  vk::DeviceMemory               m_depthMemory;       // Depth/Stencil
  vk::ImageView                  m_depthView;         // Depth/Stencil
  vk::RenderPass                 m_renderPass;        // Base render pass
  vk::Extent2D                   m_size{0, 0};        // Size of the window
  vk::PipelineCache              m_pipelineCache;     // Cache for pipeline/shaders
  bool                           m_vsync{false};      // Swapchain with vsync
  bool                           m_useNvlink{false};  // NVLINK usage
  GLFWwindow*                    m_window{nullptr};   // GLFW Window

  // Surface buffer formats
  vk::Format m_colorFormat{vk::Format::eB8G8R8A8Unorm};
  vk::Format m_depthFormat{vk::Format::eUndefined};

  // Camera manipulators
  nvh::CameraManipulator::Inputs m_inputs;  // Mouse button pressed
  std::set<int>                  m_keys;    // Keyboard pressed

  nvh::Stopwatch m_timer;  // measure time from frame to frame to base camera movement on

  // Other
  bool m_showHelp{false};  // Show help, pressing
  bool m_show_gui{true};
};

}  // namespace nvvk
