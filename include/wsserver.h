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

/// @brief The maximum number of clients supported by this server
constexpr int WS_SERVER_MAX_CLIENT_COUNT = 10;

/// @brief A WebSocket Server implementation
class WsServer
{
public:
    /// @brief Client entry structure used for storing a list of connected clients
    struct ClientEntry
    {
        /// @brief The GUID of the client
        Guid guid;
        /// @brief The WebSocket of the client
        WebSocket *ws;
        /// @brief The requested path by the client
        std::string requestedPath;

        ClientEntry();
        ClientEntry(Guid guid, WebSocket *ws, std::string requestedPath);
    };

    /// @brief Create a new WebSocket server at a port
    /// @param port The port to listen for new connections on
    /// @note Does not create the socket
    WsServer(int port);
    /// @brief Close all connections and free resources
    ~WsServer();

    /// @brief Start listening on the target port
    void start();
    /// @brief Stop the server
    void stop();

    /// @brief Returns true if the server is listening for new connections
    bool isListening();
    /// @brief Returns true if a client exists with the specified guid and is connected
    bool isClientConnected(const Guid &guid);
    /// @brief Gracefully disconnectes a client with guid
    void disconnectClient(const Guid &guid);

    /// @brief Sends a ping frame to a client
    /// @param guid The guid of the client
    void ping(const Guid &guid);
    /// @brief Sends a ping frame to a client with a custom payload
    /// @param guid The guid of the client
    /// @param payload The payload
    /// @param payloadLength The length of the payload
    void ping(const Guid &guid, const uint8_t *payload, size_t payloadLength);

    /// @brief Sends a text message to a client
    /// @param guid The guid of the client
    /// @param data The message
    /// @param messageType Usually WebSocketMessageType::Text
    /// @return True on success
    bool send(const Guid &guid, std::string_view data, WebSocketMessageType messageType = WebSocketMessageType::Text);
    /// @brief Sends a binary message to a client
    /// @param guid The guid of the client
    /// @param data The payload
    /// @param length The length of the payload
    /// @param messageType Determines how the binary data is interpreted
    /// @return True on success
    bool send(const Guid &guid, const uint8_t *data, size_t length, WebSocketMessageType messageType = WebSocketMessageType::Binary);

    EventHandler<void (*)()> clientConnected;
    EventHandler<void (*)()> clientDisconnected;
    EventHandler<void (*)()> messageReceived;

    /// @brief List of currently connected clients
    std::vector<ClientEntry *> clients;

    /// @brief Used internally to handle a client connection
    void handleRawConnection(TcpClient *client);
    /// @brief Used internally to start accepting connections
    void acceptConnections();

    typedef std::string (*WsServerProtocolCallback)(const std::vector<std::string> &requestedProtocols);
    WsServerProtocolCallback protocolCallback = nullptr;

    /// @brief Callback for pong frames, contains the WsServer instance, guid and an optional payload
    typedef void (*WsServerPongCallback)(WsServer *server, const Guid &guid, const uint8_t *payload, size_t payloadLength);
    /// @brief Called whenever a ping is answered with a pong
    WsServerPongCallback pongCallback = nullptr;
    /// @brief Callback for close frames, contains the WsServer instance, guid and the status code+reason for closing
    typedef void (*WsServerCloseCallback)(WsServer *server, const Guid &guid, WebSocketStatusCode statusCode, const std::string_view &reason);
    /// @brief Called whenever a close frame is received
    WsServerCloseCallback closeCallback = nullptr;
    /// @brief Callback for data frames, contains the WsServer instance, guid and the received frame
    typedef void (*WsServerReceivedCallback)(WsServer *server, const Guid &guid, const WebSocketFrame &frame);
    /// @brief Called whenever a data frame is received
    WsServerReceivedCallback receivedCallback = nullptr;

private:
    /// @brief The port it is listening on
    int port;
    /// @brief Underlying tcp socket
    TcpListener *listener;
    /// @brief Handle to the accept connections task
    TaskHandle_t acceptConnectionsTask;
};

#endif