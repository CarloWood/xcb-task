#pragma once

#include "ConnectionData.h"
#include "XcbConnection.h"
#include "statefultask/BrokerKey.h"
#include <farmhash.h>

namespace xcb {

class ConnectionBrokerKey : public statefultask::BrokerKey, public ConnectionData
{
 protected:
  uint64_t hash() const final
  {
    return util::Hash64(m_display_name);
  }

  void initialize(boost::intrusive_ptr<AIStatefulTask> task) const final
  {
    task::XcbConnection& xcb_connection = static_cast<task::XcbConnection&>(*task);
    ConnectionData::initialize(xcb_connection);
  }

  unique_ptr copy() const final
  {
    return unique_ptr(new ConnectionBrokerKey(*this));
  }

  bool equal_to_impl(BrokerKey const& other) const final
  {
    ConnectionData const& data = static_cast<ConnectionBrokerKey const&>(other);
    return ConnectionData::operator==(data);
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const final
  {
    ConnectionData::print_on(os);
  }

  friend std::ostream& operator<<(std::ostream& os, ConnectionBrokerKey const& connection_broker_key)
  {
    connection_broker_key.print_on(os);
    return os;
  }
#endif
};

} // namespace xcb
