#include <pico/stdlib.h>
#include <lwip/netif.h>
#include <lwip/ip4_addr.h>
#include <lwip/sockets.h>
#include "tcpclient.h"

TcpClient::TcpClient(int sock) : sock(sock), connected(true)
{
}

TcpClient::TcpClient(ip4_addr_t addr, int port)
{
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    struct sockaddr_in listen_addr = {};
    listen_addr.sin_len = sizeof(struct sockaddr_in);
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);
    listen_addr.sin_addr.s_addr = addr.addr;

    if (sock < 0)
    {
        printf("[RADIO] Unable to create TCP socket: error %d\n", errno);
        connected = false;
        return;
    }

    if (connect(sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0)
    {
        printf("[RADIO] Unable to connect socket: error %d\n", errno);
        connected = false;
        return;
    }

    connected = true;
}

TcpClient::~TcpClient()
{
    if (connected)
    {
        close(sock);
        connected = false;
    }
}

void TcpClient::disconnect()
{
    if (connected)
    {
        close(sock);
        connected = false;
    }
}

bool TcpClient::isConnected()
{
    return connected;
}

ssize_t TcpClient::readBytes(void *mem, size_t len)
{
    return readBytes(mem, len, TCP_INFINITE_TIMEOUT);
}

/// @brief Checks if any data is available on the socket within the timeout
/// @param sock The socket to check
/// @param tmo Timeout in milliseconds
/// @return 0 if data becomes available, otherwise -1
int readtmo(int sock, uint32_t tmo)
{
    fd_set recSet;
    struct timeval tv;
    tv.tv_sec = tmo / 1000;           // ms -> sec
    tv.tv_usec = (tmo % 1000) * 1000; // ms -> us
    FD_ZERO(&recSet);
    FD_SET(sock, &recSet);
    return select(sock + 1, &recSet, 0, 0, &tv) > 0 ? 0 : -1; // call system select() to check for data asynchronously
}

ssize_t TcpClient::readBytes(void *mem, size_t len, uint32_t timeout)
{
    assert(connected == true);

    if (len <= 0)
        return -1;

    ssize_t recLen;
    if (timeout != TCP_INFINITE_TIMEOUT)
    {
        if (readtmo(sock, timeout)) // check if any data is available
            return 0;               // hit timeout
    }

    recLen = recv(sock, mem, len, 0);
    if (recLen <= 0)
    {
        connected = false; // socket error means not connected
        return -1;
    }
    return recLen;
}

ssize_t TcpClient::writeBytes(const void *data, size_t size)
{
    assert(connected == true);
    ssize_t res = send(sock, data, size, 0);
    if (res < 0)
    {
        connected = false; // socket error means not connected
    }
    return res;
}