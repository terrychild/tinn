#ifndef LIB_NET_SERVER_H
#define LIB_NET_SERVER_H

#include "lib/types.h"
#include "lib/sys/polling.h"

// Inbound interface
typedef void (*ServerReceivedFunc)(void* context, Slice data);
typedef void (*ServerSentFunc)(void* context);
typedef void (*ServerDisconnectedFunc)(void* context);

typedef struct {
    ServerReceivedFunc onReceived;
    ServerSentFunc onSent;
    ServerDisconnectedFunc onDisconnected;
    void* context;
} ServerInbound;

void serverClientRecived(ServerInbound interface, Slice data);
void serverClientSent(ServerInbound interface);
void serverClientDisconnected(ServerInbound interface);

// Outbound interface
typedef void (*ServerReadFunc)(void* context, U64 length);
typedef void (*ServerSendFunc)(void* context, Slice message);
typedef void (*ServerDisconnectFunc)(void* context);

typedef struct {
    Memory* mem;
    ServerReadFunc read;
    ServerSendFunc send;
    ServerDisconnectFunc disconnect;
    void* context;
} ServerOutbound;

void serverRead(ServerOutbound interface, U64 length);
void serverSend(ServerOutbound interface, Slice data);
void serverDisconnect(ServerOutbound interface);

// Server interface
typedef ServerInbound (*ServerConnectedFunc)(void* context, ServerOutbound server);
typedef void (*ServerListenFunc)(void* server_context, ServerConnectedFunc onConnect, void* context);

typedef struct {
    ServerListenFunc addListener;
    void* context;
} ServerInterface;

bool serverAddListener(ServerInterface server, ServerConnectedFunc onConnect, void* context);

// New server
ServerInterface serverNew(Memory* mem, Polling* polling, const char* port);

#endif