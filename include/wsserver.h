#ifndef _WS_SERVER_H_
#define _WS_SERVER_H_

#include <stdlib.h>
#include <string>
#include <FreeRTOS.h>
#include <task.h>
#include "tcplistener.h"
#include "tcpclient.h"
#include "eventhandler.h"
#include "guid.h"
#include "websocket.h"
#include <semphr.h>

enum class WebSocketMessageType
{
    Text,
    Binary,
    Close
};

constexpr int WS_SERVER_MAX_CLIENT_COUNT = 10;

class WsServer
{
public:
    struct ClientEntry
    {
        Guid guid;
        WebSocket *ws;
        std::string requestedPath;
        SemaphoreHandle_t sendMutex;

        ClientEntry();
        ClientEntry(Guid guid, WebSocket *ws, std::string requestedPath, SemaphoreHandle_t sendMutex);
    };

    WsServer(int port);
    ~WsServer();

    void start();
    void stop();

    bool isListening();
    bool isClientConnected(const Guid &guid);
    void disconnectClient(const Guid &guid);

    bool Send(const Guid &guid, std::string_view data, WebSocketMessageType messageType = WebSocketMessageType::Text);
    bool Send(const Guid &guid, const uint8_t *data, size_t length, WebSocketMessageType messageType = WebSocketMessageType::Binary);

    EventHandler<void (*)()> clientConnected;
    EventHandler<void (*)()> clientDisconnected;
    EventHandler<void (*)()> messageReceived;

    int clientCount;
    ClientEntry clients[WS_SERVER_MAX_CLIENT_COUNT];

    void handleRawConnection(TcpClient *client);
    void acceptConnections();

private:
    int port;
    TcpListener *listener;
    TaskHandle_t acceptConnectionsTask;
};

#endif