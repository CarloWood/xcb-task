 #pragma once

#include <string>
#include <iosfwd>

namespace xcb::errors {
namespace org::freedesktop::xcb::Error {

// Standard errors defined for org.freedesktop.xcb.Error.*
// See https://xcb.freedesktop.org/manual/group__XCB__Core__API.html
enum Errors
{
  EC_XCB_CONN_ERROR = 1,
  EC_XCB_CONN_CLOSED_EXT_NOTSUPPORTED,
  EC_XCB_CONN_CLOSED_MEM_INSUFFICIENT,
  EC_XCB_CONN_CLOSED_REQ_LEN_EXCEED,
  EC_XCB_CONN_CLOSED_PARSE_ERR,
  EC_XCB_CONN_CLOSED_INVALID_SCREEN
};

// Functions that will be found using Argument-dependent lookup.
std::string to_string(Errors error);
std::ostream& operator<<(std::ostream& os, Errors error);
inline char const* get_domain(Errors) { return "xcb:org.freedesktop.xcb.Error"; }
std::error_code make_error_code(Errors ec);

} // namespace org::freedesktop::xcb::Error
} // namespace xcb::errors

// Register Errors as valid error code.
namespace std {
template<> struct is_error_code_enum<xcb::errors::org::freedesktop::xcb::Error::Errors> : true_type { };
} // namespace std
