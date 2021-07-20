#pragma once

/// \nodoc (keyword to exclude this file from automatic README.md generation)

struct ImDrawData;

namespace ImGui
{
  void InitGL();
  void ShutdownGL();

  void RenderDrawDataGL(const ImDrawData* drawData);
}
