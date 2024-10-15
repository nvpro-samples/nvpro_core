#pragma once

/* @DOC_START
ImGui GLFW binding with OpenGL3 + shaders.
More or less copy-pasted from https://github.com/ocornut/imgui/blob/b911f96a56e4f0612f14fb15c7839b1bbfa17afc/examples/opengl3_example/main.cpp
@DOC_END */

struct ImDrawData;

namespace ImGui {
void InitGL();
void ShutdownGL();

void RenderDrawDataGL(const ImDrawData* drawData);
}  // namespace ImGui
