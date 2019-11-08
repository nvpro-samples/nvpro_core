/* Copyright (c) 2014-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <algorithm>
#include <iostream>
#include <set>


#include "gltfscene.hpp"

namespace nvmath {

template <typename T>
T mix(T x, T y, float a)
{
  return x + a * (y - x);
}

template <typename T>
quaternion<T> slerp(quaternion<T> const& x, quaternion<T> const& y, T a)
{
  quaternion<T> z = y;

  T cosTheta = dot(x, y);

  // If cosTheta < 0, the interpolation will take the long way around the sphere.
  // To fix this, one quat must be negated.
  if(cosTheta < T(0))
  {
    z.conjugate();
    cosTheta = -cosTheta;
  }

  // Perform a linear interpolation when cosTheta is close to 1 to avoid side effect of sin(angle) becoming a zero denominator
  if(cosTheta > T(1) - std::numeric_limits<T>::epsilon())
  {
    // Linear interpolation
    return quaternion<T>(mix(x.w, z.w, a), mix(x.x, z.x, a), mix(x.y, z.y, a), mix(x.z, z.z, a));
  }

  // Essential Mathematics, page 467
  T angle = acos(cosTheta);
  // TODO --- not working
  return x;
  //return (sin((T(1) - a) * angle) * x + sin(a * angle) * z) / sin(angle);
}

}  // namespace nvmath


//--------------------------------------------------------------------------------------------------

namespace nvh {

namespace gltf {


void Primitive::setDimensions(const nvmath::vec3f& min, const nvmath::vec3f& max)
{
  m_dimensions.min    = min;
  m_dimensions.max    = max;
  m_dimensions.size   = max - min;
  m_dimensions.center = (min + max) / 2.0f;
  m_dimensions.radius = nvmath::length(max - min) / 2.0f;
}


//--------------------------------------------------------------------------------------------------
// Converting the transform and matrices into a single matrix
//
nvmath::mat4f Node::localMatrix() const
{
  nvmath::mat4f mtranslation, mscale, mrot;
  nvmath::quatf mrotation;
  mtranslation.as_translation(m_translation);
  mscale.as_scale(m_scale);
  m_rotation.to_matrix(mrot);

  return mtranslation * mrot * mscale * m_matrix;
}

//--------------------------------------------------------------------------------------------------
// Returning the world Matrix by cumulating all parents
//
nvmath::mat4f Node::worldMatrix() const
{
  nvmath::mat4f m = localMatrix();
  gltf::Node*   p = m_parent;
  while(p != nullptr)
  {
    m = p->localMatrix() * m;
    p = p->m_parent;
  }
  return m;
}


//--------------------------------------------------------------------------------------------------
// Destroy the entire scene
//
void Scene::destroy()
{
  for(auto node : m_nodes)
  {
    delete node;
  }
  for(auto mesh : m_linearMeshes)
  {
    delete mesh;
  }
  m_nodes.clear();
  m_linearMeshes.clear();
}


//--------------------------------------------------------------------------------------------------
// Convert tinyNode to our representation
//
void Scene::loadNode(gltf::Node* parentNode, const tinygltf::Node& tinyNode, uint32_t nodeIndex, const tinygltf::Model& tinyModel)
{
  auto newNode         = new gltf::Node{};
  newNode->m_index     = nodeIndex;
  newNode->m_parent    = parentNode;
  newNode->m_name      = tinyNode.name;
  newNode->m_skinIndex = tinyNode.skin;
  newNode->m_matrix.identity();

  // Generate local node matrix
  if(tinyNode.translation.size() == 3)
  {
    newNode->m_translation.x = static_cast<float>(tinyNode.translation[0]);
    newNode->m_translation.y = static_cast<float>(tinyNode.translation[1]);
    newNode->m_translation.z = static_cast<float>(tinyNode.translation[2]);
  }

  if(tinyNode.rotation.size() == 4)
  {
    newNode->m_rotation.x = static_cast<float>(tinyNode.rotation[0]);
    newNode->m_rotation.y = static_cast<float>(tinyNode.rotation[1]);
    newNode->m_rotation.z = static_cast<float>(tinyNode.rotation[2]);
    newNode->m_rotation.w = static_cast<float>(tinyNode.rotation[3]);
  }

  if(tinyNode.scale.size() == 3)
  {
    newNode->m_scale.x = static_cast<float>(tinyNode.scale[0]);
    newNode->m_scale.y = static_cast<float>(tinyNode.scale[1]);
    newNode->m_scale.z = static_cast<float>(tinyNode.scale[2]);
  }

  if(tinyNode.matrix.size() == 16)
  {
    for(int i = 0; i < 16; ++i)
    {
      newNode->m_matrix.mat_array[i] = static_cast<float>(tinyNode.matrix[i]);
    }
  };

  // Node with children
  if(!tinyNode.children.empty())
  {
    for(int nodeIndex : tinyNode.children)
    {
      loadNode(newNode, tinyModel.nodes[nodeIndex], nodeIndex, tinyModel);
    }
  }

  // Node contains mesh data
  if(tinyNode.mesh > -1)
  {
    newNode->m_mesh = tinyNode.mesh;
  }

  if(parentNode != nullptr)
  {
    parentNode->m_children.push_back(newNode);
  }
  else
  {
    m_nodes.push_back(newNode);
  }

  m_linearNodes.push_back(newNode);
}


//--------------------------------------------------------------------------------------------------
// Loading meshes and returning all the positions and attributes in the vertex data
//
void Scene::loadMeshes(const tinygltf::Model& tinyModel, std::vector<uint32_t>& indexBuffer, VertexData& vertexData)
{
  // Keeping the default values of all attributes
  auto defaultValues = vertexData.attributes;

  // Find the size of indices and position for all meshes.
  uint32_t indexTotCount  = 0;
  uint32_t vertexTotCount = 0;
  for(const auto& mesh : tinyModel.meshes)
  {
    for(const tinygltf::Primitive& primitive : mesh.primitives)
    {
      const tinygltf::Accessor& posAccessor = tinyModel.accessors[primitive.attributes.find("POSITION")->second];
      vertexTotCount += static_cast<uint32_t>(posAccessor.count);
      const tinygltf::Accessor& indexAccessor = tinyModel.accessors[primitive.indices];
      indexTotCount += static_cast<uint32_t>(indexAccessor.count);
    }
  }

  // Using the information about the size, to reserve the memory of all buffers
  indexBuffer.reserve(indexTotCount);
  vertexData.position.reserve(vertexTotCount);
  for(auto& attrib : vertexData.attributes)
  {
    auto nbElem = defaultValues[attrib.first].size();
    attrib.second.clear();
    attrib.second.reserve(vertexTotCount * nbElem);
  }


  // Per primitive Index array, keeping it here to avoid reallocation of vectors
  std::vector<uint32_t> primitiveIndices32u;
  std::vector<uint16_t> primitiveIndices16u;
  std::vector<uint8_t>  primitiveIndices8u;


  // Looping over all meshes in the scene
  for(const auto& mesh : tinyModel.meshes)
  {
    nvmath::mat4f matrix;
    matrix.identity();
    auto newMesh = new gltf::Mesh(matrix);

    // Looping over all primitives in
    for(const tinygltf::Primitive& primitive : mesh.primitives)
    {
      if(primitive.indices < 0)
      {
        continue;
      }

      // Keeping the stating position of the arrays
      auto vertexStart = static_cast<uint32_t>(vertexData.position.size());
      auto indexStart  = static_cast<uint32_t>(indexBuffer.size());


      uint32_t      indexCount = 0;
      nvmath::vec3f posMin{};
      nvmath::vec3f posMax{};


      // Vertices
      {
        // Position attribute is required
        assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

        size_t nbPositions = 0;

        {
          // Retrieving the positions of the primitive
          const tinygltf::Accessor&   posAccessor = tinyModel.accessors[primitive.attributes.find("POSITION")->second];
          const tinygltf::BufferView& posView     = tinyModel.bufferViews[posAccessor.bufferView];

          // Keeping the number of positions to make all attributes the same size
          nbPositions = posAccessor.count;

          // Copying the position to local data
          auto bufferPos = reinterpret_cast<const float*>(
              &(tinyModel.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
          vertexData.position.resize(vertexStart + posAccessor.count);
          memcpy(&vertexData.position[vertexStart], bufferPos, posAccessor.count * 3 * sizeof(float));

          // Keeping the size of this primitive (Spec says this is required information)
          if(!posAccessor.minValues.empty())
          {
            posMin = nvmath::vec3f(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
          }
          if(!posAccessor.maxValues.empty())
          {
            posMax = nvmath::vec3f(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
          }
        }


        // Looping over all the attributes we are interested in.
        for(auto& attrib : vertexData.attributes)
        {
          auto nbElem      = defaultValues[attrib.first].size();  // Number of elements this attribute has
          auto startAttrib = attrib.second.size();  // Keeping track of the starting position in the vector
          attrib.second.resize(startAttrib + nbPositions * nbElem);  // Resize the attribute to hold the new data

          // Find if the attribute we are looking for is present
          bool attribSucceed = false;
          if(primitive.attributes.find(attrib.first) != primitive.attributes.end())
          {
            const tinygltf::Accessor& attribAccessor = tinyModel.accessors[primitive.attributes.find(attrib.first)->second];
            const tinygltf::BufferView& attribBufferView = tinyModel.bufferViews[attribAccessor.bufferView];

            auto attribBuffer = reinterpret_cast<const float*>(
                &(tinyModel.buffers[attribBufferView.buffer].data[attribAccessor.byteOffset + attribBufferView.byteOffset]));

            assert(attribAccessor.count == nbPositions);

            // Check if the data is compatible
            if(attribAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
              attribSucceed = true;
              if(attribAccessor.type == nbElem)
              {
                memcpy(&attrib.second[startAttrib], attribBuffer, attribAccessor.count * nbElem * sizeof(float));
              }
              else
              {
                // Size is different, so we adjust
                for(size_t i = 0; i < nbPositions; ++i)
                {
                  // Copy the default value
                  attrib.second.insert(std::end(attrib.second), std::begin(defaultValues[attrib.first]),
                                       std::end(defaultValues[attrib.first]));


                  for(size_t e = 0; e < std::min(nbElem, static_cast<size_t>(attribAccessor.type)); ++e)
                  {
                    attrib.second[startAttrib + i * nbElem + e] = attribBuffer[i * attribAccessor.type + e];
                  }
                }
              }
            }
            else
            {
              // #TODO implement type converters to Float
            }
          }

          if(!attribSucceed)
          {
            // The attribute was not present or it was of a different type, so all elements will have the default value
            float*              writingAttrib = &attrib.second[startAttrib];
            std::vector<float>& values(defaultValues[attrib.first]);
            for(size_t i = 0; i < nbPositions; ++i)
            {
              for(size_t e = 0; e < nbElem; ++e)
              {
                *writingAttrib++ = values[e];
              }
            }
          }
        }
      }

      // Indices
      {
        const tinygltf::Accessor&   indexAccessor = tinyModel.accessors[primitive.indices];
        const tinygltf::BufferView& bufferView    = tinyModel.bufferViews[indexAccessor.bufferView];
        const tinygltf::Buffer&     buffer        = tinyModel.buffers[bufferView.buffer];

        indexCount = static_cast<uint32_t>(indexAccessor.count);

        switch(indexAccessor.componentType)
        {
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
          {
            primitiveIndices32u.resize(indexAccessor.count);
            memcpy(primitiveIndices32u.data(), &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset],
                   indexAccessor.count * sizeof(uint32_t));
            indexBuffer.insert(std::end(indexBuffer), std::begin(primitiveIndices32u), std::end(primitiveIndices32u));
            break;
          }
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
          {
            primitiveIndices16u.resize(indexAccessor.count);
            memcpy(primitiveIndices16u.data(), &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset],
                   indexAccessor.count * sizeof(uint16_t));
            indexBuffer.insert(std::end(indexBuffer), std::begin(primitiveIndices16u), std::end(primitiveIndices16u));
            break;
          }
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
          {
            primitiveIndices8u.resize(indexAccessor.count);
            memcpy(primitiveIndices8u.data(), &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset],
                   indexAccessor.count * sizeof(uint8_t));
            indexBuffer.insert(std::end(indexBuffer), std::begin(primitiveIndices8u), std::end(primitiveIndices8u));
            break;
          }
          default:
            std::cerr << "Index component type " << indexAccessor.componentType << " not supported!" << std::endl;
            return;
        }
      }

      Primitive newPrimitive;
      newPrimitive.m_firstIndex    = indexStart;
      newPrimitive.m_indexCount    = indexCount;
      newPrimitive.m_vertexOffset  = vertexStart;
      newPrimitive.m_materialIndex = primitive.material;
      newPrimitive.setDimensions(posMin, posMax);
      newMesh->m_primitives.push_back(newPrimitive);
    }
    m_linearMeshes.push_back(newMesh);
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void Scene::loadNodes(const tinygltf::Model& gltfModel)
{
  const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene];

  for(int i : scene.nodes)
  {
    const tinygltf::Node& node = gltfModel.nodes[i];
    loadNode(nullptr, node, i, gltfModel);
  }
}

//--------------------------------------------------------------------------------------------------
// Retrieve all skins from tiny scene
//
void Scene::loadSkins(const tinygltf::Model& gltfModel)
{
  for(const tinygltf::Skin& source : gltfModel.skins)
  {
    auto newSkin    = new Skin{};
    newSkin->m_name = source.name;

    // Find skeleton root node
    if(source.skeleton > -1)
    {
      newSkin->m_skeletonRoot = nodeFromIndex(source.skeleton);
    }

    // Find joint nodes
    for(int jointIndex : source.joints)
    {
      gltf::Node* node = nodeFromIndex(jointIndex);
      if(node != nullptr)
      {
        newSkin->m_joints.push_back(nodeFromIndex(jointIndex));
      }
    }

    // Get inverse bind matrices from buffer
    if(source.inverseBindMatrices > -1)
    {
      const tinygltf::Accessor&   accessor   = gltfModel.accessors[source.inverseBindMatrices];
      const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
      const tinygltf::Buffer&     buffer     = gltfModel.buffers[bufferView.buffer];
      newSkin->m_inverseBindMatrices.resize(accessor.count);
      memcpy(newSkin->m_inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset],
             accessor.count * sizeof(nvmath::mat4f));
    }

    m_skins.push_back(newSkin);
  }
}

template <typename T>
static T getVector(const tinygltf::Value& value)
{
  T result;
  if(!value.IsArray())
    return result;

  for(int i = 0; i < value.ArrayLen(); i++)
  {
    result[i] = static_cast<float>(value.Get(i).IsNumber() ? value.Get(i).Get<double>() : value.Get(i).Get<int>());
  }
  return result;
}

template <typename T>
static T getNumber(const tinygltf::Value& value)
{
  T result = T(0);
  if(!value.IsNumber() && !value.IsInt())
    return result;

  result = static_cast<float>(value.IsNumber() ? value.Get<double>() : value.Get<int>());
  return result;
}

//--------------------------------------------------------------------------------------------------
// Convert all tiny Materials to our representation
//
void Scene::loadMaterials(tinygltf::Model& gltfModel)
{
  m_numTextures = (uint32_t)gltfModel.images.size();

  for(tinygltf::Material& mat : gltfModel.materials)
  {
    Material material{};

    if(mat.values.find("baseColorFactor") != mat.values.end())
    {
      tinygltf::ColorValue cv          = mat.values["baseColorFactor"].ColorFactor();
      material.m_mat.baseColorFactor.x = static_cast<float>(cv[0]);
      material.m_mat.baseColorFactor.y = static_cast<float>(cv[1]);
      material.m_mat.baseColorFactor.z = static_cast<float>(cv[2]);
      material.m_mat.baseColorFactor.w = static_cast<float>(cv[3]);
    }

    if(mat.values.find("roughnessFactor") != mat.values.end())
    {
      material.m_mat.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
    }

    if(mat.values.find("metallicFactor") != mat.values.end())
    {
      material.m_mat.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
    }

    if(mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end())
    {
      tinygltf::ColorValue cv         = mat.additionalValues["emissiveFactor"].ColorFactor();
      material.m_mat.emissiveFactor.x = static_cast<float>(cv[0]);
      material.m_mat.emissiveFactor.y = static_cast<float>(cv[1]);
      material.m_mat.emissiveFactor.z = static_cast<float>(cv[2]);
    }

    if(mat.additionalValues.find("alphaMode") != mat.additionalValues.end())
    {
      tinygltf::Parameter param = mat.additionalValues["alphaMode"];

      if(param.string_value == "BLEND")
      {
        material.m_mat.alphaMode = (int)AlphaMode::eBlend;
      }

      if(param.string_value == "MASK")
      {
        material.m_mat.alphaMode = (int)AlphaMode::eMask;
      }
    }

    if(mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end())
    {
      material.m_mat.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
    }

    if(mat.additionalValues.find("doubleSided") != mat.additionalValues.end())
    {
      material.m_mat.doubleSided = mat.additionalValues["doubleSided"].bool_value ? 1 : 0;
    }


    if(mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end())
    {
      material.m_mat.metallicFactor = 0.0f;

      tinygltf::Value pbr  = mat.extensions["KHR_materials_pbrSpecularGlossiness"];
      auto            keys = pbr.Keys();
      if(pbr.Has("diffuseFactor"))
      {
        material.m_mat.baseColorFactor = getVector<nvmath::vec4f>(pbr.Get("diffuseFactor"));
      }
      if(pbr.Has("glossinessFactor"))
      {
        material.m_mat.glossinessFactor = getNumber<float>(pbr.Get("glossinessFactor"));
      }
      if(pbr.Has("specularFactor"))
      {
        material.m_mat.specularFactor = getVector<nvmath::vec3f>(pbr.Get("specularFactor"));
      }
      if(pbr.Has("diffuseTexture"))
      {
        auto index                  = pbr.Get("diffuseTexture").Get("index").Get<int>();
        material.m_baseColorTexture = gltfModel.textures[index].source;
      }
    }


    // For textures to exist, loadImage should be done earlier.
    if(m_numTextures != 0)
    {
      if(mat.values.find("baseColorTexture") != mat.values.end())
      {
        material.m_baseColorTexture = gltfModel.textures[mat.values["baseColorTexture"].TextureIndex()].source;
      }

      if(mat.values.find("metallicRoughnessTexture") != mat.values.end())
      {
        material.m_metallicRoughnessTexture = gltfModel.textures[mat.values["metallicRoughnessTexture"].TextureIndex()].source;
      }

      if(mat.additionalValues.find("normalTexture") != mat.additionalValues.end())
      {
        material.m_normalTexture = gltfModel.textures[mat.additionalValues["normalTexture"].TextureIndex()].source;
      }

      if(mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end())
      {
        material.m_emissiveTexture = gltfModel.textures[mat.additionalValues["emissiveTexture"].TextureIndex()].source;
      }

      if(mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end())
      {
        material.m_occlusionTexture = gltfModel.textures[mat.additionalValues["occlusionTexture"].TextureIndex()].source;
      }
    }

    m_materials.emplace_back(material);
  }
}

//--------------------------------------------------------------------------------------------------
// Convert all tiny animation to our representation
//
void Scene::loadAnimations(const tinygltf::Model& gltfModel)
{
  for(const tinygltf::Animation& anim : gltfModel.animations)
  {
    Animation animation{};
    animation.m_name = anim.name;

    if(anim.name.empty())
    {
      animation.m_name = std::to_string(m_animations.size());
    }

    // Samplers
    for(auto& samp : anim.samplers)
    {
      AnimationSampler sampler{};

      if(samp.interpolation == "LINEAR")
      {
        sampler.m_interpolation = InterpolationType::eLinear;
      }

      if(samp.interpolation == "STEP")
      {
        sampler.m_interpolation = InterpolationType::eStep;
      }

      if(samp.interpolation == "CUBICSPLINE")
      {
        sampler.m_interpolation = InterpolationType::eCubicSpline;
      }

      // Read sampler input time values
      {
        const tinygltf::Accessor&   accessor   = gltfModel.accessors[samp.input];
        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
        const tinygltf::Buffer&     buffer     = gltfModel.buffers[bufferView.buffer];

        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        auto buf = new float[accessor.count];
        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(float));
        for(size_t index = 0; index < accessor.count; index++)
        {
          sampler.m_inputs.push_back(buf[index]);
        }

        for(auto input : sampler.m_inputs)
        {
          if(input < animation.m_start)
          {
            animation.m_start = input;
          };

          if(input > animation.m_end)
          {
            animation.m_end = input;
          }
        }
      }

      // Read sampler output T/R/S values
      {
        const tinygltf::Accessor&   accessor   = gltfModel.accessors[samp.output];
        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
        const tinygltf::Buffer&     buffer     = gltfModel.buffers[bufferView.buffer];

        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        switch(accessor.type)
        {
          case TINYGLTF_TYPE_VEC3:
          {
            auto buf = new nvmath::vec3f[accessor.count];
            memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(nvmath::vec3f));
            for(size_t index = 0; index < accessor.count; index++)
            {
              sampler.m_outputsVec4.emplace_back(buf[index], 0.0f);
            }
            break;
          }

          case TINYGLTF_TYPE_VEC4:
          {
            auto buf = new nvmath::vec4f[accessor.count];
            memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(nvmath::vec4f));
            for(size_t index = 0; index < accessor.count; index++)
            {
              sampler.m_outputsVec4.emplace_back(buf[index]);
            }
            break;
          }

          default:
          {
            std::cout << "unknown type" << std::endl;
            break;
          }
        }
      }

      animation.m_samplers.push_back(sampler);
    }

    // Channels
    for(auto& source : anim.channels)
    {
      AnimationChannel channel{};

      if(source.target_path == "rotation")
      {
        channel.m_path = PathType::eRotation;
      }

      if(source.target_path == "translation")
      {
        channel.m_path = PathType::eTranslation;
      }

      if(source.target_path == "scale")
      {
        channel.m_path = PathType::eScale;
      }

      if(source.target_path == "weights")
      {
        std::cout << "weights not yet supported, skipping channel" << std::endl;
        continue;
      }

      channel.m_samplerIndex = source.sampler;
      channel.m_node         = nodeFromIndex(source.target_node);

      if(channel.m_node == nullptr)
      {
        continue;
      }

      animation.m_channels.push_back(channel);
    }

    m_animations.push_back(animation);
  }
}

//--------------------------------------------------------------------------------------------------
// Get the dimension of a node
//
void Scene::getNodeDimensions(const gltf::Node* node, nvmath::vec3f& min, nvmath::vec3f& max)
{
  if(node->m_mesh != ~0u)
  {
    for(auto& primitive : m_linearMeshes[node->m_mesh]->m_primitives)
    {
      nvmath::vec4f locMin = node->worldMatrix() * nvmath::vec4f(primitive.m_dimensions.min, 1.0f);
      nvmath::vec4f locMax = node->worldMatrix() * nvmath::vec4f(primitive.m_dimensions.max, 1.0f);

      min = {std::min(locMin.x, min.x), std::min(locMin.y, min.y), std::min(locMin.z, min.z)};
      max = {std::max(locMax.x, max.x), std::max(locMax.y, max.y), std::max(locMax.z, max.z)};
    }
  }

  for(auto child : node->m_children)
  {
    getNodeDimensions(child, min, max);
  }
}

//--------------------------------------------------------------------------------------------------
// Get the dimension of the scene
//
void Scene::computeSceneDimensions()
{
  m_dimensions.min = nvmath::vec3f(FLT_MAX);
  m_dimensions.max = nvmath::vec3f(-FLT_MAX);
  for(auto node : m_nodes)
  {
    getNodeDimensions(node, m_dimensions.min, m_dimensions.max);
  }
  m_dimensions.size   = m_dimensions.max - m_dimensions.min;
  m_dimensions.center = (m_dimensions.min + m_dimensions.max) / 2.0f;
  m_dimensions.radius = nvmath::length(m_dimensions.max - m_dimensions.min) / 2.0f;
}

//--------------------------------------------------------------------------------------------------
// If there is an animation, update all channels and update the matrices
//
void Scene::updateAnimation(uint32_t index, float time)
{
  if(index > static_cast<uint32_t>(m_animations.size()) - 1)
  {
    std::cout << "No animation with index " << index << std::endl;
    return;
  }
  Animation& animation = m_animations[index];

  bool updated = false;
  for(auto& channel : animation.m_channels)
  {
    AnimationSampler& sampler = animation.m_samplers[channel.m_samplerIndex];
    if(sampler.m_inputs.size() > sampler.m_outputsVec4.size())
    {
      continue;
    }

    for(auto i = 0; i < sampler.m_inputs.size() - 1; i++)
    {
      if((time >= sampler.m_inputs[i]) && (time <= sampler.m_inputs[i + 1]))
      {
        float u = std::max(0.0f, time - sampler.m_inputs[i]) / (sampler.m_inputs[i + 1] - sampler.m_inputs[i]);
        if(u <= 1.0f)
        {
          switch(channel.m_path)
          {
            case PathType::eTranslation:
            {
              nvmath::vec4f trans           = nvmath::mix(sampler.m_outputsVec4[i], sampler.m_outputsVec4[i + 1], u);
              channel.m_node->m_translation = nvmath::vec3f(trans);
              break;
            }
            case PathType::eScale:
            {
              nvmath::vec4f trans     = nvmath::mix(sampler.m_outputsVec4[i], sampler.m_outputsVec4[i + 1], u);
              channel.m_node->m_scale = nvmath::vec3f(trans);
              break;
            }
            case PathType::eRotation:
            {
              nvmath::quatf q1;
              q1.x = sampler.m_outputsVec4[i].x;
              q1.y = sampler.m_outputsVec4[i].y;
              q1.z = sampler.m_outputsVec4[i].z;
              q1.w = sampler.m_outputsVec4[i].w;
              nvmath::quatf q2;
              q2.x = sampler.m_outputsVec4[i + 1].x;
              q2.y = sampler.m_outputsVec4[i + 1].y;
              q2.z = sampler.m_outputsVec4[i + 1].z;
              q2.w = sampler.m_outputsVec4[i + 1].w;

              channel.m_node->m_rotation = nvmath::normalize(nvmath::slerp(q1, q2, u));
              break;
            }
          }
          updated = true;
        }
      }
    }
  }
  if(updated)
  {
    for(auto& node : m_nodes)
    {
      //node->update();
    }
  }
}


//--------------------------------------------------------------------------------------------------
// Utility function
//
Node* Scene::nodeFromIndex(uint32_t index)
{
  for(auto& node : m_linearNodes)
  {
    if(node->m_index == index)
    {
      return node;
    }
  }
  return nullptr;
}

}  // namespace gltf
}  // namespace nvh
