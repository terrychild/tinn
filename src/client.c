#include <string.h>
#include <sys/stat.h>

#include "console.h"
#include "client.h"
#include "buffer.h"

void client_reset_headers(ClientState* state) {
	state->h_if_modified_Since = 0;
}
ClientState* client_state_new() {
	ClientState* state = allocate(NULL, sizeof(*state));
	state->in = buf_new(1024);
	state->out = buf_new(1024);
	client_reset_headers(state);
	return state;
}
void client_state_free(ClientState* state) {
	buf_free(state->in);
	buf_free(state->out);
	free(state);
}

static void header(Buffer* buf, const char* name, const char* value) {
	buf_append_format(buf, "%s: %s\r\n", name, value);
}

static void date_header(Buffer* buf, const char* name, time_t date) {
	buf_append_str(buf, name);
	buf_append_str(buf, ": ");
	to_imf_date(buf_reserve(buf, IMF_DATE_LEN), IMF_DATE_LEN, date);
	buf_advance_write(buf, -1);
	buf_append_str(buf, "\r\n");
}

static void start_headers(Buffer* buf, const char* status_code, const char* status_text) {
	// status line
	buf_append_format(buf, "HTTP/1.1 %s %s\r\n", status_code, status_text);

	// default headers
	date_header(buf, "Date", time(NULL));
	header(buf, "Server", "Tinn");
}

static void end_headers(Buffer* buf) {
	buf_append_str(buf, "\r\n");
}

static void content_type(Buffer* buf, char* ext) {
	buf_append_str(buf, "Content-Type: ");
	if (ext == NULL || strlen(ext) == 0) {
		buf_append_str(buf, "text/plain; charset=utf-8");
	} else {
		if (ext[0] == '.') {
			ext += 1;
		}

		if (strcmp(ext, "html")==0 || strcmp(ext, "htm")==0) {
			buf_append_str(buf, "text/html; charset=utf-8");
		} else if (strcmp(ext, "css")==0) {
			buf_append_str(buf, "text/css; charset=utf-8");
		} else if (strcmp(ext, "js")==0) {
			buf_append_str(buf, "text/javascript; charset=utf-8");
		} else if (strcmp(ext, "jpeg")==0 || strcmp(ext, "jpg")==0) {
			buf_append_str(buf, "image/jpeg");
		} else if (strcmp(ext, "png")==0) {
			buf_append_str(buf, "image/png");
		} else if (strcmp(ext, "gif")==0) {
			buf_append_str(buf, "image/gif");
		} else if (strcmp(ext, "bmp")==0) {
			buf_append_str(buf, "image/bmp");
		} else if (strcmp(ext, "svg")==0) {
			buf_append_str(buf, "image/svg+xml");
		} else if (strcmp(ext, "ico")==0) {
			buf_append_str(buf, "image/vnd.microsoft.icon");
		} else if (strcmp(ext, "mp3")==0) {
			buf_append_str(buf, "audio/mpeg");
			//buf_append_str(buf, "audio/mpeg\r\nContent-Disposition: attachment; filename=\"my-file.mp3\"");
		} else {
			buf_append_str(buf, "text/plain; charset=utf-8");
		}
	}
	buf_append_str(buf, "\r\n");
}

static void content_length(Buffer* buf, long length) {
	buf_append_format(buf, "Content-Length: %ld\r\n", length);
}

static void prep_simple_status(ClientState* state, char *status_code, char *status_text, char *description) {
	Buffer* body = buf_new(256);
	buf_append_format(body, "<html><body><h1>%s - %s</h1><p>%s</p></body></html>", status_code, status_text, description);

	// headers
	start_headers(state->out, status_code, status_text);
	content_type(state->out, "html");
	content_length(state->out, body->length);
	end_headers(state->out);

	// content
	buf_append_buf(state->out, body);
}

static void prep_redirect(ClientState* state, char* location) {
	start_headers(state->out, "301", "Moved Permanently");
	buf_append_format(state->out, "Location: %s\r\n", location);
	end_headers(state->out);
}

static bool prep_file(ClientState* state, char* path) {
	// check modified time
	struct stat attrib;
    stat(path, &attrib);

    if (state->h_if_modified_Since>0 && state->h_if_modified_Since>=attrib.st_mtime) {
    	TRACE_DETAIL("not modified");
    	start_headers(state->out, "304", "Not Modified");
    	end_headers(state->out);
    	return true;
    }

	// open file and get content length
	long length;
	FILE *file = fopen(path, "rb");
	
	if (file == NULL) {
		return false;
	}

	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);

	// populate header
	start_headers(state->out, "200", "OK");
	//header(state->out, "Cache-Control", "max-age=604800"); // one week
	header(state->out, "Cache-Control", "no-cache");
	date_header(state->out, "Last-Modified", attrib.st_mtime);
	content_type(state->out, strrchr(path, '.'));
	content_length(state->out, length);
	end_headers(state->out);

	// populate content
	char* body = buf_reserve(state->out, length);
	fread(body, 1, length, file);
	fclose(file);

	return true; // did we real look for error? no.
}

static void prep_buffer(ClientState* state, Buffer* buffer) {
	// populate header
	start_headers(state->out, "200", "OK");
	content_type(state->out, "html");
	content_length(state->out, buffer->length);
	end_headers(state->out);

	// populate content
	buf_append_buf(state->out, buffer);
}

static bool send_response(struct pollfd* pfd, ClientState* state) {
	long len = (long)buf_read_max(state->out);
	long sent = send(pfd->fd, buf_read_ptr(state->out), len, MSG_DONTWAIT);
	TRACE("sent: %ld/%ld", sent, len);
	if (sent < 0) {
		ERROR("send error for %s (%d)", state->address, pfd->fd);
		return false;
	}
	if (sent < len) {
		buf_advance_read(state->out, sent);
		pfd->events = POLLOUT;
	} else {
		buf_reset(state->out);
		pfd->events = POLLIN;
	}
	return true;
}

static int blank_line(Buffer* buf) {
	for (int i=3; i<buf->length; i++) {
		if (buf->data[i-3]=='\r' && buf->data[i-2]=='\n' && buf->data[i-1]=='\r' && buf->data[i]=='\n') {
			return i-1;
		}
	}
	return -1;
}

static bool read_request(struct pollfd* pfd, ClientState* state, Routes* routes) {
	int recvied = recv(pfd->fd, buf_write_ptr(state->in), buf_write_max(state->in), 0);
	if (recvied <= 0) {
		if (recvied < 0) {
			ERROR("recv error from %s (%d)", state->address, pfd->fd);
		} else {
			LOG("connection from %s (%d) closed", state->address, pfd->fd);
		}
		return false;
	} else {
		// update buffer
		buf_advance_write(state->in, recvied);

		// check for end of line
		int end = blank_line(state->in);

		// make buffer bigger ?
		if (end<0) {			
			buf_grow(state->in);
			return true;
		}

		//TRACE("%.*s", end, state->in->data);

		Scanner scanner = scanner_new(state->in->data, end);

		Token method = scan_token(&scanner, " ");
		Token target = scan_token(&scanner, " ");
		Token version = scan_token(&scanner, "\r\n");

		if (method.length==0 || target.length==0 || version.length==0) {
			WARN("Bad request from %s (%d)", state->address, pfd->fd);
			TRACE_DETAIL("method: %.*s", method.length, method.start);
			TRACE_DETAIL("target: %.*s", target.length, target.start);
			TRACE_DETAIL("version: %.*s", version.length, version.start);
			prep_simple_status(state, "400", "Bad Request", "Opps, that request made no sense.");

		} else if (!token_is(version, "HTTP/1.0") && !token_is(version, "HTTP/1.1")) {
			WARN("Unsupported HTTP version (%.*s) from %s (%d)", version.length, version.start, state->address, pfd->fd);
			prep_simple_status(state, "505", "HTTP Version Not Supported", "Opps, that version of HTTP is not supported.");

		} else {
			char path[target.length + 1];
			memcpy(path, target.start, target.length);
			path[target.length] = '\0';

			char* query = strchr(path, '?');
			if (query != NULL) {
				*(query) = '\0';
				query++;
			}

			LOG("\"%.*s\" \"%s\" from %s (%d)", method.length, method.start, path, state->address, pfd->fd);

			client_reset_headers(state);
			Token line;
			while ((line = scan_token(&scanner, "\r\n")).length>0) {
				Scanner header_scanner = scanner_new(line.start, line.length);
				Token name = scan_token(&header_scanner, ": \t");
				Token value = scan_token(&header_scanner, "");

				TRACE_DETAIL("%.*s: %.*s", name.length, name.start, value.length, value.start);
				if (token_is(name, "If-Modified-Since")) {
					state->h_if_modified_Since = from_imf_date(value.start, value.length);
				}				
			}		

			bool is_get = token_is(method, "GET");
			//bool is_head = token_is(method, "HEAD");

			if (is_get) {
				Route* route = routes_find(routes, path);
				route_log(route, CL_DEBUG);
				if (route == NULL) {
					prep_simple_status(state, "404", "Not Found", "Opps, that resource can not be found.");
				} else if (route->type == RT_FILE) {
					if (!prep_file(state, route->to)) {
						prep_simple_status(state, "404", "Not Found", "Opps, that resource can not be found.");
					}
				} else if (route->type == RT_REDIRECT) {
					prep_redirect(state, route->to);
				} else if (route->type == RT_BUFFER) {
					prep_buffer(state, route->to);
				} else {
					prep_simple_status(state, "500", "Internal Server Error", "Opps, something went wrong that is probably not your fault, probably.");
				}
			} else {
				prep_simple_status(state, "501", "Not Implemented", "Opps, that functionality has not been implemented.");
			}
		}

		// done reading request so reset buffer
		buf_consume(state->in, end+2);

		// send
		return send_response(pfd, state);
	}
}

void client_listener(Sockets* sockets, int index, Routes* routes) {
	struct pollfd* pfd = &sockets->pollfds[index];
	ClientState* state = sockets->states[index];

	bool flag = true;
	if (pfd->revents & POLLHUP) {
		LOG("connection from %s (%d) hung up", state->address, pfd->fd);
		flag = false;
	} else if (pfd->revents & (POLLERR | POLLNVAL)) {
		ERROR("Socket error from %s (%d): %d", state->address, pfd->fd, pfd->revents);
		flag = false;
	} else {
		if (pfd->revents & POLLIN) {
			flag = read_request(pfd, state, routes);
		} else if (pfd->revents & POLLOUT) {
			flag = send_response(pfd, state);
		}
	}

	if (!flag) {
		close(pfd->fd);
		client_state_free(state);
		sockets_rm(sockets, index);
	}
}