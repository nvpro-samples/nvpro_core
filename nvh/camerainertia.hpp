/*
 * Copyright (c) 2013-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2013-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */
//--------------------------------------------------------------------
#pragma once

#include <nvh/nvprint.hpp>
#include <glm/glm.hpp>

#include <cmath>
#include "glm/gtc/matrix_transform.hpp"

//------------------------------------------------------------------------------------------
/// \struct InertiaCamera
/// \brief Struct that offers a camera moving with some inertia effect around a target point
///
/// InertiaCamera exposes a mix of pseudo polar rotation around a target point and
/// some other movements to translate the target point, zoom in and out.
///
/// Either the keyboard or mouse can be used for all of the moves.
//------------------------------------------------------------------------------------------
struct InertiaCamera
{
  glm::vec3 curEyePos, curFocusPos, curObjectPos;  ///< Current position of the motion
  glm::vec3 eyePos, focusPos, objectPos;           ///< expected posiions to reach
  float     tau;                                   ///< acceleration factor in the motion function
  float     epsilon;
  float     eyeD;
  float     focusD;
  float     objectD;
  glm::mat4 m4_view;  ///< transformation matrix resulting from the computation
  //------------------------------------------------------------------------------
  //
  //------------------------------------------------------------------------------
  InertiaCamera(const glm::vec3 eye    = glm::vec3(0.0f, 1.0f, -3.0f),
                const glm::vec3 focus  = glm::vec3(0, 0, 0),
                const glm::vec3 object = glm::vec3(0, 0, 0))
  {
    epsilon          = 0.001f;
    tau              = 0.2f;
    curEyePos        = eye;
    eyePos           = eye;
    curFocusPos      = focus;
    focusPos         = focus;
    curObjectPos     = object;
    objectPos        = object;
    eyeD             = 0.0f;
    focusD           = 0.0f;
    objectD          = 0.0f;
    m4_view          = glm::mat4(1);
    glm::mat4 Lookat = glm::lookAt(curEyePos, curFocusPos, glm::vec3(0, 1, 0));
    m4_view *= Lookat;
  }
  //------------------------------------------------------------------------------
  //
  //------------------------------------------------------------------------------
  void rotateH(float s, bool bPan = false)
  {
    glm::vec3 p  = eyePos;
    glm::vec3 o  = focusPos;
    glm::vec3 po = p - o;
    float     l  = glm::length(po);
    glm::vec3 dv = glm::cross(po, glm::vec3(0, 1, 0));
    dv *= s;
    p += dv;
    po       = p - o;
    float l2 = glm::length(po);
    l        = l2 - l;
    p -= (l / l2) * (po);
    eyePos = p;
    if(bPan)
      focusPos += dv;
  }
  //------------------------------------------------------------------------------
  //
  //------------------------------------------------------------------------------
  void rotateV(float s, bool bPan = false)
  {
    glm::vec3 p   = eyePos;
    glm::vec3 o   = focusPos;
    glm::vec3 po  = p - o;
    float     l   = glm::length(po);
    glm::vec3 dv  = glm::cross(po, glm::vec3(0, -1, 0));
    dv            = glm::normalize(dv);
    glm::vec3 dv2 = glm::cross(po, dv);
    dv2 *= s;
    p += dv2;
    po       = p - o;
    float l2 = glm::length(po);

    if(bPan)
      focusPos += dv2;

    // protect against gimbal lock
    if(std::fabs(dot(po / l2, glm::vec3(0, 1, 0))) > 0.99)
      return;

    l = l2 - l;
    p -= (l / l2) * (po);
    eyePos = p;
  }
  //------------------------------------------------------------------------------
  //
  //------------------------------------------------------------------------------
  void move(float s, bool bPan)
  {
    glm::vec3 p  = eyePos;
    glm::vec3 o  = focusPos;
    glm::vec3 po = p - o;
    po *= s;
    p -= po;
    if(bPan)
      focusPos -= po;
    eyePos = p;
  }
  //------------------------------------------------------------------------------------
  /// \brief simulation step to call with a proper time interval to update the animation
  //------------------------------------------------------------------------------------
  bool update(float dt)
  {
    if(dt > (1.0f / 60.0f))
      dt = (1.0f / 60.0f);
    bool             bContinue = false;
    static glm::vec3 eyeVel    = glm::vec3(0, 0, 0);
    static glm::vec3 eyeAcc    = glm::vec3(0, 0, 0);
    eyeD                       = glm::length(curEyePos - eyePos);
    if(eyeD > epsilon)
    {
      bContinue    = true;
      glm::vec3 dV = curEyePos - eyePos;
      eyeAcc       = (-2.0f / tau) * eyeVel - dV / (tau * tau);
      // integrate
      eyeVel += eyeAcc * glm::vec3(dt, dt, dt);
      curEyePos += eyeVel * glm::vec3(dt, dt, dt);
    }
    else
    {
      eyeVel = glm::vec3(0, 0, 0);
      eyeAcc = glm::vec3(0, 0, 0);
    }

    static glm::vec3 focusVel = glm::vec3(0, 0, 0);
    static glm::vec3 focusAcc = glm::vec3(0, 0, 0);
    focusD                    = glm::length(curFocusPos - focusPos);
    if(focusD > epsilon)
    {
      bContinue    = true;
      glm::vec3 dV = curFocusPos - focusPos;
      focusAcc     = (-2.0f / tau) * focusVel - dV / (tau * tau);
      // integrate
      focusVel += focusAcc * glm::vec3(dt, dt, dt);
      curFocusPos += focusVel * glm::vec3(dt, dt, dt);
    }
    else
    {
      focusVel = glm::vec3(0, 0, 0);
      focusAcc = glm::vec3(0, 0, 0);
    }

    static glm::vec3 objectVel = glm::vec3(0, 0, 0);
    static glm::vec3 objectAcc = glm::vec3(0, 0, 0);
    objectD                    = glm::length(curObjectPos - objectPos);
    if(objectD > epsilon)
    {
      bContinue    = true;
      glm::vec3 dV = curObjectPos - objectPos;
      objectAcc    = (-2.0f / tau) * objectVel - dV / (tau * tau);
      // integrate
      objectVel += objectAcc * glm::vec3(dt, dt, dt);
      curObjectPos += objectVel * glm::vec3(dt, dt, dt);
    }
    else
    {
      objectVel = glm::vec3(0, 0, 0);
      objectAcc = glm::vec3(0, 0, 0);
    }
    //
    // Camera View matrix
    //
    glm::vec3 up(0, 1, 0);
    m4_view          = glm::mat4(1);
    glm::mat4 Lookat = glm::lookAt(curEyePos, curFocusPos, up);
    m4_view *= Lookat;
    return bContinue;
  }
  //------------------------------------------------------------------------------
  /// \brief Call this function to update the camera position and targets position
  /// \arg *reset* set to true will directly update the actual positions without
  /// performing the animation for transitioning.
  //------------------------------------------------------------------------------
  void look_at(const glm::vec3& eye, const glm::vec3& center /*, const glm::vec3& up*/, bool reset = false)
  {
    eyePos   = eye;
    focusPos = center;
    if(reset)
    {
      curEyePos   = eye;
      curFocusPos = center;
      glm::vec3 up(0, 1, 0);
      m4_view          = glm::mat4(1);
      glm::mat4 Lookat = glm::lookAt(curEyePos, curFocusPos, up);
      m4_view *= Lookat;
    }
  }
  //------------------------------------------------------------------------------
  /// \brief debug information of camera position and target position
  /// Particularily useful to record a bunch of positions that can later be
  /// reuses as "recorded" presets
  //------------------------------------------------------------------------------
  void print_look_at(bool cppLike = false)
  {
    if(cppLike)
    {
      LOGI("{glm::vec3(%.2f, %.2f, %.2f), glm::vec3(%.2f, %.2f, %.2f)},\n", eyePos.x, eyePos.y, eyePos.z, focusPos.x,
           focusPos.y, focusPos.z);
    }
    else
    {
      LOGI("%.2f %.2f %.2f %.2f %.2f %.2f 0.0\n", eyePos.x, eyePos.y, eyePos.z, focusPos.x, focusPos.y, focusPos.z);
    }
  }
};
