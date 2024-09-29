#ifndef _RADIO_H_
#define _RADIO_H_

#include <stdlib.h>
#include "dhcpserver.h"

/// @brief Wifi Radio that uses cyw43 driver
class Radio
{
public:
    /// @brief Initializes wifi driver and connects to or creates a network
    Radio();
    /// @brief Deinitializes the wifi driver
    ~Radio();

    /// @brief Deinitializes the wifi driver
    void deinit();
    /// @brief Returns true if the radio is initialized
    bool isInitialized();

private:
    bool initialized;
    dhcp_server_t dhcp_server;
};

#endif