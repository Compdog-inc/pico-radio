#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <lwip/netif.h>
#include <lwip/ip4_addr.h>
#include <lwip/udp.h>
#include <lwip/sockets.h>
#include "udpsocket.h"
#include <string>

using namespace std::literals;

Datagram::Datagram(void *data, uint16_t length, uint16_t port) : data(data),
                                                                 length(length),
                                                                 address(nullptr),
                                                                 port(port)
{
}

Datagram::Datagram(const void *data, uint16_t length, uint16_t port) : data(data),
                                                                       length(length),
                                                                       address(nullptr),
                                                                       port(port)
{
}

Datagram::Datagram(void *data, uint16_t length, const ip_addr_t *address, uint16_t port) : data(data),
                                                                                           length(length),
                                                                                           address(address),
                                                                                           port(port)
{
}

Datagram::Datagram(const void *data, uint16_t length, const ip_addr_t *address, uint16_t port) : data(data),
                                                                                                 length(length),
                                                                                                 address(address),
                                                                                                 port(port)
{
}

Datagram Datagram::asReply(void *data, uint16_t length)
{
    return Datagram(data, length, this->address, this->port);
}

Datagram Datagram::asReply(const void *data, uint16_t length)
{
    return Datagram(data, length, this->address, this->port);
}

/// @brief
/// @param udp
/// @param datagram
/// @return The number of bytes actually sent
static int udp_send_datagram(struct udp_pcb **udp, Datagram *datagram)
{
    if (datagram->length > 0xffff)
    {
        datagram->length = 0xffff;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, datagram->length, PBUF_RAM);
    if (p == NULL)
    {
        return -ENOMEM;
    }

    memcpy(p->payload, datagram->data, datagram->length);

    err_t err = udp_sendto(*udp, p, datagram->address, datagram->port);

    pbuf_free(p);

    if (err != ERR_OK)
    {
        return err;
    }

    return datagram->length;
}

/**
 * @brief This function is called when an UDP datagrm has been received.
 * @param arg The UdpSocket instance that received this
 * @param pcb the udp_pcb which received data
 * @param p the packet buffer that was received
 * @param addr the remote IP address from which the packet was received
 * @param port the remote port from which the packet was received
 */
void udp_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    UdpSocket *sock = (UdpSocket *)arg;
    if (sock->receiveCallback != nullptr)
    {
        Datagram datagram(p->payload, p->len, addr, port);
        sock->receiveCallback(sock, &datagram);
    }
}

UdpSocket::UdpSocket(int port)
{
    udp = udp_new();
    udp_recv(udp, udp_receive_callback, (void *)this);
    if (udp_bind(udp, IP_ANY_TYPE, port) != ERR_OK)
    {
        udp_remove(udp);
        udp = nullptr;
    }
}

UdpSocket::UdpSocket(ip4_addr_t addr, int port)
{
    udp = udp_new();
    udp_recv(udp, udp_receive_callback, (void *)this);
    if (udp_connect(udp, &addr, port) != ERR_OK)
    {
        udp_remove(udp);
        udp = nullptr;
    }
}

UdpSocket::~UdpSocket()
{
    if (udp != nullptr)
    {
        udp_remove(udp);
        udp = nullptr;
    }
}

void UdpSocket::deinit()
{
    if (udp != nullptr)
    {
        udp_remove(udp);
        udp = nullptr;
    }
}

void UdpSocket::disconnect()
{
    udp_disconnect(udp);
}

bool UdpSocket::isOpen()
{
    return udp != nullptr;
}

bool UdpSocket::broadcast(Datagram *datagram)
{
    assert(udp != nullptr);

    u32_t netmask = netif_ip4_netmask(cyw43_state.netif)->addr;
    u32_t ip = netif_ip4_addr(cyw43_state.netif)->addr;
    u32_t broadcast = ip | (~netmask); // set all masked bits to 1

    ip4_addr_t addr = {};
    addr.addr = broadcast;
    datagram->address = &addr;

    return udp_send_datagram(&udp, datagram) == datagram->length;
}

bool UdpSocket::sendDatagram(Datagram *datagram)
{
    assert(udp != nullptr);

    return udp_send_datagram(&udp, datagram) == datagram->length;
}