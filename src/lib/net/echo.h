#ifndef LIB_NET_ECHO_H
#define LIB_NET_ECHO_H

#include "lib/sys/polling.h"
#include "lib/net/server.h"

Server* echoServer(Allocator* allocator, Polling* polling, const char* port);

#endif