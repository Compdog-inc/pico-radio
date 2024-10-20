#ifndef _UDP_SOCKET_H_
#define _UDP_SOCKET_H_

#include <stdlib.h>
#include <lwip/netif.h>
#include <lwip/ip4_addr.h>

/// @brief Represents a UDP payload, together with a target address and por
struct Datagram
{
    /// @brief Create a new Datagram with a specified port. Used for broadcasting
    /// @param data Datagram payload
    /// @param length Payload length
    /// @param port The target port to broadcast to
    Datagram(void *data, uint16_t length, uint16_t port);
    /// @brief Create a new Datagram with a specified port. Used for broadcasting
    /// @param data Datagram payload
    /// @param length Payload length
    /// @param port The target port to broadcast to
    Datagram(const void *data, uint16_t length, uint16_t port);
    /// @brief Create a new Datagram with a specified address and port. Used for individual addressing
    /// @param data Datagram payload
    /// @param length Payload length
    /// @param address The target address to broadcast to
    /// @param port The target port to broadcast to
    Datagram(void *data, uint16_t length, const ip_addr_t *address, uint16_t port);
    /// @brief Create a new Datagram with a specified address and port. Used for individual addressing
    /// @param data Datagram payload
    /// @param length Payload length
    /// @param address The target address to broadcast to
    /// @param port The target port to broadcast to
    Datagram(const void *data, uint16_t length, const ip_addr_t *address, uint16_t port);

    /// @brief The payload data
    const void *data;
    /// @brief The length of the payload
    uint16_t length;
    /// @brief The target address of this Datagram
    const ip_addr_t *address;
    /// @brief the target port of this Datagram
    uint16_t port;

    /// @brief Create a new Datagram with the same address and port
    /// @param data The new payload
    /// @param length The length of the payload
    /// @return A new Datagram
    Datagram asReply(void *data, uint16_t length);
    /// @brief Create a new Datagram with the same address and port
    /// @param data The new payload
    /// @param length The length of the payload
    /// @return A new Datagram
    Datagram asReply(const void *data, uint16_t length);
};

/// @brief A UDP socket implementation
class UdpSocket
{
public:
    /// @brief Create and bind a UDP socket on a port
    /// @param port The port to bind to
    UdpSocket(int port);
    /// @brief Create and connect a UDP socket to a remote address and port
    /// @param addr The remote address to connect to
    /// @param port The remote port to connect to
    UdpSocket(ip4_addr_t addr, int port);
    /// @brief Create and connect a UDP socket to a remote address and port, while binding to a local port
    /// @param addr The remote address to connect to
    /// @param localPort The local port to bind to
    /// @param remotePort The remote port to connect to
    UdpSocket(ip4_addr_t addr, int localPort, int remotePort);
    /// @brief Remove the UDP socket
    ~UdpSocket();

    /// @brief Remove the UDP socket
    void deinit();
    /// @brief Only disconnect the UDP socket from a remote port
    void disconnect();
    /// @brief Returns true if the socket is bound
    bool isOpen();

    /// @brief Broadcasts a Datagram over the local network
    ///        using the subnet mask and the Datagram's port.
    /// @param datagram The datagram to broadcast
    /// @return True if succeeded
    /// @note  This overwrites the existing address of the Datagram
    bool broadcast(Datagram *datagram);
    /// @brief Sends a Datagram to a specific address and port
    /// @param datagram The datagram to send
    /// @return True if succeeded
    bool sendDatagram(Datagram *datagram);

    // Internal socket reference
    struct udp_pcb *udp;

    /// @brief Custom args for the udp callbacks, set by the user
    void *callbackArgs = nullptr;

    /// @brief This callback is called whenever the socket receives a Datagram from an address
    typedef void (*UdpSocketReceiveCallback)(UdpSocket *socket, Datagram *datagram, void *args);

    /// @brief Null by default, set this if you want to handle incoming Datagrams
    UdpSocketReceiveCallback receiveCallback = nullptr;
};

#endif