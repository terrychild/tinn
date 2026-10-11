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
    TLS_CS_AES_128_GCM_SHA256 = 0x1301,
    TLS_CS_AES_256_GCM_SHA384 = 0x1302,
    TLS_CS_CHACHA20_POLY1305_SHA256 = 0x1303,
    TLS_CS_AES_128_CCM_SHA256 = 0x1304,
    TLS_CS_AES_128_CCM_8_SHA256 = 0x1305,
} CipherSuite;

typedef enum:U16 {
    TLS_ET_SERVER_NAME = 0,
    TLS_ET_SUPPORTED_GROUPS = 10,
    TLS_ET_SIGNATURE_ALGORITHMS = 13,
    TLS_ET_SUPPORTED_VERSIONS = 43,
    TLS_ET_COOKIE = 44,
    TLS_ET_SIGNATURE_ALGORITHMS_CERT = 50,
    TLS_ET_KEY_SHARE = 51
} ExtensionType;

typedef enum:U16 {
    TLS_SA_RSA_PKCS1_SHA256 = 0x0401,
    TLS_SA_ECDSA_SECP256R1_SHA256 = 0x0403,
    TLS_SA_RSA_PSS_RSAE_SHA256 = 0x0804
} SignatureAlgorithm;

// alerts
static void sendAlert(ServerConnection* connection, AlertLevel level, AlertDescription description) {
    Alert* alert = allocate(connection->exchange_scope, sizeof(Alert));
    alert->level = level;
    alert->description = description;
    connectionSend(connection, sliceNew((U8*)alert, sizeof(Alert)), level == TLS_AL_FATAL);
}

// handshake
typedef struct {
    Alert alert;
    Version version;
    Slice server_name;
    CipherSuite cipher_suite;
    SignatureAlgorithm signature_algorithum;
    SignatureAlgorithm certificate_algorithum;
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
    settings->alert.level = TLS_AL_FATAL;
    settings->alert.description = TLS_AD_PROTOCOL_VERSION;
}

static void decodeServerName(Settings* settings, Reader reader) {
    if (readU16(&reader) != readerRemaining(&reader)) {
        ERROR("TLS Server Name - More than one specified");
        settings->alert.level = TLS_AL_FATAL;
        settings->alert.description = TLS_AD_ILLEGAL_PARAMETER;
        return;
    }

    if (readU8(&reader) != 0) {
        ERROR("TLS Server Name - Invalid type");
        settings->alert.level = TLS_AL_FATAL;
        settings->alert.description = TLS_AD_ILLEGAL_PARAMETER;
        return;
    }

    U16 length = readU16(&reader);
    if (length != readerRemaining(&reader)) {
        ERROR("TLS Server Name - Invalid length");
        settings->alert.level = TLS_AL_FATAL;
        settings->alert.description = TLS_AD_DECODE_ERROR;
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
            case TLS_CS_AES_128_GCM_SHA256:
                settings->cipher_suite = suite;
                return;
        }
        DEBUG("unknown cipher suite: %04X", suite);
    }
}

static void decodeSignatureAlgorithms(Settings* settings, bool cert, Reader reader) {
    SignatureAlgorithm length = readU16(&reader);
    for (U16 i=0; i<length; i+=2) {
        SignatureAlgorithm algorithum = readU16(&reader);
        switch (algorithum) {
            case TLS_SA_RSA_PKCS1_SHA256:
            case TLS_SA_ECDSA_SECP256R1_SHA256:
            case TLS_SA_RSA_PSS_RSAE_SHA256:
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
/*A TLS-compliant application MUST support digital signatures with
    rsa_pkcs1_sha256 (for certificates),
    rsa_pss_rsae_sha256 (for CertificateVerify and certificates), and
    ecdsa_secp256r1_sha256.
A TLS-compliant application MUST support key exchange with
    secp256r1 (NIST P-256) and
    SHOULD support key exchange with X25519
*/

static void decodeSupportedGroups(Settings* settings, Reader reader) {
    DEBUG("TLS supported groups length: %d", readU16(&reader));
}
static void decodeKeyShare(Settings* settings, Reader reader) {
    DEBUG("TLS key share length: %d", readU16(&reader));
}

static void decodeClientHello(ServerConnection* connection, Reader reader) {
    DEBUG("TLS Handshake - Client Hello");

    Settings settings = {0};

    // legacy version
    Version legacy_version = readU16(&reader);
    DEBUG("legacy version: %04X", legacy_version);
    if (legacy_version != TLS_1_2) {
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_PROTOCOL_VERSION);
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
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_HANDSHAKE_FAILURE);
        return;
    }
    DEBUG("selected cipher suite: %04X", settings.cipher_suite);

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
        Reader extension_reader = readReader(&reader, readU16(&reader));
        switch (extension_type) {
            case TLS_ET_SERVER_NAME:
                decodeServerName(&settings, extension_reader);
                break;

            case TLS_ET_SUPPORTED_VERSIONS:
                decodeSupportedVersions(&settings, extension_reader);
                break;

            case TLS_ET_COOKIE:
                decodeCookie(&settings, extension_reader);
                break;

            case TLS_ET_SIGNATURE_ALGORITHMS:
                decodeSignatureAlgorithms(&settings, false, extension_reader);
                if (settings.signature_algorithum == 0) {
                    ERROR("Unable to find a matching signature algorithum");
                    sendAlert(connection, TLS_AL_FATAL, TLS_AD_HANDSHAKE_FAILURE);
                    return;
                }
                DEBUG("selected signature algorithum: %04X", settings.signature_algorithum);
                break;

            case TLS_ET_SIGNATURE_ALGORITHMS_CERT:
                decodeSignatureAlgorithms(&settings, true, extension_reader);
                if (settings.certificate_algorithum == 0) {
                    ERROR("Unable to find a matching certificate algorithum");
                    sendAlert(connection, TLS_AL_FATAL, TLS_AD_HANDSHAKE_FAILURE);
                    return;
                }
                DEBUG("selected certificate algorithum: %04X", settings.certificate_algorithum);
                break;

            case TLS_ET_SUPPORTED_GROUPS:
                decodeSupportedGroups(&settings, extension_reader);
                break;

            case TLS_ET_KEY_SHARE:
                decodeKeyShare(&settings, extension_reader);
                break;

            default:
                DEBUG("Unknown extension: %d", extension_type);
        }

        if (settings.alert.level > 0) {
            sendAlert(connection, settings.alert.level, settings.alert.description);
            return;
        }
    }

    if (settings.version != TLS_1_3) {
        ERROR("Missing supported versions extension");
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_PROTOCOL_VERSION);
        return;
    }
    if (settings.signature_algorithum == 0) {
        ERROR("Missing signature algorithum");
        sendAlert(connection, TLS_AL_FATAL, TLS_AD_MISSING_EXTENSION);
        return;
    }
    if (settings.certificate_algorithum == 0) {
        settings.certificate_algorithum = settings.signature_algorithum;
    }
}

static void decodeHandshake(ServerConnection* connection, Reader reader) {
    do {
        HandshakeType type = readU8(&reader);
        U32 length = readU24(&reader);
        if (length > readerRemaining(&reader)) {
            ERROR("Invlaid handshake message length");
            sendAlert(connection, TLS_AL_FATAL, TLS_AD_DECODE_ERROR);
            return;
        }
        Reader handshake_reader = readReader(&reader, length);

        switch (type) {
            case TLS_HT_CLIENT_HELLO:
                decodeClientHello(connection, handshake_reader);
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
    if (length > readerRemaining(&reader)) {
        WARN("TLS message is shorter than fragment length, waiting...");
        return;
    }

    Reader record_reader = readReader(&reader, length);
    switch (type) {
        case TLS_CT_HANDSHAKE:
            decodeHandshake(connection, record_reader);
            break;
        default:
            ERROR("TLS content type not implemented: %d", type);
            sendAlert(connection, TLS_AL_FATAL, TLS_AD_INTERNAL_ERROR);
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