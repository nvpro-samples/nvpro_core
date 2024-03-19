## Table of Contents
- [alloc_vma.hpp](#alloc_vmahpp)
- [appbase_vk.hpp](#appbase_vkhpp)
- [appbase_vkpp.hpp](#appbase_vkpphpp)
- [application.hpp](#applicationhpp)
- [element_benchmark_parameters.hpp](#element_benchmark_parametershpp)
- [element_camera.hpp](#element_camerahpp)
- [element_dbgprintf.hpp](#element_dbgprintfhpp)
- [element_gui.hpp](#element_guihpp)
- [element_inspector.hpp](#element_inspectorhpp)
- [element_logger.hpp](#element_loggerhpp)
- [element_nvml.hpp](#element_nvmlhpp)
- [element_profiler.hpp](#element_profilerhpp)
- [element_testing.hpp](#element_testinghpp)
- [gbuffer.hpp](#gbufferhpp)
- [glsl_compiler.hpp](#glsl_compilerhpp)
- [gltf_scene.hpp](#gltf_scenehpp)
- [gltf_scene_rtx.hpp](#gltf_scene_rtxhpp)
- [gltf_scene_vk.hpp](#gltf_scene_vkhpp)
- [hdr_env.hpp](#hdr_envhpp)
- [hdr_env_dome.hpp](#hdr_env_domehpp)
- [pipeline_container.hpp](#pipeline_containerhpp)
- [scene_camera.hpp](#scene_camerahpp)
- [sky.hpp](#skyhpp)
- [tonemap_postprocess.hpp](#tonemap_postprocesshpp)

## alloc_vma.hpp
### class nvvkhl::AllocVma


>  This class is an element of the application that is responsible for the resource allocation. It is using the `VMA` library to allocate buffers, images and acceleration structures.

This allocator uses VMA (Vulkan Memory Allocator) to allocate buffers, images and acceleration structures. It is using the `nvvk::ResourceAllocator` to manage the allocation and deallocation of the resources.


## appbase_vk.hpp

### class nvvkhl::AppBaseVk


nvvkhl::AppBaseVk is used in a few samples, can serve as base class for various needs.
They might differ a bit in setup and functionality, but in principle aid the setup of context and window,
as well as some common event processing.

The nvvkhl::AppBaseVk serves as the base class for many ray tracing examples and makes use of the Vulkan C API.
It does the basics for Vulkan, by holding a reference to the instance and device, but also comes with optional default setups
for the render passes and the swapchain.

## Usage

An example will derive from this class:

```cpp
class VkSample : public AppBaseVk
{
};
```

## Setup

In the `main()` of an application,  call `setup()` which is taking a Vulkan instance, device, physical device,
and a queue family index.  Setup copies the given Vulkan handles into AppBase, and query the 0th VkQueue of the
specified family, which must support graphics operations and drawing to the surface passed to createSurface.
Furthermore, it is creating a VkCommandPool.

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

```cpp
vkSample.createDepthBuffer();
vkSample.createRenderPass();
vkSample.createFrameBuffers();
```cpp


The nvvk::Swapchain will create n images, typically 3. With this information, AppBase is also creating 3 VkFence,
3 VkCommandBuffer and 3 VkFrameBuffer.

### Frame Buffers

The created frame buffers are *display* frame buffers,  made to be presented on screen. The frame buffers will be created
using one of the images from swapchain, and a depth buffer. There is only one depth buffer because that resource is not
used simultaneously. For example, when we clear the depth buffer, it is not done immediately, but done through a command
buffer, which will be executed later.


**Note**: the imageView(s) are part of the swapchain.

### Command Buffers

AppBase works with 3 *frame command buffers*. Each frame is filling a command buffer and gets submitted, one after the
other. This is a design choice that can be debated, but makes it simple. I think it is still possible to submit other
command buffers in a frame, but those command buffers will have to be submitted before the *frame* one. The *frame*
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

```cpp
// Create example
VulkanSample vkSample;

// Window need to be opened to get the surface on which to draw
const VkSurfaceKHR surface = vkSample.getVkSurface(vkctx.m_instance, window);
vkctx.setGCTQueueWithPresent(surface);

vkSample.setup(vkctx.m_instance, vkctx.m_device, vkctx.m_physicalDevice, vkctx.m_queueGCT.familyIndex);
vkSample.createSwapchain(surface, SAMPLE_WIDTH, SAMPLE_HEIGHT);
vkSample.createDepthBuffer();
vkSample.createRenderPass();
vkSample.createFrameBuffers();
vkSample.initGUI(0);
vkSample.setupGlfwCallbacks(window);

ImGui_ImplGlfw_InitForVulkan(window, true);
```

## Drawing loop

The drawing loop in the main() is the typicall loop you will find in glfw examples. Note that
AppBase has a convenient function to tell if the window is minimize, therefore not doing any
work and contain a sleep(), so the CPU is not going crazy.


```cpp
// Window system loop
while(!glfwWindowShouldClose(window))
{
glfwPollEvents();
if(vkSample.isMinimized())
continue;

vkSample.display();  // infinitely drawing
}
```

## Display

A typical display() function will need the following:

* Acquiring the next image: `prepareFrame()`
* Get the command buffer for the frame. There are n command buffers equal to the number of in-flight frames.
* Clearing values
* Start rendering pass
* Drawing
* End rendering
* Submitting frame to display

```cpp
void VkSample::display()
{
// Acquire
prepareFrame();

// Command buffer for current frame
auto                   curFrame = getCurFrame();
const VkCommandBuffer& cmdBuf   = getCommandBuffers()[curFrame];

VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
vkBeginCommandBuffer(cmdBuf, &beginInfo);

// Clearing values
std::array<VkClearValue, 2> clearValues{};
clearValues[0].color        = {{1.f, 1.f, 1.f, 1.f}};
clearValues[1].depthStencil = {1.0f, 0};

// Begin rendering
VkRenderPassBeginInfo renderPassBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
renderPassBeginInfo.clearValueCount = 2;
renderPassBeginInfo.pClearValues    = clearValues.data();
renderPassBeginInfo.renderPass      = m_renderPass;
renderPassBeginInfo.framebuffer     = m_framebuffers[curFram];
renderPassBeginInfo.renderArea      = {{0, 0}, m_size};
vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);


// .. draw scene ...

// Draw UI
ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(),cmdBuff)

// End rendering
vkCmdEndRenderPass(cmdBuf);

// End of the frame and present the one which is ready
vkEndCommandBuffer(cmdBuf);
submitFrame();
}
```

## Closing

Finally, all resources can be destroyed by calling `destroy()` at the end of main().

```cpp
vkSample.destroy();
```


## appbase_vkpp.hpp
### class nvvkhl::AppBase


nvvkhl::AppBaseVk is the same as nvvkhl::AppBaseVk but makes use of the Vulkan C++ API (`vulkan.hpp`).


## application.hpp
### class nvvk::Application


The Application is basically a small modification of the ImGui example for Vulkan.
Because we support multiple viewports, duplicating the code would be not necessary
and the code is very well explained.

To use the application,
* Fill the ApplicationCreateInfo with all the information, including the Vulkan creation information (nvvk::ContextCreateInfo).
* Attach elements to the application, such as rendering, camera, etc.
* Call run() to start the application.
*
* The application will create the window, the Vulkan context, and the ImGui context.

Worth notice
* ::init() : will create the GLFW window, call nvvk::context for the creation of the
Vulkan context, initialize ImGui , create the surface and window (::setupVulkanWindow)
* ::shutdown() : the oposite of init
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



## element_benchmark_parameters.hpp
### class nvvkhl::ElementBenchmarkParameters


This element allows you to control an application with command line parameters. There are default
parameters, but others can be added using the parameterLists().add(..) function.

It can also use a file containing several sets of parameters, separated by "benchmark" and
which can be used to benchmark an application.

If a profiler is set, the measured performance at the end of each benchmark group is logged.


There are default parameters that can be used:
-logfile            Set a logfile.txt. If string contains $DEVICE$ it will be replaced by the GPU device name
-winsize            Set window size (width and height)
-winpos             Set window position (x and y)
-vsync              Enable or disable vsync
-screenshot         Save a screenshot into this file
-benchmarkframes    Set number of benchmarkframes
-benchmark          Set benchmark filename
-test               Enabling Testing
-test-frames        If test is on, number of frames to run
-test-time          If test is on, time that test will run

Example of Setup:

```cpp
std::shared_ptr<nvvkhl::ElementBenchmarkParameters> g_benchmark;
std::shared_ptr<nvvkhl::ElementProfiler>  g_profiler;

main() {
...
g_benchmark   = std::make_shared<nvvkhl::ElementBenchmarkParameters>(argc, argv);
g_profiler = std::make_shared<nvvkhl::ElementProfiler>(false);
g_benchmark->setProfiler(g_profiler);
app->addElement(g_profiler);
app->addElement(g_benchmark);
...
}
```


Applications can also get their parameters modified:

```cpp
void MySample::MySample() {
g_benchmark->parameterLists().add("speed|The speed", &m_speed);
g_benchmark->parameterLists().add("color", &m_color, nullptr, 3);
g_benchmark->parameterLists().add("complex", &m_complex, [&](int p){ doSomething(); });
```cpp


Example of a benchmark.txt could look like

\code{.bat}
#how many frames to average
-benchmarkframes 12
-winpos 10 10
-winsize 500 500

benchmark "No vsync"
-vsync 0
-benchmarkframes 100
-winpos 500 500
-winsize 100 100

benchmark "Image only"
-screenshot "temporal_mdi.jpg"
```


## element_camera.hpp
### class nvvkhl::ElementCamera


This class is an element of the application that is responsible for the camera manipulation. It is using the `nvh::CameraManipulator` to handle the camera movement and interaction.

To use this class, you need to add it to the `nvvkhl::Application` using the `addElement` method.


## element_dbgprintf.hpp
### class nvvkhl::ElementDbgPrintf


>  This class is an element of the application that is responsible for the debug printf in the shader. It is using the `VK_EXT_debug_printf` extension to print information from the shader.

To use this class, you need to add it to the `nvvkhl::Application` using the `addElement` method.

Create the element such that it will be available to the target application
- Example:
```cpp
std::shared_ptr<nvvkhl::ElementDbgPrintf> g_dbgPrintf = std::make_shared<nvvkhl::ElementDbgPrintf>();
```

Add to main
- Before creating the nvvkhl::Application, set:
` spec.vkSetup.instanceCreateInfoExt = g_dbgPrintf->getFeatures(); `
- Add the Element to the Application
` app->addElement(g_dbgPrintf); `
- In the target application, push the mouse coordinated
` m_pushConst.mouseCoord = g_dbgPrintf->getMouseCoord(); `

In the Shader, do:
- Add the extension
` #extension GL_EXT_debug_printf : enable `
- Where to get the information
```cpp
ivec2 fragCoord = ivec2(floor(gl_FragCoord.xy));
if(fragCoord == ivec2(pushC.mouseCoord))
debugPrintfEXT("Value: %f\n", myVal);
```

## element_gui.hpp
### class nvvkhl::ElementDefaultMenu


>  This class is an element of the application that is responsible for the default menu of the application. It is using the `ImGui` library to create a menu with File/Exit and View/V-Sync.

To use this class, you need to add it to the `nvvkhl::Application` using the `addElement` method.

### class nvvkhl::ElementDefaultWindowTitle


>  This class is an element of the application that is responsible for the default window title of the application. It is using the `GLFW` library to set the window title with the application name, the size of the window and the frame rate.

To use this class, you need to add it to the `nvvkhl::Application` using the `addElement` method.


## element_inspector.hpp

### class nvvkhl::ElementInspector


--------------------------------------------------------------------------------------------------
This element is used to facilitate GPU debugging by inspection of:
- Image contents
- Buffer contents
- Variables in compute shaders
- Variables in fragment shaders

IMPORTANT NOTE: if used in a multi threaded environment synchronization needs to be performed
externally by the application.

Basic usage:
------------------------------------------------------------------------------------------------
###                 INITIALIZATION
------------------------------------------------------------------------------------------------

Create the element as a global variable, and add it to the applications
```cpp
std::shared_ptr<ElementInspector> g_inspectorElement = std::make_shared<ElementInspector>();

void main(...)
{
...
app->addElement(g_inspectorElement);
...
}
```
Upon attachment of the main app element, initialize the Inspector and specify the number of
buffers, images, compute shader variables and fragment shader variables that it will need to
inspect
```cpp
void onAttach(nvvkhl::Application* app) override
{
...
ElementInspector::InitInfo initInfo{};
initInfo.allocator     = m_alloc.get();
initInfo.imageCount    = imageInspectionCount;
initInfo.bufferCount   = bufferInspectionCount;
initInfo.computeCount  = computeInspectionCount;
initInfo.fragmentCount = fragmentInspectionCount;
initInfo.customCount   = customInspectionCount;
initInfo.device = m_app->getDevice();
initInfo.graphicsQueueFamilyIndex = m_app->getQueueGCT().familyIndex;

g_inspectorElement->init(initInfo);
...
}
```

------------------------------------------------------------------------------------------------
###                 BUFFER INSPECTION
------------------------------------------------------------------------------------------------

Each inspection needs to be initialized before use:
Inspect a buffer of size bufferSize, where each entry contains 5 values. The buffer format specifies the data
structure of the entries. The following format is the equivalent of
```cpp
// struct
// {
//   uint32_t counterU32;
//   float    counterF32;
//   int16_t  anI16Value;
//   uint16_t myU16;
//   int32_t  anI32;
// };
```

```cpp
bufferFormat    = std::vector<ElementInspector::ValueFormat>(5);
bufferFormat[0] = {ElementInspector::eUint32, "counterU32"};
bufferFormat[1] = {ElementInspector::eFloat32, "counterF32"};
bufferFormat[2] = {ElementInspector::eInt16, "anI16Value"};
bufferFormat[3] = {ElementInspector::eUint16, "myU16"};
bufferFormat[4] = {ElementInspector::eInt32, "anI32"};
ElementInspector::BufferInspectionInfo info{};
info.entryCount   = bufferSize;
info.format       = bufferFormat;
info.name         = "myBuffer";
info.sourceBuffer = m_buffer.buffer;
g_inspectorElement->initBufferInspection(0, info);
```

When the inspection is desired, simply add it to the current command buffer. Required barriers are added internally.
IMPORTANT: the buffer MUST have been created with the VK_BUFFER_USAGE_TRANSFER_SRC_BIT flag
```cpp
g_inspectorElement->inspectBuffer(cmd, 0);
```

------------------------------------------------------------------------------------------------
###                 IMAGE INSPECTION
------------------------------------------------------------------------------------------------

Inspection of the image stored in m_texture, with format RGBA32F. Other formats can be specified using the syntax
above
```cpp
ElementInspector::ImageInspectionInfo info{};
info.createInfo  = create_info;
info.format      = g_inspectorElement->formatRGBA32();
info.name        = "MyImageInspection";
info.sourceImage = m_texture.image;
g_inspectorElement->initImageInspection(0, info);
```

When the inspection is desired, simply add it to the current command buffer. Required barriers are added internally.
```cpp
g_inspectorElement->inspectImage(cmd, 0, imageCurrentLayout);
```

------------------------------------------------------------------------------------------------
###                 COMPUTE SHADER VARIABLE INSPECTION
------------------------------------------------------------------------------------------------

Inspect a compute shader variable for a given 3D grid and block size (use 1 for unused dimensions). This mode applies
to shaders where invocation IDs (e.g. gl_LocalInvocationID) are defined, such as compute, mesh and task shaders.
Since grids may contain many threads capturing a variable for all threads
may incur large memory consumption and performance loss. The blocks to inspect, and the warps within those blocks can
be specified using inspectedMin/MaxBlocks and inspectedMin/MaxWarp.
```cpp
computeInspectionFormat    = std::vector<ElementInspector::ValueFormat>(...);
ElementInspector::ComputeInspectionInfo info{}; info.blockSize = blockSize;

// Create a 4-component vector format where each component is a uint32_t. The components will be labeled myVec4u.x,
// myVec4u.y, myVec4u.z, myVec4u.w in the UI
info.format           = ElementInspector::formatVector4(eUint32, "myVec4u");
info.gridSizeInBlocks = gridSize;
info.minBlock         = inspectedMinBlock;
info.maxBlock         = inspectedMaxBlock;
info.minWarp          = inspectedMinWarp;
info.maxWarp          = inspectedMaxWarp;
info.name             = "My Compute Inspection";
g_inspectorElement->initComputeInspection(0, info);
```

To allow variable inspection two buffers need to be made available to the target shader:
m_computeShader.updateBufferBinding(eThreadInspection, g_inspectorElement->getComputeInspectionBuffer(0));
m_computeShader.updateBufferBinding(eThreadMetadata, g_inspectorElement->getComputeMetadataBuffer(0));

The shader code needs to indicate include the Inspector header along with preprocessor variables to set the
inspection mode to Compute, and indicate the binding points for the buffers:
```cpp
#define INSPECTOR_MODE_COMPUTE
#define INSPECTOR_DESCRIPTOR_SET 0
#define INSPECTOR_INSPECTION_DATA_BINDING 1
#define INSPECTOR_METADATA_BINDING 2
#include "dh_inspector.h"
```

The inspection of a variable is then done as follows. For alignment purposes the inspection is done with a 32-bit
granularity. The shader is responsible for packing the inspected variables in 32-bit uint words. Those will be
unpacked within the Inspector for display according to the specified format.
```cpp
uint32_t myVariable = myFunction(...);
inspect32BitValue(0, myVariable);
```

The inspection is triggered on the host side right after the compute shader invocation:
```cpp
m_computeShader.dispatchBlocks(cmd, computGridSize, &constants);

g_inspectorElement->inspectComputeVariables(cmd, 0);
```

------------------------------------------------------------------------------------------------
###                 FRAGMENT SHADER VARIABLE INSPECTION
------------------------------------------------------------------------------------------------

Inspect a fragment shader variable for a given output image resolution. Since the image may have high resolution
capturing a variable for all threads may incur large memory consumption and performance loss. The bounding box of the
fragments to inspect can be specified using inspectedMin/MaxCoord.
IMPORTANT: Overlapping geometry may trigger
several fragment shader invocations for a given pixel. The inspection will only store the value of the foremost
fragment (with the lowest gl_FragCoord.z).
```cpp
fragmentInspectionFormat    = std::vector<ElementInspector::ValueFormat>(...);
FragmentInspectionInfo info{};
info.name        = "My Fragment Inspection";
info.format      = fragmentInspectionFormat;
info.renderSize  = imageSize;
info.minFragment = inspectedMinCoord;
info.maxFragment = inspectedMaxCoord;
g_inspectorElement->initFragmentInspection(0, info);
```

To allow variable inspection two storage buffers need to be declared in the pipeline layout and made available
as follows:
```cpp
std::vector<VkWriteDescriptorSet> writes;

const VkDescriptorBufferInfo inspectorInspection{
g_inspectorElement->getFragmentInspectionBuffer(0),
0,
VK_WHOLE_SIZE};
writes.emplace_back(m_dset->makeWrite(0, 1, &inspectorInspection));
const VkDescriptorBufferInfo inspectorMetadata{
g_inspectorElement->getFragmentMetadataBuffer(0),
0,
VK_WHOLE_SIZE};
writes.emplace_back(m_dset->makeWrite(0, 2, &inspectorMetadata));

vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
```

The shader code needs to indicate include the Inspector header along with preprocessor variables to set the
inspection mode to Fragment, and indicate the binding points for the buffers:
```cpp
#define INSPECTOR_MODE_FRAGMENT
#define INSPECTOR_DESCRIPTOR_SET 0
#define INSPECTOR_INSPECTION_DATA_BINDING 1
#define INSPECTOR_METADATA_BINDING 2
#include "dh_inspector.h"
```

The inspection of a variable is then done as follows. For alignment purposes the inspection is done with a 32-bit
granularity. The shader is responsible for packing the inspected variables in 32-bit uint words. Those will be
unpacked within the Inspector for display according to the specified format.
```cpp
uint32_t myVariable = myFunction(...);
inspect32BitValue(0, myVariable);
```

The inspection data for a pixel will only be written if a fragment actually covers that pixel. To avoid ghosting
where no fragments are rendered it is useful to clear the inspection data before rendering:
```cpp
g_inspectorElement->clearFragmentVariables(cmd, 0);
vkCmdBeginRendering(...);
```

The inspection is triggered on the host side right after rendering:
```cpp
vkCmdEndRendering(cmd);
g_inspectorElement->inspectFragmentVariables(cmd, 0);
```

------------------------------------------------------------------------------------------------
###                 CUSTOM SHADER VARIABLE INSPECTION
------------------------------------------------------------------------------------------------

In case some in-shader data has to be inspected in other shader types, or not on a once-per-thread basis, the custom
inspection mode can be used. This mode allows the user to specify the overall size of the generated data as well as
an effective inspection window. This mode may be used in conjunction with the COMPUTE and FRAGMENT modes.
std::vector<ElementInspector::ValueFormat> customCaptureFormat;
```cpp
...
ElementInspector::CustomInspectionInfo info{};
info.extent   = totalInspectionSize;
info.format   = customCaptureFormat;
info.minCoord = inspectionWindowMin;
info.maxCoord = inspectionWindowMax;
info.name     = "My Custom Capture";
g_inspectorElement->initCustomInspection(0, info);
```

To allow variable inspection two buffers need to be made available to the target pipeline:
```cpp
std::vector<VkWriteDescriptorSet> writes;
const VkDescriptorBufferInfo      inspectorInspection{
g_inspectorElement->getCustomInspectionBuffer(0),
0,
VK_WHOLE_SIZE};
writes.emplace_back(m_dset->makeWrite(0, 1, &inspectorInspection));
const VkDescriptorBufferInfo inspectorMetadata{
g_inspectorElement->getCustomMetadataBuffer(0),
0,
VK_WHOLE_SIZE};
writes.emplace_back(m_dset->makeWrite(0, 2, &inspectorMetadata));

vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
```

The shader code needs to indicate include the Inspector header along with preprocessor variables to set the
inspection mode to Fragment, and indicate the binding points for the buffers:
```cpp
#define INSPECTOR_MODE_CUSTOM
#define INSPECTOR_DESCRIPTOR_SET 0
#define INSPECTOR_CUSTOM_INSPECTION_DATA_BINDING 1
#define INSPECTOR_CUSTOM_METADATA_BINDING 2
#include "dh_inspector.h"
```
The inspection of a variable is then done as follows. For alignment purposes the inspection is done with a 32-bit
granularity. The shader is responsible for packing the inspected variables in 32-bit uint words. Those will be
unpacked within the Inspector for display according to the specified format.
```cpp
uint32_t myVariable = myFunction(...);
inspectCustom32BitValue(0, myCoordinates, myVariable);
```

The inspection is triggered on the host side right after running the pipeline:
```cpp
g_inspectorElement->inspectCustomVariables(cmd, 0);
```

## element_logger.hpp
### class nvvkhl::ElementLogger


>  This class is an element of the application that can redirect all logs to a ImGui window in the application

To use this class, you need to add it to the `nvvkhl::Application` using the `addElement` method.

Create the element such that it will be available to the target application

Example:
```cpp
static nvvkhl::SampleAppLog g_logger;
nvprintSetCallback([](int level, const char* fmt)
{
g_logger.addLog("%s", fmt);
});

app->addElement(std::make_unique<nvvkhl::ElementLogger>(&g_logger, true));  // Add logger window
```


## element_nvml.hpp
### class nvvkhl::ElementNvml


>  This class is an element of the application that is responsible for the NVML monitoring. It is using the `NVML` library to get information about the GPU and display it in the application.

To use this class, you need to add it to the `nvvkhl::Application` using the `addElement` method.


## element_profiler.hpp
### class nvvkhl::ElementProfiler

>  This class is an element of the application that is responsible for the profiling of the application. It is using the `nvvk::ProfilerVK` to profile the time parts of the computation done on the GPU.

To use this class, you need to add it to the `nvvkhl::Application` using the `addElement` method.

The profiler element, is there to help profiling the time parts of the
computation is done on the GPU. To use it, follow those simple steps

In the main() program, create an instance of the profiler and add it to the
nvvkhl::Application

```cpp
std::shared_ptr<nvvkhl::ElementProfiler> profiler = std::make_shared<nvvkhl::ElementProfiler>();
app->addElement(profiler);
```

In the application where profiling needs to be done, add profiling sections

```cpp
void mySample::onRender(VkCommandBuffer cmd)
{
auto sec = m_profiler->timeRecurring(__FUNCTION__, cmd);
...
// Subsection
{
auto sec = m_profiler->timeRecurring("Dispatch", cmd);
vkCmdDispatch(cmd, (size.width + (GROUP_SIZE - 1)) / GROUP_SIZE, (size.height + (GROUP_SIZE - 1)) / GROUP_SIZE, 1);
}
...
```

This is it and the execution time on the GPU for each part will be showing in the Profiler window.


## element_testing.hpp

> Todo: Add documentation

## gbuffer.hpp
### class nvvkhl::GBuffer

>  This class is an help for creating GBuffers.

This can be use to create a GBuffer with multiple color images and a depth image. The GBuffer can be used to render the scene in multiple passes, such as deferred rendering.

To use this class, you need to create it and call the `create` method to create the GBuffer. The `create` method will create the images and the descriptor set for the GBuffer. The `destroy` method will destroy the images and the descriptor set.

Note: the `getDescriptorSet` method can be use to display the image in ImGui. Ex: `ImGui::Image((ImTextureID)gbuffer.getDescriptorSet(), ImVec2(128, 128));`

## glsl_compiler.hpp
### class nvvkhl::GlslCompiler


>  This class is a wrapper around the shaderc compiler to help compiling GLSL to Spir-V using shaderC


## gltf_scene.hpp
### class nvvkhl::Scene


>  This class is responsible for loading and managing a glTF scene.

It is using the `nvh::GltfScene` to load and manage the scene and `tinygltf` to parse the glTF file.


## gltf_scene_rtx.hpp
### class nvvkhl::SceneRtx


>  This class is responsible for the ray tracing acceleration structure.

It is using the `nvvkhl::Scene` and `nvvkhl::SceneVk` information to create the acceleration structure.


## gltf_scene_vk.hpp
### class nvvkhl::SceneVk


>  This class is responsible for the Vulkan version of the scene.

It is using `nvvkhl::Scene` to create the Vulkan buffers and images.


## hdr_env.hpp
### class nvvkhl::HdrEnv


>  Load an environment image (HDR) and create an acceleration structure for important light sampling.


## hdr_env_dome.hpp
### class nvvkhl::HdrEnvDome


>  Use an environment image (HDR) and create the cubic textures for glossy reflection and diffuse illumination. It also has the ability to render the HDR environment, in the background of an image.

Using 4 compute shaders
- hdr_dome: to make the HDR as background
- hdr_integrate_brdf     : generate the BRDF lookup table
- hdr_prefilter_diffuse  : integrate the diffuse contribution in a cubemap
- hdr_prefilter_glossy   : integrate the glossy reflection in a cubemap


## pipeline_container.hpp
### struct nvvkhl::PipelineContainer


>  Small multi-pipeline container

## scene_camera.hpp
### Function nvvkhl::setCameraFromScene

>  Set the camera from the scene, if no camera is found, it will fit the camera to the scene.

## sky.hpp
### class nvvkhl::SkyDome


>  This class is responsible for the sky dome.

This class can render a sky dome with a sun, for both the rasterizer and the ray tracer.

The `draw` method is responsible for rendering the sky dome for the rasterizer. For ray tracing, there is no need to call this method, as the sky dome is part of the ray tracing shaders (see shaders/dh_sky.h).


## tonemap_postprocess.hpp

### class nvvkhl::TonemapperPostProcess


>  This class is meant to be use for displaying the final image rendered in linear space (sRGB).


There are two ways to use it, one which is graphic, the other is compute.

- The graphic will render a full screen quad with the input image. It is to the
application to set the rendering target ( -> G-Buffer0 )

- The compute takes an image as input and write to an another one using a compute shader

- It is either one or the other, both rendering aren't needed to post-process. If both are provided
it is for convenience.

Note: It is important in any cases to place a barrier if there is a transition from
fragment to compute and compute to fragment to avoid missing results.

