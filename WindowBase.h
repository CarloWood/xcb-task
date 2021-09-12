#pragma once

#include <cstdint>

namespace xcb {

class WindowBase
{
 public:
  virtual void OnWindowSizeChanged() = 0;

  virtual void MouseMove(int x, int y) = 0;
  virtual void MouseClick(size_t button, bool pressed) = 0;
  virtual void ResetMouse() = 0;

  virtual void On_WM_DELETE_WINDOW(uint32_t timestamp) = 0;

  virtual ~WindowBase() = default;
};

} // namespace xcb
