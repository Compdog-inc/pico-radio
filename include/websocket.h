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

typedef struct WebSocketFrameHeader
{
    unsigned int FIN : 1;
    unsigned int RSV1 : 1;
    unsigned int RSV2 : 1;
    unsigned int RSV3 : 1;

    WebSocketOpCode opcode : 4;

    unsigned int MASK : 1;

    unsigned int payloadLen : 7;
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

    void close();

    bool isConnected();

    bool send(std::string_view data, WebSocketMessageType messageType = WebSocketMessageType::Text);
    bool send(const uint8_t *data, size_t length, WebSocketMessageType messageType = WebSocketMessageType::Binary);

    void joinMessageLoop();

private:
    TcpClient *tcp;
    SemaphoreHandle_t sendMutex;

    void parseFrameHeader(const WebSocketFrameHeader &header);
    void maskPayload(uint8_t *payload, size_t payloadLength, uint32_t maskingKey);
    void handleFrame(const WebSocketFrameHeader &header, uint8_t *payload, size_t payloadLength);

    WebSocketFrame currentFrame;
};

#endif