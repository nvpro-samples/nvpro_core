# Vulkan C++ Api Helpers

non-exhaustive list of utilities provided in the `nvvkpp` directory

## allocator_dedicated_vkpp.hpp

This is the allocator specialization using only Vulkan where there will be one memory
allocation for each buffer or image.
See [allocatorVMA](#Allocators-in-nvvkpp) for details on how to use the allocators.

> Note: this should be used only when really needed, as it is making one allocation per buffer,
>       which is not efficient. 

### Initialization

~~~~~~ C++
nvvkpp::AllocatorVk m_alloc;
m_alloc.init(device, physicalDevice);
~~~~~~ 

### **AllocatorVkExport**
This version of the allocator will export all memory allocations, which can then be use by Cuda or OpenGL.

## allocator_dma_vkpp.hpp

This is the allocator specialization using: **nvvk::DeviceMemoryAllocator**.
See [allocatorVMA](#Allocators-in-nvvkpp) for details on how to use the allocators.

### Initialization

~~~~~~ C++
nvvk::DeviceMemoryAllocator m_dmaAllocator;
nvvkpp::AllocatorDma        m_alloc
m_dmaAllocator.init(device, physicalDevice);
m_alloc.init(device, &m_dmaAllocator);
~~~~~~

## allocator_vma_vkpp.hpp

### Allocators in nvvkpp
Memory allocation shouldn't be a one-to-one with buffers and images, but larger memory block should 
be allocated and the buffers and images are mapped to a section of it. For better management of 
memory, it is suggested to use [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator).  
But in some cases, like for Vulkan Interop, the best is to use `AllocatorVkExport` which is export 
all memory allocations and make them available for Cuda or OpenGL.

#### Initialization
For VMA, you need first to create the `VmaAllocator`. In the following example, it creates the allocator and also will use dedicated memory in some cases.
~~~~C++
VmaAllocator m_vmaAllocator
VmaAllocatorCreateInfo allocatorInfo = {};
allocatorInfo.physicalDevice         = physicalDevice;
allocatorInfo.device                 = device;
allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
                     VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator);
~~~~

> Note: For dedicated memory, it is required to enable device extensions: <br>`VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME` and `VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME`

Then initialize the allocator itself:
~~~~c++
nvvkpp::AllocatorVma m_alloc;
m_alloc.init(device, m_vmaAllocator);
~~~~



#### **Buffers**
Either you create a simple buffer using `createBuffer(bufferCreateInfo)` which is mostly for allocating buffer on the device or you upload data using `createBuffer(cmdBuff, size, data)`. 
The second one will be staging the transfer of the data to the device and there is a version where you can pass a std::vector instead of size and data.

Example
~~~~C++
// Creating a device buffer
m_sceneBuffer = m_alloc.createBuffer({}, sizeof(SceneUBO), vk::BufferUsageFlagBits::eUniformBuffer |
                                                           vk::BufferUsageFlagBits::eTransferDst});

// Creating a device buffer and uploading the vertices
m_vertexBuffer = m_alloc.createBuffer(cmdBuf, m_vertices.position,
                                      vk::BufferUsageFlagBits::eVertexBuffer |
                                      vk::BufferUsageFlagBits::eStorageBuffer);
~~~~

The Vk, DMA or VMA createBuffer are slightly different. They all have similar input and all return **BufferVk**, but the associated memory in the structure varies.

Example of the buffer structure for VMA
~~~~C++
struct BufferVma
{
  vk::Buffer    buffer;
  VmaAllocation allocation{};
};
~~~~



#### Images
For images, it is identical to buffers. Either you create only an image, or you create and initialize it with data.
See the usage [example](#full-example) below.

Image structure for VMA:
~~~~C++
struct ImageVma
{
  vk::Image     image;
  VmaAllocation allocation{};
};
~~~~

#### Textures
For convenience, there is also a texture structure that differs from the image by the addition of 
the descriptor which has the sampler and image view required for be used in shaders.

~~~~C++
struct TextureVma : public ImageVma
{
  vk::DescriptorImageInfo descriptor;
}
~~~~

To help creating textures and images this there is a few functions in `images_vkpp`:
* `create2DInfo`: return `vk::ImageCreateInfo`, which is used for the creation of the image
* `create2DDescriptor`: returns the `vk::ImageDescritorInfo`
* `generateMipmaps`: to generate all mipmaps level of an image
* `setImageLayout`: transition the image layout (`vk::ImageLayout`)


#### Full example
~~~~C++
vk::Format          format = vk::Format::eR8G8B8A8Unorm;
vk::Extent2D        imgSize{width, height};
vk::ImageCreateInfo imageCreateInfo = nvvkpp::image::create2DInfo(imgSize, format, vk::ImageUsageFlagBits::eSampled, true);
vk::DeviceSize      bufferSize = width * height * 4 * sizeof(uint8_t);

m_texture = m_alloc.createImage(cmdBuf, bufferSize, buffer, imageCreateInfo);

vk::SamplerCreateInfo samplerCreateInfo{{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear};
nvvkpp::image::generateMipmaps(cmdBuf, m_texture.image, format, imgSize, imageCreateInfo.mipLevels);
m_texture.descriptor = nvvkpp::image::create2DDescriptor(m_device, m_texture.image, samplerCreateInfo);
~~~~




#### Acceleration structure
For this one, there are no staging variant, it will return the acceleration structure and memory binded.

#### Destroy
To destroy buffers, image, or acceleration structures call the `destroy()` method with the object in argument. It will destroy the Vulkkan object and free the memory.

~~~~C++
m_alloc.destroy(m_vertexBuffer);
m_alloc.destroy(m_texture);
~~~~

---------------------------------------------

### Staging
In case data was uploaded using one of the staging method, it will be important to _flush_ the temporary allocations. You can call `flushStaging` directly after submitting the command buffer or pass a `vk::Fence` corresponding to the command buffer submission. See next section on **MultipleCommandBuffers**.

Flushing is required to recover memory, but cannot be done until the copy is completed. This is why there is an argument to pass a fence. Either you are making sure the **Queue** on which the command buffer that have placed the copy commands is idle, or the internal system will flush the staging buffers when the fence is released.

**Method 1 - Good**
~~~~C++
// Submit current command buffer
auto fence = m_cmdBufs.submit();
// Adds all staging buffers to garbage collection, delete what it can
m_alloc.flushStaging(fence);
~~~~

**Method 2 - Not so Good**
~~~~C++
m_cmdBufs.submit();        // Submit current command buffer
m_cmdBufs.waitForUpload(); // Wait that all commands are done
m_alloc.flushStaging();    // Destroy staging buffers
~~~~

## appbase_vkpp.hpp

This is the base class for many examples. It does the basic for calling the initialization of
Vulkan, the creation of the logical device, but also is a placeholder for the render passes and
the swapchain

## axis_vkpp.hpp

Display an Axis representing the orientation of the camera in the bottom left corner of the window.
- Initialize the Axis using `init()`
- Add `display()` in a inline rendering pass, one of the lass command

Example:  
~~~~~~ C++
m_axis.display(cmdBuf, CameraManip.getMatrix(), windowSize);
~~~~~~

## commands_vkpp.hpp

### class **nvvkpp::SingleCommandBuffer**

With `SingleCommandBuffer`, you create the the command buffer by calling `createCommandBuffer()` and submit all the work by calling `flushCommandBuffer()`

~~~~ c++
nvvkpp::SingleCommandBuffer sc(m_device, m_graphicsQueueIndex);
vk::CommandBuffer cmdBuf = sc.createCommandBuffer();
...
sc.flushCommandBuffer(cmdBuf);
~~~~


### class **nvvkpp::ScopeCommandBuffer**
The `ScopeCommandBuffer` is similar, but the creation and flush is automatic. The submit of the operations will be done when it goes out of scope.

~~~~ c++
{
  nvvkpp::ScopeCommandBuffer cmdBuf(m_device, m_graphicQueueIndex);
  functionWithCommandBufferInParameter(cmdBuf);
} // internal functions are executed
~~~~

> Note: The above methods are not good for performance critical areas as it is stalling the execution.


### class **nvvkpp::MultipleCommandBuffers**
This is the suggested way to use command buffers while building up the scene. The reason is it will 
not be blocking and will transfer the staging buffers in a different thread. There are by default 10
command buffers which could be in theory executed in parallel.

**Setup:** create one instance of it, as a member of the class and pass it around. You will need the
`vk::Device` and the family queue index.

**Get:** call `getCmdBuffer()` for the next available command buffer.

**Submit:** `submit()` will submit the current active command buffer. It will return a fence which 
can be used for flushing the staging buffers. See previous chapter.

**Flushing the **Queue****: in case there are still pending commands, you can call `waitForUpload()` 
and this will make sure that the queue is idle.

~~~~C++
nvvkpp::MultipleCommandBuffers m_cmdBufs;
m_cmdBufs.setup(device, graphicsQueueIndex);
...
auto cmdBuf = m_cmdBufs.getCmdBuffer();
// Create buffers
// Create images
auto fence = m_cmdBufs.submit();
m_alloc.flushStaging(fence);
~~~~


### functions in nvvk

- **makeAccessMaskPipelineStageFlags** : depending on accessMask returns appropriate `vk::PipelineStageFlagBits`
- **cmdBegin** : wraps `vkBeginCommandBuffer` with `vk::CommandBufferUsageFlags` and implicitly handles `vk::CommandBufferBeginInfo` setup
- **makeSubmitInfo** : `vk::SubmitInfo` struct setup using provided arrays of signals and commandbuffers, leaving rest zeroed


### class **nvvkpp::CmdPool**

**CmdPool** stores a single `vk::CommandPool` and provides utility functions
to create `vk::CommandBuffers` from it.


### class **nvvkpp::ScopeSubmitCmdPool**

**ScopeSubmitCmdPool** extends **CmdPool** and lives within a scope.
It directly submits its commandbufers to the provided queue.
Intent is for non-critical actions where performance is NOT required,
as it waits until the device has completed the operation.

Example:
``` C++
{
  nvvkpp::ScopeSubmitCmdPool scopePool(...);

  // some batch of work
  {
    vkCommandBuffer cmd = scopePool.begin();
    ... record commands ...
    // blocking operation
    scopePool.end(cmd);
  }

  // other operations done here
  {
    vkCommandBuffer cmd = scopePool.begin();
    ... record commands ...
    // blocking operation
    scopePool.end(cmd);
  }
}
```


### class **nvvkpp::ScopeSubmitCmdBuffer**

Provides a single `vk::CommandBuffer` that lives within the scope
and is directly submitted, deleted when the scope is left.

Example:
``` C++
{
  ScopeSubmitCmdBuffer cmd(device, queue, queueFamilyIndex);
  ... do stuff
  vkCmdCopyBuffer(cmd, ...);
}
```


### classes **nvvkpp::Ring...**

In real-time processing, the CPU typically generates commands
in advance to the GPU and send them in batches for exeuction.

To avoid having he CPU to wait for the GPU'S completion and let it "race ahead"
we make use of double, or tripple-buffering techniques, where we cycle through
a pool of resources every frame. We know that those resources are currently
not in use by the GPU and can therefore manipulate them directly.

Especially in Vulkan it is the developer's responsibility to avoid such
access of resources that are in-flight.

The "Ring" classes cycle through a pool of 3 resources, as that is typically
the maximum latency drivers may let the CPU get in advance of the GPU.


#### class **nvvkpp::RingFences**

Recycles a fixed number of fences, provides information in which cycle
we are currently at, and prevents accidental access to a cycle inflight.

A typical frame would start by waiting for older cycle's completion ("wait")
and be ended by "advanceCycle".

Safely index other resources, for example ring buffers using the "getCycleIndex"
for the current frame.


#### class **nvvkpp::RingCmdPool**

Manages a fixed cycle set of `vk::CommandBufferPools` and
one-shot command buffers allocated from them.

Every cycle a different command buffer pool is used for
providing the command buffers. Command buffers are automatically
deleted after a full cycle (MAX_RING_FRAMES) has been completed.

The usage of multiple command buffer pools also means we get nice allocation
behavior (linear allocation from frame start to frame end) without fragmentation.
If we were using a single command pool, it would fragment easily.

Example:

~~~ C++
{
  // wait until we can use the new cycle (normally we never have to wait)
  ringFences.wait();

  ringPool.setCycle( ringFences.getCycleIndex() )

  vk::CommandBuffer cmd = ringPool.createCommandBuffer(...)
  ... do stuff / submit etc...

  vk::Fence fence = ringFences.advanceCycle();
  // use this fence in the submit
  vkQueueSubmit(...)
}
~~~


### class **nvvkpp::BatchSubmission**

Batches the submission arguments of `vk::SubmitInfo` for vk::QueueSubmit.

`vkQueueSubmit` is a rather costly operation (depending on OS)
and should be avoided to be done too often (< 10). Therefore this utility
class allows adding commandbuffers, semaphores etc. and submit in a batch.

When using manual locks, it can also be useful to feed commandbuffers
from different threads and then later kick it off.

Example

~~~ C++
// within upload logic
{
  semTransfer = handleUpload(...);
  // for example trigger async upload on transfer queue here
  vkQueueSubmit(..);

  // tell next frame's batch submission
  // that its commandbuffers should wait for transfer
  // to be completed
  graphicsSubmission.enqueWait(semTransfer)
}

// within present logic
{
  // for example ensure the next frame waits until proper present semaphore was triggered
  graphicsSubmission.enqueueWait(presentSemaphore);
}

// within drawing logic
{
  // enqeue some graphics work for submission
  graphicsSubmission.enqueue(getSceneCmdBuffer());
  graphicsSubmission.enqueue(getUiCmdBuffer());

  graphicsSubmission.execute(frameFence);
}
~~~

## context_vkpp.hpp

To run a Vulkan application, you need to create the Vulkan instance and device.
This is done using the `nvvk::Context`, which wraps the creation of `VkInstance`
and `VkDevice`.

First, any application needs to specify how instance and device should be created:
Version, layers, instance and device extensions influence the features available.
This is done through a temporary and intermediate class that will allow you to gather
all the required conditions for the device creation.


### struct **ContextCreateInfo**

This structure allows the application to specify a set of features
that are expected for the creation of
- `VkInstance`
- `VkDevice`

It is consumed by the `nvvk::Context::init` function.

Example on how to populate information in it : 

~~~~ C++
nvvkpp::ContextCreateInfo ctxInfo;
ctxInfo.setVersion(1, 1);
ctxInfo.addInstanceLayer("VK_LAYER_KHRONOS_validation");
ctxInfo.addInstanceLayer("VK_LAYER_LUNARG_monitor", true);
ctxInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME, false);
ctxInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, false);
ctxInfo.addInstanceExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
ctxInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME, false);
// Enabling the extension feature
vk::PhysicalDeviceRayTracingFeaturesKHR raytracingFeature;
ctxInfo.addDeviceExtension(VK_KHR_RAY_TRACING_EXTENSION_NAME, false, &raytracingFeature);

~~~~

then you are ready to create initialize `nvvk::Context`

> Note: In debug builds, the extension `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` and the layer `VK_LAYER_KHRONOS_validation` are added to help finding issues early.


### class **nvvk::Context**

**Context** class helps creating the Vulkan instance and to choose the logical device for the mandatory extensions. First is to fill the `ContextCreateInfo` structure, then call:

~~~ C++
// Creating the Vulkan instance and device
nvvkpp::ContextCreateInfo ctxInfo;
... see above ...

nvvkpp::Context vkctx;
vkctx.init(ctxInfo);

// after init the ctxInfo is no longer needed
~~~ 

At this point, the class will have created the `VkInstance` and `VkDevice` according to the information passed. It will also keeps track or have query the information of:
 
* Physical Device information that you can later query : `PhysicalDeviceInfo` in which lots of `VkPhysicalDevice...` are stored
* `vk::Instance` : the one instance being used for the program
* `vk::PhysicalDevice` : physical device(s) used for the logical device creation. In case of more than one physical device, we have a std::vector for this purpose...
* `vk::Device` : the logical device instantiated
* `vk::Queue` : we will enumerate all the available queues and make them available in `nvvk::Context`. Some queues are specialized, while other are for general purpose (most of the time, only one can handle everything, while other queues are more specialized). We decided to make them all available in some explicit way :
 * `Queue m_queueGCT` : Graphics/Compute/Transfer **Queue** + family index
 * `Queue m_queueT` : async Transfer **Queue** + family index
 * `Queue m_queueC` : Compute **Queue** + family index
* maintains what extensions are finally available
* implicitly hooks up the debug callback

#### Choosing the device
When there are multiple devices, the `init` method is choosing the first compatible device available, but it is also possible the choose another one.
~~~ C++
vkctx.initInstance(deviceInfo); 
// Find all compatible devices
auto compatibleDevices = vkctx.getCompatibleDevices(deviceInfo);
assert(!compatibleDevices.empty());

// Use first compatible device
vkctx.initDevice(compatibleDevices[0], deviceInfo);
~~~

#### Multi-GPU

When multiple graphic cards should be used as a single device, the `ContextCreateInfo::useDeviceGroups` need to be set to `true`.
The above methods will transparently create the `vk::Device` using `vk::DeviceGroupDeviceCreateInfo`.
Especially in the context of NVLink connected cards this is useful.

## debug_util_vkpp.hpp

This is a companion utility to add debug information to an application. 
See [Chapter 39](https://vulkan.lunarg.com/doc/sdk/1.1.114.0/windows/chunked_spec/chap39.html).

- User defined name to objects
- Logically annotate region of command buffers
- Scoped command buffer label to make thing simpler


Example
~~~~~~ C++
nvvkpp::DebugUtil    m_debug;
m_debug.setup(device);
...
m_debug.setObjectName(m_vertices.buffer, "sceneVertex");
m_debug.setObjectName(m_pipeline, "scenePipeline");
~~~~~~

## descriptorsets_vkpp.hpp

There is a utility to create the `DescriptorSet`, the `DescriptorPool` and `DescriptorSetLayout`. 
All the information to create those structures are included in the `DescriptorSetLayoutBinding`. 
Therefore, it is only necessary to fill the `DescriptorSetLayoutBinding` and let the utilities to 
fill the information.

For assigning the information to a descriptor set, multiple `WriteDescriptorSet` need to be filled. 
Using the `nvvk::util::createWrite` with the buffer, image or acceleration structure descriptor will 
return the correct `WriteDescriptorSet` to push back.

Example of usage:
~~~~ c++
m_descSetLayoutBind[eScene].emplace_back(vk::DescriptorSetLayoutBinding(0, vkDT::eUniformBuffer, 1, vkSS::eVertex | vkSS::eFragment));
m_descSetLayoutBind[eScene].emplace_back(vk::DescriptorSetLayoutBinding(1, vkDT::eStorageBufferDynamic, 1, vkSS::eVertex ));
m_descSetLayout[eScene] = nvvk::util::createDescriptorSetLayout(m_device, m_descSetLayoutBind[eScene]);
m_descPool[eScene]      = nvvk::util::createDescriptorPool(m_device, m_descSetLayoutBind[eScene], 1);
m_descSet[eScene]       = nvvk::util::createDescriptorSet(m_device, m_descPool[eScene], m_descSetLayout[eScene]);
...
std::vector<vk::WriteDescriptorSet> writes;
writes.emplace_back(nvvk::util::createWrite(m_descSet[eScene], m_descSetLayoutBind[eScene][0], &m_uniformBuffers.scene.descriptor));
writes.emplace_back(nvvk::util::createWrite(m_descSet[eScene], m_descSetLayoutBind[eScene][1], &m_uniformBuffers.matrices.descriptor));
m_device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
~~~~

## images_vkpp.hpp

Various Image Utilities
- Transition Pipeline Layout tools
- 2D Texture helper creation
- Cubic Texture helper creation


**mipLevels**: Returns the number of mipmaps an image can have


**setImageLayout**, **accessFlagsForLayout**, **pipelineStageForLayout**: Transition Pipeline Layout tools


**create2DInfo**: creates a `vk::ImageCreateInfo`

**create2DDescriptor**: creates `vk::create2DDescriptor`

**generateMipmaps**: generate all mipmaps for a vk::Image


**create2DInfo**: creates a `vk::ImageCreateInfo`

**create2DDescriptor**: creates `vk::create2DDescriptor`

**generateMipmaps**: generate all mipmaps for a vk::Image

## pipeline_vkpp.hpp

Most graphic pipeline are similar, therefor the helper `GraphicsPipelineGenerator` holds all the elements and initialize the structure with the proper default, such as `eTriangleList`, `PipelineColorBlendAttachmentState` with their mask, `DynamicState` for viewport and scissor, adjust depth test if enable, line width to 1, for example. As those are all C++, setters exist for any members and deep copy for any attachments.

Example of usage :
~~~~ c++
nvvk::GraphicsPipelineGenerator pipelineGenerator(m_device, m_pipelineLayout, m_renderPass);
pipelineGenerator.loadShader(readFile("shaders/vert_shader.vert.spv"), vk::ShaderStageFlagBits::eVertex);
pipelineGenerator.loadShader(readFile("shaders/frag_shader.frag.spv"), vk::ShaderStageFlagBits::eFragment);
pipelineGenerator.depthStencilState = {true};
pipelineGenerator.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
pipelineGenerator.vertexInputState.bindingDescriptions   = {{0, sizeof(Vertex)}};
pipelineGenerator.vertexInputState.attributeDescriptions = {
    {0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
    {1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nrm))},
    {2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, col))}};
m_pipeline = pipelineGenerator.create();
~~~~

## raytraceKHR_vkpp.hpp

Base functionality of raytracing

This class does not implement all what you need to do raytracing, but
helps creating the BLAS and TLAS, which then can be used by different
raytracing usage.

### Setup and Usage
~~~~ C++
m_rtBuilder.setup(device, memoryAllocator, queueIndex);
// Create array of vk::GeometryNV
m_rtBuilder.buildBlas(allBlas);
// Create array of RaytracingBuilder::instance
m_rtBuilder.buildTlas(instances);
// Retrieve the acceleration structure
const vk::AccelerationStructureNV& tlas = m.rtBuilder.getAccelerationStructure()
~~~~

## raytrace_vkpp.hpp

Base functionality of raytracing

This class does not implement all what you need to do raytracing, but
helps creating the BLAS and TLAS, which then can be used by different
raytracing usage.

### Setup and Usage
~~~~ C++
m_rtBuilder.setup(device, memoryAllocator, queueIndex);
// Create array of vk::GeometryNV 
m_rtBuilder.buildBlas(allBlas);
// Create array of RaytracingBuilder::instance
m_rtBuilder.buildTlas(instances);
// Retrieve the acceleration structure
const vk::AccelerationStructureNV& tlas = m.rtBuilder.getAccelerationStructure() 
~~~~

## renderpass_vkpp.hpp

Simple creation of render passes
Example of usage:
~~~~~~ C++
m_renderPass   = nvvkpp::util::createRenderPass(m_device, {m_swapChain.colorFormat}, m_depthFormat, 1, true, true);
m_renderPassUI = nvvkpp::util::createRenderPass(m_device, {m_swapChain.colorFormat}, m_depthFormat, 1, false, false);
~~~~~~

## swapchain_vkpp.hpp

### **nvvkpp::SwapChain**

The swapchain class manage the frames to be displayed

Example :
``` C++
nvvk::SwapChain m_swapChain;
- The creation
m_swapChain.setup(m_physicalDevice, m_device, m_queue, m_graphicsQueueIndex, surface, colorFormat);
m_swapChain.create(m_size, vsync);
- On resize
m_swapChain.create(m_size, m_vsync);
m_framebuffers = m_swapChain.createFramebuffers(framebufferCreateInfo);
- Acquire the next image from the swap chain
vk::Result res = m_swapChain.acquire(m_acquireComplete, &m_curFramebuffer);
- Present the frame
vk::Result res = m_swapChain.present(m_curFramebuffer, m_renderComplete);
```

## utilities_vkpp.hpp

Various utility functions


**clearColor**: easy clear color contructor.


**createShaderModule**: create a shader module from SPIR-V code.


**linker**: link VK structures through their pNext

Example of Usage:
~~~~~~ C++
vk::Something a;
vk::SomethingElse b;
vk::Other c;
nvvk::util::linker(a,b,c); // will get a.pNext -> b.pNext -> c.pNext -> nullptr
~~~~~~




_____
auto-generated by `docgen.lua`
