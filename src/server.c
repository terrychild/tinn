#include "console.h"
#include "server.h"
#include "client.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void server_listener(Sockets* sockets, int index, Routes* routes) {
	(void)routes; //un-used

	struct sockaddr_storage address;
	socklen_t address_size;
	int client_socket;
	ClientState* client_state;
	int client_index;

	if (sockets->pollfds[index].revents & (POLLERR | POLLHUP | POLLNVAL)) {
		PANIC("error on server socket: %d", sockets->pollfds[index].revents);
	} 
	
	address_size = sizeof(address);
	if ((client_socket = accept(sockets->pollfds[index].fd, (struct sockaddr *)&address, &address_size)) < 0) {
		ERROR("accept");
	} else {
		client_state = client_state_new();
		client_index = sockets_add(sockets, client_socket, client_listener);

		sockets->states[client_index] = client_state;
		inet_ntop(address.ss_family, get_in_addr((struct sockaddr *)&address), client_state->address, INET6_ADDRSTRLEN);

		LOG("connection from %s (%d) opened", client_state->address, client_socket);
	}
}