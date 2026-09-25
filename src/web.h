#ifndef WEB_H
#define WEB_H

#include "lib/types.h"
#include "lib/sys/polling.h"
#include "lib/net/server.h"

Server* startWebServer(Allocator* allocator, Polling* polling, const char* port);

#endif