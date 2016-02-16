/*
 * Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

#ifndef NV_CAMCONTROL_INCLUDED
#define NV_CAMCONTROL_INCLUDED

#include <nv_math/nv_math.h>
#include <algorithm>

namespace nv_helpers
{

  class CameraControl 
  {
  public:

    CameraControl()
      : m_lastButtonFlags(0)
      , m_lastWheel(0)
      , m_senseWheelZoom(0.05f/120.0f)
      , m_senseZoom(0.001f)
      , m_senseRotate((nv_pi*0.5f)/256.0f)
      , m_sensePan(1.0f)
      , m_sceneOrbit(0.0f)
      , m_sceneDimension(1.0f)
      , m_sceneOrtho(false)
      , m_sceneOrthoZoom(1.0f)
    {

    }

    inline void  processActions(const nv_math::vec2i &window, const nv_math::vec2f &mouse, int mouseButtonFlags, int wheel)
    {
      int changed  = m_lastButtonFlags ^ mouseButtonFlags;
      m_lastButtonFlags = mouseButtonFlags;

      int panFlag  = m_sceneOrtho ? 1<<0 : 1<<2;
      int zoomFlag = 1<<1;
      int rotFlag  = m_sceneOrtho ? 1<<2 : 1<<0;


      m_panning  = !!(mouseButtonFlags & panFlag);
      m_zooming  = !!(mouseButtonFlags & zoomFlag);
      m_rotating = !!(mouseButtonFlags & rotFlag);
      m_zoomingWheel = wheel != m_lastWheel;

      m_startZoomWheel = m_lastWheel;
      m_lastWheel = wheel;

      if (m_rotating){
        m_panning = false;
        m_zooming = false;
      }

      if (m_panning && (changed & panFlag)){
        // pan
        m_startPan = mouse;
        m_startMatrix = m_viewMatrix;
      }
      if (m_zooming && (changed & zoomFlag)){
        // zoom
        m_startMatrix = m_viewMatrix;
        m_startZoom = mouse;
        m_startZoomOrtho = m_sceneOrthoZoom;
      }
      if (m_rotating && (changed & rotFlag)){
        // rotate
        m_startRotate = mouse;
        m_startMatrix = m_viewMatrix;
      }

      if (m_zooming || m_zoomingWheel){
        float dist = 
          m_zooming ? -(nv_math::dot( mouse - m_startZoom ,nv_math::vec2f(-1,1)) * m_sceneDimension * m_senseZoom) 
          : (float(wheel - m_startZoomWheel) * m_sceneDimension * m_senseWheelZoom);

        if (m_zoomingWheel){
          m_startZoomOrtho = m_sceneOrthoZoom;
          m_startMatrix = m_viewMatrix;
        }

        if (m_sceneOrtho){
          float newzoom = m_startZoomOrtho - (dist);
          if (m_zoomingWheel){
            if (newzoom < 0){
              m_sceneOrthoZoom *= 0.5;
            }
            else if (m_sceneOrthoZoom < abs(dist)){
              m_sceneOrthoZoom *= 2.0;
            }
            else{
              m_sceneOrthoZoom = newzoom;
            }
          }
          else{
            m_sceneOrthoZoom = newzoom;
          }
          m_sceneOrthoZoom = std::max(0.0001f,m_sceneOrthoZoom);
        }
        else{
          nv_math::mat4f delta = nv_math::translation_mat4(nv_math::vec3f(0,0,dist * 2.0f));
          m_viewMatrix = delta * m_startMatrix;
        }

      }

      if (m_panning){
        float aspect = float(window.x)/float(window.y);

        nv_math::vec3f winsize(window.x, window.y, 1.0f);
        nv_math::vec3f ortho(m_sceneOrthoZoom * aspect, m_sceneOrthoZoom, 1.0f);
        nv_math::vec3f sub( mouse - m_startPan, 0.0f);
        sub /= winsize;
        sub *= ortho;
        sub.y *= -1.0;
        if (!m_sceneOrtho){
          sub *= m_sensePan * m_sceneDimension;
        }

        nv_math::mat4f delta;
        delta.as_translation(sub);
        m_viewMatrix = delta * m_startMatrix;
      }

      if (m_rotating){
        float aspect = float(window.x)/float(window.y);

        nv_math::vec2f angles = (mouse - m_startRotate) * m_senseRotate;
        nv_math::vec3f center = nv_math::vec3f(m_startMatrix * nv_math::vec4f(m_sceneOrbit,1.0f));

        nv_math::mat4f rot   = nv_math::rotation_yaw_pitch_roll(angles.x, angles.y, 0.0f);
        nv_math::mat4f delta = nv_math::translation_mat4(center) * rot * nv_math::translation_mat4(-center);

        m_viewMatrix = delta * m_startMatrix;
      }
    }
    
    bool        m_sceneOrtho;
    float       m_sceneOrthoZoom;
    float       m_sceneDimension;

    nv_math::vec3f   m_sceneOrbit;
    nv_math::mat4f   m_viewMatrix;

    float       m_senseWheelZoom;
    float       m_senseZoom;
    float       m_senseRotate;
    float       m_sensePan;

  private:
    bool        m_zooming;
    bool        m_zoomingWheel;
    bool        m_panning;
    bool        m_rotating;

    nv_math::vec2f   m_startPan;
    nv_math::vec2f   m_startZoom;
    nv_math::vec2f   m_startRotate;
    nv_math::mat4f   m_startMatrix;
    int         m_startZoomWheel;
    float       m_startZoomOrtho;

    int         m_lastButtonFlags;
    int         m_lastWheel;

  };
}

#endif
