#include <pico/stdlib.h>
#include <lwip/netif.h>
#include <lwip/ip4_addr.h>
#include <lwip/udp.h>
#include <lwip/sockets.h>
#include "udpsocket.h"
#include <string>
using namespace std::literals;

static int dhcp_socket_sendto(struct udp_pcb **udp, struct netif *nif, const void *buf, size_t len, uint32_t ip, uint16_t port)
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

    ip_addr_t dest;
    IP4_ADDR(ip_2_ip4(&dest), ip >> 24 & 0xff, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
    err_t err;
    if (nif != NULL)
    {
        err = udp_sendto_if(*udp, p, &dest, port, nif);
    }
    else
    {
        err = udp_sendto(*udp, p, &dest, port);
    }

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
    struct netif *nif = ip_current_input_netif();
    auto msg = "howdy"sv;
    dhcp_socket_sendto(&sock->udp, nif, msg.data(), msg.length(), addr->addr, port);
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