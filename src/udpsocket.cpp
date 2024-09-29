#include <pico/stdlib.h>
#include <lwip/netif.h>
#include <lwip/ip4_addr.h>
#include <lwip/udp.h>
#include <lwip/sockets.h>
#include "udpsocket.h"
#include <string>

using namespace std::literals;

static int udp_send_buffer(struct udp_pcb **udp, const void *buf, size_t len, const ip_addr_t *addr, uint16_t port)
{
    if (len > 0xffff)
    {
        len = 0xffff;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL)
    {
        return -ENOMEM;
    }

    memcpy(p->payload, buf, len);

    err_t err = udp_sendto(*udp, p, addr, port);

    pbuf_free(p);

    if (err != ERR_OK)
    {
        return err;
    }

    return len;
}

/**
 * @brief This function is called when an UDP datagrm has been received on the port UDP_PORT.
 * @param arg user supplied argument (udp_pcb.recv_arg)
 * @param pcb the udp_pcb which received data
 * @param p the packet buffer that was received
 * @param addr the remote IP address from which the packet was received
 * @param port the remote port from which the packet was received
 * @retval None
 */
void udp_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    UdpSocket *sock = (UdpSocket *)arg;
    auto msg = "howdy!\n"sv;
    udp_send_buffer(&sock->udp, msg.data(), msg.length(), addr, port);
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