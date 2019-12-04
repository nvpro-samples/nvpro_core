# Vulkan C Api Helpers

non-exhaustive list of utilities provided in the `nvvk` directory

## allocator_dma_vk.hpp

### class **nvvk::AllocatorDma**

The goal of the `AllocatorABC` classes is to have common work-flow
even if the underlying allocator classes are different.
This should make it relatively easy to switch between different
allocator implementations (more or less only changing typedefs).

The `BufferABC`, `ImageABC` etc. structs always contain the native
resource handle as well as the allocator system's handle.

This utility class wraps the usage of **nvvk::DeviceMemoryAllocator**
as well as **nvvk::StagingMemoryManager** to have a simpler interface
for handling resources with content uploads.

> Note: These classes are foremost to showcase principle components that
> a Vulkan engine would most likely have.
> They are geared towards ease of use in this sample framework, and 
> not optimized nor meant for production code.

~~~ C++
AllocatorDma allocator;
allocator.init(memAllocator, staging);

...

VkCommandBuffer cmd = ... transfer queue command buffer

// creates new resources and 
// implicitly triggers staging transfer copy operations into cmd
BufferDma vbo = allocator.createBuffer(cmd, vboSize, vboUsage, vboData);
BufferDma ibo = allocator.createBuffer(cmd, iboSize, iboUsage, iboData);

// use functions from staging memory manager
// here we associate the temporary staging space with a fence
allocator.finalizeStaging(fence);

// submit cmd buffer with staging copy operations
vkQueueSubmit(... cmd ... fence ...)

...

// if you do async uploads you would
// trigger garbage collection somewhere per frame
allocator.tryReleaseFencedStaging();

~~~

## allocator_dma_vkgl.hpp

This file contains helpers for resource interoperability between OpenGL and Vulkan.
they only exist if the shared_sources project is compiled with Vulkan AND OpenGL support.

> WARNING: untested code


### class **nvkk::AllocatorDmaGL**

This utility has the same operations like nvvk::AllocatorDMA (see for more help), but
targets interop between OpenGL and Vulkan.
It uses **nvkk::DeviceMemoryAllocatorGL** to provide **BufferDmaGL** and **ImageDmaGL** utility classes that wrap an **nvvk::AllocationID**
as well as the native Vulkan and OpenGL resource objects.

## allocator_vma_vk.hpp

### class **nvvk::StagingMemoryManagerVma**

This utility class wraps the usage of [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
to allocate the memory for **nvvk::StagingMemoryManager**


### class **nvvk::AllocatorVma**

This utility class wraps the usage of [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
as well as **nvvk::StagingMemoryManager** to have a simpler interface
for handling resources with content uploads.

See more details in description of **nvvk::AllocatorDma**.

## appwindowprofiler_vk.hpp

### class **nvvk::AppWindowProfilerVK**

**AppWindowProfilerVK** derives from **nvh::AppWindowProfiler**
and overrides the context and swapbuffer functions.

To influence the vulkan instance/device creation modify 
`m_contextInfo` prior running AppWindowProfiler::run,
which triggers instance, device, window, swapchain creation etc.

The class comes with a **nvvk::ProfilerVK** instance that references the 
AppWindowProfiler::m_profiler's data.

## axis_vk.hpp

Display an Axis representing the orientation of the camera in the bottom left corner of the window.
- Initialize the Axis using `init()`
- Add `display()` in a inline rendering pass, one of the lass command

Example:  
~~~~~~ C++
m_axis.display(cmdBuf, CameraManip.getMatrix(), windowSize);
~~~~~~

## buffers_vk.hpp

The utilities in this file provide a more direct approach, we encourage to use
higher-level mechanisms also provided in the allocator / memorymanagement classes.

### functions in nvvk

- **makeBufferCreateInfo** : wraps setup of `VkBufferCreateInfo` (implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT)
- **makeBufferViewCreateInfo** : wraps setup of `VkBufferViewCreateInfo`
- **createBuffer** : wraps `vkCreateBuffer` (implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT)
- **createBufferView** : wraps `vkCreateBufferView`

~~~ C++
VkBufferCreateInfo bufferCreate =   makeBufferCreateInfo (256, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
~~~


### class **nvvk::StagingBuffer**

Stage uploads to images and buffers using this *host visible buffer*.
After init, use `enqueue` operations for uploads, then execute via flushing
the commands to a commandbuffer that you execute.May need to use cannotEnqueue if
the allocated size is too small.Enqueues greater than initial bufferSize *will cause re - allocation * .

*You must synchronize explicitly!*

A single buffer and memory allocation is used, therefore new copy operations are
only safe once copy commands prior flush were completed.

> Look at the **nvvk::StagingMemoryManager** for a more sophisticated version
> that manages multiple staging buffers and allows easier use of asynchronous
> transfers.

Example:
~~~ C++
StagingBuffer staging;
staging.init(device, physicalDevice, 128 * 1024 * 1024);

staging.cmdToBuffer(cmd,targetBuffer, 0, targetSize, targetData);
staging.flush();

... submit cmd ...

// later once completed

staging.deinit();

~~~

## commands_vk.hpp

### functions in nvvk

- **makeAccessMaskPipelineStageFlags** : depending on accessMask returns appropriate `VkPipelineStageFlagBits`
- **cmdBegin** : wraps `vkBeginCommandBuffer` with `VkCommandBufferUsageFlags` and implicitly handles `VkCommandBufferBeginInfo` setup
- **makeSubmitInfo** : `VkSubmitInfo` struct setup using provided arrays of signals and commandbuffers, leaving rest zeroed


### class **nvvk::CmdPool**

**CmdPool** stores a single `VkCommandPool` and provides utility functions
to create `VkCommandBuffers` from it.


### class **nvvk::ScopeSubmitCmdPool**

**ScopeSubmitCmdPool** extends **CmdPool** and lives within a scope.
It directly submits its commandbufers to the provided queue.
Intent is for non-critical actions where performance is NOT required,
as it waits until the device has completed the operation.

Example:
``` C++
{
  nvvk::ScopeSubmitCmdPool scopePool(...);

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


### class **nvvk::ScopeSubmitCmdBuffer**

Provides a single `VkCommandBuffer` that lives within the scope
and is directly submitted, deleted when the scope is left.

Example:
``` C++
{
  ScopeSubmitCmdBuffer cmd(device, queue, queueFamilyIndex);
  ... do stuff
  vkCmdCopyBuffer(cmd, ...);
}
```


### classes **nvvk::Ring...**

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


#### class **nvvk::RingFences**

Recycles a fixed number of fences, provides information in which cycle
we are currently at, and prevents accidental access to a cycle inflight.

A typical frame would start by waiting for older cycle's completion ("wait")
and be ended by "advanceCycle".

Safely index other resources, for example ring buffers using the "getCycleIndex"
for the current frame.


#### class **nvvk::RingCmdPool**

Manages a fixed cycle set of `VkCommandBufferPools` and
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

  VkCommandBuffer cmd = ringPool.createCommandBuffer(...)
  ... do stuff / submit etc...

  VkFence fence = ringFences.advanceCycle();
  // use this fence in the submit
  vkQueueSubmit(...)
}
~~~


### class **nvvk::BatchSubmission**

Batches the submission arguments of `VkSubmitInfo` for VkQueueSubmit.

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

## context_vk.hpp

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
nvvk::ContextCreateInfo ctxInfo;
ctxInfo.setVersion(1, 1);
ctxInfo.addInstanceLayer("VK_LAYER_KHRONOS_validation");
ctxInfo.addInstanceLayer("VK_LAYER_LUNARG_monitor");
ctxInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME, false);
ctxInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, false);
ctxInfo.addInstanceExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
ctxInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME, false);
~~~~

then you are ready to create initialize `nvvk::Context`

> Note: In debug builds, the extension `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` and the layer `VK_LAYER_KHRONOS_validation` are added to help finding issues early.


### class **nvvk::Context**

**Context** class helps creating the Vulkan instance and to choose the logical device for the mandatory extensions. First is to fill the `ContextCreateInfo` structure, then call:

~~~ C++
// Creating the Vulkan instance and device
nvvk::ContextCreateInfo ctxInfo;
... see above ...

nvvk::Context vkctx;
vkctx.init(ctxInfo);

// after init the ctxInfo is no longer needed
~~~ 

At this point, the class will have created the `VkInstance` and `VkDevice` according to the information passed. It will also keeps track or have query the information of:
 
* Physical Device information that you can later query : `PhysicalDeviceInfo` in which lots of `VkPhysicalDevice...` are stored
* `VkInstance` : the one instance being used for the programm
* `VkPhysicalDevice` : physical device(s) used for the logical device creation. In case of more than one physical device, we have a std::vector for this purpose...
* `VkDevice` : the logical device instanciated
* `VkQueue` : we will enumerate all the available queues and make them available in `nvvk::Context`. Some queues are specialized, while other are for general purpose (most of the time, only one can handle everything, while other queues are more specialized). We decided to make them all available in some explicit way :
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
The above methods will transparently create the `VkDevice` using `VkDeviceGroupDeviceCreateInfo`.
Especially in the context of NVLink connected cards this is useful.

## descriptorsets_vk.hpp

### functions in nvvk

- **createDescriptorSetLayout** : wrappers for `vkCreateDescriptorSetLayout`
- **createDescriptorPool** : wrappers for `vkCreateDescriptorPool`
- **allocateDescriptorSet** : allocates a single `VkDescriptorSet`
- **allocateDescriptorSets** : allocates multiple VkDescriptorSets


### class **nvvk::DescriptorSetReflection**

Helper class that keeps a collection of `VkDescriptorSetLayoutBinding` for a single
`VkDescriptorSetLayout`. Provides helper functions to create `VkDescriptorSetLayout`
as well as `VkDescriptorPool` based on this information, as well as utilities
to fill the `VkWriteDescriptorSet` structure with binding information stored
within the class.

Example :
~~~C++
DescriptorSetReflection refl;

refl.addBinding( VIEW_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
refl.addBinding(XFORM_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);

VkDescriptorSetLayout layout = refl.createLayout(device);

// let's create a pool with 2 sets
VkDescriptorPool      pool   = refl.createPool(device, 2);

std::vector<VkWriteDescriptorSet> updates;

fill them
updates.push_back(refl.getWrite(0, VIEW_BINDING, &view0BufferInfo));
updates.push_back(refl.getWrite(1, VIEW_BINDING, &view1BufferInfo));
updates.push_back(refl.getWrite(0, XFORM_BINDING, &xform0BufferInfo));
updates.push_back(refl.getWrite(1, XFORM_BINDING, &xform1BufferInfo));

vkUpdateDescriptorSets(device, updates.size(), updates.data(), 0, nullptr);
~~~


### class **nvvk::DescriptorSetContainer**

Container class that stores allocated DescriptorSets
as well as reflection, layout and pool

Example:
~~~ C++
container.init(device, allocator);

// setup dset layouts
container.addBinding(0, UBO...)
container.addBinding(1, SSBO...)
container.initLayout();

// allocate descriptorsets
container.initPool(17);

// update descriptorsets
writeUpdates.push_back( container.getWrite(0, 0, ..) );
writeUpdates.push_back( container.getWrite(0, 1, ..) );
writeUpdates.push_back( container.getWrite(1, 0, ..) );
writeUpdates.push_back( container.getWrite(1, 1, ..) );
writeUpdates.push_back( container.getWrite(2, 0, ..) );
writeUpdates.push_back( container.getWrite(2, 1, ..) );
...

// at render time

vkCmdBindDescriptorSets(cmd, GRAPHICS, pipeLayout, 1, 1, container.at(7).getSets());
~~~


### class **nvvk::TDescriptorSetContainer**<SETS,PIPES=1>

Templated version of **DescriptorSetContainer** :

- SETS  - many **DescriptorSetContainers**
- PIPES - many `VkPipelineLayouts`

The pipeline layouts are stored separately, the class does
not use the pipeline layouts of the embedded **DescriptorSetContainers**.

Example :

~~~ C++
Usage, e.g.SETS = 2, PIPES = 2

container.init(device, allocator);

// setup dset layouts
container.at(0).addBinding(0, UBO...)
container.at(0).addBinding(1, SSBO...)
container.at(0).initLayout();
container.at(1).addBinding(0, COMBINED_SAMPLER...)
container.at(1).initLayout();

// pipe 0 uses set 0 alone
container.initPipeLayout(0, 1);
// pipe 1 uses sets 0, 1
container.initPipeLayout(1, 2);

// allocate descriptorsets
container.at(0).initPool(1);
container.at(1).initPool(16);

// update descriptorsets

writeUpdates.push_back(container.at(0).getWrite(0, 0, ..));
writeUpdates.push_back(container.at(0).getWrite(0, 1, ..));
writeUpdates.push_back(container.at(1).getWrite(0, 0, ..));
writeUpdates.push_back(container.at(1).getWrite(1, 0, ..));
writeUpdates.push_back(container.at(1).getWrite(2, 0, ..));
...

// at render time

vkCmdBindDescriptorSets(cmd, GRAPHICS, container.getPipeLayout(0), 0, 1, container.at(0).getSets());
..
vkCmdBindDescriptorSets(cmd, GRAPHICS, container.getPipeLayout(1), 1, 1, container.at(1).getSets(7));
~~~

## error_vk.hpp

### function nvvk::checkResult
Returns true on critical error result, logs errors.
Use `NVVK_CHECK(result)` to automatically log filename/linenumber.

## extensions_vk.hpp

### Vulkan Extension Loader

The extensions_vk files takes care of loading and providing the symbols of
Vulkan C Api extensions.
It is generated by `extensions_vk.lua` which contains a whitelist of
extensions to be made available.

The framework triggers this implicitly in the `nvvk::Context` class.

If you want to use it in your own code, see the instructions in the 
lua file how to generate it.

~~~ c++
// loads all known extensions
load_VK_EXTENSION_SUBSET(instance, vkGetInstanceProcAddr, device, vkGetDeviceProcAddr);

// load individual extension
load_VK_KHR_push_descriptor(instance, vkGetInstanceProcAddr, device, vkGetDeviceProcAddr);
~~~

## images_vk.hpp

### functions in nvvk

- **makeImageMemoryBarrier** : returns `VkImageMemoryBarrier` for an image based on provided layouts and access flags.
- **makeImageMemoryBarrierReversed** : returns `VkImageMemoryBarrier` that revereses the src/dst fields of the provided barrier.
- **setupImageMemoryBarrier** : same as makeImageMemoryBarrier but in-place operation on the reference.
- **reverseImageMemoryBarrier** : same as makeImageMemoryBarrierReversed but in-place operation on the reference.
- **createImage2D** : wraps `vkCreateImage` for basic 2d images
- **createImage2DView** : wraps `vkCreateImageView` for basic 2d images
- **cmdTransitionImage** : sets up the `VkImageMemoryBarrier` for layout transitions and triggers `vkCmdPipelineBarrier`
- **cmdBlitImage** : wraps vkCmdBlitImage


### class **DedicatedImage**

**DedicatedImages** have their own dedicated device memory allocation.
This can be beneficial for render pass attachments.

Also provides utility function setup the initial image layout.

## memorymanagement_vk.hpp

This framework assumes that memory heaps exists that support:

- VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
  for uploading data to the device
- VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & VK_MEMORY_PROPERTY_HOST_CACHED_BIT
  for downloading data from the device

This is typical on all major desktop platforms and vendors.
See http://vulkan.gpuinfo.org for information of various devices and platforms.

### functions in nvvk

* getMemoryInfo : fills the `VkMemoryAllocateInfo` based on device's memory properties and memory requirements and property flags. Returns `true` on success.


### class **nvvk::DeviceMemoryAllocator**

**DeviceMemoryAllocator** allocates and manages device memory in fixed-size memory blocks.

It sub-allocates from the blocks, and can re-use memory if it finds empty
regions. Because of the fixed-block usage, you can directly create resources
and don't need a phase to compute the allocation sizes first.

It will create compatible chunks according to the memory requirements and
usage flags. Therefore you can easily create mappable host allocations
and delete them after usage, without inferring device-side allocations.

We return `AllocationID` rather than the allocation details directly, which
you can query separately.

Several utility functions are provided to handle the binding of memory
directly with the resource creation of buffers, images and acceleration
structures. This utilities also make implicit use of Vulkan's dedicated
allocation mechanism.

> **WARNING** : The memory manager serves as proof of concept for some key concepts
> however it is not meant for production use and it currently lacks de-fragmentation logic
> as well. You may want to look at [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
> for a more production-focused solution.

You can derive from this calls and overload the 

Example :
~~~ C++
nvvk::DeviceMemoryAllocator memAllocator;

memAllocator.init(device, physicalDevice);

// low-level
aid = memAllocator.alloc(memRequirements,...);
...
memAllocator.free(aid);

// utility wrapper
buffer = memAllocator.createBuffer(bufferSize, bufferUsage, bufferAid);
...
memAllocator.free(bufferAid);


// It is also possible to not track individual resources
// and free everything in one go. However, this is
// not recommended for general purpose use.

bufferA = memAllocator.createBuffer(sizeA, usageA);
bufferB = memAllocator.createBuffer(sizeB, usageB);
...
memAllocator.freeAll();

~~~


### class **nvvk::StagingMemoryManager**

**StagingMemoryManager** class is a utility that manages host visible
buffers and their allocations in an opaque fashion to assist
asynchronous transfers between device and host.

The collection of such resources is represented by **nvvk::StagingID**.
The necessary memory is sub-allocated and recycled in blocks internally.

The default implementation will create one dedicated memory allocation per block.
You can derive from this class and overload the virtual functions,
if you want to use a different allocation system.

- **allocBlockMemory**
- **freeBlockMemory**

> **WARNING:**
> - cannot manage copy > 4 GB

Usage:
- Enqueue transfers into the provided `VkCommandBuffer` and then finalize the copy operations.
- You can either signal the completion of the operations manually by the **nvvk::StagingID**
  or implicitly by associating the copy operations with a VkFence.
- The release of the resources allows to safely recycle the memory for future transfers.

Example :

~~~ C++
StagingMemoryManager  staging;
staging.init(device, physicalDevice);


// Enqueue copy operations of data to target buffer.
// This internally manages the required staging resources
staging.cmdToBuffer(cmd, targetBufer, 0, targetSize, targetData);

// you can also get access to a temporary mapped pointer and fill
// the staging buffer directly
vertices = staging.cmdToBufferT<Vertex>(cmd, targetBufer, 0, targetSize);

// OPTION A:
// associate all previous copy operations with a fence
staging.finalizeCmds(myfence);

...

// every once in a while call
staging.tryReleaseFenced();

// OPTION B
// alternatively manage the resources and fence waiting yourself
sid = staging.finalizeCmds();

... need to ensure uploads completed

staging.recycleTask(sid);

~~~


### class **nvvk::StagingMemoryManagerDma**

Derives from **nvvk::StagingMemoryManager** and uses the referenced **nvvk::DeviceMemoryAllocator**
for allocations.

~~~ C++
DeviceMemoryAllocator    memAllocator;
memAllocator.init(device, physicalDevice);

StagingMemoryManagerDma  staging;
staging.init(memAllocator);

// rest as usual
staging.cmdToBuffer(cmd, targetBufer, 0, targetSize, targetData);
~~~

## memorymanagement_vkgl.hpp

This file contains helpers for resource interoperability between OpenGL and Vulkan.
they only exist if the shared_sources project is compiled with Vulkan AND OpenGL support.


### class **nvvk::DeviceMemoryAllocatorGL**

Derived from **nvvk::DeviceMemoryAllocator** it uses vulkan memory that is exported
and directly imported into OpenGL. Requires GL_EXT_memory_object.

Used just like the original class however a new function to get the 
GL memory object exists: `getAllocationGL`.

Look at source of **nvvk::AllocatorDmaGL** for usage.

## pipeline_vk.hpp

### class **nvvk::GraphicsPipelineState**

**GraphicsPipelineState** wraps `VkGraphicsPipelineCreateInfo`
as well as the structures it points to. It also comes with
"sane" default values. This makes it easier to generate a 
graphics `VkPipeline` and also allows to keep the configuration
around.

Example :
~~~ C++
m_gfxState = nvvk::GraphicsPipelineState();

m_gfxState.addVertexInputAttribute(VERTEX_POS_OCTNORMAL, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0);
m_gfxState.addVertexInputBinding(0, sizeof(CadScene::Vertex));
m_gfxState.setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

m_gfxState.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
m_gfxState.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);

m_gfxState.setDepthTest(true, true, VK_COMPARE_OP_LESS);
m_gfxState.setRasterizationSamples(drawPassSamples);
m_gfxState.setAttachmentColorMask(0, 
 VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

m_gfxState.setRenderPass(drawPass);
m_gfxState.setPipelineLayout(drawPipeLayout);

for(uint32_t m = 0; m < NUM_MATERIAL_SHADERS; m++)
{
  m_gfxState.clearShaderStages();
  m_gfxState.addShaderStage(VK_SHADER_STAGE_VERTEX_BIT,   vertexShaders[m]);
  m_gfxState.addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaders[m]);

  result = vkCreateGraphicsPipelines(m_device, nullptr, 1, m_gfxState, nullptr, &pipelines[m]);
}
~~~

## profiler_vk.hpp

### class **nvvk::ProfilerVK**

**ProfilerVK** derives from **nvh::Profiler** and uses `vkCmdWriteTimestamp`
to measure the gpu time within a section.

If profiler.setMarkerUsage(true) was used then it will make use
of `vkCmdDebugMarkerBeginEXT` and `vkCmdDebugMarkerEndEXT` for each
section so that it shows up in tools like NsightGraphics and renderdoc.

Currently the commandbuffers must support `vkCmdResetQueryPool` as well.

When multiple queues are used there could be problems with the "nesting"
of sections. In that case multiple profilers, one per queue, are most
likely better.


Example:

``` c++
nvv::ProfilerVK profiler;
std::string     profilerStats;

profiler.init(device, physicalDevice);
profiler.setMarkerUsage(true); // depends on VK_EXT_debug_utils

while(true)
{
  profiler.beginFrame();

  ... setup frame ...


  {
    // use the Section class to time the scope
    auto sec = profiler.timeRecurring("draw", cmd);

    vkCmdDraw(cmd, ...);
  }

  ... submit cmd buffer ...

  profiler.endFrame();

  // generic print to string
  profiler.print(profilerStats);

  // or access data directly
  nvh::Profiler::TimerInfo info;
  if( profiler.getTimerInfo("draw", info)) {
    // do some updates
    updateProfilerUi("draw", info.gpu.average);
  }
}

```

## renderpasses_vk.hpp

### functions in nvvk

- **findSupportedFormat** : returns supported `VkFormat` from a list of candidates (returns first match)
- **findDepthFormat** : returns supported depth format (24, 32, 16-bit)
- **findDepthStencilFormat** : returns supported depth-stencil format (24/8, 32/8, 16/8-bit)
- **createRenderPass** : wrapper for vkCreateRenderPass

## shadermodulemanager_vk.hpp

### class **nvvk::ShaderModuleManager**

The **ShaderModuleManager** manages `VkShaderModules` stored in files (SPIR-V or GLSL)

Using **ShaderFileManager** it will find the files and resolve #include for GLSL.
You must register includes to the base-class for this.

It also comes with some convenience functions to reload shaders etc.
That is why we pass out the **ShaderModuleID** rather than a `VkShaderModule` directly.

To change the compilation behavior manipulate the public member variables
prior createShaderModule.

m_filetype is crucial for this. You can pass raw spir-v files or GLSL.
If GLSL is used either the backdoor m_useNVextension must be used, or
preferrable shaderc (which must be added via _add_package_ShaderC() in CMake of the project)

Example:

``` c++
ShaderModuleManager mgr(myDevice);

// derived from ShaderFileManager
mgr.addDirectory("shaders/");
mgr.registerInclude("noise.glsl");

// all shaders get this injected after #version statement
mgr.m_prepend = "#define USE_NOISE 1\n";

vid = mgr.createShaderModule(VK_SHADER_STAGE_VERTEX_BIT,   "object.vert.glsl");
fid = mgr.createShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "object.frag.glsl");

// ... later use module
info.module = mgr.get(vid);
```

## shaders_vk.hpp

### function in nvvk

returns `VkShaderModule` and is overloaded for several inputs

- **createShaderModule** : from input as const uint32_t* binarycode
- **createShaderModule** : from input as const char* binarycode
- **createShaderModule** : from input as std::vector<char>&
- **createShaderModule** : from input as std::vector<uint32_t>&
- **createShaderModule** : from input as std::string&

## structs_vk.hpp

### function nvvk::make, nvvk::clear
Contains templated `nvvk::make<T>` and `nvvk::clear<T>` functions that are 
auto-generated by `structs.lua`. The functions provide default 
structs for the Vulkan C api by initializing the `VkStructureType sType`
field (also for nested structs) and clearing the rest to zero.

``` c++
auto compCreateInfo = nvvk::make<VkComputePipelineCreateInfo>;
```

## swapchain_vk.hpp

### class **nvvk::SwapChain**

Its role is to help using VkSwapchainKHR. In Vulkan we have 
to synchronize the backbuffer access ourselves, meaning we
must not write into images that the operating system uses for
presenting the image on the desktop or monitor.


To setup the swapchain :

* get the window handle
* create its related surface
* make sure the **Queue** is the one we need to render in this surface

~~~ C++
// could be arguments of a function/method :
nvvk::Context ctx;
NVPWindow     win;
...

// get the surface of the window in which to render
VkWin32SurfaceCreateInfoKHR createInfo = {};
... populate the fields of createInfo ...
createInfo.hwnd = glfwGetWin32Window(win.m_internal);
result = vkCreateWin32SurfaceKHR(ctx.m_instance, &createInfo, nullptr, &m_surface);

...
// make sure we assign the proper Queue to m_queueGCT, from what the surface tells us
ctx.setGCTQueueWithPresent(m_surface);
~~~

The initialization can happen now :

~~~ C+
m_swapChain.init(ctx.m_device, ctx.m_physicalDevice, ctx.m_queueGCT, ctx.m_queueGCT.familyIndex, 
                 m_surface, VK_FORMAT_B8G8R8A8_UNORM);
...
// after init or update you also have to setup the image layouts at some point
VkCommandBuffer cmd = ...
m_swapChain.cmdUpdateBarriers(cmd);
~~~

During a resizing of a window, you must update the swapchain as well :

~~~ C++
bool WindowSurface::resize(int w, int h)
{
...
  m_swapChain.update(w, h);
  // be cautious to also transition the image layouts
...
}
~~~


A typical renderloop would look as follows:

~~~ C++
// handles vkAcquireNextImageKHR and setting the active image
if(!m_swapChain.acquire())
{
  ... handle acquire error
}

VkCommandBuffer cmd = ...

if (m_swapChain.getChangeID() != lastChangeID){
  // after init or resize you have to setup the image layouts
  m_swapChain.cmdUpdateBarriers(cmd);

  lastChangeID = m_swapChain.getChangeID();
}

// do render operations either directly using the imageview
VkImageView swapImageView = m_swapChain.getActiveImageView();

// or you may always render offline int your own framebuffer
// and then simply blit into the backbuffer
VkImage swapImage = m_swapChain.getActiveImage();
vkCmdBlitImage(cmd, ... swapImage ...);

// present via a queue that supports it
m_swapChain.present(m_queue);
~~~




_____
auto-generated by `docgen.lua`
