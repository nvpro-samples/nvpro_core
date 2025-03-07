/*
 * Copyright (c) 2014-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <vulkan/vulkan.h>
#include <functional>
#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "imgui.h"
#include "app_swapchain_vk.hpp"
#include "imgui/imgui_handler.h"


/* @DOC_START
# class nvapp::Application

To use the application, 
* Fill the ApplicationCreateInfo with all the information, 

Example:
````cpp
    nvapp::ApplicationCreateInfo appInfo;
    appInfo.name           = "Minimal Test";
    appInfo.width          = 800;
    appInfo.height         = 600;
    appInfo.vSync          = false;
    appInfo.instance       = vkContext.getInstance();
    appInfo.physicalDevice = vkContext.getPhysicalDevice();
    appInfo.device         = vkContext.getDevice();
    appInfo.queues         = vkContext.getQueueInfos();
 ```

* Attach elements to the application, the main rendering, and others like, camera, etc.
* Call run() to start the application.
*
* The application will create the window and the ImGui context.

Worth notice
* ::init() : will create the GLFW window, call nvvk::context for the creation of the 
              Vulkan context, initialize ImGui , create the surface and window (::setupVulkanWindow)  
* ::shutdown() : the opposite of init
* ::run() : while running, render the frame and present the frame. Check for resize, minimize window 
              and other changes. In the loop, it will call some functions for each 'element' that is connected.
              onUIRender, onUIMenu, onRender. See IApplication for details.
* The Application is a singleton, and the main loop is inside the run() function.
* The Application is the owner of the elements, and it will call the onRender, onUIRender, onUIMenu
    for each element that is connected to it.
* The Application is the owner of the Vulkan context, and it will create the surface and window.
* The Application is the owner of the ImGui context, and it will create the dockspace and the main menu.
* The Application is the owner of the GLFW window, and it will create the window and handle the events.


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


## Docking

The layout can be customized by providing a function to the ApplicationCreateInfo.

Example:
````
    // Setting up the layout of the application
    appInfo.dockSetup = [](ImGuiID viewportID) {
      ImGuiID settingID = ImGui::DockBuilderSplitNode(viewportID, ImGuiDir_Right, 0.2F, nullptr, &viewportID);
      ImGui::DockBuilderDockWindow("Settings", settingID);
    };
````



@DOC_END */


namespace nvvkhl {
// Forward declarations
class Application;


//-------------------------------------------------------------------------------------------------
// Interface for application elements
struct IAppElement
{
  // Interface
  virtual void                onAttach(Application* app) {}                 // Called once at start
  virtual void                onDetach() {}                                 // Called before destroying the application
  [[deprecated]] virtual void onResize(uint32_t width, uint32_t height) {}  // Called when the viewport size is changing
  virtual void                onResize(VkCommandBuffer cmd, const VkExtent2D& size)
  {
    onResize(size.width, size.height);  // (LEGACY) need to be removed
  }  // Called when the viewport size is changing
  virtual void onUIRender() {}                      // Called for anything related to UI
  virtual void onUIMenu() {}                        // This is the menubar to create
  virtual void onRender(VkCommandBuffer cmd) {}     // For anything to render within a frame
  virtual void onFileDrop(const char* filename) {}  // For when a file is dragged on top of the window
  virtual void onLastHeadlessFrame() {};            // Called at the end of the last frame in headless mode

  virtual ~IAppElement() = default;
};

// Application creation info
struct ApplicationCreateInfo
{
  // General
  std::string name{"Vulkan_App"};  // Application name

  // Vulkan
  VkInstance                     instance{VK_NULL_HANDLE};        // Vulkan instance
  VkDevice                       device{VK_NULL_HANDLE};          // Logical device
  VkPhysicalDevice               physicalDevice{VK_NULL_HANDLE};  // Physical device
  std::vector<nvvkhl::QueueInfo> queues;                          // Queue family and properties (0: Graphics)

  // GLFW
  glm::uvec2 windowSize{0, 0};  // Window size (width, height) or Viewport size (headless)
  bool       vSync{true};       // Enable V-Sync by default

  // UI
  bool                         useMenu{true};                 // Include a menubar
  bool                         hasUndockableViewport{false};  // Allow floating windows
  std::function<void(ImGuiID)> dockSetup;                     // Dock layout setup
  ImGuiConfigFlags             imguiConfigFlags{ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable};

  // Headless
  bool     headless{false};        // Run without a window
  uint32_t headlessFrameCount{1};  // Frames to render in headless mode
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
  void addElement(const std::shared_ptr<IAppElement>& layer);

  // Safely freeing up resources
  void submitResourceFree(std::function<void()>&& func);

  // Utilities
  bool isVsync() const { return m_vsyncWanted; }  // Return true if V-Sync is on
  void setVsync(bool v);                          // Set V-Sync on or off
  bool isHeadless() const { return m_headless; }  // Return true if headless

  // Following three functions affect the preparation of the current frame's submit info.
  // Content is appended to vectors that are reset every frame
  void addWaitSemaphore(const VkSemaphoreSubmitInfoKHR& wait);
  void addSignalSemaphore(const VkSemaphoreSubmitInfoKHR& signal);
  // these command buffers are enqueued before the command buffer that is provided `onRender(cmd)`
  void prependCommandBuffer(const VkCommandBufferSubmitInfoKHR& cmd);

  // Set viewport and scissor the the size of the viewport
  void setViewport(const VkCommandBuffer& cmd);

  // Utility to create a temporary command buffer
  VkCommandBuffer createTempCmdBuffer() const;
  void            submitAndWaitTempCmdBuffer(VkCommandBuffer cmd);

  // Getters
  inline VkInstance               getInstance() const { return m_instance; }
  inline VkPhysicalDevice         getPhysicalDevice() const { return m_physicalDevice; }
  inline VkDevice                 getDevice() const { return m_device; }
  inline const nvvkhl::QueueInfo& getQueue(uint32_t index) const { return m_queues[index]; }
  inline VkCommandPool            getCommandPool() const { return m_transientCmdPool; }
  inline VkDescriptorPool         getDescriptorPool() const { return m_descriptorPool; }
  inline const VkExtent2D&        getViewportSize() const { return m_viewportSize; }
  inline const VkExtent2D&        getWindowSize() const { return m_windowSize; }
  inline GLFWwindow*              getWindowHandle() const { return m_windowHandle; }
  inline uint32_t                 getFrameCycleIndex() const { return m_frameRingCurrent; }
  inline uint32_t                 getFrameCycleSize() const { return uint32_t(m_frameData.size()); }

  // Simulate the Drag&Drop of a file
  void onFileDrop(const char* filename);


  void screenShot(const std::string& filename, int quality = 90);  // Delay the screen shot to the end of the frame

  // Utility functions
  void saveImageToFile(VkImage srcImage, VkExtent2D imageSize, const std::string& filename, int quality = 90);
  //
  // Utility to save the image to a file, useful for headless mode
  void imageToRgba8Linear(VkCommandBuffer  cmd,
                          VkDevice         device,
                          VkPhysicalDevice physicalDevice,
                          VkImage          srcImage,
                          VkExtent2D       size,
                          VkImage&         dstImage,
                          VkDeviceMemory&  dstImageMemory);
  void saveImageToFile(VkDevice           device,
                       VkImage            linearImage,
                       VkDeviceMemory     imageMemory,
                       VkExtent2D         imageSize,
                       const std::string& filename,
                       int                quality = 90);


private:
  void            init(ApplicationCreateInfo& info);
  void            initGlfw(ApplicationCreateInfo& info);
  void            shutdown();
  void            createTransientCommandPool();
  void            createFrameSubmission(uint32_t numFrames);
  void            initImGui(ImGuiConfigFlags configFlags);
  void            createDescriptorPool();
  void            onViewportSizeChange(VkExtent2D size);
  void            headlessRun();
  VkCommandBuffer beginFrame();
  void            drawFrame(VkCommandBuffer cmd);
  void            endFrame(VkCommandBuffer cmd);
  void            presentFrame();
  void            beginDynamicRenderingToSwapchain(VkCommandBuffer cmd) const;
  void            endDynamicRenderingToSwapchain(VkCommandBuffer cmd);
  void            saveScreenShot(const std::string& filename, int quality);  // Immediately save the frame
  void            resetFreeQueue(uint32_t size);

  std::vector<std::shared_ptr<IAppElement>> m_elements;  // List of application elements to be called

  bool        m_useMenubar{true};             // Will use a menubar
  bool        m_useDockMenubar{false};        // Will use an extra menubar
  bool        m_vsyncWanted{true};            // Wanting swapchain with vsync
  bool        m_vsyncSet{true};               // Vsync currently set
  int         m_minImageCount{2};             // Nb frames in-flight
  bool        m_swapChainRebuild{false};      // Need to rebuild swapchain?
  bool        m_hasUndockableViewport{true};  // Using a default viewport
  std::string m_iniFilename;                  // Holds on .ini name

  // Vulkan resources
  VkInstance                     m_instance{VK_NULL_HANDLE};
  VkPhysicalDevice               m_physicalDevice{VK_NULL_HANDLE};
  VkDevice                       m_device{VK_NULL_HANDLE};
  std::vector<nvvkhl::QueueInfo> m_queues{};            // All queues, first one should be a graphics queue
  VkSurfaceKHR                   m_surface{};           // The window surface
  VkCommandPool                  m_transientCmdPool{};  // The command pool
  VkDescriptorPool               m_descriptorPool{};    // Application descriptor pool

  // Frame resources and synchronization (Swapchain, Command buffers, Semaphores, Fences)
  AppSwapchain m_swapchain;
  struct FrameData
  {
    VkCommandPool   cmdPool{};      // Command pool for recording commands for this frame
    VkCommandBuffer cmdBuffer{};    // Command buffer containing the frame's rendering commands
    uint64_t        frameNumber{};  // Timeline value for synchronization (increases each frame)
  };
  std::vector<FrameData> m_frameData{};    // Collection of per-frame resources to support multiple frames in flight
  VkSemaphore m_frameTimelineSemaphore{};  // Timeline semaphore used to synchronize CPU submission with GPU completion
  uint32_t m_frameRingCurrent{0};  // Current frame index in the ring buffer (cycles through available frames) : static for resource free queue

  // Fine control over the frame submission
  std::vector<VkSemaphoreSubmitInfoKHR>     m_waitSemaphores;    // Possible extra frame wait semaphores
  std::vector<VkSemaphoreSubmitInfoKHR>     m_signalSemaphores;  // Possible extra frame signal semaphores
  std::vector<VkCommandBufferSubmitInfoKHR> m_commandBuffers;    // Possible extra frame command buffers


  GLFWwindow* m_windowHandle{nullptr};  // GLFW Window
  VkExtent2D  m_viewportSize{0, 0};     // Size of the viewport
  VkExtent2D  m_windowSize{0, 0};       // Size of the window

  //--
  std::vector<std::vector<std::function<void()>>> m_resourceFreeQueue;  // Queue of functions to free resources

  //--
  std::function<void(ImGuiID)> m_dockSetup;  // Function to setup the docking

  bool        m_headless{false};
  uint32_t    m_headlessFrameCount{1};
  bool        m_screenShotRequested = false;
  int         m_screenShotFrame     = 0;
  std::string m_screenShotFilename;

  // Use for persist the data
  ImGuiH::SettingsHandler m_settingsHandler;
  glm::ivec2              m_winPos{};
  glm::ivec2              m_winSize{};
};


}  // namespace nvvkhl
