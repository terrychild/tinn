#ifndef TINN_TLS_H
#define TINN_TLS_H

#include "lib/sys/polling.h"
#include "lib/net/server.h"

Server* tlsTestServer(Memory* mem, Polling* polling, const char* port);

#endif