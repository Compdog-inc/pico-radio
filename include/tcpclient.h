#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include <stdlib.h>
#include <lwip/ip4_addr.h>

constexpr uint32_t TCP_INFINITE_TIMEOUT = ~((uint32_t)0);

/// @brief A tcp client implementation
class TcpClient
{
public:
    /// @brief Wraps a tcp client around an existing network socket
    /// @param sock The network socket
    TcpClient(int sock);
    /// @brief Creates and connects a tcp client to a server
    /// @param addr The address of the server
    /// @param port The port to connect to
    TcpClient(ip4_addr_t addr, int port);
    /// @brief Closes the network socket
    ~TcpClient();

    /// @brief Closes the network socket
    void disconnect();
    /// @brief Returns true if the client is connected
    bool isConnected();

    /// @brief Reads a specified amount of bytes into a buffer
    /// @param mem The output buffer
    /// @param len The number of bytes to read
    /// @return The number of bytes actually read. A negative number is an error.
    ssize_t readBytes(void *mem, size_t len);
    /// @brief Reads a specified amount of bytes into a buffer with a timeout
    /// @param mem The output buffer
    /// @param len The number of bytes to read
    /// @param timeout The timeout in milliseconds
    /// @return The number of bytes actually read. A negative number is an error and zero is timeout.
    ssize_t readBytes(void *mem, size_t len, uint32_t timeout);

    /// @brief Writes a buffer of data
    /// @param data The buffer to write
    /// @param size The number of bytes to write
    /// @return The number of bytes actually written. A negative number is an error.
    ssize_t writeBytes(const void *data, size_t size);

private:
    int sock;
    bool connected;
};

#endif