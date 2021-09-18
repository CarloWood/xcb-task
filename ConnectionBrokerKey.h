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

  unique_ptr canonical_copy() const final
  {
    auto new_key = new ConnectionBrokerKey(*this);
    new_key->m_display_name = get_canonical_display_name();
    return unique_ptr(new_key);
  }

  bool equal_to_impl(BrokerKey const& other) const final
  {
    ConnectionData const& data = static_cast<ConnectionBrokerKey const&>(other);
    return ConnectionData::operator==(data);
  }

  void print_on(std::ostream& os) const final
  {
    ConnectionData::print_on(os);
  }
};

} // namespace xcb
