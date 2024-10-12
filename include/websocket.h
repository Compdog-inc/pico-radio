#ifndef _WEBSOCKET_H_
#define _WEBSOCKET_H_

#include <stdlib.h>
#include <string>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "tcpclient.h"

enum class WebSocketMessageType
{
    Text,
    Binary,
    Close
};

class WebSocket
{
public:
    WebSocket(TcpClient *tcp);

    void close();

    bool isConnected();

    bool send(std::string_view data, WebSocketMessageType messageType = WebSocketMessageType::Text);
    bool send(const uint8_t *data, size_t length, WebSocketMessageType messageType = WebSocketMessageType::Binary);

    void joinMessageLoop();

private:
    TcpClient *tcp;
    SemaphoreHandle_t sendMutex;
};

#endif