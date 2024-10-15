## Table of Contents
- [khr_df.h](#khr_dfh)
- [nv_dds.h](#nv_ddsh)
- [nv_ktx.h](#nv_ktxh)
- [texture_formats.h](#texture_formatsh)
- [tinygltf_utils.hpp](#tinygltf_utilshpp)
- [tiny_converter.hpp](#tiny_converterhpp)

## khr_df.h

> The Khronos Data Format header, from https://registry.khronos.org/DataFormat/.

This header defines a structure that can describe the layout of image
formats in memory. This means that the data format is transparent to
the application, and the expectation is that this should be used when
the layout is defined external to the API. Many Khronos APIs deliberately
keep the internal layout of images opaque, to allow proprietary layouts
and optimisations. This structure is not appropriate for describing
opaque layouts.


## nv_dds.h
### nv_dds 2.1.0

> A small yet complete library for reading and writing DDS files.

Other than the C++ standard library, nv_dds only requires five files:
dxgiformat.h, nv_dds.h, nv_dds.cpp, texture_formats.h, and texture_formats.cpp.

To load a DDS file, use `Image::readFromFile()`:

```cpp
nv_dds::Image image;
nv_dds::ErrorWithText maybeError = image.readFromFile("data/image.dds", {});
if(maybeError.has_value())
{
  // Do something with the error message, maybeError.value()
}
else
{
  // Access subresources using image.subresource(...), and upload them
  // to the GPU using your graphics API of choice.
}
```

`Image`'s format field is a DXGI format. If you need to use this data with
another API, you can use the functions in texture_formats.h to look up
the corresponding API format.

To write a DDS file, use `Image::writeToFile()`:

```cpp
nv_dds::ErrorWithText maybe_error = image.writeToFile("output.dds", {});
if(maybe_error.has_value())
{
  LOGE("Failed to write to output.dds! The DDS writer reported: %s\n",
       maybe_error.value().c_str());
}
```

`Image` also provides functions to read and write streams. Each of these
read and write functions supports various settings; see `ReadSettings`
and `WriteSettings`.

Images can also be created from raw data:
```cpp
nv_dds::Image image;
image.allocate(1,  // _numMips
               1,  // _numLayers
               1); // _numFaces
image.subresource(0, 0, 0) // mip, layer, face
  .create(512, 512, 1, subresource_data); // width, height, depth, data
  = nv_dds::Subresource(512, 512, 1, subresource_data); // width, height, depth, data
```

#### Limitations

Not currently supported:
* Multi-plane YUV textures with chroma subsampling
  (e.g. `DXGI_FORMAT_R8G8_B8G8_UNORM`)
* Paletted textures
* DirectDraw Surface versions before DX9

#### Changes from nv_dds 1.0

nv_dds adds support for many more formats, fixes many issues, and now aims
to be secure against untrusted input, so it's worth updating. However, the API
has almost entirely changed, and now looks much more like nv_ktx's API:

* `CSurface` is now `nv_dds::Subresource`
* `CDDSImage` is now `nv_dds::Image`
* `CTexture` (representing a mip chain) has been moved to part of `Image`.
* Images can no longer be flipped. It turns out the code for this only worked
for mips whose height was a multiple of the block size; in all other cases,
flipping a block-compressed image across the y axis requires decompressing
and recompressing it, which is outside the scope of nv_dds. (We recommend
flipping your sampling y axis instead; if flipping compressed image data is
absolutely necessary, consider using a library such as NVTT.)


## nv_ktx.h

> A mostly self-contained reader and writer for KTX2 files and reader for KTX1
> files.

Relies on Vulkan (for KTX2), GL (for KTX1), and the Khronos Data Format.

Sample usage for reading files:

```cpp
KTXImage     image;
ErrorWithText maybe_error = image.readFromFile("data/image.ktx2", {});
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

Define `NVP_SUPPORTS_ZSTD`, `NVP_SUPPORTS_GZLIB`, and `NVP_SUPPORTS_BASISU` to
include the Zstd, Zlib, and Basis Universal headers respectively, and to
enable reading these formats. This will also enable writing Zstd and
Basis Universal-compressed formats.
If you're using this inside the nvpro-samples framework, you can add all
three quickly by adding `_add_package_KTX()` to your dependencies
in CMakeLists.txt.


## texture_formats.h

Provides:
* Methods for translating texture format names between DirectX, Vulkan, and
OpenGL.
* A method for getting the size of a Vulkan subresource using linear tiling.
* The extended ASTC values for DXGI_FORMAT.


## tinygltf_utils.hpp
### namespace `tinygltf::utils`
> Utility functions for extracting structs from tinygltf's representation of glTF.
#### Function `getValue<T>`
> Gets the value of type T for the attribute `name`.

This function retrieves the value of the specified attribute from a tinygltf::Value
and stores it in the provided result variable.

Parameters:
- value: The `tinygltf::Value` from which to retrieve the attribute.
- name: The name of the attribute to retrieve.
- result: The variable to store the retrieved value in.
#### Function `getValue(..., float& result)`
> Specialization of `getValue()` for float type.

Retrieves the value of the specified attribute as a float and stores it in the result variable.
#### Function `getValue(..., tinygltf::TextureInfo& result)`
> Specialization of `getValue()` for `nvvkhl::Gltf::Texture` type.

Retrieves the texture attribute values and stores them in the result variable.
#### Function `setValue<T>`
> Sets attribute `key` to value `val`.
#### Function `setValue(... tinygltf::TextureInfo)`
> Sets attribute `key` to a JSON object with an `index` and `texCoord` set from
> the members of `textureInfo`.
#### Function `getArrayValue<T>`
> Gets the value of type T for the attribute `name`.

This function retrieves the array value of the specified attribute from a `tinygltf::Value`
and stores it in the provided result variable. It is used for types such as `glm::vec3`, `glm::vec4`, `glm::mat4`, etc.

Parameters:
- value: The `tinygltf::Value` from which to retrieve the attribute.
- name: The name of the attribute to retrieve.
- result: The variable to store the retrieved array value in.
#### Function `setArrayValue<T>`
> Sets attribute `name` of the given `value` to an array with the first
> `numElements` elements from the `array` pointer.
#### Function `convertToTinygltfValue`
> Converts a vector of elements to a `tinygltf::Value`.

This function converts a given array of float elements into a `tinygltf::Value::Array`,
suitable for use within the tinygltf library.

Parameters:
- numElements: The number of elements in the array.
- elements: A pointer to the array of float elements.

Returns:
- A `tinygltf::Value` representing the array of elements.
#### Function `getNodeTRS`
> Retrieves the translation, rotation, and scale of a GLTF node.

This function extracts the translation, rotation, and scale (TRS) properties
from the given GLTF node. If the node has a matrix defined, it decomposes
the matrix to obtain these properties. Otherwise, it directly retrieves
the TRS values from the node's properties.

Parameters:
- node: The GLTF node from which to extract the TRS properties.
- translation: Output parameter for the translation vector.
- rotation: Output parameter for the rotation quaternion.
- scale: Output parameter for the scale vector.
* ## Function `setNodeTRS`
> Sets the translation, rotation, and scale of a GLTF node.

This function sets the translation, rotation, and scale (TRS) properties of
the given GLTF node using the provided values.

Parameters:
- node: The GLTF node to modify.
- translation: The translation vector to set.
- rotation: The rotation quaternion to set.
- scale: The scale vector to set.
* ## Function `getNodeMatrix`
> Retrieves the transformation matrix of a GLTF node.

This function computes the transformation matrix for the given GLTF node.
If the node has a direct matrix defined, it returns that matrix as defined in
the specification. Otherwise, it computes the matrix from the node's translation,
rotation, and scale (TRS) properties.

Parameters:
- node: The GLTF node for which to retrieve the transformation matrix.

Returns:
- The transformation matrix of the node.
#### Function `generatePrimitiveKey`
> Generates a unique key for a GLTF primitive based on its attributes.

This function creates a unique string key for the given GLTF primitive by
concatenating its attribute keys and values. This is useful for caching
the primitive data, thereby avoiding redundancy.

Parameters:
- primitive: The GLTF primitive for which to generate the key.

Returns:
- A unique string key representing the primitive's attributes.
#### Function `traverseSceneGraph`
> Traverses the scene graph and calls the provided functions for each element.

This utility function recursively traverses the scene graph starting from the
specified node ID. It calls the provided functions for cameras, lights, and
meshes when encountered. The traversal can be stopped early if any function
returns `true`.

Parameters:
- model: The GLTF model containing the scene graph.
- nodeID: The ID of the node to start traversal from.
- parentMat: The transformation matrix of the parent node.
- fctCam: Function to call when a camera is encountered. Can be `nullptr`.
- fctLight: Function to call when a light is encountered. Can be `nullptr`.
- fctMesh: Function to call when a mesh is encountered. Can be `nullptr`.
#### Function `getVertexCount`
> Returns the number of vertices in a primitive.

This function retrieves the number of vertices for the given GLTF primitive
by accessing the "POSITION" attribute in the model's accessors.

Parameters:
- model: The GLTF model containing the primitive data.
- primitive: The GLTF primitive for which to retrieve the vertex count.

Returns:
- The number of vertices in the primitive.
#### Function `getIndexCount`
> Returns the number of indices in a primitive.

This function retrieves the number of indices for the given GLTF primitive
by accessing the indices in the model's accessors. If no indices are present,
it returns the number of vertices instead.

Parameters:
- model: The GLTF model containing the primitive data.
- primitive: The GLTF primitive for which to retrieve the index count.

Returns:
- The number of indices in the primitive, or the number of vertices if no indices are present.
#### Function `hasElementName<MapType>`
> Check if the map has the specified element.

Can be used for extensions, extras, or any other map.
Returns `true` if the map has the specified element, `false` otherwise.
#### Function `getElementValue<MapType>`
> Get the value of the specified element from the map.

Can be `extensions`, `extras`, or any other map.
Returns the value of the element.
#### Function `getBufferDataSpan<T>`
> Retrieves the buffer data for the specified accessor from the GLTF model
> and returns it as a span of type `T`.

The function assumes that the buffer data is of type `T`.
It also performs assertions to ensure that the accessor and buffer data are compatible; these will be ignored
if assertions are off.

Example usage:

```cpp
int accessorIndex = primitive.attributes.at("POSITION");
std::span<const glm::vec3> positions = tinygltf::utils::getBufferDataSpan<glm::vec3>(model, accessorIndex);
```
#### Function `forEachSparseValue<T>`
> Calls a function (such as a lambda function) for each `(index, value)` pair in
> a sparse accessor.

It's only potentially called for indices from
`accessorFirstElement` through `accessorFirstElement + numElementsToProcess - 1`.
#### Function `copyAccessorData<T>`
> Copies accessor elements `accessorFirstElement` through
> `accessorFirstElement + numElementsToCopy - 1` to `outData` elements
> `outFirstElement` through `outFirstElement + numElementsToCopy - 1`.

This handles sparse accessors correctly! It's intended as a replacement for
what would be `memcpy(..., &buffer.data[...], ...)` calls.

However, it performs no conversion: it assumes (but does not check) that
accessor's elements are of type `T`. For instance, `T` should be a struct of two
floats for a `VEC2` float accessor.

This is range-checked, so elements that would be out-of-bounds are not
copied. We assume `size_t` overflow does not occur.

Note that `outDataSizeInT` is the number of elements in the `outDataBuffer`,
while `numElementsToCopy` is the number of elements to copy, not the number
of elements in `accessor`.
#### Function `copyAccessorData<T>(std::vector<T>& outData, ...)`
> Same as `copyAccessorData(T*, ...)`, but taking a vector.
#### Function `getAccessorData<T>`
> Appends all the values of `accessor` to `attribVec`.

Returns `false` if the accessor is invalid.
`T` must be `glm::vec2`, `glm::vec3`, or `glm::vec4`.
#### Function `getAttribute<T>`
> Appends all the values of `attribName` to `attribVec`.

Returns `false` if the attribute is missing or invalid.
`T` must be `glm::vec2`, `glm::vec3`, or `glm::vec4`.
#### Function `appendData<T>`
> Appends data from `inData` to the binary buffer `buffer` and returns the number
> of bytes of data added.

`T` should be a type like `std::vector`.
#### Function `getTextureImageIndex`
> Retrieves the image index of a texture, accounting for extensions such as
> `MSFT_texture_dds` and `KHR_texture_basisu`.
> Retrieves the visibility of a node using `KHR_node_visibility`.

Does not search up the node hierarchy, so e.g. if node A points to node B and
node A is set to invisible and node B is set to visible, then
`getNodeVisibility(B)` will return `KHR_node_visibility{true}` even though
node B would not be visible due to node A.

## tiny_converter.hpp

Class TinyConverter

> This class is used to convert a tinyobj::ObjReader to a tinygltf::Model.

