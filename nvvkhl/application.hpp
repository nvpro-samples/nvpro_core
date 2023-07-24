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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <memory>
#include <vulkan/vulkan_core.h>

#include "nvmath/nvmath.h"
#include "nvvk/context_vk.hpp"

#include "imgui.h"

/*******************************************************************************************************************

  The Application class is what is creating a GLFW window with Vulkan support and ImGui for UI and interaction.
  
  To use the application, fill the ApplicationCreateInfo with all the information, including the Vulkan
  creation information (nvvk::ContextCreateInfo). 
 
  The application itself does not render per se. It contains control buffers for the images in flight,
  it calls ImGui rendering for Vulkan, but that's it. Note that none of the samples render
  directly into the swapchain. Instead, they render into an image, and the image is displayed in the ImGui window
  window called "Viewport".
  
  Application elements must be created to render scenes or add "elements" to the application.  Several elements 
  can be added to an application, and each of them will be called during the frame. This allows the application 
  to be divided into smaller parts, or to reuse elements in various samples. For example, there is an element 
  that adds a default menu (File/Tools), another that changes the window title with FPS, the resolution, and there
  is also an element for our automatic tests.
    
  Each added element will be called in a frame, see the IAppElement interface for information on virtual functions.
  Basically there is a call to create and destroy, a call to render the user interface and a call to render the 
  frame with the command buffer.

  Note: order of Elements can be important if one depends on the other. For example, the ElementCamera should
        be added before the rendering sample, such that its matrices are updated before pulled by the renderer.
 
*******************************************************************************************************************/

// Forward declarations
struct GLFWwindow;
struct ImGui_ImplVulkanH_Window;

namespace nvvkhl {
// Forward declarations
struct ContextCreateInfo;
class Context;
struct IAppElement;

// Information for creating the application
struct ApplicationCreateInfo
{
  std::string                  name{"Vulkan App"};  // Name of the GLFW
  int32_t                      width{-1};           // Width of the Window
  int32_t                      height{-1};          // Height of the window
  bool                         vSync{true};         // Is V-Sync on by default?
  bool                         useMenu{true};       // Is the application will have a menubar?
  bool                         useDockMenu{false};  // Is there an extra menubar ?
  nvvk::ContextCreateInfo      vkSetup{};           // Vulkan creation context information (see nvvk::Context)
  std::vector<int>             ignoreDbgMessages;   // Turn off debug messages
  ImVec4                       clearColor{0.F, 0.F, 0.F, 1.F};
  std::function<void(ImGuiID)> dockSetup;  // Allow to configure the dock layout
};


class Application
{
public:
  explicit Application(ApplicationCreateInfo& info);
  virtual ~Application();

  // Application control
  void run();    // Run indefinitely until close is requested
  void close();  // Stopping the application

  // Adding engines
  template <typename T>
  void addElement();
  void addElement(const std::shared_ptr<IAppElement>& layer);

  // Safely freeing up resources
  static void submitResourceFree(std::function<void()>&& func);

  // Utilities
  void setViewport(const VkCommandBuffer& cmd);  // Set viewport and scissor the the size of the viewport
  bool isVsync() const;                          // Return true if V-Sync is on
  void setVsync(bool v);                         // Set V-Sync on or off
  void setViewportClearColor(ImVec4 col) { m_clearColor = col; }

  // Sync for special cases
  void addWaitSemaphore(const VkSemaphoreSubmitInfoKHR& wait);
  void addSignalSemaphore(const VkSemaphoreSubmitInfoKHR& signal);

  VkCommandBuffer createTempCmdBuffer();
  void            submitAndWaitTempCmdBuffer(VkCommandBuffer cmd);

  // Getters
  inline std::shared_ptr<nvvk::Context> getContext() { return m_context; }  // Return the Vulkan context
  inline VkCommandPool     getCommandPool() { return m_cmdPool; }  // Return command pool to create command buffers
  inline GLFWwindow*       getWindowHandle() { return m_windowHandle; }  // Return the handle of the Window
  inline const VkExtent2D& getViewportSize() { return m_viewportSize; }  // Return the size of the rendering viewport
  inline const VkExtent2D& getWindowSize() { return m_windowSize; }      // Return the size of the window
  inline VkDevice          getDevice() { return m_context->m_device; }
  inline VkInstance        getInstance() { return m_context->m_instance; }
  inline VkPhysicalDevice  getPhysicalDevice() { return m_context->m_physicalDevice; }
  inline const nvvk::Context::Queue& getQueueGCT() { return m_context->m_queueGCT; }
  inline const nvvk::Context::Queue& getQueueC() { return m_context->m_queueC; }
  inline const nvvk::Context::Queue& getQueueT() { return m_context->m_queueT; }

  void onFileDrop(const char* filename);

private:
  void init(ApplicationCreateInfo& info);
  void shutdown();
  void frameRender();
  void framePresent();
  void setupVulkanWindow(VkSurfaceKHR surface, int width, int height);
  void createDock() const;
  void setPresentMode(VkPhysicalDevice physicalDevice, ImGui_ImplVulkanH_Window* wd);
  void createDescriptorPool();

  std::shared_ptr<nvvk::Context>            m_context;
  std::vector<std::shared_ptr<IAppElement>> m_elements;

  bool        m_running{false};           // Is currently running
  bool        m_useMenubar{true};         // Will use a menubar
  bool        m_useDockMenubar{false};    // Will use an exta menubar
  bool        m_vsyncWanted{true};        // Wanting swapchain with vsync
  bool        m_vsyncSet{true};           // Vsync currently set
  int         m_minImageCount{2};         // Nb frames in-flight
  bool        m_swapChainRebuild{false};  // Need to rebuild swapchain?
  std::string m_iniFilename;              // Holds on .ini name
  ImVec4      m_clearColor{0.0F, 0.0F, 0.0F, 1.0F};

  VkCommandPool          m_cmdPool{VK_NULL_HANDLE};         //
  VkPipelineCache        m_pipelineCache{VK_NULL_HANDLE};   // Cache for pipeline/shaders
  VkDescriptorPool       m_descriptorPool{VK_NULL_HANDLE};  //
  GLFWwindow*            m_windowHandle{nullptr};           // GLFW Window
  VkExtent2D             m_viewportSize{0, 0};              // Size of the viewport
  VkExtent2D             m_windowSize{0, 0};                // Size of the window
  VkAllocationCallbacks* m_allocator{nullptr};

  std::vector<VkSemaphoreSubmitInfoKHR> m_waitSemaphores;    // Possible extra frame wait semaphores
  std::vector<VkSemaphoreSubmitInfoKHR> m_signalSemaphores;  // Possible extra frame signal semaphores

  std::unique_ptr<ImGui_ImplVulkanH_Window> m_mainWindowData;

  //--
  static uint32_t                                        m_currentFrameIndex;  // (eg. 0, 1, 2, 0, 1, 2)
  static std::vector<std::vector<std::function<void()>>> m_resourceFreeQueue;  // Queue of functions to free resources

  //--
  std::function<void(ImGuiID)> m_dockSetup;
};

template <typename T>
void Application::addElement()
{
  static_assert(std::is_base_of<IAppElement, T>::value, "Type is not subclass of IApplication");
  m_elements.emplace_back(std::make_shared<T>())->onAttach(this);
}


/*
*  Interface for application elements
*/
struct IAppElement
{
  // Interface
  virtual void onAttach(Application* app) {}                 // Called once at start
  virtual void onDetach() {}                                 // Called before destroying the application
  virtual void onResize(uint32_t width, uint32_t height) {}  // Called when the viewport size is changing
  virtual void onUIRender() {}                               // Called for anything related to UI
  virtual void onUIMenu() {}                                 // This is the menubar to create
  virtual void onRender(VkCommandBuffer cmd) {}              // For anything to render within a frame
  virtual void onFileDrop(const char* filename) {}           // For when a file is dragged on top of the window

  virtual ~IAppElement() = default;
};


}  // namespace nvvkhl
