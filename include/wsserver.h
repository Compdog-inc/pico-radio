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
#include <queue>

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

    /// @brief Set the HTTP response sent to the client when the request is not a valid WebSocket request
    void setBadRequestResponse(std::string_view response);
    /// @brief Get the HTTP response sent to the client when the request is not a valid WebSocket request
    std::string_view getBadRequestResponse();

    /// @brief Start listening on the target port
    void start();
    /// @brief Stop the server
    void stop();
    /// @brief Starts the dispatch queue. Use when calling websocket functions from unsupported places (like interrupts)
    void startDispatchQueue();

    /// @brief Returns true if the server is listening for new connections
    bool isListening();
    /// @brief Returns true if the dispatch queue is running
    bool isDispatchQueueRunning();
    /// @brief Returns true if a client exists with the specified guid and is connected
    bool isClientConnected(const Guid &guid);
    /// @brief Gracefully disconnects a client with guid
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
    /// @brief Sends a binary message to a client
    /// @param guid The guid of the client
    /// @param data The payload
    /// @param messageType Determines how the binary data is interpreted
    /// @return True on success
    bool send(const Guid &guid, const std::vector<uint8_t> &data, WebSocketMessageType messageType = WebSocketMessageType::Binary);

    /// @brief List of currently connected clients
    std::vector<ClientEntry *>
        clients;

    /// @brief Used internally to handle a client connection
    void handleRawConnection(TcpClient *client);
    /// @brief Used internally to start accepting connections
    void acceptConnections();
    /// @brief Used internally to start the dispatch queue
    void joinDispatchQueue();

    /// @brief Custom args for the WebSocket callbacks, set by the user
    void *callbackArgs = nullptr;

    /// @brief Callback for accepting protocols requested by the client
    typedef std::string_view (*WsServerProtocolCallback)(const std::vector<std::string> &requestedProtocols, void *args);
    /// @brief Called whenever a client requests protocols
    WsServerProtocolCallback protocolCallback = nullptr;

    /// @brief Callback for pong frames, contains the WsServer instance, guid and an optional payload
    typedef void (*WsServerPongCallback)(WsServer *server, const Guid &guid, const uint8_t *payload, size_t payloadLength, void *args);
    /// @brief Called whenever a ping is answered with a pong
    WsServerPongCallback pongCallback = nullptr;
    /// @brief Callback for client connected events, contains the WsServer instance and client entry
    typedef void (*ClientConnectedCallback)(WsServer *server, const ClientEntry *entry, void *args);
    /// @brief Called whenever a client is connected
    EventHandler<ClientConnectedCallback> clientConnected;
    /// @brief Callback for client disconnected events, contains the WsServer instance, guid and the status code+reason for closing
    typedef void (*ClientDisconnectedCallback)(WsServer *server, const Guid &guid, WebSocketStatusCode statusCode, const std::string_view &reason, void *args);
    /// @brief Called whenever a close frame is received or connection is interrupted
    EventHandler<ClientDisconnectedCallback> clientDisconnected;
    /// @brief Callback for data frames, contains the WsServer instance, guid and the received frame
    typedef void (*ClientReceivedCallback)(WsServer *server, const Guid &guid, const WebSocketFrame &frame, void *args);
    /// @brief Called whenever a data frame is received
    EventHandler<ClientReceivedCallback> messageReceived;

private:
    /// @brief The port it is listening on
    int port;
    /// @brief Underlying tcp socket
    TcpListener *listener;
    /// @brief Handle to the accept connections task
    TaskHandle_t acceptConnectionsTask;
    /// @brief Handle to the dispatch queue task
    TaskHandle_t dispatchQueueTask;
    /// @brief True when the dispatch queue is running
    bool dispatchQueueRunning;
    /// @brief The HTTP response sent to the client when the request is not a valid WebSocket request
    std::string badRequestResponse;

    enum class DispatchQueueElementType
    {
        Disconnect,
        Ping,
        PingPayload,
        SendString,
        SendBytes
    };

    struct DispatchQueueElement
    {
        DispatchQueueElementType type;

        struct Disconnect
        {
            Guid guid;
        };

        struct Ping
        {
            Guid guid;
        };

        struct PingPaylod
        {
            Guid guid;
            uint8_t *payload;
            size_t payloadLength;
        };

        struct SendString
        {
            Guid guid;
            std::string_view data;
            WebSocketMessageType messageType;
        };

        struct SendBytes
        {
            Guid guid;
            uint8_t *data;
            size_t length;
            WebSocketMessageType messageType;
        };

        union
        {
            Disconnect disconnect;
            Ping ping;
            PingPaylod pingPayload;
            SendString sendString;
            SendBytes sendBytes;
        };
    };

    std::queue<DispatchQueueElement> dispatchQueue;
};

#endif