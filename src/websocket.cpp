#include <pico/stdlib.h>
#include <cstring>
#include <string>
#include <pico/rand.h>
#include "websocket.h"

using namespace std::literals;

WebSocket::WebSocket(TcpClient *tcp) : tcp(tcp)
{
    sendMutex = xSemaphoreCreateMutex();
    useMasking = false;
}

void WebSocket::disconnect()
{
    tcp->disconnect();
    tcp->~TcpClient();
}

void WebSocket::close()
{
    close(WebSocketStatusCode::NormalClosure);
}

void WebSocket::close(WebSocketStatusCode statusCode)
{
    close(statusCode, ""sv);
}

void WebSocket::close(WebSocketStatusCode statusCode, const std::string_view &reason)
{
    close((uint16_t)statusCode, reason);
}

void WebSocket::close(uint16_t statusCode, const std::string_view &reason)
{
    if (!closeFrameSent)
    {
        statusCode = htons(statusCode);
        size_t payloadLength = reason.length() + 2 /* 2 byte status code */;
        WebSocketFrameHeader header = {
            WebSocketOpCode::ConnectionClose, // opcode
            0, 0, 0,                          // RSVn
            1,                                // FIN
            payloadLength >= UINT16_MAX ? 127 : payloadLength >= 126 ? 126
                                                                     : payloadLength,
            0};

        sendFrame(header, (uint8_t *)&statusCode, 2, (uint8_t *)reason.data(), reason.length(), 0);
        closeFrameSent = true;
    }
    else
    {
        if (closeCallback != nullptr)
        {
            closeCallback(this, callbackArgs, (WebSocketStatusCode)statusCode, reason);
        }
        disconnect();
    }
}

void WebSocket::ping()
{
    ping(nullptr, 0);
}

void WebSocket::ping(const uint8_t *payload, size_t payloadLength)
{
    WebSocketFrameHeader header = {
        WebSocketOpCode::Ping, // opcode
        0, 0, 0,               // RSVn
        1,                     // FIN
        payloadLength >= UINT16_MAX ? 127 : payloadLength >= 126 ? 126
                                                                 : payloadLength,
        0};

    sendFrame(header, payload, payloadLength, 0);
}

void WebSocket::pong()
{
    pong(nullptr, 0);
}

void WebSocket::pong(const uint8_t *payload, size_t payloadLength)
{
    WebSocketFrameHeader header = {
        WebSocketOpCode::Pong, // opcode
        0, 0, 0,               // RSVn
        1,                     // FIN
        payloadLength >= UINT16_MAX ? 127 : payloadLength >= 126 ? 126
                                                                 : payloadLength,
        0};

    sendFrame(header, payload, payloadLength, 0);
}

bool WebSocket::isConnected()
{
    return tcp->isConnected();
}

void WebSocket::joinMessageLoop()
{
    WebSocketFrameHeader header = {};

    while (isConnected())
    {
        if (tcp->readBytes(&header, 2, 1000) == 2)
        {
            parseFrameHeader(header);
        }
    }
}

void WebSocket::parseFrameHeader(const WebSocketFrameHeader &header)
{
    switch (header.opcode)
    {
        /* Control frames */
    case WebSocketOpCode::ConnectionClose:
    case WebSocketOpCode::Ping:
    case WebSocketOpCode::Pong:
        /* Data frames */
    case WebSocketOpCode::ContinuationFrame:
    case WebSocketOpCode::TextFrame:
    case WebSocketOpCode::BinaryFrame:
    {
        break; // keep parsing
    }
    default:
    {
        disconnect(); // invalid op code - close socket
        return;
    }
    }

    size_t payloadLength;

    if (header.payloadLen == 126)
    {
        if (tcp->readBytes(&payloadLength, 2, 1000) != 2)
        {
            disconnect();
            return;
        }
    }
    else if (header.payloadLen == 127)
    {
        if (tcp->readBytes(&payloadLength, 8, 1000) != 8)
        {
            disconnect();
            return;
        }
    }
    else
    {
        payloadLength = header.payloadLen;
    }

    uint32_t maskingKey = 0;
    if (header.MASK)
    {
        if (tcp->readBytes(&maskingKey, 4, 1000) != 4)
        {
            disconnect();
            return;
        }
    }

    uint8_t *payload = (uint8_t *)pvPortMalloc(payloadLength);
    if ((size_t)tcp->readBytes(payload, payloadLength, 5000) != payloadLength)
    {
        vPortFree(payload);
        disconnect();
        return;
    }

    if (header.MASK)
    {
        maskPayload(payload, payloadLength, maskingKey);
    }

    handleFrame(header, payload, payloadLength);
    vPortFree(payload);
}

void WebSocket::maskPayload(uint8_t *payload, size_t payloadLength, uint32_t maskingKey)
{
    for (size_t i = 0; i < payloadLength; i++)
    {
        payload[i] ^= (maskingKey >> (8 * (i % 4))) & 0xFF;
    }
}

void WebSocket::handleFrame(const WebSocketFrameHeader &header, uint8_t *payload, size_t payloadLength)
{
    if (!header.FIN) // fragmented message (always data frame)
    {
        if (header.opcode != WebSocketOpCode::ContinuationFrame) // first fragment in series
        {
            uint8_t *newPayload = (uint8_t *)pvPortMalloc(payloadLength);
            memcpy(newPayload, payload, payloadLength);
            currentFrame = WebSocketFrame(true, header.opcode, newPayload, payloadLength);
        }
        else // subsequent fragments in series
        {
            uint8_t *newPayload = (uint8_t *)pvPortMalloc(currentFrame.payloadLength + payloadLength);

            memcpy(newPayload, currentFrame.payload, currentFrame.payloadLength);
            memcpy(newPayload + currentFrame.payloadLength, payload, payloadLength);
            vPortFree(currentFrame.payload);

            currentFrame.payload = newPayload;
            currentFrame.payloadLength += payloadLength;
        }

        if (receivedCallback != nullptr)
        {
            receivedCallback(this, callbackArgs, currentFrame);
        }
    }
    else
    {
        switch (header.opcode)
        {
        case WebSocketOpCode::ContinuationFrame: // last fragment in series
        {
            uint8_t *newPayload = (uint8_t *)pvPortMalloc(currentFrame.payloadLength + payloadLength);

            memcpy(newPayload, currentFrame.payload, currentFrame.payloadLength);
            memcpy(newPayload + currentFrame.payloadLength, payload, payloadLength);
            vPortFree(currentFrame.payload);

            currentFrame.isFragment = false; // no longer a fragment
            currentFrame.payload = newPayload;
            currentFrame.payloadLength += payloadLength;

            if (receivedCallback != nullptr)
            {
                receivedCallback(this, callbackArgs, currentFrame);
            }

            vPortFree(newPayload);
            break;
        }
        case WebSocketOpCode::Ping:
        {
            pong(payload, payloadLength);
            break;
        }
        case WebSocketOpCode::Pong:
        {
            if (pongCallback != nullptr)
            {
                pongCallback(this, callbackArgs, payload, payloadLength);
            }
            break;
        }
        case WebSocketOpCode::ConnectionClose:
        {
            if (payloadLength >= 2) // has status code
            {
                uint16_t statusCode = ntohs(*(uint16_t *)payload);
                std::string_view reason = std::string_view((char *)(payload + 2), payloadLength - 2);
                close(statusCode, reason);
                disconnect();
            }
            else
            {
                close();
            }
            break;
        }
        case WebSocketOpCode::TextFrame:
        case WebSocketOpCode::BinaryFrame:
        {
            if (receivedCallback != nullptr)
            {
                currentFrame = WebSocketFrame(false, header.opcode, payload, payloadLength);
                receivedCallback(this, callbackArgs, currentFrame);
            }
            break;
        }
        }
    }
}

bool WebSocket::send(std::string_view data, WebSocketMessageType messageType)
{
    return send((const uint8_t *)data.data(), data.length(), messageType);
}

bool WebSocket::send(const uint8_t *data, size_t length, WebSocketMessageType messageType)
{
    WebSocketOpCode opcode;
    switch (messageType)
    {
    case WebSocketMessageType::Binary:
    {
        opcode = WebSocketOpCode::BinaryFrame;
        break;
    }
    case WebSocketMessageType::Text:
    {
        opcode = WebSocketOpCode::TextFrame;
        break;
    }
    }

    WebSocketFrameHeader header = {
        opcode,  // opcode
        0, 0, 0, // RSVn
        1,       // FIN
        length >= UINT16_MAX ? 127 : length >= 126 ? 126
                                                   : length,
        useMasking ? 1u : 0u};

    return sendFrame(header, data, length, useMasking ? get_rand_32() : 0);
}

bool WebSocket::sendFrame(const WebSocketFrameHeader &header, const uint8_t *payload, size_t payloadLength, uint32_t maskingKey)
{
    if (!xSemaphoreTake(sendMutex, 1000))
    {
        return false;
    }

    size_t frameLength = 2 /* header */ + (header.payloadLen == 257 ? 8 : header.payloadLen == 256 ? 2
                                                                                                   : 0) +
                         (header.MASK ? 4 : 0) /* masking key */ + payloadLength;

    uint8_t *buffer = (uint8_t *)pvPortMalloc(frameLength);
    size_t index = 0;
    *(WebSocketFrameHeader *)(buffer + index) = header;
    index += 2;

    if (header.payloadLen == 257)
    {
        *(uint64_t *)(buffer + index) = (uint64_t)payloadLength;
        index += 8;
    }
    else if (header.payloadLen == 256)
    {
        *(uint16_t *)(buffer + index) = (uint16_t)payloadLength;
        index += 2;
    }

    if (header.MASK)
    {
        *(uint32_t *)(buffer + index) = maskingKey;
        index += 4;
    }

    memcpy(buffer + index, payload, payloadLength);

    ssize_t ret = tcp->writeBytes(buffer, frameLength);
    vPortFree(buffer);
    xSemaphoreGive(sendMutex);
    return (size_t)ret == frameLength;
}

bool WebSocket::sendFrame(const WebSocketFrameHeader &header, const uint8_t *payload1, size_t payload1Length, const uint8_t *payload2, size_t payload2Length, uint32_t maskingKey)
{
    if (!xSemaphoreTake(sendMutex, 500))
    {
        return false;
    }

    size_t frameLength = 2 /* header */ + (header.payloadLen == 257 ? 8 : header.payloadLen == 256 ? 2
                                                                                                   : 0) +
                         (header.MASK ? 4 : 0) /* masking key */ + payload1Length + payload2Length;

    uint8_t *buffer = (uint8_t *)pvPortMalloc(frameLength);
    size_t index = 0;
    *(WebSocketFrameHeader *)(buffer + index) = header;
    index += 2;

    if (header.payloadLen == 257)
    {
        *(uint64_t *)(buffer + index) = (uint64_t)(payload1Length + payload2Length);
        index += 8;
    }
    else if (header.payloadLen == 256)
    {
        *(uint16_t *)(buffer + index) = (uint16_t)(payload1Length + payload2Length);
        index += 2;
    }

    if (header.MASK)
    {
        *(uint32_t *)(buffer + index) = maskingKey;
        index += 4;
    }

    memcpy(buffer + index, payload1, payload1Length);
    index += payload1Length;
    memcpy(buffer + index, payload2, payload2Length);
    index += payload2Length;

    ssize_t ret = tcp->writeBytes(buffer, frameLength);
    vPortFree(buffer);
    xSemaphoreGive(sendMutex);
    return (size_t)ret == frameLength;
}
