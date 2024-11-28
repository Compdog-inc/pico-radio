#ifndef _WEBSOCKET_H_
#define _WEBSOCKET_H_

#include <stdlib.h>
#include <string>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <vector>
#include "tcpclient.h"

static constexpr size_t WEBSOCKET_MAX_PACKET_SIZE = TCP_MSS;

/// @brief Supported message formats for websocket communication
enum class WebSocketMessageType
{
    /// @brief Text message
    Text,
    /// @brief Any binary payload
    Binary
};

/// @brief Possible dataframe opcodes
enum class WebSocketOpCode : unsigned int
{
    /// @brief This frame is part of a series of fragments
    ContinuationFrame = 0x0,
    /// @brief This is a text message frame
    TextFrame = 0x1,
    /// @brief This is a binary message frame
    BinaryFrame = 0x2,
    /// @brief This is a connection close control frame
    ConnectionClose = 0x8,
    /// @brief This is a ping control frame
    Ping = 0x9,
    /// @brief This is a pong (response) control frame
    Pong = 0xA
};

/// @brief Possible Websocket close status codes
enum class WebSocketStatusCode : uint16_t
{
    NormalClosure = 1000,
    GoingAway = 1001,
    ProctolError = 1002,
    UnsupportedFormat = 1003,
    NoStatus = 1005,
    ClosedAbnormally = 1006,
    UnexpectedData = 1007,
    MessageViolation = 1008,
    MessageTooLong = 1009,
    MissingExtension = 1010,
    UnexpectedCondition = 1011,
    TLSFailed = 1015
};

/// @brief Dataframe header format
typedef struct WebSocketFrameHeader
{
    // opcode (4 bits)
    WebSocketOpCode opcode : 4;
    // extension bits (1x3 bits)
    unsigned int RSV3 : 1;
    unsigned int RSV2 : 1;
    unsigned int RSV1 : 1;
    // is final frame flag (1 bit)
    unsigned int FIN : 1;

    // payload length (0-125, 126, or 127) 7 bits
    unsigned int payloadLen : 7;
    // is masked flag (1 bit)
    unsigned int MASK : 1;
} WebSocketFrameHeader;

/// @brief Websocket Dataframe
typedef struct WebSocketFrame
{
    /// @brief Is this frame a fragment of a larger payload
    bool isFragment;
    /// @brief Opcode of this frame (if this frame is a fragment, the opcode of the first frame)
    WebSocketOpCode opcode;
    /// @brief The payload (if this frame is a fragment, this contains both the previous fragments and this one)
    uint8_t *payload;
    /// @brief The length of the payload stored in `payload`
    size_t payloadLength;

    WebSocketFrame(bool isFragment, WebSocketOpCode opcode, uint8_t *payload, size_t payloadLength) : isFragment(isFragment), opcode(opcode), payload(payload), payloadLength(payloadLength)
    {
    }

    WebSocketFrame() : WebSocketFrame(false, WebSocketOpCode::ContinuationFrame, nullptr, 0)
    {
    }
} WebSocketFrame;

/// @brief A WebSocket implementation
class WebSocket
{
public:
    /// @brief Create a new WebSocket client from an existing connection
    /// @param tcp The TCP socket to use
    /// @warning THIS DOES NOT PERFORM THE HANDSHAKE
    WebSocket(TcpClient *tcp);
    /// @brief Creates a new WebSocket client by connecting to a url
    /// @param url The url to connect to
    /// @note There is no DNS resolving, the url is expected to include the IP address of the server
    WebSocket(std::string_view url);
    /// @brief Creates a new WebSocket client by connecting to a url, and optionally requesting protocols
    /// @param url The url to connect to
    /// @param protocols Requested protocols (to get the accepted protocol use `WebSocket::serverProtocol`)
    /// @note There is no DNS resolving, the url is expected to include the IP address of the server
    WebSocket(std::string_view url, std::vector<std::string> protocols);

    /// @brief Closes and disconnects the socket
    ~WebSocket();

    /// @brief Force disconnect the socket
    void disconnect();

    /// @brief Gracefully close the WebSocket connection by sending a closing control frame
    void close();
    /// @brief Gracefully close the WebSocket connection by sending a closing control frame
    /// @param statusCode The closing status code
    void close(WebSocketStatusCode statusCode);
    /// @brief Gracefully close the WebSocket connection by sending a closing control frame
    /// @param statusCode The closing status code
    /// @param reason A reason to send with the status code
    void close(WebSocketStatusCode statusCode, const std::string_view &reason);
    /// @brief Gracefully close the WebSocket connection by sending a closing control frame
    /// @param statusCode The closing status code
    /// @param reason A reason to send with the status code
    void close(uint16_t statusCode, const std::string_view &reason);

    /// @brief Sends a ping control frame to the server/client
    void ping();
    /// @brief Sends a ping control frame to the server/client with a custom payload
    /// @param payload The payload to send with the ping frame
    /// @param payloadLength The length of the payload
    void ping(const uint8_t *payload, size_t payloadLength);

    /// @brief Returns true if the client is connected
    bool isConnected();
    /// @brief Returns true if the client has gracefully closed the connection
    bool hasGracefullyClosed();
    /// @brief Returns true if the WebSocket client is running the message loop on an internal thread
    bool isSelfHostedMessageLoop();

    /// @brief Send a text message to the server/client
    /// @param data The text data to send
    /// @param messageType Normally set to `WebSocketMessageType::Text`
    /// @return True if succeeded
    bool send(std::string_view data, WebSocketMessageType messageType = WebSocketMessageType::Text);
    /// @brief Send a binary message to the server/client
    /// @param data The binary payload to send
    /// @param length The length of the payload
    /// @param messageType Determines how to interpret the binary message
    /// @return True if succeeded
    bool send(const uint8_t *data, size_t length, WebSocketMessageType messageType = WebSocketMessageType::Binary);
    /// @brief Send a binary message to the server/client
    /// @param data The binary payload to send
    /// @param messageType Determines how to interpret the binary message
    /// @return True if succeeded
    bool send(const std::vector<uint8_t> &data, WebSocketMessageType messageType = WebSocketMessageType::Binary);

    /// @brief Runs the WebSocket message loop on the calling thread, blocking execution until the socket is closed
    void joinMessageLoop();

    /// @brief Callback for pong frames, contains the WebSocket instance, callbackArgs and an optional payload
    typedef void (*WebsocketPongCallback)(WebSocket *ws, void *args, const uint8_t *payload, size_t payloadLength);
    /// @brief Called whenever a ping is answered with a pong
    WebsocketPongCallback pongCallback = nullptr;
    /// @brief Callback for close frames, contains the WebSocket instance, callbackArgs and the status code+reason for closing
    typedef void (*WebsocketCloseCallback)(WebSocket *ws, void *args, WebSocketStatusCode statusCode, const std::string_view &reason);
    /// @brief Called whenever a close frame is received
    WebsocketCloseCallback closeCallback = nullptr;
    /// @brief Callback for data frames, contains the WebSocket instance, callbackArgs and the received frame
    typedef void (*WebsocketReceivedCallback)(WebSocket *ws, void *args, const WebSocketFrame &frame);
    /// @brief Called whenever a data frame is received
    WebsocketReceivedCallback receivedCallback = nullptr;

    /// @brief Custom args for the WebSocket callbacks, set by the user
    void *callbackArgs = nullptr;

    /// @brief The accepted protocol by the server
    std::string serverProtocol;

    /// @brief Returns the connected socket address
    struct sockaddr_in getSocketAddress();

private:
    /// @brief Underlying tcp socket
    TcpClient *tcp;
    /// @brief Mutex to prevent multithreaded socket access
    SemaphoreHandle_t sendMutex;
    /// @brief Handle to self hosted message loop task
    TaskHandle_t messageLoopTask;
    /// @brief True if the message loop task is started
    bool selfHostedMessageLoop;

    /// @brief Enters the message poll loop on the current thread
    void enterPollLoop();
    /// @brief Parses a websocket frame using its header
    /// @param header The frame's header
    void parseFrameHeader(const WebSocketFrameHeader &header);
    /// @brief Masks a payload using a masking key
    /// @param payload The payload to mask
    /// @param payloadLength The length of the payload
    /// @param maskingKey The masking key
    void maskPayload(uint8_t *payload, size_t payloadLength, uint32_t maskingKey);
    /// @brief Handles the different types of frames
    /// @param header The frame header
    /// @param payload The frame payload
    /// @param payloadLength The length of the payload
    void handleFrame(const WebSocketFrameHeader &header, uint8_t *payload, size_t payloadLength);

    /// @brief Sends a ping response frame (only called internally)
    void pong();
    /// @brief Sends a ping response frame with a custom payload (only called internally)
    /// @param payload The pong payload
    /// @param payloadLength The payload length
    void pong(const uint8_t *payload, size_t payloadLength);

    /// @brief Sends a raw websocket frame
    /// @param header The header of the frame
    /// @param payload The payload of the frame
    /// @param payloadLength The length of the payload
    /// @param maskingKey The masking key to mask the payload with
    /// @return True on success
    bool sendFrame(const WebSocketFrameHeader &header, const uint8_t *payload, size_t payloadLength, uint32_t maskingKey);
    /// @brief Sends a raw websocket frame consisting of two separate payloads
    /// @param header The header of the frame
    /// @param payload1 The first payload
    /// @param payload1Length The length of the first payload
    /// @param payload2 The second payload
    /// @param payload2Length The length of the second payload
    /// @param maskingKey The masking key to mask both payloads with
    /// @return True on success
    bool sendFrame(const WebSocketFrameHeader &header, const uint8_t *payload1, size_t payload1Length, const uint8_t *payload2, size_t payload2Length, uint32_t maskingKey);

    /// @brief Initiates a HTTP upgrade request and handshake with a server
    /// @param path The path to request
    /// @param host The host to use in the request headers
    /// @param protocols The procotols to request from the server
    /// @return True on a successful handshake
    bool initiateHandshake(std::string_view path, std::string_view host, std::vector<std::string> protocols);

    /// @brief The current full or partial data frame being received
    WebSocketFrame currentFrame;
    /// @brief True if this client requested a closing of the socket
    bool closeFrameSent = false;
    /// @brief True if this client gracefully closed the connection
    bool gracefullyClosed = false;
    /// @brief True if this client should randomly mask its payloads
    bool useMasking;
};

#endif