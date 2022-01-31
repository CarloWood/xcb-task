#pragma once

#include "utils/AIAlert.h"
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>
#include "debug.h"
#include <iomanip>
// Really?
#define explicit _explicit
#include <xcb/xkb.h>
#undef explicit

namespace xcb {

class Xkb
{
 private:
  struct xkb_context* m_ctx = {};
  struct xkb_keymap* m_keymap = {};
  struct xkb_state* m_state = {};
  uint8_t m_device_id;
  uint8_t m_xkb_opcode;
  uint8_t m_xkb_base_error;

 public:
  void init(xcb_connection_t* conn)
  {
    // Discover if this X server supports the XKB extention.
    auto reply = xcb_get_extension_data(conn, &xcb_xkb_id);                     // xcb_xkb_id is an external symbol declared in <xcb/xkb.h>.
    if (!reply)
      THROW_ALERT("Failed in querying XKB extension which is required");

    // Create a XKB context.
    {
      m_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
      if (!m_ctx)
        THROW_ALERT("Failed to get XKB context");
    }

    // Negotiate the XKB extension with the X server.
    {
      unsigned short major_xkb_version, minor_xkb_version;

      // The documenation of xkb_x11_setup_xkb_extension says that this returns base_event (first event)
      // and base_error; but it seems that this is really the major-opcode of the XKB extension
      // (see https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html#requests:QueryExtension).
      // Whereas xkbType of the received event messages is then the minor-opcode.
      int ret = xkb_x11_setup_xkb_extension(conn,
          XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
          XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
          &major_xkb_version, &minor_xkb_version,
          &m_xkb_opcode, &m_xkb_base_error);

      if (!ret)
        THROW_ALERT("X server doesn't have the XKB extension, version [MAJOR].[MINOR] or newer",
            AIArgs("[MAJOR]", XKB_X11_MIN_MAJOR_XKB_VERSION)("[MINOR]", XKB_X11_MIN_MINOR_XKB_VERSION));

      Dout(dc::notice, "Using XKB version " << major_xkb_version << '.' << minor_xkb_version <<
          "; m_xkb_opcode = " << (int)m_xkb_opcode << ", m_xkb_base_error = " << (int)m_xkb_base_error << '.');
    }

    // Get the core-keyboard device ID.
    {
      int32_t device_id = xkb_x11_get_core_keyboard_device_id(conn);
      if (device_id == -1 || device_id > 255)
        THROW_ALERT("Failed to get XKB core keyboard device id");
      m_device_id = static_cast<uint8_t>(device_id);    // Device ID's are sent with the XKB protocol as a single byte.
    }

    // Create initial xkb_keymap and xkb_state for the core device.
    create_keymap_and_state(conn);

    // According to https://xkbcommon.org/doc/current/group__x11.html you have to listen to NewKeyboardNotify and MapNotify
    // events and recreate m_keymap and m_state when received.
    //
    // I am not going to support NewKeyboardNotify at the moment, and MapNotify doesn't seem to contain any useful information
    // in itself. XCB already generates the XCB_MAPPING_NOTIFY event, so might as well use that instead of MapNotify.
    // However, With XKB enabled, X won't send XCB_MAPPING_NOTIFY anymore! So we have to listen to XCB_XKB_EVENT_TYPE_MAP_NOTIFY
    // anyway.

    static constexpr uint16_t required_map_parts =
//        XCB_XKB_MAP_PART_KEY_TYPES |
        XCB_XKB_MAP_PART_KEY_SYMS |
        XCB_XKB_MAP_PART_MODIFIER_MAP;
//        XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
//        XCB_XKB_MAP_PART_KEY_ACTIONS |
//        XCB_XKB_MAP_PART_VIRTUAL_MODS |
//        XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;

    static constexpr uint16_t required_events =
/*        XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY | */
        XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
        XCB_XKB_EVENT_TYPE_STATE_NOTIFY;

    xcb_void_cookie_t cookie = xcb_xkb_select_events(conn, m_device_id, required_events, 0, required_events, required_map_parts, required_map_parts, nullptr);

    xcb_generic_error_t* error = xcb_request_check(conn, cookie);
    if (error)
      THROW_ALERT("Failed to request XKB events");
  }

  void create_keymap_and_state(xcb_connection_t* conn)
  {
    DoutEntering(dc::notice, "xcb::Xkb::create_keymap_and_state()");

    m_keymap = xkb_x11_keymap_new_from_device(m_ctx, conn, m_device_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!m_keymap)
      THROW_ALERT("Failed to get keymap from X11 server");

    m_state = xkb_x11_state_new_from_device(m_keymap, conn, m_device_id);
    if (!m_state)
      THROW_ALERT("Failed to create a keyboard state object");
  }

  void update_state(xcb_xkb_state_notify_event_t const* ev)
  {
    xkb_state_update_mask(m_state, ev->baseMods, ev->latchedMods, ev->lockedMods, ev->baseGroup, ev->latchedGroup, ev->lockedGroup);
  }

  xkb_keysym_t get_one_sym(xcb_keycode_t code)
  {
    return xkb_state_key_get_one_sym(m_state, code);
  }

  xkb_mod_mask_t get_active_mods()
  {
    return xkb_state_serialize_mods(m_state, XKB_STATE_MODS_EFFECTIVE);
  }

  xkb_mod_mask_t get_consumed_mods(xcb_keycode_t code)
  {
#if 0
    // Just left here to show how to print the names of the modifiers.
    ASSERT(m_keymap == xkb_state_get_keymap(m_state));
    xkb_layout_index_t layout = xkb_state_key_get_layout(m_state, code);
    Dout(dc::notice, "layout [ " << xkb_keymap_layout_get_name(m_keymap, layout) << " (" << layout << ") ]");
    Dout(dc::notice, "level [ " << xkb_state_key_get_level(m_state, code, layout) << " ]");
    auto number_of_mods = xkb_keymap_num_mods(m_keymap);
    Dout(dc::notice|continued_cf, "mods [ ");
    for (xkb_mod_index_t mod = 0; mod < number_of_mods; ++mod)
    {
      if (xkb_state_mod_index_is_active(m_state, mod, XKB_STATE_MODS_EFFECTIVE) <= 0)
        continue;
      if (xkb_state_mod_index_is_consumed(m_state, code, mod))
        Dout(dc::continued, '-' << xkb_keymap_mod_get_name(m_keymap, mod) << ' ');
      else
        Dout(dc::continued, xkb_keymap_mod_get_name(m_keymap, mod) << ' ');
    }
    Dout(dc::finish, "]");
#endif
    return xkb_state_key_get_consumed_mods2(m_state, code, XKB_CONSUMED_MODE_XKB);
  }

  uint8_t device_id() const { return m_device_id; }
  uint8_t opcode() const { return m_xkb_opcode; }
  uint8_t base_error() const { return m_xkb_base_error; }

  ~Xkb()
  {
    if (m_state)
      xkb_state_unref(m_state);
    if (m_keymap)
      xkb_keymap_unref(m_keymap);
    if (m_ctx)
      xkb_context_unref(m_ctx);
  }
};

} // namespace xcb


