#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>

#include "net.h"
#include "console.h"

int get_server_socket(char* port) {
	int status;

	struct addrinfo hints;
	struct addrinfo* addresses;
	struct addrinfo* address;

	int sock;

	// get local address info
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(NULL, port, &hints, &addresses)) != 0) {
		ERROR("gettaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}

	// get a socket and bind to it
	for (address = addresses; address != NULL; address = address->ai_next) {
		if ((sock = socket(address->ai_family, address->ai_socktype, address->ai_protocol)) < 0) {
			continue;
		}

		// re-use socket if in use
		int yes=1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(sock, address->ai_addr, address->ai_addrlen) != 0) {
			close(sock);
			continue;
		}

		break;
	}

	// if address is null nothing bound
	if (address == NULL) {
		freeaddrinfo(addresses);
		ERROR("unable to bind to a socket");
		return -1;
	}

	freeaddrinfo(addresses);

	// listen to socket
	if (listen(sock, 10) != 0) {
		ERROR("uable to listen to a socket");
		return -1;
	}

	return sock;
}

Sockets* sockets_new() {
	Sockets* list = malloc(sizeof(*list));
	if (list  == NULL) {
		PANIC("unable to allocate memory for sockets list");
	}

	list->size = 5;
	list->count = 0;

	list->pollfds = malloc(sizeof(*list->pollfds) * list->size);
	list->listeners = malloc(sizeof(*list->listeners) * list->size);
	list->states = malloc(sizeof(*list->states) * list->size);

	if (list->pollfds == NULL || list->listeners == NULL || list->states == NULL) {
		PANIC("unable to allocate memory for sockets list");
	}

	return list;
}

int sockets_add(Sockets* list, int new_socket, socket_listener new_listener) {
	// do we need to expand the arrays
	if (list->count == list->size) {
		list->size *= 2;

		list->pollfds = realloc(list->pollfds, sizeof(*list->pollfds) * list->size);
		list->listeners = realloc(list->listeners, sizeof(*list->listeners) * list->size);
		list->states = realloc(list->states, sizeof(*list->states) * list->size);

		if (list->pollfds == NULL || list->listeners == NULL || list->states == NULL) {
			PANIC("unable to allocate memory for sockets list");
		}
	}

	// add new socket
	list->pollfds[list->count].fd = new_socket;
	list->pollfds[list->count].events = POLLIN;
	list->pollfds[list->count].revents = 0;

	list->listeners[list->count] = new_listener;

	list->states[list->count] = NULL;

	// update count
	list->count++;
	return list->count - 1;
}

void sockets_rm(Sockets* list, size_t index) {
	if (index < list->count) {
		list->pollfds[index] = list->pollfds[list->count-1];
		list->listeners[index] = list->listeners[list->count-1];
		list->states[index] = list->states[list->count-1];
		list->count--;
	}
}