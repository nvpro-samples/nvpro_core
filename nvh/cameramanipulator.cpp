/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2018-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */
//--------------------------------------------------------------------

#include "cameramanipulator.hpp"
#include <chrono>
#include <iostream>
#include <nvpwindow.hpp>

namespace nvh {

//--------------------------------------------------------------------------------------------------
//
//
CameraManipulator::CameraManipulator()
{
  update();
}

//--------------------------------------------------------------------------------------------------
// Set the new camera as a goal
//
void CameraManipulator::setCamera(Camera camera, bool instantSet /*=true*/)
{
  m_anim_done = true;

  if(instantSet)
  {
    m_current = camera;
    update();
  }
  else if(camera != m_current)
  {
    m_goal       = camera;
    m_snapshot   = m_current;
    m_anim_done  = false;
    m_start_time = getSystemTime();
    findBezierPoints();
  }
}

//--------------------------------------------------------------------------------------------------
// Creates a viewing matrix derived from an eye point, a reference point indicating the center of
// the scene, and an up vector
//
void CameraManipulator::setLookat(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool instantSet)
{
  Camera camera{eye, center, up, m_current.fov};
  setCamera(camera, instantSet);
}

//-----------------------------------------------------------------------------
// Get the current camera's look-at parameters.
void CameraManipulator::getLookat(glm::vec3& eye, glm::vec3& center, glm::vec3& up) const
{
  eye    = m_current.eye;
  center = m_current.ctr;
  up     = m_current.up;
}

//--------------------------------------------------------------------------------------------------
// Pan the camera perpendicularly to the light of sight.
//
void CameraManipulator::pan(float dx, float dy)
{
  if(m_mode == Fly)
  {
    dx *= -1;
    dy *= -1;
  }

  glm::vec3 z(m_current.eye - m_current.ctr);
  float     length = static_cast<float>(glm::length(z)) / 0.785f;  // 45 degrees
  z                = glm::normalize(z);
  glm::vec3 x      = glm::cross(m_current.up, z);
  glm::vec3 y      = glm::cross(z, x);
  x                = glm::normalize(x);
  y                = glm::normalize(y);

  glm::vec3 panVector = (-dx * x + dy * y) * length;
  m_current.eye += panVector;
  m_current.ctr += panVector;
}

//--------------------------------------------------------------------------------------------------
// Orbit the camera around the center of interest. If 'invert' is true,
// then the camera stays in place and the interest orbit around the camera.
//
void CameraManipulator::orbit(float dx, float dy, bool invert)
{
  if(dx == 0 && dy == 0)
    return;

  // Full width will do a full turn
  dx *= glm::two_pi<float>();
  dy *= glm::two_pi<float>();

  // Get the camera
  glm::vec3 origin(invert ? m_current.eye : m_current.ctr);
  glm::vec3 position(invert ? m_current.ctr : m_current.eye);

  // Get the length of sight
  glm::vec3 centerToEye(position - origin);
  float     radius = glm::length(centerToEye);
  centerToEye      = glm::normalize(centerToEye);
  glm::vec3 axe_z  = centerToEye;

  // Find the rotation around the UP axis (Y)
  glm::mat4 rot_y = glm::rotate(glm::mat4(1), -dx, m_current.up);

  // Apply the (Y) rotation to the eye-center vector
  centerToEye = rot_y * glm::vec4(centerToEye, 0);

  // Find the rotation around the X vector: cross between eye-center and up (X)
  glm::vec3 axe_x = glm::normalize(glm::cross(m_current.up, axe_z));
  glm::mat4 rot_x = glm::rotate(glm::mat4(1), -dy, axe_x);

  // Apply the (X) rotation to the eye-center vector
  glm::vec3 vect_rot = rot_x * glm::vec4(centerToEye, 0);

  if(glm::sign(vect_rot.x) == glm::sign(centerToEye.x))
    centerToEye = vect_rot;

  // Make the vector as long as it was originally
  centerToEye *= radius;

  // Finding the new position
  glm::vec3 newPosition = centerToEye + origin;

  if(!invert)
  {
    m_current.eye = newPosition;  // Normal: change the position of the camera
  }
  else
  {
    m_current.ctr = newPosition;  // Inverted: change the interest point
  }
}

//--------------------------------------------------------------------------------------------------
// Move the camera toward the interest point, but don't cross it
//
void CameraManipulator::dolly(float dx, float dy)
{
  glm::vec3 z      = m_current.ctr - m_current.eye;
  float     length = static_cast<float>(glm::length(z));

  // We are at the point of interest, and don't know any direction, so do nothing!
  if(length < 0.000001f)
    return;

  // Use the larger movement.
  float dd;
  if(m_mode != Examine)
    dd = -dy;
  else
    dd = fabs(dx) > fabs(dy) ? dx : -dy;
  float factor = m_speed * dd;

  // Adjust speed based on distance.
  if(m_mode == Examine)
  {
    // Don't move over the point of interest.
    if(factor >= 1.0f)
      return;

    z *= factor;
  }
  else
  {
    // Normalize the Z vector and make it faster
    z *= factor / length * 10.0f;
  }

  // Not going up
  if(m_mode == Walk)
  {
    if(m_current.up.y > m_current.up.z)
      z.y = 0;
    else
      z.z = 0;
  }

  m_current.eye += z;

  // In fly mode, the interest moves with us.
  if(m_mode != Examine)
    m_current.ctr += z;
}

//--------------------------------------------------------------------------------------------------
// Modify the position of the camera over time
// - The camera can be updated through keys. A key set a direction which is added to both
//   eye and center, until the key is released
// - A new position of the camera is defined and the camera will reach that position
//   over time.
void CameraManipulator::updateAnim()
{
  auto elapse = static_cast<float>(getSystemTime() - m_start_time) / 1000.f;

  // Key animation
  if(m_key_vec != glm::vec3(0, 0, 0))
  {
    m_current.eye += m_key_vec * elapse;
    m_current.ctr += m_key_vec * elapse;
    update();
    m_start_time = getSystemTime();
    return;
  }

  // Camera moving to new position
  if(m_anim_done)
    return;

  float t = std::min(elapse / float(m_duration), 1.0f);
  // Evaluate polynomial (smoother step from Perlin)
  t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
  if(t >= 1.0f)
  {
    m_current   = m_goal;
    m_anim_done = true;
    return;
  }

  // Interpolate camera position and interest
  // The distance of the camera between the interest is preserved to
  // create a nicer interpolation
  m_current.ctr = glm::mix(m_snapshot.ctr, m_goal.ctr, t);
  m_current.up  = glm::mix(m_snapshot.up, m_goal.up, t);
  m_current.eye = computeBezier(t, m_bezier[0], m_bezier[1], m_bezier[2]);
  m_current.fov = glm::mix(m_snapshot.fov, m_goal.fov, t);

  update();
}

//--------------------------------------------------------------------------------------------------
//
void CameraManipulator::setMatrix(const glm::mat4& matrix, bool instantSet, float centerDistance)
{
  Camera camera;
  camera.eye = matrix[3];

  auto rotMat = glm::mat3(matrix);
  camera.ctr  = {0, 0, -centerDistance};
  camera.ctr  = camera.eye + (rotMat * camera.ctr);
  camera.up   = {0, 1, 0};
  camera.fov  = m_current.fov;

  m_anim_done = instantSet;

  if(instantSet)
  {
    m_current = camera;
  }
  else
  {
    m_goal       = camera;
    m_snapshot   = m_current;
    m_start_time = getSystemTime();
    findBezierPoints();
  }
  update();
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::setMousePosition(int x, int y)
{
  m_mouse = glm::vec2(x, y);
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::getMousePosition(int& x, int& y)
{
  x = static_cast<int>(m_mouse.x);
  y = static_cast<int>(m_mouse.y);
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::setWindowSize(int w, int h)
{
  m_width  = w;
  m_height = h;
}

//--------------------------------------------------------------------------------------------------
//
// Low level function for when the camera move.
//
void CameraManipulator::motion(int x, int y, int action)
{
  float dx = float(x - m_mouse[0]) / float(m_width);
  float dy = float(y - m_mouse[1]) / float(m_height);

  switch(action)
  {
    case Orbit:
      orbit(dx, dy, false);
      break;
    case CameraManipulator::Dolly:
      dolly(dx, dy);
      break;
    case CameraManipulator::Pan:
      pan(dx, dy);
      break;
    case CameraManipulator::LookAround:
      orbit(dx, -dy, true);
      break;
  }

  // Resetting animation
  m_anim_done = true;

  update();

  m_mouse[0] = static_cast<float>(x);
  m_mouse[1] = static_cast<float>(y);
}

//
// Function for when the camera move with keys (ex. WASD).
//
void CameraManipulator::keyMotion(float dx, float dy, int action)
{
  if(action == NoAction)
  {
    m_key_vec = {0, 0, 0};
    return;
  }

  auto d = glm::normalize(m_current.ctr - m_current.eye);
  dx *= m_speed * 2.f;
  dy *= m_speed * 2.f;

  glm::vec3 key_vec;
  if(action == Dolly)
  {
    key_vec = d * dx;
    if(m_mode == Walk)
    {
      if(m_current.up.y > m_current.up.z)
        key_vec.y = 0;
      else
        key_vec.z = 0;
    }
  }
  else if(action == Pan)
  {
    auto r  = glm::cross(d, m_current.up);
    key_vec = r * dx + m_current.up * dy;
  }

  m_key_vec += key_vec;

  // Resetting animation
  m_start_time = getSystemTime();
}

//--------------------------------------------------------------------------------------------------
// To call when the mouse is moving
// It find the appropriate camera operator, based on the mouse button pressed and the
// keyboard modifiers (shift, ctrl, alt)
//
// Returns the action that was activated
//
CameraManipulator::Actions CameraManipulator::mouseMove(int x, int y, const Inputs& inputs)
{
  if(!inputs.lmb && !inputs.rmb && !inputs.mmb)
  {
    setMousePosition(x, y);
    return NoAction;  // no mouse button pressed
  }

  Actions curAction = NoAction;
  if(inputs.lmb)
  {
    if(((inputs.ctrl) && (inputs.shift)) || inputs.alt)
      curAction = m_mode == Examine ? LookAround : Orbit;
    else if(inputs.shift)
      curAction = Dolly;
    else if(inputs.ctrl)
      curAction = Pan;
    else
      curAction = m_mode == Examine ? Orbit : LookAround;
  }
  else if(inputs.mmb)
    curAction = Pan;
  else if(inputs.rmb)
    curAction = Dolly;

  if(curAction != NoAction)
    motion(x, y, curAction);

  return curAction;
}

//--------------------------------------------------------------------------------------------------
// Trigger a dolly when the wheel change, or change the FOV if the shift key was pressed
//
void CameraManipulator::wheel(int value, const Inputs& inputs)
{
  float fval(static_cast<float>(value));
  float dx = (fval * fabsf(fval)) / static_cast<float>(m_width);

  if(inputs.shift)
  {
    setFov(m_current.fov + fval);
  }
  else
  {
    dolly(dx * m_speed, dx * m_speed);
    update();
  }
}

// Set and clamp FOV between 0.01 and 179 degrees
void CameraManipulator::setFov(float _fov)
{
  m_current.fov = std::min(std::max(_fov, 0.01f), 179.0f);
}

glm::vec3 CameraManipulator::computeBezier(float t, glm::vec3& p0, glm::vec3& p1, glm::vec3& p2)
{
  float u  = 1.f - t;
  float tt = t * t;
  float uu = u * u;

  glm::vec3 p = uu * p0;  // first term
  p += 2 * u * t * p1;    // second term
  p += tt * p2;           // third term

  return p;
}

void CameraManipulator::findBezierPoints()
{
  glm::vec3 p0 = m_current.eye;
  glm::vec3 p2 = m_goal.eye;
  glm::vec3 p1, pc;

  // point of interest
  glm::vec3 pi = (m_goal.ctr + m_current.ctr) * 0.5f;

  glm::vec3 p02    = (p0 + p2) * 0.5f;                            // mid p0-p2
  float     radius = (length(p0 - pi) + length(p2 - pi)) * 0.5f;  // Radius for p1
  glm::vec3 p02pi(p02 - pi);                                      // Vector from interest to mid point
  p02pi = glm::normalize(p02pi);
  p02pi *= radius;
  pc   = pi + p02pi;                        // Calculated point to go through
  p1   = 2.f * pc - p0 * 0.5f - p2 * 0.5f;  // Computing p1 for t=0.5
  p1.y = p02.y;                             // Clamping the P1 to be in the same height as p0-p2

  m_bezier[0] = p0;
  m_bezier[1] = p1;
  m_bezier[2] = p2;
}

//--------------------------------------------------------------------------------------------------
// Return the time in fraction of milliseconds
//
double CameraManipulator::getSystemTime()
{
  auto now(std::chrono::system_clock::now());
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000.0;
}

//--------------------------------------------------------------------------------------------------
// Return a string which can be included in help dialogs
//
const std::string& CameraManipulator::getHelp()
{
  static std::string helpText =
      "LMB: rotate around the target\n"
      "RMB: Dolly in/out\n"
      "MMB: Pan along view plane\n"
      "LMB + Shift: Dolly in/out\n"
      "LMB + Ctrl: Pan\n"
      "LMB + Alt: Look aroundPan\n"
      "Mouse wheel: Dolly in/out\n"
      "Mouse wheel + Shift: Zoom in/out\n";
  return helpText;
}

//--------------------------------------------------------------------------------------------------
// Move the camera closer or further from the center of the the bounding box, to see it completely
//
// boxMin - lower corner of the bounding box
// boxMax - upper corner of the bounding box
// instantFit - true: set the new position, false: will animate to new position.
// tight - true: fit exactly the corner, false: fit to radius (larger view, will not get closer or further away)
// aspect - aspect ratio of the window.
//
void CameraManipulator::fit(const glm::vec3& boxMin, const glm::vec3& boxMax, bool instantFit /*= true*/, bool tightFit /*=false*/, float aspect /*=1.0f*/)
{
  // Calculate the half extents of the bounding box
  const glm::vec3 boxHalfSize = 0.5f * (boxMax - boxMin);

  // Calculate the center of the bounding box
  const glm::vec3 boxCenter = 0.5f * (boxMin + boxMax);

  const float yfov = tan(glm::radians(m_current.fov * 0.5f));
  const float xfov = yfov * aspect;

  // Calculate the ideal distance for a tight fit or fit to radius
  float idealDistance = 0;

  if(tightFit)
  {
    // Get only the rotation matrix
    glm::mat3 mView = glm::lookAt(m_current.eye, boxCenter, m_current.up);

    // Check each 8 corner of the cube
    for(int i = 0; i < 8; i++)
    {
      // Rotate the bounding box in the camera view
      glm::vec3 vct(i & 1 ? boxHalfSize.x : -boxHalfSize.x,   //
                    i & 2 ? boxHalfSize.y : -boxHalfSize.y,   //
                    i & 4 ? boxHalfSize.z : -boxHalfSize.z);  //
      vct = mView * vct;

      if(vct.z < 0)  // Take only points in front of the center
      {
        // Keep the largest offset to see that vertex
        idealDistance = std::max(fabs(vct.y) / yfov + fabs(vct.z), idealDistance);
        idealDistance = std::max(fabs(vct.x) / xfov + fabs(vct.z), idealDistance);
      }
    }
  }
  else  // Using the bounding sphere
  {
    const float radius = glm::length(boxHalfSize);
    idealDistance      = std::max(radius / xfov, radius / yfov);
  }

  // Calculate the new camera position based on the ideal distance
  const glm::vec3 newEye = boxCenter - idealDistance * glm::normalize(boxCenter - m_current.eye);

  // Set the new camera position and interest point
  setLookat(newEye, boxCenter, m_current.up, instantFit);
}

}  // namespace nvh
