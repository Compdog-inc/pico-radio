#include <pico/stdlib.h>
#include <cstring>
#include <string>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <string>
#include <algorithm>
#include <cctype>
#include <locale>
#include <ranges>
#include <vector>
#include "config.h"
#include "wsserver.h"
#include "tcpclient.h"
#include "textstream.h"
#include "sha1.hpp"

using namespace std::literals;

static SemaphoreHandle_t sha1_mutex = NULL;
static SHA1 sha1 = SHA1();

constexpr std::string_view WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"sv;

WsServer::ClientEntry::ClientEntry() : guid(), ws(nullptr)
{
}

WsServer::ClientEntry::ClientEntry(Guid guid, WebSocket *ws, std::string requestedPath) : guid(guid), ws(ws), requestedPath(requestedPath)
{
}

WsServer::WsServer(int port) : clients(), port(port), dispatchQueueRunning(false), badRequestResponse("HTTP/1.1 400 Bad Request\r\n\r\n"sv), dispatchQueue()
{
    clients.reserve(WS_SERVER_MAX_CLIENT_COUNT);

    listener = nullptr;
    if (!sha1_mutex)
    {
        sha1_mutex = xSemaphoreCreateMutex();
    }
}

WsServer::~WsServer()
{
    for (size_t i = 0; i < clients.size(); i++)
    {
        clients[i]->ws->close();
    }

    listener->stop();
    delete listener;
}

void WsServer::setBadRequestResponse(std::string_view response)
{
    badRequestResponse = response;
}

std::string_view WsServer::getBadRequestResponse()
{
    return badRequestResponse;
}

struct _handleRawConnection_taskargs
{
    WsServer *server;
    TcpClient *client;
};

// From https://stackoverflow.com/a/217605
// trim from start (in place)
inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                    { return !std::isspace(ch); }));
}

// trim from end (in place)
inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                         { return !std::isspace(ch); })
                .base(),
            s.end());
}

void ws_pong(WebSocket *ws, void *args, const uint8_t *payload, size_t payloadLength)
{
    WsServer *server = (WsServer *)args;
    if (server->pongCallback != nullptr)
    {
        const Guid &guid = (*std::find_if(server->clients.begin(), server->clients.end(), [ws](WsServer::ClientEntry *entry)
                                          { return entry->ws == ws; }))
                               ->guid;
        server->pongCallback(server, guid, payload, payloadLength, server->callbackArgs);
    }
}

void ws_close(WebSocket *ws, void *args, WebSocketStatusCode statusCode, const std::string_view &reason)
{
    WsServer *server = (WsServer *)args;
    if (server->clientDisconnected.Count() > 0)
    {
        const Guid &guid = (*std::find_if(server->clients.begin(), server->clients.end(), [ws](WsServer::ClientEntry *entry)
                                          { return entry->ws == ws; }))
                               ->guid;
        for (int i = 0; i < server->clientDisconnected.Count(); i++)
        {
            server->clientDisconnected.Get(i)(server, guid, statusCode, reason, server->callbackArgs);
        }
    }
}

void ws_received(WebSocket *ws, void *args, const WebSocketFrame &frame)
{
    WsServer *server = (WsServer *)args;
    if (server->messageReceived.Count() > 0)
    {
        const Guid &guid = (*std::find_if(server->clients.begin(), server->clients.end(), [ws](WsServer::ClientEntry *entry)
                                          { return entry->ws == ws; }))
                               ->guid;
        for (int i = 0; i < server->messageReceived.Count(); i++)
        {
            server->messageReceived.Get(i)(server, guid, frame, server->callbackArgs);
        }
    }
}

void WsServer::handleRawConnection(TcpClient *client)
{
    uint8_t methodBuf[4];
    if (client->readBytes(methodBuf, 4, WEBSOCKET_TIMEOUT) != 4 || methodBuf[0] != 'G' || methodBuf[1] != 'E' || methodBuf[2] != 'T' || methodBuf[3] != ' ')
    {
        client->disconnect();
        delete client;
        return;
    }

    TextStream *stream = new TextStream(client, 1024);
    std::string line = stream->readLine(WEBSOCKET_TIMEOUT);
    auto parts = std::views::split(line, " "sv);
    auto iter = parts.begin();
    std::string path = std::string((*iter).begin(), (*iter).end());

    bool foundConnectionHeader = false;
    bool foundUpgradeHeader = false;
    std::string clientKey;
    std::string protocols;

    do
    {
        line = stream->readLine(WEBSOCKET_TIMEOUT);
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
            else if (headerName == "Sec-WebSocket-Key"sv)
            {
                clientKey = headerValue;
            }
            else if (headerName == "Sec-WebSocket-Protocol")
            {
                protocols = headerValue;
            }
        }
    } while (!line.empty());

    if (!foundConnectionHeader || !foundUpgradeHeader || clientKey.empty() || clients.size() == WS_SERVER_MAX_CLIENT_COUNT /* at capacity */)
    {
        stream->writeString(badRequestResponse);
        delete stream;
    }
    else
    {
        xSemaphoreTake(sha1_mutex, 1000);
        sha1.update(clientKey.append(WS_GUID));
        std::string handshakeKey = sha1.final();
        xSemaphoreGive(sha1_mutex);

        auto requestedProtocols = std::views::split(protocols, ","sv) | std::views::transform([](auto &&elem)
                                                                                              {
            std::string str(elem.begin(), elem.end());
            ltrim(str);
            rtrim(str);
            return str; });
        std::vector<std::string> requestedProtocolsVec(requestedProtocols.begin(), requestedProtocols.end());

        std::string acceptedProtocol = protocolCallback == nullptr ? ""s : std::string(protocolCallback(requestedProtocolsVec, callbackArgs));

        std::string response = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-Websocket-Accept: "s +
                               handshakeKey +
                               "\r\n"s;

        if (!acceptedProtocol.empty())
        {
            response.append("Sec-Websocket-Protocol: "s + acceptedProtocol + "\r\n"s);
        }

        response.append("\r\n"sv);

        stream->writeString(response);
        delete stream;

        Guid guid = Guid::NewGuid();
        WebSocket *ws = new WebSocket(client);
        ws->callbackArgs = this; // set args to reference of this instance
        ws->serverProtocol = acceptedProtocol;
        ws->pongCallback = ws_pong;
        ws->closeCallback = ws_close;
        ws->receivedCallback = ws_received;
        ClientEntry *entry = new ClientEntry(guid, ws, path);

        clients.push_back(entry);
        if (clientConnected.Count() > 0)
        {
            for (int i = 0; i < clientConnected.Count(); i++)
            {
                clientConnected.Get(i)(this, entry, callbackArgs);
            }
        }

        ws->joinMessageLoop();

        if (!ws->hasGracefullyClosed())
        {
            if (clientDisconnected.Count() > 0)
            {
                for (int i = 0; i < clientDisconnected.Count(); i++)
                {
                    clientDisconnected.Get(i)(this, guid, WebSocketStatusCode::ClosedAbnormally, "Message loop has ungracefully exited."sv, callbackArgs);
                }
            }
        }

        clients.erase(std::remove_if(clients.begin(), clients.end(),
                                     [entry](ClientEntry *i)
                                     { return i == entry; }));
        delete entry;
        delete ws;
        return;
    }

    client->disconnect();
    delete client;
}

void WsServer::acceptConnections()
{
    while (isListening())
    {
        TcpClient *client = listener->acceptClient();

        if (client != nullptr)
        {
            _handleRawConnection_taskargs *args = (_handleRawConnection_taskargs *)pvPortMalloc(sizeof(_handleRawConnection_taskargs));
            args->server = this;
            args->client = client;

            TaskHandle_t task;
            xTaskCreate([](void *ins) -> void
                        { _handleRawConnection_taskargs *targs = (_handleRawConnection_taskargs *)ins;
            targs->server->handleRawConnection(targs->client);
            vPortFree(ins);
            vTaskDelete(NULL); }, "wsclient", (uint32_t)WEBSOCKET_THREAD_STACK_SIZE, args, 3, &task);
        }
    }
}

void WsServer::joinDispatchQueue()
{
    while (isListening() && dispatchQueueRunning)
    {
        while (!dispatchQueue.empty())
        {
            DispatchQueueElement &elem = dispatchQueue.front();
            switch (elem.type)
            {
            case DispatchQueueElementType::Disconnect:
                disconnectClient(elem.disconnect.guid);
                break;
            case DispatchQueueElementType::Ping:
                ping(elem.ping.guid);
                break;
            case DispatchQueueElementType::PingPayload:
                ping(elem.pingPayload.guid, elem.pingPayload.payload, elem.pingPayload.payloadLength);
                if (elem.pingPayload.payload != nullptr)
                {
                    vPortFree(elem.pingPayload.payload);
                }
                break;
            case DispatchQueueElementType::SendString:
                send(elem.sendString.guid, elem.sendString.data, elem.sendString.messageType);
                break;
            case DispatchQueueElementType::SendBytes:
                send(elem.sendBytes.guid, elem.sendBytes.data, elem.sendBytes.length, elem.sendBytes.messageType);
                if (elem.sendBytes.data != nullptr)
                {
                    vPortFree(elem.sendBytes.data);
                }
                break;
            }

            dispatchQueue.pop();
        }

        vTaskDelay(1);
    }

    dispatchQueueRunning = false;
}

void WsServer::start()
{
    assert(isListening() == false);
    listener = new TcpListener(port);

    xTaskCreate([](void *ins) -> void
                { ((WsServer *)ins)->acceptConnections(); vTaskDelete(NULL); },
                "wsserver_task", configMINIMAL_STACK_SIZE, this, 2, &acceptConnectionsTask);
}

void WsServer::stop()
{
    assert(isListening() == true);
    listener->stop();
}

void WsServer::startDispatchQueue()
{
    assert(isListening() == true);
    dispatchQueueRunning = true;
    xTaskCreate([](void *ins) -> void
                { ((WsServer *)ins)->joinDispatchQueue(); vTaskDelete(NULL); },
                "wsserver_dispatch", (uint32_t)WEBSOCKET_THREAD_STACK_SIZE, this, 1, &dispatchQueueTask);
}

bool WsServer::isListening()
{
    if (listener == nullptr)
        return false;
    return listener->isOpen();
}

bool WsServer::isDispatchQueueRunning()
{
    return dispatchQueueRunning;
}

bool WsServer::isClientConnected(const Guid &guid)
{
    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i]->guid == guid)
        {
            return clients[i]->ws->isConnected();
        }
    }

    return false;
}

void WsServer::disconnectClient(const Guid &guid)
{
    if (portCHECK_IF_IN_ISR() && isDispatchQueueRunning())
    {
        DispatchQueueElement elem = {};
        elem.type = DispatchQueueElementType::Disconnect;
        elem.disconnect = {guid};
        dispatchQueue.push(elem);
        return;
    }

    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i]->guid == guid)
        {
            clients[i]->ws->close();
            return;
        }
    }
}

void WsServer::ping(const Guid &guid)
{
    if (portCHECK_IF_IN_ISR() && isDispatchQueueRunning())
    {
        DispatchQueueElement elem = {};
        elem.type = DispatchQueueElementType::Ping;
        elem.ping = {guid};
        dispatchQueue.push(elem);
        return;
    }

    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i]->guid == guid && clients[i]->ws->isConnected())
        {
            clients[i]->ws->ping();
            return;
        }
    }
}

void WsServer::ping(const Guid &guid, const uint8_t *payload, size_t payloadLength)
{
    if (portCHECK_IF_IN_ISR() && isDispatchQueueRunning())
    {
        DispatchQueueElement elem = {};
        elem.type = DispatchQueueElementType::PingPayload;
        uint8_t *newPayload = nullptr;

        if (payload != nullptr && payloadLength > 0)
        {
            newPayload = (uint8_t *)pvPortMalloc(payloadLength);
            std::memcpy(newPayload, payload, payloadLength);
        }

        elem.pingPayload = {guid, newPayload, payloadLength};
        dispatchQueue.push(elem);
        return;
    }

    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i]->guid == guid && clients[i]->ws->isConnected())
        {
            clients[i]->ws->ping(payload, payloadLength);
            return;
        }
    }
}

bool WsServer::send(const Guid &guid, std::string_view data, WebSocketMessageType messageType)
{
    if (portCHECK_IF_IN_ISR() && isDispatchQueueRunning())
    {
        DispatchQueueElement elem = {};
        elem.type = DispatchQueueElementType::SendString;
        elem.sendString = {guid, data, messageType};
        dispatchQueue.push(elem);
        return true;
    }

    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i]->guid == guid && clients[i]->ws->isConnected())
        {
            return clients[i]->ws->send(data, messageType);
        }
    }

    return false;
}

bool WsServer::send(const Guid &guid, const uint8_t *data, size_t length, WebSocketMessageType messageType)
{
    if (portCHECK_IF_IN_ISR() && isDispatchQueueRunning())
    {
        DispatchQueueElement elem = {};
        elem.type = DispatchQueueElementType::SendBytes;
        uint8_t *newPayload = nullptr;

        if (data != nullptr && length > 0)
        {
            newPayload = (uint8_t *)pvPortMalloc(length);
            std::memcpy(newPayload, data, length);
        }

        elem.sendBytes = {guid, newPayload, length, messageType};
        dispatchQueue.push(elem);
        return true;
    }

    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i]->guid == guid && clients[i]->ws->isConnected())
        {
            return clients[i]->ws->send(data, length, messageType);
        }
    }

    return false;
}

bool WsServer::send(const Guid &guid, const std::vector<uint8_t> &data, WebSocketMessageType messageType)
{
    return send(guid, data.data(), data.size(), messageType);
}