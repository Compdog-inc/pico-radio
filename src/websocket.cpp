#include <pico/stdlib.h>
#include "websocket.h"

WebSocket::WebSocket(TcpClient *tcp) : tcp(tcp)
{
}