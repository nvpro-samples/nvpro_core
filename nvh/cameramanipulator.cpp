/*-----------------------------------------------------------------------
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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
*/ //--------------------------------------------------------------------

#include "cameramanipulator.hpp"
#include <chrono>
#include <nvpwindow.hpp>

namespace nvh {

inline float sign(float s)
{
  return (s < 0.f) ? -1.f : 1.f;
}

//--------------------------------------------------------------------------------------------------
//
//
CameraManipulator::CameraManipulator()
{
  update();
}

//--------------------------------------------------------------------------------------------------
// Creates a viewing matrix derived from an eye point, a reference point indicating the center of
// the scene, and an up vector
//
void CameraManipulator::setLookat(const nvmath::vec3& eye, const nvmath::vec3& center, const nvmath::vec3& up, bool instantSet)
{
  if(instantSet)
  {
    m_current.eye = eye;
    m_current.ctr = center;
    m_current.up  = up;
    m_goal        = m_current;
	m_start_time  = 0;
  }
  else
  {
    m_goal.eye = eye;
    m_goal.ctr = center;
    m_goal.up  = up;
    m_snapshot = m_current;
    m_start_time = getSystemTime();
    findBezierPoints();
  }
  update();
}

void CameraManipulator::updateAnim()
{
  auto elapse = static_cast<float>(getSystemTime() - m_start_time) / 1000.f;
  if(elapse > m_duration)
    return;

  float t = elapse / float(m_duration);
  // Evaluate polynomial (smoother step from Perlin)
  t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);

  // Interpolate camera position and interest
  // The distance of the camera between the interest is preserved to
  // create a nicer interpolation
  nvmath::vec3f vpos, vint, vup;
  m_current.ctr = nvmath::lerp(t, m_snapshot.ctr, m_goal.ctr);
  m_current.up  = nvmath::lerp(t, m_snapshot.up, m_goal.up);
  m_current.eye = computeBezier(t, m_bezier[0], m_bezier[1], m_bezier[2]);

  update();
}

//-----------------------------------------------------------------------------
// Get the current camera information
//   position  camera position
//   interest  camera interesting point (look at position)
//   up        camera up vector
//
void CameraManipulator::getLookat(nvmath::vec3& eye, nvmath::vec3& center, nvmath::vec3& up) const
{
  eye    = m_current.eye;
  center = m_current.ctr;
  up     = m_current.up;
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::setMode(Modes mode)
{
  m_mode = mode;
}

//--------------------------------------------------------------------------------------------------
//
//
CameraManipulator::Modes CameraManipulator::getMode() const
{
  return m_mode;
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::setRoll(float roll)
{
  m_roll = roll;
  update();
}

//--------------------------------------------------------------------------------------------------
//
//
float CameraManipulator::getRoll() const
{
  return m_roll;
}

//--------------------------------------------------------------------------------------------------
//
//
const nvmath::mat4& CameraManipulator::getMatrix() const
{
  return m_matrix;
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::setSpeed(float speed)
{
  m_speed = speed;
}

//--------------------------------------------------------------------------------------------------
//
//
float CameraManipulator::getSpeed()
{
  return m_speed;
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::setMousePosition(int x, int y)
{
  m_mouse[0] = static_cast<float>(x);
  m_mouse[1] = static_cast<float>(y);
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::getMousePosition(int& x, int& y)
{
  x = static_cast<int>(m_mouse[0]);
  y = static_cast<int>(m_mouse[1]);
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
    case CameraManipulator::Orbit:
      if(m_mode == Trackball)
        orbit(dx, dy, true);  // trackball(x, y);
      else
        orbit(dx, dy, false);
      break;
    case CameraManipulator::Dolly:
      dolly(dx, dy);
      break;
    case CameraManipulator::Pan:
      pan(dx, dy);
      break;
    case CameraManipulator::LookAround:
      if(m_mode == Trackball)
        trackball(x, y);
      else
        orbit(dx, -dy, true);
      break;
  }

  // Resetting animation
  m_start_time = 0;

  update();

  m_mouse[0] = static_cast<float>(x);
  m_mouse[1] = static_cast<float>(y);
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
    return None;  // no mouse button pressed
  }

  Actions curAction = None;
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

  if(curAction != None)
    motion(x, y, curAction);

  return curAction;
}

//--------------------------------------------------------------------------------------------------
// Trigger a dolly when the wheel change
//
void CameraManipulator::wheel(int value, const Inputs& inputs)
{
  float fval(static_cast<float>(value));
  float dx = (fval * fabsf(fval)) / static_cast<float>(m_width);

  if(inputs.shift)
  {
    m_fov += fval;
  }
  else
  {
    nvmath::vec3 z(m_current.eye - m_current.ctr);

    float length = z.norm() * 0.1f;
    length       = length < 0.001f ? 0.001f : length;

    dolly(dx * m_speed, dx * m_speed);
    update();
  }
}

//--------------------------------------------------------------------------------------------------
//
//
int CameraManipulator::getWidth() const
{
  return m_width;
}

//--------------------------------------------------------------------------------------------------
//
//
int CameraManipulator::getHeight() const
{
  return m_height;
}

//--------------------------------------------------------------------------------------------------
//
// Start trackball calculation
// Calculate the axis and the angle (radians) by the given mouse coordinates.
// Project the points onto the virtual trackball, then figure out the axis of rotation, which is the
// cross product of p0 p1 and O p0 (O is the center of the ball, 0,0,0)
//
// NOTE: This is a deformed trackball -- is a trackball in the center, but is deformed into a
// hyperbolic sheet of rotation away from the center.
//
void CameraManipulator::trackball(int x, int y)
{
  nvmath::vec2 p0(2 * (m_mouse[0] - m_width / 2) / double(m_width), 2 * (m_height / 2 - m_mouse[1]) / double(m_height));
  nvmath::vec2 p1(2 * (x - m_width / 2) / double(m_width), 2 * (m_height / 2 - y) / double(m_height));

  // determine the z coordinate on the sphere
  nvmath::vec3 pTB0(p0[0], p0[1], projectOntoTBSphere(p0));
  nvmath::vec3 pTB1(p1[0], p1[1], projectOntoTBSphere(p1));

  // calculate the rotation axis via cross product between p0 and p1
  nvmath::vec3 axis = nvmath::cross(pTB0, pTB1);
  axis              = nvmath::normalize(axis);

  // calculate the angle
  double t = nvmath::length(pTB0 - pTB1) / (2.f * m_tbsize);

  // clamp between -1 and 1
  if(t > 1.0)
    t = 1.0;
  else if(t < -1.0)
    t = -1.0;

  float rad = (float)(2.0 * asin(t));

  {
    nvmath::vec4 rot_axis = m_matrix * nvmath::vec4(axis, 0);
    nvmath::mat4 rot_mat  = nvmath::mat4f().as_rot(rad, nvmath::vec3(rot_axis.x, rot_axis.y, rot_axis.z));

    nvmath::vec3 pnt  = m_current.eye - m_current.ctr;
    nvmath::vec4 pnt2 = rot_mat * nvmath::vec4(pnt.x, pnt.y, pnt.z, 1);
    m_current.eye     = m_current.ctr + nvmath::vec3(pnt2.x, pnt2.y, pnt2.z);
    nvmath::vec4 up2  = rot_mat * nvmath::vec4(m_current.up.x, m_current.up.y, m_current.up.z, 0);
    m_current.up      = nvmath::vec3(up2.x, up2.y, up2.z);
  }
}

//--------------------------------------------------------------------------------------------------
// Project an x,y pair onto a sphere of radius r OR a hyperbolic sheet
// if we are away from the center of the sphere.
//
double CameraManipulator::projectOntoTBSphere(const nvmath::vec2& p)
{
  double z;
  double d = length(p);
  if(d < m_tbsize * 0.70710678118654752440)
  {
    // inside sphere
    z = sqrt(m_tbsize * m_tbsize - d * d);
  }
  else
  {
    // on hyperbola
    double t = m_tbsize / 1.41421356237309504880;
    z        = t * t / d;
  }

  return z;
}


nvmath::vec3f CameraManipulator::computeBezier(float t, nvmath::vec3f& p0, nvmath::vec3f& p1, nvmath::vec3f& p2)
{
  float u  = 1.f - t;
  float tt = t * t;
  float uu = u * u;

  nvmath::vec3f p = uu * p0;  // first term
  p += 2 * u * t * p1;        // second term
  p += tt * p2;               // third term

  return p;
}

void CameraManipulator::findBezierPoints()
{
  nvmath::vec3f p0 = m_current.eye;
  nvmath::vec3f p2 = m_goal.eye;
  nvmath::vec3f p1, pc;

  // point of interest
  nvmath::vec3f pi = (m_goal.ctr + m_current.ctr) * 0.5f;

  nvmath::vec3f p02    = (p0 + p2) * 0.5f;                            // mid p0-p2
  float         radius = (length(p0 - pi) + length(p2 - pi)) * 0.5f;  // Radius for p1
  nvmath::vec3f p02pi(p02 - pi);                                      // Vector from interest to mid point
  p02pi.normalize();
  p02pi *= radius;
  pc   = pi + p02pi;                        // Calculated point to go through
  p1   = 2.f * pc - p0 * 0.5f - p2 * 0.5f;  // Computing p1 for t=0.5
  p1.y = p02.y;                             // Clamping the P1 to be in the same height as p0-p2

  m_bezier[0] = p0;
  m_bezier[1] = p1;
  m_bezier[2] = p2;
}

//--------------------------------------------------------------------------------------------------
// Update the internal matrix.
//
void CameraManipulator::update()
{
  m_matrix = nvmath::look_at(m_current.eye, m_current.ctr, m_current.up);

  if(m_roll != 0.f)
  {
    nvmath::mat4 rot = nvmath::mat4f().as_rot(m_roll, nvmath::vec3(0, 0, 1));
    m_matrix         = m_matrix * rot;
  }
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

  nvmath::vec3 z(m_current.eye - m_current.ctr);
  float        length = static_cast<float>(nvmath::length(z)) / 0.785f;  // 45 degrees
  z                   = nvmath::normalize(z);
  nvmath::vec3 x      = nvmath::cross(m_current.up, z);
  x                   = nvmath::normalize(x);
  nvmath::vec3 y      = nvmath::cross(z, x);
  y                   = nvmath::normalize(y);
  x *= -dx * length;
  y *= dy * length;

  m_current.eye += x + y;
  m_current.ctr += x + y;
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
  dx *= nv_two_pi;
  dy *= nv_two_pi;

  // Get the camera
  nvmath::vec3 origin(invert ? m_current.eye : m_current.ctr);
  nvmath::vec3 position(invert ? m_current.ctr : m_current.eye);

  // Get the length of sight
  nvmath::vec3 centerToEye(position - origin);
  float        radius = nvmath::length(centerToEye);
  centerToEye         = nvmath::normalize(centerToEye);

  nvmath::mat4 rot_x, rot_y;

  // Find the rotation around the UP axis (Y)
  nvmath::vec3 axe_z(nvmath::normalize(centerToEye));
  rot_y = nvmath::mat4f().as_rot(-dx, m_current.up);

  // Apply the (Y) rotation to the eye-center vector
  nvmath::vec4 vect_tmp = rot_y * nvmath::vec4(centerToEye.x, centerToEye.y, centerToEye.z, 0);
  centerToEye           = nvmath::vec3(vect_tmp.x, vect_tmp.y, vect_tmp.z);

  // Find the rotation around the X vector: cross between eye-center and up (X)
  nvmath::vec3 axe_x = nvmath::cross(m_current.up, axe_z);
  axe_x              = nvmath::normalize(axe_x);
  rot_x              = nvmath::mat4f().as_rot(-dy, axe_x);

  // Apply the (X) rotation to the eye-center vector
  vect_tmp = rot_x * nvmath::vec4(centerToEye.x, centerToEye.y, centerToEye.z, 0);
  nvmath::vec3 vect_rot(vect_tmp.x, vect_tmp.y, vect_tmp.z);
  if(sign(vect_rot.x) == sign(centerToEye.x))
    centerToEye = vect_rot;

  // Make the vector as long as it was originally
  centerToEye *= radius;

  // Finding the new position
  nvmath::vec3 newPosition = centerToEye + origin;

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
  nvmath::vec3 z      = m_current.ctr - m_current.eye;
  float        length = static_cast<float>(nvmath::length(z));

  // We are at the point of interest, and don't know any direction, so do nothing!
  if(length < 0.000001f)
    return;

  // Use the larger movement.
  float dd;
  if(m_mode != Examine)
    dd = -dy;
  else
    dd = fabs(dx) > fabs(dy) ? dx : -dy;
  float factor = m_speed * dd / length;

  // Adjust speed based on distance.
  length /= 10;
  length = length < 0.001f ? 0.001f : length;
  factor *= length;

  // Don't move to or through the point of interest.
  if(factor >= 1.0f)
    return;

  z *= factor;

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

CameraManipulator::Inputs CameraManipulator::getInputs(const int& mouseButtonFlags, const bool* keyPressed)
{
  CameraManipulator::Inputs inputs;
  inputs.lmb   = !!(mouseButtonFlags & NVPWindow::MOUSE_BUTTONFLAG_LEFT);
  inputs.mmb   = !!(mouseButtonFlags & NVPWindow::MOUSE_BUTTONFLAG_MIDDLE);
  inputs.rmb   = !!(mouseButtonFlags & NVPWindow::MOUSE_BUTTONFLAG_RIGHT);
  inputs.ctrl  = keyPressed[NVPWindow::KEY_LEFT_CONTROL];
  inputs.shift = keyPressed[NVPWindow::KEY_LEFT_SHIFT];
  inputs.alt   = keyPressed[NVPWindow::KEY_LEFT_ALT];
  return inputs;
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

}  // namespace nvh
