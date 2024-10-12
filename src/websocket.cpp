#include <pico/stdlib.h>
#include "websocket.h"

WebSocket::WebSocket(TcpClient *tcp) : tcp(tcp)
{
    sendMutex = xSemaphoreCreateMutex();
}

void WebSocket::close()
{
    tcp->disconnect();
    tcp->~TcpClient();
}

bool WebSocket::isConnected()
{
    return tcp->isConnected();
}

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)         \
    ((byte) & 0x80 ? '1' : '0'),     \
        ((byte) & 0x40 ? '1' : '0'), \
        ((byte) & 0x20 ? '1' : '0'), \
        ((byte) & 0x10 ? '1' : '0'), \
        ((byte) & 0x08 ? '1' : '0'), \
        ((byte) & 0x04 ? '1' : '0'), \
        ((byte) & 0x02 ? '1' : '0'), \
        ((byte) & 0x01 ? '1' : '0')

void WebSocket::joinMessageLoop()
{
    uint8_t buf[256];

    while (isConnected())
    {
        if (tcp->readBytes(buf, 1, 1000) == 1)
        {
            printf("Recieved frame 0b" BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(buf[0]));
        }
    }
}

bool WebSocket::send(std::string_view data, WebSocketMessageType messageType)
{
    return send((const uint8_t *)data.data(), data.length(), messageType);
}

bool WebSocket::send(const uint8_t *data, size_t length, WebSocketMessageType messageType)
{
    if (!xSemaphoreTake(sendMutex, 500))
    {
        return false;
    }

    // send ws

    xSemaphoreGive(sendMutex);
    return true;
}