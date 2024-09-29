#ifndef _TCP_LISTENER_H_
#define _TCP_LISTENER_H_

#include <stdlib.h>
#include "tcpclient.h"

/// @brief A tcp server/listener implementation
class TcpListener
{
public:
    /// @brief Bind the listener on a port and start listening
    /// @param port The port to bind to
    TcpListener(int port);
    /// @brief Close the network socket
    ~TcpListener();

    /// @brief Wait for a client to connect
    /// @return The connected tcp client
    /// @note To reject a connection, call disconnect() on the client immediately.
    TcpClient *acceptClient();

    /// @brief Close the network socket
    void stop();
    /// @brief Returns true if the tcp listener is open
    bool isOpen();

private:
    int sock;
    bool open;
};

#endif