#pragma once

#include "utils/AIAlert.h"
#include <string>
#include "debug.h"

namespace task {
class XcbConnection;
} // namespace task

namespace xcb {

// Initialization data (before running the task).
class ConnectionData
{
 protected:
  // Input variables.
  std::string m_display_name;           // Uses the DISPLAY environment variable if not set.

  ConnectionData() = default;

  // Used by XcbConnectionBrokerKey.
  void initialize(task::XcbConnection& xcb_connection) const;

  bool operator==(ConnectionData const& other) const
  {
    // These should already be set; if that is not possible then we should use
    // the DISPLAY environment variable to compare with instead.
    ASSERT(!m_display_name.empty() && !other.m_display_name.empty());
    return m_display_name == other.m_display_name;
  }

 public:
  void canonicalize();

  // Set the X server DISPLAY that we should connect to.
  void set_display_name(std::string display_name)
  {
    DoutEntering(dc::notice, "ConnectionData::set_display_name(\"" << display_name << "\")");
    m_display_name = std::move(display_name);
    if (m_display_name.empty())
      THROW_ALERT("Attempting to set an empty DISPLAY name");
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace xcb
