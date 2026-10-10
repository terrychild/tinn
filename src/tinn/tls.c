#include <stdlib.h>

#include "tls.h"
#include "lib/mem/allocator.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"
#include "lib/log.h"

// reader help
typedef struct {
    Slice slice;
    U64 read;
} Reader;
Reader newReader(Slice source) {
    return (Reader) {
        .slice = source,
        .read = 0
    };
}

U64 readerRemaining(Reader* reader) {
    return reader->slice.length - reader->read;
}
bool readerEOF(Reader* reader) {
    return reader->read >= reader->slice.length;
}
void skip(Reader* reader, U64 length) {
    reader->read += length;
}

Slice readSlice(Reader* reader, U64 length) {
    Slice rv = slice(reader->slice, reader->read, reader->read + length);
    reader->read += length;
    return rv;
}
static Reader readReader(Reader* reader, U64 length) {
    return newReader(readSlice(reader, length));
}
U8 readU8(Reader* reader) {
    Slice slice = readSlice(reader, 1);
    if (slice.length != 1) {
        return (U8)0;
    }
    return slice.start[0];
}
U16 readU16(Reader* reader) {
    Slice slice = readSlice(reader, 2);
    if (slice.length != 2) {
        return (U16)0;
    }
    return fromBig16(slice.start);
}
U32 readU24(Reader* reader) {
    Slice slice = readSlice(reader, 3);
    if (slice.length != 3) {
        return (U32)0;
    }
    return fromBig24(slice.start);
}
U32 readU32(Reader* reader) {
    Slice slice = readSlice(reader, 4);
    if (slice.length != 4) {
        return (U32)0;
    }
    return fromBig32(slice.start);
}

// TLS types and enums

typedef enum:U8 {
	TLS_CT_INVALID = 0,
    TLS_CT_CHANGE_CIPHER_SPEC = 20,
    TLS_CT_ALERT = 21,
    TLS_CT_HANDSHAKE = 22,
    TLS_CT_APPLICATION_DATA = 23
} ContentType;

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

typedef enum:U16 {
    TLS_ET_SERVER_NAME = 0,
    TLS_ET_SUPPORTED_GROUPS = 10,
    TLS_ET_SIGNATURE_ALGORITHMS = 13,
    TLS_ET_SUPPORTED_VERSIONS = 43,
    TLS_ET_COOKIE = 44,
    TLS_ET_SIGNATURE_ALGORITHMS_CERT = 50,
    TLS_ET_KEY_SHARE = 51
} ExtensionType;

// TLS function declerations
static void readRecord(ServerConnection* connection, Slice data);
static void sendAlert(ServerConnection* connection, AlertLevel level, AlertDescription description);
static void readHandshake(ServerConnection* connection, Reader reader);
static void readHandshakeClientHello(ServerConnection* connection, Reader reader);
static void readExtensionServerName(ServerConnection* connection, Reader reader);
static void readExtensionSupportedVersions(ServerConnection* connection, Reader reader);
static void readExtensionCookie(ServerConnection* connection, Reader reader);
static void readExtensionSupportedGroups(ServerConnection* connection, Reader reader);
static void readExtensionSignatureAlgorithms(ServerConnection* connection, Reader reader);
static void readExtensionKeyShare(ServerConnection* connection, Reader reader);

// the server

Server* tlsTestServer(Allocator* allocator, Polling* polling, const char* port) {
    Server* server = serverNew(allocator, polling, port);
    if (server) {
        server->onReceive = readRecord;
    }
    return server;
}

// the functions

static void readRecord(ServerConnection* connection, Slice data) {
    if (data.length < 5) {
        WARN("TLS message is shorter than a complete header, waiting...");
        return;
    }

    Reader reader = newReader(data);
    ContentType type = readU8(&reader);
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

    DEBUG("Record legacy version: %04X", readU16(&reader));

    U16 length = readU16(&reader);
    if (length > (2 << 14)) {
        DEBUG("TLS fragment length exceeds 2^14 bytes.");
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_RECORD_OVERFLOW);
        return;
    }
    if (length > data.length - 5) {
        WARN("TLS message is shorter than fragment length, waiting...");
        return;
    }

    Reader record_reader = readReader(&reader, length);
    switch (type) {
        case TLS_CT_HANDSHAKE:
            readHandshake(connection, record_reader);
            break;
        default:
            ERROR("TLS content type not implemented: %d", type);
            sendAlert(connection, TLS_AL_FATAL, TLS_AD_INTERNAL_ERROR);
            return;
    }
}

static void sendAlert(ServerConnection* connection, AlertLevel level, AlertDescription description) {
    Alert* alert = allocate(connection->exchange_scope, sizeof(Alert));
    alert->level = level;
    alert->description = description;
    connectionSend(connection, sliceNew((U8*)alert, sizeof(Alert)), level == TLS_AL_FATAL);
}

static void readHandshake(ServerConnection* connection, Reader reader) {
    HandshakeType type = readU8(&reader);
    U32 length = readU24(&reader);
    Reader handshake_reader = readReader(&reader, length);

    switch (type) {
        case TLS_HT_CLIENT_HELLO:
            readHandshakeClientHello(connection, handshake_reader);
            break;
        case TLS_HT_SERVER_HELLO:
        case TLS_HT_NEW_SESSION_TICKET:
        case TLS_HT_END_OF_EARLY_DATA:
        case TLS_HT_ENCRYPTED_EXTENSIONS:
        case TLS_HT_CERTIFICATE:
        case TLS_HT_CERTIFICATE_REQUEST:
        case TLS_HT_CERTIFICATE_VERIFY:
        case TLS_HT_FINISHED:
        case TLS_HT_KEY_UPDATE:
        case TLS_HT_MESSAGE_HASH:
            ERROR("TLS handshake type not implemented: %d", type);
            sendAlert(connection, TLS_AL_FATAL, TLS_AD_INTERNAL_ERROR);
            break;
        default:
            DEBUG("Unknown TLS handshake type: %d", type);
            sendAlert(connection, TLS_AL_FATAL, TLS_AD_UNEXPECTED_MESSAGE);
            return;
    }
}
static void readHandshakeClientHello(ServerConnection* connection, Reader reader) {
    DEBUG("TLS Handshake - Client Hello");

    // legacy version
    U16 legacy_version = readU16(&reader);
    DEBUG("legacy version: %04X", legacy_version);
    if (legacy_version != 0x0303) {
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_PROTOCOL_VERSION);
        return;
    }

    // legacy random data
    //hexDump(readSlice(&reader, 32));
    skip(&reader, 32);

    // legacy session id
    U8 legacy_session_length = readU8(&reader);
    //DEBUG("legacy session length: %d", legacy_session_length);
    //hexDump(readSlice(&reader, legacy_session_length));
    skip(&reader, legacy_session_length);

    // cipher suites
    U16 cipher_suites_length = readU16(&reader);
    DEBUG("cipher suites length: %d", cipher_suites_length);
    hexDump(readSlice(&reader, cipher_suites_length));

    // legacy compression methods
    if (readU8(&reader) != 1 || readU8(&reader) != 0) {
        ERROR("Invalid compression method");
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_ILLEGAL_PARAMETER);
        return;
    }

    // extensions
    U16 extensions_length = readU16(&reader);
    if (readerRemaining(&reader) != extensions_length) {
        ERROR("Invalid client hello length");
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_DECODE_ERROR);
        return;
    }

    while (!readerEOF(&reader)) {
        U16 extension_type = readU16(&reader);
        U16 extension_length = readU16(&reader);
        Reader extension_reader = readReader(&reader, extension_length);
        switch (extension_type) {
            case TLS_ET_SERVER_NAME:
                readExtensionServerName(connection, extension_reader);
                break;
            case TLS_ET_SUPPORTED_VERSIONS:
                readExtensionSupportedVersions(connection, extension_reader);
                break;
            case TLS_ET_COOKIE:
                readExtensionCookie(connection, extension_reader);
                break;
            case TLS_ET_SUPPORTED_GROUPS:
                readExtensionSupportedGroups(connection, extension_reader);
                break;
            case TLS_ET_SIGNATURE_ALGORITHMS:
                readExtensionSignatureAlgorithms(connection, extension_reader);
                break;
            case TLS_ET_SIGNATURE_ALGORITHMS_CERT:
                readExtensionSignatureAlgorithms(connection, extension_reader);
                break;
            case TLS_ET_KEY_SHARE:
                readExtensionKeyShare(connection, extension_reader);
                break;
            default:
                DEBUG("Unknown extension: %d", extension_type);
        }
    }
}

static void readExtensionServerName(ServerConnection* connection, Reader reader) {
    if (readU16(&reader) != reader.slice.length-2) {
        ERROR("TLS Server Name - More than one specified");
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_ILLEGAL_PARAMETER);
        return;
    }

    if (readU8(&reader) != 0) {
        ERROR("TLS Server Name - Invalid type");
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_ILLEGAL_PARAMETER);
        return;
    }

    U16 length = readU16(&reader);
    if (length != reader.slice.length-5) {
        ERROR("TLS Server Name - Invalid length");
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_DECODE_ERROR);
        return;
    }

    Slice name = readSlice(&reader, length);
    DEBUG("TLS Server Name: %.*s", length, name.start);
}

static void readExtensionSupportedVersions(ServerConnection* connection, Reader reader) {
    U8 length = readU8(&reader);
    for (U8 i=0; i<length; i+=2) {
        DEBUG("TLS version: %04X", readU16(&reader));
    }
}

static void readExtensionCookie(ServerConnection* connection, Reader reader) {
    DEBUG("TLS cookie length: %d", readU16(&reader));
}

static void readExtensionSupportedGroups(ServerConnection* connection, Reader reader) {
    DEBUG("TLS supported groups length: %d", readU16(&reader));
}
static void readExtensionSignatureAlgorithms(ServerConnection* connection, Reader reader) {
    DEBUG("TLS signature algorithms length: %d", readU16(&reader));
}
static void readExtensionKeyShare(ServerConnection* connection, Reader reader) {
    DEBUG("TLS key share length: %d", readU16(&reader));
}