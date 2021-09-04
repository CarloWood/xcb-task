#pragma once

#include <system_error>
#include <string>
#include <iosfwd>
#include <xcb/xcb.h>

namespace xcb::errors {
namespace org::freedesktop::xcb {

// Standard errors defined for org.freedesktop.xcb.Error.*
// See https://xcb.freedesktop.org/manual/group__XCB__Core__API.html
enum class Error
{
  Success = 0,
  XE_XCB_CONN_ERROR                     = XCB_CONN_ERROR,
  XE_XCB_CONN_CLOSED_EXT_NOTSUPPORTED   = XCB_CONN_CLOSED_EXT_NOTSUPPORTED,
  XE_XCB_CONN_CLOSED_MEM_INSUFFICIENT   = XCB_CONN_CLOSED_MEM_INSUFFICIENT,
  XE_XCB_CONN_CLOSED_REQ_LEN_EXCEED     = XCB_CONN_CLOSED_REQ_LEN_EXCEED,
  XE_XCB_CONN_CLOSED_PARSE_ERR          = XCB_CONN_CLOSED_PARSE_ERR,
  XE_XCB_CONN_CLOSED_INVALID_SCREEN     = XCB_CONN_CLOSED_INVALID_SCREEN
};

// Functions that will be found using Argument-dependent lookup.
std::string to_string(Error error);
std::ostream& operator<<(std::ostream& os, Error error);
inline char const* get_domain(Error) { return "xcb:org.freedesktop.xcb.Error"; }
std::error_code make_error_code(Error ec);

} // namespace org::freedesktop::xcb::Error
} // namespace xcb::errors

// Register Errors as valid error code.
namespace std {
template<> struct is_error_code_enum<xcb::errors::org::freedesktop::xcb::Error> : true_type { };
} // namespace std
