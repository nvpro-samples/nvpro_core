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

#pragma once

#pragma warning(disable : 4201)
#include <nvmath/nvmath.h>
#include <nvmath/nvmath_glsltypes.h>


namespace nvh {
/**
  # class nvh::CameraManipulator

  This is a camera manipulator help class
  It allow to simply do
  - Orbit        (LMB)
  - Pan          (LMB + CTRL  | MMB)
  - Dolly        (LMB + SHIFT | RMB)
  - Look Around  (LMB + ALT   | LMB + CTRL + SHIFT)
  - Trackball

  In a various ways:
  - examiner(orbit around object)
  - walk (look up or down but stays on a plane)
  - fly ( go toward the interest point)

  Do use the camera manipulator, you need to do the following
  - Call setWindowSize() at creation of the application and when the window size change
  - Call setLookat() at creation to initialize the camera look position
  - Call setMousePosition() on application mouse down
  - Call mouseMove() on application mouse move

  Retrieve the camera matrix by calling getMatrix()

  See: appbase_vkpp.hpp

  Note: There is a singleton `CameraManip` which can be use across the entire application

  ~~~ C++
  // Retrieve/set camera information
  CameraManip.getLookat(eye, center, up);
  CameraManip.setLookat(eye, center, nvmath::vec3f(m_upVector == 0, m_upVector == 1, m_upVector == 2));
  CameraManip.getFov();
  CameraManip.setSpeed(navSpeed);
  CameraManip.setMode(navMode == 0 ? nvh::CameraManipulator::Examine : nvh::CameraManipulator::Fly);
  // On mouse down, keep mouse coordinates
  CameraManip.setMousePosition(x, y);
  // On mouse move and mouse button down
  if(m_inputs.lmb || m_inputs.rmb || m_inputs.mmb)
  {
  CameraManip.mouseMove(x, y, m_inputs);
  }
  // Wheel changes the FOV
  CameraManip.wheel(delta > 0 ? 1 : -1, m_inputs);
  // Retrieve the matrix to push to the shader
  m_ubo.view = CameraManip.getMatrix();	
  ~~~

*/

class CameraManipulator
{
public:
  // clang-format off
    enum Modes { Examine, Fly, Walk, Trackball };
    enum Actions { None, Orbit, Dolly, Pan, LookAround };
    struct Inputs {bool lmb=false; bool mmb=false; bool rmb=false; 
                   bool shift=false; bool ctrl=false; bool alt=false;};
  // clang-format on

  /// Main function to call from the application
  /// On application mouse move, call this function with the current mouse position, mouse
  /// button presses and keyboard modifier. The camera matrix will be updated and
  /// can be retrieved calling getMatrix
  Actions mouseMove(int x, int y, const Inputs& inputs);

  /// Set the camera to look at the interest point
  void setLookat(const nvmath::vec3& eye, const nvmath::vec3& center, const nvmath::vec3& up, bool instantSet = true);

  void updateAnim();
  /// Changing the size of the window. To call when the size of the window change.
  /// This allows to do nicer movement according to the window size.
  void setWindowSize(int w, int h);

  /// Setting the current mouse position, to call on mouse button down
  void setMousePosition(int x, int y);

  // Factory.
  static CameraManipulator& Singleton()
  {
    static CameraManipulator manipulator;
    return manipulator;
  }

  /// Retrieve the position, interest and up vector of the camera
  void getLookat(nvmath::vec3& eye, nvmath::vec3& center, nvmath::vec3& up) const;

  /// Set the manipulator mode, from Examiner, to walk, to fly, ...
  void setMode(Modes mode);

  /// Retrieve the current manipulator mode
  Modes getMode() const;

  /// Setting the roll (radian) around the Z axis
  void setRoll(float roll);

  /// Retrieve the camera roll
  float getRoll() const;

  /// Retrieving the transformation matrix of the camera
  const nvmath::mat4& getMatrix() const;

  /// Changing the default speed movement
  void setSpeed(float speed);

  /// Retrieving the current speed
  float getSpeed();

  /// Retrieving the last mouse position
  void getMousePosition(int& x, int& y);

  /// Main function which is called to apply a camera motion.
  /// It is preferable to
  void motion(int x, int y, int action = 0);

  /// To call when the mouse wheel change
  void wheel(int value, const Inputs& inputs);

  /// Retrieve the screen width
  int getWidth() const;

  /// Retrieve the screen height
  int getHeight() const;

  /// Field of view
  void  setFov(float _fov) { m_fov = _fov; }
  float getFov() { return m_fov; }

  CameraManipulator::Inputs getInputs(const int& mouseButtonFlags, const bool* keyPressed);


  double getDuration() const { return m_duration; }
  void   setDuration(double val) { m_duration = val; }

protected:
  CameraManipulator();

private:
  // Update the internal matrix.
  void update();
  // Do panning: movement parallels to the screen
  void pan(float dx, float dy);
  // Do orbiting: rotation around the center of interest. If invert, the interest orbit around the
  // camera position
  void orbit(float dx, float dy, bool invert = false);
  // Do dolly: movement toward the interest.
  void dolly(float dx, float dy);
  // Trackball: movement like the object is inside a ball
  void trackball(int x, int y);
  // Used by the trackball
  double projectOntoTBSphere(const nvmath::vec2& p);
  double getSystemTime();

  nvmath::vec3f computeBezier(float t, nvmath::vec3f& p0, nvmath::vec3f& p1, nvmath::vec3f& p2);
  void          findBezierPoints();

protected:
  struct Camera
  {
    nvmath::vec3f eye = nvmath::vec3f(10, 10, 10);
    nvmath::vec3f ctr = nvmath::vec3f(0, 0, 0);
    nvmath::vec3f up  = nvmath::vec3f(0, 1, 0);
  };


  float         m_roll   = 0;  // Rotation around the Z axis in RAD
  nvmath::mat4f m_matrix = nvmath::mat4f(1);
  float         m_fov    = 60.0f;

  Camera m_current;   // Current camera position
  Camera m_goal;      // Wish camera position
  Camera m_snapshot;  // Current camera the moment a set lookat is done

  nvmath::vec3f m_bezier[3];
  double        m_start_time = 0;
  double        m_duration   = 0.5;

  // Screen
  int m_width  = 1;
  int m_height = 1;

  // Other
  float         m_speed = 30;
  nvmath::vec2f m_mouse = nvmath::vec2f(0.f, 0.f);

  bool  m_button = false;  // Button pressed
  bool  m_moving = false;  // Mouse is moving
  float m_tbsize = 0.8f;   // Trackball size;

  Modes m_mode = Examine;
};

// Global Manipulator

}  // namespace nvh

#define CameraManip nvh::CameraManipulator::Singleton()
