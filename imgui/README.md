# Helpers imgui

Table of Contents

- [imgui_camera_widget.h](#imgui_camera_widgeth)
- [imgui_helper.h](#imgui_helperh)
- [imgui_orient.h](#imgui_orienth)
_____

# imgui_camera_widget.h

<a name="imgui_camera_widgeth"></a>
## functions in ImGuiH
  
- CameraWidget : CameraWidget is a Camera widget for the the Camera Manipulator
- SetCameraJsonFile : set the name (without .json) of the setting file. It will load and replace all camera and settings
- SetHomeCamera : set the home camera - replace the one on load
- AddCamera : adding a camera to the list of cameras



_____

# imgui_orient.h

<a name="imgui_orienth"></a>
## struct ImOrient
> This is a really nice implementation of an orientation widget; all due respect to the original author ;)

Notes from: www.github.com/cmaughan

Ported from AntTweakBar

Dependencies kept to a minimum.  I basically vectorized the original code, added a few math types, cleaned things up and
made it clearer what the maths was doing.

I tried to make it more imgui-like, and removed all the excess stuff not needed here.  This still needs work.

I also added triangle culling because ImGui doesn't support winding clip

The widget works by transforming the 3D object to screen space and clipping the triangles.  This makes it work with any
imgui back end, without modifications to the renderers.

\todo More cleanup.
\todo Figure out what ShowDir is for.
\todo Test direction vectors more



