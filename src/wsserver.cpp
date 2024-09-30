#include <pico/stdlib.h>
#include <string>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <string>
#include <ranges>
#include "wsserver.h"
#include "tcpclient.h"
#include "textstream.h"

using namespace std::literals;

WsServer::ClientEntry::ClientEntry() : guid(), ws(nullptr)
{
}

WsServer::ClientEntry::ClientEntry(Guid guid, WebSocket *ws, std::string requestedPath, SemaphoreHandle_t sendMutex) : guid(guid), ws(ws), requestedPath(requestedPath), sendMutex(sendMutex)
{
}

WsServer::WsServer(int port) : port(port)
{
    listener = nullptr;
    clientCount = 0;
}

WsServer::~WsServer()
{
    for (int i = 0; i < clientCount; i++)
    {
        // close ws
    }

    listener->stop();
    listener->~TcpListener();
}

struct _handleRawConnection_taskargs
{
    WsServer *server;
    TcpClient *client;
};

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
        }
    } while (!line.empty());

    if (!foundConnectionHeader || !foundUpgradeHeader || clientCount == WS_SERVER_MAX_CLIENT_COUNT /* at capacity */)
    {
        stream->writeString("HTTP/1.1 400 Bad Request\r\n\r\n"sv);
        stream->~TextStream();
    }
    else
    {
        stream->writeString("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-Websocket-Accept: "s +
                            "T+6DKKn30U5GpA2CkOU0pXhGflY="s + // accept key
                            "\r\nSec-Websocket-Protocol: "s +
                            "testprotocol"s + // supported protocols
                            "\r\n\r\n"s);
        stream->~TextStream();

        Guid guid = Guid::NewGuid();
        WebSocket *ws = new WebSocket();
        SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
        ClientEntry *entry = new ClientEntry(guid, ws, path, mutex);

        clients[clientCount++] = entry;
        handleClient(entry);
    }

    client->disconnect();
    client->~TcpClient();
}

void WsServer::handleClient(ClientEntry *client)
{
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
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
                        { _handleRawConnection_taskargs *args = (_handleRawConnection_taskargs *)ins;
            args->server->handleRawConnection(args->client);
            vPortFree(args);
            vTaskDelete(NULL); }, "wsclient", configMINIMAL_STACK_SIZE, args, 3, &task);
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
    for (int i = 0; i < clientCount; i++)
    {
        if (clients[i]->guid == guid)
        {
            return true;
        }
    }

    return false;
}

void WsServer::disconnectClient(const Guid &guid)
{
    for (int i = 0; i < clientCount; i++)
    {
        if (clients[i]->guid == guid)
        {
            // close and dispose ws
            return;
        }
    }
}

bool WsServer::send(const Guid &guid, std::string_view data, WebSocketMessageType messageType)
{
    return send(guid, (const uint8_t *)data.data(), data.length(), messageType);
}

bool WsServer::send(const Guid &guid, const uint8_t *data, size_t length, WebSocketMessageType messageType)
{
    for (int i = 0; i < clientCount; i++)
    {
        if (clients[i]->guid == guid)
        {
            if (!xSemaphoreTake(clients[i]->sendMutex, 500))
            {
                return false;
            }

            // send ws

            xSemaphoreGive(clients[i]->sendMutex);
            return true;
        }
    }

    return false;
}