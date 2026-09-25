#ifndef WEB_H
#define WEB_H

#include "lib/types.h"
#include "lib/net/sockets.h"
#include "lib/net/server.h"

Server* startWebServer(Allocator* allocator, Sockets* sockets, const char* port);

#endif