#include "sys.h"
#include "Connection.h"
#ifdef CWDEBUG
#include "utils/popcount.h"
#include <libcwd/buf2str.h>
#endif

namespace xcb {

void Connection::connect(std::string display_name)
{
  DoutEntering(dc::notice, "xcb::Connection::connect(\"" << display_name << "\")");

  using namespace xcb::errors;
  using namespace org::freedesktop::xcb;

  int screen_index;
  m_connection = xcb_connect(display_name.c_str(), &screen_index);
  auto error = static_cast<Error>(xcb_connection_has_error(m_connection));
  if (error != Error::Success)
  {
    m_connection = nullptr;
    THROW_ALERTC(error, "xcb_connect");
  }
  m_screen = xcb_setup_roots_iterator(xcb_get_setup(m_connection)).data;

  // Prepare notification for window destruction.
  xcb_intern_atom_cookie_t  protocols_cookie = xcb_intern_atom(m_connection, 1, 12, "WM_PROTOCOLS");
  xcb_intern_atom_reply_t*  protocols_reply  = xcb_intern_atom_reply(m_connection, protocols_cookie, 0);
  m_wm_protocols_atom = protocols_reply->atom;
  free(protocols_reply);

  xcb_intern_atom_cookie_t  delete_cookie    = xcb_intern_atom(m_connection, 0, 16, "WM_DELETE_WINDOW");
  xcb_intern_atom_reply_t*  delete_reply     = xcb_intern_atom_reply(m_connection, delete_cookie, 0);
  m_wm_delete_window_atom = delete_reply->atom;
  free(delete_reply);

  int fd = xcb_get_file_descriptor(m_connection);
  fd_init(fd);

  // Don't close the fd when calling close() because it will be closed by the call to xcb_disconnect.
  state_t::wat(m_state)->m_flags.set_dont_close();
  start_input_device();
}

void Connection::close()
{
  DoutEntering(dc::notice, "xcb::Connection::close()");

  FileDescriptor::close();
  if (m_connection)
  {
    xcb_disconnect(m_connection);
    m_connection = nullptr;
  }
}

void Connection::add(xcb_window_t handle, WindowBase* window)
{
  handle_to_window_map_t::wat handle_to_window_map_w(m_handle_to_window_map);
  handle_to_window_map_w->emplace(handle_to_window_map_container_t::value_type{handle, window});
}

void Connection::destroyed(xcb_window_t handle)
{
  handle_to_window_map_t::wat handle_to_window_map_w(m_handle_to_window_map);
  auto iter = handle_to_window_map_w->find(handle);
  // Can this ever happen?
  ASSERT(iter != handle_to_window_map_w->end());
  iter->second = nullptr;
}

bool Connection::remove(xcb_window_t handle)
{
  handle_to_window_map_t::wat handle_to_window_map_w(m_handle_to_window_map);
  handle_to_window_map_w->erase(handle);
  return handle_to_window_map_w->empty();
}

WindowBase* Connection::lookup(xcb_window_t handle) const
{
  handle_to_window_map_t::crat handle_to_window_map_r(m_handle_to_window_map);
  auto search = handle_to_window_map_r->find(handle);
  if (AI_UNLIKELY(search == handle_to_window_map_r->end()))
    THROW_ALERT("No such xcb window handle: [HANDLE]", AIArgs("[HANDLE]", handle));
  return search->second;
}

xcb_void_cookie_t Connection::create_main_window(xcb_window_t handle,
    int16_t x, int16_t y, uint16_t width, uint16_t height,
    std::string_view const& title,
    uint16_t border_width, uint16_t _class, uint32_t value_mask, std::vector<uint32_t> const& value_list) const
{
  DoutEntering(dc::notice, "xcb::Connection::create_main_window(" << handle << ", " <<
      x << ", " << y << ", " << width << ", " << height << ", \"" << title << "\", " <<
      border_width << ", " << _class << ", 0x" << std::hex << value_mask << std::dec << ", " << value_list << ")");

  // value_list should have an entry for each bit in value_mask.
  ASSERT(utils::popcount(value_mask) == value_list.size());

  xcb_void_cookie_t ret = xcb_create_window(m_connection, XCB_COPY_FROM_PARENT, handle, m_screen->root,
      x, y, width, height,
      border_width, _class, m_screen->root_visual, value_mask, value_list.data());

  xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, handle,
    XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
    title.size(), title.data());

  xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, handle, m_wm_protocols_atom, 4, 32, 1, &m_wm_delete_window_atom);

  // Display window.
  xcb_map_window(m_connection, handle);
  xcb_flush(m_connection);

  return ret;
}

namespace {

#ifdef CWDEBUG
std::string print_modifiers(uint32_t mask)
{
  static char const* mods[] = {
    "Shift", "Lock", "Ctrl", "Alt",
    "Mod2", "Mod3", "Mod4", "Mod5",
    "Button1", "Button2", "Button3", "Button4", "Button5",
    "XXX6", "XXX7", "XXX8"
  };
  char const** mod;
  std::string result = "Modifier mask:";
  for (mod = mods ; mask; mask >>= 1, mod++)
  {
    if ((mask & 1))
    {
      result += ' ';
      result += *mod;
    }
  }
  return result;
}

char const* response_type_to_string(uint8_t response_type)
{
  switch (response_type & 0x7f)
  {
    AI_CASE_RETURN(XCB_KEY_PRESS);
    AI_CASE_RETURN(XCB_KEY_RELEASE);
    AI_CASE_RETURN(XCB_BUTTON_PRESS);
    AI_CASE_RETURN(XCB_BUTTON_RELEASE);
    AI_CASE_RETURN(XCB_MOTION_NOTIFY);
    AI_CASE_RETURN(XCB_ENTER_NOTIFY);
    AI_CASE_RETURN(XCB_LEAVE_NOTIFY);
    AI_CASE_RETURN(XCB_FOCUS_IN);
    AI_CASE_RETURN(XCB_FOCUS_OUT);
    AI_CASE_RETURN(XCB_KEYMAP_NOTIFY);
    AI_CASE_RETURN(XCB_EXPOSE);
    AI_CASE_RETURN(XCB_GRAPHICS_EXPOSURE);
    AI_CASE_RETURN(XCB_NO_EXPOSURE);
    AI_CASE_RETURN(XCB_VISIBILITY_NOTIFY);
    AI_CASE_RETURN(XCB_CREATE_NOTIFY);
    AI_CASE_RETURN(XCB_DESTROY_NOTIFY);
    AI_CASE_RETURN(XCB_UNMAP_NOTIFY);
    AI_CASE_RETURN(XCB_MAP_NOTIFY);
    AI_CASE_RETURN(XCB_MAP_REQUEST);
    AI_CASE_RETURN(XCB_REPARENT_NOTIFY);
    AI_CASE_RETURN(XCB_CONFIGURE_NOTIFY);
    AI_CASE_RETURN(XCB_CONFIGURE_REQUEST);
    AI_CASE_RETURN(XCB_GRAVITY_NOTIFY);
    AI_CASE_RETURN(XCB_RESIZE_REQUEST);
    AI_CASE_RETURN(XCB_CIRCULATE_NOTIFY);
    AI_CASE_RETURN(XCB_CIRCULATE_REQUEST);
    AI_CASE_RETURN(XCB_PROPERTY_NOTIFY);
    AI_CASE_RETURN(XCB_SELECTION_CLEAR);
    AI_CASE_RETURN(XCB_SELECTION_REQUEST);
    AI_CASE_RETURN(XCB_SELECTION_NOTIFY);
    AI_CASE_RETURN(XCB_COLORMAP_NOTIFY);
    AI_CASE_RETURN(XCB_CLIENT_MESSAGE);
    AI_CASE_RETURN(XCB_MAPPING_NOTIFY);
    AI_CASE_RETURN(XCB_GE_GENERIC);
  }
  return "<UNKNOWN RESPONSE TYPE>";
}
#endif

} // namespace

#ifdef CWDEBUG
std::string Connection::print_atom(xcb_atom_t atom) const
{
  xcb_get_atom_name_cookie_t atom_name_cookie = xcb_get_atom_name(m_connection, atom);
  xcb_get_atom_name_reply_t* reply = xcb_get_atom_name_reply(m_connection, atom_name_cookie, nullptr);
  if (!reply)
    return {};
  std::string atom_name_str(xcb_get_atom_name_name(reply), xcb_get_atom_name_name_length(reply));
  free(reply);
  return atom_name_str;
}

void Connection::print_on(std::ostream& os, xcb_generic_event_t const& event) const
{
  os << '{';
  os << "response_type:" << response_type_to_string(event.response_type) <<
    /*", pad0:" << (int)event.pad0 <<*/ ", sequence:" << event.sequence <<
    /*", pad[]:" << libcwd::buf2str(reinterpret_cast<char const*>(event.pad), sizeof(event.pad)) <<*/
    ", full_sequence:" << event.full_sequence;
  switch (event.response_type & 0x7f)
  {
    case XCB_KEY_PRESS:
    {
      xcb_key_press_event_t const& ev = reinterpret_cast<xcb_key_press_event_t const&>(event);

      break;
    }
    case XCB_KEY_RELEASE:
    {
      xcb_key_release_event_t const& ev = reinterpret_cast<xcb_key_release_event_t const&>(event);
      break;
    }
    case XCB_BUTTON_PRESS:
    {
      xcb_button_press_event_t const& ev = reinterpret_cast<xcb_button_press_event_t const&>(event);
      break;
    }
    case XCB_BUTTON_RELEASE:
    {
      xcb_button_release_event_t const& ev = reinterpret_cast<xcb_button_release_event_t const&>(event);
      break;
    }
    case XCB_MOTION_NOTIFY:
    {
      xcb_motion_notify_event_t const& ev = reinterpret_cast<xcb_motion_notify_event_t const&>(event);
      break;
    }
    case XCB_ENTER_NOTIFY:
    {
      xcb_enter_notify_event_t const& ev = reinterpret_cast<xcb_enter_notify_event_t const&>(event);
      break;
    }
    case XCB_LEAVE_NOTIFY:
    {
      xcb_leave_notify_event_t const& ev = reinterpret_cast<xcb_leave_notify_event_t const&>(event);
      break;
    }
    case XCB_FOCUS_IN:
    {
      xcb_focus_in_event_t const& ev = reinterpret_cast<xcb_focus_in_event_t const&>(event);
      break;
    }
    case XCB_FOCUS_OUT:
    {
      xcb_focus_out_event_t const& ev = reinterpret_cast<xcb_focus_out_event_t const&>(event);
      break;
    }
    case XCB_KEYMAP_NOTIFY:
    {
      xcb_keymap_notify_event_t const& ev = reinterpret_cast<xcb_keymap_notify_event_t const&>(event);
      break;
    }
    case XCB_EXPOSE:
    {
      // Double click on title bar --> maximize.
      xcb_expose_event_t const& ev = reinterpret_cast<xcb_expose_event_t const&>(event);
      os << ", window:" << ev.window << ", x:" << ev.x << ", y:" << ev.y << ", width:" << ev.width << ", height:" << ev.height << ", count:" << ev.count;
      break;
    }
    case XCB_GRAPHICS_EXPOSURE:
    {
      xcb_graphics_exposure_event_t const& ev = reinterpret_cast<xcb_graphics_exposure_event_t const&>(event);
      break;
    }
    case XCB_NO_EXPOSURE:
    {
      xcb_no_exposure_event_t const& ev = reinterpret_cast<xcb_no_exposure_event_t const&>(event);
      break;
    }
    case XCB_VISIBILITY_NOTIFY:
    {
      xcb_visibility_notify_event_t const& ev = reinterpret_cast<xcb_visibility_notify_event_t const&>(event);
      break;
    }
    case XCB_CREATE_NOTIFY:
    {
      xcb_create_notify_event_t const& ev = reinterpret_cast<xcb_create_notify_event_t const&>(event);
      break;
    }
    case XCB_DESTROY_NOTIFY:
    {
      xcb_destroy_notify_event_t const& ev = reinterpret_cast<xcb_destroy_notify_event_t const&>(event);
      os << ", event:" << ev.event << ", window:" << ev.window;
      break;
    }
    case XCB_UNMAP_NOTIFY:
    {
      xcb_unmap_notify_event_t const& ev = reinterpret_cast<xcb_unmap_notify_event_t const&>(event);
      break;
    }
    case XCB_MAP_NOTIFY:
    {
      xcb_map_notify_event_t const& ev = reinterpret_cast<xcb_map_notify_event_t const&>(event);
      break;
    }
    case XCB_MAP_REQUEST:
    {
      xcb_map_request_event_t const& ev = reinterpret_cast<xcb_map_request_event_t const&>(event);
      break;
    }
    case XCB_REPARENT_NOTIFY:
    {
      xcb_reparent_notify_event_t const& ev = reinterpret_cast<xcb_reparent_notify_event_t const&>(event);
      break;
    }
    case XCB_CONFIGURE_NOTIFY:
    {
      // Left-click title bar, draw and release button: moved window.
      xcb_configure_notify_event_t const& ev = reinterpret_cast<xcb_configure_notify_event_t const&>(event);
      os << ", event:" << ev.event << ", window:" << ev.window << ", above_sibling:" << ev.above_sibling <<
        ", x:" << ev.x << ", y:" << ev.y << ", width:" << ev.width << ", height:" << ev.height <<
        ", border_width:" << ev.border_width << ", override_redirect:" << (int)ev.override_redirect;
      break;
    }
    case XCB_CONFIGURE_REQUEST:
    {
      xcb_configure_request_event_t const& ev = reinterpret_cast<xcb_configure_request_event_t const&>(event);
      break;
    }
    case XCB_GRAVITY_NOTIFY:
    {
      xcb_gravity_notify_event_t const& ev = reinterpret_cast<xcb_gravity_notify_event_t const&>(event);
      break;
    }
    case XCB_RESIZE_REQUEST:
    {
      xcb_resize_request_event_t const& ev = reinterpret_cast<xcb_resize_request_event_t const&>(event);
      break;
    }
    case XCB_CIRCULATE_NOTIFY:
    {
      xcb_circulate_notify_event_t const& ev = reinterpret_cast<xcb_circulate_notify_event_t const&>(event);
      break;
    }
    case XCB_CIRCULATE_REQUEST:
    {
      xcb_circulate_request_event_t const& ev = reinterpret_cast<xcb_circulate_request_event_t const&>(event);
      break;
    }
    case XCB_PROPERTY_NOTIFY:
    {
      xcb_property_notify_event_t const& ev = reinterpret_cast<xcb_property_notify_event_t const&>(event);
      break;
    }
    case XCB_SELECTION_CLEAR:
    {
      xcb_selection_clear_event_t const& ev = reinterpret_cast<xcb_selection_clear_event_t const&>(event);
      break;
    }
    case XCB_SELECTION_REQUEST:
    {
      xcb_selection_request_event_t const& ev = reinterpret_cast<xcb_selection_request_event_t const&>(event);
      break;
    }
    case XCB_SELECTION_NOTIFY:
    {
      xcb_selection_notify_event_t const& ev = reinterpret_cast<xcb_selection_notify_event_t const&>(event);
      break;
    }
    case XCB_COLORMAP_NOTIFY:
    {
      xcb_colormap_notify_event_t const& ev = reinterpret_cast<xcb_colormap_notify_event_t const&>(event);
      break;
    }
    case XCB_CLIENT_MESSAGE:
    {
      xcb_client_message_event_t const& ev = reinterpret_cast<xcb_client_message_event_t const&>(event);
      os << ", format:" << (int)ev.format << ", window:" << ev.window << ", type:" << print_atom(ev.type);
      if (ev.type == m_wm_protocols_atom)
        os << ", data:{atom:" << print_atom(ev.data.data32[0]) << ", timestamp:" << ev.data.data32[1] << "}";
      else
        os << ", data:" << libcwd::buf2str(reinterpret_cast<char const*>(ev.data.data8), sizeof(ev.data));
      break;
    }
    case XCB_MAPPING_NOTIFY:
    {
      xcb_mapping_notify_event_t const& ev = reinterpret_cast<xcb_mapping_notify_event_t const&>(event);
      break;
    }
    case XCB_GE_GENERIC:
    {
      xcb_ge_generic_event_t const& ev = reinterpret_cast<xcb_ge_generic_event_t const&>(event);
      break;
    }
  }
  os << '}';
}
#endif

void Connection::read_from_fd(int& allow_deletion_count, int fd)
{
  DoutEntering(dc::notice, "xcb::Connection::read_from_fd()");
  bool destroyed = false;
  xcb_generic_event_t const* event;
  while ((event = xcb_poll_for_event(m_connection)))
  {
    Dout(dc::notice, "Processing event " << print_using(*event, [this](std::ostream& os, xcb_generic_event_t const& event_) { print_on(os, event_); }));
    // Errors of requests which have no reply cause an 'event' with response_type 0 by default,
    // for requests with a reply you have to use the _unchecked version to get the errors delivered here.
    if (AI_UNLIKELY(event->response_type == 0))
    {
      xcb_generic_error_t const* error = reinterpret_cast<xcb_generic_error_t const*>(event);
      Dout(dc::warning, "Received X11 error " << error->error_code);
      free(const_cast<xcb_generic_event_t*>(event));
      continue;
    }
    // Process events
    switch (event->response_type & 0x7f)
    {
      // Mouse button press
      case XCB_BUTTON_PRESS:
      {
        xcb_button_press_event_t const* ev = reinterpret_cast<xcb_button_press_event_t const*>(event);

        uint32_t mask = ev->state;
        Dout(dc::notice, print_modifiers(mask));

        switch (ev->detail)
        {
          case 4:
            Dout(dc::notice, "Wheel Button up in window " << ev->event << ", at coordinates (" << ev->event_x << ", " << ev->event_y << ")");
            break;
          case 5:
            Dout(dc::notice, "Wheel Button down in window " << ev->event << ", at coordinates (" << ev->event_x << ", " << ev->event_y << ")");
            break;
          default:
            Dout(dc::notice, "Button " << ev->detail << "pressed in window " << ev->event << ", at coordinates (" << ev->event_x << ", " << ev->event_y << ")");
#if 0
            project.MouseMove(ev->event_x, ev->event_y);
            if (ev->detail == 1)
              project.MouseClick(0, true);
            else if (ev->detail == 3)
              project.MouseClick(1, true);
#endif
        }
        break;
      }
        // Mouse button release
      case XCB_BUTTON_RELEASE:
      {
        xcb_button_release_event_t const* ev = reinterpret_cast<xcb_button_release_event_t const*>(event);
        Dout(dc::notice, print_modifiers(ev->state));
        Dout(dc::notice, "Button " << ev->detail << "released in window " << ev->event << ", at coordinates (" << ev->event_x << ", " << ev->event_y << ")");

#if 0
        project.MouseMove(ev->event_x, ev->event_y);
        if (ev->detail == 1)
          project.MouseClick(0, false);
        else if (ev->detail == 3)
          project.MouseClick(1, false);
#endif
        break;
      }
        // Resize
      case XCB_CONFIGURE_NOTIFY:
      {
        xcb_configure_notify_event_t const* configure_event = reinterpret_cast<xcb_configure_notify_event_t const*>(event);
        static uint16_t s_width = 0;
        static uint16_t s_height = 0;

        if (((configure_event->width > 0) && (s_width != configure_event->width)) ||
          ((configure_event->height > 0) && (s_height != configure_event->height)))
        {
          WindowBase* window = lookup(configure_event->window);
          uint32_t last_width;
          uint32_t last_height;
          window->get_extent(last_width, last_height);
          if (configure_event->width != last_width ||
              configure_event->height != last_height)
            window->OnWindowSizeChanged(configure_event->width, configure_event->height);

          s_width = configure_event->width;
          s_height = configure_event->height;
        }
        break;
      }
        // Close
      case XCB_CLIENT_MESSAGE:
      {
        xcb_client_message_event_t const* client_message_event = reinterpret_cast<xcb_client_message_event_t const*>(event);
        if (client_message_event->format == 32 &&
            client_message_event->type == m_wm_protocols_atom &&
            client_message_event->data.data32[0] == m_wm_delete_window_atom)
        {
          uint32_t timestamp = client_message_event->data.data32[1];
          WindowBase* window = lookup(client_message_event->window);
          if (AI_LIKELY(window))
            window->On_WM_DELETE_WINDOW(timestamp);
        }
        break;
      }
      case XCB_KEY_PRESS:
//        loop = false;
        break;
      case XCB_DESTROY_NOTIFY:
      {
        xcb_destroy_notify_event_t const* destroy_notify_event = reinterpret_cast<xcb_destroy_notify_event_t const*>(event);
#ifdef CWDEBUG
        WindowBase* window = lookup(destroy_notify_event->window);
        // destroyed should have been called before we can receive this message!?
        ASSERT(window == nullptr);
#endif
        if (remove(destroy_notify_event->window))
          destroyed = true;

        break;
      }
    }
    free(const_cast<xcb_generic_event_t*>(event));

    if (AI_UNLIKELY(destroyed))
      break;
  }
}

} // namespace xcb
