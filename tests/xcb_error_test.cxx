#include "sys.h"
#include "xcb-task/org.freedesktop.Xcb.Error/Errors.h"
#include "utils/AIAlert.h"
#include <sstream>
#include <xcb/xcb.h>
#include "debug.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif

int main()
{
  Debug(debug::init());

  using namespace xcb::errors;
  using namespace org::freedesktop::xcb;

  std::stringstream ss;

  // The error enums can be converted to a std::error_code.
  std::error_code ecA = Error::XE_XCB_CONN_CLOSED_REQ_LEN_EXCEED;
  std::error_code ecB = Error::XE_XCB_CONN_CLOSED_INVALID_SCREEN;
  Dout(dc::notice, "ecA = " << ecA << " [" << ecA.message() << "]; ecB = " << ecB << " [" << ecB.message() << "]");

  ss << ecA << " [" << ecA.message() << "]";
  ASSERT(ss.str() == "xcb:4 [XCB_CONN_CLOSED_REQ_LEN_EXCEED]");
  ss.str(std::string{});
  ss.clear();

  ss << ecB << " [" << ecB.message() << "]";
  ASSERT(ss.str() == "xcb:6 [XCB_CONN_CLOSED_INVALID_SCREEN]");
  ss.str(std::string{});
  ss.clear();

  Error dbe1 = static_cast<Error>(XCB_CONN_CLOSED_PARSE_ERR);

  // xcb::errors::org::freedesktop::xcb::Error seamless converts into a std::error_code.
  std::error_code ec1 = dbe1;

  Dout(dc::notice,
      "dbe1 = " << dbe1 << " (error_code = " << ec1 << " [" << ec1.message() << "])");

  ss << dbe1 << ' ' << ec1 << " [" << ec1.message() << "]";

  ASSERT(ss.str() == "XCB_CONN_CLOSED_PARSE_ERR xcb:5 [XCB_CONN_CLOSED_PARSE_ERR]");
  std::cout << ss.str() << std::endl;
  // The corresponding enum in this case is xcb::errors::org::freedesktop::xcb::Error::XCB_CONN_CLOSED_PARSE_ERR.
  ASSERT(ec1.value() == XCB_CONN_CLOSED_PARSE_ERR);
  ss.str(std::string{});
  ss.clear();

  try
  {
    THROW_ALERTC(static_cast<Error>(XCB_CONN_CLOSED_REQ_LEN_EXCEED), "There was an error");
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error << " caught in xcb_error_test.cxx");
  }

  Dout(dc::notice, "Success.");
}
