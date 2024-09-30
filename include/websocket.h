#ifndef _WEBSOCKET_H_
#define _WEBSOCKET_H_

#include <stdlib.h>
#include <string>
#include <FreeRTOS.h>
#include <task.h>
#include "tcpclient.h"

class WebSocket
{
public:
    WebSocket(TcpClient *tcp);

private:
    TcpClient *tcp;
};

#endif