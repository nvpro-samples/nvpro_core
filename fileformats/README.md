## Table of Contents
- [khr_df.h](#khr_dfh)
- [nv_ktx.h](#nv_ktxh)
- [tiny_converter.hpp](#tiny_converterhpp)

## khr_df.h

This header defines a structure that can describe the layout of image
formats in memory. This means that the data format is transparent to
the application, and the expectation is that this should be used when
the layout is defined external to the API. Many Khronos APIs deliberately
keep the internal layout of images opaque, to allow proprietary layouts
and optimizations. This structure is not appropriate for describing
opaque layouts.


## nv_ktx.h

A mostly self-contained reader and writer for KTX2 files and reader for KTX1
files. Relies on Vulkan (for KTX2), GL (for KTX1), and the
Khronos Data Format.

Sample usage for reading files:

```cpp
KTXImage image;
ErrorWithText maybe_error = image.readFromFile("data/image.ktx2");
if(maybe_error.has_value())
{
// Do something with the error message, maybe_error.value()
}
else
{
// Access subresources using image.subresource(...), and upload them
// to the GPU using your graphics API of choice.
}
```

Define NVP_SUPPORTS_ZSTD, NVP_SUPPORTS_GZLIB, and NVP_SUPPORTS_BASISU to
include the Zstd, Zlib, and Basis Universal headers respectively, and to
enable reading these formats. This will also enable writing Zstd and
Basis Universal-compressed formats.
If you're using this inside the nvpro-samples framework, you can add all
three quickly by adding _add_package_KTX() to your dependencies
in CMakeLists.txt.


## tiny_converter.hpp

Class TinyConverter

> This class is used to convert a tinyobj::ObjReader to a tinygltf::Model.

