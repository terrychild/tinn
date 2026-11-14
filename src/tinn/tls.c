#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tls.h"
#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/mem/buffer.h"
#include "lib/macros.h"
#include "lib/bytes.h"
#include "lib/log.h"
#include "lib/maths/bigint.h"
#include "lib/crypto/x25519.h"

#include "test/expect.h"

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

typedef struct {
    U8 type;
    U8 legacy_version[2];
    U8 length[2];
    U8 fragment[];
} Record;

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

// alerts
static Slice sendAlert(Memory* mem, AlertLevel level, AlertDescription description) {
    Record* message = allocate(mem, sizeof(Record) + sizeof(Alert));
    message->type = ALERT;
    toBE16(message->legacy_version, 0x0303);
    toBE16(message->length, sizeof(Alert));

    Alert* alert = (Alert*)message->fragment;
    alert->level = level;
    alert->description = description;

    //connectionSend(connection, sliceNew((U8*)message, sizeof(Record) + sizeof(Alert)), level == FATAL);
    return sliceNew((U8*)message, sizeof(Record) + sizeof(Alert));
}

static Slice decodeAlert(Memory* mem, Reader reader) {
    Alert alert;
    alert.level = readBE8(&reader);
    alert.description = readBE8(&reader);

    WARN("TLS Alert: %d - %d", alert.level, alert.description);

    return sliceEmpty();
}

// handshake
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

    U8 server_random[32];

    Alert alert;
    Slice key;
} Settings;

static void random(U8* buffer, U64 len) {
	FILE* fd = fopen("/dev/urandom", "r");
	if (fd == NULL) {
		PANIC("Unable to open /dev/urandom");
	}

	U64 read = fread(buffer, 1, len, fd);
    while(read < len) {
        read += fread(buffer + read, 1, len - read, fd);
    }

	fclose(fd);
}

static Slice sendServerHello(Memory* mem, Settings* settings) {
    Buffer* buf = bufNew(mem, KiB(4));

    // record
    bufAppendBE8(buf, HANDSHAKE);
    bufAppendBE16(buf, 0x0303);
    U64 record_size_offset = bufReserve(buf, 2);

    // handshake header
    bufAppendBE8(buf, SERVER_HELLO);
    U64 handshake_size_offset = bufReserve(buf, 3);

    // server version
    bufAppendBE16(buf, 0x0303);

    // server random
    //TODO:random(settings->server_random, 32);
    //bufAppend(buf, settings->server_random, 32);
    bufAppendHex(buf, "707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f");
    DEBUG("Server random");
    hexDump(sliceNew(settings->server_random, 32));

    // session id
    bufAppendBE8(buf, settings->legacy_session_id.length);
    bufAppend(buf, settings->legacy_session_id);

    // cipher suite
    bufAppendBE16(buf, settings->cipher_suite);

    // legacy compression method
    bufAppendBE8(buf, 0);

    // extensions
    U64 extensions_size_offset = bufReserve(buf, 2);

    // supported versions
    bufAppendBE16(buf, SUPPORTED_VERSIONS);
    bufAppendBE16(buf, 2);
    bufAppendBE16(buf, settings->version);

    // key share
    bufAppendBE16(buf, KEY_SHARE);
    bufAppendBE16(buf, 36); //TODO: calculate
    bufAppendBE16(buf, X25519);
    bufAppendBE16(buf, 32);
    BigInt* key = x25519PublicKey(mem, biFromLESlice(mem, sliceFromHex(mem, "909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeaf"))); //TODO: make random
    bufAppend(buf, slice(biToLESlice(mem, key), 0, 32));

    // updates sizes
    bufSetBE16(buf, extensions_size_offset, buf->length - extensions_size_offset - 2);
    bufSetBE24(buf, handshake_size_offset, buf->length - handshake_size_offset - 3);
    bufSetBE16(buf, record_size_offset, buf->length - record_size_offset - 2);

    // send
    return bufAsSlice(buf);
    //connectionSend(connection, bufAsSlice(buf), false);
}

static void decodeSupportedVersions(Settings* settings, Reader reader) {
    U8 length = readBE8(&reader);
    for (U8 i=0; i<length; i+=2) {
        Version version = readBE16(&reader);
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
    if (readBE16(&reader) != readerRemaining(&reader)) {
        ERROR("TLS Server Name - More than one specified");
        settings->alert.level = FATAL;
        settings->alert.description = ILLEGAL_PARAMETER;
        return;
    }

    if (readBE8(&reader) != 0) {
        ERROR("TLS Server Name - Invalid type");
        settings->alert.level = FATAL;
        settings->alert.description = ILLEGAL_PARAMETER;
        return;
    }

    U16 length = readBE16(&reader);
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
    DEBUG("TLS cookie length: %d", readBE16(&reader));
}

static void decodeCipherSuites(Settings* settings, Reader reader) {
    while (!readerEOF(&reader)) {
        CipherSuite suite = readBE16(&reader);
        switch (suite) {
            case AES_256_GCM_SHA384:
                settings->cipher_suite = suite;
                return;
        }
        DEBUG("unknown cipher suite: %04X", suite);
    }
}

static void decodeSignatureAlgorithms(Settings* settings, bool cert, Reader reader) {
    U16 length = readBE16(&reader);
    for (U16 i=0; i<length; i+=2) {
        SignatureAlgorithm algorithum = readBE16(&reader);
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
    U16 length = readBE16(&reader);
    for (U16 i=0; i<length; i+=2) {
        NamedGroup group = readBE16(&reader);
        switch (group) {
            case X25519:
                settings->group = group;
                return;
        }
        DEBUG("unknown group: %04X", group);
    }
}

static void decodeKeyShare(Settings* settings, Reader reader) {
    if (readBE16(&reader) != readerRemaining(&reader)) {
        ERROR("TLS Key Share - Invalid length");
        settings->alert.level = FATAL;
        settings->alert.description = DECODE_ERROR;
        return;
    }

    do {
        if (readBE16(&reader) == settings->group) {
            settings->key = readSlice(&reader, readBE16(&reader));
            return;
        }
    } while (!readerEOF(&reader));
}

static Slice decodeClientHello(Memory* mem, Reader reader) {
    DEBUG("TLS Handshake - Client Hello");

    Settings settings = {0};
    Reader key_share = {0};

    // legacy version
    Version legacy_version = readBE16(&reader);
    DEBUG("legacy version: %04X", legacy_version);
    if (legacy_version != TLS_1_2) {
        return sendAlert(mem, FATAL, PROTOCOL_VERSION);
    }

    // random data
    settings.client_random = readSlice(&reader, 32);
    DEBUG("Client random");
    hexDump(settings.client_random);

    // legacy session id
    U8 legacy_session_length = readBE8(&reader);
    settings.legacy_session_id = readSlice(&reader, legacy_session_length);
    DEBUG("legacy session length: %d", legacy_session_length);
    hexDump(settings.legacy_session_id);

    // cipher suites
    decodeCipherSuites(&settings, readReader(&reader, readBE16(&reader)));
    if (settings.cipher_suite == 0) {
        ERROR("Unable to find a matching cipher suite");
        return sendAlert(mem, FATAL, HANDSHAKE_FAILURE);
    }
    DEBUG("selected cipher suite: %04X", settings.cipher_suite);

    // legacy compression methods
    if (readBE8(&reader) != 1 || readBE8(&reader) != 0) {
        ERROR("Invalid compression method");
        return sendAlert(mem, FATAL, ILLEGAL_PARAMETER);
    }

    // extensions
    U16 extensions_length = readBE16(&reader);
    if (readerRemaining(&reader) != extensions_length) {
        ERROR("Invalid client hello length");
        return sendAlert(mem, FATAL, DECODE_ERROR);

    }

    while (!readerEOF(&reader)) {
        U16 extension_type = readBE16(&reader);
        Reader extension_reader = readReader(&reader, readBE16(&reader));
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
                    return sendAlert(mem, FATAL, HANDSHAKE_FAILURE);
                }
                DEBUG("selected signature algorithum: %04X", settings.signature_algorithum);
                break;

            case SIGNATURE_ALGORITHMS_CERT:
                decodeSignatureAlgorithms(&settings, true, extension_reader);
                if (settings.certificate_algorithum == 0) {
                    ERROR("Unable to find a matching certificate algorithum");
                    return sendAlert(mem, FATAL, HANDSHAKE_FAILURE);
                }
                DEBUG("selected certificate algorithum: %04X", settings.certificate_algorithum);
                break;

            case SUPPORTED_GROUPS:
                decodeSupportedGroups(&settings, extension_reader);
                if (settings.group == 0) {
                    ERROR("Unable to find a matching named group");
                    return sendAlert(mem, FATAL, HANDSHAKE_FAILURE);
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
            return sendAlert(mem, settings.alert.level, settings.alert.description);
        }
    }

    // validate extensions
    if (settings.version != TLS_1_3) {
        ERROR("Missing supported versions extension");
        return sendAlert(mem, FATAL, PROTOCOL_VERSION);
    }
    if (settings.signature_algorithum == 0) {
        ERROR("Missing signature algorithum");
        return sendAlert(mem, FATAL, MISSING_EXTENSION);
    }
    if (settings.certificate_algorithum == 0) {
        settings.certificate_algorithum = settings.signature_algorithum;
    }
    if (settings.group == 0) {
        ERROR("Missing supported groups");
        return sendAlert(mem, FATAL, MISSING_EXTENSION);
    }
    if (key_share.slice.length == 0) {
        ERROR("Missing key share");
        return sendAlert(mem, FATAL, MISSING_EXTENSION);
        // TODO: this should send a HelloRetryRequest insteadd
    }
    decodeKeyShare(&settings, key_share);
    if (settings.key.length == 0) {
        ERROR("Unable to find a matching key");
        return sendAlert(mem, FATAL, MISSING_EXTENSION);
        // TODO: this should send a HelloRetryRequest insteadd
    }
    DEBUG("Key");
    hexDump(settings.key);

    return sendServerHello(mem, &settings);
}

static Slice decodeHandshake(Memory* mem, Reader reader) {
    do {
        HandshakeType type = readBE8(&reader);
        U32 length = readBE24(&reader);
        if (length > readerRemaining(&reader)) {
            ERROR("Invlaid handshake message length");
            return sendAlert(mem, FATAL, DECODE_ERROR);
        }
        Reader handshake_reader = readReader(&reader, length);

        switch (type) {
            case CLIENT_HELLO:
                return decodeClientHello(mem, handshake_reader);
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
                return sendAlert(mem, FATAL, INTERNAL_ERROR);
            default:
                DEBUG("Unknown TLS handshake type: %d", type);
                return sendAlert(mem, FATAL, UNEXPECTED_MESSAGE);
        }
    } while (!readerEOF(&reader));
}

// record processing
static Slice decodeRecord(Memory* mem, Slice data) {
    if (data.length < 5) {
        WARN("TLS message is shorter than a complete header, waiting...");
        return sliceEmpty();
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
            return sendAlert(mem, FATAL, UNEXPECTED_MESSAGE);
    }

    DEBUG("Record legacy version: %04X", readBE16(&reader));

    U16 length = readBE16(&reader);
    if (length > (2 << 14)) {
        DEBUG("TLS fragment length exceeds 2^14 bytes.");
        return sendAlert(mem, FATAL, RECORD_OVERFLOW);
    }
    if (length > readerRemaining(&reader)) {
        WARN("TLS message is shorter than fragment length, waiting...");
        return sliceEmpty();
    }

    Reader record_reader = readReader(&reader, length);
    switch (type) {
        case HANDSHAKE:
            return decodeHandshake(mem, record_reader);
        case ALERT:
            return decodeAlert(mem, record_reader);
        default:
            ERROR("TLS content type not implemented: %d", type);
            return sendAlert(mem, FATAL, INTERNAL_ERROR);
    }
}

// the server
void tlsTest(Memory* mem) {
    Slice response = decodeRecord(mem, sliceFromHex(mem, "16030100f8010000f40303000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20e0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff000813021303130100ff010000a30000001800160000136578616d706c652e756c666865696d2e6e6574000b000403000102000a00160014001d0017001e0019001801000101010201030104002300000016000000170000000d001e001c040305030603080708080809080a080b080408050806040105010601002b0003020304002d00020101003300260024001d0020358072d6365880d1aeea329adf9121383851ed21a28e3b75e965d0d2cd166254"));
    expect("Response", response, sliceFromHex(mem, "160303007a020000760303707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f20e0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff130200002e002b0002030400330024001d00209fd7ad6dcff4298dd3f96d5b1b2af910a0535b1488d7f8fabb349a982880b615"));
}