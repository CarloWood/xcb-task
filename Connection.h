#pragma once

#include "WindowBase.h"
#include "evio/RawInputDevice.h"
#include "evio/RawOutputDevice.h"
#include "org.freedesktop.Xcb.Error/Errors.h"
#include "threadsafe/aithreadsafe.h"
#include "threadsafe/AIReadWriteSpinLock.h"
#include <xcb/xcb.h>
#include <string>
#include <string_view>
#include <vector>
#include "debug.h"

namespace task {

} // namespace task

namespace xcb {

class Connection : public evio::RawInputDevice
{
 private:
  xcb_connection_t* m_connection = nullptr;
  xcb_screen_t* m_screen = nullptr;
  xcb_atom_t m_wm_protocols_atom;
  xcb_atom_t m_wm_delete_window_atom;

  using handle_to_window_map_container_t = std::map<xcb_window_t, WindowBase*>;
  using handle_to_window_map_t = aithreadsafe::Wrapper<handle_to_window_map_container_t, aithreadsafe::policy::ReadWrite<AIReadWriteSpinLock>>;

  handle_to_window_map_t m_handle_to_window_map;

  std::function<void()> m_cb_closed;

 public:
  void set_closed_callback(std::function<void()> cb_closed) { m_cb_closed = std::move(cb_closed); }
  void connect(std::string display_name);
  void close();

  //---------------------------------------------------------------------------
  // After calling `connect` and before calling `close`, you may call:

  // Generate a new window ID.
  xcb_window_t generate_id() const
  {
    return xcb_generate_id(m_connection);
  }

  // Map handle to window.
  void add(xcb_window_t handle, WindowBase* window);

  // Remove handle from the map. Return true if this was the last window.
  bool remove(xcb_window_t handle);

  // Look up the WindowBase* that was added with `add`.
  WindowBase* lookup(xcb_window_t handle) const;

  // Use the ID returned by generate_id to create a window that is a child window of the root.
  xcb_void_cookie_t create_main_window(xcb_window_t handle,
      int16_t x, int16_t y, uint16_t width, uint16_t height,
      std::string_view const& title,
      uint16_t border_width, uint16_t _class, uint32_t value_mask, std::vector<uint32_t> const& value_list) const;

  // Destroy a window using its ID (as returned by generate_id.
  void destroy_window(xcb_window_t handle)
  {
    DoutEntering(dc::notice, "xcb::Connection::destroy_window(" << handle << ")");
    if (m_connection)
    {
      Dout(dc::notice, "Calling xcb_destroy_window(" << m_connection << ", " << handle << ")");
      xcb_destroy_window(m_connection, handle);
      xcb_flush(m_connection);
    }
    destroyed(handle);
  }

  auto white_pixel() const
  {
    return m_screen->white_pixel;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os, xcb_generic_event_t const& event) const;
  std::string print_atom(xcb_atom_t atom) const;
#endif

 private:
  void destroyed(xcb_window_t handle);

  void read_from_fd(int& allow_deletion_count, int fd) override final;
  void hup(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd)) override final { DoutEntering(dc::notice, "xcb::Connection::hup"); }
  void err(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd)) override final { DoutEntering(dc::notice, "xcb::Connection::err"); close(); }
  void closed(int& allow_deletion_count) override final { m_cb_closed(); }
};

} // namespace xcb
