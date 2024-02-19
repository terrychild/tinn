#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdarg.h> // for varidic functions
#include <unistd.h> // for system calls, like read and close etc
#include <fts.h> // for file system access stuff
#include <netdb.h> // for getaddrinfo
#include <arpa/inet.h> // for inet stuff
#include <poll.h>

// formated printing with the time
void tprintf(const char* format, ...) {
	// output the time
	time_t seconds = time(NULL);
	struct tm* gmt = gmtime(&seconds);
	printf("%02d:%02d:%02d ", gmt->tm_hour, gmt->tm_min, gmt->tm_sec);

	// output the rest
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
}

// ================ Static files ================

struct static_file {
	char* local_path;
	char* server_path;
	struct static_file* next;
};

struct static_file* files; // a global?????

struct static_file* find_static_files(char* path) {
	// a simple linked list of files
	struct static_file temp_head;
	temp_head.next = NULL;
	struct static_file* tail = &temp_head;

	// To work out the server path, we are going to remove the traversal to 
	// the root directory, but we want to keep the forward slash so all 
	// server paths start with one.
	int pathlen = strlen(path);
	if (pathlen>0) {
		if (path[pathlen-1] == '/') {
			pathlen--;
		}
	}

	// read the file system
	FTS* file_system = NULL;
	FTSENT* node = NULL;

	file_system = fts_open((char* const[]){path, NULL}, FTS_LOGICAL, NULL);
	if (file_system == NULL) {
		perror("read_static_files");
		return NULL;
	}
	
	while ((node = fts_read(file_system)) != NULL) {
		// ignore dot (hidden) files unless it's the first directory and it's . or ..
		if(node->fts_name[0]=='.' && strcmp(node->fts_path, ".")!=0 && strcmp(node->fts_path, "..")!=0) {
			fts_set(file_system, node, FTS_SKIP);
		} else if (node->fts_info == FTS_F) {
			if (node->fts_pathlen > pathlen) {

				// a valid file, add it to the list
				struct static_file* new_file = malloc(sizeof(*new_file));
				new_file->next = NULL;
				tail->next = new_file;
				tail = new_file;

				// file paths
				new_file->local_path = malloc(node->fts_pathlen+1);
				strcpy(new_file->local_path, node->fts_path);
				new_file->server_path = new_file->local_path+pathlen;				
			}
		}
	}
	fts_close(file_system);

	// return the first file in the list
	return temp_head.next;
}

void free_static_files(struct static_file* file) {
	while (file != NULL) {
		struct static_file* next = file->next;
		free(file->local_path);
		free(file);
		file = next;
	}
}

char* get_file_path(struct static_file* file, char* server_path) {
	while (file != NULL) {
		if (strcmp(file->server_path, server_path)==0) {
			return file->local_path;
		}
		file = file->next;
	}
	return NULL;
}

// ================ Basic Network stuff ================
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
		fprintf(stderr, "gettaddrinfo error: %s\n", gai_strerror(status));
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
		fprintf(stderr, "bind error.\n");
		return -1;
	}

	freeaddrinfo(addresses);

	// listen to socket
	if (listen(sock, 10) != 0) {
		perror("listen");
		return -1;
	}

	return sock;
}

// ================ socket handelers ================
struct sockets_list;

typedef void (*socket_listener)(struct sockets_list*, int);

struct sockets_list {
	int size;
	int count;
	struct pollfd* pollfds;
	socket_listener* listeners;
	void** states;
};

struct sockets_list* sockets_list_new() {
	struct sockets_list* list;

	if ((list = malloc(sizeof(*list))) == NULL) {
		fprintf(stderr, "Unable to allocate memory for sockets list");
		return NULL;
	}

	list->size = 5;
	list->count = 0;
	list->pollfds = malloc(sizeof(*list->pollfds) * list->size);
	list->listeners = malloc(sizeof(*list->listeners) * list->size);
	list->states = malloc(sizeof(*list->states) * list->size);
	if (list->pollfds == NULL || list->listeners == NULL || list->states == NULL) {
		fprintf(stderr, "Unable to allocate memory for sockets list");
		return NULL;
	}
	return list;
}

int sockets_list_add(struct sockets_list* list, int new_socket, socket_listener new_listener) {
	if (list->count == list->size) {
		int new_size = list->size * 2;
		struct pollfd* new_pollfds = realloc(list->pollfds, sizeof(*list->pollfds) * new_size);
		socket_listener* new_listeners = realloc(list->listeners, sizeof(*list->listeners) * new_size);
		void* new_states = realloc(list->states, sizeof(*list->states) * new_size);
		if (new_pollfds == NULL || new_listeners == NULL || new_states == NULL) {
			fprintf(stderr, "Unable to allocate memory for sockets list");
			return -1;
		}

		list->size = new_size;
		list->pollfds = new_pollfds;
		list->listeners = new_listeners;
	}

	list->pollfds[list->count].fd = new_socket;
	list->pollfds[list->count].events = POLLIN;
	list->pollfds[list->count].revents = 0;
	list->listeners[list->count] = new_listener;
	list->states[list->count] = NULL;

	list->count++;

	return list->count-1;
}

void sockets_list_rm(struct sockets_list* list, int index) {
	if (index>=0 && index<list->count) {
		list->pollfds[index] = list->pollfds[list->count-1];
		list->listeners[index] = list->listeners[list->count-1];
		list->states[index] = list->states[list->count-1];
		list->count--;
	}
}

// ================ client code ================
struct client_state {
	char address[INET6_ADDRSTRLEN];
	int data_in_size;
	int data_in_length;
	char* data_in;
	int data_out_size;
	int data_out_length;
	char* data_out;
};

struct client_state* client_state_new() {
	struct client_state* state;

	if ((state = malloc(sizeof(*state))) == NULL) {
		fprintf(stderr, "Unable to allocate memory for client state");
		return NULL;
	}

	state->data_in_size = 256;
	state->data_in_length = 0;
	state->data_in = malloc(state->data_in_size);

	state->data_out_size = 256;
	state->data_out_length = 0;
	state->data_out = malloc(state->data_out_size);

	if (state->data_in == NULL || state->data_out == NULL) {
		fprintf(stderr, "Unable to allocate memory for client state");
		return NULL;
	}
	return state;
}
void client_state_free(struct client_state* state) {
	free(state->data_in);
	free(state->data_out);
	free(state);
}

#define IMF_DATE_LEN 29

char* imf_date(char* buf, size_t max_len) {
	time_t seconds = time(NULL);
	struct tm* gmt = gmtime(&seconds);
	strftime(buf, max_len, "%a, %d %b %Y %H:%M:%S GMT", gmt);
	return buf;
}

void send_all(int sock, char *buf, int len) {
	int total = 0;
	int todo = len;
	int sent;
	
	while (total < len) {
		sent = send(sock, buf+total, len-total, 0);
		if (sent == -1) {
			fprintf(stderr, "send error: %s", strerror(errno));
			return;
		} else {
			tprintf("sent %d\n", sent);
		}
		total += sent;
	}
}

void send_response(int sock, struct client_state* state, char *status_code, char *status_text, char* ext) {
	int size = 1024;
	int len = 0;
	char headers[size];
	
	len += snprintf(headers, size-len, "HTTP/1.1 %s %s\r\nDate: ", status_code, status_text);
	imf_date(headers+len, IMF_DATE_LEN+1);
	len += IMF_DATE_LEN;
	len += snprintf(headers+len, size-len, "\r\nServer: Stub\r\n");
	len += snprintf(headers+len, size-len, "Content-Type: ");
	if (ext == NULL) {
		len += snprintf(headers+len, size-len, "text/plain; charset=utf-8");
	} else {
		if (strcmp(ext, "html")==0 || strcmp(ext, "htm")==0) {
			len += snprintf(headers+len, size-len, "text/html; charset=utf-8");
		} else if (strcmp(ext, "css")==0) {
			len += snprintf(headers+len, size-len, "text/css; charset=utf-8");
		} else if (strcmp(ext, "js")==0) {
			len += snprintf(headers+len, size-len, "text/javascript; charset=utf-8");
		} else if (strcmp(ext, "jpeg")==0 || strcmp(ext, ".jpg")==0) {
			len += snprintf(headers+len, size-len, "image/jpeg");
		} else if (strcmp(ext, "png")==0) {
			len += snprintf(headers+len, size-len, "image/png");
		} else if (strcmp(ext, "gif")==0) {
			len += snprintf(headers+len, size-len, "image/gif");
		} else if (strcmp(ext, "bmp")==0) {
			len += snprintf(headers+len, size-len, "image/bmp");
		} else if (strcmp(ext, "svg")==0) {
			len += snprintf(headers+len, size-len, "image/svg+xml");
		} else if (strcmp(ext, "ico")==0) {
			len += snprintf(headers+len, size-len, "image/vnd.microsoft.icon");
		} else {
			len += snprintf(headers+len, size-len, "text/plain; charset=utf-8");
		}
	}
	len += snprintf(headers+len, size-len, "\r\nContent-Length: %ld\r\n\r\n", state->data_out_length);
	send_all(sock, headers, len);
	send_all(sock, state->data_out, state->data_out_length);
}

void send_simple_status(int sock, struct client_state* state, char *status_code, char *status_text, char *description) {
	snprintf(state->data_out, state->data_out_size, "<html><body><h1>%s - %s</h1><p>%s</p></body></html>", status_code, status_text, description);
	state->data_out_length = strlen(state->data_out);
	send_response(sock, state, status_code, status_text, "html");
}

int send_file(int sock, struct client_state* state, char* filename) {
	int new_size;
	char* new_data;
	FILE *f = fopen(filename, "rb");

	if (f) {
		fseek(f, 0, SEEK_END);
		new_size = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (new_size > state->data_out_size) {
			new_data = realloc(state->data_out, new_size);
			if (new_data == NULL) {
				fprintf(stderr, "error allocating output buffer");
				return -1;
			}
			state->data_out_size = new_size;
			state->data_out = new_data;
		}
		state->data_out_length = new_size;

		fread(state->data_out, 1, new_size, f);

		fclose(f);

		char *ext;
		ext = strrchr(filename, '.');
		if (strlen(ext) <= 1) {
			ext = ".txt"; // hack!
		}

		send_response(sock, state, "200", "OK", ext+1);
	}

	return 0;
}

void client_listener(struct sockets_list* sockets, int index) {
	int sock = sockets->pollfds[index].fd;
	struct client_state* state = sockets->states[index];
	
	int recvied = recv(sock, state->data_in+state->data_in_length, state->data_in_size-state->data_in_length, 0);
	tprintf("recvied %d\n", recvied);

	if (recvied <= 0) {
		if (recvied < 0) {
			fprintf(stderr, "recv error from %s (%d): %s\n", state->address, sock, strerror(errno));
		} else {
			tprintf("connection from %s (%d) closed\n", state->address, sock);
		}

		close(sock);
		client_state_free(state);
		sockets_list_rm(sockets, index);
	} else {
		// parse request
		state->data_in_length += recvied;

		int space_1 = -1;
		int space_2 = -1;
		int end = -1;
		for (int i=0; i<state->data_in_length; i++) {
			if (space_1<0) {
				if (state->data_in[i]==' ') {
					space_1 = i;
				}
			} else if (space_2<0) {
				if (state->data_in[i]==' ') {
					space_2 = i;
				}
			}
			// look for a blank line
			if (i>2 && state->data_in[i-3]=='\r' && state->data_in[i-2]=='\n' && state->data_in[i-1]=='\r' && state->data_in[i]=='\n') {
				end = i;
			}
		}

		if (end<0) {
			// make buffer bigger
			int new_size = state->data_in_size * 2;
			char* new_data = realloc(state->data_in, new_size);

			if (new_data == NULL) {
				fprintf(stderr, "unable to allocate data in buffer for %s (%d): %d\n", state->address, sock, new_size);

				close(sock);
				client_state_free(state);
				sockets_list_rm(sockets, index);
			} else {
				state->data_in_size = new_size;
				state->data_in = new_data;
			}
		} else {
			if (space_1>0 && space_2>space_1+1) {
				tprintf("method is %d long, needing %d space\n", space_1, space_1+1);
				char method[space_1+1];
				strncpy(method, state->data_in, space_1);
				method[space_1] = '\0';

				tprintf("path is %d long, needing %d space\n", space_2-space_1-1, space_2-space_1);
				char path[space_2-space_1+5];
				strncpy(path, state->data_in+space_1+1, space_2-space_1-1);
				path[space_2-space_1-1] = '\0';

				tprintf("\"%s\" \"%s\" from %s (%d)\n", method, path, state->address, sock);
				if (strcmp(method, "GET")==0) {
					char* file_path = get_file_path(files, path);
					if (file_path != NULL) {
						send_file(sock, state, file_path);
					} else {
						// special urls
						if (strcmp(path, "/")==0) {
							send_file(sock, state, get_file_path(files, "/content.html"));
						} else if (strcmp(path, "/blog")==0 || strcmp(path, "/blog/")==0) {
							send_file(sock, state, get_file_path(files, "/blog/all.html"));
						} else if (strncmp(path, "/blog/", 6)==0) {
							strcat(path, ".html");
							file_path = get_file_path(files, path);
							if (file_path != NULL) {
								send_file(sock, state, file_path);
							} else {
								send_simple_status(sock, state, "404", "Not Found", "Opps, that resource can not be found.");
							}
						} else {
							send_simple_status(sock, state, "404", "Not Found", "Opps, that resource can not be found.");
						}
					}
				} else {
					send_simple_status(sock, state, "501", "Not Implemented", "Opps, that functionality has not been implemented.");
				}
			} else {
				send_simple_status(sock, state, "400", "Bad Request", "Opps, that request made no sense.");
			}

			// done with this request to reset the length
			state->data_in_length = 0;
		}
	}
}

// ================ server code ================

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void server_listener(struct sockets_list* sockets, int index) {
	struct sockaddr_storage address;
	socklen_t address_size;
	int client_socket;
	struct client_state* client_state;
	int client_index;
	
	address_size = sizeof(address);
	if ((client_socket = accept(sockets->pollfds[index].fd, (struct sockaddr *)&address, &address_size)) < 0) {
		perror("accept");
	} else {
		if ((client_state = client_state_new()) == NULL) {
			close(client_socket);
			return;
		}

		if ((client_index = sockets_list_add(sockets, client_socket, client_listener)) < 0) {
			close(client_socket);
			client_state_free(client_state);
			return;
		}

		sockets->states[client_index] = client_state;
		inet_ntop(address.ss_family, get_in_addr((struct sockaddr *)&address), client_state->address, INET6_ADDRSTRLEN);

		tprintf("connection from %s (%d) opened\n", client_state->address, client_socket);
	}
}

// ================ Main loop etc ================
int main(int argc, char* argv[]) {
	// validate arguments
	if (argc != 3) {
		fprintf(stderr, "Usage: %s port root_dir\n", argv[0]);
		return EXIT_FAILURE;
	}

	// get static files
	files = find_static_files(argv[2]);
	
	tprintf("available files\n");
	for (struct static_file* file = files; file != NULL; file = file->next) {
		printf("  %s\n", file->server_path);
	}

	// create list of sockets
	struct sockets_list* sockets = sockets_list_new();
	if (sockets == NULL) {
		return EXIT_FAILURE;
	}

	// open server socket
	int server_socket = get_server_socket(argv[1]);
	if (server_socket < 0) {
		fprintf(stderr, "error getting server socket\n");
		return EXIT_FAILURE;
	}

	sockets_list_add(sockets, server_socket, server_listener);
	tprintf("waiting for connections\n");

	// loop for ever directing network traifc
	for (;;) {
		int poll_count = poll(sockets->pollfds, sockets->count, -1);
		
		if (poll_count == -1) {
			perror("poll");
			return EXIT_FAILURE;
		}

		for (int i = 0; i < sockets->count; i++) {
			if (sockets->pollfds[i].revents & POLLIN) {
				sockets->listeners[i](sockets, i);
			}
		}
	}

	// tidy up, but we should never get here?
	close(server_socket);
	free_static_files(files);
	
	return EXIT_SUCCESS;
}
