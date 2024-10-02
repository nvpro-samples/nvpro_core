
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

#include "tinygltf_utils.hpp"


KHR_materials_displacement tinygltf::utils::getDisplacement(const tinygltf::Material& tmat)
{
  KHR_materials_displacement gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_DISPLACEMENT_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_DISPLACEMENT_EXTENSION_NAME);
    tinygltf::utils::getValue(ext, "displacementGeometryTexture", gmat.displacementGeometryTexture);
    tinygltf::utils::getValue(ext, "displacementGeometryFactor", gmat.displacementGeometryFactor);
    tinygltf::utils::getValue(ext, "displacementGeometryOffset", gmat.displacementGeometryOffset);
  }
  return gmat;
}

void tinygltf::utils::setDisplacement(tinygltf::Material& tmat, const KHR_materials_displacement& displacement)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_DISPLACEMENT_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_DISPLACEMENT_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_DISPLACEMENT_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "displacementGeometryTexture", displacement.displacementGeometryTexture);
  tinygltf::utils::setValue(ext, "displacementGeometryFactor", displacement.displacementGeometryFactor);
  tinygltf::utils::setValue(ext, "displacementGeometryOffset", displacement.displacementGeometryOffset);
}

KHR_materials_emissive_strength tinygltf::utils::getEmissiveStrength(const tinygltf::Material& tmat)
{
  KHR_materials_emissive_strength gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_EMISSIVE_STRENGTH_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_EMISSIVE_STRENGTH_EXTENSION_NAME);
    tinygltf::utils::getValue(ext, "emissiveStrength", gmat.emissiveStrength);
  }
  return gmat;
}

void tinygltf::utils::setEmissiveStrength(tinygltf::Material& tmat, const KHR_materials_emissive_strength& emissiveStrength)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_EMISSIVE_STRENGTH_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_EMISSIVE_STRENGTH_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_EMISSIVE_STRENGTH_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "emissiveStrength", emissiveStrength.emissiveStrength);
}


KHR_materials_volume tinygltf::utils::getVolume(const tinygltf::Material& tmat)
{
  KHR_materials_volume gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_VOLUME_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_VOLUME_EXTENSION_NAME);
    tinygltf::utils::getValue(ext, "thicknessFactor", gmat.thicknessFactor);
    tinygltf::utils::getValue(ext, "thicknessTexture", gmat.thicknessTexture);
    tinygltf::utils::getValue(ext, "attenuationDistance", gmat.attenuationDistance);
    tinygltf::utils::getArrayValue(ext, "attenuationColor", gmat.attenuationColor);
  }
  return gmat;
}

void tinygltf::utils::setVolume(tinygltf::Material& tmat, const KHR_materials_volume& volume)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_VOLUME_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_VOLUME_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_VOLUME_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "thicknessFactor", volume.thicknessFactor);
  tinygltf::utils::setValue(ext, "thicknessTexture", volume.thicknessTexture);
  tinygltf::utils::setValue(ext, "attenuationDistance", volume.attenuationDistance);
  tinygltf::utils::setArrayValue(ext, "attenuationColor", 3, glm::value_ptr(volume.attenuationColor));
}

KHR_materials_unlit tinygltf::utils::getUnlit(const tinygltf::Material& tmat)
{
  KHR_materials_unlit gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_UNLIT_EXTENSION_NAME))
  {
    gmat.active = 1;
  }
  return gmat;
}

void tinygltf::utils::setUnlit(tinygltf::Material& tmat, const KHR_materials_unlit& unlit)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_UNLIT_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_UNLIT_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_UNLIT_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "unlit", true);
}


KHR_materials_specular tinygltf::utils::getSpecular(const tinygltf::Material& tmat)
{
  KHR_materials_specular gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_SPECULAR_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_SPECULAR_EXTENSION_NAME);
    tinygltf::utils::getValue(ext, "specularFactor", gmat.specularFactor);
    tinygltf::utils::getValue(ext, "specularTexture", gmat.specularTexture);
    tinygltf::utils::getArrayValue(ext, "specularColorFactor", gmat.specularColorFactor);
    tinygltf::utils::getValue(ext, "specularColorTexture", gmat.specularColorTexture);
  }
  return gmat;
}

void tinygltf::utils::setSpecular(tinygltf::Material& tmat, const KHR_materials_specular& specular)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_SPECULAR_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_SPECULAR_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_SPECULAR_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "specularFactor", specular.specularFactor);
  tinygltf::utils::setValue(ext, "specularTexture", specular.specularTexture);
  tinygltf::utils::setValue(ext, "specularColorTexture", specular.specularColorTexture);
  tinygltf::utils::setArrayValue(ext, "specularColorFactor", 3, glm::value_ptr(specular.specularColorFactor));
}


KHR_materials_clearcoat tinygltf::utils::getClearcoat(const tinygltf::Material& tmat)
{
  KHR_materials_clearcoat gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_CLEARCOAT_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_CLEARCOAT_EXTENSION_NAME);
    tinygltf::utils::getValue(ext, "clearcoatFactor", gmat.factor);
    tinygltf::utils::getValue(ext, "clearcoatTexture", gmat.texture);
    tinygltf::utils::getValue(ext, "clearcoatRoughnessFactor", gmat.roughnessFactor);
    tinygltf::utils::getValue(ext, "clearcoatRoughnessTexture", gmat.roughnessTexture);
    tinygltf::utils::getValue(ext, "clearcoatNormalTexture", gmat.normalTexture);
  }
  return gmat;
}

void tinygltf::utils::setClearcoat(tinygltf::Material& tmat, const KHR_materials_clearcoat& clearcoat)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_CLEARCOAT_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_CLEARCOAT_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_CLEARCOAT_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "clearcoatFactor", clearcoat.factor);
  tinygltf::utils::setValue(ext, "clearcoatRoughnessFactor", clearcoat.roughnessFactor);
  tinygltf::utils::setValue(ext, "clearcoatTexture", clearcoat.texture);
  tinygltf::utils::setValue(ext, "clearcoatRoughnessTexture", clearcoat.roughnessTexture);
  tinygltf::utils::setValue(ext, "clearcoatNormalTexture", clearcoat.normalTexture);
}

KHR_materials_sheen tinygltf::utils::getSheen(const tinygltf::Material& tmat)
{
  KHR_materials_sheen gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_SHEEN_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_SHEEN_EXTENSION_NAME);
    tinygltf::utils::getArrayValue(ext, "sheenColorFactor", gmat.sheenColorFactor);
    tinygltf::utils::getValue(ext, "sheenColorTexture", gmat.sheenColorTexture);
    tinygltf::utils::getValue(ext, "sheenRoughnessFactor", gmat.sheenRoughnessFactor);
    tinygltf::utils::getValue(ext, "sheenRoughnessTexture", gmat.sheenRoughnessTexture);
  }
  return gmat;
}

void tinygltf::utils::setSheen(tinygltf::Material& tmat, const KHR_materials_sheen& sheen)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_SHEEN_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_SHEEN_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_SHEEN_EXTENSION_NAME];
  tinygltf::utils::setArrayValue(ext, "sheenColorFactor", 3, glm::value_ptr(sheen.sheenColorFactor));
  tinygltf::utils::setValue(ext, "sheenColorTexture", sheen.sheenColorTexture);
  tinygltf::utils::setValue(ext, "sheenRoughnessFactor", sheen.sheenRoughnessFactor);
  tinygltf::utils::setValue(ext, "sheenRoughnessTexture", sheen.sheenRoughnessTexture);
}


KHR_materials_transmission tinygltf::utils::getTransmission(const tinygltf::Material& tmat)
{
  KHR_materials_transmission gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME);
    tinygltf::utils::getValue(ext, "transmissionFactor", gmat.factor);
    tinygltf::utils::getValue(ext, "transmissionTexture", gmat.texture);
  }
  return gmat;
}

void tinygltf::utils::setTransmission(tinygltf::Material& tmat, const KHR_materials_transmission& transmission)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "transmissionFactor", transmission.factor);
  tinygltf::utils::setValue(ext, "transmissionTexture", transmission.texture);
}

KHR_materials_anisotropy tinygltf::utils::getAnisotropy(const tinygltf::Material& tmat)
{
  KHR_materials_anisotropy gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_ANISOTROPY_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_ANISOTROPY_EXTENSION_NAME);
    tinygltf::utils::getValue(ext, "anisotropyStrength", gmat.anisotropyStrength);
    tinygltf::utils::getValue(ext, "anisotropyRotation", gmat.anisotropyRotation);
    tinygltf::utils::getValue(ext, "anisotropyTexture", gmat.anisotropyTexture);
  }
  return gmat;
}

void tinygltf::utils::setAnisotropy(tinygltf::Material& tmat, const KHR_materials_anisotropy& anisotropy)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_ANISOTROPY_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_ANISOTROPY_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_ANISOTROPY_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "anisotropyStrength", anisotropy.anisotropyStrength);
  tinygltf::utils::setValue(ext, "anisotropyRotation", anisotropy.anisotropyRotation);
  tinygltf::utils::setValue(ext, "anisotropyTexture", anisotropy.anisotropyTexture);
}

KHR_materials_ior tinygltf::utils::getIor(const tinygltf::Material& tmat)
{
  KHR_materials_ior gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_IOR_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_IOR_EXTENSION_NAME);
    tinygltf::utils::getValue(ext, "ior", gmat.ior);
  }
  return gmat;
}

void tinygltf::utils::setIor(tinygltf::Material& tmat, const KHR_materials_ior& ior)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_IOR_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_IOR_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_IOR_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "ior", ior.ior);
}


KHR_materials_iridescence tinygltf::utils::getIridescence(const tinygltf::Material& tmat)
{
  KHR_materials_iridescence gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_IRIDESCENCE_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_IRIDESCENCE_EXTENSION_NAME);

    tinygltf::utils::getValue(ext, "iridescenceFactor", gmat.iridescenceFactor);
    tinygltf::utils::getValue(ext, "iridescenceTexture", gmat.iridescenceTexture);
    tinygltf::utils::getValue(ext, "iridescenceIor", gmat.iridescenceIor);
    tinygltf::utils::getValue(ext, "iridescenceThicknessMinimum", gmat.iridescenceThicknessMinimum);
    tinygltf::utils::getValue(ext, "iridescenceThicknessMaximum", gmat.iridescenceThicknessMaximum);
    tinygltf::utils::getValue(ext, "iridescenceThicknessTexture", gmat.iridescenceThicknessTexture);
  }
  return gmat;
}

void tinygltf::utils::setIridescence(tinygltf::Material& tmat, const KHR_materials_iridescence& iridescence)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_IRIDESCENCE_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_IRIDESCENCE_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_IRIDESCENCE_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "iridescenceFactor", iridescence.iridescenceFactor);
  tinygltf::utils::setValue(ext, "iridescenceTexture", iridescence.iridescenceTexture);
  tinygltf::utils::setValue(ext, "iridescenceIor", iridescence.iridescenceIor);
  tinygltf::utils::setValue(ext, "iridescenceThicknessMinimum", iridescence.iridescenceThicknessMinimum);
  tinygltf::utils::setValue(ext, "iridescenceThicknessMaximum", iridescence.iridescenceThicknessMaximum);
  tinygltf::utils::setValue(ext, "iridescenceThicknessTexture", iridescence.iridescenceThicknessTexture);
}


KHR_materials_dispersion tinygltf::utils::getDispersion(const tinygltf::Material& tmat)
{
  KHR_materials_dispersion gmat;
  if(tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_DISPERSION_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(tmat.extensions, KHR_MATERIALS_DISPERSION_EXTENSION_NAME);
    tinygltf::utils::getValue(ext, "dispersion", gmat.dispersion);
  }
  return gmat;
}

void tinygltf::utils::setDispersion(tinygltf::Material& tmat, const KHR_materials_dispersion& dispersion)
{
  if(!tinygltf::utils::hasElementName(tmat.extensions, KHR_MATERIALS_DISPERSION_EXTENSION_NAME))
  {
    tmat.extensions[KHR_MATERIALS_DISPERSION_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = tmat.extensions[KHR_MATERIALS_DISPERSION_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "dispersion", dispersion.dispersion);
}

KHR_node_visibility tinygltf::utils::getNodeVisibility(const tinygltf::Node& node)
{
  KHR_node_visibility gnode;
  if(tinygltf::utils::hasElementName(node.extensions, KHR_NODE_VISIBILITY_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = tinygltf::utils::getElementValue(node.extensions, KHR_NODE_VISIBILITY_EXTENSION_NAME);
    tinygltf::utils::getValue(ext, "visible", gnode.visible);
  }
  return gnode;
}

void tinygltf::utils::setNodeVisibility(tinygltf::Node& node, const KHR_node_visibility& visibility)
{
  if(!tinygltf::utils::hasElementName(node.extensions, KHR_NODE_VISIBILITY_EXTENSION_NAME))
  {
    node.extensions[KHR_NODE_VISIBILITY_EXTENSION_NAME] = tinygltf::Value(tinygltf::Value::Object());
  }

  tinygltf::Value& ext = node.extensions[KHR_NODE_VISIBILITY_EXTENSION_NAME];
  tinygltf::utils::setValue(ext, "visible", visibility.visible);
}

tinygltf::Value tinygltf::utils::convertToTinygltfValue(int numElements, const float* elements)
{
  tinygltf::Value::Array result;
  result.reserve(numElements);

  for(int i = 0; i < numElements; ++i)
  {
    result.emplace_back(static_cast<double>(elements[i]));
  }

  return tinygltf::Value(result);
}

void tinygltf::utils::getNodeTRS(const tinygltf::Node& node, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale)
{
  // Initialize translation, rotation, and scale to default values
  translation = glm::vec3(0.0f, 0.0f, 0.0f);
  rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  scale       = glm::vec3(1.0f, 1.0f, 1.0f);

  // Check if the node has a matrix defined
  if(node.matrix.size() == 16)
  {
    glm::mat4 matrix = glm::make_mat4(node.matrix.data());
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(matrix, scale, rotation, translation, skew, perspective);
    return;
  }

  // Retrieve translation if available
  if(node.translation.size() == 3)
  {
    translation = glm::make_vec3(node.translation.data());
  }

  // Retrieve rotation if available
  if(node.rotation.size() == 4)
  {
    rotation.x = float(node.rotation[0]);
    rotation.y = float(node.rotation[1]);
    rotation.z = float(node.rotation[2]);
    rotation.w = float(node.rotation[3]);
  }

  // Retrieve scale if available
  if(node.scale.size() == 3)
  {
    scale = glm::make_vec3(node.scale.data());
  }
}

void tinygltf::utils::setNodeTRS(tinygltf::Node& node, const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
{
  node.translation = {translation.x, translation.y, translation.z};
  node.rotation    = {rotation.x, rotation.y, rotation.z, rotation.w};
  node.scale       = {scale.x, scale.y, scale.z};
}

glm::mat4 tinygltf::utils::getNodeMatrix(const tinygltf::Node& node)
{
  if(node.matrix.size() == 16)
  {
    return glm::make_mat4(node.matrix.data());
  }

  glm::vec3 translation;
  glm::quat rotation;
  glm::vec3 scale;
  getNodeTRS(node, translation, rotation, scale);

  return glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
}

std::string tinygltf::utils::generatePrimitiveKey(const tinygltf::Primitive& primitive)
{
  std::stringstream o;
  for(const auto& kv : primitive.attributes)
  {
    o << kv.first << ":" << kv.second << " ";
  }
  return o.str();
}

void tinygltf::utils::traverseSceneGraph(const tinygltf::Model&                            model,
                                         int                                               nodeID,
                                         const glm::mat4&                                  parentMat,
                                         const std::function<bool(int, const glm::mat4&)>& fctCam /*= nullptr*/,
                                         const std::function<bool(int, const glm::mat4&)>& fctLight /*= nullptr*/,
                                         const std::function<bool(int, const glm::mat4&)>& fctMesh /*= nullptr*/)
{
  const auto& node     = model.nodes[nodeID];
  glm::mat4   worldMat = parentMat * tinygltf::utils::getNodeMatrix(node);

  if(node.camera > -1 && fctCam && fctCam(nodeID, worldMat))
  {
    return;
  }
  if(node.light > -1 && fctLight && fctLight(nodeID, worldMat))
  {
    return;
  }
  if(node.mesh > -1 && fctMesh && fctMesh(nodeID, worldMat))
  {
    return;
  }

  for(const auto& child : node.children)
  {
    traverseSceneGraph(model, child, worldMat, fctCam, fctLight, fctMesh);
  }
}

size_t tinygltf::utils::getVertexCount(const tinygltf::Model& model, const tinygltf::Primitive& primitive)
{
  const tinygltf::Accessor& vertexAccessor = model.accessors.at(primitive.attributes.at("POSITION"));
  return vertexAccessor.count;
}

size_t tinygltf::utils::getIndexCount(const tinygltf::Model& model, const tinygltf::Primitive& primitive)
{
  if(primitive.indices > -1)
  {
    const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
    return indexAccessor.count;
  }
  // Return the vertex count when no indices are present
  return getVertexCount(model, primitive);
}

int tinygltf::utils::getTextureImageIndex(const tinygltf::Texture& texture)
{
  int source_image = texture.source;

  // MSFT_texture_dds: if the texture is a DDS file, we need to get the source image from the extension
  if(hasElementName(texture.extensions, MSFT_TEXTURE_DDS_NAME))
  {
    const tinygltf::Value& ext = getElementValue(texture.extensions, MSFT_TEXTURE_DDS_NAME);
    getValue(ext, "source", source_image);
  }

  // KHR_texture_basisu: if the texture has this extension, we need to get the source image from that extension.
  // glTF doesn't specify what happens if both KHR_texture_basisu and MSFT_texture_dds exist;
  // for now, we arbitrarily prefer the KTX source.
  if(hasElementName(texture.extensions, KHR_TEXTURE_BASISU_EXTENSION_NAME))
  {
    const tinygltf::Value& ext = getElementValue(texture.extensions, KHR_TEXTURE_BASISU_EXTENSION_NAME);
    getValue(ext, "source", source_image);
  }

  return source_image;
}
