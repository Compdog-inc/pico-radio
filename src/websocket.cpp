#include <pico/stdlib.h>
#include <cstring>
#include <string>
#include <pico/rand.h>
#include <lwip/ip4_addr.h>
#include <vector>
#include <format>
#include <ranges>
#include "websocket.h"
#include "tcpclient.h"
#include "textstream.h"
#include "guid.h"
#include "sha1.hpp"

using namespace std::literals;

static SemaphoreHandle_t sha1_mutex = NULL;
static SHA1 sha1 = SHA1();

constexpr std::string_view WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"sv;

WebSocket::WebSocket(TcpClient *tcp) : tcp(tcp)
{
    sendMutex = xSemaphoreCreateMutex();
    useMasking = false;
    selfHostedMessageLoop = false;
}

WebSocket::WebSocket(std::string_view url) : WebSocket(url, std::vector<std::string>())
{
}

WebSocket::WebSocket(std::string_view url, std::vector<std::string> protocols)
{
    if (!sha1_mutex)
    {
        sha1_mutex = xSemaphoreCreateMutex();
    }

    size_t host_start = url.find('/') + 2;
    size_t host_end = url.find('/', host_start);
    std::string_view host = url.substr(host_start, host_end - host_start);
    std::string_view path = url.substr(host_end);

    size_t hostSep = host.find(':');
    std::string_view serverStr;
    int port;
    if (hostSep != std::string::npos)
    {
        serverStr = host.substr(0, hostSep);
        std::string_view portStr = host.substr(hostSep + 1);
        std::from_chars(portStr.cbegin(), portStr.cend(), port);
    }
    else
    {
        serverStr = host;
        port = 80;
    }

    ip4_addr_t server;
    ip4addr_aton(std::string(serverStr).c_str(), &server);

    tcp = new TcpClient(server, port);

    if (!tcp->isConnected() || !initiateHandshake(path, host, protocols))
    {
        disconnect();
        return;
    }

    sendMutex = xSemaphoreCreateMutex();
    useMasking = true;

    selfHostedMessageLoop = true;
    xTaskCreate([](void *ins) -> void
                { WebSocket *ws = (WebSocket *)ins;
        ws->enterPollLoop();
        vTaskDelete(NULL); }, "wsmsgs", (uint32_t)4096, this, 3, &messageLoopTask);
}

bool WebSocket::initiateHandshake(std::string_view path, std::string_view host, std::vector<std::string> protocols)
{
    TextStream *stream = new TextStream(tcp, 1024);

    Guid key = Guid::NewGuid();
    std::string keyStr = key.toString();

    std::string req = "GET "s
                          .append(path)
                          .append(" HTTP/1.1\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nHost: "sv)
                          .append(host)
                          .append("\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: ")
                          .append(keyStr);

    if (protocols.size() > 0)
    {
        req.append("\r\nSec-WebSocket-Protocol: "sv);
        for (auto &protocol : protocols)
        {
            req.append(protocol);
            if (protocol != protocols.back())
            {
                req.append(", "sv);
            }
        }
    }

    req.append("\r\n\r\n"sv);

    if (!stream->writeString(req))
    {
        stream->~TextStream();
        return false;
    }

    std::string line = stream->readLine(5000);
    auto parts = std::views::split(line, " "sv);
    auto iter = parts.begin();
    iter++;
    std::string status = std::string((*iter).begin(), (*iter).end());
    if (status != "101"sv)
    {
        stream->~TextStream();
        return false;
    }

    bool foundConnectionHeader = false;
    bool foundUpgradeHeader = false;
    std::string acceptKey;
    std::string acceptedProtocol;

    do
    {
        line = stream->readLine(5000);
        auto sep = line.find(':');
        if (sep != std::string::npos)
        {
            std::string headerName = line.substr(0, sep);

            auto valueStart = line.find(' ', sep);
            if (valueStart == std::string::npos)
                valueStart = sep;

            std::string headerValue = line.substr(valueStart + 1);

            if (headerName == "Connection"sv && headerValue == "Upgrade"sv)
            {
                foundConnectionHeader = true;
            }
            else if (headerName == "Upgrade"sv && headerValue == "websocket"sv)
            {
                foundUpgradeHeader = true;
            }
            else if (headerName == "Sec-WebSocket-Accept"sv)
            {
                acceptKey = headerValue;
            }
            else if (headerName == "Sec-WebSocket-Protocol")
            {
                acceptedProtocol = headerValue;
            }
        }
    } while (!line.empty());

    if (!foundConnectionHeader || !foundUpgradeHeader || acceptKey.empty())
    {
        stream->~TextStream();
        return false;
    }

    xSemaphoreTake(sha1_mutex, 1000);
    sha1.update(keyStr.append(WS_GUID));
    std::string handshakeKey = sha1.final();
    xSemaphoreGive(sha1_mutex);

    if (acceptKey != handshakeKey)
    {
        stream->~TextStream();
        return false;
    }

    serverProtocol = acceptedProtocol;

    stream->~TextStream();
    return true;
}

void WebSocket::disconnect()
{
    tcp->disconnect();
    tcp->~TcpClient();
    tcp = nullptr;
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
    assert(isConnected() == true);
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
    assert(isConnected() == true);
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
    assert(isConnected() == true);
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
    if (tcp == nullptr) // uninitialized state
        return false;
    return tcp->isConnected();
}

bool WebSocket::isSelfHostedMessageLoop()
{
    return selfHostedMessageLoop;
}

void WebSocket::joinMessageLoop()
{
    // can't join a self hosted message loop
    assert(selfHostedMessageLoop == false);
    enterPollLoop();
}

void WebSocket::enterPollLoop()
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
    assert(isConnected() == true);
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
    std::memcpy(&buffer[index], &header, 2);
    index += 2;

    if (header.payloadLen == 257)
    {
        uint64_t len = (uint64_t)payloadLength;
        std::memcpy(&buffer[index], &len, sizeof(len));
        index += sizeof(len);
    }
    else if (header.payloadLen == 256)
    {
        uint16_t len = (uint16_t)payloadLength;
        std::memcpy(&buffer[index], &len, sizeof(len));
        index += sizeof(len);
    }

    if (header.MASK)
    {
        std::memcpy(&buffer[index], &maskingKey, sizeof(maskingKey));
        index += sizeof(maskingKey);
    }

    std::memcpy(&buffer[index], payload, payloadLength);

    if (header.MASK)
    {
        maskPayload(&buffer[index], payloadLength, maskingKey);
    }

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
    std::memcpy(&buffer[index], &header, 2);
    index += 2;

    if (header.payloadLen == 257)
    {
        uint64_t len = (uint64_t)(payload1Length + payload2Length);
        std::memcpy(&buffer[index], &len, sizeof(len));
        index += sizeof(len);
    }
    else if (header.payloadLen == 256)
    {
        uint16_t len = (uint16_t)(payload1Length + payload2Length);
        std::memcpy(&buffer[index], &len, sizeof(len));
        index += sizeof(len);
    }

    if (header.MASK)
    {
        std::memcpy(&buffer[index], &maskingKey, sizeof(maskingKey));
        index += sizeof(maskingKey);
    }

    size_t payloadStart = index;
    std::memcpy(&buffer[index], payload1, payload1Length);
    index += payload1Length;
    std::memcpy(&buffer[index], payload2, payload2Length);
    index += payload2Length;

    if (header.MASK)
    {
        maskPayload(&buffer[payloadStart], payload1Length + payload2Length, maskingKey);
    }

    ssize_t ret = tcp->writeBytes(buffer, frameLength);
    vPortFree(buffer);
    xSemaphoreGive(sendMutex);
    return (size_t)ret == frameLength;
}
