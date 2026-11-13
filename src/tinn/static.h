#ifndef TINN_STATIC_H
#define TINN_STATIC_H

#include "lib/net/http.h"

bool staticContent(Memory* exchange_mem, HttpConnection* connection, HttpServerRequest* request, void* context);

#endif