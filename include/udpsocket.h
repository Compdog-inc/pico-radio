#ifndef _UDP_SOCKET_H_
#define _UDP_SOCKET_H_

#include <stdlib.h>
#include <lwip/ip4_addr.h>

class UdpSocket
{
public:
    UdpSocket(int port);
    UdpSocket(ip4_addr_t addr, int port);
    ~UdpSocket();

    void deinit();
    void disconnect();
    bool isOpen();

    struct udp_pcb *udp;
};

#endif