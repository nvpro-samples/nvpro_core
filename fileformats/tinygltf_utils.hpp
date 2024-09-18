/*
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2024 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#ifdef USE_CPP_20
#include <span>
#endif

#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>


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


namespace tinygltf {

namespace utils {

//--------------------------------------------------------------------------------------------------
// Utility functions to parse the glTF file
//--------------------------------------------------------------------------------------------------


// Get the value of type T for the attribute `name`.
// This function retrieves the value of the specified attribute from a tinygltf::Value
// and stores it in the provided result variable.
//
// Parameters:
// - value: The tinygltf::Value from which to retrieve the attribute.
// - name: The name of the attribute to retrieve.
// - result: The variable to store the retrieved value in.
template <typename T>
inline void getValue(const tinygltf::Value& value, const std::string& name, T& result)
{
  if(value.Has(name))
  {
    result = value.Get(name).Get<T>();
  }
}

// Specialization for float type.
// Retrieves the value of the specified attribute as a float and stores it in the result variable.
template <>
inline void getValue(const tinygltf::Value& value, const std::string& name, float& result)
{
  if(value.Has(name))
  {
    result = static_cast<float>(value.Get(name).Get<double>());
  }
}

// Specialization for nvvkhl::Gltf::Texture type.
// Retrieves the texture attribute values and stores them in the result variable.
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

// Get the value of type T for the attribute `name`.
// This function retrieves the array value of the specified attribute from a tinygltf::Value
// and stores it in the provided result variable. It is used for types such as glm::vec3, glm::vec4, glm::mat4, etc.
//
// Parameters:
// - value: The tinygltf::Value from which to retrieve the attribute.
// - name: The name of the attribute to retrieve.
// - result: The variable to store the retrieved array value in.
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

// Converts a vector of elements to a tinygltf::Value.
// This function converts a given array of float elements into a tinygltf::Value::Array,
// suitable for use within the tinygltf library.
//
// Parameters:
// - numElements: The number of elements in the array.
// - elements: A pointer to the array of float elements.
//
// Returns:
// - A tinygltf::Value representing the array of elements.
tinygltf::Value convertToTinygltfValue(int numElements, const float* elements);


// Retrieves the translation, rotation, and scale of a GLTF node.
// This function extracts the translation, rotation, and scale (TRS) properties
// from the given GLTF node. If the node has a matrix defined, it decomposes
// the matrix to obtain these properties. Otherwise, it directly retrieves
// the TRS values from the node's properties.
//
// Parameters:
// - node: The GLTF node from which to extract the TRS properties.
// - translation: Output parameter for the translation vector.
// - rotation: Output parameter for the rotation quaternion.
// - scale: Output parameter for the scale vector.
void getNodeTRS(const tinygltf::Node& node, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);


// Sets the translation, rotation, and scale of a GLTF node.
// This function sets the translation, rotation, and scale (TRS) properties of
// the given GLTF node using the provided values.
//
// Parameters:
// - node: The GLTF node to modify.
// - translation: The translation vector to set.
// - rotation: The rotation quaternion to set.
// - scale: The scale vector to set.
void setNodeTRS(tinygltf::Node& node, const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale);

// Retrieves the transformation matrix of a GLTF node.
// This function computes the transformation matrix for the given GLTF node.
// If the node has a direct matrix defined, it returns that matrix as defined in
// the specification. Otherwise, it computes the matrix from the node's translation,
// rotation, and scale (TRS) properties.
//
// Parameters:
// - node: The GLTF node for which to retrieve the transformation matrix.
//
// Returns:
// - The transformation matrix of the node.
glm::mat4 getNodeMatrix(const tinygltf::Node& node);


// Generates a unique key for a GLTF primitive based on its attributes.
// This function creates a unique string key for the given GLTF primitive by
// concatenating its attribute keys and values. This is useful for caching
// the primitive data, thereby avoiding redundancy.
//
// Parameters:
// - primitive: The GLTF primitive for which to generate the key.
//
// Returns:
// - A unique string key representing the primitive's attributes.
std::string generatePrimitiveKey(const tinygltf::Primitive& primitive);

// Traverses the scene graph and calls the provided functions for each element.
// This utility function recursively traverses the scene graph starting from the
// specified node ID. It calls the provided functions for cameras, lights, and
// meshes when encountered. The traversal can be stopped early if any function
// returns true.
//
// Parameters:
// - model: The GLTF model containing the scene graph.
// - nodeID: The ID of the node to start traversal from.
// - parentMat: The transformation matrix of the parent node.
// - fctCam: Function to call when a camera is encountered. Can be nullptr.
// - fctLight: Function to call when a light is encountered. Can be nullptr.
// - fctMesh: Function to call when a mesh is encountered. Can be nullptr.
void traverseSceneGraph(const tinygltf::Model&                            model,
                        int                                               nodeID,
                        const glm::mat4&                                  parentMat,
                        const std::function<bool(int, const glm::mat4&)>& fctCam   = nullptr,
                        const std::function<bool(int, const glm::mat4&)>& fctLight = nullptr,
                        const std::function<bool(int, const glm::mat4&)>& fctMesh  = nullptr);

// Return the number of vertices in a primitive.
// This function retrieves the number of vertices for the given GLTF primitive
// by accessing the "POSITION" attribute in the model's accessors.
//
// Parameters:
// - model: The GLTF model containing the primitive data.
// - primitive: The GLTF primitive for which to retrieve the vertex count.
//
// Returns:
// - The number of vertices in the primitive.
size_t getVertexCount(const tinygltf::Model& model, const tinygltf::Primitive& primitive);

// Return the number of indices in a primitive.
// This function retrieves the number of indices for the given GLTF primitive
// by accessing the indices in the model's accessors. If no indices are present,
// it returns the number of vertices instead.
//
// Parameters:
// - model: The GLTF model containing the primitive data.
// - primitive: The GLTF primitive for which to retrieve the index count.
//
// Returns:
// - The number of indices in the primitive, or the number of vertices if no indices are present.
size_t getIndexCount(const tinygltf::Model& model, const tinygltf::Primitive& primitive);

// Check if the map has the specified element.
// Can be used for extensions, extras, or any other map.
// Returns true if the map has the specified element, false otherwise.
template <typename MapType>
bool hasElementName(const MapType& map, const std::string& key)
{
  return map.find(key) != map.end();
}

// Get the value of the specified element from the map.
// Can be extension, extras, or any other map.
// Returns the value of the element.
template <typename MapType>
const typename MapType::mapped_type& getElementValue(const MapType& map, const std::string& key)
{
  return map.at(key);
}


// This function retrieves the buffer data for the specified accessor from the GLTF model
// and returns it as a span of type T. The function assumes that the buffer data is of type T.
// The function performs assertions to ensure that the accessor and buffer data are compatible.
// Example usage:
// int accessorIndex = primitive.attributes.at("POSITION");
// std::span<const glm::vec3> positions = tinygltf::utils::getBufferDataSpan<glm::vec3>(model, accessorIndex);

template <typename T>
#ifdef USE_CPP_20
std::span<const T> getBufferDataSpan(const tinygltf::Model& model, int accessorIndex)
#else
std::pair<const T*, size_t> getBufferDataSpan(const tinygltf::Model& model, int accessorIndex)
#endif
{
  const tinygltf::Accessor&   accessor = model.accessors[accessorIndex];
  const tinygltf::BufferView& view     = model.bufferViews[accessor.bufferView];
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
#ifdef USE_CPP_20
  return std::span<const T>(bufferData, accessor.count);
#else
  return {bufferData, accessor.count};
#endif
}


// Extract the vector of type T for the attribute.
// This function retrieves the data for the specified attribute from the GLTF model
// and copies it into a vector of type T. Note that this method copies the data,
// which may not be the most efficient way to handle large datasets.
//
// Example usage:
// std::vector<glm::vec3> translations = tinygltf::utils::extractAttributeData<glm::vec3>(model, attributes, "TRANSLATION");
//
// Parameters:
// - model: The GLTF model containing the attribute data.
// - attributes: The attribute values to parse.
// - attributeName: The name of the attribute to retrieve.
//
// Returns:
// - A vector of type T containing the attribute data.
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


// Calls a function (such as a lambda function) for each (index, value) pair in
// a sparse accessor. It's only potentially called for indices from
// accessorFirstElement through accessorFirstElement + numElementsToProcess - 1.
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

// Copies accessor elements accessorFirstElement through
// accessorFirstElement + numElementsToCopy - 1 to outData elements
// outFirstElement through outFirstElement + numElementsToCopy - 1.
// This handles sparse accessors correctly! It's intended as a replacement for
// what would be memcpy(..., &buffer.data[...], ...) calls.
//
// However, it performs no conversion: it assumes (but does not check) that
// accessor's elements are of type T. For instance, T should be a struct of two
// floats for a VEC2 float accessor.
//
// This is range-checked, so elements that would be out-of-bounds are not
// copied. We assume size_t overflow does not occur.
// Note that outDataSizeInT is the number of elements in the outDataBuffer,
// while numElementsToCopy is the number of elements to copy, not the number
// of elements in accessor.
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

// Same as copyAccessorData(T*, ...), but taking a vector.
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

// Appending to \p attribVec, all the values of \p accessor
// Return false if the accessor is invalid.
// T must be glm::vec2, glm::vec3, or glm::vec4.
template <typename T>
inline bool getAccessorData(const tinygltf::Model& tmodel, const tinygltf::Accessor& accessor, std::vector<T>& attribVec)
{
  // Retrieving the data of the accessor
  const auto nbElems = accessor.count;

  const size_t oldNumElements = attribVec.size();
  attribVec.resize(oldNumElements + nbElems);

  // Copying the attributes
  if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
  {
    copyAccessorData<T>(attribVec, oldNumElements, tmodel, accessor, 0, accessor.count);
  }
  else
  {
    // The component is smaller than float and need to be converted
    const auto&          bufView    = tmodel.bufferViews[accessor.bufferView];
    const auto&          buffer     = tmodel.buffers[bufView.buffer];
    const unsigned char* bufferByte = &buffer.data[accessor.byteOffset + bufView.byteOffset];

    // 2, 3, 4 for VEC2, VEC3, VEC4
    const int nbComponents = tinygltf::GetNumComponentsInType(accessor.type);
    if(nbComponents == -1)
      return false;  // Invalid

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
        switch(accessor.componentType)
        {
          case TINYGLTF_COMPONENT_TYPE_BYTE:
            vecValue[c] = float(*(reinterpret_cast<const char*>(pElement) + c));
            if(accessor.normalized)
            {
              vecValue[c] = std::max(vecValue[c] / 127.f, -1.f);
            }
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            vecValue[c] = float(*(reinterpret_cast<const unsigned char*>(pElement) + c));
            if(accessor.normalized)
            {
              vecValue[c] = vecValue[c] / 255.f;
            }
            break;
          case TINYGLTF_COMPONENT_TYPE_SHORT:
            vecValue[c] = float(*(reinterpret_cast<const short*>(pElement) + c));
            if(accessor.normalized)
            {
              vecValue[c] = std::max(vecValue[c] / 32767.f, -1.f);
            }
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            vecValue[c] = float(*(reinterpret_cast<const unsigned short*>(pElement) + c));
            if(accessor.normalized)
            {
              vecValue[c] = vecValue[c] / 65535.f;
            }
            break;
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

// Appending to \p attribVec, all the values of \p attribName
// Return false if the attribute is missing or invalid.
// T must be glm::vec2, glm::vec3, or glm::vec4.
template <typename T>
inline bool getAttribute(const tinygltf::Model& tmodel, const tinygltf::Primitive& primitive, std::vector<T>& attribVec, const std::string& attribName)
{
  const auto& it = primitive.attributes.find(attribName);
  if(it == primitive.attributes.end())
    return false;
  const auto& accessor = tmodel.accessors[it->second];
  return getAccessorData(tmodel, accessor, attribVec);
}


// This is appending the incoming data to the binary buffer (just one)
// and return the amount in byte of data that was added.
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
KHR_materials_unlit             getUnlit(const tinygltf::Material& tmat);
KHR_materials_specular          getSpecular(const tinygltf::Material& tmat);
KHR_materials_clearcoat         getClearcoat(const tinygltf::Material& tmat);
KHR_materials_sheen             getSheen(const tinygltf::Material& tmat);
KHR_materials_transmission      getTransmission(const tinygltf::Material& tmat);
KHR_materials_anisotropy        getAnisotropy(const tinygltf::Material& tmat);
KHR_materials_ior               getIor(const tinygltf::Material& tmat);
KHR_materials_volume            getVolume(const tinygltf::Material& tmat);
KHR_materials_displacement      getDisplacement(const tinygltf::Material& tmat);
KHR_materials_emissive_strength getEmissiveStrength(const tinygltf::Material& tmat);
KHR_materials_iridescence       getIridescence(const tinygltf::Material& tmat);

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

// Retrieves the image index of a texture, accounting for extensions such as
// MSFT_texture_dds and KHR_texture_basisu.
int getTextureImageIndex(const tinygltf::Texture& texture);

}  // namespace utils

}  // namespace tinygltf
