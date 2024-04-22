#include "console.h"
#include "server.h"
#include "client.h"

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
	ClientState* client_state;
	int client_index;

	if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
		PANIC("error on server socket: %d", pfd->revents);
	} 
	
	address_size = sizeof(address);
	if ((client_socket = accept(pfd->fd, (struct sockaddr *)&address, &address_size)) < 0) {
		ERROR("accept");
	} else {
		client_index = sockets_add(sockets, client_socket, client_listener);

		client_state = client_state_new();
		client_state->content = server_state->content;
		inet_ntop(address.ss_family, get_in_addr((struct sockaddr *)&address), client_state->address, INET6_ADDRSTRLEN);
		sockets->states[client_index] = client_state;		

		LOG("connection from %s (%d) opened", client_state->address, client_socket);
	}
}

void server_new(Sockets* sockets, int socket, ContentGenerators* content) {
	int index = sockets_add(sockets, socket, server_listener);

	ServerState* state = server_state_new();
	state->content = content;
	sockets->states[index] = state;	
}