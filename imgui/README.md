## Table of Contents
- [imgui_axis.hpp](#imgui_axishpp)
- [imgui_camera_widget.h](#imgui_camera_widgeth)
- [imgui_orient.h](#imgui_orienth)

## imgui_axis.hpp

Function `Axis(ImVec2 pos, const glm::mat4& modelView, float size = 20.f)`
which display right-handed axis in a ImGui window.

Example

```cpp
{  // Display orientation axis at the bottom left corner of the window
float  axisSize = 25.F;
ImVec2 pos      = ImGui::GetWindowPos();
pos.y += ImGui::GetWindowSize().y;
pos += ImVec2(axisSize * 1.1F, -axisSize * 1.1F) * ImGui::GetWindowDpiScale();  // Offset
ImGuiH::Axis(pos, CameraManip.getMatrix(), axisSize);
}
```


## imgui_camera_widget.h

### functions in ImGuiH


- CameraWidget : CameraWidget is a Camera widget for the the Camera Manipulator
- SetCameraJsonFile : set the name (without .json) of the setting file. It will load and replace all camera and settings
- SetHomeCamera : set the home camera - replace the one on load
- AddCamera : adding a camera to the list of cameras


## imgui_orient.h

### struct ImOrient

> brief This is a really nice implementation of an orientation widget; all due respect to the original author ;)

This is a port of the AntTweakBar orientation widget, which is a 3D orientation widget that allows the user to specify a
3D orientation using a quaternion, axis-angle, or direction vector.  It is a very useful widget for 3D applications.

