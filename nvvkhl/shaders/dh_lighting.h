/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DH_LIGHTING_H
#define DH_LIGHTING_H 1

#ifdef __cplusplus
namespace nvvkhl_shaders {

using vec3 = glm::vec3;
#endif  // __cplusplus

const int eLightTypeNone        = 0;
const int eLightTypeDirectional = 1;
const int eLightTypeSpot        = 2;
const int eLightTypePoint       = 3;

//-----------------------------------------------------------------------
// Use for light/env contribution
struct VisibilityContribution
{
  vec3  radiance;   // Radiance at the point if light is visible
  vec3  lightDir;   // Direction to the light, to shoot shadow ray
  float lightDist;  // Distance to the light (1e32 for infinite or sky)
  bool  visible;    // true if in front of the face and should shoot shadow ray
};

struct LightContrib
{
  vec3  incidentVector;
  float halfAngularSize;
  vec3  intensity;
};

struct Light
{
  vec3 direction;
  int  type;

  vec3  position;
  float radius;

  vec3  color;
  float intensity;  // illuminance (lm/m2) for directional lights, luminous intensity (lm/sr) for positional lights

  float angularSizeOrInvRange;  // angular size for directional lights, 1/range for spot and point lights
  float innerAngle;
  float outerAngle;
  float outOfBoundsShadow;
};

#ifdef __cplusplus
inline Light defaultLight()
{
  Light l;
  l.position              = vec3{5.0F, 5.F, 5.F};
  l.direction             = glm::normalize(vec3{0.0F, -.7F, -.7F});
  l.type                  = eLightTypeDirectional;
  l.angularSizeOrInvRange = glm::radians(0.53F);
  l.color                 = {1.0F, 1.0F, 1.0F};
  l.intensity             = 0.F;  // Dark
  l.innerAngle            = glm::radians(10.F);
  l.outerAngle            = glm::radians(30.F);
  l.radius                = 1.0F;

  return l;
}
#endif  //__cplusplus

#ifdef __cplusplus
}  // namespace nvvkhl_shaders
#endif

#endif  // DH_LIGHTING_H
