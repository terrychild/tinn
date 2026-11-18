#include "lib/net/tls.h"
#include "lib/mem/memory.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"
#include "lib/mem/reader.h"
#include "lib/crypto/x25519.h"
#include "lib/crypto/sha.h"
#include "lib/crypto/hmac.h"
#include "lib/macros.h"
#include "lib/bytes.h"
#include "lib/log.h"

#include <string.h>
#include "lib/maths/bigint.h"

static Slice hkdfExpandLabel(Memory* mem, const Slice secret, const char* label, const Slice context, Size length) {
    ScratchMemory scratch = scratchMemory(mem);

    Buffer* buf = bufNew(scratch.mem, length + 256);
    bufAppendBE16(buf, length);
    bufAppendBE8(buf, 6+strlen(label));
    bufAppend(buf, "tls13 %s", label);
    bufAppendBE8(buf, context.length);
    bufAppend(buf, context);
    bufAppendBE8(buf, 1);

    Slice key_material = hmacSha384(mem, secret, bufAsSlice(buf));

    scratchRelease(scratch);

    return key_material;
}

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
    OK = 255,
} AlertDescription;

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
    /*AES_128_GCM_SHA256 = 0x1301,*/
    AES_256_GCM_SHA384 = 0x1302/*,
    CHACHA20_POLY1305_SHA256 = 0x1303,
    AES_128_CCM_SHA256 = 0x1304,
    AES_128_CCM_8_SHA256 = 0x1305*/
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
    SECP256R1 = 0x0017,
    X25519 = 0x001D
} NamedGroup;

// TLS stucts
typedef struct {
    ContentType type;
    Version legacy_version;
    U16 length;
    U8 fragment[];
} Record;

typedef struct {
    AlertLevel level;
    AlertDescription description;
} Alert;

typedef struct {
    Slice client_random;
    Slice legacy_session_id;
    CipherSuite cipher_suite;
    Slice server_name;
    Version version;
    //cookies
    SignatureAlgorithm signature_algorithum;
    SignatureAlgorithm certificate_algorithum;
    NamedGroup group;
    BigInt* client_public_key;

    U8 server_random[32];
    BigInt* server_private_key;
    BigInt* server_public_key;
    Slice server_public_key_encoded;
    BigInt* shared_secret;
} Handshake;

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
    Handshake handshake;
} Connection;

// Alerts
static void sendError(Connection* conn, AlertDescription description) {
    Buffer* buf = bufNew(conn->exchange_mem, sizeof(Record) + sizeof(Alert));
    bufAppendBE8(buf, ALERT);
    bufAppendBE16(buf, TLS_1_2);
    bufAppendBE16(buf, sizeof(Alert));

    bufAppendBE8(buf, FATAL);
    bufAppendBE8(buf, description);

    conn->status = CLOSE;
    serverSend(conn->server, bufAsSlice(buf));
}

static void sendClose(Connection* conn) {
    Buffer* buf = bufNew(conn->exchange_mem, sizeof(Record) + sizeof(Alert));
    bufAppendBE8(buf, ALERT);
    bufAppendBE16(buf, TLS_1_2);
    bufAppendBE16(buf, sizeof(Alert));

    bufAppendBE8(buf, WARNING);
    bufAppendBE8(buf, CLOSE_NOTIFY);

    serverSend(conn->server, bufAsSlice(buf));
}

static void decodeAlert(Connection* conn, Slice fragment) {
    Reader* reader = readerFromSlice(conn->exchange_mem, fragment);

    Alert alert;
    alert.level = readBE8(reader);
    alert.description = readBE8(reader);

    WARN("TLS Alert: %d - %d", alert.level, alert.description);

    if (alert.description == CLOSE_NOTIFY) {
        conn->status = CLOSE;
        sendClose(conn);
    }
}

// TLS Handshake
static Slice sendServerHello(Connection* conn) {
    Buffer* buf = bufNew(conn->exchange_mem, KiB(4));

    // record
    bufAppendBE8(buf, HANDSHAKE);
    bufAppendBE16(buf, 0x0303);
    Size record_size_offset = bufReserve(buf, 2);

    // handshake header
    bufAppendBE8(buf, SERVER_HELLO);
    Size handshake_size_offset = bufReserve(buf, 3);

    // server version
    bufAppendBE16(buf, 0x0303);

    // server random
    bufAppend(buf, conn->handshake.server_random, 32);

    // session id
    bufAppendBE8(buf, conn->handshake.legacy_session_id.length);
    bufAppend(buf, conn->handshake.legacy_session_id);

    // cipher suite
    bufAppendBE16(buf, conn->handshake.cipher_suite);

    // legacy compression method
    bufAppendBE8(buf, 0);

    // extensions
    Size extensions_size_offset = bufReserve(buf, 2);

    // supported versions
    bufAppendBE16(buf, SUPPORTED_VERSIONS);
    bufAppendBE16(buf, 2);
    bufAppendBE16(buf, conn->handshake.version);

    // key share
    bufAppendBE16(buf, KEY_SHARE);
    bufAppendBE16(buf, conn->handshake.server_public_key_encoded.length + 4);
    bufAppendBE16(buf, conn->handshake.group);
    bufAppendBE16(buf, conn->handshake.server_public_key_encoded.length);
    bufAppend(buf, conn->handshake.server_public_key_encoded);

    // updates sizes
    bufSetBE16(buf, extensions_size_offset, buf->length - extensions_size_offset - 2);
    bufSetBE24(buf, handshake_size_offset, buf->length - handshake_size_offset - 3);
    bufSetBE16(buf, record_size_offset, buf->length - record_size_offset - 2);

    // send
    serverSend(conn->server, bufAsSlice(buf));
    return slice(buf, 5, -1);
}

static void decodeSupportedVersions(Connection* conn, Reader* reader) {
    U8 length = readBE8(reader);
    for (U8 i=0; i<length; i+=2) {
        Version version = readBE16(reader);
        if (version == TLS_1_3) {
            conn->handshake.version = version;
            DEBUG("TLS version: %04X", version);
            return;
        }
    }
    sendError(conn, PROTOCOL_VERSION);
}

static void decodeCipherSuites(Connection* conn, Reader* reader) {
    while (!readerEOF(reader)) {
        CipherSuite suite = readBE16(reader);
        switch (suite) {
            case AES_256_GCM_SHA384:
                conn->handshake.cipher_suite = suite;
                return;
        }
        DEBUG("unknown cipher suite: %04X", suite);
    }
}

static void decodeServerName(Connection* conn, Reader* reader) {
    if (readBE16(reader) != readerRemaining(reader)) {
        ERROR("TLS Server Name - More than one specified");
        sendError(conn, ILLEGAL_PARAMETER);
        return;
    }

    if (readBE8(reader) != 0) {
        ERROR("TLS Server Name - Invalid type");
        sendError(conn, ILLEGAL_PARAMETER);
        return;
    }

    U16 length = readBE16(reader);
    if (length != readerRemaining(reader)) {
        ERROR("TLS Server Name - Invalid length");
        sendError(conn, DECODE_ERROR);
        return;
    }

    conn->handshake.server_name = readSlice(reader, length);
    DEBUG("TLS Server Name: %.*s", length, conn->handshake.server_name.start);
}

static void decodeCookie(Connection* conn, Reader* reader) {
    DEBUG("TLS cookie length: %d", readBE16(reader));
}

static void decodeSignatureAlgorithms(Connection* conn, bool cert, Reader* reader) {
    U16 length = readBE16(reader);
    for (U16 i=0; i<length; i+=2) {
        SignatureAlgorithm algorithum = readBE16(reader);
        switch (algorithum) {
            case RSA_PKCS1_SHA256:
            case ECDSA_SECP256R1_SHA256:
            case RSA_PSS_RSAE_SHA256:
                if (cert) {
                    conn->handshake.certificate_algorithum = algorithum;
                } else {
                    conn->handshake.signature_algorithum = algorithum;
                }
                return;
        }
        DEBUG("unknown signature algorithum: %04X", algorithum);
    }
}

static void decodeSupportedGroups(Connection* conn, Reader* reader) {
    U16 length = readBE16(reader);
    for (U16 i=0; i<length; i+=2) {
        NamedGroup group = readBE16(reader);
        switch (group) {
            case X25519:
                conn->handshake.group = group;
                return;
        }
        DEBUG("unknown group: %04X", group);
    }
}

static void decodeKeyShare(Connection* conn, Reader* reader) {
    if (readBE16(reader) != readerRemaining(reader)) {
        ERROR("TLS Key Share - Invalid length");
        sendError(conn, DECODE_ERROR);
        return;
    }

    do {
        if (readBE16(reader) == conn->handshake.group) {
            conn->handshake.client_public_key = biFromLESlice(conn->exchange_mem, readSlice(reader, readBE16(reader)));
            DEBUG("Client key");
            hexDump(biToLESlice(conn->exchange_mem, conn->handshake.client_public_key));
            return;
        }
    } while (!readerEOF(reader));
}

static void decodeClientHello(Connection* conn, Slice client_hello, Reader* reader) {
    DEBUG("TLS Handshake - Client Hello");

    // legacy version
    Version legacy_version = readBE16(reader);
    DEBUG("legacy version: %04X", legacy_version);
    if (legacy_version != TLS_1_2) {
        sendError(conn, PROTOCOL_VERSION);
        return;
    }

    // random data
    conn->handshake.client_random = readSlice(reader, 32);
    DEBUG("Client random");
    hexDump(conn->handshake.client_random);

    // legacy session id
    U8 legacy_session_length = readBE8(reader);
    conn->handshake.legacy_session_id = readSlice(reader, legacy_session_length);
    DEBUG("legacy session length: %d", legacy_session_length);
    hexDump(conn->handshake.legacy_session_id);

    // cipher suites
    decodeCipherSuites(conn, readReader(conn->exchange_mem, reader, readBE16(reader)));
    if (conn->handshake.cipher_suite == 0) {
        ERROR("Unable to find a matching cipher suite");
        sendError(conn, HANDSHAKE_FAILURE);
        return;
    }
    DEBUG("selected cipher suite: %04X", conn->handshake.cipher_suite);

    // legacy compression methods
    if (readBE8(reader) != 1 || readBE8(reader) != 0) {
        ERROR("Invalid compression method");
        sendError(conn, ILLEGAL_PARAMETER);
        return;
    }

    // extensions
    Reader* key_share = NULL;

    U16 extensions_length = readBE16(reader);
    if (readerRemaining(reader) != extensions_length) {
        ERROR("Invalid client hello length");
        sendError(conn, DECODE_ERROR);
        return;
    }

    while (!readerEOF(reader)) {
        U16 extension_type = readBE16(reader);
        Reader* extension_reader = readReader(conn->exchange_mem, reader, readBE16(reader));
        switch (extension_type) {
            case SERVER_NAME:
                decodeServerName(conn, extension_reader);
                break;

            case SUPPORTED_VERSIONS:
                decodeSupportedVersions(conn, extension_reader);
                break;

            case COOKIE:
                decodeCookie(conn, extension_reader);
                break;

            case SIGNATURE_ALGORITHMS:
                decodeSignatureAlgorithms(conn, false, extension_reader);
                if (conn->handshake.signature_algorithum == 0) {
                    ERROR("Unable to find a matching signature algorithum");
                    sendError(conn, HANDSHAKE_FAILURE);
                    return;
                }
                DEBUG("selected signature algorithum: %04X", conn->handshake.signature_algorithum);
                break;

            case SIGNATURE_ALGORITHMS_CERT:
                decodeSignatureAlgorithms(conn, true, extension_reader);
                if (conn->handshake.certificate_algorithum == 0) {
                    ERROR("Unable to find a matching certificate algorithum");
                    sendError(conn, HANDSHAKE_FAILURE);
                    return;
                }
                DEBUG("selected certificate algorithum: %04X", conn->handshake.certificate_algorithum);
                break;

            case SUPPORTED_GROUPS:
                decodeSupportedGroups(conn, extension_reader);
                if (conn->handshake.group == 0) {
                    ERROR("Unable to find a matching named group");
                    sendError(conn, HANDSHAKE_FAILURE);
                    return;
                }
                DEBUG("selected group: %04X", conn->handshake.group);
                break;

            case KEY_SHARE:
                key_share = extension_reader;
                break;

            default:
                DEBUG("Unknown extension: %d", extension_type);
        }

        if (conn->status == CLOSE) {
            return;
        }
    }

    // validate extensions
    if (conn->handshake.version != TLS_1_3) {
        ERROR("Missing supported versions extension");
        sendError(conn, PROTOCOL_VERSION);
        return;
    }
    if (conn->handshake.signature_algorithum == 0) {
        ERROR("Missing signature algorithum");
        sendError(conn, MISSING_EXTENSION);
        return;
    }
    if (conn->handshake.certificate_algorithum == 0) {
        conn->handshake.certificate_algorithum = conn->handshake.signature_algorithum;
    }
    if (conn->handshake.group == 0) {
        ERROR("Missing supported groups");
        sendError(conn, MISSING_EXTENSION);
        return;
    }
    if (!key_share) {
        ERROR("Missing key share");
        sendError(conn, MISSING_EXTENSION);
        return;
        // TODO: this should send a HelloRetryRequest insteadd
    }
    decodeKeyShare(conn, key_share);
    if (conn->status == CLOSE) {
        return;
    }
    if (conn->handshake.client_public_key == NULL) {
        ERROR("Unable to find a matching key");
        sendError(conn, MISSING_EXTENSION);
        return;
        // TODO: this should send a HelloRetryRequest insteadd
    }
    DEBUG("Key");
    hexDump(biToLESlice(conn->exchange_mem, conn->handshake.client_public_key));

    // server random
    //TODO:random(conn->handshake.server_random, 32);
    readHexStr(conn->handshake.server_random, 64, "707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f");
    DEBUG("Server random");
    hexDump(sliceNew(conn->handshake.server_random, 32));

    // generate keys //TODO: base it on conn->handshake.group
    //TODO:conn->handshake.server_private_key = x25519PrivateKey(conn->exchange_mem);
    conn->handshake.server_private_key = biFromLESlice(conn->exchange_mem, sliceFromHex(conn->exchange_mem, "909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeaf"));
    conn->handshake.server_public_key = x25519PublicKey(conn->exchange_mem, conn->handshake.server_private_key);
    conn->handshake.server_public_key_encoded = slice(biToLESlice(conn->exchange_mem, conn->handshake.server_public_key), 0, 32);
    conn->handshake.shared_secret = x25519(conn->exchange_mem, conn->handshake.server_private_key, conn->handshake.client_public_key);
    DEBUG("Shared secret");
    hexDump(biToLESlice(conn->exchange_mem, conn->handshake.shared_secret));

    Slice server_hello = sendServerHello(conn);

    // Hello hash
    DEBUG("client hello");
    hexDump(client_hello);
    DEBUG("server hello");
    hexDump(server_hello);
    Slice hello_hash = sha384(conn->exchange_mem, readerFromArray(conn->exchange_mem, (Slice[]){client_hello, server_hello}, 2));
    DEBUG("Hello hash");
    hexDump(hello_hash);

    U8 zero_key[48] = {0};
    Slice early_secret = hmacSha384(conn->exchange_mem, sliceFromHex(conn->exchange_mem, "00"), sliceNew(zero_key, 48));
    DEBUG("early_secret");
    hexDump(early_secret);

    Slice empty_hash = sha384(conn->exchange_mem, sliceFromStr(""));
    DEBUG("empty_hash");
    hexDump(empty_hash);

    Slice derived_secret = hkdfExpandLabel(conn->exchange_mem, early_secret, "derived", empty_hash, 48);
    DEBUG("derived_secret");
    hexDump(derived_secret);

    Slice handshake_secret = hmacSha384(conn->exchange_mem, derived_secret, biToLESlice(conn->exchange_mem, conn->handshake.shared_secret));
    DEBUG("handshake secret");
    hexDump(handshake_secret);
}

static void decodeHandshake(Connection* conn, Slice fragment) {
    Reader* reader = readerFromSlice(conn->exchange_mem, fragment);
    do {
        HandshakeType type = readBE8(reader);
        U32 length = readBE24(reader);
        if (length > readerRemaining(reader)) {
            //TODO: deal with headers that span multiple records
            ERROR("Invlaid handshake message length");
            sendError(conn, DECODE_ERROR);
            return;
        }
        Reader* message_reader = readReader(conn->exchange_mem, reader, length);

        switch (type) {
            case CLIENT_HELLO:
                decodeClientHello(conn, fragment, message_reader);
                return;
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
                sendError(conn, INTERNAL_ERROR);
                return;
            default:
                DEBUG("Unknown TLS handshake type: %d", type);
                sendError(conn, UNEXPECTED_MESSAGE);
                return;
        }
    } while (!readerEOF(reader));
}

// TLS Records
static void decodeRecord(Connection* conn, Slice data) {
    if (data.length < 5) {
        WARN("TLS message is shorter than a complete header, waiting...");
        return;
    }

    Reader* reader = readerFromSlice(conn->exchange_mem, data);
    Record record;
    record.type = readBE8(reader);
    switch (record.type) {
        case ALERT:
        case APPLICATION_DATA:
        case CHANGE_CIPHER_SPEC:
        case HANDSHAKE:
            break;
        default:
            DEBUG("Unknown TLS content type: %d", record.type);
            sendError(conn, UNEXPECTED_MESSAGE);
            return;
    }

    record.legacy_version = readBE16(reader);
    DEBUG("Record legacy version: %04X", record.legacy_version);

    record.length = readBE16(reader);
    if (record.length > (2 << 14)) {
        DEBUG("TLS fragment length exceeds 2^14 bytes.");
        sendError(conn, RECORD_OVERFLOW);
        return;
    }
    if (record.length > readerRemaining(reader)) {
        WARN("TLS message is shorter than fragment length, waiting...");
    }

    Slice fragment = readSlice(reader, record.length);
    switch (record.type) {
        case HANDSHAKE:
            decodeHandshake(conn, fragment);
            return;
        case ALERT:
            decodeAlert(conn, fragment);
            return;
        default:
            ERROR("TLS content type not implemented: %d", record.type);
            sendError(conn, INTERNAL_ERROR);
            return;
    }
}

// Events
static void onReceived(void* context, Slice data) {
    Connection* conn = context;

    if (conn->status == CLOSE) {
        return; // ingnore input.
    }

    decodeRecord(conn, data);
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
    return serverSetListener(server, onConnect, NULL);
}