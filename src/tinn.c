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

#define IMF_DATE_LEN 30 // length of a date in Internet Messaging Format with null terminator

// ================ date/time stuff ================

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

// generate a date stamp in Internet Messaging Format
char* imf_date(char* buf, size_t max_len) {
	time_t seconds = time(NULL);
	struct tm* gmt = gmtime(&seconds);
	strftime(buf, max_len, "%a, %d %b %Y %H:%M:%S GMT", gmt);
	return buf;
}

// ================ Routes ================
#define RT_LOCAL 1
#define RT_REDIRECT 2

struct route {
	unsigned short type;
	char* from;
	char* to;
	struct route* next;
};
struct routes {
	struct route* head;
	struct route** tail_next;
};
struct routes* routes_new() {
	struct routes* list;
	if ((list = malloc(sizeof(*list))) != NULL) {
		list->head = NULL;
		list->tail_next = &list->head;
	}
	return list;
}
struct route* routes_add(struct routes* list, unsigned short type, char* from, char* to) {
	struct route* new_route;
	if ((new_route = malloc(sizeof(*new_route))) != NULL) {
		new_route->type = type;
		new_route->from = from;
		new_route->to = to;

		new_route->next = NULL;
		*list->tail_next = new_route;
		list->tail_next = &new_route->next;
	}
	return new_route;
}

struct route* routes_find(struct routes* list, char* from) {
	struct route* route = list->head;
	while (route != NULL) {
		if (strcmp(route->from, from)==0) {
			return route;
		}
		route = route->next;
	}
	return NULL;
}

struct routes* routes_build(char* path) {
	struct routes* list = routes_new();

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
				// a valid file, add route
				char* file_path = malloc(node->fts_pathlen+1);
				strcpy(file_path, node->fts_path);

				routes_add(list, RT_LOCAL, file_path+pathlen, file_path);

				// add directory?
				if (strcmp(node->fts_name, "index.html") == 0) {
					size_t len = node->fts_pathlen - pathlen - 10;
					char* dir_path = malloc(len+1);
					strncpy(dir_path, node->fts_path+pathlen, len);
					dir_path[len] = '\0';

					routes_add(list, RT_LOCAL, dir_path, file_path);

					if (len>1) {
						char* redirect_path = malloc(len);
						strncpy(redirect_path, node->fts_path+pathlen, len-1);
						redirect_path[len-1] = '\0';

						routes_add(list, RT_REDIRECT, redirect_path, dir_path);
					}
				} else if (strcmp(node->fts_name, "post.html") == 0) {
					size_t len = node->fts_pathlen - pathlen - 10;
					char* dir_path = malloc(len+1);
					strncpy(dir_path, node->fts_path+pathlen, len);
					dir_path[len] = '\0';

					routes_add(list, RT_LOCAL, dir_path, file_path);
				}
			}
		}
	}

	fts_close(file_system);

	// return the first file in the list
	return list;
}

// ================ basic network stuff ================
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

// ================ socket list ================
struct sockets_list;

typedef void (*socket_listener)(struct sockets_list* sockets, int index, struct routes* routes);

struct sockets_list {
	size_t  size;
	size_t  count;
	struct pollfd* pollfds;
	socket_listener* listeners;
	void** states;
};

struct sockets_list* sockets_list_new() {
	struct sockets_list* list;

	if ((list = malloc(sizeof(*list))) == NULL) {
		fprintf(stderr, "PANIC: Unable to allocate memory for sockets list");
		exit(EXIT_FAILURE);
	}

	list->size = 5;
	list->count = 0;

	list->pollfds = malloc(sizeof(*list->pollfds) * list->size);
	list->listeners = malloc(sizeof(*list->listeners) * list->size);
	list->states = malloc(sizeof(*list->states) * list->size);

	if (list->pollfds == NULL || list->listeners == NULL || list->states == NULL) {
		fprintf(stderr, "PANIC: Unable to allocate memory for sockets list");
		exit(EXIT_FAILURE);
	}

	return list;
}

int sockets_list_add(struct sockets_list* list, int new_socket, socket_listener new_listener) {
	// do we need to expand the arrays
	if (list->count == list->size) {
		list->size *= 2;

		list->pollfds = realloc(list->pollfds, sizeof(*list->pollfds) * list->size);
		list->listeners = realloc(list->listeners, sizeof(*list->listeners) * list->size);
		list->states = realloc(list->states, sizeof(*list->states) * list->size);

		if (list->pollfds == NULL || list->listeners == NULL || list->states == NULL) {
			fprintf(stderr, "PANIC: Unable to allocate memory for sockets list");
			exit(EXIT_FAILURE);
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

void sockets_list_rm(struct sockets_list* list, size_t index) {
	if (index < list->count) {
		list->pollfds[index] = list->pollfds[list->count-1];
		list->listeners[index] = list->listeners[list->count-1];
		list->states[index] = list->states[list->count-1];
		list->count--;
	}
}

// ================ buffer ================
struct buffer {
	size_t size;
	size_t length;
	char* data;
};

struct buffer* buf_new(size_t size) {
	struct buffer* buf;

	if ((buf = malloc(sizeof(*buf))) == NULL) {
		fprintf(stderr, "Unable to allocate memory for buffer");
		return NULL;
	}

	buf->size = size;
	buf->length = 0;
	if ((buf->data = malloc(buf->size)) == NULL) {
		fprintf(stderr, "Unable to allocate memory for buffer");
		free(buf);
		return NULL;
	}
	return buf;
}

void buf_free(struct buffer* buf) {
	free(buf->data);
	free(buf);
}

int buf_extend(struct buffer* buf, size_t new_size) {
	if (new_size > buf->size) {
		char* new_data = realloc(buf->data, new_size);
		if (new_data == NULL) {
			return -1;
		}

		buf->size = new_size;
		buf->data = new_data;
	}
	return 0;
}

int buf_ensure(struct buffer* buf, size_t n) {
	if (buf->length + n > buf->size) {
		int new_size = buf->size * 2;
		while (new_size < buf->length + n) {
			new_size *= 2;
		}
		if (buf_extend(buf, new_size) < 0) {
			return -1;
		}
	}
	return 0;
}

int buf_append(struct buffer* buf, char* data, size_t n) {
	if (buf_ensure(buf, n) < 0) {
		return -1;
	}

	memcpy(buf->data + buf->length, data, n);
	buf->length += n;
	return buf->length;
}

int buf_append_str(struct buffer* buf, char* str) {
	return buf_append(buf, str, strlen(str));	
}

int buf_append_format(struct buffer* buf, size_t max, char* format, ...) {
	if (buf_ensure(buf, max+1) < 0) {
		return -1;
	}

	va_list args;
	va_start(args, format);
	int added = vsnprintf(buf->data+buf->length, max+1, format, args);
	va_end(args);

	if (added < 0) {
		return -1;
	}
	buf->length += added;
	return buf->length;
}

char* buf_reserve(struct buffer* buf, size_t n) {
	if (buf_ensure(buf, n) < 0) {
		return NULL;
	}

	char* rv = buf->data + buf->length;
	buf->length += n;
	return rv;
}

void buf_seek(struct buffer* buf, int offset) {
	if (buf->length + offset < 0) {
		buf->length = 0;
	} else if (buf->length + offset > buf->size) {
		buf->length = buf->size;
	} else {
		buf-> length += offset;
	}
}

void buf_reset(struct buffer* buf) {
	buf->length = 0;
}

char* buf_as_str(struct buffer* buf) {
	if (buf->length == buf->size) {
		if (buf_extend(buf, buf->size + 1) < 0) {
			return NULL;
		}
	}
	buf->data[buf->length] = '\0';
	return buf->data;
}

// ================ client code ================
struct client_state {
	char address[INET6_ADDRSTRLEN];
	struct buffer* in;
};

struct client_state* client_state_new() {
	struct client_state* state;

	if ((state = malloc(sizeof(*state))) == NULL) {
		fprintf(stderr, "PANIC: Unable to allocate memory for client state");
		exit(EXIT_FAILURE);
	}

	if ((state->in = buf_new(256)) == NULL) {
		fprintf(stderr, "PANIC: Unable to allocate memory for client state");
		exit(EXIT_FAILURE);
	}
	return state;
}
void client_state_free(struct client_state* state) {
	buf_free(state->in);
	free(state);
}

void send_all(int sock, char *buf, size_t len) {
	size_t total = 0;
	int sent;
	
	while (total < len) {
		sent = send(sock, buf+total, len-total, 0);
		if (sent == -1) {
			fprintf(stderr, "send error: %s", strerror(errno));
			return;
		}
		total += sent;
	}
}

void send_redirect(int sock, struct client_state* state, char *location) {
	struct buffer* header = buf_new(128);

	// status line
	buf_append_str(header, "HTTP/1.1 301 Moved Permanently\r\n");

	// date
	buf_append_str(header, "Date: ");
	imf_date(buf_reserve(header, IMF_DATE_LEN), IMF_DATE_LEN);
	buf_seek(header, -1);
	buf_append_str(header, "\r\n");

	// server
	buf_append_str(header, "Server: Tinn\r\n");

	// location
	buf_append_str(header, "Location: ");
	buf_append_str(header, location);	
	buf_append_str(header, "\r\n");

	// end header
	buf_append_str(header, "\r\n");

	// send it
	send_all(sock, header->data, header->length);
	buf_free(header);
}

void send_header(int sock, struct client_state* state, char *status_code, char *status_text, char* ext, size_t length) {
	struct buffer* header = buf_new(128);

	// status line
	buf_append_format(header, 12 + strlen(status_code) + strlen(status_text), "HTTP/1.1 %s %s\r\n", status_code, status_text);

	// date
	buf_append_str(header, "Date: ");
	imf_date(buf_reserve(header, IMF_DATE_LEN), IMF_DATE_LEN);
	buf_seek(header, -1);
	buf_append_str(header, "\r\n");

	// server
	buf_append_str(header, "Server: Tinn\r\n");

	// content type
	buf_append_str(header, "Content-Type: ");
	if (ext == NULL) {
		buf_append_str(header, "text/plain; charset=utf-8");
	} else {
		if (strcmp(ext, "html")==0 || strcmp(ext, "htm")==0) {
			buf_append_str(header, "text/html; charset=utf-8");
		} else if (strcmp(ext, "css")==0) {
			buf_append_str(header, "text/css; charset=utf-8");
		} else if (strcmp(ext, "js")==0) {
			buf_append_str(header, "text/javascript; charset=utf-8");
		} else if (strcmp(ext, "jpeg")==0 || strcmp(ext, "jpg")==0) {
			buf_append_str(header, "image/jpeg");
		} else if (strcmp(ext, "png")==0) {
			buf_append_str(header, "image/png");
		} else if (strcmp(ext, "gif")==0) {
			buf_append_str(header, "image/gif");
		} else if (strcmp(ext, "bmp")==0) {
			buf_append_str(header, "image/bmp");
		} else if (strcmp(ext, "svg")==0) {
			buf_append_str(header, "image/svg+xml");
		} else if (strcmp(ext, "ico")==0) {
			buf_append_str(header, "image/vnd.microsoft.icon");
		} else {
			buf_append_str(header, "text/plain; charset=utf-8");
		}
	}
	buf_append_str(header, "\r\n");

	// content length
	buf_append_format(header, 30, "Content-Length: %ld\r\n", length);

	// end header
	buf_append_str(header, "\r\n");

	// send it
	send_all(sock, header->data, header->length);
	buf_free(header);
}

void send_simple_status(int sock, struct client_state* state, char *status_code, char *status_text, char *description) {
	struct buffer* body = buf_new(256);

	buf_append_str(body, "<html><body><h1>");
	buf_append_str(body, status_code);
	buf_append_str(body, " - ");
	buf_append_str(body, status_text);
	buf_append_str(body, "</h1><p>");
	buf_append_str(body, description);
	buf_append_str(body, "</p></body></html>");

	send_header(sock, state, status_code, status_text, "html", body->length);
	send_all(sock, body->data, body->length);
	buf_free(body);
}

int send_file(int sock, struct client_state* state, char* path) {
	size_t length;
	FILE *file = fopen(path, "rb");
	
	if (file == NULL) {
		return -1;
	}

	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);

	struct buffer* body = buf_new(length);
	if (body == NULL) {
		fclose(file);
		return -1;
	}

	fread(buf_reserve(body, length), 1, length, file);
	fclose(file);

	char *ext;
	ext = strrchr(path, '.');
	if (strlen(ext) <= 1) {
		ext = ".txt"; // hack!
	}

	send_header(sock, state, "200", "OK", ext+1, length);
	send_all(sock, body->data, length);
	buf_free(body);
}

void client_listener(struct sockets_list* sockets, int index, struct routes* routes) {
	int sock = sockets->pollfds[index].fd;
	struct client_state* state = sockets->states[index];
	
	int recvied = recv(sock, state->in->data + state->in->length, state->in->size - state->in->length, 0);
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
		// update buffer
		buf_seek(state->in, recvied);

		// parse request
		int space_1 = -1;
		int space_2 = -1;
		int end = -1;
		for (int i=0; i<state->in->length; i++) {
			if (space_1<0) {
				if (state->in->data[i]==' ') {
					space_1 = i;
				}
			} else if (space_2<0) {
				if (state->in->data[i]==' ') {
					space_2 = i;
				}
			}
			// look for a blank line
			if (i>2 && state->in->data[i-3]=='\r' && state->in->data[i-2]=='\n' && state->in->data[i-1]=='\r' && state->in->data[i]=='\n') {
				end = i;
			}
		}

		if (end<0) {
			// make buffer bigger
			buf_extend(state->in, state->in->size * 2);
		} else {
			if (space_1>0 && space_2>space_1+1) {
				char method[space_1+1];
				memcpy(method, state->in->data, space_1);
				method[space_1] = '\0';

				char path[space_2-space_1+10]; // extra 10 is for index.html
				memcpy(path, state->in->data+space_1+1, space_2-space_1-1);
				path[space_2-space_1-1] = '\0';

				char* anchor = strchr(path, '#');
				if (anchor != NULL) {
					*(anchor) = '\0';
					anchor++;
				}

				char* query = strchr(path, '?');
				if (query != NULL) {
					*(query) = '\0';
					query++;
				}

				tprintf("\"%s\" \"%s\" from %s (%d)\n", method, path, state->address, sock);
				if (strcmp(method, "GET")==0) {
					struct route* route = routes_find(routes, path);
					if (route == NULL) {
						send_simple_status(sock, state, "404", "Not Found", "Opps, that resource can not be found.");
					} else if (route->type == RT_LOCAL) {
						send_file(sock, state, route->to);
					} else if (route->type == RT_REDIRECT) {
						send_redirect(sock, state, route->to);
					} else {
						send_simple_status(sock, state, "500", "Internal Server Error", "Opps, something went wrong that is probably not your fault, probably.");
					}
				} else {
					send_simple_status(sock, state, "501", "Not Implemented", "Opps, that functionality has not been implemented.");
				}
			} else {
				send_simple_status(sock, state, "400", "Bad Request", "Opps, that request made no sense.");
			}

			// done with this request to reset
			buf_reset(state->in);
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

void server_listener(struct sockets_list* sockets, int index, struct routes* routes) {
	struct sockaddr_storage address;
	socklen_t address_size;
	int client_socket;
	struct client_state* client_state;
	int client_index;
	
	address_size = sizeof(address);
	if ((client_socket = accept(sockets->pollfds[index].fd, (struct sockaddr *)&address, &address_size)) < 0) {
		perror("accept");
	} else {
		client_state = client_state_new();
		client_index = sockets_list_add(sockets, client_socket, client_listener);

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

	// build routes
	struct routes* routes = routes_build(argv[2]);

	/*struct route* route = routes->head;
	while (route != NULL) {
		printf("\"%s\" %s \"%s\"\n", route->from, route->type==RT_LOCAL ? "->" : "rd", route->to);
		route = route->next;
	}*/

	// create list of sockets
	struct sockets_list* sockets = sockets_list_new();
	
	// open server socket
	int server_socket = get_server_socket(argv[1]);
	if (server_socket < 0) {
		fprintf(stderr, "error getting server socket\n");
		return EXIT_FAILURE;
	}

	sockets_list_add(sockets, server_socket, server_listener);
	tprintf("waiting for connections\n");

	// loop forever directing network traffic
	for (;;) {
		if (poll(sockets->pollfds, sockets->count, -1) < 0 ) {
			perror("poll");
			return EXIT_FAILURE;
		}

		for (int i = 0; i < sockets->count; i++) {
			if (sockets->pollfds[i].revents & POLLIN) {
				sockets->listeners[i](sockets, i, routes);
			}
		}
	}

	// tidy up, but we should never get here?
	close(server_socket);
	//routes_free(routes);
	
	return EXIT_SUCCESS;
}
