#include "sys.h"
#include "xcb-task/org.freedesktop.Xcb.Error/Errors.h"
#include "xcb-task/Error.h"
#include "utils/AIAlert.h"
#include <sstream>
#include "debug.h"

int main()
{
  Debug(debug::init());

  using xcb::errors::org::freedesktop::xcb::Error;

  std::stringstream ss;

  // The error enums can be converted to a std::error_code.
  std::error_code ecA = Error::EC_XCB_CONN_CLOSED_REQ_LEN_EXCEED;
  std::error_code ecB = Error::EC_XCB_CONN_CLOSED_INVALID_SCREEN;
  Dout(dc::notice, "ecA = " << ecA << " [" << ecA.message() << "]; ecB = " << ecB << " [" << ecB.message() << "]");

  ss << ecA << " [" << ecA.message() << "]";
  std::cout << ss.str() << std::endl;
  ASSERT(ss.str() == "xcb:org.freedesktop.xcb.Error:95 [XCB_CONN_CLOSED_REQ_LEN_EXCEED]");
  ss.str(std::string{});
  ss.clear();

  ss << ecB << " [" << ecB.message() << "]";
  std::cout << ss.str() << std::endl;
  ASSERT(ss.str() == "xcb:org.freedesktop.xcb.Error:1 [XCB_CONN_CLOSED_INVALID_SCREEN]");
  ss.str(std::string{});
  ss.clear();

  Error dbe1{XCB_CONN_CLOSED_PARSE_ERR};

  // dbus::Error seamless converts into a std::error_code.
  std::error_code ec1 = dbe1;

  Dout(dc::notice,
      "dbe1 = " << dbe1 << " (error_code = " << ec1 << " [" << ec1.message() << "])");

  using namespace dbus::errors;

  ss << dbe1 << ' ' << ec1 << " [" << ec1.message() << "]";

  ASSERT(ss.str() == "{ \"org.sdbuscpp.Concatenator.Error.NoNumbers\" [some message] } DBus:org.sdbuscpp.Concatenator.Error:1 [NoNumbers]");
  std::cout << ss.str() << std::endl;
  // The corresponding enum in this case is dbus::errors::org::sdbuscpp::Concatenator::Error::NoNumbers
  ASSERT(ec1.value() == org::sdbuscpp::Concatenator::Error::NoNumbers);
  ss.str(std::string{});
  ss.clear();

  Dout(dc::notice, "Success.");
}
