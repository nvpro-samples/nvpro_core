/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#include <filesystem>

#include "gltfscene.hpp"
#include "boundingbox.hpp"
#include "nvprint.hpp"
#include <iostream>
#include <numeric>
#include <limits>
#include <set>
#include <sstream>
#include <thread>
#include "parallel_work.hpp"

#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "timesampler.hpp"


// Loading a GLTF file and extracting all information
bool nvh::gltf::Scene::load(const std::string& filename)
{
  nvh::ScopedTimer st(std::string(__FUNCTION__) + "\n");
  LOGI("%s%s\n", nvh::ScopedTimer::indent().c_str(), filename.c_str());

  m_filename = filename;
  m_model    = {};
  tinygltf::TinyGLTF tcontext;
  std::string        warn;
  std::string        error;
  tcontext.SetMaxExternalFileSize(1ULL << 33);  // 8GB
  auto ext = std::filesystem::path(filename).extension().string();
  bool result{false};
  if(ext == ".gltf")
  {
    result = tcontext.LoadASCIIFromFile(&m_model, &error, &warn, filename);
  }
  else if(ext == ".glb")
  {
    result = tcontext.LoadBinaryFromFile(&m_model, &error, &warn, filename);
  }
  else
  {
    LOGE("Unknown file extension: %s\n", ext.c_str());
    return false;
  }

  if(!result)
  {
    LOGE("Error loading file: %s\n", filename.c_str());
    LOGW("%s%s\n", st.indent().c_str(), warn.c_str());
    LOGE("%s%s\n", st.indent().c_str(), error.c_str());
    clearParsedData();
    //assert(!"Error while loading scene");
    return result;
  }

  m_currentScene   = m_model.defaultScene > -1 ? m_model.defaultScene : 0;
  m_currentVariant = 0;  // Default KHR_materials_variants
  parseScene();

  return result;
}

bool nvh::gltf::Scene::save(const std::string& filename)
{
  nvh::ScopedTimer st(std::string(__FUNCTION__) + "\n");

  std::string saveFilename = filename;

  // Make sure the extension is correct
  std::string ext = std::filesystem::path(filename).extension().string();
  if(ext != ".gltf")
  {
    // replace the extension
    saveFilename = std::filesystem::path(filename).replace_extension(".gltf").string();
  }

  // Copy the images to the destination folder
  if(!m_model.images.empty())
  {
    std::string srcPath   = std::filesystem::path(m_filename).parent_path().string();
    std::string dstPath   = std::filesystem::path(filename).parent_path().string();
    int         numCopied = 0;
    for(auto& image : m_model.images)
    {
      std::string uri_decoded;
      tinygltf::URIDecode(image.uri, &uri_decoded, nullptr);  // ex. whitespace may be represented as %20

      std::string srcFile = srcPath + "/" + uri_decoded;
      std::string dstFile = dstPath + "/" + uri_decoded;
      if(srcFile != dstFile)
      {
        if(std::filesystem::copy_file(srcFile, dstFile, std::filesystem::copy_options::update_existing))
          numCopied++;
      }
    }
    if(numCopied > 0)
      LOGI("%sImages copied: %d\n", st.indent().c_str(), numCopied);
  }

  // Save the glTF file
  tinygltf::TinyGLTF tcontext;
  bool               result = tcontext.WriteGltfSceneToFile(&m_model, saveFilename, false, false, true, false);
  LOGI("%sSaved: %s\n", st.indent().c_str(), saveFilename.c_str());
  return result;
}


void nvh::gltf::Scene::takeModel(tinygltf::Model&& model)
{
  m_model = std::move(model);
  parseScene();
}

void nvh::gltf::Scene::setCurrentScene(int sceneID)
{
  assert(sceneID >= 0 && sceneID < static_cast<int>(m_model.scenes.size()) && "Invalid scene ID");
  m_currentScene = sceneID;
  parseScene();
}

// Parses the scene from the glTF model, initializing and setting up scene elements, materials, animations, and the camera.
void nvh::gltf::Scene::parseScene()
{
  // Ensure there are nodes in the glTF model and the current scene ID is valid
  assert(m_model.nodes.size() > 0 && "No nodes in the glTF file");
  assert(m_currentScene >= 0 && m_currentScene < static_cast<int>(m_model.scenes.size()) && "Invalid scene ID");

  // Clear previous scene data and initialize scene elements
  clearParsedData();
  setSceneElementsDefaultNames();

  // Ensure only one top node per scene, creating a new node if necessary
  // This is done to be able to transform the entire scene as a single node
  for(auto& scene : m_model.scenes)
  {
    createRootIfMultipleNodes(scene);
  }
  m_sceneRootNode = m_model.scenes[m_currentScene].nodes[0];  // Set the root node of the scene

  // There must be at least one material in the scene
  if(m_model.materials.empty())
  {
    m_model.materials.emplace_back();
  }

  // Collect all draw objects; RenderNode and RenderPrimitive
  // Also it will be used  to compute the scene bounds for the camera
  for(auto& sceneNode : m_model.scenes[m_currentScene].nodes)
  {
    tinygltf::utils::traverseSceneGraph(m_model, sceneNode, glm::mat4(1), nullptr, nullptr,
                                        [this](int nodeID, const glm::mat4& worldMat) {
                                          return handleRenderNode(nodeID, worldMat);
                                        });
  }

  // Search for the first camera in the scene and exit traversal upon finding it
  for(auto& sceneNode : m_model.scenes[m_currentScene].nodes)
  {
    tinygltf::utils::traverseSceneGraph(
        m_model, sceneNode, glm::mat4(1),
        [&](int nodeID, glm::mat4 mat) {
          m_sceneCameraNode = nodeID;
          return true;  // Stop traversal
        },
        nullptr, nullptr);
  }

  // Create a default camera if none is found in the scene
  if(m_sceneCameraNode == -1)
  {
    createSceneCamera();
  }

  // Parse various scene components
  parseVariants();
  parseAnimations();
}

// Set the default names for the scene elements if they are empty
void nvh::gltf::Scene::setSceneElementsDefaultNames()
{
  auto setDefaultName = [](auto& elements, const std::string& prefix) {
    for(size_t i = 0; i < elements.size(); ++i)
    {
      if(elements[i].name.empty())
      {
        elements[i].name = fmt::format("{}-{}", prefix, i);
      }
    }
  };

  setDefaultName(m_model.scenes, "Scene");
  setDefaultName(m_model.meshes, "Mesh");
  setDefaultName(m_model.materials, "Material");
  setDefaultName(m_model.nodes, "Node");
  setDefaultName(m_model.cameras, "Camera");
  setDefaultName(m_model.lights, "Light");
}


// Creates a new root node for the scene and assigns existing top nodes as its children.
void nvh::gltf::Scene::createRootIfMultipleNodes(tinygltf::Scene& scene)
{
  // Already a single node in the scene
  if(scene.nodes.size() == 1)
    return;

  tinygltf::Node newNode;
  newNode.name = scene.name;
  newNode.children.swap(scene.nodes);  // Move the scene nodes to the new node
  m_model.nodes.push_back(newNode);    // Add to then to avoid invalidating any references
  scene.nodes.clear();                 // Should be already empty, due to the swap
  scene.nodes.push_back(int(m_model.nodes.size()) - 1);
}

// If there is no camera in the scene, we create one
// The camera is placed at the center of the scene, looking at the scene
void nvh::gltf::Scene::createSceneCamera()
{
  tinygltf::Camera& tcamera        = m_model.cameras.emplace_back();  // Add a camera
  int               newCameraIndex = static_cast<int>(m_model.cameras.size() - 1);
  tinygltf::Node&   tnode          = m_model.nodes.emplace_back();  // Add a node for the camera
  int               newNodeIndex   = static_cast<int>(m_model.nodes.size() - 1);
  tnode.name                       = "Camera";
  tnode.camera                     = newCameraIndex;
  int rootID                       = m_model.scenes[m_currentScene].nodes[0];
  m_model.nodes[rootID].children.push_back(newNodeIndex);  // Add the camera node to the root

  // Set the camera to look at the scene
  nvh::Bbox bbox   = getSceneBounds();
  glm::vec3 center = bbox.center();
  glm::vec3 eye = center + glm::vec3(0, 0, bbox.radius() * 2.414f);  //2.414 units away from the center of the sphere to fit it within a 45 - degree FOV
  glm::vec3 up                    = glm::vec3(0, 1, 0);
  tcamera.type                    = "perspective";
  tcamera.name                    = "Camera";
  tcamera.perspective.aspectRatio = 16.0f / 9.0f;
  tcamera.perspective.yfov        = glm::radians(45.0f);
  tcamera.perspective.zfar        = bbox.radius() * 10.0f;
  tcamera.perspective.znear       = bbox.radius() * 0.1f;

  // Add extra information to the node/camera
  tinygltf::Value::Object extras;
  extras["camera::eye"]    = tinygltf::utils::convertToTinygltfValue(3, glm::value_ptr(eye));
  extras["camera::center"] = tinygltf::utils::convertToTinygltfValue(3, glm::value_ptr(center));
  extras["camera::up"]     = tinygltf::utils::convertToTinygltfValue(3, glm::value_ptr(up));
  tnode.extras             = tinygltf::Value(extras);

  // Set the node transformation
  tnode.translation = {eye.x, eye.y, eye.z};
  glm::quat q       = glm::quatLookAt(glm::normalize(center - eye), up);
  tnode.rotation    = {q.x, q.y, q.z, q.w};
}

// This function will update the matrices and the materials of the render nodes
void nvh::gltf::Scene::updateRenderNodes()
{
  const tinygltf::Scene& scene = m_model.scenes[m_currentScene];
  assert(scene.nodes.size() > 0 && "No nodes in the glTF file");
  //assert(scene.nodes.size() == 1 && "Only one top node per scene is supported");
  assert(m_sceneRootNode > -1 && "No root node in the scene");

  uint32_t renderNodeID = 0;  // Index of the render node
  for(auto& sceneNode : scene.nodes)
  {
    tinygltf::utils::traverseSceneGraph(m_model, sceneNode, glm::mat4(1), nullptr, nullptr, [&](int nodeID, const glm::mat4& mat) {
      tinygltf::Node&       tnode = m_model.nodes[nodeID];
      const tinygltf::Mesh& mesh  = m_model.meshes[tnode.mesh];
      for(size_t j = 0; j < mesh.primitives.size(); j++)
      {
        const tinygltf::Primitive& primitive  = mesh.primitives[j];
        gltf::RenderNode&          renderNode = m_renderNodes[renderNodeID];
        renderNode.worldMatrix                = mat;
        renderNode.materialID                 = getMaterialVariantIndex(primitive, m_currentVariant);
        renderNodeID++;
      }
      return false;  // Continue traversal
    });
  }
}

void nvh::gltf::Scene::setCurrentVariant(int variant)
{
  m_currentVariant = variant;
  // Updating the render nodes with the new material variant
  updateRenderNodes();
}


void nvh::gltf::Scene::clearParsedData()
{
  m_cameras.clear();
  m_lights.clear();
  m_animations.clear();
  m_renderNodes.clear();
  m_renderPrimitives.clear();
  m_uniquePrimitiveIndex.clear();
  m_variants.clear();
  m_numTriangles    = 0;
  m_sceneBounds     = {};
  m_sceneCameraNode = -1;
  m_sceneRootNode   = -1;
}

void nvh::gltf::Scene::destroy()
{
  clearParsedData();
  m_filename = {};
  m_model    = {};
}


// Get the unique index of a primitive, and add it to the list if it is not already there
int nvh::gltf::Scene::getUniqueRenderPrimitive(const tinygltf::Primitive& primitive)
{
  const std::string& key = tinygltf::utils::generatePrimitiveKey(primitive);

  // Attempt to insert the key with the next available index if it doesn't exist
  auto [it, inserted] = m_uniquePrimitiveIndex.try_emplace(key, static_cast<int>(m_uniquePrimitiveIndex.size()));

  // If the primitive was newly inserted, add it to the render primitives list
  if(inserted)
  {
    gltf::RenderPrimitive renderPrim;
    renderPrim.primitive   = primitive;
    renderPrim.vertexCount = int(tinygltf::utils::getVertexCount(m_model, primitive));
    renderPrim.indexCount  = int(tinygltf::utils::getIndexCount(m_model, primitive));
    m_renderPrimitives.push_back(renderPrim);
  }

  return it->second;
}


// Function to extract eye, center, and up vectors from a view matrix
inline void extractCameraVectors(const glm::mat4& viewMatrix, const glm::vec3& sceneCenter, glm::vec3& eye, glm::vec3& center, glm::vec3& up)
{
  eye                    = glm::vec3(viewMatrix[3]);
  glm::mat3 rotationPart = glm::mat3(viewMatrix);
  glm::vec3 forward      = -rotationPart * glm::vec3(0.0f, 0.0f, 1.0f);

  // Project sceneCenter onto the forward vector
  glm::vec3 eyeToSceneCenter = sceneCenter - eye;
  float     projectionLength = std::abs(glm::dot(eyeToSceneCenter, forward));
  center                     = eye + projectionLength * forward;

  up = glm::vec3(0.0f, 1.0f, 0.0f);  // Assume the up vector is always (0, 1, 0)
}


// Retrieve the list of render cameras in the scene.
// This function returns a vector of render cameras present in the scene. If the `force`
// parameter is set to true, it clears and regenerates the list of cameras.
//
// Parameters:
// - force: If true, forces the regeneration of the camera list.
//
// Returns:
// - A const reference to the vector of render cameras.
const std::vector<nvh::gltf::RenderCamera>& nvh::gltf::Scene::getRenderCameras(bool force /*= false*/)
{
  if(force)
  {
    m_cameras.clear();
  }

  if(m_cameras.empty())
  {
    assert(m_sceneRootNode > -1 && "No root node in the scene");
    tinygltf::utils::traverseSceneGraph(m_model, m_sceneRootNode, glm::mat4(1), [&](int nodeID, const glm::mat4& worldMatrix) {
      return handleCameraTraversal(nodeID, worldMatrix);
    });
  }
  return m_cameras;
}


bool nvh::gltf::Scene::handleCameraTraversal(int nodeID, const glm::mat4& worldMatrix)
{
  tinygltf::Node& node = m_model.nodes[nodeID];
  m_sceneCameraNode    = nodeID;

  tinygltf::Camera&  tcam = m_model.cameras[node.camera];
  gltf::RenderCamera camera;
  if(tcam.type == "perspective")
  {
    camera.type  = gltf::RenderCamera::CameraType::ePerspective;
    camera.znear = tcam.perspective.znear;
    camera.zfar  = tcam.perspective.zfar;
    camera.yfov  = tcam.perspective.yfov;
  }
  else
  {
    camera.type  = gltf::RenderCamera::CameraType::eOrthographic;
    camera.znear = tcam.orthographic.znear;
    camera.zfar  = tcam.orthographic.zfar;
    camera.xmag  = tcam.orthographic.xmag;
    camera.ymag  = tcam.orthographic.ymag;
  }

  nvh::Bbox bbox = getSceneBounds();
  // From the view matrix, we extract the eye, center, and up vectors
  extractCameraVectors(worldMatrix, bbox.center(), camera.eye, camera.center, camera.up);

  // If the node/camera has extras, we extract the eye, center, and up vectors from the extras
  auto& extras = node.extras;
  if(extras.IsObject())
  {
    tinygltf::utils::getArrayValue(extras, "camera::eye", camera.eye);
    tinygltf::utils::getArrayValue(extras, "camera::center", camera.center);
    tinygltf::utils::getArrayValue(extras, "camera::up", camera.up);
  }

  m_cameras.push_back(camera);
  return false;
}

// Retrieve the list of render lights in the scene.
// This function returns a vector of render lights present in the scene. If the `force`
// parameter is set to true, it clears and regenerates the list of lights.
//
// Parameters:
// - force: If true, forces the regeneration of the light list.
//
// Returns:
// - A const reference to the vector of render lights.
const std::vector<nvh::gltf::RenderLight>& nvh::gltf::Scene::getRenderLights(bool force)
{
  if(force)
  {
    m_lights.clear();
  }

  if(m_lights.empty())
  {
    assert(m_sceneRootNode > -1 && "No root node in the scene");
    tinygltf::utils::traverseSceneGraph(m_model, m_sceneRootNode, glm::mat4(1), nullptr, [&](int nodeID, const glm::mat4& worldMatrix) {
      tinygltf::Node&   node = m_model.nodes[nodeID];
      gltf::RenderLight renderLight;
      renderLight.light       = m_model.lights[node.light];
      renderLight.worldMatrix = worldMatrix;
      m_lights.push_back(renderLight);
      return false;  // Continue traversal
    });
  }

  return m_lights;
}


// Return the bounding volume of the scene
nvh::Bbox nvh::gltf::Scene::getSceneBounds()
{
  if(!m_sceneBounds.isEmpty())
    return m_sceneBounds;

  for(const gltf::RenderNode& rnode : m_renderNodes)
  {
    glm::vec3 minValues = {0.f, 0.f, 0.f};
    glm::vec3 maxValues = {0.f, 0.f, 0.f};

    const gltf::RenderPrimitive& rprim    = m_renderPrimitives[rnode.renderPrimID];
    const tinygltf::Accessor&    accessor = m_model.accessors[rprim.primitive.attributes.at("POSITION")];
    if(!accessor.minValues.empty())
      minValues = glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]);
    if(!accessor.maxValues.empty())
      maxValues = glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]);
    nvh::Bbox bbox(minValues, maxValues);
    bbox = bbox.transform(rnode.worldMatrix);
    m_sceneBounds.insert(bbox);
  }

  if(m_sceneBounds.isEmpty() || !m_sceneBounds.isVolume())
  {
    LOGE("glTF: Scene bounding box invalid, Setting to: [-1,-1,-1], [1,1,1]");
    m_sceneBounds.insert({-1.0f, -1.0f, -1.0f});
    m_sceneBounds.insert({1.0f, 1.0f, 1.0f});
  }

  return m_sceneBounds;
}

// Handles the creation of render nodes for a given primitive in the scene.
// For each primitive in the node's mesh, it:
// - Generates a unique render primitive index.
// - Creates a render node with the appropriate world matrix, material ID, render primitive ID, primitive ID, and reference node ID.
// If the primitive has the EXT_mesh_gpu_instancing extension, multiple render nodes are created for instancing.
// Otherwise, a single render node is added to the render nodes list.
// Returns false to continue traversal of the scene graph.
bool nvh::gltf::Scene::handleRenderNode(int nodeID, glm::mat4 worldMatrix)
{
  const tinygltf::Node& node = m_model.nodes[nodeID];
  const tinygltf::Mesh& mesh = m_model.meshes[node.mesh];
  for(size_t primID = 0; primID < mesh.primitives.size(); primID++)
  {
    const tinygltf::Primitive& primitive    = mesh.primitives[primID];
    int                        rprimID      = getUniqueRenderPrimitive(primitive);
    int                        numTriangles = m_renderPrimitives[rprimID].indexCount / 3;

    nvh::gltf::RenderNode renderNode;
    renderNode.worldMatrix  = worldMatrix;
    renderNode.materialID   = getMaterialVariantIndex(primitive, m_currentVariant);
    renderNode.renderPrimID = rprimID;
    renderNode.primID       = static_cast<int>(primID);
    renderNode.refNodeID    = nodeID;

    if(tinygltf::utils::hasElementName(node.extensions, EXT_MESH_GPU_INSTANCING_EXTENSION_NAME))
    {
      const tinygltf::Value& ext = tinygltf::utils::getElementValue(node.extensions, EXT_MESH_GPU_INSTANCING_EXTENSION_NAME);
      const tinygltf::Value& attributes   = ext.Get("attributes");
      size_t                 numInstances = handleGpuInstancing(attributes, renderNode, worldMatrix);
      m_numTriangles += numTriangles * static_cast<uint32_t>(numInstances);  // Statistics
    }
    else
    {
      m_renderNodes.push_back(renderNode);
      m_numTriangles += numTriangles;  // Statistics
    }
  }
  return false;  // Continue traversal
}

// Handle GPU instancing : EXT_mesh_gpu_instancing
size_t nvh::gltf::Scene::handleGpuInstancing(const tinygltf::Value& attributes, gltf::RenderNode renderNode, glm::mat4 worldMatrix)
{
  std::vector<glm::vec3> translations = tinygltf::utils::extractAttributeData<glm::vec3>(m_model, attributes, "TRANSLATION");
  std::vector<glm::quat> rotations = tinygltf::utils::extractAttributeData<glm::quat>(m_model, attributes, "ROTATION");
  std::vector<glm::vec3> scales    = tinygltf::utils::extractAttributeData<glm::vec3>(m_model, attributes, "SCALE");
  size_t                 numInstances = std::max({translations.size(), rotations.size(), scales.size()});

  // Note: the specification says, that the number of elements in the attributes should be the same if they are present
  for(size_t i = 0; i < numInstances; i++)
  {
    gltf::RenderNode instNode    = renderNode;
    glm::vec3        translation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat        rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3        scale       = glm::vec3(1.0f, 1.0f, 1.0f);
    if(!translations.empty())
      translation = translations[i];
    if(!rotations.empty())
      rotation = rotations[i];
    if(!scales.empty())
      scale = scales[i];

    glm::mat4 mat = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);

    instNode.worldMatrix = worldMatrix * mat;
    m_renderNodes.push_back(instNode);
  }
  return numInstances;
}

//-------------------------------------------------------------------------------------------------
// Find which nodes are solid or translucent, helps for raster rendering
//
std::vector<uint32_t> nvh::gltf::Scene::getShadedNodes(PipelineType type)
{
  std::vector<uint32_t> result;

  for(uint32_t i = 0; i < m_renderNodes.size(); i++)
  {
    const auto& tmat               = m_model.materials[m_renderNodes[i].materialID];
    float       transmissionFactor = 0;
    if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME))
    {
      const auto& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME);
      tinygltf::utils::getValue(ext, "transmissionFactor", transmissionFactor);
    }
    switch(type)
    {
      case eRasterSolid:
        if(tmat.alphaMode == "OPAQUE" && !tmat.doubleSided && (transmissionFactor == 0.0F))
          result.push_back(i);
        break;
      case eRasterSolidDoubleSided:
        if(tmat.alphaMode == "OPAQUE" && tmat.doubleSided)
          result.push_back(i);
        break;
      case eRasterBlend:
        if(tmat.alphaMode != "OPAQUE" || (transmissionFactor != 0))
          result.push_back(i);
        break;
      case eRasterAll:
        result.push_back(i);
        break;
    }
  }
  return result;
}

tinygltf::Node nvh::gltf::Scene::getSceneRootNode() const
{
  const tinygltf::Scene& scene = m_model.scenes[m_currentScene];
  assert(scene.nodes.size() == 1 && "There should be exactly one node under the scene.");
  const tinygltf::Node& node = m_model.nodes[scene.nodes[0]];  // Root node

  return node;
}

void nvh::gltf::Scene::setSceneRootNode(const tinygltf::Node& node)
{
  const tinygltf::Scene& scene = m_model.scenes[m_currentScene];
  assert(scene.nodes.size() == 1 && "There should be exactly one node under the scene.");
  tinygltf::Node& rootNode = m_model.nodes[scene.nodes[0]];  // Root node
  rootNode                 = node;

  updateRenderNodes();
}

void nvh::gltf::Scene::setSceneCamera(const nvh::gltf::RenderCamera& camera)
{
  assert(m_sceneCameraNode != -1 && "No camera node found in the scene");

  // Set the tinygltf::Node
  tinygltf::Node& tnode = m_model.nodes[m_sceneCameraNode];
  glm::quat       q     = glm::quatLookAt(glm::normalize(camera.center - camera.eye), camera.up);
  tnode.translation     = {camera.eye.x, camera.eye.y, camera.eye.z};
  tnode.rotation        = {q.x, q.y, q.z, q.w};

  // Set the tinygltf::Camera
  tinygltf::Camera& tcamera = m_model.cameras[tnode.camera];
  tcamera.type              = "perspective";
  tcamera.perspective.znear = camera.znear;
  tcamera.perspective.zfar  = camera.zfar;
  tcamera.perspective.yfov  = camera.yfov;

  // Add extras to the tinygltf::Camera, to store the eye, center, and up vectors
  tinygltf::Value::Object extras;
  extras["camera::eye"]    = tinygltf::utils::convertToTinygltfValue(3, glm::value_ptr(camera.eye));
  extras["camera::center"] = tinygltf::utils::convertToTinygltfValue(3, glm::value_ptr(camera.center));
  extras["camera::up"]     = tinygltf::utils::convertToTinygltfValue(3, glm::value_ptr(camera.up));
  tnode.extras             = tinygltf::Value(extras);
}

// Collects all animation data
void nvh::gltf::Scene::parseAnimations()
{
  m_animations.clear();
  m_animations.reserve(m_model.animations.size());
  for(tinygltf::Animation& anim : m_model.animations)
  {
    Animation animation;

    // Samplers
    for(auto& samp : anim.samplers)
    {
      AnimationSampler sampler;

      if(samp.interpolation == "LINEAR")
      {
        sampler.interpolation = AnimationSampler::InterpolationType::eLinear;
      }
      if(samp.interpolation == "STEP")
      {
        sampler.interpolation = AnimationSampler::InterpolationType::eStep;
      }
      if(samp.interpolation == "CUBICSPLINE")
      {
        sampler.interpolation = AnimationSampler::InterpolationType::eCubicSpline;
      }

      // Read sampler input time values
      {
        const tinygltf::Accessor&   accessor   = m_model.accessors[samp.input];
        const tinygltf::BufferView& bufferView = m_model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer&     buffer     = m_model.buffers[bufferView.buffer];

        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        sampler.inputs.resize(accessor.count);
        std::memcpy(sampler.inputs.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                    accessor.count * sizeof(float));

        // Protect against invalid values
        for(auto input : sampler.inputs)
        {
          if(input < animation.start)
          {
            animation.start = input;
          }
          if(input > animation.end)
          {
            animation.end = input;
          }
        }
      }

      // Read sampler output T/R/S values
      {
        const tinygltf::Accessor&   accessor   = m_model.accessors[samp.output];
        const tinygltf::BufferView& bufferView = m_model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer&     buffer     = m_model.buffers[bufferView.buffer];
        const uint8_t*              dataPtr8   = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        switch(accessor.type)
        {
          case TINYGLTF_TYPE_VEC3: {
            const glm::vec3* dataPtr = reinterpret_cast<const glm::vec3*>(dataPtr8);
            for(size_t index = 0; index < accessor.count; index++)
            {
              sampler.outputsVec4.push_back(glm::vec4(*dataPtr++, 0.0f));
            }
            break;
          }
          case TINYGLTF_TYPE_VEC4: {
            const glm::vec4* dataPtr = reinterpret_cast<const glm::vec4*>(dataPtr8);
            for(size_t index = 0; index < accessor.count; index++)
            {
              sampler.outputsVec4.push_back(*dataPtr++);
            }
            break;
          }
          case TINYGLTF_TYPE_SCALAR: {
            sampler.outputsFloat.resize(sampler.inputs.size());
            size_t       elemPerKey = accessor.count / sampler.inputs.size();
            const float* dataPtr    = reinterpret_cast<const float*>(dataPtr8);
            for(size_t i = 0; i < sampler.inputs.size(); i++)
            {
              for(int j = 0; j < elemPerKey; j++)
              {
                sampler.outputsFloat[i].push_back(*dataPtr++);
              }
            }
            break;
          }
          default: {
            LOGE("unknown type");
            break;
          }
        }
      }

      animation.samplers.push_back(sampler);
    }

    // Channels
    for(auto& source : anim.channels)
    {
      AnimationChannel channel;

      if(source.target_path == "rotation")
      {
        channel.path = AnimationChannel::PathType::eRotation;
      }
      if(source.target_path == "translation")
      {
        channel.path = AnimationChannel::PathType::eTranslation;
      }
      if(source.target_path == "scale")
      {
        channel.path = AnimationChannel::PathType::eScale;
      }
      if(source.target_path == "weights")
      {
        channel.path = AnimationChannel::PathType::eWeights;
      }
      channel.samplerIndex = source.sampler;
      channel.node         = source.target_node;

      animation.channels.push_back(channel);
    }

    m_animations.push_back(animation);
  }
}

bool nvh::gltf::Scene::updateAnimations(float deltaTime)
{
  bool animated = false;
  for(size_t i = 0; i < m_animations.size(); i++)
  {
    animated |= updateAnimation(uint32_t(i), deltaTime, false);
  }
  if(animated)
    updateRenderNodes();
  return animated;
}


bool nvh::gltf::Scene::resetAnimations()
{
  bool animated = false;
  for(size_t i = 0; i < m_animations.size(); i++)
  {
    animated |= updateAnimation(uint32_t(i), 0, true);
  }
  if(animated)
    updateRenderNodes();
  return animated;
}


// Update a specific animation
bool nvh::gltf::Scene::updateAnimation(uint32_t animationIndex, float deltaTime, bool reset)
{
  bool animated = false;

  Animation& animation = m_animations[animationIndex];

  animation.currentTime += deltaTime;
  if(animation.currentTime > animation.end)
  {
    animation.currentTime -= animation.end;
  }

  if(reset)
  {
    animation.currentTime = animation.start;
  }

  float time = animation.currentTime;

  for(auto& channel : animation.channels)
  {
    tinygltf::Node&   tnode   = m_model.nodes[channel.node];
    AnimationSampler& sampler = animation.samplers[channel.samplerIndex];

    for(auto i = 0; i < int(sampler.inputs.size()) - 1; i++)
    {
      // With reset, use the first keyframe
      if(reset)
        time = sampler.inputs[0];

      if(sampler.inputs[i] <= time && time <= sampler.inputs[i + 1])
      {
        float keyDelta = sampler.inputs[i + 1] - sampler.inputs[i];
        float t        = (time - sampler.inputs[i]) / keyDelta;
        //t       = std::clamp(t, 0.0f, 1.0f);

        animated = true;

        if(sampler.interpolation == AnimationSampler::InterpolationType::eLinear)
        {
          switch(channel.path)
          {
            case AnimationChannel::PathType::eRotation: {
              const glm::quat q1 = glm::make_quat(glm::value_ptr(sampler.outputsVec4[i]));
              const glm::quat q2 = glm::make_quat(glm::value_ptr(sampler.outputsVec4[i + 1]));
              glm::quat       q  = glm::normalize(glm::slerp(q1, q2, t));
              tnode.rotation     = {q.x, q.y, q.z, q.w};
              break;
            }
            case AnimationChannel::PathType::eTranslation: {
              glm::vec3 trans   = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], t);
              tnode.translation = {trans.x, trans.y, trans.z};
              break;
            }
            case AnimationChannel::PathType::eScale: {
              glm::vec3 s = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], t);
              tnode.scale = {s.x, s.y, s.z};
              break;
            }
            case AnimationChannel::PathType::eWeights: {
              {
                static bool onceFlag = true;
                if(onceFlag)
                {
                  LOGE("AnimationChannel::PathType::WEIGHTS not implemented");
                  onceFlag = false;
                }
                break;
              }
            }
          }
        }
        else if(sampler.interpolation == AnimationSampler::InterpolationType::eStep)
        {
          switch(channel.path)
          {
            case AnimationChannel::PathType::eRotation: {
              glm::quat q    = glm::quat(sampler.outputsVec4[i]);
              tnode.rotation = {q.x, q.y, q.z, q.w};
              break;
            }
            case AnimationChannel::PathType::eTranslation: {
              glm::vec3 t       = glm::vec3(sampler.outputsVec4[i]);
              tnode.translation = {t.x, t.y, t.z};
              break;
            }
            case AnimationChannel::PathType::eScale: {
              glm::vec3 s = glm::vec3(sampler.outputsVec4[i]);
              tnode.scale = {s.x, s.y, s.z};
              break;
            }
          }
        }
        else if(sampler.interpolation == AnimationSampler::InterpolationType::eCubicSpline)
        {
          int prevIndex = i * 3;
          int nextIndex = (i + 1) * 3;
          int A         = 0;
          int V         = 1;
          int B         = 2;

          float tSq = t * t;
          float tCb = tSq * t;
          float tD  = keyDelta;

          const glm::vec4 v0 = sampler.outputsVec4[prevIndex + V];
          const glm::vec4 a  = sampler.outputsVec4[nextIndex + A];
          const glm::vec4 b  = sampler.outputsVec4[prevIndex + B];
          const glm::vec4 v1 = sampler.outputsVec4[nextIndex + V];

          // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-cubic
          glm::vec4 result = ((2 * tCb - 3 * tSq + 1) * v0) + (tD * (tCb - 2 * tSq + t) * b)
                             + ((-2 * tCb + 3 * tSq) * v1) + (tD * (tCb - tSq) * a);

          switch(channel.path)
          {
            case AnimationChannel::PathType::eRotation: {
              glm::quat quatResult = glm::make_quat(glm::value_ptr(result));
              quatResult           = glm::normalize(quatResult);
              tnode.rotation       = {quatResult.x, quatResult.y, quatResult.z, quatResult.w};
              break;
            }
            case AnimationChannel::PathType::eTranslation: {
              tnode.translation = {result.x, result.y, result.z};
              break;
            }
            case AnimationChannel::PathType::eScale: {
              tnode.scale = {result.x, result.y, result.z};
              break;
            }
          }
        }
      }
    }
  }

  return animated;
}

// Parse the variants of the materials
void nvh::gltf::Scene::parseVariants()
{
  if(m_model.extensions.find(KHR_MATERIALS_VARIANTS_EXTENSION_NAME) != m_model.extensions.end())
  {
    const auto& ext = m_model.extensions.find(KHR_MATERIALS_VARIANTS_EXTENSION_NAME)->second;
    if(ext.Has("variants"))
    {
      auto& variants = ext.Get("variants");
      for(size_t i = 0; i < variants.ArrayLen(); i++)
      {
        std::string name = variants.Get(int(i)).Get("name").Get<std::string>();
        m_variants.push_back(name);
      }
    }
  }
}

// Return the material index based on the variant, or the material set on the primitive
int nvh::gltf::Scene::getMaterialVariantIndex(const tinygltf::Primitive& primitive, int currentVariant)
{
  if(primitive.extensions.find(KHR_MATERIALS_VARIANTS_EXTENSION_NAME) != primitive.extensions.end())
  {
    const auto& ext     = primitive.extensions.find(KHR_MATERIALS_VARIANTS_EXTENSION_NAME)->second;
    auto&       mapping = ext.Get("mappings");
    for(auto& map : mapping.Get<tinygltf::Value::Array>())
    {
      auto& variants   = map.Get("variants");
      int   materialID = map.Get("material").Get<int>();
      for(auto& variant : variants.Get<tinygltf::Value::Array>())
      {
        int variantID = variant.Get<int>();
        if(variantID == currentVariant)
          return materialID;
      }
    }
  }

  return std::max(0, primitive.material);
}


// ***********************************************************************************************************************
// DEPRICATED SECTION
// ***********************************************************************************************************************


//--------------------------------------------------------------------------------------------------
// Collect the value of all materials
//
void nvh::GltfScene::importMaterials(const tinygltf::Model& tmodel)
{
  m_materials.reserve(tmodel.materials.size());

  for(auto& tmat : tmodel.materials)
  {
    GltfMaterial gmat;
    gmat.tmaterial = &tmat;  // Reference

    gmat.alphaCutoff              = static_cast<float>(tmat.alphaCutoff);
    gmat.alphaMode                = tmat.alphaMode == "MASK" ? 1 : (tmat.alphaMode == "BLEND" ? 2 : 0);
    gmat.doubleSided              = tmat.doubleSided ? 1 : 0;
    gmat.emissiveFactor           = tmat.emissiveFactor.size() == 3 ?
                                        glm::vec3(tmat.emissiveFactor[0], tmat.emissiveFactor[1], tmat.emissiveFactor[2]) :
                                        glm::vec3(0.f);
    gmat.emissiveTexture          = tmat.emissiveTexture.index;
    gmat.normalTexture            = tmat.normalTexture.index;
    gmat.normalTextureScale       = static_cast<float>(tmat.normalTexture.scale);
    gmat.occlusionTexture         = tmat.occlusionTexture.index;
    gmat.occlusionTextureStrength = static_cast<float>(tmat.occlusionTexture.strength);

    // PbrMetallicRoughness
    auto& tpbr = tmat.pbrMetallicRoughness;
    gmat.baseColorFactor =
        glm::vec4(tpbr.baseColorFactor[0], tpbr.baseColorFactor[1], tpbr.baseColorFactor[2], tpbr.baseColorFactor[3]);
    gmat.baseColorTexture         = tpbr.baseColorTexture.index;
    gmat.metallicFactor           = static_cast<float>(tpbr.metallicFactor);
    gmat.metallicRoughnessTexture = tpbr.metallicRoughnessTexture.index;
    gmat.roughnessFactor          = static_cast<float>(tpbr.roughnessFactor);

    // Extensions
    gmat.specular         = tinygltf::utils::getSpecular(tmat);
    gmat.textureTransform = tinygltf::utils::getTextureTransform(tpbr.baseColorTexture);
    gmat.unlit            = tinygltf::utils::getUnlit(tmat);
    gmat.anisotropy       = tinygltf::utils::getAnisotropy(tmat);
    gmat.clearcoat        = tinygltf::utils::getClearcoat(tmat);
    gmat.sheen            = tinygltf::utils::getSheen(tmat);
    gmat.transmission     = tinygltf::utils::getTransmission(tmat);
    gmat.ior              = tinygltf::utils::getIor(tmat);
    gmat.volume           = tinygltf::utils::getVolume(tmat);
    gmat.displacement     = tinygltf::utils::getDisplacement(tmat);
    gmat.emissiveStrength = tinygltf::utils::getEmissiveStrength(tmat);
    gmat.emissiveFactor *= gmat.emissiveStrength.emissiveStrength;

    m_materials.emplace_back(gmat);
  }

  // Make default
  if(m_materials.empty())
  {
    GltfMaterial gmat;
    gmat.metallicFactor = 0;
    m_materials.emplace_back(gmat);
  }
}

//--------------------------------------------------------------------------------------------------
// Linearize the scene graph to world space nodes.
//
void nvh::GltfScene::importDrawableNodes(const tinygltf::Model& tmodel,
                                         GltfAttributes         requestedAttributes,
                                         GltfAttributes         forceRequested /*= GltfAttributes::All*/)
{
  checkRequiredExtensions(tmodel);

  int         defaultScene = tmodel.defaultScene > -1 ? tmodel.defaultScene : 0;
  const auto& tscene       = tmodel.scenes[defaultScene];

  // Finding only the mesh that are used in the scene
  std::set<uint32_t> usedMeshes;
  for(auto nodeIdx : tscene.nodes)
  {
    findUsedMeshes(tmodel, usedMeshes, nodeIdx);
  }

  // Find the number of vertex(attributes) and index
  //uint32_t nbVert{0};
  uint32_t nbIndex{0};
  uint32_t primCnt{0};  //  "   "  "  "
  for(const auto& m : usedMeshes)
  {
    auto&                 tmesh = tmodel.meshes[m];
    std::vector<uint32_t> vprim;
    for(const auto& primitive : tmesh.primitives)
    {
      if(primitive.mode != 4)  // Triangle
        continue;
      const auto& posAccessor = tmodel.accessors[primitive.attributes.find("POSITION")->second];
      //nbVert += static_cast<uint32_t>(posAccessor.count);
      if(primitive.indices > -1)
      {
        const auto& indexAccessor = tmodel.accessors[primitive.indices];
        nbIndex += static_cast<uint32_t>(indexAccessor.count);
      }
      else
      {
        nbIndex += static_cast<uint32_t>(posAccessor.count);
      }
      vprim.emplace_back(primCnt++);
    }
    m_meshToPrimMeshes[m] = std::move(vprim);  // mesh-id = { prim0, prim1, ... }
  }

  // Reserving memory
  m_indices.reserve(nbIndex);

  // Convert all mesh/primitives+ to a single primitive per mesh
  for(const auto& m : usedMeshes)
  {
    auto& tmesh = tmodel.meshes[m];
    for(const auto& tprimitive : tmesh.primitives)
    {
      processMesh(tmodel, tprimitive, requestedAttributes, forceRequested, tmesh.name);
      if(!m_primMeshes.empty())
      {
        m_primMeshes.back().tmesh = &tmesh;
        m_primMeshes.back().tprim = &tprimitive;
      }
    }
  }

  // Fixing tangents, if any were null
  uint32_t num_threads = std::min((uint32_t)m_tangents.size(), std::thread::hardware_concurrency());
  nvh::parallel_batches(
      m_tangents.size(),
      [&](uint64_t i) {
        auto& t = m_tangents[i];
        if(glm::length2(glm::vec3(t)) < 0.01F || std::abs(t.w) < 0.5F)
        {
          const auto& n   = m_normals[i];
          const float sgn = n.z > 0.0F ? 1.0F : -1.0F;
          const float a   = -1.0F / (sgn + n.z);
          const float b   = n.x * n.y * a;
          t               = glm::vec4(1.0f + sgn * n.x * n.x * a, sgn * b, -sgn * n.x, sgn);
        }
      },
      num_threads);

  // Transforming the scene hierarchy to a flat list
  for(auto nodeIdx : tscene.nodes)
  {
    processNode(tmodel, nodeIdx, glm::mat4(1));
  }

  computeSceneDimensions();
  computeCamera();

  m_meshToPrimMeshes.clear();
  primitiveIndices32u.clear();
  primitiveIndices16u.clear();
  primitiveIndices8u.clear();
}

//--------------------------------------------------------------------------------------------------
// Return the matrix of the node
//
static glm::mat4 getLocalMatrix(const tinygltf::Node& tnode)
{
  glm::mat4 mtranslation{1};
  glm::mat4 mscale{1};
  glm::mat4 mrot{1};
  glm::mat4 matrix{1};
  glm::quat mrotation;

  if(!tnode.translation.empty())
    mtranslation = glm::translate(glm::mat4(1), glm::vec3(tnode.translation[0], tnode.translation[1], tnode.translation[2]));
  if(!tnode.scale.empty())
    mscale = glm::scale(glm::mat4(1), glm::vec3(tnode.scale[0], tnode.scale[1], tnode.scale[2]));
  if(!tnode.rotation.empty())
  {
    mrotation = glm::make_quat(tnode.rotation.data());
    mrot      = glm::mat4_cast(mrotation);
  }
  if(!tnode.matrix.empty())
  {
    matrix = glm::make_mat4(tnode.matrix.data());
  }
  return mtranslation * mrot * mscale * matrix;
}


//--------------------------------------------------------------------------------------------------
//
//
void nvh::GltfScene::processNode(const tinygltf::Model& tmodel, int& nodeIdx, const glm::mat4& parentMatrix)
{
  const auto& tnode = tmodel.nodes[nodeIdx];

  glm::mat4 matrix      = getLocalMatrix(tnode);
  glm::mat4 worldMatrix = parentMatrix * matrix;

  if(tnode.mesh > -1)
  {
    const auto& meshes = m_meshToPrimMeshes[tnode.mesh];  // A mesh could have many primitives
    for(const auto& mesh : meshes)
    {
      GltfNode node;
      node.primMesh    = mesh;
      node.worldMatrix = worldMatrix;
      node.tnode       = &tnode;
      m_nodes.emplace_back(node);
    }
  }
  else if(tnode.camera > -1)
  {
    GltfCamera camera;
    camera.worldMatrix = worldMatrix;
    camera.cam         = tmodel.cameras[tmodel.nodes[nodeIdx].camera];

    // If the node has the Iray extension, extract the camera information.
    if(tinygltf::utils::hasElementName(tnode.extensions, EXTENSION_ATTRIB_IRAY))
    {
      auto& iray_ext   = tnode.extensions.at(EXTENSION_ATTRIB_IRAY);
      auto& attributes = iray_ext.Get("attributes");
      tinygltf::utils::getArrayValue(attributes, "iview:position", camera.eye);
      tinygltf::utils::getArrayValue(attributes, "iview:interest", camera.center);
      tinygltf::utils::getArrayValue(attributes, "iview:up", camera.up);
    }

    m_cameras.emplace_back(camera);
  }
  else if(tnode.extensions.find(KHR_LIGHTS_PUNCTUAL_EXTENSION_NAME) != tnode.extensions.end())
  {
    GltfLight   light;
    const auto& ext      = tnode.extensions.find(KHR_LIGHTS_PUNCTUAL_EXTENSION_NAME)->second;
    auto        lightIdx = ext.Get("light").GetNumberAsInt();
    light.light          = tmodel.lights[lightIdx];
    light.worldMatrix    = worldMatrix;
    m_lights.emplace_back(light);
  }

  // Recursion for all children
  for(auto child : tnode.children)
  {
    processNode(tmodel, child, worldMatrix);
  }
}

//--------------------------------------------------------------------------------------------------
// Extracting the values to a linear buffer
//
void nvh::GltfScene::processMesh(const tinygltf::Model&     tmodel,
                                 const tinygltf::Primitive& tmesh,
                                 GltfAttributes             requestedAttributes,
                                 GltfAttributes             forceRequested,
                                 const std::string&         name)
{
  // Only triangles are supported
  // 0:point, 1:lines, 2:line_loop, 3:line_strip, 4:triangles, 5:triangle_strip, 6:triangle_fan
  if(tmesh.mode != 4)
    return;

  GltfPrimMesh resultMesh;
  resultMesh.name          = name;
  resultMesh.materialIndex = std::max(0, tmesh.material);
  resultMesh.vertexOffset  = static_cast<uint32_t>(m_positions.size());
  resultMesh.firstIndex    = static_cast<uint32_t>(m_indices.size());

  // Create a key made of the attributes, to see if the primitive was already
  // processed. If it is, we will re-use the cache, but allow the material and
  // indices to be different.
  std::stringstream o;
  for(auto& a : tmesh.attributes)
  {
    o << a.first << a.second;
  }
  std::string key            = o.str();
  bool        primMeshCached = false;

  // Found a cache - will not need to append vertex
  auto it = m_cachePrimMesh.find(key);
  if(it != m_cachePrimMesh.end())
  {
    primMeshCached          = true;
    GltfPrimMesh cacheMesh  = it->second;
    resultMesh.vertexCount  = cacheMesh.vertexCount;
    resultMesh.vertexOffset = cacheMesh.vertexOffset;
  }


  // INDICES
  if(tmesh.indices > -1)
  {
    const tinygltf::Accessor& indexAccessor = tmodel.accessors[tmesh.indices];
    resultMesh.indexCount                   = static_cast<uint32_t>(indexAccessor.count);

    switch(indexAccessor.componentType)
    {
      case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
        primitiveIndices32u.resize(indexAccessor.count);
        tinygltf::utils::copyAccessorData(primitiveIndices32u, 0, tmodel, indexAccessor, 0, indexAccessor.count);
        m_indices.insert(m_indices.end(), primitiveIndices32u.begin(), primitiveIndices32u.end());
        break;
      }
      case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
        primitiveIndices16u.resize(indexAccessor.count);
        tinygltf::utils::copyAccessorData(primitiveIndices16u, 0, tmodel, indexAccessor, 0, indexAccessor.count);
        m_indices.insert(m_indices.end(), primitiveIndices16u.begin(), primitiveIndices16u.end());
        break;
      }
      case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
        primitiveIndices8u.resize(indexAccessor.count);
        tinygltf::utils::copyAccessorData(primitiveIndices8u, 0, tmodel, indexAccessor, 0, indexAccessor.count);
        m_indices.insert(m_indices.end(), primitiveIndices8u.begin(), primitiveIndices8u.end());
        break;
      }
      default:
        LOGE("Index component type %i not supported!\n", indexAccessor.componentType);
        return;
    }
  }
  else
  {
    // Primitive without indices, creating them
    const auto& accessor = tmodel.accessors[tmesh.attributes.find("POSITION")->second];
    for(auto i = 0; i < accessor.count; i++)
      m_indices.push_back(i);
    resultMesh.indexCount = static_cast<uint32_t>(accessor.count);
  }

  if(primMeshCached == false)  // Need to add this primitive
  {
    // POSITION
    {
      const bool hadPosition = tinygltf::utils::getAttribute<glm::vec3>(tmodel, tmesh, m_positions, "POSITION");
      if(!hadPosition)
      {
        LOGE("This glTF file is invalid: it had a primitive with no POSITION attribute.\n");
        return;
      }
      // Keeping the size of this primitive (Spec says this is required information)
      const auto& accessor   = tmodel.accessors[tmesh.attributes.find("POSITION")->second];
      resultMesh.vertexCount = static_cast<uint32_t>(accessor.count);
      if(!accessor.minValues.empty())
      {
        resultMesh.posMin = glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]);
      }
      else
      {
        resultMesh.posMin = glm::vec3(std::numeric_limits<float>::max());
        for(const auto& p : m_positions)
        {
          for(int i = 0; i < 3; i++)
          {
            if(p[i] < resultMesh.posMin[i])
              resultMesh.posMin[i] = p[i];
          }
        }
      }
      if(!accessor.maxValues.empty())
      {
        resultMesh.posMax = glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]);
      }
      else
      {
        resultMesh.posMax = glm::vec3(-std::numeric_limits<float>::max());
        for(const auto& p : m_positions)
          for(int i = 0; i < 3; i++)
          {
            if(p[i] > resultMesh.posMax[i])
              resultMesh.posMax[i] = p[i];
          }
      }
    }

    // NORMAL
    if(hasFlag(requestedAttributes, GltfAttributes::Normal))
    {
      bool normalCreated = tinygltf::utils::getAttribute<glm::vec3>(tmodel, tmesh, m_normals, "NORMAL");

      if(!normalCreated && hasFlag(forceRequested, GltfAttributes::Normal))
        createNormals(resultMesh);
    }

    // TEXCOORD_0
    if(hasFlag(requestedAttributes, GltfAttributes::Texcoord_0))
    {
      bool texcoordCreated = tinygltf::utils::getAttribute<glm::vec2>(tmodel, tmesh, m_texcoords0, "TEXCOORD_0");
      if(!texcoordCreated)
        texcoordCreated = tinygltf::utils::getAttribute<glm::vec2>(tmodel, tmesh, m_texcoords0, "TEXCOORD");
      if(!texcoordCreated && hasFlag(forceRequested, GltfAttributes::Texcoord_0))
        createTexcoords(resultMesh);
    }


    // TANGENT
    if(hasFlag(requestedAttributes, GltfAttributes::Tangent))
    {
      bool tangentCreated = tinygltf::utils::getAttribute<glm::vec4>(tmodel, tmesh, m_tangents, "TANGENT");

      if(!tangentCreated && hasFlag(forceRequested, GltfAttributes::Tangent))
        createTangents(resultMesh);
    }

    // COLOR_0
    if(hasFlag(requestedAttributes, GltfAttributes::Color_0))
    {
      bool colorCreated = tinygltf::utils::getAttribute<glm::vec4>(tmodel, tmesh, m_colors0, "COLOR_0");
      if(!colorCreated && hasFlag(forceRequested, GltfAttributes::Color_0))
        createColors(resultMesh);
    }
  }

  // Keep result in cache
  m_cachePrimMesh[key] = resultMesh;

  // Append prim mesh to the list of all primitive meshes
  m_primMeshes.emplace_back(resultMesh);
}  // namespace nvh


void nvh::GltfScene::createNormals(GltfPrimMesh& resultMesh)
{
  // Need to compute the normals
  std::vector<glm::vec3> geonormal(resultMesh.vertexCount);
  for(size_t i = 0; i < resultMesh.indexCount; i += 3)
  {
    uint32_t    ind0 = m_indices[resultMesh.firstIndex + i + 0];
    uint32_t    ind1 = m_indices[resultMesh.firstIndex + i + 1];
    uint32_t    ind2 = m_indices[resultMesh.firstIndex + i + 2];
    const auto& pos0 = m_positions[ind0 + resultMesh.vertexOffset];
    const auto& pos1 = m_positions[ind1 + resultMesh.vertexOffset];
    const auto& pos2 = m_positions[ind2 + resultMesh.vertexOffset];
    const auto  v1   = glm::normalize(pos1 - pos0);  // Many normalize, but when objects are really small the
    const auto  v2   = glm::normalize(pos2 - pos0);  // cross will go below nv_eps and the normal will be (0,0,0)
    const auto  n    = glm::cross(v1, v2);
    geonormal[ind0] += n;
    geonormal[ind1] += n;
    geonormal[ind2] += n;
  }
  for(auto& n : geonormal)
    n = glm::normalize(n);
  m_normals.insert(m_normals.end(), geonormal.begin(), geonormal.end());
}

void nvh::GltfScene::createTexcoords(GltfPrimMesh& resultMesh)
{

  // Set them all to zero
  //      m_texcoords0.insert(m_texcoords0.end(), resultMesh.vertexCount, glm::vec2(0, 0));

  // Cube map projection
  for(uint32_t i = 0; i < resultMesh.vertexCount; i++)
  {
    const auto& pos  = m_positions[resultMesh.vertexOffset + i];
    float       absX = fabs(pos.x);
    float       absY = fabs(pos.y);
    float       absZ = fabs(pos.z);

    int isXPositive = pos.x > 0 ? 1 : 0;
    int isYPositive = pos.y > 0 ? 1 : 0;
    int isZPositive = pos.z > 0 ? 1 : 0;

    float maxAxis{}, uc{}, vc{};  // Zero-initialize in case pos = {NaN, NaN, NaN}

    // POSITIVE X
    if(isXPositive && absX >= absY && absX >= absZ)
    {
      // u (0 to 1) goes from +z to -z
      // v (0 to 1) goes from -y to +y
      maxAxis = absX;
      uc      = -pos.z;
      vc      = pos.y;
    }
    // NEGATIVE X
    if(!isXPositive && absX >= absY && absX >= absZ)
    {
      // u (0 to 1) goes from -z to +z
      // v (0 to 1) goes from -y to +y
      maxAxis = absX;
      uc      = pos.z;
      vc      = pos.y;
    }
    // POSITIVE Y
    if(isYPositive && absY >= absX && absY >= absZ)
    {
      // u (0 to 1) goes from -x to +x
      // v (0 to 1) goes from +z to -z
      maxAxis = absY;
      uc      = pos.x;
      vc      = -pos.z;
    }
    // NEGATIVE Y
    if(!isYPositive && absY >= absX && absY >= absZ)
    {
      // u (0 to 1) goes from -x to +x
      // v (0 to 1) goes from -z to +z
      maxAxis = absY;
      uc      = pos.x;
      vc      = pos.z;
    }
    // POSITIVE Z
    if(isZPositive && absZ >= absX && absZ >= absY)
    {
      // u (0 to 1) goes from -x to +x
      // v (0 to 1) goes from -y to +y
      maxAxis = absZ;
      uc      = pos.x;
      vc      = pos.y;
    }
    // NEGATIVE Z
    if(!isZPositive && absZ >= absX && absZ >= absY)
    {
      // u (0 to 1) goes from +x to -x
      // v (0 to 1) goes from -y to +y
      maxAxis = absZ;
      uc      = -pos.x;
      vc      = pos.y;
    }

    // Convert range from -1 to 1 to 0 to 1
    float u = 0.5f * (uc / maxAxis + 1.0f);
    float v = 0.5f * (vc / maxAxis + 1.0f);

    m_texcoords0.emplace_back(u, v);
  }
}

void nvh::GltfScene::createTangents(GltfPrimMesh& resultMesh)
{

  // #TODO - Should calculate tangents using default MikkTSpace algorithms
  // See: https://github.com/mmikk/MikkTSpace

  std::vector<glm::vec3> tangent(resultMesh.vertexCount);
  std::vector<glm::vec3> bitangent(resultMesh.vertexCount);

  // Current implementation
  // http://foundationsofgameenginedev.com/FGED2-sample.pdf
  for(size_t i = 0; i < resultMesh.indexCount; i += 3)
  {
    // local index
    uint32_t i0 = m_indices[resultMesh.firstIndex + i + 0];
    uint32_t i1 = m_indices[resultMesh.firstIndex + i + 1];
    uint32_t i2 = m_indices[resultMesh.firstIndex + i + 2];
    assert(i0 < resultMesh.vertexCount);
    assert(i1 < resultMesh.vertexCount);
    assert(i2 < resultMesh.vertexCount);


    // global index
    uint32_t gi0 = i0 + resultMesh.vertexOffset;
    uint32_t gi1 = i1 + resultMesh.vertexOffset;
    uint32_t gi2 = i2 + resultMesh.vertexOffset;

    const auto& p0 = m_positions[gi0];
    const auto& p1 = m_positions[gi1];
    const auto& p2 = m_positions[gi2];

    const auto& uv0 = m_texcoords0[gi0];
    const auto& uv1 = m_texcoords0[gi1];
    const auto& uv2 = m_texcoords0[gi2];

    glm::vec3 e1 = p1 - p0;
    glm::vec3 e2 = p2 - p0;

    glm::vec2 duvE1 = uv1 - uv0;
    glm::vec2 duvE2 = uv2 - uv0;

    float r = 1.0F;
    float a = duvE1.x * duvE2.y - duvE2.x * duvE1.y;
    if(fabs(a) > 0)  // Catch degenerated UV
    {
      r = 1.0f / a;
    }

    glm::vec3 t = (e1 * duvE2.y - e2 * duvE1.y) * r;
    glm::vec3 b = (e2 * duvE1.x - e1 * duvE2.x) * r;


    tangent[i0] += t;
    tangent[i1] += t;
    tangent[i2] += t;

    bitangent[i0] += b;
    bitangent[i1] += b;
    bitangent[i2] += b;
  }

  for(uint32_t a = 0; a < resultMesh.vertexCount; a++)
  {
    const auto& t = tangent[a];
    const auto& b = bitangent[a];
    const auto& n = m_normals[resultMesh.vertexOffset + a];

    // Gram-Schmidt orthogonalize
    glm::vec3 otangent = glm::normalize(t - (glm::dot(n, t) * n));

    // In case the tangent is invalid
    if(otangent == glm::vec3(0, 0, 0))
    {
      if(fabsf(n.x) > fabsf(n.y))
        otangent = glm::vec3(n.z, 0, -n.x) / sqrtf(n.x * n.x + n.z * n.z);
      else
        otangent = glm::vec3(0, -n.z, n.y) / sqrtf(n.y * n.y + n.z * n.z);
    }

    // Calculate handedness
    float handedness = (glm::dot(glm::cross(n, t), b) < 0.0F) ? 1.0F : -1.0F;
    m_tangents.emplace_back(otangent.x, otangent.y, otangent.z, handedness);
  }
}

void nvh::GltfScene::createColors(GltfPrimMesh& resultMesh)
{
  // Set them all to one
  m_colors0.insert(m_colors0.end(), resultMesh.vertexCount, glm::vec4(1, 1, 1, 1));
}


void nvh::GltfScene::destroy()
{
  m_materials.clear();
  m_nodes.clear();
  m_primMeshes.clear();
  m_cameras.clear();
  m_lights.clear();

  m_positions.clear();
  m_indices.clear();
  m_normals.clear();
  m_tangents.clear();
  m_texcoords0.clear();
  m_texcoords1.clear();
  m_colors0.clear();
  m_cameras.clear();
  //m_joints0.clear();
  //m_weights0.clear();
  m_dimensions = {};
  m_meshToPrimMeshes.clear();
  primitiveIndices32u.clear();
  primitiveIndices16u.clear();
  primitiveIndices8u.clear();
  m_cachePrimMesh.clear();
}

//--------------------------------------------------------------------------------------------------
// Get the dimension of the scene
//
void nvh::GltfScene::computeSceneDimensions()
{
  Bbox scnBbox;

  for(const auto& node : m_nodes)
  {
    const auto& mesh = m_primMeshes[node.primMesh];

    Bbox bbox(mesh.posMin, mesh.posMax);
    bbox = bbox.transform(node.worldMatrix);
    scnBbox.insert(bbox);
  }

  if(scnBbox.isEmpty() || !scnBbox.isVolume())
  {
    LOGE("glTF: Scene bounding box invalid, Setting to: [-1,-1,-1], [1,1,1]");
    scnBbox.insert({-1.0f, -1.0f, -1.0f});
    scnBbox.insert({1.0f, 1.0f, 1.0f});
  }

  m_dimensions.min    = scnBbox.min();
  m_dimensions.max    = scnBbox.max();
  m_dimensions.size   = scnBbox.extents();
  m_dimensions.center = scnBbox.center();
  m_dimensions.radius = scnBbox.radius();
}


static uint32_t recursiveTriangleCount(const tinygltf::Model& model, int nodeIdx, const std::vector<uint32_t>& meshTriangle)
{
  auto&    node = model.nodes[nodeIdx];
  uint32_t nbTriangles{0};
  for(const auto child : node.children)
  {
    nbTriangles += recursiveTriangleCount(model, child, meshTriangle);
  }

  if(node.mesh >= 0)
    nbTriangles += meshTriangle[node.mesh];

  return nbTriangles;
}

//--------------------------------------------------------------------------------------------------
// Retrieving information about the scene
//
nvh::GltfStats nvh::GltfScene::getStatistics(const tinygltf::Model& tinyModel)
{
  GltfStats stats;

  stats.nbCameras   = static_cast<uint32_t>(tinyModel.cameras.size());
  stats.nbImages    = static_cast<uint32_t>(tinyModel.images.size());
  stats.nbTextures  = static_cast<uint32_t>(tinyModel.textures.size());
  stats.nbMaterials = static_cast<uint32_t>(tinyModel.materials.size());
  stats.nbSamplers  = static_cast<uint32_t>(tinyModel.samplers.size());
  stats.nbNodes     = static_cast<uint32_t>(tinyModel.nodes.size());
  stats.nbMeshes    = static_cast<uint32_t>(tinyModel.meshes.size());
  stats.nbLights    = static_cast<uint32_t>(tinyModel.lights.size());

  // Computing the memory usage for images
  for(const auto& image : tinyModel.images)
  {
    stats.imageMem += image.width * image.height * image.component * image.bits / 8;
  }

  // Computing the number of triangles
  std::vector<uint32_t> meshTriangle(tinyModel.meshes.size());
  uint32_t              meshIdx{0};
  for(const auto& mesh : tinyModel.meshes)
  {
    for(const auto& primitive : mesh.primitives)
    {
      if(primitive.indices > -1)
      {
        const tinygltf::Accessor& indexAccessor = tinyModel.accessors[primitive.indices];
        meshTriangle[meshIdx] += static_cast<uint32_t>(indexAccessor.count) / 3;
      }
      else
      {
        const auto& posAccessor = tinyModel.accessors[primitive.attributes.find("POSITION")->second];
        meshTriangle[meshIdx] += static_cast<uint32_t>(posAccessor.count) / 3;
      }
    }
    meshIdx++;
  }

  stats.nbUniqueTriangles = std::accumulate(meshTriangle.begin(), meshTriangle.end(), 0, std::plus<>());
  for(auto& node : tinyModel.scenes[0].nodes)
  {
    stats.nbTriangles += recursiveTriangleCount(tinyModel, node, meshTriangle);
  }

  return stats;
}


//--------------------------------------------------------------------------------------------------
// Going through all cameras and find the position and center of interest.
// - The eye or position of the camera is found in the translation part of the matrix
// - The center of interest is arbitrary set in front of the camera to a distance equivalent
//   to the eye and the center of the scene. If the camera is pointing toward the middle
//   of the scene, the camera center will be equal to the scene center.
// - The up vector is always Y up for now.
//
void nvh::GltfScene::computeCamera()
{
  for(auto& camera : m_cameras)
  {
    if(camera.eye == camera.center)  // Applying the rule only for uninitialized camera.
    {
      camera.eye         = glm::vec3(camera.worldMatrix[3]);
      float     distance = glm::length(m_dimensions.center - camera.eye);
      glm::mat3 rotMat   = glm::mat3(camera.worldMatrix);
      camera.center      = {0, 0, -distance};
      camera.center      = camera.eye + (rotMat * camera.center);
      camera.up          = {0, 1, 0};
    }
  }
}

void nvh::GltfScene::checkRequiredExtensions(const tinygltf::Model& tmodel)
{
  std::set<std::string> supportedExtensions{
      KHR_LIGHTS_PUNCTUAL_EXTENSION_NAME,
      KHR_TEXTURE_TRANSFORM_EXTENSION_NAME,
      KHR_MATERIALS_SPECULAR_EXTENSION_NAME,
      KHR_MATERIALS_UNLIT_EXTENSION_NAME,
      KHR_MATERIALS_ANISOTROPY_EXTENSION_NAME,
      KHR_MATERIALS_IOR_EXTENSION_NAME,
      KHR_MATERIALS_VOLUME_EXTENSION_NAME,
      KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME,
      KHR_TEXTURE_BASISU_NAME,
  };

  for(auto& e : tmodel.extensionsRequired)
  {
    if(supportedExtensions.find(e) == supportedExtensions.end())
    {
      LOGE(
          "\n---------------------------------------\n"
          "The extension %s is REQUIRED and not supported \n",
          e.c_str());
    }
  }
}

void nvh::GltfScene::findUsedMeshes(const tinygltf::Model& tmodel, std::set<uint32_t>& usedMeshes, int nodeIdx)
{
  const auto& node = tmodel.nodes[nodeIdx];
  if(node.mesh >= 0)
    usedMeshes.insert(node.mesh);
  for(const auto& c : node.children)
    findUsedMeshes(tmodel, usedMeshes, c);
}

//--------------------------------------------------------------------------------------------------
//
//
void nvh::GltfScene::exportDrawableNodes(tinygltf::Model& tmodel, GltfAttributes requestedAttributes)
{

  // We are rewriting buffer 0, but we will keep the information accessing the other buffers.
  // Information will be added at the end of the function.

  // Copy the buffer view and its position, such that we have a reference from the accessors
  std::unordered_map<uint32_t, tinygltf::BufferView> tempBufferView;
  for(uint32_t i = 0; i < (uint32_t)tmodel.bufferViews.size(); i++)
  {
    if(tmodel.bufferViews[i].buffer > 0)
      tempBufferView[i] = tmodel.bufferViews[i];
  }
  // Copy all accessors, referencing to one of the temp BufferView
  std::vector<tinygltf::Accessor> tempAccessors;
  for(auto& accessor : tmodel.accessors)
  {
    if(tempBufferView.find(accessor.bufferView) != tempBufferView.end())
    {
      tempAccessors.push_back(accessor);
    }
  }


  // Clear what will the rewritten
  tmodel.bufferViews.clear();
  tmodel.accessors.clear();
  tmodel.scenes[0].nodes.clear();

  // Default assets
  tmodel.asset.copyright = "NVIDIA Corporation";
  tmodel.asset.generator = "glTF exporter";
  tmodel.asset.version   = "2.0";  // glTF version 2.0

  bool hasExt = std::find(tmodel.extensionsUsed.begin(), tmodel.extensionsUsed.end(), EXTENSION_ATTRIB_IRAY)
                != tmodel.extensionsUsed.end();
  if(!hasExt)
    tmodel.extensionsUsed.emplace_back(EXTENSION_ATTRIB_IRAY);

  // Reset all information in the buffer 0
  tmodel.buffers[0] = {};
  auto& tBuffer     = tmodel.buffers[0];
  tBuffer.data.reserve(1000000000);  // Reserving 1 GB to make the allocations faster (avoid re-allocations)

  struct OffsetLen
  {
    uint32_t offset{0};
    uint32_t len{0};
  };
  OffsetLen olIdx, olPos, olNrm, olTan, olCol, olTex;
  olIdx.len    = tinygltf::utils::appendData(tBuffer, m_indices);
  olPos.offset = olIdx.offset + olIdx.len;
  olPos.len    = tinygltf::utils::appendData(tBuffer, m_positions);
  olNrm.offset = olPos.offset + olPos.len;
  if(hasFlag(requestedAttributes, GltfAttributes::Normal) && !m_normals.empty())
  {
    olNrm.len = tinygltf::utils::appendData(tBuffer, m_normals);
  }
  olTex.offset = olNrm.offset + olNrm.len;
  if(hasFlag(requestedAttributes, GltfAttributes::Texcoord_0) && !m_texcoords0.empty())
  {
    olTex.len = tinygltf::utils::appendData(tBuffer, m_texcoords0);
  }
  olTan.offset = olTex.offset + olTex.len;
  if(hasFlag(requestedAttributes, GltfAttributes::Tangent) && !m_tangents.empty())
  {
    olTan.len = tinygltf::utils::appendData(tBuffer, m_tangents);
  }
  olCol.offset = olTan.offset + olTan.len;
  if(hasFlag(requestedAttributes, GltfAttributes::Color_0) && !m_colors0.empty())
  {
    olCol.len = tinygltf::utils::appendData(tBuffer, m_colors0);
  }

  glm::dmat4 identityMat(1);

  // Nodes
  std::vector<tinygltf::Node> tnodes;
  uint32_t                    nodeID = 0;
  for(auto& n : m_nodes)
  {
    auto* node = &tnodes.emplace_back();
    node->name = "";
    if(n.tnode)
    {
      node->name = n.tnode->name;
    }


    // Matrix -- Adding it only if not identity
    {
      bool                isIdentity{true};
      std::vector<double> matrix(16);
      for(int i = 0; i < 16; i++)
      {
        int row = i / 4;
        int col = i % 4;
        isIdentity &= (identityMat[row][col] == n.worldMatrix[row][col]);
        matrix[i] = n.worldMatrix[row][col];
      }
      if(!isIdentity)
        node->matrix = matrix;
    }
    node->mesh = n.primMesh;

    // Adding node to the scene
    tmodel.scenes[0].nodes.push_back(nodeID++);
  }

  // Camera
  if(!m_cameras.empty())
  {
    tmodel.cameras.clear();
    auto& camera = tmodel.cameras.emplace_back();
    // Copy the tiny camera
    camera = m_cameras[0].cam;

    // Add camera node
    auto& node  = tnodes.emplace_back();
    node.name   = "Camera";
    node.camera = 0;
    node.matrix.resize(16);
    for(int i = 0; i < 16; i++)
    {
      int row        = i / 4;
      int col        = i % 4;
      node.matrix[i] = m_cameras[0].worldMatrix[row][col];
    }

    // Add extension with pos, interest and up
    auto fct = [](const std::string& name, glm::vec3 val) {
      tinygltf::Value::Array tarr;
      tarr.emplace_back(val.x);
      tarr.emplace_back(val.y);
      tarr.emplace_back(val.z);

      tinygltf::Value::Object oattrib;
      oattrib["name"]  = tinygltf::Value(name);
      oattrib["type"]  = tinygltf::Value(std::string("Float32<3>"));
      oattrib["value"] = tinygltf::Value(tarr);
      return oattrib;
    };

    tinygltf::Value::Array vattrib;
    vattrib.emplace_back(fct("iview:position", m_cameras[0].eye));
    vattrib.emplace_back(fct("iview:interest", m_cameras[0].center));
    vattrib.emplace_back(fct("iview:up", m_cameras[0].up));
    tinygltf::Value::Object oattrib;
    oattrib["attributes"]                  = tinygltf::Value(vattrib);
    node.extensions[EXTENSION_ATTRIB_IRAY] = tinygltf::Value(oattrib);

    // Add camera to scene
    tmodel.scenes[0].nodes.push_back((int)tnodes.size() - 1);
  }
  tmodel.nodes = tnodes;

  // Mesh/Primitive
  std::vector<tinygltf::Mesh> tmeshes;
  for(const auto& pm : m_primMeshes)
  {
    Bbox bb;
    {
      for(size_t v_ctx = 0; v_ctx < pm.vertexCount; v_ctx++)
      {
        size_t idx = pm.vertexOffset + v_ctx;
        bb.insert(m_positions[idx]);
      }
    }

    // Adding a new mesh
    tmeshes.emplace_back();
    auto& tMesh = tmeshes.back();
    tMesh.name  = pm.name;

    if(pm.tmesh)
    {
      tMesh.name                   = pm.tmesh->name;
      tMesh.extensions             = pm.tmesh->extensions;
      tMesh.extensions_json_string = pm.tmesh->extensions_json_string;
    }


    //  primitive
    tMesh.primitives.emplace_back();
    auto& tPrim = tMesh.primitives.back();
    tPrim.mode  = TINYGLTF_MODE_TRIANGLES;

    if(pm.tprim)
    {
      tPrim.extensions             = pm.tprim->extensions;
      tPrim.extensions_json_string = pm.tprim->extensions_json_string;
    }


    // Material reference
    tPrim.material = pm.materialIndex;

    // Attributes
    auto& attributes = tPrim.attributes;


    {
      // Buffer View (INDICES)
      tmodel.bufferViews.emplace_back();
      auto& tBufferView      = tmodel.bufferViews.back();
      tBufferView.buffer     = 0;
      tBufferView.byteOffset = olIdx.offset + pm.firstIndex * sizeof(uint32_t);
      tBufferView.byteStride = 0;  // "bufferView.byteStride must not be defined for indices accessor." ;
      tBufferView.byteLength = sizeof(uint32_t) * pm.indexCount;

      // Accessor (INDICES)
      tmodel.accessors.emplace_back();
      auto& tAccessor         = tmodel.accessors.back();
      tAccessor.bufferView    = static_cast<int>(tmodel.bufferViews.size() - 1);
      tAccessor.byteOffset    = 0;
      tAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
      tAccessor.count         = pm.indexCount;
      tAccessor.type          = TINYGLTF_TYPE_SCALAR;

      // The accessor for the indices
      tPrim.indices = static_cast<int>(tmodel.accessors.size() - 1);
    }


    {
      // Buffer View (POSITION)
      tmodel.bufferViews.emplace_back();
      auto& tBufferView      = tmodel.bufferViews.back();
      tBufferView.buffer     = 0;
      tBufferView.byteOffset = olPos.offset + pm.vertexOffset * sizeof(glm::vec3);
      tBufferView.byteStride = 3 * sizeof(float);
      tBufferView.byteLength = pm.vertexCount * tBufferView.byteStride;

      // Accessor (POSITION)
      tmodel.accessors.emplace_back();
      auto& tAccessor         = tmodel.accessors.back();
      tAccessor.bufferView    = static_cast<int>(tmodel.bufferViews.size() - 1);
      tAccessor.byteOffset    = 0;
      tAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
      tAccessor.count         = pm.vertexCount;
      tAccessor.type          = TINYGLTF_TYPE_VEC3;
      tAccessor.minValues     = {bb.min()[0], bb.min()[1], bb.min()[2]};
      tAccessor.maxValues     = {bb.max()[0], bb.max()[1], bb.max()[2]};
      assert(tAccessor.count > 0);

      attributes["POSITION"] = static_cast<int>(tmodel.accessors.size() - 1);
    }

    if(hasFlag(requestedAttributes, GltfAttributes::Normal) && !m_normals.empty())
    {
      // Buffer View (NORMAL)
      tmodel.bufferViews.emplace_back();
      auto& tBufferView      = tmodel.bufferViews.back();
      tBufferView.buffer     = 0;
      tBufferView.byteOffset = olNrm.offset + pm.vertexOffset * sizeof(glm::vec3);
      tBufferView.byteStride = 3 * sizeof(float);
      tBufferView.byteLength = pm.vertexCount * tBufferView.byteStride;

      // Accessor (NORMAL)
      tmodel.accessors.emplace_back();
      auto& tAccessor         = tmodel.accessors.back();
      tAccessor.bufferView    = static_cast<int>(tmodel.bufferViews.size() - 1);
      tAccessor.byteOffset    = 0;
      tAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
      tAccessor.count         = pm.vertexCount;
      tAccessor.type          = TINYGLTF_TYPE_VEC3;

      attributes["NORMAL"] = static_cast<int>(tmodel.accessors.size() - 1);
    }

    if(hasFlag(requestedAttributes, GltfAttributes::Texcoord_0) && !m_texcoords0.empty())
    {
      // Buffer View (TEXCOORD_0)
      tmodel.bufferViews.emplace_back();
      auto& tBufferView      = tmodel.bufferViews.back();
      tBufferView.buffer     = 0;
      tBufferView.byteOffset = olTex.offset + pm.vertexOffset * sizeof(glm::vec2);
      tBufferView.byteStride = 2 * sizeof(float);
      tBufferView.byteLength = pm.vertexCount * tBufferView.byteStride;

      // Accessor (TEXCOORD_0)
      tmodel.accessors.emplace_back();
      auto& tAccessor         = tmodel.accessors.back();
      tAccessor.bufferView    = static_cast<int>(tmodel.bufferViews.size() - 1);
      tAccessor.byteOffset    = 0;
      tAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
      tAccessor.count         = pm.vertexCount;
      tAccessor.type          = TINYGLTF_TYPE_VEC2;

      attributes["TEXCOORD_0"] = static_cast<int>(tmodel.accessors.size() - 1);
    }

    if(hasFlag(requestedAttributes, GltfAttributes::Tangent) && !m_tangents.empty())
    {
      // Buffer View (TANGENT)
      tmodel.bufferViews.emplace_back();
      auto& tBufferView      = tmodel.bufferViews.back();
      tBufferView.buffer     = 0;
      tBufferView.byteOffset = olTan.offset + pm.vertexOffset * sizeof(glm::vec4);
      tBufferView.byteStride = 4 * sizeof(float);
      tBufferView.byteLength = pm.vertexCount * tBufferView.byteStride;

      // Accessor (TANGENT)
      tmodel.accessors.emplace_back();
      auto& tAccessor         = tmodel.accessors.back();
      tAccessor.bufferView    = static_cast<int>(tmodel.bufferViews.size() - 1);
      tAccessor.byteOffset    = 0;
      tAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
      tAccessor.count         = pm.vertexCount;
      tAccessor.type          = TINYGLTF_TYPE_VEC4;

      attributes["TANGENT"] = static_cast<int>(tmodel.accessors.size() - 1);
    }

    if(hasFlag(requestedAttributes, GltfAttributes::Color_0) && !m_colors0.empty())
    {
      // Buffer View (COLOR_O)
      tmodel.bufferViews.emplace_back();
      auto& tBufferView      = tmodel.bufferViews.back();
      tBufferView.buffer     = 0;
      tBufferView.byteOffset = olCol.offset + pm.vertexOffset * sizeof(glm::vec4);
      tBufferView.byteStride = 4 * sizeof(float);
      tBufferView.byteLength = pm.vertexCount * tBufferView.byteStride;

      // Accessor (COLOR_O)
      tmodel.accessors.emplace_back();
      auto& tAccessor         = tmodel.accessors.back();
      tAccessor.bufferView    = static_cast<int>(tmodel.bufferViews.size() - 1);
      tAccessor.byteOffset    = 0;
      tAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
      tAccessor.count         = pm.vertexCount;
      tAccessor.type          = TINYGLTF_TYPE_VEC4;

      attributes["COLOR_O"] = static_cast<int>(tmodel.accessors.size() - 1);
    }
  }
  tmodel.meshes = tmeshes;


  // Add back accessors for extra buffers (see collection at start of function)
  std::unordered_map<uint32_t, uint32_t> correspondance;
  for(auto& t : tempBufferView)
  {
    correspondance[t.first] = (uint32_t)tmodel.bufferViews.size();  // remember position of buffer view
    tmodel.bufferViews.push_back(t.second);
  }
  for(auto& t : tempAccessors)
  {
    t.bufferView = correspondance[t.bufferView];  // Find from previous buffer view, the new index
    tmodel.accessors.push_back(t);
  }
}
