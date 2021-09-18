#include "sys.h"
#include "XcbConnection.h"
#include "Connection.h"
#include "utils/split.h"
#include <cstdlib>
#include <array>
#include <string_view>
#ifdef CWDEBUG
#include <iostream>
#endif

namespace task {

XcbConnection::XcbConnection(CWDEBUG_ONLY(bool debug,) std::function<void()> cb_closed) CWDEBUG_ONLY(: AIStatefulTask(debug))
{
  DoutEntering(dc::statefultask(mSMDebug), "XcbConnection() [" << (void*)this << "]");
  m_connection = evio::create<xcb::Connection>();
  m_connection->set_closed_callback(std::move(cb_closed));
}

void XcbConnection::close()
{
  DoutEntering(dc::statefultask(mSMDebug), "XcbConnection::close()");
  if (m_connection)
  {
    m_connection->close();
    m_connection.reset();
  }
}

char const* XcbConnection::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(XcbConnection_start);
    AI_CASE_RETURN(XcbConnection_done);
  }
  ASSERT(false);
  return "UNKNOWN STATE";
}

void XcbConnection::initialize_impl()
{
  DoutEntering(dc::statefultask(mSMDebug), "XcbConnection::initialize_impl() [" << (void*)this << "]");
  set_state(XcbConnection_start);
}

void XcbConnection::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case XcbConnection_start:
      m_connection->connect(m_display_name);
      set_state(XcbConnection_done);
      [[fallthrough]];
    case XcbConnection_done:
      finish();
      break;
  }
}

} // namespace task

#ifdef CWDEBUG
namespace xcb {

namespace {

unsigned int read_unsigned_int(char const*& ptr)
{
  if (*ptr == 0)
    THROW_ALERT("Unexpected end of string (expected digit)");
  if (!std::isdigit(*ptr))
    THROW_ALERT("Expected digit instead of '[CHAR]'", AIArgs("[CHAR]", *ptr));
  unsigned int result = 0;
  while (std::isdigit(*ptr))
  {
    result = result * 10 + *ptr - '0';
    ++ptr;
  }
  return result;
}

} // namespace

std::string ConnectionData::get_canonical_display_name() const
{
  std::string display_name = m_display_name;

  // If empty, use the DISPLAY environment variable (if set).
  if (display_name.empty())
  {
    char const* DISPLAY_environment_variable = std::getenv("DISPLAY");
    if (DISPLAY_environment_variable == nullptr)
      DISPLAY_environment_variable = ":0";
    display_name = DISPLAY_environment_variable;
  }

  // Canonicalize the DISPLAY name.
  try
  {
    std::array<std::string_view, 2> tokens;
    utils::splitN(display_name, ':', tokens);   // Exactly one colon is required, otherwise this will throw AIAlert::Error.
    std::string hostname = tokens[0].empty() ? std::string("unix") : std::string(tokens[0]);
    char const* ptr = tokens[1].data();
    unsigned int display_number = read_unsigned_int(ptr);
    if (*ptr != '.' && *ptr != 0)
      THROW_ALERT("Unexpected character(s) after display number");
    unsigned int screen_number = 0;
    if (*ptr == '.')
    {
      ++ptr;
      screen_number = read_unsigned_int(ptr);
      if (*ptr != 0)
        THROW_ALERT("Unexpected character(s) after screen number");
    }
    std::stringstream ss;
    ss << hostname << ':' << display_number << '.' << screen_number;
    display_name = ss.str();
  }
  catch (AIAlert::Error const& error)
  {
    THROW_ALERT("Parse error scanning DISPLAY name \"[DISPLAY]\"", AIArgs("[DISPLAY]", display_name), error);
  }

  return display_name;
}

void ConnectionData::initialize(task::XcbConnection& xcb_connection) const
{
  std::string canonical_display_name = get_canonical_display_name();
  if (canonical_display_name != m_display_name)
  {
    // Store the result.
    xcb_connection.set_display_name(canonical_display_name);
  }
}

void ConnectionData::print_on(std::ostream& os) const
{
  os << "{\"" << m_display_name << "\"}";
}

} // namespace xcb
#endif
