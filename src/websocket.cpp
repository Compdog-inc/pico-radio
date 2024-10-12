#include <pico/stdlib.h>
#include <cstring>
#include "websocket.h"

WebSocket::WebSocket(TcpClient *tcp) : tcp(tcp)
{
    sendMutex = xSemaphoreCreateMutex();
}

void WebSocket::close()
{
    // TODO: graceful close
    tcp->disconnect();
    tcp->~TcpClient();
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
        close(); // invalid op code - close socket
        return;
    }
    }

    size_t payloadLength;

    if (header.payloadLen == 126)
    {
        if (tcp->readBytes(&payloadLength, 2, 1000) != 2)
        {
            close();
            return;
        }
    }
    else if (header.payloadLen == 127)
    {
        if (tcp->readBytes(&payloadLength, 8, 1000) != 8)
        {
            close();
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
            close();
            return;
        }
    }

    uint8_t *payload = (uint8_t *)pvPortMalloc(payloadLength);
    if (tcp->readBytes(payload, payloadLength, 5000) != payloadLength)
    {
        vPortFree(payload);
        close();
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
        payload[i] ^= (maskingKey << (i % 4)) & 0xFF;
    }
}

void WebSocket::handleFrame(const WebSocketFrameHeader &header, uint8_t *payload, size_t payloadLength)
{
    if (!header.FIN) // fragmented message (always data frame)
    {
        if (header.opcode != WebSocketOpCode::ContinuationFrame) // first fragment in series
        {
            // dispatch data fragment
            uint8_t *newPayload = (uint8_t *)pvPortMalloc(payloadLength);
            memcpy(newPayload, payload, payloadLength);
            currentFrame = WebSocketFrame(true, header.opcode, newPayload, payloadLength);
        }
        else // subsequent fragments in series
        {
            // dispatch data fragment
            uint8_t *newPayload = (uint8_t *)pvPortMalloc(currentFrame.payloadLength + payloadLength);

            memcpy(newPayload, currentFrame.payload, currentFrame.payloadLength);
            memcpy(newPayload + currentFrame.payloadLength, payload, payloadLength);
            vPortFree(currentFrame.payload);

            currentFrame.payload = newPayload;
            currentFrame.payloadLength += payloadLength;
        }
    }
    else
    {
        switch (header.opcode)
        {
        case WebSocketOpCode::ContinuationFrame: // last fragment in series
        {
            // dispatch data frame
            uint8_t *newPayload = (uint8_t *)pvPortMalloc(currentFrame.payloadLength + payloadLength);

            memcpy(newPayload, currentFrame.payload, currentFrame.payloadLength);
            memcpy(newPayload + currentFrame.payloadLength, payload, payloadLength);
            vPortFree(currentFrame.payload);

            currentFrame.isFragment = false; // no longer a fragment
            currentFrame.payload = newPayload;
            currentFrame.payloadLength += payloadLength;
            break;
        }
        case WebSocketOpCode::Ping:
        case WebSocketOpCode::Pong:
        case WebSocketOpCode::ConnectionClose:
        {
            // dispatch control frame
            break;
        }
        case WebSocketOpCode::TextFrame:
        case WebSocketOpCode::BinaryFrame:
        {
            // dispatch data frame
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
    if (!xSemaphoreTake(sendMutex, 500))
    {
        return false;
    }

    // send ws

    xSemaphoreGive(sendMutex);
    return true;
}