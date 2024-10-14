#include <pico/stdlib.h>
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

WsServer::WsServer(int port) : clients(), port(port)
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
    listener->~TcpListener();
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
        server->pongCallback(server, guid, payload, payloadLength);
    }
}

void ws_close(WebSocket *ws, void *args, WebSocketStatusCode statusCode, const std::string_view &reason)
{
    WsServer *server = (WsServer *)args;
    if (server->closeCallback != nullptr)
    {
        const Guid &guid = (*std::find_if(server->clients.begin(), server->clients.end(), [ws](WsServer::ClientEntry *entry)
                                          { return entry->ws == ws; }))
                               ->guid;
        server->closeCallback(server, guid, statusCode, reason);
    }
}

void ws_received(WebSocket *ws, void *args, const WebSocketFrame &frame)
{
    WsServer *server = (WsServer *)args;
    if (server->receivedCallback != nullptr)
    {
        const Guid &guid = (*std::find_if(server->clients.begin(), server->clients.end(), [ws](WsServer::ClientEntry *entry)
                                          { return entry->ws == ws; }))
                               ->guid;
        server->receivedCallback(server, guid, frame);
    }
}

void WsServer::handleRawConnection(TcpClient *client)
{
    uint8_t methodBuf[4];
    if (client->readBytes(methodBuf, 4, 5000) != 4 || methodBuf[0] != 'G' || methodBuf[1] != 'E' || methodBuf[2] != 'T' || methodBuf[3] != ' ')
    {
        client->disconnect();
        client->~TcpClient();
        return;
    }

    TextStream *stream = new TextStream(client, 1024);
    std::string line = stream->readLine(5000);
    auto parts = std::views::split(line, " "sv);
    auto iter = parts.begin();
    std::string path = std::string((*iter).begin(), (*iter).end());

    bool foundConnectionHeader = false;
    bool foundUpgradeHeader = false;
    std::string clientKey;
    std::string protocols;

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
        stream->writeString("HTTP/1.1 400 Bad Request\r\n\r\n"sv);
        stream->~TextStream();
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

        std::string acceptedProtocol = protocolCallback == nullptr ? ""s : protocolCallback(requestedProtocolsVec);

        std::string response = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-Websocket-Accept: "s +
                               handshakeKey +
                               "\r\n"s;

        if (!acceptedProtocol.empty())
        {
            response.append("Sec-Websocket-Protocol: "s + acceptedProtocol + "\r\n"s);
        }

        response.append("\r\n"sv);

        stream->writeString(response);
        stream->~TextStream();

        Guid guid = Guid::NewGuid();
        WebSocket *ws = new WebSocket(client);
        ws->callbackArgs = this; // set args to reference of this instance
        ws->pongCallback = ws_pong;
        ws->closeCallback = ws_close;
        ws->receivedCallback = ws_received;
        ClientEntry *entry = new ClientEntry(guid, ws, path);

        clients.push_back(entry);
        ws->joinMessageLoop();

        clients.erase(std::remove_if(clients.begin(), clients.end(),
                                     [entry](ClientEntry *i)
                                     { return i == entry; }));
        delete entry;
        delete ws;
        return;
    }

    client->disconnect();
    client->~TcpClient();
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
            vTaskDelete(NULL); }, "wsclient", (uint32_t)4096, args, 3, &task);
        }
    }
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

bool WsServer::isListening()
{
    if (listener == nullptr)
        return false;
    return listener->isOpen();
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
    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i]->guid == guid)
        {
            clients[i]->ws->ping();
            return;
        }
    }
}

void WsServer::ping(const Guid &guid, const uint8_t *payload, size_t payloadLength)
{
    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i]->guid == guid)
        {
            clients[i]->ws->ping(payload, payloadLength);
            return;
        }
    }
}

bool WsServer::send(const Guid &guid, std::string_view data, WebSocketMessageType messageType)
{
    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i]->guid == guid)
        {
            return clients[i]->ws->send(data, messageType);
        }
    }

    return false;
}

bool WsServer::send(const Guid &guid, const uint8_t *data, size_t length, WebSocketMessageType messageType)
{
    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i]->guid == guid)
        {
            return clients[i]->ws->send(data, length, messageType);
        }
    }

    return false;
}