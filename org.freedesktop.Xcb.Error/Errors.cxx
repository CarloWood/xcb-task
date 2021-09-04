#include "sys.h"
#include "Errors.h"
#include <magic_enum.hpp>
#include <iostream>

namespace xcb::errors {
namespace org::freedesktop::xcb {

std::string to_string(Error error)
{
  auto sv = magic_enum::enum_name(error);
  sv.remove_prefix(3);  // Remove "XE_" from the name.
  return std::string{sv};
}

std::ostream& operator<<(std::ostream& os, Error error)
{
  os << to_string(error);
  return os;
}

//std::error_code ErrorDomain::get_error_code(std::string const& member_name) const
//{
//  return make_error_code(*magic_enum::enum_cast<Error>("EC_" + member_name));
//}

//----------------------------------------------------------------------------
// xcb error codes (as returned by xcb_connection_has_error).

namespace {

struct XcbErrorCategory : std::error_category
{
  char const* name() const noexcept override;
  std::string message(int ev) const override;
};

char const* XcbErrorCategory::name() const noexcept
{
  return "xcb";
}

std::string XcbErrorCategory::message(int ev) const
{
  auto error = static_cast<Error>(ev);
  return to_string(error);
}

XcbErrorCategory const theXcbErrorCategory { };

} // namespace

std::error_code make_error_code(Error code)
{
  return std::error_code(static_cast<int>(code), theXcbErrorCategory);
}

} // namespace org::freedesktop::xcb::Error
} // namespace xcb::errors
