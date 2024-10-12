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
#include <vector>

constexpr int WS_SERVER_MAX_CLIENT_COUNT = 10;

class WsServer
{
public:
    struct ClientEntry
    {
        Guid guid;
        WebSocket *ws;
        std::string requestedPath;

        ClientEntry();
        ClientEntry(Guid guid, WebSocket *ws, std::string requestedPath);
    };

    WsServer(int port);
    ~WsServer();

    void start();
    void stop();

    bool isListening();
    bool isClientConnected(const Guid &guid);
    void disconnectClient(const Guid &guid);

    bool send(const Guid &guid, std::string_view data, WebSocketMessageType messageType = WebSocketMessageType::Text);
    bool send(const Guid &guid, const uint8_t *data, size_t length, WebSocketMessageType messageType = WebSocketMessageType::Binary);

    EventHandler<void (*)()> clientConnected;
    EventHandler<void (*)()> clientDisconnected;
    EventHandler<void (*)()> messageReceived;

    std::vector<ClientEntry *> clients;

    void handleRawConnection(TcpClient *client);
    void acceptConnections();

    typedef std::string (*WsServerProtocolCallback)(const std::vector<std::string> &requestedProtocols);
    WsServerProtocolCallback protocolCallback = nullptr;

private:
    int port;
    TcpListener *listener;
    TaskHandle_t acceptConnectionsTask;
};

#endif