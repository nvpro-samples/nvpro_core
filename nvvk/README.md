## Table of Contents
- [appwindowprofiler_vk.hpp](#appwindowprofiler_vkhpp)
- [buffersuballocator_vk.hpp](#buffersuballocator_vkhpp)
- [buffers_vk.hpp](#buffers_vkhpp)
- [commands_vk.hpp](#commands_vkhpp)
- [compute_vk.hpp](#compute_vkhpp)
- [context_vk.hpp](#context_vkhpp)
- [debug_util_vk.hpp](#debug_util_vkhpp)
- [descriptorsets_vk.hpp](#descriptorsets_vkhpp)
- [dynamicrendering_vk.hpp](#dynamicrendering_vkhpp)
- [error_vk.hpp](#error_vkhpp)
- [extensions_vk.hpp](#extensions_vkhpp)
- [gizmos_vk.hpp](#gizmos_vkhpp)
- [images_vk.hpp](#images_vkhpp)
- [memallocator_dedicated_vk.hpp](#memallocator_dedicated_vkhpp)
- [memallocator_dma_vk.hpp](#memallocator_dma_vkhpp)
- [memallocator_vk.hpp](#memallocator_vkhpp)
- [memallocator_vma_vk.hpp](#memallocator_vma_vkhpp)
- [memorymanagement_vk.hpp](#memorymanagement_vkhpp)
- [memorymanagement_vkgl.hpp](#memorymanagement_vkglhpp)
- [pipeline_vk.hpp](#pipeline_vkhpp)
- [profiler_vk.hpp](#profiler_vkhpp)
- [raypicker_vk.hpp](#raypicker_vkhpp)
- [raytraceKHR_vk.hpp](#raytraceKHR_vkhpp)
- [raytraceNV_vk.hpp](#raytraceNV_vkhpp)
- [renderpasses_vk.hpp](#renderpasses_vkhpp)
- [resourceallocator_vk.hpp](#resourceallocator_vkhpp)
- [samplers_vk.hpp](#samplers_vkhpp)
- [sbtwrapper_vk.hpp](#sbtwrapper_vkhpp)
- [shadermodulemanager_vk.hpp](#shadermodulemanager_vkhpp)
- [shaders_vk.hpp](#shaders_vkhpp)
- [sparse_image_vk.hpp](#sparse_image_vkhpp)
- [specialization.hpp](#specializationhpp)
- [stagingmemorymanager_vk.hpp](#stagingmemorymanager_vkhpp)
- [structs_vk.hpp](#structs_vkhpp)
- [swapchain_vk.hpp](#swapchain_vkhpp)
- [vulkanhppsupport.hpp](#vulkanhppsupporthpp)
- [vulkanhppsupport_vkgl.hpp](#vulkanhppsupport_vkglhpp)

## appwindowprofiler_vk.hpp
### class nvvk::AppWindowProfilerVK


nvvk::AppWindowProfilerVK derives from nvh::AppWindowProfiler
and overrides the context and swapbuffer functions.
The nvh class itself provides several utilities and
command line options to run automated benchmarks etc.

To influence the vulkan instance/device creation modify
`m_contextInfo` prior running AppWindowProfiler::run,
which triggers instance, device, window, swapchain creation etc.

The class comes with a nvvk::ProfilerVK instance that references the
AppWindowProfiler::m_profiler's data.

## buffersuballocator_vk.hpp
### class nvvk::BufferSubAllocator


nvvk::BufferSubAllocator provides buffer sub allocation using larger buffer blocks.
The blocks are one VkBuffer each and are allocated via the
provided [nvvk::MemAllocator](#class-nvvkmemallocator).

The requested buffer space is sub-allocated and recycled in blocks internally.
This way we avoid creating lots of small VkBuffers and can avoid calling the Vulkan
API at all, when there are blocks with sufficient empty space.
While Vulkan is more efficient than previous APIs, creating lots
of objects for it, is still not good for overall performance. It will result
into more cache misses and use more system memory over all.

Be aware that each sub-allocation is always BASE_ALIGNMENT aligned.
A custom alignment during allocation can be requested, it will ensure
that the returned sub-allocation range of offset & size can account for
the original requested size fitting within and respecting the requested

This, however, means the regular offset and may not match the requested
alignment, and the regular size can be bigger to account for the shift
caused by manual alignment.

It is therefore necessary to pass the alignment that was used at allocation time
to the query functions as well.

```cpp
// alignment <= BASE_ALIGNMENT
handle  = subAllocator.subAllocate(size);
binding = subAllocator.getSubBinding(handle);

// alignment > BASE_ALIGNMENT
handle  = subAllocator.subAllocate(size, alignment);
binding = subAllocator.getSubBinding(handle, alignment);
```

## buffers_vk.hpp
The utilities in this file provide a more direct approach, we encourage to use
higher-level mechanisms also provided in the allocator / memorymanagement classes.

### functions in nvvk


- makeBufferCreateInfo : wraps setup of VkBufferCreateInfo (implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT)
- makeBufferViewCreateInfo : wraps setup of VkBufferViewCreateInfo
- createBuffer : wraps vkCreateBuffer
- createBufferView : wraps vkCreateBufferView
- getBufferDeviceAddressKHR : wraps vkGetBufferDeviceAddressKHR
- getBufferDeviceAddress : wraps vkGetBufferDeviceAddress

```cpp
VkBufferCreateInfo bufferCreate = makeBufferCreateInfo (size, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
VkBuffer buffer                 = createBuffer(device, bufferCreate);
VkBufferView bufferView         = createBufferView(device, makeBufferViewCreateInfo(buffer, VK_FORMAT_R8G8B8A8_UNORM, size));
```

## commands_vk.hpp
### functions in nvvk


- makeAccessMaskPipelineStageFlags : depending on accessMask returns appropriate VkPipelineStageFlagBits
- cmdBegin : wraps vkBeginCommandBuffer with VkCommandBufferUsageFlags and implicitly handles VkCommandBufferBeginInfo setup
- makeSubmitInfo : VkSubmitInfo struct setup using provided arrays of signals and commandbuffers, leaving rest zeroed
### class nvvk::CommandPool


nvvk::CommandPool stores a single VkCommandPool and provides utility functions
to create VkCommandBuffers from it.

Example:
```cpp
{
nvvk::CommandPool cmdPool;
cmdPool.init(...);

// some setup/one shot work
{
vkCommandBuffer cmd = scopePool.createAndBegin();
... record commands ...
// trigger execution with a blocking operation
// not recommended for performance
// but useful for sample setup
scopePool.submitAndWait(cmd, queue);
}

// other cmds you may batch, or recycle
std::vector<VkCommandBuffer> cmds;
{
vkCommandBuffer cmd = scopePool.createAndBegin();
... record commands ...
cmds.push_back(cmd);
}
{
vkCommandBuffer cmd = scopePool.createAndBegin();
... record commands ...
cmds.push_back(cmd);
}

// do some form of batched submission of cmds

// after completion destroy cmd
cmdPool.destroy(cmds.size(), cmds.data());
cmdPool.deinit();
}
```
### class nvvk::ScopeCommandBuffer


nvvk::ScopeCommandBuffer provides a single VkCommandBuffer that lives within the scope
and is directly submitted and deleted when the scope is left.
Not recommended for efficiency, since it results in a blocking
operation, but aids sample writing.

Example:
```cpp
{
ScopeCommandBuffer cmd(device, queueFamilyIndex, queue);
... do stuff
vkCmdCopyBuffer(cmd, ...);
}
```
### class **nvvk::Ring...**


In real-time processing, the CPU typically generates commands
in advance to the GPU and send them in batches for execution.

To avoid having the CPU to wait for the GPU'S completion and let it "race ahead"
we make use of double, or tripple-buffering techniques, where we cycle through
a pool of resources every frame. We know that those resources are currently
not in use by the GPU and can therefore manipulate them directly.

Especially in Vulkan it is the developer's responsibility to avoid such
access of resources that are in-flight.

The "Ring" classes cycle through a pool of resources. The default value
is set to allow two frames in-flight, assuming one fence is used per-frame.
### class nvvk::RingFences


nvvk::RingFences recycles a fixed number of fences, provides information in which cycle
we are currently at, and prevents accidental access to a cycle in-flight.

A typical frame would start by "setCycleAndWait", which waits for the
requested cycle to be available.
## class nvvk::RingCommandPool

nvvk::RingCommandPool manages a fixed cycle set of VkCommandBufferPools and
one-shot command buffers allocated from them.

The usage of multiple command buffer pools also means we get nice allocation
behavior (linear allocation from frame start to frame end) without fragmentation.
If we were using a single command pool over multiple frames, it could fragment easily.

You must ensure cycle is available manually, typically by keeping in sync
with ring fences.

Example:

```cpp
{
frame++;

// wait until we can use the new cycle
// (very rare if we use the fence at then end once per-frame)
ringFences.setCycleAndWait( frame );

// update cycle state, allows recycling of old resources
ringPool.setCycle( frame );

VkCommandBuffer cmd = ringPool.createCommandBuffer(...);
... do stuff / submit etc...

VkFence fence = ringFences.getFence();
// use this fence in the submit
vkQueueSubmit(...fence..);
}
```
### class nvvk::BatchSubmission


nvvk::BatchSubmission batches the submission arguments of VkSubmitInfo for VkQueueSubmit.

vkQueueSubmit is a rather costly operation (depending on OS)
and should be avoided to be done too often (e.g. < 10 per frame). Therefore
this utility class allows adding commandbuffers, semaphores etc. and
submit them later in a batch.

When using manual locks, it can also be useful to feed commandbuffers
from different threads and then later kick it off.

Example

```cpp
// within upload logic
{
semTransfer = handleUpload(...);
// for example trigger async upload on transfer queue here
vkQueueSubmit(... semTransfer ...);

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
// enqueue some graphics work for submission
graphicsSubmission.enqueue(getSceneCmdBuffer());
graphicsSubmission.enqueue(getUiCmdBuffer());

graphicsSubmission.execute(frameFence);
}
```
### class nvvk::FencedCommandPools


nvvk::FencedCommandPools container class contains the typical utilities to handle
command submission. It contains RingFences, RingCommandPool and BatchSubmission
with a convenient interface.


## compute_vk.hpp
### class nvvk::PushComputeDispatcher


nvvk::PushComputeDispatcher is a convenience structure for easily creating
compute-only pipelines by defining the bindings and providing SPV code.
The descriptor set updates are carried out using the KHR_push_descriptor
extension.


Example:

```cpp

enum BindingLocation
{
eMyBindingLocation = 0
};

struct PushConstant{
...
}
pushConstant;

nvvk::PushComputeDispatcher<PushConstant, BindingLocation> myCompute;
VkBuffer myFirstBuffer = createMyFirstBuffer(...);
VkBuffer mySecondBuffer = createMySecondBuffer(...);
VkDevice device = getMyVkDevice(...);
const uint8_t* spvCode = getMyComputeShaderCode(...);
size_t spvCodeSize = getMyComputeShaderCodeSize(...);
myCompute.addBufferBinding(BindingLocation::eMyBindingLocation, myFirstBuffer);
myCompute.setCode(device, spvCode, spvCodeSize);
myCompute.finalizePipeline(device);

...
VkCommandBuffer cmd = getMyCommandBuffer(...);
myCompute.dispatch(cmd, targetThreadCount, &pushConstant);
...
myCompute.updateBufferBinding(BindingLocation::eMyBindingLocation, mySecondBuffer)
myCompute.dispatch(cmd, targetThreadCount, &pushConstant);
...
```

## context_vk.hpp
To run a Vulkan application, you need to create the Vulkan instance and device.
This is done using the `nvvk::Context`, which wraps the creation of `VkInstance`
and `VkDevice`.

First, any application needs to specify how instance and device should be created:
Version, layers, instance and device extensions influence the features available.
This is done through a temporary and intermediate class that will allow you to gather
all the required conditions for the device creation.
### struct ContextCreateInfo


This structure allows the application to specify a set of features
that are expected for the creation of
- VkInstance
- VkDevice

It is consumed by the `nvvk::Context::init` function.

Example on how to populate information in it :

```cpp
nvvk::ContextCreateInfo ctxInfo;
ctxInfo.setVersion(1, 2);
ctxInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME, false);
ctxInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, false);
ctxInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME, false);

// adding an extension with a feature struct:
//
VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR pipePropFeatures = {
VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR};
// Be aware of the lifetime of the pointer of the feature struct.
// ctxInfo stores the pointer directly and context init functions use it for read & write access.
ctxInfo.addDeviceExtension(VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME, true, &pipePropFeatures);

// disabling a feature:
//
// This callback is called after the feature structs were filled with physical device information
// and prior logical device creation.
// The callback iterates over all feature structs, including those from
// the vulkan versions.
ctxInfo.fnDisableFeatures = [](VkStructureType sType, void *pFeatureStruct)
{
switch(sType){
case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES:
{
auto* features11 = reinterpret_cast<VkPhysicalDeviceVulkan11Features>(pFeatureStruct);
// at this point the struct is populated with what the device supports
// and therefore it is only legal to disable features, not enable them.

// let's say we wanted to disable multiview
features11->multiView = VK_FALSE;
}
break;
default:
break;
}
};

```

then you are ready to create initialize `nvvk::Context`

> Note: In debug builds, the extension `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` and the layer `VK_LAYER_KHRONOS_validation` are added to help finding issues early.

### class nvvk::Context


nvvk::Context class helps creating the Vulkan instance and to choose the logical device for the mandatory extensions. First is to fill the `ContextCreateInfo` structure, then call:

```cpp
// Creating the Vulkan instance and device
nvvk::ContextCreateInfo ctxInfo;
... see above ...

nvvk::Context vkctx;
vkctx.init(ctxInfo);

// after init the ctxInfo is no longer needed
```

At this point, the class will have created the `VkInstance` and `VkDevice` according to the information passed. It will also keeps track or have query the information of:

* Physical Device information that you can later query : `PhysicalDeviceInfo` in which lots of `VkPhysicalDevice...` are stored
* `VkInstance` : the one instance being used for the program
* `VkPhysicalDevice` : physical device(s) used for the logical device creation. In case of more than one physical device, we have a std::vector for this purpose...
* `VkDevice` : the logical device instantiated
* `VkQueue` : By default, 3 queues are created, one per family: Graphic-Compute-Transfer, Compute and Transfer.
For any additionnal queue, they need to be requested with `ContextCreateInfo::addRequestedQueue()`. This is creating information of the best suitable queues,
but not creating them. To create the additional queues,
`Context::createQueue()` **must be call after** creating the Vulkan context.
</br>The following queues are always created and can be directly accessed without calling createQueue :
* `Queue m_queueGCT` : Graphics/Compute/Transfer Queue + family index
* `Queue m_queueT` : async Transfer Queue + family index
* `Queue m_queueC` : async Compute Queue + family index
* maintains what extensions are finally available
* implicitly hooks up the debug callback

## Choosing the device
When there are multiple devices, the `init` method is choosing the first compatible device available, but it is also possible the choose another one.
```cpp
vkctx.initInstance(deviceInfo);
// Find all compatible devices
auto compatibleDevices = vkctx.getCompatibleDevices(deviceInfo);
assert(!compatibleDevices.empty());

// Use first compatible device
vkctx.initDevice(compatibleDevices[0], deviceInfo);
```

## Multi-GPU

When multiple graphic cards should be used as a single device, the `ContextCreateInfo::useDeviceGroups` need to be set to `true`.
The above methods will transparently create the `VkDevice` using `VkDeviceGroupDeviceCreateInfo`.
Especially in the context of NVLink connected cards this is useful.


## debug_util_vk.hpp

> Todo: Add documentation

## descriptorsets_vk.hpp
### functions in nvvk


- createDescriptorPool : wrappers for vkCreateDescriptorPool
- allocateDescriptorSet : allocates a single VkDescriptorSet
- allocateDescriptorSets : allocates multiple VkDescriptorSets

### class nvvk::DescriptorSetBindings


nvvk::DescriptorSetBindings is a helper class that keeps a vector of `VkDescriptorSetLayoutBinding` for a single
`VkDescriptorSetLayout`. Provides helper functions to create `VkDescriptorSetLayout`
as well as `VkDescriptorPool` based on this information, as well as utilities
to fill the `VkWriteDescriptorSet` structure with binding information stored
within the class.

The class comes with the convenience functionality that when you make a
VkWriteDescriptorSet you provide the binding slot, rather than the
index of the binding's storage within this class. This results in a small
linear search, but makes it easy to change the content/order of bindings
at creation time.

Example :
```cpp
DescriptorSetBindings binds;

binds.addBinding( VIEW_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
binds.addBinding(XFORM_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);

VkDescriptorSetLayout layout = binds.createLayout(device);

#if SINGLE_LAYOUT_POOL
// let's create a pool with 2 sets
VkDescriptorPool      pool   = binds.createPool(device, 2);
#else
// if you want to combine multiple layouts into a common pool
std::vector<VkDescriptorPoolSize> poolSizes;
bindsA.addRequiredPoolSizes(poolSizes, numSetsA);
bindsB.addRequiredPoolSizes(poolSizes, numSetsB);
VkDescriptorPool      pool   = nvvk::createDescriptorPool(device, poolSizes,
numSetsA + numSetsB);
#endif

// fill them
std::vector<VkWriteDescriptorSet> updates;

updates.push_back(binds.makeWrite(0, VIEW_BINDING, &view0BufferInfo));
updates.push_back(binds.makeWrite(1, VIEW_BINDING, &view1BufferInfo));
updates.push_back(binds.makeWrite(0, XFORM_BINDING, &xform0BufferInfo));
updates.push_back(binds.makeWrite(1, XFORM_BINDING, &xform1BufferInfo));

vkUpdateDescriptorSets(device, updates.size(), updates.data(), 0, nullptr);
```
### class nvvk::DescriptorSetContainer


nvvk::DescriptorSetContainer is a container class that stores allocated DescriptorSets
as well as reflection, layout and pool for a single
VkDescripterSetLayout.

Example:
```cpp
container.init(device, allocator);

// setup dset layouts
container.addBinding(0, UBO...)
container.addBinding(1, SSBO...)
container.initLayout();

// allocate descriptorsets
container.initPool(17);

// update descriptorsets
writeUpdates.push_back( container.makeWrite(0, 0, &..) );
writeUpdates.push_back( container.makeWrite(0, 1, &..) );
writeUpdates.push_back( container.makeWrite(1, 0, &..) );
writeUpdates.push_back( container.makeWrite(1, 1, &..) );
writeUpdates.push_back( container.makeWrite(2, 0, &..) );
writeUpdates.push_back( container.makeWrite(2, 1, &..) );
...

// at render time

vkCmdBindDescriptorSets(cmd, GRAPHICS, pipeLayout, 1, 1, container.at(7).getSets());
```

### class nvvk::TDescriptorSetContainer<SETS,PIPES=1>


nvvk::TDescriptorSetContainer is a templated version of DescriptorSetContainer :

- SETS  - many DescriptorSetContainers
- PIPES - many VkPipelineLayouts

The pipeline layouts are stored separately, the class does
not use the pipeline layouts of the embedded DescriptorSetContainers.

Example :

```cpp
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

writeUpdates.push_back(container.at(0).makeWrite(0, 0, &..));
writeUpdates.push_back(container.at(0).makeWrite(0, 1, &..));
writeUpdates.push_back(container.at(1).makeWrite(0, 0, &..));
writeUpdates.push_back(container.at(1).makeWrite(1, 0, &..));
writeUpdates.push_back(container.at(1).makeWrite(2, 0, &..));
...

// at render time

vkCmdBindDescriptorSets(cmd, GRAPHICS, container.getPipeLayout(0), 0, 1, container.at(0).getSets());
..
vkCmdBindDescriptorSets(cmd, GRAPHICS, container.getPipeLayout(1), 1, 1, container.at(1).getSets(7));
```

## dynamicrendering_vk.hpp
### struct nvvk::RenderPass


>  Wrapper for VkRenderingInfoKHR

This class is a wrapper around the VkRenderingInfoKHR structure, which is used to create a render pass with dynamic attachments.

## error_vk.hpp
### Function nvvk::checkResult

>  Returns true on critical error result, logs errors.

Use `NVVK_CHECK(result)` to automatically log filename/linenumber.
### Function nvvk::setCheckResultHook

>  Allow replacing nvvk::checkResult() calls. E.g. to catch
`VK_ERROR_DEVICE_LOST` and wait for aftermath to write the crash dump.

## extensions_vk.hpp

### Function load_VK_EXTENSIONS

>  load_VK_EXTENSIONS : Vulkan Extension Loader

The extensions_vk files takes care of loading and providing the symbols of Vulkan C Api extensions.
It is generated by `extensions_vk.py` and generates all extensions found in vk.xml. See script for details.
.

The framework triggers this implicitly in the `nvvk::Context` class, immediately after creating the device.

```cpp
// loads all known extensions
load_VK_EXTENSIONS(instance, vkGetInstanceProcAddr, device, vkGetDeviceProcAddr);
```


## gizmos_vk.hpp
### class nvvk::Axis


nvvk::Axis displays an Axis representing the orientation of the camera in the bottom left corner of the window.
- Initialize the Axis using `init()`
- Add `display()` in a inline rendering pass, one of the lass command

Example:
```cpp
m_axis.display(cmdBuf, CameraManip.getMatrix(), windowSize);
```

## images_vk.hpp
### functions in nvvk


- makeImageMemoryBarrier : returns VkImageMemoryBarrier for an image based on provided layouts and access flags.
- mipLevels : return number of mips for 2d/3d extent

- accessFlagsForImageLayout : helps resource transtions
- pipelineStageForLayout : helps resource transitions
- cmdBarrierImageLayout : inserts barrier for image transition

- cmdGenerateMipmaps : basic mipmap creation for images (meant for one-shot operations)

- makeImage2DCreateInfo : aids 2d image creation
- makeImage3DCreateInfo : aids 3d descriptor set updating
- makeImageCubeCreateInfo : aids cube descriptor set updating
- makeImageViewCreateInfo : aids common image view creation, derives info from VkImageCreateInfo
- makeImage2DViewCreateInfo : aids 2d image view creation

## memallocator_dedicated_vk.hpp
### class nvvk::DedicatedMemoryAllocator

nvvk::DedicatedMemoryAllocator is a simple implementation of the MemAllocator interface, using
one vkDeviceMemory allocation per allocMemory() call. The simplicity of the implementation is
bought with potential slowness (vkAllocateMemory tends to be very slow) and running
out of operating system resources quickly (as some OSs limit the number of physical
memory allocations per process).

## memallocator_dma_vk.hpp
### class nvvk::DMAMemoryAllocator

nvvk::DMAMemoryAllocator is  using nvvk::DeviceMemoryAllocator internally.
nvvk::DeviceMemoryAllocator derives from nvvk::MemAllocator as well, so this class here is for those prefering a reduced wrapper;

## memallocator_vk.hpp
### class nvvk::MemHandle


nvvk::MemHandle represents a memory allocation or sub-allocation from the
generic nvvk::MemAllocator interface. Ideally use `nvvk::NullMemHandle` for
setting to 'NULL'. MemHandle may change to a non-pointer type in future.

### class nvvk::MemAllocateInfo


nvvk::MemAllocateInfo is collecting almost all parameters a Vulkan allocation could potentially need.
This keeps MemAllocator's interface simple and extensible.
### class nvvk::MemAllocator


nvvk::MemAllocator is a Vulkan memory allocator interface extensively used by ResourceAllocator.
It provides means to allocate, free, map and unmap pieces of Vulkan device memory.
Concrete implementations derive from nvvk::MemoryAllocator.
They can implement the allocator dunctionality themselves or act as an adapter to another
memory allocator implementation.

A nvvk::MemAllocator hands out opaque 'MemHandles'. The implementation of the MemAllocator interface
may chose any type of payload to store in a MemHandle. A MemHandle's relevant information can be
retrieved via getMemoryInfo().

## memallocator_vma_vk.hpp
### class nvvk::VMAMemoryAllocator

nvvk::VMAMemoryAllocator using the GPUOpen [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) underneath.
As VMA comes as a header-only library, when using it you'll have to:
1) provide _add_package_VMA() in your CMakeLists.txt
2) put these lines into one of your compilation units:
```cpp
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
```
### class nvvk::ResourceAllocatorVMA

nvvk::ResourceAllocatorVMA is a convencience class creating, initializing and owning a nvvk::VmaAllocator
and associated nvvk::MemAllocator object.

## memorymanagement_vk.hpp
This framework assumes that memory heaps exists that support:

- VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
for uploading data to the device
- VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & VK_MEMORY_PROPERTY_HOST_CACHED_BIT
for downloading data from the device

This is typical on all major desktop platforms and vendors.
See http://vulkan.gpuinfo.org for information of various devices and platforms.

### functions in nvvk


* getMemoryInfo : fills the VkMemoryAllocateInfo based on device's memory properties and memory requirements and property flags. Returns `true` on success.
### class nvvk::DeviceMemoryAllocator


The nvvk::DeviceMemoryAllocator allocates and manages device memory in fixed-size memory blocks.
It implements the nvvk::MemAllocator interface.

It sub-allocates from the blocks, and can re-use memory if it finds empty
regions. Because of the fixed-block usage, you can directly create resources
and don't need a phase to compute the allocation sizes first.

It will create compatible chunks according to the memory requirements and
usage flags. Therefore you can easily create mappable host allocations
and delete them after usage, without inferring device-side allocations.

An `AllocationID` is returned rather than the allocation details directly, which
one can query separately.

Several utility functions are provided to handle the binding of memory
directly with the resource creation of buffers, images and acceleration
structures. These utilities also make implicit use of Vulkan's dedicated
allocation mechanism.

We recommend the use of the nvvk::ResourceAllocator class,
rather than the various create functions provided here, as we may deprecate them.

> **WARNING** : The memory manager serves as proof of concept for some key concepts
> however it is not meant for production use and it currently lacks de-fragmentation logic
> as well. You may want to look at [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
> for a more production-focused solution.

You can derive from this class and overload a few functions to alter the
chunk allocation behavior.

Example :
```cpp
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

```

## memorymanagement_vkgl.hpp
This file contains helpers for resource interoperability between OpenGL and Vulkan.
they only exist if the nvpro_core project is compiled with Vulkan AND OpenGL support.
### class nvvk::DeviceMemoryAllocatorGL


nvvk::DeviceMemoryAllocatorGL is derived from nvvk::DeviceMemoryAllocator it uses vulkan memory that is exported
and directly imported into OpenGL. Requires GL_EXT_memory_object.

Used just like the original class however a new function to get the
GL memory object exists: `getAllocationGL`.

Look at source of nvvk::AllocatorDmaGL for usage.

## pipeline_vk.hpp
### functions in nvvk


- nvprintPipelineStats : prints stats of the pipeline using VK_KHR_pipeline_executable_properties (don't forget to enable extension and set VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR)
- dumpPipelineStats    : dumps stats of the pipeline using VK_KHR_pipeline_executable_properties to a text file (don't forget to enable extension and set VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR)
- dumpPipelineBinCodes : dumps shader binaries using VK_KHR_pipeline_executable_properties to multiple binary files (don't forget to enable extension and set VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR)
@DOEC_END */
// nvprints stats to LOGLEVEL_STATS stream
void nvprintPipelineStats(VkDevice device, VkPipeline pipeline, const char* name, bool verbose = false);
// writes stats into single file
void dumpPipelineStats(VkDevice device, VkPipeline pipeline, const char* fileName);
// creates multiple files, one for each pipe executable and representation.
// The baseFilename will get appended along the lines of ".some details.bin"
void dumpPipelineInternals(VkDevice device, VkPipeline pipeline, const char* baseFileName);

//--------------------------------------------------------------------------------------------------
### struct nvvk::GraphicsPipelineState


Most graphic pipelines have similar states, therefore the helper `GraphicsPipelineStage` holds all the elements and
initialize the structures with the proper default values, such as the primitive type, `PipelineColorBlendAttachmentState`
with their mask, `DynamicState` for viewport and scissor, adjust depth test if enabled, line width to 1 pixel, for
example.

Example of usage :
```cpp
nvvk::GraphicsPipelineState pipelineState();
pipelineState.depthStencilState.setDepthTestEnable(true);
pipelineState.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
pipelineState.addBindingDescription({0, sizeof(Vertex)});
pipelineState.addAttributeDescriptions ({
{0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
{1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nrm))},
{2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, col))}});
```
### struct nvvk::GraphicsPipelineGenerator


The graphics pipeline generator takes a GraphicsPipelineState object and pipeline-specific information such as
the render pass and pipeline layout to generate the final pipeline.

Example of usage :
```cpp
nvvk::GraphicsPipelineState pipelineState();
...
nvvk::GraphicsPipelineGenerator pipelineGenerator(m_device, m_pipelineLayout, m_renderPass, pipelineState);
pipelineGenerator.addShader(readFile("spv/vert_shader.vert.spv"), VkShaderStageFlagBits::eVertex);
pipelineGenerator.addShader(readFile("spv/frag_shader.frag.spv"), VkShaderStageFlagBits::eFragment);

m_pipeline = pipelineGenerator.createPipeline();
```
### class nvvk::GraphicsPipelineGeneratorCombined


In some cases the application may have each state associated to a single pipeline. For convenience,
nvvk::GraphicsPipelineGeneratorCombined combines both the state and generator into a single object.

Example of usage :
```cpp
nvvk::GraphicsPipelineGeneratorCombined pipelineGenerator(m_device, m_pipelineLayout, m_renderPass);
pipelineGenerator.depthStencilState.setDepthTestEnable(true);
pipelineGenerator.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
pipelineGenerator.addBindingDescription({0, sizeof(Vertex)});
pipelineGenerator.addAttributeDescriptions ({
{0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
{1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nrm))},
{2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, col))}});

pipelineGenerator.addShader(readFile("spv/vert_shader.vert.spv"), VkShaderStageFlagBits::eVertex);
pipelineGenerator.addShader(readFile("spv/frag_shader.frag.spv"), VkShaderStageFlagBits::eFragment);

m_pipeline = pipelineGenerator.createPipeline();
```
### struct nvvk::GraphicShaderObjectPipeline


This is a helper to set the dynamic graphics pipeline state for shader object
- Set the pipeline state as you would do for a regular pipeline
- Call cmdSetPipelineState to set the pipeline state in the command buffer

Example of usage :
```cpp
// Member of the class
nvvk::GraphicShaderObjectPipeline m_shaderObjPipeline;


// Creation of the dynamic graphic pipeline
m_shaderObjPipeline.rasterizationState.cullMode = VK_CULL_MODE_NONE;
m_shaderObjPipeline.addBindingDescriptions({{0, sizeof(nvh::PrimitiveVertex)}});
m_shaderObjPipeline.addAttributeDescriptions({
{0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(nvh::PrimitiveVertex, p))},  // Position
{1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(nvh::PrimitiveVertex, n))},  // Normal
});
m_shaderObjPipeline.update();

// In the drawing
m_shaderObjPipeline.setViewportScissor(m_app->getViewportSize());
m_shaderObjPipeline.cmdSetPipelineState(cmd);

```

struct GraphicShaderObjectPipeline : GraphicsPipelineState
{
VkSampleMask                                       sampleMask{~0U};
std::vector<VkVertexInputBindingDescription2EXT>   vertexBindingDescriptions2;
std::vector<VkColorBlendEquationEXT>               colorBlendEquationState;
std::vector<VkBool32>                              colorBlendEnables;
std::vector<VkBool32>                              colorWriteMasks;
std::vector<VkVertexInputAttributeDescription2EXT> vertexAttributeDescriptions2;

GraphicShaderObjectPipeline()
{
viewports.resize(1);  // There should be at least one viewport
scissors.resize(1);   //
}

// Set the viewport and scissor to the full extent
void setViewportScissor(const VkExtent2D& extent)
{
viewports[0].x        = 0;
viewports[0].y        = 0;
viewports[0].width    = float(extent.width);
viewports[0].height   = float(extent.height);
viewports[0].minDepth = 0;
viewports[0].maxDepth = 1;

scissors[0].offset = {0, 0};
scissors[0].extent = extent;
}

// Update the internal state
void update()
{
GraphicsPipelineState::update();
multisampleState.pSampleMask = &sampleMask;

vertexBindingDescriptions2.resize(vertexInputState.vertexBindingDescriptionCount);
for(uint32_t i = 0; i < vertexInputState.vertexBindingDescriptionCount; i++)
{
vertexBindingDescriptions2[i].sType     = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
vertexBindingDescriptions2[i].binding   = vertexInputState.pVertexBindingDescriptions[i].binding;
vertexBindingDescriptions2[i].inputRate = vertexInputState.pVertexBindingDescriptions[i].inputRate;
vertexBindingDescriptions2[i].stride    = vertexInputState.pVertexBindingDescriptions[i].stride;
vertexBindingDescriptions2[i].divisor   = 1;
}

vertexAttributeDescriptions2.resize(vertexInputState.vertexAttributeDescriptionCount);
for(uint32_t i = 0; i < vertexInputState.vertexAttributeDescriptionCount; i++)
{
vertexAttributeDescriptions2[i].sType    = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
vertexAttributeDescriptions2[i].binding  = vertexInputState.pVertexAttributeDescriptions[i].binding;
vertexAttributeDescriptions2[i].format   = vertexInputState.pVertexAttributeDescriptions[i].format;
vertexAttributeDescriptions2[i].location = vertexInputState.pVertexAttributeDescriptions[i].location;
vertexAttributeDescriptions2[i].offset   = vertexInputState.pVertexAttributeDescriptions[i].offset;
}

colorBlendEquationState.resize(colorBlendState.attachmentCount);
colorBlendEnables.resize(colorBlendState.attachmentCount);
colorWriteMasks.resize(colorBlendState.attachmentCount);
for(uint32_t i = 0; i < colorBlendState.attachmentCount; i++)
{
colorBlendEquationState[i].srcColorBlendFactor = colorBlendState.pAttachments[i].srcColorBlendFactor;
colorBlendEquationState[i].dstColorBlendFactor = colorBlendState.pAttachments[i].dstColorBlendFactor;
colorBlendEquationState[i].colorBlendOp        = colorBlendState.pAttachments[i].colorBlendOp;
colorBlendEquationState[i].srcAlphaBlendFactor = colorBlendState.pAttachments[i].srcAlphaBlendFactor;
colorBlendEquationState[i].dstAlphaBlendFactor = colorBlendState.pAttachments[i].dstAlphaBlendFactor;
colorBlendEquationState[i].alphaBlendOp        = colorBlendState.pAttachments[i].alphaBlendOp;
colorBlendEnables[i]                           = colorBlendState.pAttachments[i].blendEnable;
colorWriteMasks[i]                             = colorBlendState.pAttachments[i].colorWriteMask;
}
}

// Set the pipeline state in the command buffer
void cmdSetPipelineState(VkCommandBuffer cmd)
{
vkCmdSetViewportWithCount(cmd, viewportState.viewportCount, viewportState.pViewports);
vkCmdSetScissorWithCount(cmd, viewportState.scissorCount, viewportState.pScissors);

vkCmdSetLineWidth(cmd, rasterizationState.lineWidth);
vkCmdSetDepthBias(cmd, rasterizationState.depthBiasConstantFactor, rasterizationState.depthBiasClamp,
rasterizationState.depthBiasSlopeFactor);
vkCmdSetCullMode(cmd, rasterizationState.cullMode);
vkCmdSetFrontFace(cmd, rasterizationState.frontFace);
vkCmdSetDepthBiasEnable(cmd, rasterizationState.depthBiasEnable);
vkCmdSetRasterizerDiscardEnable(cmd, rasterizationState.rasterizerDiscardEnable);
vkCmdSetDepthClampEnableEXT(cmd, rasterizationState.depthClampEnable);
vkCmdSetPolygonModeEXT(cmd, rasterizationState.polygonMode);

vkCmdSetBlendConstants(cmd, colorBlendState.blendConstants);

vkCmdSetDepthBounds(cmd, depthStencilState.minDepthBounds, depthStencilState.maxDepthBounds);
vkCmdSetDepthBoundsTestEnable(cmd, depthStencilState.depthBoundsTestEnable);
vkCmdSetDepthCompareOp(cmd, depthStencilState.depthCompareOp);
vkCmdSetDepthTestEnable(cmd, depthStencilState.depthTestEnable);
vkCmdSetDepthWriteEnable(cmd, depthStencilState.depthWriteEnable);
vkCmdSetStencilTestEnable(cmd, depthStencilState.stencilTestEnable);

vkCmdSetPrimitiveRestartEnable(cmd, inputAssemblyState.primitiveRestartEnable);
vkCmdSetPrimitiveTopology(cmd, inputAssemblyState.topology);

vkCmdSetRasterizationSamplesEXT(cmd, multisampleState.rasterizationSamples);
vkCmdSetSampleMaskEXT(cmd, multisampleState.rasterizationSamples, multisampleState.pSampleMask);
vkCmdSetAlphaToCoverageEnableEXT(cmd, multisampleState.alphaToCoverageEnable);
vkCmdSetAlphaToOneEnableEXT(cmd, multisampleState.alphaToOneEnable);

vkCmdSetVertexInputEXT(cmd, vertexInputState.vertexBindingDescriptionCount, vertexBindingDescriptions2.data(),
vertexInputState.vertexAttributeDescriptionCount, vertexAttributeDescriptions2.data());

vkCmdSetColorBlendEquationEXT(cmd, 0, colorBlendState.attachmentCount, colorBlendEquationState.data());
vkCmdSetColorBlendEnableEXT(cmd, 0, colorBlendState.attachmentCount, colorBlendEnables.data());
vkCmdSetColorWriteMaskEXT(cmd, 0, colorBlendState.attachmentCount, colorWriteMasks.data());
vkCmdSetLogicOpEnableEXT(cmd, colorBlendState.logicOpEnable);
}
};


}  // namespace nvvk

## profiler_vk.hpp
### class nvvk::ProfilerVK


nvvk::ProfilerVK derives from nvh::Profiler and uses vkCmdWriteTimestamp
to measure the gpu time within a section.

If profiler.setLabelUsage(true) was used then it will make use
of vkCmdDebugMarkerBeginEXT and vkCmdDebugMarkerEndEXT for each
section so that it shows up in tools like NsightGraphics and renderdoc.

Currently the commandbuffers must support vkCmdResetQueryPool as well.

When multiple queues are used there could be problems with the "nesting"
of sections. In that case multiple profilers, one per queue, are most
likely better.


Example:

```cpp
nvvk::ProfilerVK profiler;
std::string     profilerStats;

profiler.init(device, physicalDevice, queueFamilyIndex);
profiler.setLabelUsage(true); // depends on VK_EXT_debug_utils

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

## raypicker_vk.hpp
### class nvvk::RayPickerKHR


nvvk::RayPickerKHR is a utility to get hit information under a screen coordinate.

The information returned is:
- origin and direction in world space
- hitT, the distance of the hit along the ray direction
- primitiveID, instanceID and instanceCustomIndex
- the barycentric coordinates in the triangle

Setting up:
- call setup() once with the Vulkan device, and allocator
- call setTlas with the TLAS previously build

Getting results, for example, on mouse down:
- fill the PickInfo structure
- call run()
- call getResult() to get all the information above


Example to set the camera interest point
```cpp
RayPickerKHR::PickResult pr = m_picker.getResult();
if(pr.instanceID != ~0) // Hit something
{
glm::vec3 worldPos = pr.worldRayOrigin + pr.worldRayDirection * pr.hitT;
glm::vec3 eye, center, up;
CameraManip.getLookat(eye, center, up);
CameraManip.setLookat(eye, worldPos, up, false); // Nice with CameraManip.updateAnim();
}
```

## raytraceKHR_vk.hpp

### class nvvk::RaytracingBuilderKHR


>  nvvk::RaytracingBuilderKHR is a base functionality of raytracing

This class acts as an owning container for a single top-level acceleration
structure referencing any number of bottom-level acceleration structures.
We provide functions for building (on the device) an array of BLASs and a
single TLAS from vectors of BlasInput and Instance, respectively, and
a destroy function for cleaning up the created acceleration structures.

Generally, we reference BLASs by their index in the stored BLAS array,
rather than using raw device pointers as the pure Vulkan acceleration
structure API uses.

This class does not support replacing acceleration structures once
built, but you can update the acceleration structures. For educational
purposes, this class prioritizes (relative) understandability over
performance, so vkQueueWaitIdle is implicitly used everywhere.

### Setup and Usage

```cpp
// Borrow a VkDevice and memory allocator pointer (must remain
// valid throughout our use of the ray trace builder), and
// instantiate an unspecified queue of the given family for use.
m_rtBuilder.setup(device, memoryAllocator, queueIndex);

// You create a vector of RayTracingBuilderKHR::BlasInput then
// pass it to buildBlas.
std::vector<RayTracingBuilderKHR::BlasInput> inputs = // ...
m_rtBuilder.buildBlas(inputs);

// You create a vector of RaytracingBuilder::Instance and pass to
// buildTlas. The blasId member of each instance must be below
// inputs.size() (above).
std::vector<VkAccelerationStructureInstanceKHR> instances = // ...
m_rtBuilder.buildTlas(instances);

// Retrieve the handle to the acceleration structure.
const VkAccelerationStructureKHR tlas = m.rtBuilder.getAccelerationStructure()
```

## raytraceNV_vk.hpp

### class nvvk::RaytracingBuilderNV


>  nvvk::RaytracingBuilderNV is a base functionality of raytracing

This class does not implement all what you need to do raytracing, but
helps creating the BLAS and TLAS, which then can be used by different
raytracing usage.

### Setup and Usage

```cpp
m_rtBuilder.setup(device, memoryAllocator, queueIndex);
// Create array of VkGeometryNV
m_rtBuilder.buildBlas(allBlas);
// Create array of RaytracingBuilder::instance
m_rtBuilder.buildTlas(instances);
// Retrieve the acceleration structure
const VkAccelerationStructureNV& tlas = m.rtBuilder.getAccelerationStructure()
```

## renderpasses_vk.hpp
### functions in nvvk


- findSupportedFormat : returns supported VkFormat from a list of candidates (returns first match)
- findDepthFormat : returns supported depth format (24, 32, 16-bit)
- findDepthStencilFormat : returns supported depth-stencil format (24/8, 32/8, 16/8-bit)
- createRenderPass : wrapper for vkCreateRenderPass


## resourceallocator_vk.hpp
### class nvvk::ResourceAllocator


The goal of nvvk::ResourceAllocator is to aid creation of typical Vulkan
resources (VkBuffer, VkImage and VkAccelerationStructure).
All memory is allocated using the provided [nvvk::MemAllocator](#class-nvvkmemallocator)
and bound to the appropriate resources. The allocator contains a
[nvvk::StagingMemoryManager](#class-nvvkstagingmemorymanager) and
[nvvk::SamplerPool](#class-nvvksamplerpool) to aid this process.

ResourceAllocator separates object creation and memory allocation by delegating allocation
of memory to an object of interface type 'nvvk::MemAllocator'.
This way the ResourceAllocator can be used with different memory allocation strategies, depending on needs.
nvvk provides three implementations of MemAllocator:
* nvvk::DedicatedMemoryAllocator is using a very simple allocation scheme, one VkDeviceMemory object per allocation.
This strategy is only useful for very simple applications due to the overhead of vkAllocateMemory and
an implementation dependent bounded number of vkDeviceMemory allocations possible.
* nvvk::DMAMemoryAllocator delegates memory requests to a 'nvvk:DeviceMemoryAllocator',
as an example implemention of a suballocator
* nvvk::VMAMemoryAllocator delegates memory requests to a [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)

Utility wrapper structs contain the appropriate Vulkan resource and the
appropriate nvvk::MemHandle :

- nvvk::Buffer
- nvvk::Image
- nvvk::Texture  contains VkImage and VkImageView as well as an
optional VkSampler stored witin VkDescriptorImageInfo
- nvvk::AccelNV
- nvvk::AccelKHR

nvvk::Buffer, nvvk::Image, nvvk::Texture and nvvk::AccelKHR nvvk::AccelNV objects can be copied
by value. They do not track lifetime of the underlying Vulkan objects and memory allocations.
The corresponding destroy() functions of nvvk::ResourceAllocator destroy created objects and
free up their memory. ResourceAllocator does not track usage of objects either. Thus, one has to
make sure that objects are no longer in use by the GPU when they get destroyed.

> Note: These classes are foremost to showcase principle components that
> a Vulkan engine would most likely have.
> They are geared towards ease of use in this sample framework, and
> not optimized nor meant for production code.

```cpp
nvvk::DeviceMemoryAllocator memAllocator;
nvvk::ResourceAllocator     resAllocator;

memAllocator.init(device, physicalDevice);
resAllocator.init(device, physicalDevice, &memAllocator);

...

VkCommandBuffer cmd = ... transfer queue command buffer

// creates new resources and
// implicitly triggers staging transfer copy operations into cmd
nvvk::Buffer vbo = resAllocator.createBuffer(cmd, vboSize, vboData, vboUsage);
nvvk::Buffer ibo = resAllocator.createBuffer(cmd, iboSize, iboData, iboUsage);

// use functions from staging memory manager
// here we associate the temporary staging resources with a fence
resAllocator.finalizeStaging( fence );

// submit cmd buffer with staging copy operations
vkQueueSubmit(... cmd ... fence ...)

...

// if you do async uploads you would
// trigger garbage collection somewhere per frame
resAllocator.releaseStaging();

```

Separation of memory allocation and resource creation is very flexible, but it
can be tedious to set up for simple usecases. nvvk offers three helper ResourceAllocator
derived classes which internally contain the MemAllocator object and manage its lifetime:
* [ResourceAllocatorDedicated](#class nvvk::ResourceAllocatorDedicated)
* [ResourceAllocatorDma](#class nvvk::ResourceAllocatorDma)
* [ResourceAllocatorVma](#cass nvvk::ResourceAllocatorVma)

In these cases, only one object needs to be created and initialized.

ResourceAllocator can also be subclassed to specialize some of its functionality.
Examples are [ExportResourceAllocator](#class ExportResourceAllocator) and [ExplicitDeviceMaskResourceAllocator](#class ExplicitDeviceMaskResourceAllocator).
ExportResourceAllocator injects itself into the object allocation process such that
the resulting allocations can be exported or created objects may be bound to exported
memory
ExplicitDeviceMaskResourceAllocator overrides the devicemask of allocations such that
objects can be created on a specific device in a device group.
### class nvvk::ResourceAllocatorDma

nvvk::ResourceAllocatorDMA is a convencience class owning a nvvk::DMAMemoryAllocator and nvvk::DeviceMemoryAllocator object
### class nvvk::ResourceAllocatorDedicated

>  nvvk::ResourceAllocatorDedicated is a convencience class automatically creating and owning a DedicatedMemoryAllocator object
#class nvvk::ExportResourceAllocator

ExportResourceAllocator specializes the object allocation process such that resulting memory allocations are
exportable and buffers and images can be bound to external memory.
### class nvvk::ExportResourceAllocatorDedicated

nvvk::ExportResourceAllocatorDedicated is a resource allocator that is using DedicatedMemoryAllocator to allocate memory
and at the same time it'll make all allocations exportable.
### class nvvk::ExplicitDeviceMaskResourceAllocator

nvvk::ExplicitDeviceMaskResourceAllocator is a resource allocator that will inject a specific devicemask into each
allocation, making the created allocations and objects available to only the devices in the mask.

## samplers_vk.hpp
### class nvvk::SamplerPool


This nvvk::SamplerPool class manages unique VkSampler objects. To minimize the total
number of sampler objects, this class ensures that identical configurations
return the same sampler

Example :
```cpp
nvvk::SamplerPool pool(device);

for (auto it : textures) {
VkSamplerCreateInfo info = {...};

// acquire ensures we create the minimal subset of samplers
it.sampler = pool.acquireSampler(info);
}

// you can manage releases individually, or just use deinit/destructor of pool
for (auto it : textures) {
pool.releaseSampler(it.sampler);
}
```

- makeSamplerCreateInfo : aids for sampler creation


## sbtwrapper_vk.hpp

### class nvvk::SBTWrapper


nvvk::SBTWrapper is a generic SBT builder from the ray tracing pipeline

The builder will iterate through the pipeline create info `VkRayTracingPipelineCreateInfoKHR`
to find the number of raygen, miss, hit and callable shader groups were created.
The handles for those group will be retrieved from the pipeline and written in the right order in
separated buffer.

Convenient functions exist to retrieve all information to be used in TraceRayKHR.

## Usage
- Setup the builder (`setup()`)
- After the pipeline creation, call `create()` with the same info used for the creation of the pipeline.
- Use `getRegions()` to get all the vk::StridedDeviceAddressRegionKHR needed by TraceRayKHR()


### Example
```cpp
m_sbtWrapper.setup(m_device, m_graphicsQueueIndex, &m_alloc, m_rtProperties);
// ...
m_sbtWrapper.create(m_rtPipeline, rayPipelineInfo);
// ...
auto& regions = m_stbWrapper.getRegions();
vkCmdTraceRaysKHR(cmdBuf, &regions[0], &regions[1], &regions[2], &regions[3], size.width, size.height, 1);
```


## Extra

If data are attached to a shader group (see shaderRecord), it need to be provided independently.
In this case, the user must know the group index for the group type.

Here the Hit group 1 and 2 has data, but not the group 0.
Those functions must be called before create.

```cpp
m_sbtWrapper.addData(SBTWrapper::eHit, 1, m_hitShaderRecord[0]);
m_sbtWrapper.addData(SBTWrapper::eHit, 2, m_hitShaderRecord[1]);
```


## Special case

It is also possible to create a pipeline with only a few groups but having a SBT representing many more groups.

The following example shows a more complex setup.
There are: 1 x raygen, 2 x miss, 2 x hit.
BUT the SBT will have 3 hit by duplicating the second hit in its table.
So, the same hit shader defined in the pipeline, can be called with different data.

In this case, the use must provide manually the information to the SBT.
All extra group must be explicitly added.

The following show how to get handle indices provided in the pipeline, and we are adding another hit group, re-using the 4th pipeline entry.
Note: we are not providing the pipelineCreateInfo, because we are manually defining it.

```cpp
// Manually defining group indices
m_sbtWrapper.addIndices(rayPipelineInfo); // Add raygen(0), miss(1), miss(2), hit(3), hit(4) from the pipeline info
m_sbtWrapper.addIndex(SBTWrapper::eHit, 4);  // Adding a 3rd hit, duplicate from the hit:1, which make hit:2 available.
m_sbtWrapper.addHitData(SBTWrapper::eHit, 2, m_hitShaderRecord[1]); // Adding data to this hit shader
m_sbtWrapper.create(m_rtPipeline);
```


## shadermodulemanager_vk.hpp
### class nvvk::ShaderModuleManager


The nvvk::ShaderModuleManager manages VkShaderModules stored in files (SPIR-V or GLSL)

Using ShaderFileManager it will find the files and resolve #include for GLSL.
You must add include directories to the base-class for this.

It also comes with some convenience functions to reload shaders etc.
That is why we pass out the ShaderModuleID rather than a VkShaderModule directly.

To change the compilation behavior manipulate the public member variables
prior createShaderModule.

m_filetype is crucial for this. You can pass raw spir-v files or GLSL.
If GLSL is used, shaderc must be used as well (which must be added via
_add_package_ShaderC() in CMake of the project)

Example:

```cpp
ShaderModuleManager mgr(myDevice);

// derived from ShaderFileManager
mgr.addDirectory("spv/");

// all shaders get this injected after #version statement
mgr.m_prepend = "#define USE_NOISE 1\n";

vid = mgr.createShaderModule(VK_SHADER_STAGE_VERTEX_BIT,   "object.vert.glsl");
fid = mgr.createShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "object.frag.glsl");

// ... later use module
info.module = mgr.get(vid);
```

## shaders_vk.hpp
### functions in nvvk


- createShaderModule : create the shader module from various binary code inputs
- createShaderStageInfo: create the shader module and setup the stage from the incoming binary code

## sparse_image_vk.hpp
### struct nvvk::SparseImagePage

>  Virtual texture page as a part of the partially resident texture. Contains memory bindings, offsets and status information.

The virtual texture page is a part of the sparse texture, and contains information about its memory binding, location in the texture, and status information.
### struct nvvk::SparseImage

>  Sparse image object, containing the virtual texture pages and memory bindings.

The sparse image object contains the virtual texture pages and memory bindings, as well as the memory requirements for the mip tail and the image granularity.

## specialization.hpp
### class nvvk::Specialization

>  Helper to generate specialization info

Examples:
```cpp
nvvk::Specialization specialization;
specialization.add(0, 5); // Adding value 5 to constant_id=0
VkPipelineShaderStageCreateInfo info;
...
info.pSpecializationInfo = specialization.getSpecialization();
createPipeline();
```

Note: this is adding information in a vector, therefor add all values
before calling getSpecialization(). Construct the pipeline before
specialization get out of scope, or pointer getting invalidated
by adding new values or clearing the vector of data.

## stagingmemorymanager_vk.hpp
### class nvvk::StagingMemoryManager


nvvk::StagingMemoryManager class is a utility that manages host visible
buffers and their allocations in an opaque fashion to assist
asynchronous transfers between device and host.
The memory for this is allocated using the provided
[nvvk::MemAllocator](#class-nvvkmemallocator).

The collection of the transfer resources is represented by nvvk::StagingID.

The necessary buffer space is sub-allocated and recycled by using one
[nvvk::BufferSubAllocator](#class-nvvkbuffersuballocator) per transfer direction (to or from device).

> **WARNING:**
> - cannot manage a copy > 4 GB

Usage:
- Enqueue transfers into your VkCommandBuffer and then finalize the copy operations.
- Associate the copy operations with a VkFence or retrieve a SetID
- The release of the resources allows to safely recycle the buffer space for future transfers.

> We use fences as a way to garbage collect here, however a more robust solution
> may be implementing some sort of ticketing/timeline system.
> If a fence is recycled, then this class may not be aware that the fence represents a different
> submission, likewise if the fence is deleted elsewhere problems can occur.
> You may want to use the manual "SetID" system in that case.

Example :

```cpp
StagingMemoryManager  staging;
staging.init(memAllocator);


// Enqueue copy operations of data to target buffer.
// This internally manages the required staging resources
staging.cmdToBuffer(cmd, targetBufer, 0, targetSize, targetData);

// you can also get access to a temporary mapped pointer and fill
// the staging buffer directly
vertices = staging.cmdToBufferT<Vertex>(cmd, targetBufer, 0, targetSize);

// OPTION A:
// associate all previous copy operations with a fence (or not)
staging.finalizeResources( fence );
..
// every once in a while call
staging.releaseResources();
// this will release all those without fence, or those
// who had a fence that completed (but never manual SetIDs, see next).

// OPTION B
// alternatively manage the resource release yourself.
// The SetID represents the staging resources
// since any last finalize.
sid = staging.finalizeResourceSet();

...
// You need to ensure these transfers and their staging
// data access completed yourself prior releasing the set.
//
// This is particularly useful for managing downloads from
// device. The "from" functions return a pointer  where the
// data will be copied to. You want to use this pointer
// after the device-side transfer completed, and then
// release its resources once you are done using it.

staging.releaseResourceSet(sid);

```

## structs_vk.hpp
## `nvvk::make`

Contains templated `nvvk::make<T>` function that is
auto-generated by `structs.lua`. The function provide default
structs for the Vulkan C api by initializing the `VkStructureType sType`
field (also for nested structs) and clearing the rest to zero.

```cpp
auto compCreateInfo = nvvk::make<VkComputePipelineCreateInfo>;
```

## `nvvk::clear`

Contains templated `nvvk::clear<T>` function
auto-generated by `structs.lua`.

## swapchain_vk.hpp
### class nvvk::SwapChain


>  nvvk::SwapChain is a helper to handle swapchain setup and use

In Vulkan, we have to use `VkSwapchainKHR` to request a swap chain
(front and back buffers) from the operating system and manually
synchronize our and OS's access to the images within the swap chain.
This helper abstracts that process.

For each swap chain image there is an ImageView, and one read and write
semaphore synchronizing it (see `SwapChainAcquireState`).

To start, you need to call `init`, then `update` with the window's
initial framebuffer size (for example, use `glfwGetFramebufferSize`).
Then, in your render loop, you need to call `acquire()` to get the
swap chain image to draw to, draw your frame (waiting and signalling
the appropriate semaphores), and call `present()`.

Sometimes, the swap chain needs to be re-created (usually due to
window resizes). `nvvk::SwapChain` detects this automatically and
re-creates the swap chain for you. Every new swap chain is assigned a
unique ID (`getChangeID()`), allowing you to detect swap chain
re-creations. This usually triggers a `VkDeviceWaitIdle`; however, if
this is not appropriate, see `setWaitQueue()`.

Finally, there is a utility function to setup the image transitions
from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
which is the format an image must be in before it is presented.

Example in combination with nvvk::Context :

* get the window handle
* create its related surface
* make sure the Queue is the one we need to render in this surface

```cpp
// could {.cpp}be arguments of a function/method :
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
```

The initialization can happen now :

```cpp
m_swapChain.init(ctx.m_device, ctx.m_physicalDevice, ctx.m_queueGCT, ctx.m_queueGCT.familyIndex,
m_surface, VK_FORMAT_B8G8R8A8_UNORM);
...
// after init or update you also have to setup the image layouts at some point
VkCommandBuffer cmd = ...
m_swapChain.cmdUpdateBarriers(cmd);
```

During a resizing of a window, you can update the swapchain as well :

```cpp
bool WindowSurface::resize(int w, int h)
{
...
m_swapChain.update(w, h);
// be cautious to also transition the image layouts
...
}
```


A typical renderloop would look as follows:

```cpp
// handles vkAcquireNextImageKHR and setting the active image
// w,h only needed if update(w,h) not called reliably.
int w, h;
bool recreated;
glfwGetFramebufferSize(window, &w, &h);
if(!m_swapChain.acquire(w, h, &recreated, [, optional SwapChainAcquireState ptr]))
{
... handle acquire error (shouldn't happen)
}

VkCommandBuffer cmd = ...

// acquire might have recreated the swap chain: respond if needed here.
// NOTE: you can also check the recreated variable above, but this
// only works if the swap chain was recreated this frame.
if (m_swapChain.getChangeID() != lastChangeID){
// after init or resize you have to setup the image layouts
m_swapChain.cmdUpdateBarriers(cmd);

lastChangeID = m_swapChain.getChangeID();
}

// do render operations either directly using the imageview
VkImageView swapImageView = m_swapChain.getActiveImageView();

// or you may always render offline int your own framebuffer
// and then simply blit into the backbuffer. NOTE: use
// m_swapChain.getWidth() / getHeight() to get blit dimensions,
// actual swap chain image size may differ from requested width/height.
VkImage swapImage = m_swapChain.getActiveImage();
vkCmdBlitImage(cmd, ... swapImage ...);

// setup submit
VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers    = &cmd;

// we need to ensure to wait for the swapchain image to have been read already
// so we can safely blit into it

VkSemaphore swapchainReadSemaphore      = m_swapChain->getActiveReadSemaphore();
VkPipelineStageFlags swapchainReadFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
submitInfo.waitSemaphoreCount = 1;
submitInfo.pWaitSemaphores    = &swapchainReadSemaphore;
submitInfo.pWaitDstStageMask  = &swapchainReadFlags);

// once this submit completed, it means we have written the swapchain image
VkSemaphore swapchainWrittenSemaphore = m_swapChain->getActiveWrittenSemaphore();
submitInfo.signalSemaphoreCount = 1;
submitInfo.pSignalSemaphores    = &swapchainWrittenSemaphore;

// submit it
vkQueueSubmit(m_queue, 1, &submitInfo, fence);

// present via a queue that supports it
// this will also setup the dependency for the appropriate written semaphore
// and bump the semaphore cycle
m_swapChain.present(m_queue);
```


## vulkanhppsupport.hpp
### class nvvk::GraphicsPipelineGeneratorCombined


In some cases the application may have each state associated to a single pipeline. For convenience,
nvvk::GraphicsPipelineGeneratorCombined combines both the state and generator into a single object.

Example of usage :
```cpp
nvvk::GraphicsPipelineGeneratorCombined pipelineGenerator(m_device, m_pipelineLayout, m_renderPass);
pipelineGenerator.depthStencilState.setDepthTestEnable(true);
pipelineGenerator.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
pipelineGenerator.addBindingDescription({0, sizeof(Vertex)});
pipelineGenerator.addAttributeDescriptions ({
{0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
{1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nrm))},
{2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, col))}});

pipelineGenerator.addShader(readFile("spv/vert_shader.vert.spv"), VkShaderStageFlagBits::eVertex);
pipelineGenerator.addShader(readFile("spv/frag_shader.frag.spv"), VkShaderStageFlagBits::eFragment);

m_pipeline = pipelineGenerator.createPipeline();
```
### class nvvk::ResourceAllocatorDma

nvvk::ResourceAllocatorDMA is a convencience class owning a nvvk::DMAMemoryAllocator and nvvk::DeviceMemoryAllocator object
### class nvvk::ResourceAllocatorDedicated

>  nvvk::ResourceAllocatorDedicated is a convencience class automatically creating and owning a DedicatedMemoryAllocator object
### class nvvk::ExportResourceAllocator


ExportResourceAllocator specializes the object allocation process such that resulting memory allocations are
exportable and buffers and images can be bound to external memory.
### class nvvk::ExportResourceAllocatorDedicated

nvvk::ExportResourceAllocatorDedicated is a resource allocator that is using DedicatedMemoryAllocator to allocate memory
and at the same time it'll make all allocations exportable.
### class nvvk::ExplicitDeviceMaskResourceAllocator

nvvk::ExplicitDeviceMaskResourceAllocator is a resource allocator that will inject a specific devicemask into each
allocation, making the created allocations and objects available to only the devices in the mask.

## vulkanhppsupport_vkgl.hpp
### class nvvkpp::ResourceAllocatorGLInterop


>  ResourceAllocatorGLInterop is a helper class to manage Vulkan and OpenGL memory allocation and interop.

This class is a wrapper around the `nvvk::DeviceMemoryAllocatorGL` and `nvvk::DeviceMemoryAllocator` classes, which are used to allocate memory for Vulkan and OpenGL resources.

