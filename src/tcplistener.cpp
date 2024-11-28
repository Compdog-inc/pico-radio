#include <pico/stdlib.h>
#include <lwip/netif.h>
#include <lwip/ip4_addr.h>
#include <lwip/sockets.h>
#include "tcplistener.h"
#include "tcpclient.h"

TcpListener::TcpListener(int port)
{
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    struct sockaddr_in listen_addr = {};
    listen_addr.sin_len = sizeof(struct sockaddr_in);
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);
    listen_addr.sin_addr.s_addr = 0;

    if (sock < 0)
    {
        printf("[RADIO] Unable to create TCP socket: error %d\n", errno);
        open = false;
        return;
    }

    const int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        printf("[RADIO] Unable to set socket opt (SO_REUSEADDR): error %d\n", errno);
        open = false;
        return;
    }

    if (bind(sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0)
    {
        printf("[RADIO] Unable to bind socket: error %d\n", errno);
        open = false;
        return;
    }

    if (listen(sock, 1) < 0)
    {
        printf("[RADIO] Unable to listen on socket: error %d\n", errno);
        open = false;
        return;
    }

    printf("[RADIO] Started TCP server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), ntohs(listen_addr.sin_port));

    open = true;
}

TcpListener::~TcpListener()
{
    if (open)
    {
        close(sock);
        open = false;
    }
}

void TcpListener::stop()
{
    if (open)
    {
        close(sock);
        open = false;
    }
}

bool TcpListener::isOpen()
{
    return open;
}

TcpClient *TcpListener::acceptClient()
{
    assert(open == true);

    struct sockaddr_storage remote_addr;
    socklen_t len = sizeof(remote_addr);
    int conn_sock = accept(sock, (struct sockaddr *)&remote_addr, &len);
    if (conn_sock < 0)
    {
        printf("[RADIO] Unable to accept incoming tcp connection: error %d\n", errno);
        return nullptr;
    }

    struct sockaddr_in *sin = (struct sockaddr_in *)&remote_addr;
    return new TcpClient(conn_sock, *sin);
}