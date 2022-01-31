#pragma once

#include <cstdint>
#include <string>
#include <iostream>

namespace xcb {

struct ModifierMask
{
  static constexpr int Shift   = 1 << 0;        // Both, left and/or right.
  static constexpr int Lock    = 1 << 1;        // When pressed or CapsLock on.
  static constexpr int Ctrl    = 1 << 2;        // Both, left and/or right.
  static constexpr int Alt     = 1 << 3;        // Both, left and/or right.
//  static constexpr int Mod2    = 1 << 4;
//  static constexpr int Mod3    = 1 << 5;
  static constexpr int Super   = 1 << 6;
//  static constexpr int Mod5    = 1 << 7;
  static constexpr int Button1 = 1 << 8;
  static constexpr int Button2 = 1 << 9;
  static constexpr int Button3 = 1 << 10;
//  static constexpr int WheelUp = 1 << 11;
//  static constexpr int WheelDown = 1 << 12;

 private:
  uint16_t m_mask;

 public:
  ModifierMask() : m_mask(0) { }
  ModifierMask(uint16_t mask) : m_mask(mask) { }
  ModifierMask(ModifierMask const& orig) : m_mask(orig.m_mask) { }

  std::string to_string() const;

  friend std::ostream& operator<<(std::ostream& os, ModifierMask mask)
  {
    return os << mask.to_string();
  }
};

class WindowBase
{
 public:
  virtual void on_window_size_changed(uint32_t width, uint32_t height) = 0;        // Called whenever the window changed size.
  virtual void on_map_changed(bool minimized) = 0;

  virtual uint16_t convert(uint32_t modifiers) = 0;

  virtual void on_mouse_move (int16_t x, int16_t y, uint16_t converted_modifiers) = 0;
  virtual void on_key_event  (int16_t x, int16_t y, uint16_t converted_modifiers, bool pressed, uint32_t keysym) = 0;
  virtual void on_mouse_click(int16_t x, int16_t y, uint16_t converted_modifiers, bool pressed, uint8_t button) = 0;
  virtual void on_mouse_enter(int16_t x, int16_t y, uint16_t converted_modifiers, bool entered) = 0;
  virtual void on_focus_changed(bool in_focus) = 0;

  virtual void On_WM_DELETE_WINDOW(uint32_t timestamp) = 0;

  virtual ~WindowBase() = default;
};

} // namespace xcb
