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

// https://stackoverflow.com/a/12730776
int getSO_ERROR(int fd)
{
    int err = 1;
    socklen_t len = sizeof err;
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len))
        panic("getSO_ERROR");
    if (err)
        errno = err; // set errno to the socket SO_ERROR
    return err;
}

void closeSocket(int fd)
{
    if (fd >= 0)
    {
        getSO_ERROR(fd);                              // first clear any errors, which can cause close to fail
        if (shutdown(fd, SHUT_RDWR) < 0)              // secondly, terminate the 'reliable' delivery
            if (errno != ENOTCONN && errno != EINVAL) // SGI causes EINVAL
                printf("[RADIO] Unable to shutdown: error %d\n", errno);
        if (close(fd) < 0) // finally call close()
            printf("[RADIO] Unable to close: error %d\n", errno);
    }
}

TcpClient::~TcpClient()
{
    if (connected)
    {
        closeSocket(sock);
        connected = false;
    }
}

void TcpClient::disconnect()
{
    if (connected)
    {
        closeSocket(sock);
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