#include "server.h"
#include "utils.h"
#include "console.h"
#include "session.h"

ServerState* server_state_new() {
	ServerState* state = allocate(NULL, sizeof(*state));
	return state;
}
void server_state_free(ServerState* state) {
	free(state);
}

// get sockaddr, IPv4 or IPv6:
static void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static void server_listener(Sockets* sockets, int index) {
	struct pollfd* pfd = &sockets->pollfds[index];
	ServerState* server_state = sockets->states[index];

	struct sockaddr_storage address;
	socklen_t address_size;
	int client_socket;
	int client_index;
	Session* session;

	if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
		PANIC("error on server socket: %d", pfd->revents);
	} 
	
	address_size = sizeof(address);
	if ((client_socket = accept(pfd->fd, (struct sockaddr *)&address, &address_size)) < 0) {
		ERROR("accept");
	} else {
		client_index = sockets_add(sockets, client_socket, session_listener);

		session = session_new();
		session->content = server_state->content;
		session->tls = server_state->tls;
		inet_ntop(address.ss_family, get_in_addr((struct sockaddr *)&address), session->address, INET6_ADDRSTRLEN);
		sockets->states[client_index] = session;		

		LOG("connection from %s (%d) opened", session->address, client_socket);
	}
}

void server_new(Sockets* sockets, int socket, bool tls, ContentGenerators* content) {
	int index = sockets_add(sockets, socket, server_listener);

	ServerState* state = server_state_new();
	state->content = content;
	state->tls = tls;
	sockets->states[index] = state;	
}