#pragma once

#include "Connection.h"
#include "ConnectionData.h"
#include "utils/AIAlert.h"
#include "statefultask/AIStatefulTask.h"
#include "debug.h"
#include <string>
#include <iosfwd>

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
  static constexpr state_type state_end = XcbConnection_done + 1;

  /// Construct a XcbConnection object.
  XcbConnection(CWDEBUG_ONLY(bool debug));

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

  /// Implementation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override;
  char const* task_name_impl() const override;

  /// Run bs_initialize.             
  void initialize_impl() override;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
