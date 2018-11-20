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
#include "main.h"

namespace nv_helpers {

inline float sign( float s )
{
  return ( s < 0.f ) ? -1.f : 1.f;
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
void CameraManipulator::setLookat( const nv_math::vec3& eye, const nv_math::vec3& center, const nv_math::vec3& up )
{
  m_pos = eye;
  m_int = center;
  m_up  = up;
  update();
}

//-----------------------------------------------------------------------------
// Get the current camera information
//   position  camera position
//   interest  camera interesting point (look at position)
//   up        camera up vector
//
void CameraManipulator::getLookat( nv_math::vec3& eye, nv_math::vec3& center, nv_math::vec3& up ) const
{
  eye    = m_pos;
  center = m_int;
  up     = m_up;
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::setMode( Modes mode )
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
void CameraManipulator::setRoll( float roll )
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
const nv_math::mat4& CameraManipulator::getMatrix() const
{
  return m_matrix;
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::setSpeed( float speed )
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
void CameraManipulator::setMousePosition( int x, int y )
{
  m_mouse[0] = static_cast<float>( x );
  m_mouse[1] = static_cast<float>( y );
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::getMousePosition( int& x, int& y )
{
  x = static_cast<int>( m_mouse[0] );
  y = static_cast<int>( m_mouse[1] );
}

//--------------------------------------------------------------------------------------------------
//
//
void CameraManipulator::setWindowSize( int w, int h )
{
  m_width  = w;
  m_height = h;
}

//--------------------------------------------------------------------------------------------------
//
// Low level function for when the camera move.
//
void CameraManipulator::motion( int x, int y, int action )
{
  float dx = float( x - m_mouse[0] ) / float( m_width );
  float dy = float( y - m_mouse[1] ) / float( m_height );

  switch( action )
  {
    case CameraManipulator::Orbit:
      if( m_mode == Trackball )
        orbit( dx, dy, true );  // trackball(x, y);
      else
        orbit( dx, dy, false );
      break;
    case CameraManipulator::Dolly:
      dolly( dx, dy );
      break;
    case CameraManipulator::Pan:
      pan( dx, dy );
      break;
    case CameraManipulator::LookAround:
      if( m_mode == Trackball )
        trackball( x, y );
      else
        orbit( dx, -dy, true );
      break;
  }

  update();

  m_mouse[0] = static_cast<float>( x );
  m_mouse[1] = static_cast<float>( y );
}

//--------------------------------------------------------------------------------------------------
// To call when the mouse is moving
// It find the appropriate camera operator, based on the mouse button pressed and the
// keyboard modifiers (shift, ctrl, alt)
//
// Returns the action that was activated
//
CameraManipulator::Actions CameraManipulator::mouseMove( int x, int y, const Inputs& inputs )
{
  if( !inputs.lmb && !inputs.rmb && !inputs.mmb )
  {
    setMousePosition( x, y );
    return None;  // no mouse button pressed
  }

  Actions curAction = None;
  if( inputs.lmb )
  {
    if( ( ( inputs.ctrl ) && ( inputs.shift ) ) || inputs.alt )
      curAction = m_mode == Examine ? LookAround : Orbit;
    else if( inputs.shift )
      curAction = Dolly;
    else if( inputs.ctrl )
      curAction = Pan;
    else
      curAction = m_mode == Examine ? Orbit : LookAround;
  }
  else if( inputs.mmb )
    curAction = Pan;
  else if( inputs.rmb )
    curAction = Dolly;

  if( curAction != None )
    motion( x, y, curAction );

  return curAction;
}

//--------------------------------------------------------------------------------------------------
// Trigger a dolly when the wheel change
//
void CameraManipulator::wheel( int value, const Inputs& inputs )
{
  float fval( static_cast<float>( value ) );
  float dx = ( fval * fabs( fval ) ) / static_cast<float>( m_width );

  if( inputs.shift )
  {
    m_fov += fval;
  }
  else
  {
    nv_math::vec3 z( m_pos - m_int );

    float length = z.norm() * 0.1f;
    length       = length < 0.001f ? 0.001f : length;

    dolly( dx * m_speed, dx * m_speed );
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
void CameraManipulator::trackball( int x, int y )
{
  nv_math::vec2 p0( 2 * ( m_mouse[0] - m_width / 2 ) / double( m_width ), 2 * ( m_height / 2 - m_mouse[1] ) / double( m_height ) );
  nv_math::vec2 p1( 2 * ( x - m_width / 2 ) / double( m_width ), 2 * ( m_height / 2 - y ) / double( m_height ) );

  // determine the z coordinate on the sphere
  nv_math::vec3 pTB0( p0[0], p0[1], projectOntoTBSphere( p0 ) );
  nv_math::vec3 pTB1( p1[0], p1[1], projectOntoTBSphere( p1 ) );

  // calculate the rotation axis via cross product between p0 and p1
  nv_math::vec3 axis = nv_math::cross( pTB0, pTB1 );
  axis               = nv_math::normalize( axis );

  // calculate the angle
  double t = nv_math::length( pTB0 - pTB1 ) / ( 2.f * m_tbsize );

  // clamp between -1 and 1
  if( t > 1.0 )
    t = 1.0;
  else if( t < -1.0 )
    t = -1.0;

  float rad = (float)( 2.0 * asin( t ) );

  {
    nv_math::vec4 rot_axis = m_matrix * nv_math::vec4( axis, 0 );
    nv_math::mat4 rot_mat  = nv_math::mat4f().as_rot( rad, nv_math::vec3( rot_axis.x, rot_axis.y, rot_axis.z ) );

    nv_math::vec3 pnt  = m_pos - m_int;
    nv_math::vec4 pnt2 = rot_mat * nv_math::vec4( pnt.x, pnt.y, pnt.z, 1 );
    m_pos              = m_int + nv_math::vec3( pnt2.x, pnt2.y, pnt2.z );
    nv_math::vec4 up2  = rot_mat * nv_math::vec4( m_up.x, m_up.y, m_up.z, 0 );
    m_up               = nv_math::vec3( up2.x, up2.y, up2.z );
  }
}

//--------------------------------------------------------------------------------------------------
// Project an x,y pair onto a sphere of radius r OR a hyperbolic sheet
// if we are away from the center of the sphere.
//
double CameraManipulator::projectOntoTBSphere( const nv_math::vec2& p )
{
  double z;
  double d = length( p );
  if( d < m_tbsize * 0.70710678118654752440 )
  {
    // inside sphere
    z = sqrt( m_tbsize * m_tbsize - d * d );
  }
  else
  {
    // on hyperbola
    double t = m_tbsize / 1.41421356237309504880;
    z        = t * t / d;
  }

  return z;
}

//--------------------------------------------------------------------------------------------------
// Update the internal matrix.
//
void CameraManipulator::update()
{
  m_matrix = nv_math::look_at( m_pos, m_int, m_up );

  if( m_roll != 0.f )
  {
    nv_math::mat4 rot = nv_math::mat4f().as_rot( m_roll, nv_math::vec3( 0, 0, 1 ) );
    m_matrix          = m_matrix * rot;
  }
}

//--------------------------------------------------------------------------------------------------
// Pan the camera perpendicularly to the light of sight.
//
void CameraManipulator::pan( float dx, float dy )
{
  if( m_mode == Fly )
  {
    dx *= -1;
    dy *= -1;
  }

  nv_math::vec3 z( m_pos - m_int );
  float         length = static_cast<float>( nv_math::length( z ) ) / 0.785f;  // 45 degrees
  z                    = nv_math::normalize( z );
  nv_math::vec3 x      = nv_math::cross( m_up, z );
  x                    = nv_math::normalize( x );
  nv_math::vec3 y      = nv_math::cross( z, x );
  y                    = nv_math::normalize( y );
  x *= -dx * length;
  y *= dy * length;

  m_pos += x + y;
  m_int += x + y;
}

//--------------------------------------------------------------------------------------------------
// Orbit the camera around the center of interest. If 'invert' is true,
// then the camera stays in place and the interest orbit around the camera.
//
void CameraManipulator::orbit( float dx, float dy, bool invert )
{
  if( dx == 0 && dy == 0 )
    return;

  // Full width will do a full turn
  dx *= nv_two_pi;
  dy *= nv_two_pi;

  // Get the camera
  nv_math::vec3 origin( invert ? m_pos : m_int );
  nv_math::vec3 position( invert ? m_int : m_pos );

  // Get the length of sight
  nv_math::vec3 centerToEye( position - origin );
  float         radius = nv_math::length( centerToEye );
  centerToEye          = nv_math::normalize( centerToEye );

  nv_math::mat4 rot_x, rot_y;

  // Find the rotation around the UP axis (Y)
  nv_math::vec3 axe_z( nv_math::normalize( centerToEye ) );
  rot_y = nv_math::mat4f().as_rot( -dx, m_up );

  // Apply the (Y) rotation to the eye-center vector
  nv_math::vec4 vect_tmp = rot_y * nv_math::vec4( centerToEye.x, centerToEye.y, centerToEye.z, 0 );
  centerToEye            = nv_math::vec3( vect_tmp.x, vect_tmp.y, vect_tmp.z );

  // Find the rotation around the X vector: cross between eye-center and up (X)
  nv_math::vec3 axe_x = nv_math::cross( m_up, axe_z );
  axe_x               = nv_math::normalize( axe_x );
  rot_x               = nv_math::mat4f().as_rot( -dy, axe_x );

  // Apply the (X) rotation to the eye-center vector
  vect_tmp = rot_x * nv_math::vec4( centerToEye.x, centerToEye.y, centerToEye.z, 0 );
  nv_math::vec3 vect_rot( vect_tmp.x, vect_tmp.y, vect_tmp.z );
  if( sign( vect_rot.x ) == sign( centerToEye.x ) )
    centerToEye = vect_rot;

  // Make the vector as long as it was originally
  centerToEye *= radius;

  // Finding the new position
  nv_math::vec3 newPosition = centerToEye + origin;

  if( !invert )
  {
    m_pos = newPosition;  // Normal: change the position of the camera
  }
  else
  {
    m_int = newPosition;  // Inverted: change the interest point
  }
}

//--------------------------------------------------------------------------------------------------
// Move the camera toward the interest point, but don't cross it
//
void CameraManipulator::dolly( float dx, float dy )
{
  nv_math::vec3 z      = m_int - m_pos;
  float         length = static_cast<float>( nv_math::length( z ) );

  // We are at the point of interest, and don't know any direction, so do nothing!
  if( length < 0.000001f )
    return;

  // Use the larger movement.
  float dd;
  if( m_mode != Examine )
    dd = -dy;
  else
    dd = fabs( dx ) > fabs( dy ) ? dx : -dy;
  float factor = m_speed * dd / length;

  // Adjust speed based on distance.
  length /= 10;
  length = length < 0.001f ? 0.001f : length;
  factor *= length;

  // Don't move to or through the point of interest.
  if( factor >= 1.0f )
    return;

  z *= factor;

  // Not going up
  if( m_mode == Walk )
  {
    if( m_up.y > m_up.z )
      z.y = 0;
    else
      z.z = 0;
  }

  m_pos += z;

  // In fly mode, the interest moves with us.
  if( m_mode != Examine )
    m_int += z;
}

CameraManipulator::Inputs CameraManipulator::getInputs( const int& mouseButtonFlags, const bool* keyPressed )
{
  CameraManipulator::Inputs inputs;
  inputs.lmb   = !!( mouseButtonFlags & NVPWindow::MOUSE_BUTTONFLAG_LEFT );
  inputs.mmb   = !!( mouseButtonFlags & NVPWindow::MOUSE_BUTTONFLAG_MIDDLE );
  inputs.rmb   = !!( mouseButtonFlags & NVPWindow::MOUSE_BUTTONFLAG_RIGHT );
  inputs.ctrl  = keyPressed[NVPWindow::KEY_LEFT_CONTROL];
  inputs.shift = keyPressed[NVPWindow::KEY_LEFT_SHIFT];
  inputs.alt   = keyPressed[NVPWindow::KEY_LEFT_ALT];
  return inputs;
}

}  // namespace nv_helpers
