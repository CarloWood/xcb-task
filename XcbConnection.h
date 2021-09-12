#pragma once

#include "Connection.h"
#include "utils/AIAlert.h"
#include "statefultask/AIStatefulTask.h"
#include "debug.h"
#include <string>
#include <iosfwd>

namespace task {
class XcbConnection;
} // namespace task;

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
  // Set the X server DISPLAY that we should connect to.
  void set_display_name(std::string display_name)
  {
    DoutEntering(dc::notice, "set_display_name(\"" << display_name << "\")");
    m_display_name = std::move(display_name);
    if (m_display_name.empty())
      THROW_ALERT("Attempting to set an empty DISPLAY name.");
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace xcb

namespace task {

class XcbConnection : public AIStatefulTask, public xcb::ConnectionData
{
 private:
  boost::intrusive_ptr<xcb::Connection> m_connection;           // evio device.

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum XcbConnection_state_type {
    XcbConnection_start = direct_base_type::state_end,
    XcbConnection_done,
  };

 public:
  /// One beyond the largest state of this task.
  static state_type constexpr state_end = XcbConnection_done + 1;

  /// Construct a XcbConnection object.
  XcbConnection(CWDEBUG_ONLY(bool debug = false));

  boost::intrusive_ptr<xcb::Connection> const& connection() const
  {
    return m_connection;
  }

  void close();

 protected:
  /// Call finish() (or abort()), not delete.
  ~XcbConnection() override
  {
    DoutEntering(dc::statefultask(mSMDebug), "~XcbConnection() [" << (void*)this << "]");
    close();
  }

  /// Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override;

  /// Run bs_initialize.             
  void initialize_impl() override;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
