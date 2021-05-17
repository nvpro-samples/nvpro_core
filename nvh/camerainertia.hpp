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
#include <nvmath/nvmath.h>

#include <cmath>

using namespace nvmath;

//-----------------------------------------------------------------------------
// Camera
//-----------------------------------------------------------------------------
struct InertiaCamera
{
  vec3f curEyePos, curFocusPos, curObjectPos;
  vec3f eyePos, focusPos, objectPos;
  float tau;
  float epsilon;
  float eyeD;
  float focusD;
  float objectD;
  mat4f m4_view;
  //------------------------------------------------------------------------------
  //
  //------------------------------------------------------------------------------
  InertiaCamera(const vec3f eye = vec3f(0.0f, 1.0f, -3.0f), const vec3f focus = vec3f(0, 0, 0), const vec3f object = vec3f(0, 0, 0))
  {
    epsilon      = 0.001f;
    tau          = 0.2f;
    curEyePos    = eye;
    eyePos       = eye;
    curFocusPos  = focus;
    focusPos     = focus;
    curObjectPos = object;
    objectPos    = object;
    eyeD         = 0.0f;
    focusD       = 0.0f;
    objectD      = 0.0f;
    m4_view.identity();
    mat4f Lookat = nvmath::look_at(curEyePos, curFocusPos, vec3f(0, 1, 0));
    m4_view *= Lookat;
  }
  //------------------------------------------------------------------------------
  //
  //------------------------------------------------------------------------------
  void rotateH(float s, bool bPan = false)
  {
    vec3f p  = eyePos;
    vec3f o  = focusPos;
    vec3f po = p - o;
    float l  = po.norm();
    vec3f dv = cross(po, vec3f(0, 1, 0));
    dv *= s;
    p += dv;
    po       = p - o;
    float l2 = po.norm();
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
    vec3f p  = eyePos;
    vec3f o  = focusPos;
    vec3f po = p - o;
    float l  = po.norm();
    vec3f dv = cross(po, vec3f(0, -1, 0));
    dv.normalize();
    vec3f dv2 = cross(po, dv);
    dv2 *= s;
    p += dv2;
    po       = p - o;
    float l2 = po.norm();

    if(bPan)
      focusPos += dv2;

    // protect against gimbal lock
    if(std::fabs(dot(po / l2, vec3f(0, 1, 0))) > 0.99)
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
    vec3f p  = eyePos;
    vec3f o  = focusPos;
    vec3f po = p - o;
    po *= s;
    p -= po;
    if(bPan)
      focusPos -= po;
    eyePos = p;
  }
  //------------------------------------------------------------------------------
  //
  //------------------------------------------------------------------------------
  bool update(float dt)
  {
    if(dt > (1.0f / 60.0f))
      dt = (1.0f / 60.0f);
    bool         bContinue = false;
    static vec3f eyeVel    = vec3f(0, 0, 0);
    static vec3f eyeAcc    = vec3f(0, 0, 0);
    eyeD                   = nv_norm(curEyePos - eyePos);
    if(eyeD > epsilon)
    {
      bContinue = true;
      vec3f dV  = curEyePos - eyePos;
      eyeAcc    = (-2.0f / tau) * eyeVel - dV / (tau * tau);
      // integrate
      eyeVel += eyeAcc * vec3f(dt, dt, dt);
      curEyePos += eyeVel * vec3f(dt, dt, dt);
    }
    else
    {
      eyeVel = vec3f(0, 0, 0);
      eyeAcc = vec3f(0, 0, 0);
    }

    static vec3f focusVel = vec3f(0, 0, 0);
    static vec3f focusAcc = vec3f(0, 0, 0);
    focusD                = nv_norm(curFocusPos - focusPos);
    if(focusD > epsilon)
    {
      bContinue = true;
      vec3f dV  = curFocusPos - focusPos;
      focusAcc  = (-2.0f / tau) * focusVel - dV / (tau * tau);
      // integrate
      focusVel += focusAcc * vec3f(dt, dt, dt);
      curFocusPos += focusVel * vec3f(dt, dt, dt);
    }
    else
    {
      focusVel = vec3f(0, 0, 0);
      focusAcc = vec3f(0, 0, 0);
    }

    static vec3f objectVel = vec3f(0, 0, 0);
    static vec3f objectAcc = vec3f(0, 0, 0);
    objectD                = nv_norm(curObjectPos - objectPos);
    if(objectD > epsilon)
    {
      bContinue = true;
      vec3f dV  = curObjectPos - objectPos;
      objectAcc = (-2.0f / tau) * objectVel - dV / (tau * tau);
      // integrate
      objectVel += objectAcc * vec3f(dt, dt, dt);
      curObjectPos += objectVel * vec3f(dt, dt, dt);
    }
    else
    {
      objectVel = vec3f(0, 0, 0);
      objectAcc = vec3f(0, 0, 0);
    }
    //
    // Camera View matrix
    //
    vec3f up(0, 1, 0);
    m4_view.identity();
    mat4f Lookat = nvmath::look_at(curEyePos, curFocusPos, up);
    m4_view *= Lookat;
    return bContinue;
  }
  //------------------------------------------------------------------------------
  //
  //------------------------------------------------------------------------------
  void look_at(const vec3f& eye, const vec3f& center /*, const vec3f& up*/, bool reset = false)
  {
    eyePos   = eye;
    focusPos = center;
    if(reset)
    {
      curEyePos   = eye;
      curFocusPos = center;
      vec3f up(0, 1, 0);
      m4_view.identity();
      mat4f Lookat = nvmath::look_at(curEyePos, curFocusPos, up);
      m4_view *= Lookat;
    }
  }
  //------------------------------------------------------------------------------
  //
  //------------------------------------------------------------------------------
  void print_look_at(bool cppLike = false)
  {
    if(cppLike)
    {
      LOGI("{vec3f(%.2f, %.2f, %.2f), vec3f(%.2f, %.2f, %.2f)},\n", eyePos.x, eyePos.y, eyePos.z, focusPos.x,
           focusPos.y, focusPos.z);
    }
    else
    {
      LOGI("%.2f %.2f %.2f %.2f %.2f %.2f 0.0\n", eyePos.x, eyePos.y, eyePos.z, focusPos.x, focusPos.y, focusPos.z);
    }
  }
};
