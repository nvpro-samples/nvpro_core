## Table of Contents
- [appwindowcamerainertia.hpp](#appwindowcamerainertiahpp)
- [appwindowprofiler.hpp](#appwindowprofilerhpp)
- [bitarray.hpp](#bitarrayhpp)
- [boundingbox.hpp](#boundingboxhpp)
- [cameracontrol.hpp](#cameracontrolhpp)
- [camerainertia.hpp](#camerainertiahpp)
- [cameramanipulator.hpp](#cameramanipulatorhpp)
- [commandlineparser.hpp](#commandlineparserhpp)
- [fileoperations.hpp](#fileoperationshpp)
- [geometry.hpp](#geometryhpp)
- [gltfscene.hpp](#gltfscenehpp)
- [inputparser.h](#inputparserh)
- [misc.hpp](#mischpp)
- [nvml_monitor.hpp](#nvml_monitorhpp)
- [nvprint.hpp](#nvprinthpp)
- [parallel_work.hpp](#parallel_workhpp)
- [parametertools.hpp](#parametertoolshpp)
- [primitives.hpp](#primitiveshpp)
- [profiler.hpp](#profilerhpp)
- [radixsort.hpp](#radixsorthpp)
- [shaderfilemanager.hpp](#shaderfilemanagerhpp)
- [threading.hpp](#threadinghpp)
- [timesampler.hpp](#timesamplerhpp)
- [trangeallocator.hpp](#trangeallocatorhpp)

## appwindowcamerainertia.hpp
### class AppWindowCameraInertia

>  AppWindowCameraInertia is a Window base for samples, adding a camera with inertia

It derives the Window for this sample

## appwindowprofiler.hpp
### class nvh::AppWindowProfiler

nvh::AppWindowProfiler provides an alternative utility wrapper class around NVPWindow.
It is useful to derive single-window applications from and is used by some
but not all nvpro-samples.

Further functionality is provided :
- built-in profiler/timer reporting to console
- command-line argument parsing as well as config file parsing using the ParameterTools
see AppWindowProfiler::setupParameters() for built-in commands
- benchmark/automation mode using ParameterTools
- screenshot creation
- logfile based on devicename (depends on context)
- optional context/swapchain interface
the derived classes nvvk/appwindowprofiler_vk and nvgl/appwindowprofiler_gl make use of this

## bitarray.hpp
### class nvh::BitArray


> The nvh::BitArray class implements a tightly packed boolean array using single bits stored in uint64_t values.
Whenever you want large boolean arrays this representation is preferred for cache-efficiency.
The Visitor and OffsetVisitor traversal mechanisms make use of cpu intrinsics to speed up iteration over bits.

Example:
```cpp
BitArray modifiedObjects(1024);

// set some bits
modifiedObjects.setBit(24,true);
modifiedObjects.setBit(37,true);

// iterate over all set bits using the built-in traversal mechanism

struct MyVisitor {
void operator()( size_t index ){
// called with the index of a set bit
myObjects[index].update();
}
};

MyVisitor visitor;
modifiedObjects.traverseBits(visitor);
```

## boundingbox.hpp

```nvh::Bbox``` is a class to create bounding boxes.
It grows by adding 3d vector, can combine other bound boxes.
And it returns information, like its volume, its center, the min, max, etc..


## cameracontrol.hpp
### class nvh::CameraControl


> nvh::CameraControl is a utility class to create a viewmatrix based on mouse inputs.

It can operate in perspective or orthographic mode (`m_sceneOrtho==true`).

perspective:
- LMB: rotate
- RMB or WHEEL: zoom via dolly movement
- MMB: pan/move within camera plane

ortho:
- LMB: pan/move within camera plane
- RMB or WHEEL: zoom via dolly movement, application needs to use `m_sceneOrthoZoom` for projection matrix adjustment
- MMB: rotate

The camera can be orbiting (`m_useOrbit==true`) around `m_sceneOrbit` or
otherwise provide "first person/fly through"-like controls.

Speed of movement/rotation etc. is influenced by `m_sceneDimension` as well as the
sensitivity values.

## camerainertia.hpp
### struct InertiaCamera

>  Struct that offers a camera moving with some inertia effect around a target point

InertiaCamera exposes a mix of pseudo polar rotation around a target point and
some other movements to translate the target point, zoom in and out.

Either the keyboard or mouse can be used for all of the moves.

## cameramanipulator.hpp
### class nvh::CameraManipulator


nvh::CameraManipulator is a camera manipulator help class
It allow to simply do
- Orbit        (LMB)
- Pan          (LMB + CTRL  | MMB)
- Dolly        (LMB + SHIFT | RMB)
- Look Around  (LMB + ALT   | LMB + CTRL + SHIFT)

In a various ways:
- examiner(orbit around object)
- walk (look up or down but stays on a plane)
- fly ( go toward the interest point)

Do use the camera manipulator, you need to do the following
- Call setWindowSize() at creation of the application and when the window size change
- Call setLookat() at creation to initialize the camera look position
- Call setMousePosition() on application mouse down
- Call mouseMove() on application mouse move

Retrieve the camera matrix by calling getMatrix()

See: appbase_vkpp.hpp

Note: There is a singleton `CameraManip` which can be use across the entire application

```cpp
// Retrieve/set camera information
CameraManip.getLookat(eye, center, up);
CameraManip.setLookat(eye, center, glm::vec3(m_upVector == 0, m_upVector == 1, m_upVector == 2));
CameraManip.getFov();
CameraManip.setSpeed(navSpeed);
CameraManip.setMode(navMode == 0 ? nvh::CameraManipulator::Examine : nvh::CameraManipulator::Fly);
// On mouse down, keep mouse coordinates
CameraManip.setMousePosition(x, y);
// On mouse move and mouse button down
if(m_inputs.lmb || m_inputs.rmb || m_inputs.mmb)
{
CameraManip.mouseMove(x, y, m_inputs);
}
// Wheel changes the FOV
CameraManip.wheel(delta > 0 ? 1 : -1, m_inputs);
// Retrieve the matrix to push to the shader
m_ubo.view = CameraManip.getMatrix();
````


## commandlineparser.hpp
Command line parser.
```cpp
std::string inFilename = "";
bool printHelp = false;
CommandLineParser args("Test Parser");
args.addArgument({"-f", "--filename"}, &inFilename, "Input filename");
args.addArgument({"-h", "--help"}, &printHelp, "Print Help");
bool result = args.parse(argc, argv);
```

## fileoperations.hpp
### functions in nvh


- nvh::fileExists : check if file exists
- nvh::findFile : finds filename in provided search directories
- nvh::loadFile : (multiple overloads) loads file as std::string, binary or text, can also search in provided directories
- nvh::getFileName : splits filename from filename with path
- nvh::getFilePath : splits filepath from filename with path

## geometry.hpp
### namespace nvh::geometry

The geometry namespace provides a few procedural mesh primitives
that are subdivided.

nvh::geometry::Mesh template uses the provided TVertex which must have a
constructor from nvh::geometry::Vertex. You can also use nvh::geometry::Vertex
directly.

It provides triangle indices, as well as outline line indices. The outline indices
are typical feature lines (rectangle for plane, some circles for sphere/torus).

All basic primitives are within -1,1 ranges along the axis they use

- nvh::geometry::Plane (x,y subdivision)
- nvh::geometry::Box (x,y,z subdivision, made of 6 planes)
- nvh::geometry::Sphere (lat,long subdivision)
- nvh::geometry::Torus (inner, outer circle subdivision)
- nvh::geometry::RandomMengerSponge (subdivision, tree depth, probability)

Example:

```cpp
// single primitive
nvh::geometry::Box<nvh::geometry::Vertex> box(4,4,4);

// construct from primitives

```

## gltfscene.hpp
### `nvh::GltfScene`


These utilities are for loading glTF models in a
canonical scene representation. From this representation
you would create the appropriate 3D API resources (buffers
and textures).

```cpp
// Typical Usage
// Load the GLTF Scene using TinyGLTF

tinygltf::Model    gltfModel;
tinygltf::TinyGLTF gltfContext;
fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warn, m_filename);

// Fill the data in the gltfScene
gltfScene.getMaterials(tmodel);
gltfScene.getDrawableNodes(tmodel, GltfAttributes::Normal | GltfAttributes::Texcoord_0);

// Todo in App:
//   create buffers for vertices and indices, from gltfScene.m_position, gltfScene.m_index
//   create textures from images: using tinygltf directly
//   create descriptorSet for material using directly gltfScene.m_materials
```


## inputparser.h
### class InputParser

> InputParser is a Simple command line parser

Example of usage for: test.exe -f name.txt -size 200 100

Parsing the command line: mandatory '-f' for the filename of the scene

```cpp
nvh::InputParser parser(argc, argv);
std::string filename = parser.getString("-f");
if(filename.empty())  filename = "default.txt";
if(parser.exist("-size") {
auto values = parser.getInt2("-size");
```

## misc.hpp
### functions in nvh


- mipMapLevels : compute number of mip maps
- stringFormat : sprintf for std::string
- frand : random float using rand()
- permutation : fills uint vector with random permutation of values [0... vec.size-1]

## nvml_monitor.hpp

Capture the GPU load and memory for all GPUs on the system.

Usage:
- There should be only one instance of NvmlMonitor
- call refresh() in each frame. It will not pull more measurement that the interval(ms)
- isValid() : return if it can be used
- nbGpu()   : return the number of GPU in the computer
- getGpuInfo()     : static info about the GPU
- getDeviceMemory() : memory consumption info
- getDeviceUtilization() : GPU and memory utilization
- getDevicePerformanceState() : clock speeds and throttle reasons
- getDevicePowerState() : power, temperature and fan speed

Measurements:
- Uses a cycle buffer.
- Offset is the last measurement


## nvprint.hpp
Multiple functions and macros that should be used for logging purposes,
rather than printf. These can print to multiple places at once
### Function nvprintf etc


Configuration:
- nvprintSetLevel : sets default loglevel
- nvprintGetLevel : gets default loglevel
- nvprintSetLogFileName : sets log filename
- nvprintSetLogging : sets file logging state
- nvprintSetCallback : sets custom callback

Printf-style functions and macros.
These take printf-style specifiers.
- nvprintf : prints at default loglevel
- nvprintfLevel : nvprintfLevel print at a certain loglevel
- LOGI : macro that does nvprintfLevel(LOGLEVEL_INFO)
- LOGW : macro that does nvprintfLevel(LOGLEVEL_WARNING)
- LOGE : macro that does nvprintfLevel(LOGLEVEL_ERROR)
- LOGE_FILELINE : macro that does nvprintfLevel(LOGLEVEL_ERROR) combined with filename/line
- LOGD : macro that does nvprintfLevel(LOGLEVEL_DEBUG) (only in debug builds)
- LOGOK : macro that does nvprintfLevel(LOGLEVEL_OK)
- LOGSTATS : macro that does nvprintfLevel(LOGLEVEL_STATS)

std::print-style functions and macros.
These take std::format-style specifiers
(https://en.cppreference.com/w/cpp/utility/format/formatter#Standard_format_specification).
- nvprintLevel : print at a certain loglevel
- PRINTI : macro that does nvprintLevel(LOGLEVEL_INFO)
- PRINTW : macro that does nvprintLevel(LOGLEVEL_WARNING)
- PRINTE : macro that does nvprintLevel(LOGLEVEL_ERROR)
- PRINTE_FILELINE : macro that does nvprintLevel(LOGLEVEL_ERROR) combined with filename/line
- PRINTD : macro that does nvprintLevel(LOGLEVEL_DEBUG) (only in debug builds)
- PRINTOK : macro that does nvprintLevel(LOGLEVEL_OK)
- PRINTSTATS : macro that does nvprintLevel(LOGLEVEL_STATS)

Safety:
On error, all functions print an error message.
All functions are thread-safe.
Printf-style functions have annotations that should produce warnings at
compile-time or when performing static analysis. Their format strings may be
dynamic - but this can be bad if an adversary can choose the content of the
format string.
std::print-style functions are safer: they produce compile-time errors, and
their format strings must be compile-time constants. Dynamic formatting
should be performed outside of printing, like this:
```cpp
ImGui::InputText("Enter a format string: ", userFormat, sizeof(userFormat));
try
{
std::string formatted = fmt::vformat(userFormat, ...);
}
catch (const std::exception& e)
{
(error handling...)
}
PRINTI("{}", formatted);
```

Text encoding:
Printing to the Windows debug console is the only operation that assumes a
text encoding, which is ANSI. In all other cases, strings are copied into
the output.

## parallel_work.hpp
Distributes batches of loops over BATCHSIZE items across multiple threads. numItems reflects the total number
of items to process.

batches: fn (uint64_t itemIndex, uint32_t threadIndex)
callback does single item
ranges:  fn (uint64_t itemBegin, uint64_t itemEnd, uint32_t threadIndex)
callback does loop `for (uint64_t itemIndex = itemBegin; itemIndex < itemEnd; itemIndex++)`


## parametertools.hpp
### class nvh::ParameterList


The nvh::ParameterList helps parsing commandline arguments
or commandline arguments stored within ascii config files.

Parameters always update the values they point to, and optionally
can trigger a callback that can be provided per-parameter.

```cpp
ParameterList list;
std::string   modelFilename;
float         modelScale;

list.addFilename(".gltf|model filename", &modelFilename);
list.add("scale|model scale", &modelScale);

list.applyTokens(3, {"blah.gltf","-scale","4"}, "-", "/assets/");
```

Use in combination with the ParameterSequence class to iterate
sequences of parameter changes for benchmarking/automation.
### class nvh::ParameterSequence


The nvh::ParameterSequence processes provided tokens in sequences.
The sequences are terminated by a special "separator" token.
All tokens between the last iteration and the separator are applied
to the provided ParameterList.
Useful to process commands in sequences (automation, benchmarking etc.).

Example:

```cpp
ParameterSequence sequence;
ParameterList     list;
int               mode;
list.add("mode", &mode);

std::vector<const char*> tokens;
ParameterList::tokenizeString("benchmark simple -mode 10 benchmark complex -mode 20", tokens);
sequence.init(&list, tokens);

// 1 means our separator is followed by one argument (simple/complex)
// "-" as parameters in the string are prefixed with -

while(!sequence.advanceIteration("benchmark", 1, "-")) {
printf("%d %s mode %d\n", sequence.getIteration(), sequence.getSeparatorArg(0), mode);
}

// would print:
//   0 simple mode 10
//   1 complex mode 20
```

## primitives.hpp
### struct `nvh::PrimitiveMesh`

- Common primitive type, made of vertices: position, normal and texture coordinates.
- All primitives are triangles, and each 3 indices is forming a triangle.

### struct `nvh::Node`

- Structure to hold a reference to a mesh, with a material and transformation.

Primitives that can be created:
* Tetrahedron
* Icosahedron
* Octahedron
* Plane
* Cube
* SphereUv
* Cone
* SphereMesh
* Torus

Node creator: returns the instance and the position
* MengerSponge
* SunFlower

Other utilities
* mergeNodes
* removeDuplicateVertices
* wobblePrimitive


## profiler.hpp
### class nvh::Profiler


> The nvh::Profiler class is designed to measure timed sections.

Each section has a cpu and gpu time. Gpu times are typically provided
by derived classes for each individual api (e.g. OpenGL, Vulkan etc.).

There is functionality to pretty print the sections with their nesting level.
Multiple profilers can reference the same database, so one profiler
can serve as master that they others contribute to. Typically the
base class measuring only CPU time could be the master, and the api
derived classes reference it to share the same database.

Profiler::Clock can be used standalone for time measuring.

## radixsort.hpp
### function nvh::radixsort


The radixsort function sorts the provided keys based on
BYTES many bytes stored inside TKey starting at BYTEOFFSET.
The sorting result is returned as indices into the keys array.

For example:

```cpp
struct MyData {
uint32_t objectIdentifier;
uint16_t objectSortKey;
};


// 4-byte offset of objectSortKey within MyData
// 2-byte size of sorting key

result = radixsort<4,2>(keys, indicesIn, indicesTemp);

// after sorting the following is true

keys[result[i]].objectSortKey < keys[result[i + 1]].objectSortKey

// result can point either to indicesIn or indicesTemp (we swap the arrays
// after each byte iteration)
```

## shaderfilemanager.hpp
### class nvh::ShaderFileManager


The nvh::ShaderFileManager class is meant to be derived from to create the actual api-specific
shader/program managers.

The ShaderFileManager provides a system to find/load shader files.
It also allows resolving #include instructions in HLSL/GLSL source files.
Such includes can be registered before pointing to strings in memory.

If m_handleIncludePasting is true, then `#include`s are replaced by
the include file contents (recursively) before presenting the
loaded shader source code to the caller. Otherwise, the include file
loader is still available but `#include`s are left unchanged.

Furthermore it handles injecting prepended strings (typically used
for #defines) after the #version statement of GLSL files,
regardless of m_handleIncludePasting's value.


## threading.hpp
### class nvh::delayed_call 

Class returned by delay_noreturn_for to track the thread created and possibly reset the
delay timer.
Delay a call to a void function for sleep_duration.

`return`: A delayed_call object that holds the running thread.

Example:
```cpp
// Create or update a delayed call to callback. Useful to consolidate multiple events into one call.
if(!m_delayedCall.delay_for(delay))
m_delayedCall = nvh::delay_noreturn_for(delay, callback);
```

## timesampler.hpp
### struct TimeSampler

TimeSampler does time sampling work
### struct nvh::Stopwatch

> Timer in milliseconds.

Starts the timer at creation and the elapsed time is retrieved by calling `elapsed()`.
The timer can be reset if it needs to start timing later in the code execution.

Usage:
````cpp
{
nvh::Stopwatch sw;
... work ...
LOGI("Elapsed: %f ms\n", sw.elapsed()); // --> Elapsed: 128.157 ms
}
````

## trangeallocator.hpp
### class nvh::TRangeAllocator


The nvh::TRangeAllocator<GRANULARITY> template allows to sub-allocate ranges from a fixed
maximum size. Ranges are allocated at GRANULARITY and are merged back on freeing.
Its primary use is within allocators that sub-allocate from fixed-size blocks.

The implementation is based on [MakeID by Emil Persson](http://www.humus.name/3D/MakeID.h).

Example :

```cpp
TRangeAllocator<256> range;

// initialize to a certain range
range.init(range.alignedSize(128 * 1024 * 1024));

...

// allocate a sub range
// example
uint32_t size = vertexBufferSize;
uint32_t alignment = vertexAlignment;

uint32_t allocOffset;
uint32_t allocSize;
uint32_t alignedOffset;

if (range.subAllocate(size, alignment, allocOffset, alignedOffset, allocSize)) {
... use the allocation space
// [alignedOffset + size] is guaranteed to be within [allocOffset + allocSize]
}

// give back the memory range for re-use
range.subFree(allocOffset, allocSize);

...

// at the end cleanup
range.deinit();
```
