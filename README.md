# nvpro_core

> [!IMPORTANT]
> **This repository has been deprecated** and is superseded by [nvpro_core2](https://github.com/nvpro-samples/nvpro_core2),
> which provides an improved and consolidated version of this framework.
>
> If you plan to keep using nvpro_core, please be aware that this repository is 
> no longer actively maintained and will eventually be archived, including samples
> from nvpro-samples that use it.

## Repository
This folder is a repository of *shared source code* : most other samples use it as library or directly reference code from it.

It means that you must clone this repository (with submodules) prior to trying those samples that refer to this repository as a dependency.

## Folders
* **cmake**: 'find' files to search for cmake packages
* **doxygen**: folder for doxygen doc generation
* **fileformats**: various helpers to load some files
* **nvh**: API agnostic helpers to simplify the code of samples.
* **nvgl**: helpers for OpenGL
* **nvvk**: helpers for Vulkan (to aid extension use and simplify initialization of Vulkan structs, some files were generated from scripting)
* **nvvkhl**: helpers for Vulkan High-Level for creating samples; applications, loading scenes, HDR-environment, G-Buffers, etc.
* **nvvkhl/shaders**: commonly use functions for shading, tonemaper, etc.
* **nvoptix**: helpers for Optix
* **nvdx12**: helpers for DirectX 12
* **nvmath**: math library used by most samples
* **imgui**: a version of [imgui](https://github.com/ocornut/imgui) and some implementations to render it (derived from its examples).
* **GL/KHR**: include files for OpenGL (we use a subset of extensions that is generated and defined by `nvgl/extensions_gl.lua`)
* resources: icons etc.

## Files
* **nvpsystem\[_linux|win32\].hpp/cpp**: defines the base class NVPSystem
* **nvpwindow.hpp/cpp**: defines the main class NVPWindow
* **docgen.py**: python script that generates README.md files from comments gathered in various headers
* **CMakeLists.txt**: project definition for the nvpro_core library
* **resources...**: win32 resources
* **include_gl.h**: shortcut for including the file that contains the GL extension definitions
* **platform.h / NvFoundation.h**: platform specific `NV_...` macros that aid cross platform coding

## Dependencies
Some samples may rely on few additional libraries in order to compile (mostly for win32). You can find them in [third_party/binaries](https://github.com/nvpro-samples/third_party_binaries). The use of Vulkan or DirectX APIs requires that the appropriate SDKs are installed.

The minimum Vulkan SDK version is currently: 1.3.261.0

## License
nvpro_core is licensed under the [Apache License 2.0](LICENSE).

## Third-Party Libraries
This project embeds or includes (as submodules) several open-source libraries
and/or code derived from them. All such libraries' licenses are included in the
[PACKAGE-LICENSES](PACKAGE-LICENSES) folder.

The include file mechanism inside `nvh/shaderfilemanager.cpp` is derived from
the [OpenGL Samples Pack](https://github.com/g-truc/ogl-samples). Only the hash
combine logic was derived from Boost (https://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html).
