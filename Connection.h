#pragma once
    
#include "evio/RawInputDevice.h"
#include "evio/RawOutputDevice.h"
#include "debug.h"

namespace task {

} // namespace task

namespace xcb {

class Connection : public evio::RawInputDevice, public evio::RawOutputDevice
{
 private:
  xcb_connection_t* m_connection;

 public:
  void connect()
  {
    DoutEntering(dc::dbus, "xcb::Connection::connect()");
    int screen_index;
    m_connection = xcb_connect(nullptr, &screen_index);
    int error = xcb_connection_has_error(m_connection);
    if (error)
    {

    }
  }
};

} // namespace xcb
