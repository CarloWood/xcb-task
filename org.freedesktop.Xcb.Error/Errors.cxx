#include "sys.h"
#include "Errors.h"
#include <magic_enum.hpp>
#include <iostream>

namespace xcb::errors {
namespace org::freedesktop::xcb::Error {

std::string to_string(Errors error)
{
  auto sv = magic_enum::enum_name(error);
  sv.remove_prefix(3);  // Remove "EC_" from the name (see generate_SystemErrors.sh).
  return std::string{sv};
}

std::ostream& operator<<(std::ostream& os, Errors error)
{
  os << to_string(error);
  return os;
}

std::error_code ErrorDomain::get_error_code(std::string const& member_name) const
{
  return make_error_code(*magic_enum::enum_cast<Errors>("EC_" + member_name));
}

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
  Errors error = reinterpret_cast<Errors>(ev);
  return to_string(error);
}

XcbErrorCategory const theXcbErrorCategory { };

} // namespace

std::error_code make_error_code(xcb_error_codes code)
{
  return std::error_code(static_cast<int>(code), theXcbErrorCategory);
}

} // namespace xcb
