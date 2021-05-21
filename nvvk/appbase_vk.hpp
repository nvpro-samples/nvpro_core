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

#pragma once

#include "vulkan/vulkan_core.h"

// Imgui
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "imgui/imgui_camera_widget.h"
#include "imgui/imgui_helper.h"

// Utilities
#include "nvh/cameramanipulator.hpp"
#include "swapchain_vk.hpp"

#ifdef LINUX
#include <unistd.h>
#endif

// GLFW
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include <cmath>
#include <set>
#include <vector>

namespace nvvk {

//--------------------------------------------------------------------------------------------------
/**

# class nvvk::AppBase

The framework comes with a few `App???` classes, these can serve as base classes for various samples.
They might differ a bit in setup and functionality, but in principle aid the setup of context and window,
as well as some common event processing.

The nvvk::AppBase serves as the base class for many ray tracing examples and makes use of the Vulkan C++ API (`vulkan.hpp`).
It does the basics for Vulkan, by holding a reference to the instance and device, but also comes with optional default setups 
for the render passes and the swapchain.

## Usage

An example will derive from this class:

~~~~ C++
class MyExample : public AppBase 
{
};
~~~~

## Setup

In the `main()` of an application,  call `setup()` which is taking a Vulkan instance, device, physical device, 
and a queue family index.  Setup copies the given Vulkan handles into AppBase, and query the 0th VkQueue of the 
specified family, which must support graphics operations and drawing to the surface passed to createSurface.
Furthermore, it is creating a VkCommandPool plus it
will initialize all Vulkan extensions for the C++ API (vulkan.hpp). 
See: [VULKAN_HPP_DEFAULT_DISPATCHER](https://github.com/KhronosGroup/Vulkan-Hpp#vulkan_hpp_default_dispatcher)

Prior to calling setup, if you are using the `nvvk::Context` class to create and initialize Vulkan instances,
you may want to create a VkSurfaceKHR from the window (glfw for example) and call `setGCTQueueWithPresent()`.
This will make sure the m_queueGCT queue of nvvk::Context can draw to the surface, and m_queueGCT.familyIndex 
will meet the requirements of setup().

Creating the swapchain for displaying. Arguments are
width and height, color and depth format, and vsync on/off. Defaults will create the best format for the surface.


Creating framebuffers has a dependency on the renderPass and depth buffer. All those are virtual and can be overridden
in a sample, but default implementation exist.

- createDepthBuffer: creates a 2D depth/stencil image
- createRenderPass : creates a color/depth pass and clear both buffers.

Here is the dependency order:

~~~~C++
  example.createDepthBuffer();
  example.createRenderPass();
  example.createFrameBuffers();
~~~~


The nvvk::Swapchain will create n images, typically 3. With this information, AppBase is also creating 3 VkFence, 
3 VkCommandBuffer and 3 VkFrameBuffer.

### Frame Buffers

The created frame buffers are “display” frame buffers,  made to be presented on screen. The frame buffers will be created 
using one of the images from swapchain, and a depth buffer. There is only one depth buffer because that resource is not 
used simultaneously. For example, when we clear the depth buffer, it is not done immediately, but done through a command 
buffer, which will be executed later. 


**Note**: the imageView(s) are part of the swapchain. 

### Command Buffers

AppBase works with 3 “frame command buffers”. Each frame is filling a command buffer and gets submitted, one after the 
other. This is a design choice that can be debated, but makes it simple. I think it is still possible to submit other 
command buffers in a frame, but those command buffers will have to be submitted before the “frame” one. The “frame” 
command buffer when submitted with submitFrame, will use the current fence.

### Fences

There are as many fences as there are images in the swapchain. At the beginning of a frame, we call prepareFrame(). 
This is calling the acquire() from nvvk::SwapChain and wait until the image is available. The very first time, the 
fence will not stop, but later it will wait until the submit is completed on the GPU. 



## ImGui

If the application is using Dear ImGui, there are convenient functions for initializing it and
setting the callbacks (glfw). The first one to call is `initGUI(0)`, where the argument is the subpass
it will be using. Default is 0, but if the application creates a renderpass with multi-sampling and
resolves in the second subpass, this makes it possible.

## Glfw Callbacks

Call `setupGlfwCallbacks(window)` to have all the window callback: key, mouse, window resizing.
By default AppBase will handle resizing of the window and will recreate the images and framebuffers. 
If a sample needs to be aware of the resize, it can implement `onResize(width, height)`.

To handle the callbacks in Imgui, call `ImGui_ImplGlfw_InitForVulkan(window, true)`, where true 
will handle the default ImGui callback .

**Note**: All the methods are virtual and can be overloaded if they are not doing the typical setup. 

~~~~ C++
MyExample example;

const vk::SurfaceKHR surface = example.getVkSurface(vkctx.m_instance, window);
vkctx.setGCTQueueWithPresent(surface);

example.setup(vkctx.m_instance, vkctx.m_device, vkctx.m_physicalDevice, vkctx.m_queueGCT.familyIndex);
example.createSurface(surface, SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT);
example.createDepthBuffer();
example.createFrameBuffers();
example.createRenderPass();
example.initGUI(0);
example.setupGlfwCallbacks(window);

ImGui_ImplGlfw_InitForVulkan(window, true);
~~~~

## Drawing loop

The drawing loop in the main() is the typicall loop you will find in glfw examples. Note that
AppBase has a convenient function to tell if the window is minimize, therefore not doing any 
work and contain a sleep(), so the CPU is not going crazy. 


~~~~ C++
// Window system loop
while(!glfwWindowShouldClose(window))
{
  glfwPollEvents();
  if(example.isMinimized())
    continue;

  example.display();  // infinitely drawing
}
~~~~

## Display

A typical display() function will need the following: 

* Acquiring the next image: `prepareFrame()`
* Get the command buffer for the frame. There are n command buffers equal to the number of in-flight frames.
* Clearing values
* Start rendering pass
* Drawing
* End rendering
* Submitting frame to display

~~~~ C++
void MyExample::display()
{
  // Acquire 
  prepareFrame();

  // Command buffer for current frame
  const vk::CommandBuffer& cmdBuff = m_commandBuffers[getCurFrame()];
  cmdBuff.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

  // Clearing values
  vk::ClearValue clearValues[2];
  clearValues[0].setColor(std::array<float, 4>({0.1f, 0.1f, 0.4f, 0.f}));
  clearValues[1].setDepthStencil({1.0f, 0});

  // Begin rendering
  vk::RenderPassBeginInfo renderPassBeginInfo{m_renderPass, m_framebuffers[getCurFrame()], {{}, m_size}, 2, clearValues};
  cmdBuff.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  
  // .. draw scene ...

  // Draw UI
  ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(),cmdBuff)

  // End rendering
  cmdBuff.endRenderPass();

  // End of the frame and present the one which is ready
  cmdBuff.end();
  submitFrame();
}
~~~~~

## Closing

Finally, all resources can be destroyed by calling `destroy()` at the end of main().

~~~~ C++
  example.destroy();
~~~~

*/

class AppBaseVk
{
public:
  AppBaseVk()          = default;
  virtual ~AppBaseVk() = default;

  virtual void onResize(int /*w*/, int /*h*/){};  // To implement when the size of the window change
  virtual void setup(const VkInstance& instance, const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t graphicsQueueIndex);
  virtual void destroy();
  VkSurfaceKHR getVkSurface(const VkInstance& instance, GLFWwindow* window);
  virtual void createSwapchain(const VkSurfaceKHR& surface,
                               uint32_t            width,
                               uint32_t            height,
                               VkFormat            colorFormat = VK_FORMAT_B8G8R8A8_UNORM,
                               VkFormat            depthFormat = VK_FORMAT_UNDEFINED,
                               bool                vsync       = false);
  virtual void createFrameBuffers();
  virtual void createRenderPass();
  virtual void createDepthBuffer();
  virtual void prepareFrame();
  virtual void submitFrame();
  void         setViewport(const VkCommandBuffer& cmdBuf);
  virtual void onFramebufferSize(int w, int h);
  virtual void onMouseMotion(int x, int y);
  virtual void onKeyboard(int key, int scancode, int action, int mods);
  virtual void onKeyboardChar(unsigned char key);
  virtual void onMouseButton(int button, int action, int mods);
  virtual void onMouseWheel(int delta);
  virtual void onFileDrop(const char* filename) {}
  void         initGUI(uint32_t subpassID = 0);
  void         fitCamera(const nvmath::vec3f& boxMin, const nvmath::vec3f& boxMax, bool instantFit = true);
  bool         isMinimized(bool doSleeping = true);
  void         setTitle(const std::string& title) { glfwSetWindowTitle(m_window, title.c_str()); }

  // GLFW Callback setup
  void        setupGlfwCallbacks(GLFWwindow* window);
  static void framebuffersize_cb(GLFWwindow* window, int w, int h)
  {
    auto app = reinterpret_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
    app->onFramebufferSize(w, h);
  }
  static void mousebutton_cb(GLFWwindow* window, int button, int action, int mods)
  {
    auto app = reinterpret_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
    app->onMouseButton(button, action, mods);
  }
  static void cursorpos_cb(GLFWwindow* window, double x, double y)
  {
    auto app = reinterpret_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
    app->onMouseMotion(static_cast<int>(x), static_cast<int>(y));
  }
  static void scroll_cb(GLFWwindow* window, double x, double y)
  {
    auto app = reinterpret_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
    app->onMouseWheel(static_cast<int>(y));
  }
  static void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods)
  {
    auto app = reinterpret_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
    app->onKeyboard(key, scancode, action, mods);
  }
  static void char_cb(GLFWwindow* window, unsigned int key)
  {
    auto app = reinterpret_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
    app->onKeyboardChar(key);
  }
  static void drop_cb(GLFWwindow* window, int count, const char** paths)
  {
    auto app = reinterpret_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
    int  i;
    for(i = 0; i < count; i++)
      app->onFileDrop(paths[i]);
  }
  // GLFW Callback end

  // Set if Nvlink will be used
  void useNvlink(bool useNvlink) { m_useNvlink = useNvlink; }

  //--------------------------------------------------------------------------------------------------
  // Getters
  VkInstance                          getInstance() { return m_instance; }
  VkDevice                            getDevice() { return m_device; }
  VkPhysicalDevice                    getPhysicalDevice() { return m_physicalDevice; }
  VkQueue                             getQueue() { return m_queue; }
  uint32_t                            getQueueFamily() { return m_graphicsQueueIndex; }
  VkCommandPool                       getCommandPool() { return m_cmdPool; }
  VkRenderPass                        getRenderPass() { return m_renderPass; }
  VkExtent2D                          getSize() { return m_size; }
  VkPipelineCache                     getPipelineCache() { return m_pipelineCache; }
  VkSurfaceKHR                        getSurface() { return m_surface; }
  const std::vector<VkFramebuffer>&   getFramebuffers() { return m_framebuffers; }
  const std::vector<VkCommandBuffer>& getCommandBuffers() { return m_commandBuffers; }
  uint32_t                            getCurFrame() const { return m_swapChain.getActiveImageIndex(); }
  VkFormat                            getColorFormat() const { return m_colorFormat; }
  VkFormat                            getDepthFormat() const { return m_depthFormat; }
  bool                                showGui() { return m_show_gui; }

protected:
  uint32_t getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties) const;
  void     uiDisplayHelp();  // Showing help

  // Vulkan low level
  VkInstance       m_instance{};
  VkDevice         m_device{};
  VkSurfaceKHR     m_surface{};
  VkPhysicalDevice m_physicalDevice{};
  VkQueue          m_queue{VK_NULL_HANDLE};
  uint32_t         m_graphicsQueueIndex{VK_QUEUE_FAMILY_IGNORED};
  VkCommandPool    m_cmdPool{VK_NULL_HANDLE};
  VkDescriptorPool m_imguiDescPool{VK_NULL_HANDLE};

  // Drawing/Surface
  nvvk::SwapChain              m_swapChain;
  std::vector<VkFramebuffer>   m_framebuffers;                   // All framebuffers, correspond to the Swapchain
  std::vector<VkCommandBuffer> m_commandBuffers;                 // Command buffer per nb element in Swapchain
  std::vector<VkFence>         m_waitFences;                     // Fences per nb element in Swapchain
  VkImage                      m_depthImage{VK_NULL_HANDLE};     // Depth/Stencil
  VkDeviceMemory               m_depthMemory{VK_NULL_HANDLE};    // Depth/Stencil
  VkImageView                  m_depthView{VK_NULL_HANDLE};      // Depth/Stencil
  VkRenderPass                 m_renderPass{VK_NULL_HANDLE};     // Base render pass
  VkExtent2D                   m_size{0, 0};                     // Size of the window
  VkPipelineCache              m_pipelineCache{VK_NULL_HANDLE};  // Cache for pipeline/shaders
  bool                         m_vsync{false};                   // Swapchain with vsync
  bool                         m_useNvlink{false};               // NVLINK usage
  GLFWwindow*                  m_window{nullptr};                // GLFW Window

  // Surface buffer formats
  VkFormat m_colorFormat{VK_FORMAT_B8G8R8A8_UNORM};
  VkFormat m_depthFormat{VK_FORMAT_UNDEFINED};

  // Camera manipulators
  nvh::CameraManipulator::Inputs m_inputs;  // Mouse button pressed
  std::set<int>                  m_keys;    // Keyboard pressed

  // Other
  bool m_showHelp{false};  // Show help, pressing
  bool m_show_gui{true};
};


}  // namespace nvvk
