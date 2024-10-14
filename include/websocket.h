#ifndef _WEBSOCKET_H_
#define _WEBSOCKET_H_

#include <stdlib.h>
#include <string>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <vector>
#include "tcpclient.h"

enum class WebSocketMessageType
{
    Text,
    Binary
};

enum class WebSocketOpCode : unsigned int
{
    ContinuationFrame = 0x0,
    TextFrame = 0x1,
    BinaryFrame = 0x2,
    ConnectionClose = 0x8,
    Ping = 0x9,
    Pong = 0xA
};

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

typedef struct WebSocketFrameHeader
{
    WebSocketOpCode opcode : 4;
    unsigned int RSV3 : 1;
    unsigned int RSV2 : 1;
    unsigned int RSV1 : 1;
    unsigned int FIN : 1;

    unsigned int payloadLen : 7;
    unsigned int MASK : 1;
} WebSocketFrameHeader;

typedef struct WebSocketFrame
{
    bool isFragment;
    WebSocketOpCode opcode;
    uint8_t *payload;
    size_t payloadLength;

    WebSocketFrame(bool isFragment, WebSocketOpCode opcode, uint8_t *payload, size_t payloadLength) : isFragment(isFragment), opcode(opcode), payload(payload), payloadLength(payloadLength)
    {
    }

    WebSocketFrame() : WebSocketFrame(false, WebSocketOpCode::ContinuationFrame, nullptr, 0)
    {
    }
} WebSocketFrame;

class WebSocket
{
public:
    WebSocket(TcpClient *tcp);
    WebSocket(std::string_view url);
    WebSocket(std::string_view url, std::vector<std::string> protocols);

    void disconnect();

    void close();
    void close(WebSocketStatusCode statusCode);
    void close(WebSocketStatusCode statusCode, const std::string_view &reason);
    void close(uint16_t statusCode, const std::string_view &reason);

    void ping();
    void ping(const uint8_t *payload, size_t payloadLength);
    void pong();
    void pong(const uint8_t *payload, size_t payloadLength);

    bool isConnected();
    bool isSelfHostedMessageLoop();

    bool send(std::string_view data, WebSocketMessageType messageType = WebSocketMessageType::Text);
    bool send(const uint8_t *data, size_t length, WebSocketMessageType messageType = WebSocketMessageType::Binary);

    void joinMessageLoop();

    typedef void (*WebsocketPongCallback)(WebSocket *ws, void *args, const uint8_t *payload, size_t payloadLength);
    WebsocketPongCallback pongCallback = nullptr;
    typedef void (*WebsocketCloseCallback)(WebSocket *ws, void *args, WebSocketStatusCode statusCode, const std::string_view &reason);
    WebsocketCloseCallback closeCallback = nullptr;
    typedef void (*WebsocketReceivedCallback)(WebSocket *ws, void *args, const WebSocketFrame &frame);
    WebsocketReceivedCallback receivedCallback = nullptr;

    void *callbackArgs = nullptr;

    std::string serverProtocol;

private:
    TcpClient *tcp;
    SemaphoreHandle_t sendMutex;
    TaskHandle_t messageLoopTask;
    bool selfHostedMessageLoop;

    void enterPollLoop();
    void parseFrameHeader(const WebSocketFrameHeader &header);
    void maskPayload(uint8_t *payload, size_t payloadLength, uint32_t maskingKey);
    void handleFrame(const WebSocketFrameHeader &header, uint8_t *payload, size_t payloadLength);

    bool sendFrame(const WebSocketFrameHeader &header, const uint8_t *payload, size_t payloadLength, uint32_t maskingKey);
    bool sendFrame(const WebSocketFrameHeader &header, const uint8_t *payload1, size_t payload1Length, const uint8_t *payload2, size_t payload2Length, uint32_t maskingKey);

    bool initiateHandshake(std::string_view path, std::string_view host, std::vector<std::string> protocols);

    WebSocketFrame currentFrame;
    bool closeFrameSent = false;
    bool useMasking;
};

#endif