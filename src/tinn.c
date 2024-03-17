#include <stdlib.h>

#include "utils.h"
#include "console.h"
#include "routes.h"
#include "blog.h"
#include "server.h"

// ================ Main loop etc ================
int main(int argc, char* argv[]) {
	LOG("Tinn v0.8.0-alpha");

	// validate arguments
	if (argc != 3) {
		ERROR("correct usage is -> %s port content_directory", argv[0]);
		return EXIT_FAILURE;
	}

	// change working directory to content directory
	if (chdir(argv[2]) != 0) {
		ERROR("invalid content directory (%s)", argv[2]);
		return EXIT_FAILURE;
	}
	
	// build routes
	TRACE("building routes");
	Routes* routes = routes_new();
	routes_add_static(routes);
	
	if (!blog_build(routes)) {
		ERROR("loading blog");
		return EXIT_FAILURE;
	}
	routes_log(routes, CL_TRACE);
	
	// create list of sockets
	TRACE("creating list of sockets");
	Sockets* sockets = sockets_new();
	
	// open server socket
	TRACE("opening server socket");
	int server_socket = get_server_socket(argv[1]);
	if (server_socket < 0) {
		ERROR("getting server socket");
		return EXIT_FAILURE;
	}

	sockets_add(sockets, server_socket, server_listener);
	LOG("waiting for connections");

	// loop forever directing network traffic
	for (;;) {
		if (poll(sockets->pollfds, sockets->count, -1) < 0 ) {
			PANIC("when polling");
		}

		for (size_t i = 0; i < sockets->count; i++) {
			if (sockets->pollfds[i].revents) {
				sockets->listeners[i](sockets, i, routes);
			}
		}
	}

	// tidy up, but we should never get here?
	close(server_socket);
	routes_free(routes);	
	
	return EXIT_SUCCESS;
}
