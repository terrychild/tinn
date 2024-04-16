#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "console.h"
#include "routes.h"
#include "blog.h"
#include "server.h"
#include "version.h"

static void usage_exit() {
	puts("usage: tinn [OPTIONS] [content_directory]\n");
	puts("When not specified the content directory defaults to the current directory.\n");
	puts("Options:");
	puts("  -h, --help         Display this help.");
	puts("      --version      Display version.");
	puts("  -v, --verbose      Enable verbose logging.");
	puts("  -p port            Port to listen on, defaults to 8080.");
	exit(EXIT_SUCCESS);
}

static void version_exit() {
	printf("Tinn %s (%s)\n", VERSION, BUILD_DATE);
	exit(EXIT_SUCCESS);
}

struct settings_t {
	char* port;
	char* content_dir;
};

static struct settings_t parse_arguments(int count, char* values[]) {
	struct settings_t settings = {
		.port = "8080",
		.content_dir = "."
	};
	bool set_content_dir = false;

	for (int i=1; i<count; i++) {
		if (values[i][0] == '-') {
			int len = strlen(values[i]);
			if (len==1) {
				usage_exit();
			}
			if (values[i][1] == '-') {
				if (strcmp(values[i], "--help")==0) {
					usage_exit();
				} else if (strcmp(values[i], "--version")==0) {
					version_exit();
				} else if (strcmp(values[i], "--verbose")==0) {
					clevel = CL_TRACE;
				}
			} else {
				if (values[i][1] == 'h') {
					usage_exit();
				} else if (values[i][1] == 'v') {
					clevel = CL_TRACE;
				} else if (values[i][1] == 'p') {
					if (i==count-1) {
						usage_exit();
					}
					settings.port = values[i+1];
					i++;
				} else {
					usage_exit();
				}
			}
		} else {
			if (set_content_dir) {
				usage_exit();
			}
			set_content_dir = true;
			settings.content_dir = values[i];
		}
	}

	return settings;
}

// ================ Main loop etc ================
int main(int argc, char* argv[]) {
	// parse/validate settings
	struct settings_t settings = parse_arguments(argc, argv);

	LOG("Tinn %s (%s)", VERSION, BUILD_DATE);
	
	// change working directory to content directory
	if (chdir(settings.content_dir) != 0) {
		ERROR("invalid content directory (%s)", settings.content_dir);
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
	int server_socket = get_server_socket(settings.port);
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
