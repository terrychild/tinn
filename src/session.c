#include <stdint.h>
#include <string.h>

#include "session.h"
#include "utils.h"
#include "console.h"

typedef enum {
	tls_rec_invalid = 0,
	tls_rec_change_cipher_spec = 20,
	tls_rec_alert = 21,
	tls_rec_handshake = 22,
	tls_rec_application_data = 23
} ContentType;

typedef enum {
	tls_handshake_client_hello = 1,
	tls_handshake_server_hello = 2,
	tls_handshake_new_session_ticket = 4,
	tls_handshake_end_of_early_data = 5,
	tls_handshake_encrypted_extensions = 8,
	tls_handshake_certificate = 11,
	tls_handshake_certificate_request = 13,
	tls_handshake_certificate_verify = 15,
	tls_handshake_finished = 20,
	tls_handshake_key_update = 24,
	tls_handshake_message_hash = 254
} HandshakeType;

typedef enum {
	TLS_AES_128_GCM_SHA256 = 0x1301,
	TLS_AES_256_GCM_SHA384 = 0x1302,
	TLS_CHACHA20_POLY1305_SHA256 = 0x1303,
	TLS_AES_128_CCM_SHA256 = 0x1304,
	TLS_AES_128_CCM_8_SHA256 = 0x1305
} CipherSuite;

typedef enum {
	tls_extension_server_name = 0,
	tls_extension_max_fragment_length = 1,
	tls_extension_status_request = 5,
	tls_extension_supported_groups = 10,
	tls_extension_signature_algorithms = 13,
	tls_extension_use_srtp = 14,
	tls_extension_heartbeat = 15,
	tls_extension_application_layer_protocol_negotiation = 16,
	tls_extension_signed_certificate_timestamp = 18,
	tls_extension_client_certificate_type = 19,
	tls_extension_server_certificate_type = 20,
	tls_extension_padding = 21,
	tls_extension_pre_shared_key = 41,
	tls_extension_early_data = 42,
	tls_extension_supported_versions = 43,
	tls_extension_cookie = 44,
	tls_extension_psk_key_exchange_modes = 45,
	tls_extension_certificate_authorities = 47,
	tls_extension_oid_filters = 48,
	tls_extension_post_handshake_auth = 49,
	tls_extension_signature_algorithms_cert = 50,
	tls_extension_key_share = 51
} ExtensionType;

enum {
	tls_al_warning = 1, 
	tls_al_fatal = 2
} AlertLevel;

typedef enum {
	tls_alert_close_notify = 0,
	tls_alert_unexpected_message = 10,
	tls_alert_bad_record_mac = 20,
	tls_alert_record_overflow = 22,
	tls_alert_handshake_failure = 40,
	tls_alert_bad_certificate = 42,
	tls_alert_unsupported_certificate = 43,
	tls_alert_certificate_revoked = 44,
	tls_alert_certificate_expired = 45,
	tls_alert_certificate_unknown = 46,
	tls_alert_illegal_parameter = 47,
	tls_alert_unknown_ca = 48,
	tls_alert_access_denied = 49,
	tls_alert_decode_error = 50,
	tls_alert_decrypt_error = 51,
	tls_alert_protocol_version = 70,
	tls_alert_insufficient_security = 71,
	tls_alert_internal_error = 80,
	tls_alert_inappropriate_fallback = 86,
	tls_alert_user_canceled = 90,
	tls_alert_missing_extension = 109,
	tls_alert_unsupported_extension = 110,
	tls_alert_unrecognized_name = 112,
	tls_alert_bad_certificate_status_response = 113,
	tls_alert_unknown_psk_identity = 115,
	tls_alert_certificate_required = 116,
	tls_alert_no_application_protocol = 120
} AlertDescription;


Session* session_new() {
	Session* session = allocate(NULL, sizeof(*session));
	session->bufin = buf_new(1024);
	session->bufout = buf_new(1024);
	session->state = STATE_START;
	session->close = false;
	return session;
}
void session_free(Session* session) {
	buf_free(session->bufin);
	buf_free(session->bufout);
	free(session);
}

static void session_send(struct pollfd* pfd, Session* session) {
	Buffer* buf = session->bufout;
	size_t len = buf_read_max(buf);
	ssize_t sent = send(pfd->fd, buf_read_ptr(buf), len, MSG_DONTWAIT);
	if (sent < 0) {
		ERROR("send error for %s (%d)", session->address, pfd->fd);
		session->close = true;
		return;
	}
	TRACE("sent: %ld/%ld", sent, len);

	if ((size_t)sent < len) {
		buf_advance_read(buf, sent);
		pfd->events = POLLOUT;
	} else if (!session->close) {
		/*if (token_is(request->connection, "close")) {
			return false;
		}*/
		buf_reset(buf);
		buf_reset(session->bufin);
		pfd->events = POLLIN;
	}
}

/*static void session_send_error(struct pollfd* pfd, Session* session, AlertDescription error) {
	Buffer* buf = session->bufout;

	buf_reset(buf);
	buf_append_char(buf, 21);
	buf_append_char(buf, 3);
	buf_append_char(buf, 3);
	buf_append_char(buf, 0);
	buf_append_char(buf, 2);
	buf_append_char(buf, tls_al_fatal);
	buf_append_char(buf, error);

	session_send(pfd, session);
	session->close = true;
}*/

static void log_hex(Buffer* buf, int start, int end) {
	if (end<start) {
		end = buf->length;
	}
	Buffer* hex = buf_new((end-start) * 3);
	for (int i=start; i<end; i++) {
		buf_append_format(hex, "%02x ", (unsigned char)buf->data[i]);
	}
	DEBUG_DETAIL(buf_as_str(hex));
	buf_free(hex);
}

static void log_hex_array(uint8_t* str, int start, int end) {
	Buffer* hex = buf_new((end-start) * 3);
	for (int i=start; i<end; i++) {
		buf_append_format(hex, "%02x ", (unsigned char)str[i]);
	}
	DEBUG_DETAIL(buf_as_str(hex));
	buf_free(hex);
}

static uint8_t read8(Buffer* buf) {
	return (uint8_t)buf_read_char(buf);
}
static uint16_t read16(Buffer* buf) {
	return ((uint16_t)read8(buf))<<8 | (uint16_t)read8(buf);
}
static uint32_t read24(Buffer* buf) {
	return ((uint32_t)read8(buf))<<16 | ((uint32_t)read8(buf))<<8 | (uint32_t)read8(buf);
}

static void session_read(struct pollfd* pfd, Session* session) {
	Buffer* bufin = session->bufin;

	int recvied = recv(pfd->fd, buf_write_ptr(bufin), buf_write_max(bufin), 0);
	if (recvied <= 0) {
		if (recvied < 0) {
			ERROR("recv error from %s (%d)", session->address, pfd->fd);
		} else {
			LOG("connection from %s (%d) closed", session->address, pfd->fd);
		}
		session->close = true;
		return;
	}
	TRACE("recived: %d", recvied);

	// update buffer
	buf_advance_write(bufin, recvied);

	// read
	if (session->tls) {
		// tls header
		if (bufin->length < 5) {
			ERROR("Incomplete TLS header");
			session->close = true;
			return;
		}
		uint8_t rec_type = read8(bufin);
		uint16_t rec_version = read16(bufin);
		uint16_t rec_len = read16(bufin);
		DEBUG("type: %d, version: 0x%04x, len: %d", rec_type, rec_version, rec_len);

		switch(session->state) {
		case STATE_START:
			if (rec_type != tls_rec_handshake) {
				ERROR("Invalid TLS header 0x%02x", rec_type);
				session->close = true;
				return;
			}
			if (rec_version != 0x0301 && rec_version != 0x0303) {
				ERROR("Invalid TLS version 0x%04x", rec_version);
				session->close = true;
				return;
			}
			if (bufin->length < rec_len+5) {
				TRACE("incomplete handshake, wait");
				buf_advance_read(bufin, -5);
				buf_grow(bufin);
				return;
			}

			uint8_t handshake_type = read8(bufin);
			uint32_t handshake_len = read24(bufin);

			if (handshake_type != tls_handshake_client_hello) {
				ERROR("Invalid TLS handshake 0x%02x", handshake_type);
				session->close = true;
				return;
			}

			uint16_t client_hello_version = read16(bufin);
			uint8_t client_hello_random[32];
			memcpy(client_hello_random, buf_read_ptr(bufin), 32);
			buf_advance_read(bufin, 32);
			DEBUG("client random");
			log_hex_array(client_hello_random, 0, 32);

			uint8_t client_hello_session_id_len = read8(bufin);
			uint8_t client_hello_session_id[32];
			if (client_hello_session_id_len == 32) {
				memcpy(client_hello_session_id, buf_read_ptr(bufin), 32);
				buf_advance_read(bufin, 32);
			} else if (client_hello_session_id_len == 0) {
				memset(client_hello_session_id, 0, 32);
			}
			DEBUG("client session (%d)", client_hello_session_id_len);
			log_hex_array(client_hello_session_id, 0, 32);

			uint16_t cipher_suite = 0;
			uint16_t client_hello_cipher_len = read16(bufin);
			DEBUG("cilent ciphers: %d", client_hello_cipher_len);
			for (int i=0; i<client_hello_cipher_len; i+=2) {
				uint16_t client_hello_cipher_suite = read16(bufin);
				if (!cipher_suite && client_hello_cipher_suite == TLS_AES_128_GCM_SHA256) {
					cipher_suite = TLS_AES_128_GCM_SHA256;
					DEBUG("found cipher");
				}
			}

			uint8_t client_hello_compression_len = read8(bufin);
			uint8_t client_hello_compression = read8(bufin);
			if (client_hello_compression_len != 1 || client_hello_compression != 0) {
				ERROR("Invalid TLS compression (0x%02x 0x%02x)", client_hello_compression_len, client_hello_compression);
				session->close = true;
				return;
			}

			log_hex(bufin, bufin->read_pos, bufin->read_pos + 2);
			uint16_t client_hello_extensions_len = read16(bufin);
			if (client_hello_extensions_len > 0) {
				if (buf_read_max(bufin) < client_hello_extensions_len) {
					ERROR("Missing TLS extension data (%d/%d)", client_hello_extensions_len, buf_read_max(bufin));
					session->close = true;
					return;
				}
				DEBUG("extensions %d %d", client_hello_extensions_len, buf_read_max(bufin));
			}
			

			break;

		case STATE_CONNECTED:
			if (session->tls) {
				log_hex(bufin, 0, -1);
			} else {
				DEBUG_DETAIL(buf_as_str(bufin));
			}
			break;
		}
	} else {
		DEBUG_DETAIL(buf_as_str(bufin));
	}
}

void session_listener(Sockets* sockets, int index) {
	struct pollfd* pfd = &sockets->pollfds[index];
	Session* session = sockets->states[index];

	if (pfd->revents & POLLHUP) {
		LOG("connection from %s (%d) hung up", session->address, pfd->fd);
		session->close = true;
	} else if (pfd->revents & (POLLERR | POLLNVAL)) {
		ERROR("Socket error from %s (%d): %d", session->address, pfd->fd, pfd->revents);
		session->close = true;
	} else {
		if (pfd->revents & POLLIN) {
			session_read(pfd, session);
		} else if (pfd->revents & POLLOUT) {
			session_send(pfd, session);
		}
	}

	if (session->close) {
		close(pfd->fd);
		session_free(session);
		sockets_rm(sockets, index);
	}
}