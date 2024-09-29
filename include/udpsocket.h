#ifndef _UDP_SOCKET_H_
#define _UDP_SOCKET_H_

#include <stdlib.h>
#include <lwip/ip4_addr.h>

struct Datagram
{
    Datagram(void *data, uint16_t length, uint16_t port);
    Datagram(const void *data, uint16_t length, uint16_t port);
    Datagram(void *data, uint16_t length, const ip_addr_t *address, uint16_t port);
    Datagram(const void *data, uint16_t length, const ip_addr_t *address, uint16_t port);

    const void *data;
    uint16_t length;
    const ip_addr_t *address;
    uint16_t port;

    Datagram asReply(void *data, uint16_t length);
    Datagram asReply(const void *data, uint16_t length);
};

class UdpSocket
{
public:
    UdpSocket(int port);
    UdpSocket(ip4_addr_t addr, int port);
    ~UdpSocket();

    void deinit();
    void disconnect();
    bool isOpen();

    bool broadcast(Datagram *datagram);
    bool sendDatagram(Datagram *datagram);

    struct udp_pcb *udp;

    typedef void (*UdpSocketReceiveCallback)(UdpSocket *socket, Datagram *datagram);

    UdpSocketReceiveCallback receiveCallback = nullptr;
};

#endif