#include <stdlib.h>

#include "tls.h"
#include "lib/mem/allocator.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"
#include "lib/log.h"

typedef enum:U8 {
	TLS_CT_INVALID = 0,
    TLS_CT_CHANGE_CIPHER_SPEC = 20,
    TLS_CT_ALERT = 21,
    TLS_CT_HANDSHAKE = 22,
    TLS_CT_APPLICATION_DATA = 23
} ContentType;

typedef enum:U8 {
    TLS_HT_CLIENT_HELLO = 1,
    TLS_HT_SERVER_HELLO = 2,
    TLS_HT_NEW_SESSION_TICKET = 4,
    TLS_HT_END_OF_EARLY_DATA = 5,
    TLS_HT_ENCRYPTED_EXTENSIONS = 8,
    TLS_HT_CERTIFICATE = 11,
    TLS_HT_CERTIFICATE_REQUEST = 13,
    TLS_HT_CERTIFICATE_VERIFY = 15,
    TLS_HT_FINISHED = 20,
    TLS_HT_KEY_UPDATE = 24,
    TLS_HT_MESSAGE_HASH = 254
} HandshakeType;

typedef enum:U8 {
    TLS_AL_WARNING = 1,
    TLS_AL_FATAL = 2
} AlertLevel;

typedef enum:U8 {
    TLS_AD_CLOSE_NOTIFY = 0,
    TLS_AD_UNEXPECTED_MESSAGE = 10,
    TLS_AD_BAD_RECORD_MAC = 20,
    TLS_AD_RECORD_OVERFLOW = 22,
    TLS_AD_HANDSHAKE_FAILURE = 40,
    TLS_AD_BAD_CERTIFICATE = 42,
    TLS_AD_UNSUPPORTED_CERTIFICATE = 43,
    TLS_AD_CERTIFICATE_REVOKED = 44,
    TLS_AD_CERTIFICATE_EXPIRED = 45,
    TLS_AD_CERTIFICATE_UNKNOWN = 46,
    TLS_AD_ILLEGAL_PARAMETER = 47,
    TLS_AD_UNKNOWN_CA = 48,
    TLS_AD_ACCESS_DENIED = 49,
    TLS_AD_DECODE_ERROR = 50,
    TLS_AD_DECRYPT_ERROR = 51,
    TLS_AD_PROTOCOL_VERSION = 70,
    TLS_AD_INSUFFICIENT_SECURITY = 71,
    TLS_AD_INTERNAL_ERROR = 80,
    TLS_AD_INAPPROPRIATE_FALLBACK = 86,
    TLS_AD_USER_CANCELED = 90,
    TLS_AD_MISSING_EXTENSION = 109,
    TLS_AD_UNSUPPORTED_EXTENSION = 110,
    TLS_AD_UNRECOGNIZED_NAME = 112,
    TLS_AD_BAD_CERTIFICATE_STATUS_RESPONSE = 113,
    TLS_AD_UNKNOWN_PSK_IDENTITY = 115,
    TLS_AD_CERTIFICATE_REQUIRED = 116,
    TLS_AD_GENERAL_ERROR = 117,
    TLS_AD_NO_APPLICATION_PROTOCOL = 120,
} AlertDescription;

typedef struct {
    AlertLevel level;
    AlertDescription description;
} Alert;

static void readRecord(ServerConnection* connection, Slice data);

static void sendAlert(ServerConnection* connection, AlertLevel level, AlertDescription description);

static void readHandshake(ServerConnection* connection, Slice data);
static void readHandshakeClientHello(ServerConnection* connection, Slice data);

Server* tlsTestServer(Allocator* allocator, Polling* polling, const char* port) {
    Server* server = serverNew(allocator, polling, port);
    if (server) {
        server->onReceive = readRecord;
    }
    return server;
}

static void readRecord(ServerConnection* connection, Slice data) {
    if (data.length < 5) {
        WARN("TLS message is shorter than a complete header, waiting...");
        return;
    }

    DEBUG("TLS RECORD");
    hexDump(slice(data, 0, 5));
    ContentType type = *data.start;
    U16 length = fromBig16(data.start + 3);

    DEBUG("type: %d", type);
    //LOG("legacy version: %04X", fromBig16(data.start + 1));
    DEBUG("length: %d", length);

    switch (type) {
        case TLS_CT_ALERT:
        case TLS_CT_APPLICATION_DATA:
        case TLS_CT_CHANGE_CIPHER_SPEC:
        case TLS_CT_HANDSHAKE:
            break;
        default:
            DEBUG("Unknown TLS content type: %d", type);
            sendAlert(connection, TLS_AL_FATAL, TLS_AD_UNEXPECTED_MESSAGE);
            return;
    }

    if (length > (2 << 14)) {
        DEBUG("TLS fragment length exceeds 2^14 bytes.");
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_RECORD_OVERFLOW);
        return;
    }
    if (length > data.length - 5) {
        WARN("TLS message is shorter than fragment length, waiting...");
        return;
    }

    switch (type) {
        case TLS_CT_HANDSHAKE:
            readHandshake(connection, slice(data, 5, 5 + length));
            break;
        default:
            ERROR("TLS content type not implemented: %d", type);
            sendAlert(connection, TLS_AL_FATAL, TLS_AD_INTERNAL_ERROR);
    }
}

static void sendAlert(ServerConnection* connection, AlertLevel level, AlertDescription description) {
    Alert* alert = allocate(connection->exchange_scope, sizeof(Alert));
    alert->level = level;
    alert->description = description;
    connectionSend(connection, sliceNew((U8*)alert, sizeof(Alert)), level == TLS_AL_FATAL);
}

static void readHandshake(ServerConnection* connection, Slice data) {
    HandshakeType type = *data.start;
    U32 length = fromBig24(data.start + 1);

    DEBUG("TLS Handshake");
    hexDump(slice(data, 0, 4));
    DEBUG("handshake type: %d", type);
    DEBUG("length: %d", length);

    switch (type) {
        case TLS_HT_CLIENT_HELLO:
            readHandshakeClientHello(connection, data);
            break;
        default:
            sendAlert(connection, TLS_AL_FATAL, TLS_AD_UNEXPECTED_MESSAGE);
    }
}
static void readHandshakeClientHello(ServerConnection* connection, Slice data) {

}