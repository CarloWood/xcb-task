#pragma once
    
#include "evio/RawInputDevice.h"
#include "evio/RawOutputDevice.h"
#include "org.freedesktop.Xcb.Error/Errors.h"
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
    DoutEntering(dc::notice, "xcb::Connection::connect()");

    using namespace xcb::errors;

    int screen_index;
    m_connection = xcb_connect(nullptr, &screen_index);
    auto error = static_cast<org::freedesktop::xcb::Error>(xcb_connection_has_error(m_connection));
    if (error != org::freedesktop::xcb::Error::Success)
    {

    }
  }
};

} // namespace xcb
