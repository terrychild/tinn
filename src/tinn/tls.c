#include <stdlib.h>

#include "tls.h"
#include "lib/mem/allocator.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"
#include "lib/log.h"

// TLS types and enums
typedef enum:U16 {
    TLS_1_0 = 0x0301,
    TLS_1_1 = 0x0302,
    TLS_1_2 = 0x0303,
    TLS_1_3 = 0x0304
} Version;

typedef enum:U8 {
	INVALID = 0,
    CHANGE_CIPHER_SPEC = 20,
    ALERT = 21,
    HANDSHAKE = 22,
    APPLICATION_DATA = 23
} ContentType;

typedef enum:U8 {
    WARNING = 1,
    FATAL = 2
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

typedef struct {
    AlertLevel level;
    AlertDescription description;
} Alert;

typedef enum:U8 {
    CLIENT_HELLO = 1,
    SERVER_HELLO = 2,
    NEW_SESSION_TICKET = 4,
    END_OF_EARLY_DATA = 5,
    ENCRYPTED_EXTENSIONS = 8,
    CERTIFICATE = 11,
    CERTIFICATE_REQUEST = 13,
    CERTIFICATE_VERIFY = 15,
    FINISHED = 20,
    KEY_UPDATE = 24,
    MESSAGE_HASH = 254
} HandshakeType;

typedef enum:U16 {
    AES_128_GCM_SHA256 = 0x1301,
    AES_256_GCM_SHA384 = 0x1302,
    CHACHA20_POLY1305_SHA256 = 0x1303,
    AES_128_CCM_SHA256 = 0x1304,
    AES_128_CCM_8_SHA256 = 0x1305,
} CipherSuite;

typedef enum:U16 {
    SERVER_NAME = 0,
    SUPPORTED_GROUPS = 10,
    SIGNATURE_ALGORITHMS = 13,
    SUPPORTED_VERSIONS = 43,
    COOKIE = 44,
    SIGNATURE_ALGORITHMS_CERT = 50,
    KEY_SHARE = 51
} ExtensionType;

typedef enum:U16 {
    RSA_PKCS1_SHA256 = 0x0401,
    ECDSA_SECP256R1_SHA256 = 0x0403,
    RSA_PSS_RSAE_SHA256 = 0x0804
} SignatureAlgorithm;

typedef enum:U16 {
    SECP256R1 = 0x0017
} NamedGroup;

// alerts
static void sendAlert(ServerConnection* connection, AlertLevel level, AlertDescription description) {
    Alert* alert = allocate(connection->exchange_scope, sizeof(Alert));
    alert->level = level;
    alert->description = description;
    connectionSend(connection, sliceNew((U8*)alert, sizeof(Alert)), level == FATAL);
}

// handshake
typedef struct {
    Alert alert;
    Version version;
    Slice server_name;
    CipherSuite cipher_suite;
    SignatureAlgorithm signature_algorithum;
    SignatureAlgorithm certificate_algorithum;
    NamedGroup group;
    Slice key;
    Slice random;
} Settings;

static void decodeSupportedVersions(Settings* settings, Reader reader) {
    U8 length = readU8(&reader);
    for (U8 i=0; i<length; i+=2) {
        Version version = readU16(&reader);
        if (version == TLS_1_3) {
            settings->version = version;
            DEBUG("TLS version: %04X", version);
            return;
        }
    }
    settings->alert.level = FATAL;
    settings->alert.description = PROTOCOL_VERSION;
}

static void decodeServerName(Settings* settings, Reader reader) {
    if (readU16(&reader) != readerRemaining(&reader)) {
        ERROR("TLS Server Name - More than one specified");
        settings->alert.level = FATAL;
        settings->alert.description = ILLEGAL_PARAMETER;
        return;
    }

    if (readU8(&reader) != 0) {
        ERROR("TLS Server Name - Invalid type");
        settings->alert.level = FATAL;
        settings->alert.description = ILLEGAL_PARAMETER;
        return;
    }

    U16 length = readU16(&reader);
    if (length != readerRemaining(&reader)) {
        ERROR("TLS Server Name - Invalid length");
        settings->alert.level = FATAL;
        settings->alert.description = DECODE_ERROR;
        return;
    }

    settings->server_name = readSlice(&reader, length);
    DEBUG("TLS Server Name: %.*s", length, settings->server_name.start);
}

static void decodeCookie(Settings* settings, Reader reader) {
    DEBUG("TLS cookie length: %d", readU16(&reader));
}

static void decodeCipherSuites(Settings* settings, Reader reader) {
    while (!readerEOF(&reader)) {
        CipherSuite suite = readU16(&reader);
        switch (suite) {
            case AES_128_GCM_SHA256:
                settings->cipher_suite = suite;
                return;
        }
        DEBUG("unknown cipher suite: %04X", suite);
    }
}

static void decodeSignatureAlgorithms(Settings* settings, bool cert, Reader reader) {
    U16 length = readU16(&reader);
    for (U16 i=0; i<length; i+=2) {
        SignatureAlgorithm algorithum = readU16(&reader);
        switch (algorithum) {
            case RSA_PKCS1_SHA256:
            case ECDSA_SECP256R1_SHA256:
            case RSA_PSS_RSAE_SHA256:
                if (cert) {
                    settings->certificate_algorithum = algorithum;
                } else {
                    settings->signature_algorithum = algorithum;
                }
                return;
        }
        DEBUG("unknown signature algorithum: %04X", algorithum);
    }
}

static void decodeSupportedGroups(Settings* settings, Reader reader) {
    U16 length = readU16(&reader);
    for (U16 i=0; i<length; i+=2) {
        NamedGroup group = readU16(&reader);
        switch (group) {
            case SECP256R1:
                settings->group = group;
                return;
        }
        DEBUG("unknown group: %04X", group);
    }
}

static void decodeKeyShare(Settings* settings, Reader reader) {
    if (readU16(&reader) != readerRemaining(&reader)) {
        ERROR("TLS Key Share - Invalid length");
        settings->alert.level = FATAL;
        settings->alert.description = DECODE_ERROR;
        return;
    }

    do {
        if (readU16(&reader) == settings->group) {
            settings->key = readSlice(&reader, readU16(&reader));
            return;
        }
    } while (!readerEOF(&reader));
}

static void decodeClientHello(ServerConnection* connection, Reader reader) {
    DEBUG("TLS Handshake - Client Hello");

    Settings settings = {0};
    Reader key_share = {0};

    // legacy version
    Version legacy_version = readU16(&reader);
    DEBUG("legacy version: %04X", legacy_version);
    if (legacy_version != TLS_1_2) {
        sendAlert(connection, FATAL, PROTOCOL_VERSION);
        return;
    }

    // random data
    settings.random = readSlice(&reader, 32);
    DEBUG("Random");
    hexDump(settings.random);

    // legacy session id
    U8 legacy_session_length = readU8(&reader);
    //DEBUG("legacy session length: %d", legacy_session_length);
    //hexDump(readSlice(reader, legacy_session_length));
    readSkip(&reader, legacy_session_length);

    // cipher suites
    decodeCipherSuites(&settings, readReader(&reader, readU16(&reader)));
    if (settings.cipher_suite == 0) {
        ERROR("Unable to find a matching cipher suite");
        sendAlert(connection, FATAL, HANDSHAKE_FAILURE);
        return;
    }
    DEBUG("selected cipher suite: %04X", settings.cipher_suite);

    // legacy compression methods
    if (readU8(&reader) != 1 || readU8(&reader) != 0) {
        ERROR("Invalid compression method");
        sendAlert(connection, FATAL, ILLEGAL_PARAMETER);
        return;
    }

    // extensions
    U16 extensions_length = readU16(&reader);
    if (readerRemaining(&reader) != extensions_length) {
        ERROR("Invalid client hello length");
        sendAlert(connection, FATAL, DECODE_ERROR);
        return;
    }

    while (!readerEOF(&reader)) {
        U16 extension_type = readU16(&reader);
        Reader extension_reader = readReader(&reader, readU16(&reader));
        switch (extension_type) {
            case SERVER_NAME:
                decodeServerName(&settings, extension_reader);
                break;

            case SUPPORTED_VERSIONS:
                decodeSupportedVersions(&settings, extension_reader);
                break;

            case COOKIE:
                decodeCookie(&settings, extension_reader);
                break;

            case SIGNATURE_ALGORITHMS:
                decodeSignatureAlgorithms(&settings, false, extension_reader);
                if (settings.signature_algorithum == 0) {
                    ERROR("Unable to find a matching signature algorithum");
                    sendAlert(connection, FATAL, HANDSHAKE_FAILURE);
                    return;
                }
                DEBUG("selected signature algorithum: %04X", settings.signature_algorithum);
                break;

            case SIGNATURE_ALGORITHMS_CERT:
                decodeSignatureAlgorithms(&settings, true, extension_reader);
                if (settings.certificate_algorithum == 0) {
                    ERROR("Unable to find a matching certificate algorithum");
                    sendAlert(connection, FATAL, HANDSHAKE_FAILURE);
                    return;
                }
                DEBUG("selected certificate algorithum: %04X", settings.certificate_algorithum);
                break;

            case SUPPORTED_GROUPS:
                decodeSupportedGroups(&settings, extension_reader);
                if (settings.group == 0) {
                    ERROR("Unable to find a matching named group");
                    sendAlert(connection, FATAL, HANDSHAKE_FAILURE);
                    return;
                }
                DEBUG("selected group: %04X", settings.group);
                break;

            case KEY_SHARE:
                key_share = extension_reader;
                break;

            default:
                DEBUG("Unknown extension: %d", extension_type);
        }

        if (settings.alert.level > 0) {
            sendAlert(connection, settings.alert.level, settings.alert.description);
            return;
        }
    }

    // validate extensions
    if (settings.version != TLS_1_3) {
        ERROR("Missing supported versions extension");
        sendAlert(connection, FATAL, PROTOCOL_VERSION);
        return;
    }
    if (settings.signature_algorithum == 0) {
        ERROR("Missing signature algorithum");
        sendAlert(connection, FATAL, MISSING_EXTENSION);
        return;
    }
    if (settings.certificate_algorithum == 0) {
        settings.certificate_algorithum = settings.signature_algorithum;
    }
    if (settings.group == 0) {
        ERROR("Missing supported groups");
        sendAlert(connection, FATAL, MISSING_EXTENSION);
        return;
    }
    if (key_share.slice.length == 0) {
        ERROR("Missing key share");
        sendAlert(connection, FATAL, MISSING_EXTENSION);
        // TODO: this should send a HelloRetryRequest insteadd
        return;
    }
    decodeKeyShare(&settings, key_share);
    if (settings.key.length == 0) {
        ERROR("Unable to find a matching key");
        sendAlert(connection, FATAL, MISSING_EXTENSION);
        // TODO: this should send a HelloRetryRequest insteadd
        return;
    }
    DEBUG("Key");
    hexDump(settings.key);
}

static void decodeHandshake(ServerConnection* connection, Reader reader) {
    do {
        HandshakeType type = readU8(&reader);
        U32 length = readU24(&reader);
        if (length > readerRemaining(&reader)) {
            ERROR("Invlaid handshake message length");
            sendAlert(connection, FATAL, DECODE_ERROR);
            return;
        }
        Reader handshake_reader = readReader(&reader, length);

        switch (type) {
            case CLIENT_HELLO:
                decodeClientHello(connection, handshake_reader);
                break;
            case SERVER_HELLO:
            case NEW_SESSION_TICKET:
            case END_OF_EARLY_DATA:
            case ENCRYPTED_EXTENSIONS:
            case CERTIFICATE:
            case CERTIFICATE_REQUEST:
            case CERTIFICATE_VERIFY:
            case FINISHED:
            case KEY_UPDATE:
            case MESSAGE_HASH:
                ERROR("TLS handshake type not implemented: %d", type);
                sendAlert(connection, FATAL, INTERNAL_ERROR);
                break;
            default:
                DEBUG("Unknown TLS handshake type: %d", type);
                sendAlert(connection, FATAL, UNEXPECTED_MESSAGE);
                return;
        }
    } while (!readerEOF(&reader));
}

// record processing
static void decodeRecord(ServerConnection* connection, Slice data) {
    if (data.length < 5) {
        WARN("TLS message is shorter than a complete header, waiting...");
        return;
    }

    Reader reader = sliceReader(data);
    ContentType type = readU8(&reader);
    switch (type) {
        case ALERT:
        case APPLICATION_DATA:
        case CHANGE_CIPHER_SPEC:
        case HANDSHAKE:
            break;
        default:
            DEBUG("Unknown TLS content type: %d", type);
            sendAlert(connection, FATAL, UNEXPECTED_MESSAGE);
            return;
    }

    DEBUG("Record legacy version: %04X", readU16(&reader));

    U16 length = readU16(&reader);
    if (length > (2 << 14)) {
        DEBUG("TLS fragment length exceeds 2^14 bytes.");
        sendAlert(connection, FATAL, RECORD_OVERFLOW);
        return;
    }
    if (length > readerRemaining(&reader)) {
        WARN("TLS message is shorter than fragment length, waiting...");
        return;
    }

    Reader record_reader = readReader(&reader, length);
    switch (type) {
        case HANDSHAKE:
            decodeHandshake(connection, record_reader);
            break;
        default:
            ERROR("TLS content type not implemented: %d", type);
            sendAlert(connection, FATAL, INTERNAL_ERROR);
            return;
    }
}

// the server
Server* tlsTestServer(Allocator* allocator, Polling* polling, const char* port) {
    Server* server = serverNew(allocator, polling, port);
    if (server) {
        server->onReceive = decodeRecord;
    }
    return server;
}