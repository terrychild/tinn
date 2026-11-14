#include <stdlib.h>

#include "lib/net/tls.h"
#include "lib/mem/memory.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"
#include "lib/macros.h"
#include "lib/bytes.h"
#include "lib/log.h"

// TLS enums
typedef enum:U16 {
    TLS_1_0 = 0x0301,
    TLS_1_1 = 0x0302,
    TLS_1_2 = 0x0303,
    TLS_1_3 = 0x0304,
} Version;

typedef enum:U8 {
	INVALID = 0,
    CHANGE_CIPHER_SPEC = 20,
    ALERT = 21,
    HANDSHAKE = 22,
    APPLICATION_DATA = 23,
} ContentType;

typedef enum:U8 {
    WARNING = 1,
    FATAL = 2,
} AlertLevel;

typedef enum:U8 {
    CLOSE_NOTIFY = 0,
    UNEXPECTED_MESSAGE = 10,
    BAD_RECORD_MAC = 20,
    RECORD_OVERFLOW = 22,
    HANDSHAKE_FAILURE = 40,
    BAD_CERTIFICATE = 42,
    UNSUPPORTED_CERTIFICATE = 43,
    CERTIFICATE_REVOKED = 44,
    CERTIFICATE_EXPIRED = 45,
    CERTIFICATE_UNKNOWN = 46,
    ILLEGAL_PARAMETER = 47,
    UNKNOWN_CA = 48,
    ACCESS_DENIED = 49,
    DECODE_ERROR = 50,
    DECRYPT_ERROR = 51,
    PROTOCOL_VERSION = 70,
    INSUFFICIENT_SECURITY = 71,
    INTERNAL_ERROR = 80,
    INAPPROPRIATE_FALLBACK = 86,
    USER_CANCELED = 90,
    MISSING_EXTENSION = 109,
    UNSUPPORTED_EXTENSION = 110,
    UNRECOGNIZED_NAME = 112,
    BAD_CERTIFICATE_STATUS_RESPONSE = 113,
    UNKNOWN_PSK_IDENTITY = 115,
    CERTIFICATE_REQUIRED = 116,
    GENERAL_ERROR = 117,
    NO_APPLICATION_PROTOCOL = 120,
} AlertDescription;

// TLS stucts
typedef struct {
    U8 type;
    U8 legacy_version[2];
    U8 length[2];
    U8 fragment[];
} Record;

typedef struct {
    U8 level;
    U8 description;
} Alert;

// Connection state
typedef enum {
    START,

    RESTART,
    CLOSE,
} ConnectionStatus;

typedef struct {
    ServerOutbound server;
    Memory* exchange_mem;
    ConnectionStatus status;
} Connection;

// Alerts
static void sendError(Connection* conn, AlertDescription description) {
    Record* message = allocate(conn->exchange_mem, sizeof(Record) + sizeof(Alert));
    message->type = ALERT;
    toBE16(message->legacy_version, TLS_1_2);
    toBE16(message->length, sizeof(Alert));

    Alert* alert = (Alert*)message->fragment;
    alert->level = FATAL;
    alert->description = description;

    conn->status = CLOSE;
    serverSend(conn->server, sliceNew((U8*)message, sizeof(Record) + sizeof(Alert)));
}

static void sendClose(Connection* conn) {
    Record* message = allocate(conn->exchange_mem, sizeof(Record) + sizeof(Alert));
    message->type = ALERT;
    toBE16(message->legacy_version, TLS_1_2);
    toBE16(message->length, sizeof(Alert));

    Alert* alert = (Alert*)message->fragment;
    alert->level = WARNING;
    alert->description = CLOSE_NOTIFY;

    serverSend(conn->server, sliceNew((U8*)message, sizeof(Record) + sizeof(Alert)));
}

static void decodeAlert(Connection* conn, Reader reader) {
    Alert alert;
    alert.level = readBE8(&reader);
    alert.description = readBE8(&reader);

    WARN("TLS Alert: %d - %d", alert.level, alert.description);

    if (alert.description == CLOSE_NOTIFY) {
        conn->status = CLOSE;
        sendClose(conn);
    }
}

// Events
static void onReceived(void* context, Slice data) {
    Connection* conn = context;

    if (conn->status == CLOSE) {
        return; // ingnore input.
    }

    if (data.length < 5) {
        WARN("TLS message is shorter than a complete header, waiting...");
        return;
    }

    Reader reader = sliceReader(data);
    ContentType type = readBE8(&reader);
    switch (type) {
        case ALERT:
        case APPLICATION_DATA:
        case CHANGE_CIPHER_SPEC:
        case HANDSHAKE:
            break;
        default:
            DEBUG("Unknown TLS content type: %d", type);
            sendError(conn, UNEXPECTED_MESSAGE);
            return;
    }

    DEBUG("Record legacy version: %04X", readBE16(&reader));

    U16 length = readBE16(&reader);
    if (length > (2 << 14)) {
        DEBUG("TLS fragment length exceeds 2^14 bytes.");
        sendError(conn, RECORD_OVERFLOW);
        return;
    }
    if (length > readerRemaining(&reader)) {
        WARN("TLS message is shorter than fragment length, waiting...");
    }

    Reader record_reader = readReader(&reader, length);
    switch (type) {
        /*case HANDSHAKE:
            decodeHandshake(conn, record_reader);
            return;*/
        case ALERT:
            decodeAlert(conn, record_reader);
            return;
        default:
            ERROR("TLS content type not implemented: %d", type);
            sendError(conn, INTERNAL_ERROR);
            return;
    }
}

static void onSent(void* context) {
    Connection* conn = context;
    if (conn->status == CLOSE) {
        serverDisconnect(conn->server);
    }
}

static void onDisconnected(void* context) {
    Connection* conn = context;
    memoryRelease(conn->exchange_mem);
}

static ServerInbound onConnect(__attribute__((unused)) void* context, ServerOutbound server) {
    Connection* connection = allocate(server.mem, sizeof(Connection));
    connection->server = server;
    connection->exchange_mem = memoryNew(MiB(100));
    connection->status = START;

    return (ServerInbound) {
        .onReceived = onReceived,
        .onSent = onSent,
        .onDisconnected = onDisconnected,
        .context = connection
    };
}

bool tlsServer(Memory* mem, ServerInterface server) {
    return serverAddListener(server, onConnect, NULL);
}