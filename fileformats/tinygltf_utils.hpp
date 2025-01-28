/*
 * Copyright (c) 2024-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2024-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/* @DOC_START
# namespace `tinygltf::utils`
> Utility functions for extracting structs from tinygltf's representation of glTF.
@DOC_END */

#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#define KHR_MATERIALS_VARIANTS_EXTENSION_NAME "KHR_materials_variants"
#define EXT_MESH_GPU_INSTANCING_EXTENSION_NAME "EXT_mesh_gpu_instancing"
#define EXTENSION_ATTRIB_IRAY "NV_attributes_iray"
#define MSFT_TEXTURE_DDS_NAME "MSFT_texture_dds"

// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_specular/README.md
#define KHR_MATERIALS_SPECULAR_EXTENSION_NAME "KHR_materials_specular"
struct KHR_materials_specular
{
  float                 specularFactor       = 1.0f;
  tinygltf::TextureInfo specularTexture      = {};
  glm::vec3             specularColorFactor  = {1.0f, 1.0f, 1.0f};
  tinygltf::TextureInfo specularColorTexture = {};
};

// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_texture_transform
#define KHR_TEXTURE_TRANSFORM_EXTENSION_NAME "KHR_texture_transform"
struct KHR_texture_transform
{
  glm::vec2 offset      = {0.0f, 0.0f};
  float     rotation    = 0.0f;
  glm::vec2 scale       = {1.0f, 1.0f};
  int       texCoord    = 0;
  glm::mat3 uvTransform = glm::mat3(1);  // Computed transform of offset, rotation, scale
  void      updateTransform()
  {
    // Compute combined transformation matrix
    float cosR  = cos(rotation);
    float sinR  = sin(rotation);
    float tx    = offset.x;
    float ty    = offset.y;
    float sx    = scale.x;
    float sy    = scale.y;
    uvTransform = glm::mat3(sx * cosR, sx * sinR, tx, -sy * sinR, sy * cosR, ty, 0, 0, 1);
  }
};

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_clearcoat/README.md
#define KHR_MATERIALS_CLEARCOAT_EXTENSION_NAME "KHR_materials_clearcoat"
struct KHR_materials_clearcoat
{
  float                 factor           = 0.0f;
  tinygltf::TextureInfo texture          = {};
  float                 roughnessFactor  = 0.0f;
  tinygltf::TextureInfo roughnessTexture = {};
  tinygltf::TextureInfo normalTexture    = {};
};

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_sheen/README.md
#define KHR_MATERIALS_SHEEN_EXTENSION_NAME "KHR_materials_sheen"
struct KHR_materials_sheen
{
  glm::vec3             sheenColorFactor      = {0.0f, 0.0f, 0.0f};
  tinygltf::TextureInfo sheenColorTexture     = {};
  float                 sheenRoughnessFactor  = 0.0f;
  tinygltf::TextureInfo sheenRoughnessTexture = {};
};

// https://github.com/DassaultSystemes-Technology/glTF/tree/KHR_materials_volume/extensions/2.0/Khronos/KHR_materials_transmission
#define KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME "KHR_materials_transmission"
struct KHR_materials_transmission
{
  float                 factor  = 0.0f;
  tinygltf::TextureInfo texture = {};
};

// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_unlit
#define KHR_MATERIALS_UNLIT_EXTENSION_NAME "KHR_materials_unlit"
struct KHR_materials_unlit
{
  int active = 0;
};

// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_anisotropy/README.md
#define KHR_MATERIALS_ANISOTROPY_EXTENSION_NAME "KHR_materials_anisotropy"
struct KHR_materials_anisotropy
{
  float                 anisotropyStrength = 0.0f;
  float                 anisotropyRotation = 0.0f;
  tinygltf::TextureInfo anisotropyTexture  = {};
};


// https://github.com/DassaultSystemes-Technology/glTF/tree/KHR_materials_ior/extensions/2.0/Khronos/KHR_materials_ior
#define KHR_MATERIALS_IOR_EXTENSION_NAME "KHR_materials_ior"
struct KHR_materials_ior
{
  float ior = 1.5f;
};

// https://github.com/DassaultSystemes-Technology/glTF/tree/KHR_materials_volume/extensions/2.0/Khronos/KHR_materials_volume
#define KHR_MATERIALS_VOLUME_EXTENSION_NAME "KHR_materials_volume"
struct KHR_materials_volume
{
  float                 thicknessFactor     = 0;
  tinygltf::TextureInfo thicknessTexture    = {};
  float                 attenuationDistance = std::numeric_limits<float>::max();
  glm::vec3             attenuationColor    = {1.0f, 1.0f, 1.0f};
};


// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_texture_basisu/README.md
#define KHR_TEXTURE_BASISU_EXTENSION_NAME "KHR_texture_basisu"
struct KHR_texture_basisu
{
  tinygltf::TextureInfo source;
};

// https://github.com/KhronosGroup/glTF/issues/948
#define KHR_MATERIALS_DISPLACEMENT_EXTENSION_NAME "KHR_materials_displacement"
struct KHR_materials_displacement
{
  float                 displacementGeometryFactor  = 1.0f;
  float                 displacementGeometryOffset  = 0.0f;
  tinygltf::TextureInfo displacementGeometryTexture = {};
};


// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_emissive_strength/README.md
#define KHR_MATERIALS_EMISSIVE_STRENGTH_EXTENSION_NAME "KHR_materials_emissive_strength"
struct KHR_materials_emissive_strength
{
  float emissiveStrength = 1.0;
};

// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_iridescence/README.md
#define KHR_MATERIALS_IRIDESCENCE_EXTENSION_NAME "KHR_materials_iridescence"
struct KHR_materials_iridescence
{
  float                 iridescenceFactor           = 0.0f;
  tinygltf::TextureInfo iridescenceTexture          = {};
  float                 iridescenceIor              = 1.3f;
  float                 iridescenceThicknessMinimum = 100.f;
  float                 iridescenceThicknessMaximum = 400.f;
  tinygltf::TextureInfo iridescenceThicknessTexture = {};
};

// https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_dispersion
#define KHR_MATERIALS_DISPERSION_EXTENSION_NAME "KHR_materials_dispersion"
struct KHR_materials_dispersion
{
  float dispersion = 0.0f;
};

// https://github.com/KhronosGroup/glTF/pull/2410
#define KHR_NODE_VISIBILITY_EXTENSION_NAME "KHR_node_visibility"
struct KHR_node_visibility
{
  bool visible = true;
};

#define KHR_MATERIALS_PBR_SPECULAR_GLOSSINESS_EXTENSION_NAME "KHR_materials_pbrSpecularGlossiness"
struct KHR_materials_pbrSpecularGlossiness
{
  glm::vec4             diffuseFactor             = glm::vec4(1.0f);
  glm::vec3             specularFactor            = glm::vec3(1.0f);
  float                 glossinessFactor          = 1.0f;
  tinygltf::TextureInfo diffuseTexture            = {};
  tinygltf::TextureInfo specularGlossinessTexture = {};
};


// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_diffuse_transmission
#define KHR_MATERIALS_DIFFUSE_TRANSMISSION_EXTENSION_NAME "KHR_materials_diffuse_transmission"
struct KHR_materials_diffuse_transmission
{
  float                 diffuseTransmissionFactor       = 0.0f;
  tinygltf::TextureInfo diffuseTransmissionTexture      = {};
  glm::vec3             diffuseTransmissionColor        = {1.0f, 1.0f, 1.0f};
  tinygltf::TextureInfo diffuseTransmissionColorTexture = {};
};


namespace tinygltf {

namespace utils {

/* @DOC_START
## Function `getValue<T>`
> Gets the value of type T for the attribute `name`.

This function retrieves the value of the specified attribute from a tinygltf::Value
and stores it in the provided result variable.

Parameters:
- value: The `tinygltf::Value` from which to retrieve the attribute.
- name: The name of the attribute to retrieve.
- result: The variable to store the retrieved value in.
@DOC_END */
template <typename T>
inline void getValue(const tinygltf::Value& value, const std::string& name, T& result)
{
  if(value.Has(name))
  {
    result = value.Get(name).Get<T>();
  }
}

/* @DOC_START
## Function `getValue(..., float& result)`
> Specialization of `getValue()` for float type.

Retrieves the value of the specified attribute as a float and stores it in the result variable.
@DOC_END */
template <>
inline void getValue(const tinygltf::Value& value, const std::string& name, float& result)
{
  if(value.Has(name))
  {
    result = static_cast<float>(value.Get(name).Get<double>());
  }
}

/* @DOC_START
## Function `getValue(..., tinygltf::TextureInfo& result)`
> Specialization of `getValue()` for `nvvkhl::Gltf::Texture` type.

Retrieves the texture attribute values and stores them in the result variable.
@DOC_END */
template <>
inline void getValue(const tinygltf::Value& value, const std::string& name, tinygltf::TextureInfo& result)
{
  if(value.Has(name))
  {
    const auto& t = value.Get(name);
    getValue(t, "index", result.index);
    getValue(t, "texCoord", result.texCoord);
    getValue(t, "extensions", result.extensions);
  }
}


/* @DOC_START
## Function `setValue<T>`
> Sets attribute `key` to value `val`.
@DOC_END */
template <typename T>
inline void setValue(tinygltf::Value& value, const std::string& key, const T& val)
{
  value.Get<tinygltf::Value::Object>()[key] = tinygltf::Value(val);
}


/* @DOC_START
## Function `setValue(... tinygltf::TextureInfo)`
> Sets attribute `key` to a JSON object with an `index` and `texCoord` set from
> the members of `textureInfo`.
@DOC_END */
inline void setValue(tinygltf::Value& value, const std::string& key, const tinygltf::TextureInfo& textureInfo)
{
  auto& t                                      = value.Get<tinygltf::Value::Object>()[key];
  t.Get<tinygltf::Value::Object>()["index"]    = tinygltf::Value(textureInfo.index);
  t.Get<tinygltf::Value::Object>()["texCoord"] = tinygltf::Value(textureInfo.texCoord);
  value.Get<tinygltf::Value::Object>()[key]    = t;
}


/* @DOC_START
## Function `getArrayValue<T>`
> Gets the value of type T for the attribute `name`.

This function retrieves the array value of the specified attribute from a `tinygltf::Value`
and stores it in the provided result variable. It is used for types such as `glm::vec3`, `glm::vec4`, `glm::mat4`, etc.

Parameters:
- value: The `tinygltf::Value` from which to retrieve the attribute.
- name: The name of the attribute to retrieve.
- result: The variable to store the retrieved array value in.
@DOC_END */
template <class T>
inline void getArrayValue(const tinygltf::Value& value, const std::string& name, T& result)
{
  if(value.Has(name))
  {
    const auto& v = value.Get(name).Get<tinygltf::Value::Array>();
    std::transform(v.begin(), v.end(), glm::value_ptr(result),
                   [](const tinygltf::Value& v) { return static_cast<float>(v.Get<double>()); });
  }
}

/* @DOC_START
## Function `setArrayValue<T>`
> Sets attribute `name` of the given `value` to an array with the first
> `numElements` elements from the `array` pointer.
@DOC_END */
template <typename T>
inline void setArrayValue(tinygltf::Value& value, const std::string& name, uint32_t numElem, T* array)
{
  tinygltf::Value::Array arrayValue(numElem);
  for(uint32_t n = 0; n < numElem; n++)
  {
    arrayValue[n] = tinygltf::Value(array[n]);
  }
  value.Get<tinygltf::Value::Object>()[name] = tinygltf::Value(arrayValue);
}


/* @DOC_START
## Function `convertToTinygltfValue`
> Converts a vector of elements to a `tinygltf::Value`.

This function converts a given array of float elements into a `tinygltf::Value::Array`,
suitable for use within the tinygltf library.

Parameters:
- numElements: The number of elements in the array.
- elements: A pointer to the array of float elements.

Returns:
- A `tinygltf::Value` representing the array of elements.
@DOC_END */
tinygltf::Value convertToTinygltfValue(int numElements, const float* elements);


/* @DOC_START
## Function `getNodeTRS`
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
@DOC_END */
void getNodeTRS(const tinygltf::Node& node, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);


/* @DOC_START
* ## Function `setNodeTRS`
> Sets the translation, rotation, and scale of a GLTF node.

This function sets the translation, rotation, and scale (TRS) properties of
the given GLTF node using the provided values.

Parameters:
- node: The GLTF node to modify.
- translation: The translation vector to set.
- rotation: The rotation quaternion to set.
- scale: The scale vector to set.
@DOC_END */
void setNodeTRS(tinygltf::Node& node, const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale);


/* @DOC_START
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
@DOC_END */
glm::mat4 getNodeMatrix(const tinygltf::Node& node);


/* @DOC_START
## Function `generatePrimitiveKey`
> Generates a unique key for a GLTF primitive based on its attributes.

This function creates a unique string key for the given GLTF primitive by
concatenating its attribute keys and values. This is useful for caching
the primitive data, thereby avoiding redundancy.

Parameters:
- primitive: The GLTF primitive for which to generate the key.

Returns:
- A unique string key representing the primitive's attributes.
@DOC_END */
std::string generatePrimitiveKey(const tinygltf::Primitive& primitive);


/* @DOC_START
## Function `traverseSceneGraph`
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
@DOC_END */
void traverseSceneGraph(const tinygltf::Model&                            model,
                        int                                               nodeID,
                        const glm::mat4&                                  parentMat,
                        const std::function<bool(int, const glm::mat4&)>& fctCam   = nullptr,
                        const std::function<bool(int, const glm::mat4&)>& fctLight = nullptr,
                        const std::function<bool(int, const glm::mat4&)>& fctMesh  = nullptr,
                        const std::function<bool(int, const glm::mat4&)>& anyNode  = nullptr);


/* @DOC_START
## Function `getVertexCount`
> Returns the number of vertices in a primitive.

This function retrieves the number of vertices for the given GLTF primitive
by accessing the "POSITION" attribute in the model's accessors.

Parameters:
- model: The GLTF model containing the primitive data.
- primitive: The GLTF primitive for which to retrieve the vertex count.

Returns:
- The number of vertices in the primitive.
@DOC_END */
size_t getVertexCount(const tinygltf::Model& model, const tinygltf::Primitive& primitive);

/* @DOC_START
## Function `getIndexCount`
> Returns the number of indices in a primitive.

This function retrieves the number of indices for the given GLTF primitive
by accessing the indices in the model's accessors. If no indices are present,
it returns the number of vertices instead.

Parameters:
- model: The GLTF model containing the primitive data.
- primitive: The GLTF primitive for which to retrieve the index count.

Returns:
- The number of indices in the primitive, or the number of vertices if no indices are present.
@DOC_END */
size_t getIndexCount(const tinygltf::Model& model, const tinygltf::Primitive& primitive);

/* @DOC_START
## Function `hasElementName<MapType>`
> Check if the map has the specified element.

Can be used for extensions, extras, or any other map.
Returns `true` if the map has the specified element, `false` otherwise.
@DOC_END */
template <typename MapType>
bool hasElementName(const MapType& map, const std::string& key)
{
  return map.find(key) != map.end();
}


/* @DOC_START
## Function `getElementValue<MapType>`
> Get the value of the specified element from the map.

Can be `extensions`, `extras`, or any other map.
Returns the value of the element.
@DOC_END */
template <typename MapType>
const typename MapType::mapped_type& getElementValue(const MapType& map, const std::string& key)
{
  return map.at(key);
}


/* @DOC_START
## Function `getBufferDataSpan<T>`
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
@DOC_END */
template <typename T>
std::span<const T> getBufferDataSpan(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
{
  const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
  assert(view.byteStride == 0 || view.byteStride == sizeof(T));  // Not supporting stride
                                                                 // Add assertions based on the type
  if constexpr(std::is_same<T, glm::vec2>::value)
  {
    assert(accessor.type == TINYGLTF_TYPE_VEC2);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
  }
  else if constexpr(std::is_same<T, glm::vec3>::value)
  {
    assert(accessor.type == TINYGLTF_TYPE_VEC3);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
  }
  else if constexpr(std::is_same<T, glm::vec4>::value)
  {
    assert(accessor.type == TINYGLTF_TYPE_VEC4);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
  }
  else if constexpr(std::is_same<T, glm::mat4>::value)
  {
    assert(accessor.type == TINYGLTF_TYPE_MAT4);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
  }
  else if constexpr(std::is_same<T, uint16_t>::value)
  {
    assert(accessor.type == TINYGLTF_TYPE_SCALAR);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);
  }
  else if constexpr(std::is_same<T, uint32_t>::value)
  {
    assert(accessor.type == TINYGLTF_TYPE_SCALAR);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);
  }
  else if constexpr(std::is_same<T, float>::value)
  {
    assert(accessor.type == TINYGLTF_TYPE_SCALAR);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
  }


  const T* bufferData = reinterpret_cast<const T*>(&model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]);
  return std::span<const T>(bufferData, accessor.count);
}

template <typename T>
std::span<const T> getBufferDataSpan(const tinygltf::Model& model, int accessorIndex)
{
  return getBufferDataSpan<T>(model, model.accessors[accessorIndex]);
}


/* @DOC_START
## Function `extractAttributeData<T>`
> Extracts a vector of type `T` from the attribute.

This function retrieves the data for the specified attribute from the GLTF model
and copies it into a vector of type `T`. Note that this copy may not be the
most efficient way to handle large datasets compared to accessing the data
without a copy.

Example usage:
```cpp
std::vector<glm::vec3> translations
  = tinygltf::utils::extractAttributeData<glm::vec3>(model, attributes, "TRANSLATION");
```

Parameters:
- model: The GLTF model containing the attribute data.
- attributes: The attribute values to parse.
- attributeName: The name of the attribute to retrieve.

Returns:
- A vector of type T containing the attribute data.
*/
template <typename T>
inline std::vector<T> extractAttributeData(const tinygltf::Model& model, const tinygltf::Value& attributes, const std::string& attributeName)
{
  std::vector<T> attributeValues;

  if(attributes.Has(attributeName))
  {
    const tinygltf::Accessor&   accessor   = model.accessors.at(attributes.Get(attributeName).GetNumberAsInt());
    const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
    const tinygltf::Buffer&     buffer     = model.buffers.at(bufferView.buffer);

    attributeValues.resize(accessor.count);
    std::memcpy(attributeValues.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(T));
  }

  return attributeValues;
}


/* @DOC_START
## Function `forEachSparseValue<T>`
> Calls a function (such as a lambda function) for each `(index, value)` pair in
> a sparse accessor.

It's only potentially called for indices from
`accessorFirstElement` through `accessorFirstElement + numElementsToProcess - 1`.
@DOC_END */
template <class T>
void forEachSparseValue(const tinygltf::Model&                            tmodel,
                        const tinygltf::Accessor&                         accessor,
                        size_t                                            accessorFirstElement,
                        size_t                                            numElementsToProcess,
                        std::function<void(size_t index, const T* value)> fn)
{
  if(!accessor.sparse.isSparse)
  {
    return;  // Nothing to do
  }

  const auto& idxs = accessor.sparse.indices;
  if(!(idxs.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE      //
       || idxs.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT  //
       || idxs.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT))
  {
    assert(!"Unsupported sparse accessor index type.");
    return;
  }

  const tinygltf::BufferView& idxBufferView = tmodel.bufferViews[idxs.bufferView];
  const unsigned char*        idxBuffer     = &tmodel.buffers[idxBufferView.buffer].data[idxBufferView.byteOffset];
  const size_t                idxBufferByteStride =
      idxBufferView.byteStride ? idxBufferView.byteStride : tinygltf::GetComponentSizeInBytes(idxs.componentType);
  if(idxBufferByteStride == size_t(-1))
    return;  // Invalid

  const auto&                 vals          = accessor.sparse.values;
  const tinygltf::BufferView& valBufferView = tmodel.bufferViews[vals.bufferView];
  const unsigned char*        valBuffer     = &tmodel.buffers[valBufferView.buffer].data[valBufferView.byteOffset];
  const size_t                valBufferByteStride = accessor.ByteStride(valBufferView);
  if(valBufferByteStride == size_t(-1))
    return;  // Invalid

  // Note that this could be faster for lots of small copies, since we could
  // binary search for the first sparse accessor index to use (since the
  // glTF specification requires the indices be sorted)!
  for(size_t pairIdx = 0; pairIdx < accessor.sparse.count; pairIdx++)
  {
    // Read the index from the index buffer, converting its type
    size_t               index = 0;
    const unsigned char* pIdx  = idxBuffer + idxBufferByteStride * pairIdx;
    switch(idxs.componentType)
    {
      case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        index = *reinterpret_cast<const uint8_t*>(pIdx);
        break;
      case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        index = *reinterpret_cast<const uint16_t*>(pIdx);
        break;
      case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        index = *reinterpret_cast<const uint32_t*>(pIdx);
        break;
    }

    // If it's not in range, skip it
    if(index < accessorFirstElement || (index - accessorFirstElement) >= numElementsToProcess)
    {
      continue;
    }

    fn(index, reinterpret_cast<const T*>(valBuffer + valBufferByteStride * pairIdx));
  }
}

/* @DOC_START
## Function `copyAccessorData<T>`
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
@DOC_END */
template <class T>
void copyAccessorData(T*                        outData,
                      size_t                    outDataSizeInElements,
                      size_t                    outFirstElement,
                      const tinygltf::Model&    tmodel,
                      const tinygltf::Accessor& accessor,
                      size_t                    accessorFirstElement,
                      size_t                    numElementsToCopy)
{
  if(outFirstElement >= outDataSizeInElements)
  {
    assert(!"Invalid outFirstElement!");
    return;
  }

  if(accessorFirstElement >= accessor.count)
  {
    assert(!"Invalid accessorFirstElement!");
    return;
  }

  const tinygltf::BufferView& bufferView = tmodel.bufferViews[accessor.bufferView];
  const unsigned char* buffer = &tmodel.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset];

  const size_t maxSafeCopySize = std::min(accessor.count - accessorFirstElement, outDataSizeInElements - outFirstElement);
  numElementsToCopy = std::min(numElementsToCopy, maxSafeCopySize);

  if(bufferView.byteStride == 0)
  {
    memcpy(outData + outFirstElement, reinterpret_cast<const T*>(buffer) + accessorFirstElement, numElementsToCopy * sizeof(T));
  }
  else
  {
    // Must copy one-by-one
    for(size_t i = 0; i < numElementsToCopy; i++)
    {
      outData[outFirstElement + i] = *reinterpret_cast<const T*>(buffer + bufferView.byteStride * i);
    }
  }

  // Handle sparse accessors by overwriting already copied elements.
  forEachSparseValue<T>(tmodel, accessor, accessorFirstElement, numElementsToCopy,
                        [&outData](size_t index, const T* value) { outData[index] = *value; });
}


/* @DOC_START
## Function `copyAccessorData<T>(std::vector<T>& outData, ...)`
> Same as `copyAccessorData(T*, ...)`, but taking a vector.
@DOC_END */
template <class T>
void copyAccessorData(std::vector<T>&           outData,
                      size_t                    outFirstElement,
                      const tinygltf::Model&    tmodel,
                      const tinygltf::Accessor& accessor,
                      size_t                    accessorFirstElement,
                      size_t                    numElementsToCopy)
{
  copyAccessorData<T>(outData.data(), outData.size(), outFirstElement, tmodel, accessor, accessorFirstElement, numElementsToCopy);
}

// ScalarTypeGetter<T> returns the type of a single element (scalar) of T if
// T is a GLM vector, or T itself otherwise.
// We use this template specialization trick here, because T::value_type can
// only be constructed if T is a GLM vector.
template <typename T, bool isVector = std::is_class_v<T>>
struct ScalarTypeGetter
{
  using type = T::value_type;
};

template <typename T>
struct ScalarTypeGetter<T, false>
{
  using type = T;
};

/* @DOC_START
## Function `getAccessorData<T>`
> Appends all the values of `accessor` to `attribVec`, with conversion to type `T`.

Returns `false` if the accessor is invalid.
`T` must be one of the following types:
* Float vectors:            `float`,    `glm::vec2`,  `glm::vec3`,  or `glm::vec4`.
* Unsigned integer vectors: `uint32_t`, `glm::uvec2`, `glm::uvec3`, or `glm::uvec4`.
* Signed integer vectors:   `int32_t`,  `glm::ivec2`, `glm::ivec3`, or `glm::ivec4`.

@DOC_END */
template <typename T>
inline bool getAccessorData(const tinygltf::Model& tmodel, const tinygltf::Accessor& accessor, std::vector<T>& attribVec)
{
  // Retrieving the data of the accessor
  const size_t nbElems = accessor.count;

  const size_t oldNumElements = attribVec.size();
  attribVec.resize(oldNumElements + nbElems);

  // The following block of code figures out how to access T.
  using ScalarType                 = ScalarTypeGetter<T>::type;
  constexpr bool toFloat           = std::is_same_v<ScalarType, float>;
  constexpr int  gltfComponentType = (toFloat ? TINYGLTF_COMPONENT_TYPE_FLOAT :  //
                                         (std::is_same_v<ScalarType, uint32_t> ? TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT :  //
                                              TINYGLTF_COMPONENT_TYPE_INT));
  // 1, 2, 3, 4 for scalar, VEC2, VEC3, VEC4
  constexpr int nbComponents = sizeof(T) / sizeof(ScalarType);
  // This depends on how TINYGLTF_TYPE_VEC{n} == n.
  constexpr int gltfType = ((nbComponents == 1) ? TINYGLTF_TYPE_SCALAR : static_cast<int>(nbComponents));

  // Make sure the input and output have the same number of components
  if(accessor.type != gltfType)
  {
    return false;  // Invalid
  }

  // Copying the attributes
  if(accessor.componentType == gltfComponentType)
  {
    copyAccessorData<T>(attribVec, oldNumElements, tmodel, accessor, 0, accessor.count);
  }
  else
  {
    // The component is smaller than 32 bits and needs to be converted
    const auto&          bufView    = tmodel.bufferViews[accessor.bufferView];
    const auto&          buffer     = tmodel.buffers[bufView.buffer];
    const unsigned char* bufferByte = &buffer.data[accessor.byteOffset + bufView.byteOffset];

    // Stride per element
    const size_t byteStride = accessor.ByteStride(bufView);
    if(byteStride == size_t(-1))
      return false;  // Invalid

    if(!(accessor.componentType == TINYGLTF_COMPONENT_TYPE_BYTE || accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE
         || accessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT || accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT))
    {
      assert(!"Unhandled tinygltf component type!");
      return false;
    }

    const auto& copyElementFn = [&](size_t elementIdx, const unsigned char* pElement) {
      T vecValue{};

      for(int c = 0; c < nbComponents; c++)
      {
        ScalarType v{};

        switch(accessor.componentType)
        {
          case TINYGLTF_COMPONENT_TYPE_BYTE:
            v = static_cast<ScalarType>(*(reinterpret_cast<const char*>(pElement) + c));
            if constexpr(toFloat)
            {
              if(accessor.normalized)
              {
                v = std::max(v / 127.f, -1.f);
              }
            }
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            v = static_cast<ScalarType>(*(reinterpret_cast<const unsigned char*>(pElement) + c));
            if constexpr(toFloat)
            {
              if(accessor.normalized)
              {
                v = v / 255.f;
              }
            }
            break;
          case TINYGLTF_COMPONENT_TYPE_SHORT:
            v = static_cast<ScalarType>(*(reinterpret_cast<const short*>(pElement) + c));
            if constexpr(toFloat)
            {
              if(accessor.normalized)
              {
                v = std::max(v / 32767.f, -1.f);
              }
            }
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            v = static_cast<ScalarType>(*(reinterpret_cast<const unsigned short*>(pElement) + c));
            if constexpr(toFloat)
            {
              if(accessor.normalized)
              {
                v = v / 65535.f;
              }
            }
            break;
        }

        if constexpr(nbComponents == 1)
        {
          vecValue = v;
        }
        else
        {
          vecValue[c] = v;
        }
      }

      attribVec[oldNumElements + elementIdx] = vecValue;
    };

    for(size_t i = 0; i < nbElems; i++)
    {
      copyElementFn(i, bufferByte + byteStride * i);
    }

    forEachSparseValue<unsigned char>(tmodel, accessor, 0, nbElems, copyElementFn);
  }

  return true;
}

/* @DOC_START
## Function `getAccessorData2<T>`
> Returns a span with all the values of `accessor`.

This is like `getAccessorData<T>`, except it has a fast path if it can use the
buffer's data directly.

If the values needed conversion, re-packing, or had a sparse accessor, uses
the provided std::vector for storage. This vector must remain alive as long as
the pointer is in use.

Returns a span with nullptr data and 0 length if the accessor is invalid.

`T` must be one of the following types:
* Float vectors:            `float`,    `glm::vec2`,  `glm::vec3`,  or `glm::vec4`.
* Unsigned integer vectors: `uint32_t`, `glm::uvec2`, `glm::uvec3`, or `glm::uvec4`.
* Signed integer vectors:   `int32_t`,  `glm::ivec2`, `glm::ivec3`, or `glm::ivec4`.
@DOC_END */
template <typename T>
inline std::span<const T> getAccessorData2(const tinygltf::Model& tmodel, const tinygltf::Accessor& accessor, std::vector<T>& storage)
{
  const auto& bufferView = tmodel.bufferViews[accessor.bufferView];
  // Fast path: Can we return a pointer to the buffer directly?
  using ScalarType                = ScalarTypeGetter<T>::type;
  constexpr int gltfComponentType = (std::is_same_v<ScalarType, float> ? TINYGLTF_COMPONENT_TYPE_FLOAT :  //
                                         (std::is_same_v<ScalarType, uint32_t> ? TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT :  //
                                              TINYGLTF_COMPONENT_TYPE_INT));
  if((accessor.componentType == gltfComponentType)                          // Must not require conversion
     && (bufferView.byteStride == 0 || bufferView.byteStride == sizeof(T))  // Must not require re-packing
     && !accessor.sparse.isSparse)                                          // Must not be sparse
  {
    return getBufferDataSpan<T>(tmodel, accessor);
  }
  else
  {
    const bool ok = getAccessorData<T>(tmodel, accessor, storage);
    if(!ok)
    {
      return {};
    }
    return std::span<const T>(storage.data(), storage.size());
  }
}


/* @DOC_START
## Function `getAttribute<T>`
> Appends all the values of `attribName` to `attribVec`.

Returns `false` if the attribute is missing or invalid.
`T` must be `glm::vec2`, `glm::vec3`, or `glm::vec4`.
@DOC_END */
template <typename T>
inline bool getAttribute(const tinygltf::Model& tmodel, const tinygltf::Primitive& primitive, std::vector<T>& attribVec, const std::string& attribName)
{
  const auto& it = primitive.attributes.find(attribName);
  if(it == primitive.attributes.end())
    return false;
  const auto& accessor = tmodel.accessors[it->second];
  return getAccessorData(tmodel, accessor, attribVec);
}


/* @DOC_START
## Function `appendData<T>`
> Appends data from `inData` to the binary buffer `buffer` and returns the number
> of bytes of data added.

`T` should be a type like `std::vector`.
@DOC_END */
template <class T>
uint32_t appendData(tinygltf::Buffer& buffer, const T& inData)
{
  auto*    pData = reinterpret_cast<const char*>(inData.data());
  uint32_t len   = static_cast<uint32_t>(sizeof(inData[0]) * inData.size());
  buffer.data.insert(buffer.data.end(), pData, pData + len);
  return len;
}


//--------------------------------------------------------------------------------------------------
// Materials
//--------------------------------------------------------------------------------------------------
KHR_materials_unlit        getUnlit(const tinygltf::Material& tmat);
void                       setUnlit(tinygltf::Material& tmat, const KHR_materials_unlit& unlit);
KHR_materials_specular     getSpecular(const tinygltf::Material& tmat);
void                       setSpecular(tinygltf::Material& tmat, const KHR_materials_specular& specular);
KHR_materials_clearcoat    getClearcoat(const tinygltf::Material& tmat);
void                       setClearcoat(tinygltf::Material& tmat, const KHR_materials_clearcoat& clearcoat);
KHR_materials_sheen        getSheen(const tinygltf::Material& tmat);
void                       setSheen(tinygltf::Material& tmat, const KHR_materials_sheen& sheen);
KHR_materials_transmission getTransmission(const tinygltf::Material& tmat);
void                       setTransmission(tinygltf::Material& tmat, const KHR_materials_transmission& transmission);
KHR_materials_anisotropy   getAnisotropy(const tinygltf::Material& tmat);
void                       setAnisotropy(tinygltf::Material& tmat, const KHR_materials_anisotropy& anisotropy);
KHR_materials_ior          getIor(const tinygltf::Material& tmat);
void                       setIor(tinygltf::Material& tmat, const KHR_materials_ior& ior);
KHR_materials_volume       getVolume(const tinygltf::Material& tmat);
void                       setVolume(tinygltf::Material& tmat, const KHR_materials_volume& volume);
KHR_materials_displacement getDisplacement(const tinygltf::Material& tmat);
void                       setDisplacement(tinygltf::Material& tmat, const KHR_materials_displacement& displacement);
KHR_materials_emissive_strength getEmissiveStrength(const tinygltf::Material& tmat);
void setEmissiveStrength(tinygltf::Material& tmat, const KHR_materials_emissive_strength& emissiveStrength);
KHR_materials_iridescence getIridescence(const tinygltf::Material& tmat);
void                      setIridescence(tinygltf::Material& tmat, const KHR_materials_iridescence& iridescence);
KHR_materials_dispersion  getDispersion(const tinygltf::Material& tmat);
void                      setDispersion(tinygltf::Material& tmat, const KHR_materials_dispersion& dispersion);
KHR_materials_pbrSpecularGlossiness getPbrSpecularGlossiness(const tinygltf::Material& tmat);
void setPbrSpecularGlossiness(tinygltf::Material& tmat, const KHR_materials_pbrSpecularGlossiness& dispersion);
KHR_materials_diffuse_transmission getDiffuseTransmission(const tinygltf::Material& tmat);
void setDiffuseTransmission(tinygltf::Material& tmat, const KHR_materials_diffuse_transmission& diffuseTransmission);

template <typename T>
inline KHR_texture_transform getTextureTransform(const T& tinfo)
{
  KHR_texture_transform gmat;
  if(tinygltf::utils::hasElementName(tinfo.extensions, KHR_TEXTURE_TRANSFORM_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tinfo.extensions, KHR_TEXTURE_TRANSFORM_EXTENSION_NAME);
    tinygltf::utils::getArrayValue(ext, "offset", gmat.offset);
    tinygltf::utils::getArrayValue(ext, "scale", gmat.scale);
    tinygltf::utils::getValue(ext, "rotation", gmat.rotation);
    tinygltf::utils::getValue(ext, "texCoord", gmat.texCoord);

    gmat.updateTransform();
  }
  return gmat;
}

/* @DOC_START
## Function `getTextureImageIndex`
> Retrieves the image index of a texture, accounting for extensions such as
> `MSFT_texture_dds` and `KHR_texture_basisu`.
@DOC_END */
int getTextureImageIndex(const tinygltf::Texture& texture);

/* @DOC_START
> Retrieves the visibility of a node using `KHR_node_visibility`.

Does not search up the node hierarchy, so e.g. if node A points to node B and
node A is set to invisible and node B is set to visible, then
`getNodeVisibility(B)` will return `KHR_node_visibility{true}` even though
node B would not be visible due to node A.
@DOC_END */
KHR_node_visibility getNodeVisibility(const tinygltf::Node& node);
void                setNodeVisibility(tinygltf::Node& node, const KHR_node_visibility& visibility);


/* @DOC_START
## Function `getIndex`
  Return the index of the vertex in the buffer
@DOC_END */
int32_t getIndex(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const int32_t offset);

/* @DOC_START
## Function `getAttributeData`
 Return the data of the attribute: position, normal, ...
@DOC_END */
template <typename T>
inline static T* getAttributeData(tinygltf::Model& model, const tinygltf::Primitive& primitive, const int32_t vertexIndex, int32_t accessorIndex)
{
  const tinygltf::Accessor&   accessor   = model.accessors[accessorIndex];
  const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
  tinygltf::Buffer&           buffer     = model.buffers[bufferView.buffer];

  uint8_t*     data   = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
  const size_t stride = accessor.ByteStride(bufferView);
  return reinterpret_cast<T*>(data + vertexIndex * stride);
}

/* @DOC_START
## Function `createTangentAttribute`
 Create a tangent attribute for the primitive
@DOC_END */
void createTangentAttribute(tinygltf::Model& model, tinygltf::Primitive& primitive);

/* @DOC_START
## Function `simpleCreateTangents`
Compute tangents based on the texture coordinates, using also position and normal attributes
@DOC_END */
void simpleCreateTangents(tinygltf::Model& model, tinygltf::Primitive& primitive);


}  // namespace utils

}  // namespace tinygltf
