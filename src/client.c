#include "console.h"
#include "client.h"
#include "buffer.h"

ClientState* client_state_new() {
	ClientState* state = allocate(NULL, sizeof(*state));
	state->request = request_new();
	state->response = response_new();
	return state;
}
void client_state_free(ClientState* state) {
	request_free(state->request);
	response_free(state->response);
	free(state);
}

static bool send_response(struct pollfd* pfd, ClientState* state) {
	Buffer* buf = response_buf(state->response);

	long len = (long)buf_read_max(buf);
	long sent = send(pfd->fd, buf_read_ptr(buf), len, MSG_DONTWAIT);
	TRACE("sent: %ld/%ld", sent, len);
	if (sent < 0) {
		ERROR("send error for %s (%d)", state->address, pfd->fd);
		return false;
	}
	if (sent < len) {
		buf_advance_read(buf, sent);
		pfd->events = POLLOUT;
	} else {
		response_reset(state->response);
		pfd->events = POLLIN;
	}
	return true;
}

static bool read_request(struct pollfd* pfd, ClientState* state) {
	Request* request = state->request;
	Response* response = state->response;

	int recvied = request_recv(request, pfd->fd);
	if (recvied <= 0) {
		if (recvied < 0) {
			ERROR("recv error from %s (%d)", state->address, pfd->fd);
		} else {
			LOG("connection from %s (%d) closed", state->address, pfd->fd);
		}
		return false;
	} else {
		if (request->method.length==0 || !request->target->valid || request->version.length==0) {
			WARN("Bad request from %s (%d)", state->address, pfd->fd);
			TRACE_DETAIL("method: %.*s", request->method.length, request->method.start);
			TRACE_DETAIL("target: %s", request->target->data);
			TRACE_DETAIL("version: %.*s", request->version.length, request->version.start);
			response_simple_status(response, 400, "Oops, that request made no sense.");

		} else if (!token_is(request->version, "HTTP/1.0") && !token_is(request->version, "HTTP/1.1")) {
			WARN("Unsupported HTTP version (%.*s) from %s (%d)", request->version.length, request->version.start, state->address, pfd->fd);
			response_simple_status(response, 505, "Oops, that version of HTTP is not supported.");

		} else {
			LOG("\"%.*s\" \"%s\" from %s (%d)", request->method.length, request->method.start, request->target->path, state->address, pfd->fd);

			bool ready = false;
			for (int i=0; !ready && i<state->content->count; i++) {
				ready = state->content->generators[i](request, response);
			}

			if (!ready) {
				response_simple_status(response, 404, "Oops, that resource can not be found.");
			}
		}

		// done reading request so reset?
		request_reset(state->request);

		// send
		return send_response(pfd, state);
	}
}

void client_listener(Sockets* sockets, int index) {
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
			flag = read_request(pfd, state);
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