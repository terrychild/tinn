#include <stdlib.h>
#include <stdbool.h>
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
#define RT_FILE 1
#define RT_REDIRECT 2
#define RT_BUFFER 3

struct route {
	unsigned short type;
	char* from;
	void* to;
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
struct route* routes_add(struct routes* list, unsigned short type, char* from, void* to) {
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

void routes_add_static(struct routes* list) {

	// read the file system
	FTS* file_system = NULL;
	FTSENT* node = NULL;

	file_system = fts_open((char* const[]){".", NULL}, FTS_LOGICAL, NULL);
	if (file_system == NULL) {
		perror("routes_build");
		return;
	}
	
	while ((node = fts_read(file_system)) != NULL) {
		// ignore dot (hidden) files unless it's the . dir aka the current dir
		if(node->fts_name[0]=='.' && strcmp(node->fts_path, ".")!=0) {
			fts_set(file_system, node, FTS_SKIP);

		} else if (node->fts_info == FTS_F) {
			// a valid file, add route
			char* file_path = malloc(node->fts_pathlen+1);
			strcpy(file_path, node->fts_path);

			routes_add(list, RT_FILE, file_path+1, file_path);

			// add directory?
			if (strcmp(node->fts_name, "index.html") == 0) {
				size_t len = node->fts_pathlen - 10;
				char* dir_path = malloc(len+1);
				strncpy(dir_path, node->fts_path, len);
				dir_path[len] = '\0';

				routes_add(list, RT_FILE, dir_path+1, file_path);

				if (len>2) {
					char* redirect_path = malloc(len);
					strncpy(redirect_path, node->fts_path, len-1);
					redirect_path[len-1] = '\0';

					routes_add(list, RT_REDIRECT, redirect_path+1, dir_path);
				}
			}
		}
	}

	fts_close(file_system);
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
	long size;
	long length;
	long read_pos;
	char* data;
};

struct buffer* buf_new(long size) {
	struct buffer* buf;

	if ((buf = malloc(sizeof(*buf))) == NULL) {
		fprintf(stderr, "Unable to allocate memory for buffer");
		return NULL;
	}

	buf->size = size;
	buf->length = 0;
	buf->read_pos = 0;
	if ((buf->data = malloc(buf->size)) == NULL) {
		fprintf(stderr, "Unable to allocate memory for buffer");
		free(buf);
		return NULL;
	}
	return buf;
}

void buf_free(struct buffer* buf) {
	if (buf != NULL) {
		free(buf->data);
		free(buf);
	}
}

bool buf_extend(struct buffer* buf, long new_size) {
	if (new_size > buf->size) {
		char* new_data = realloc(buf->data, new_size);
		if (new_data == NULL) {
			return false;
		}

		buf->size = new_size;
		buf->data = new_data;
	}
	return true;
}

bool buf_ensure(struct buffer* buf, long n) {
	if (buf->length + n > buf->size) {
		int new_size = buf->size * 2 || 1;
		while (new_size < buf->length + n) {
			new_size *= 2;
		}
		if (buf_extend(buf, new_size) < 0) {
			return false;
		}
	}
	return true;
}

char* buf_write_ptr(struct buffer* buf) {
	return buf->data + buf->length;
}
long buf_write_max(struct buffer* buf) {
	return buf->size - buf->length;
}
long buf_mod_len(struct buffer* buf, long offset) {
	if (buf->length + offset < 0) {
		buf->length = 0;
	} else if (buf->length + offset > buf->size) {
		buf->length = buf->size;
	} else {
		buf-> length += offset;
	}
	return buf->length;
}

char* buf_read_ptr(struct buffer* buf) {
	return buf->data + buf->read_pos;
}
long buf_read_max(struct buffer* buf) {
	return buf->length - buf->read_pos;
}
long buf_seek(struct buffer* buf, long offset) {
	if (buf->read_pos + offset < 0) {
		buf->read_pos = 0;
	} else if (buf->read_pos + offset > buf->length) {
		buf->read_pos = buf->length;
	} else {
		buf-> read_pos += offset;
	}
	return buf->read_pos;
}

long buf_append(struct buffer* buf, char* data, long n) {
	if (!buf_ensure(buf, n)) {
		return -1;
	}

	memcpy(buf->data + buf->length, data, n);
	buf->length += n;
	return buf->length;
}
long buf_append_str(struct buffer* buf, char* str) {
	return buf_append(buf, str, strlen(str));	
}
long buf_append_format(struct buffer* buf, char* format, ...) {
	long max = buf_write_max(buf);

	va_list args;
	va_start(args, format);
	long len = vsnprintf(buf_write_ptr(buf), max, format, args);
	va_end(args);

	if (len >= max) {
		if (!buf_ensure(buf, len+1)) {
			return -1;
		}

		va_start(args, format);
		len = vsnprintf(buf_write_ptr(buf), len+1, format, args);
		va_end(args);
	}

	if (len < 0) {
		return -1;
	}
	buf->length += len;

	return buf->length;
}
long buf_append_buf(struct buffer* target, struct buffer* source) {
	long len = source->length;
	if (!buf_ensure(target, len)) {
		return -1;
	}

	memcpy(target->data + target->length, source->data, len);
	target->length += len;
	return target->length;
}

char* buf_reserve(struct buffer* buf, long n) {
	if (buf_ensure(buf, n) < 0) {
		return NULL;
	}

	char* rv = buf->data + buf->length;
	buf->length += n;
	return rv;
}
void buf_consume(struct buffer* buf, long n) {
	long new_len = buf->length - n;
	if (new_len > 0) {
		memmove(buf->data, buf->data+n, new_len);
		buf->length = new_len;
		buf_seek(buf, -n);
	} else {
		buf->length = 0;
		buf->read_pos = 0;	
	}
}
void buf_reset(struct buffer* buf) {
	buf->length = 0;
	buf->read_pos = 0;
}

char* buf_as_str(struct buffer* buf) {
	if (buf->length == buf->size) {
		if (!buf_extend(buf, buf->size + 1)) {
			return NULL;
		}
	}
	buf->data[buf->length] = '\0';
	return buf->data;
}

// ================ client code ================
#define CLIENT_READ 1;
#define CLIENT_WRITE 2;

struct client_state {
	char address[INET6_ADDRSTRLEN];
	unsigned short mode;
	struct buffer* in;
	struct buffer* out;
};

struct client_state* client_state_new() {
	struct client_state* state;

	if ((state = malloc(sizeof(*state))) == NULL) {
		fprintf(stderr, "PANIC: Unable to allocate memory for client state");
		exit(EXIT_FAILURE);
	}

	if ((state->in = buf_new(1024)) == NULL) {
		fprintf(stderr, "PANIC: Unable to allocate memory for client state");
		exit(EXIT_FAILURE);
	}

	if ((state->out = buf_new(1024)) == NULL) {
		fprintf(stderr, "PANIC: Unable to allocate memory for client state");
		exit(EXIT_FAILURE);
	}

	return state;
}
void client_state_free(struct client_state* state) {
	buf_free(state->in);
	buf_free(state->out);
	free(state);
}

void buf_start_headers(struct buffer* buf, char *status_code, char *status_text) {
	// status line
	buf_append_format(buf, "HTTP/1.1 %s %s\r\n", status_code, status_text);

	// date
	buf_append_str(buf, "Date: ");
	imf_date(buf_reserve(buf, IMF_DATE_LEN), IMF_DATE_LEN);
	buf_mod_len(buf, -1);
	buf_append_str(buf, "\r\n");

	// server
	buf_append_str(buf, "Server: Tinn\r\n");
}

void buf_end_headers(struct buffer* buf) {
	buf_append_str(buf, "\r\n");
}

void buf_append_content_type(struct buffer* buf, char* ext) {
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

void buf_append_content_length(struct buffer* buf, long length) {
	buf_append_format(buf, "Content-Length: %ld\r\n", length);
}

void client_prep_simple_status(struct client_state* state, char *status_code, char *status_text, char *description) {
	struct buffer* body = buf_new(256);

	buf_append_str(body, "<html><body><h1>");
	buf_append_str(body, status_code);
	buf_append_str(body, " - ");
	buf_append_str(body, status_text);
	buf_append_str(body, "</h1><p>");
	buf_append_str(body, description);
	buf_append_str(body, "</p></body></html>");

	// headers
	buf_start_headers(state->out, status_code, status_text);

	buf_append_content_type(state->out, "html");
	buf_append_content_length(state->out, body->length);

	buf_end_headers(state->out);

	// content
	buf_append_buf(state->out, body);
}

void client_prep_redirect(struct client_state* state, char* location) {
	buf_start_headers(state->out, "301", "Moved Permanently");

	buf_append_str(state->out, "Location: ");
	buf_append_str(state->out, location);	
	buf_append_str(state->out, "\r\n");

	buf_end_headers(state->out);
}

void client_prep_file(struct client_state* state, char* path) {
	// open file and get content length
	long length;
	FILE *file = fopen(path, "rb");
	
	if (file == NULL) {
		return;
	}

	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);

	// populate header
	buf_start_headers(state->out, "200", "OK");

	buf_append_content_type(state->out, strrchr(path, '.'));
	buf_append_content_length(state->out, length);

	buf_end_headers(state->out);

	// populate content
	char* body = buf_reserve(state->out, length);
	if (body != NULL) {
		fread(body, 1, length, file);
	}
	fclose(file);
}

void client_prep_buffer(struct client_state* state, struct buffer* buffer) {
	// populate header
	buf_start_headers(state->out, "200", "OK");

	buf_append_content_type(state->out, "html");
	buf_append_content_length(state->out, buffer->length);

	buf_end_headers(state->out);

	// populate content
	buf_append_buf(state->out, buffer);
}

int client_write(struct pollfd* pfd, struct client_state* state) {
	long len = buf_read_max(state->out);
	long sent = send(pfd->fd, buf_read_ptr(state->out), len, MSG_DONTWAIT);
	//LOG:tprintf("sent: %ld/%ld\n", sent, len);
	if (sent < 0) {
		fprintf(stderr, "send error from %s (%d): %s\n", state->address, pfd->fd, strerror(errno));
		return -1;
	}
	if (sent < len) {
		buf_seek(state->out, sent);
		pfd->events = POLLOUT;
	} else {
		buf_reset(state->out);
		pfd->events = POLLIN;
	}
	return sent;
}

int client_read(struct pollfd* pfd, struct client_state* state, struct routes* routes) {
	int recvied = recv(pfd->fd, buf_write_ptr(state->in), buf_write_max(state->in), 0);
	if (recvied <= 0) {
		if (recvied < 0) {
			fprintf(stderr, "recv error from %s (%d): %s\n", state->address, pfd->fd, strerror(errno));
		} else {
			tprintf("connection from %s (%d) closed\n", state->address, pfd->fd);
		}
		return -1;
	} else {
		// update buffer
		buf_mod_len(state->in, recvied);

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
			return 0;
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

				tprintf("\"%s\" \"%s\" from %s (%d)\n", method, path, state->address, pfd->fd);
				if (strcmp(method, "GET")==0) {
					struct route* route = routes_find(routes, path);
					if (route == NULL) {
						client_prep_simple_status(state, "404", "Not Found", "Opps, that resource can not be found.");
					} else if (route->type == RT_FILE) {
						client_prep_file(state, route->to);
					} else if (route->type == RT_REDIRECT) {
						client_prep_redirect(state, route->to);
					} else if (route->type == RT_BUFFER) {
						client_prep_buffer(state, route->to);
					} else {
						client_prep_simple_status(state, "500", "Internal Server Error", "Opps, something went wrong that is probably not your fault, probably.");
					}
				} else {
					client_prep_simple_status(state, "501", "Not Implemented", "Opps, that functionality has not been implemented.");
				}
			} else {
				tprintf("Bad request from %s (%d)\n", state->address, pfd->fd);
				puts(buf_as_str(state->in));
				client_prep_simple_status(state, "400", "Bad Request", "Opps, that request made no sense.");
			}

			// done reading request so reset buffer
			buf_consume(state->in, end+1);

			// send
			return client_write(pfd, state);
			/*pfd->events &= POLLOUT;
			return 0;*/
		}
	}
}

void client_listener(struct sockets_list* sockets, int index, struct routes* routes) {
	struct pollfd* pfd = &sockets->pollfds[index];
	struct client_state* state = sockets->states[index];

	int flag = 0;
	if (pfd->revents & POLLHUP) {
		tprintf("connection from %s (%d) hung up\n", state->address, pfd->fd);
		flag = -1;
	} else if (pfd->revents & (POLLERR | POLLNVAL)) {
		fprintf(stderr, "Socket error from %s (%d): %d\n", state->address, pfd->fd, pfd->revents);
		flag = -1;
	} else {
		if (pfd->revents & POLLIN) {
			flag = client_read(pfd, state, routes);
		} else if (pfd->revents & POLLOUT) {
			flag = client_write(pfd, state);
		}
	}

	if (flag < 0) {
		close(pfd->fd);
		client_state_free(state);
		sockets_list_rm(sockets, index);
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

	if (sockets->pollfds[index].revents & (POLLERR | POLLHUP | POLLNVAL)) {
		tprintf("PANIC: error on server socket: %d\n", sockets->pollfds[index].revents);
		exit(EXIT_FAILURE);
	} 
	
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

// ================ blog code ================
long buf_append_file(struct buffer* buf, char* path) {
	FILE *file = fopen(path, "rb");	
	if (file == NULL) {
		return -1;
	}

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buf_ptr = buf_reserve(buf, length);
	if (buf_ptr == NULL) {
		return -1;
	}

	fread(buf_ptr, 1, length, file);
	fclose(file);

	return buf->length;
}

struct buffer* buf_new_file(char* path) {
	struct buffer* buf = buf_new(0);
	if (buf != NULL) {
		if (buf_append_file(buf, path) < 0) {
			fprintf(stderr, "Error reading %s\n", path);
			return NULL;
		}
	}
	return buf;
}

#define MAX_PATH_LEN 256

struct post {
	char path[MAX_PATH_LEN];
	char server_path[MAX_PATH_LEN];
	char title[MAX_PATH_LEN];
	char date[20];
	struct buffer* content;
};

struct posts_list {
	size_t size;
	size_t count;
	struct post* data;
};

struct posts_list* posts_list_new() {
	struct posts_list* list;
	if ((list = malloc(sizeof(*list))) != NULL) {
		list->size = 32;
		list->count = 0;
		list->data = malloc(sizeof(*list->data) * list->size);
	}
	return list;
}
void posts_list_free(struct posts_list* list) {
	if (list != NULL) {
		for (int i=0; i<list->count; i++) {
			buf_free(list->data[i].content);
		}
		free(list->data);
		free(list);
	}
}
bool posts_list_extend(struct posts_list* list, size_t new_size) {
	struct post* new_data = realloc(list->data, sizeof(*list->data) * new_size);
	if (new_data != NULL) {
		list->size = new_size;
		list->data = new_data;
		return true;
	}
	return false;
}
struct post* posts_list_draft(struct posts_list* list) {
	if (list->count == list->size) {
		if (!posts_list_extend(list, list->size * 2)) {
			return NULL;
		}
	}
	return &(list->data[list->count]);
}
void posts_list_commit(struct posts_list* list) {
	if (list->count < list->size) {
		list->count++;
	}
}

bool blog_read_posts(struct posts_list* posts, const char* blog_dir) {
	char path[MAX_PATH_LEN];
	snprintf(path, MAX_PATH_LEN, "%s/.posts.txt", blog_dir);

	FILE* data = fopen(path, "r");
	if (data == NULL) {
		fprintf(stderr, "Error reading %s\n", path);
		return false;
	}

	for (;;) {
		struct post* post = posts_list_draft(posts);
		if (post == NULL) {
			fprintf(stderr, "Unable to get next posts");
			fclose(data);
			return false;
		}
		if (fscanf(data, "%255[^\t]%*[\t]%2556[^\t]%*[\t]%19[^\n]%*[\n]", post->path, post->title, post->date) != 3) { //TODO better parsing and no hardcoding?
			// done reading file?
			break;
		} else {
			int len = snprintf(path, MAX_PATH_LEN, "%s/%s/.post.html", blog_dir, post->path);
			if (len < 0 || len >= MAX_PATH_LEN) {
				fprintf(stderr, "Error, unable to create path for \"%s\"\n", post->path);
				continue;
			}
			snprintf(post->server_path, MAX_PATH_LEN, "/%s/%s", blog_dir, post->path);

			post->content = buf_new_file(path);
			if (post->content == NULL) {
				fprintf(stderr, "Error reading %s\n", path);
				continue;
			}

			posts_list_commit(posts);
		}
	}

	fclose(data);
	return true;
}

bool blog_build(struct routes* routes) {
	bool ok = true;
	const char blog_dir[] = "blog";
			
	// load html fragments
	struct buffer* header1 = buf_new_file(".header1.html");
	struct buffer* header2 = buf_new_file(".header2.html");
	struct buffer* footer = buf_new_file(".footer.html");

	ok = header1 != NULL && header2 != NULL && footer != NULL;

	// read blog list
	struct posts_list* posts = NULL;
	if (ok) {
		posts = posts_list_new();
		ok = posts != NULL && blog_read_posts(posts, blog_dir);
	}
	
	// build pages
	if (ok) {
		// build blog pages
		for (int i=0; i<posts->count; i++) {
			struct buffer* post = buf_new(10240);
			buf_append_buf(post, header1);
			buf_append_format(post, " - %s", posts->data[i].title);
			buf_append_buf(post, header2);
			buf_append_format(post, "<article><h1>%s</h1><h2>%s</h2>\n", posts->data[i].title, posts->data[i].date);
			buf_append_buf(post, posts->data[i].content);
			buf_append_str(post, "<nav>");
			if (i < posts->count-1) {
				buf_append_format(post, "<a href=\"%s\">prev</a>", posts->data[i+1].path);
			} else {
				buf_append_str(post, "<span>&nbsp;</span>");
			}
			if (i > 0) {
				buf_append_format(post, "<a href=\"%s\">next</a>", posts->data[i-1].path);
			}
			buf_append_str(post, "</nav></article>\n");
			buf_append_buf(post, footer);
			
			routes_add(routes, RT_BUFFER, posts->data[i].server_path, post);
		}

		// build archive page
		struct buffer* archive = buf_new(10240);
		buf_append_buf(archive, header1);
		buf_append_str(archive, " - Blog");
		buf_append_buf(archive, header2);
		buf_append_str(archive, "<h1>Blog Archive</h1>\n");
		buf_append_str(archive, "<p>If you, like me, sometimes want to read an entire blog in chronological order without any unnecessary navigating and/or scrolling back and forth, you can do that <a href=\"/log\">here</a>.</p>\n");

		char archive_date[15] = "";

		for (int i=0; i<posts->count; i++) {
			if (strcmp(archive_date, strchr(posts->data[i].date, ' ')+1) != 0) {
				strcpy(archive_date, strchr(posts->data[i].date, ' ')+1);

				buf_append_format(archive, "<hr>\n<h3>%s</h3>\n", archive_date);
			}
			buf_append_format(archive, "<p><a href=\"%s\">%s</a></p>\n", posts->data[i].server_path, posts->data[i].title);
		}

		buf_append_buf(archive, footer);
		routes_add(routes, RT_BUFFER, "/blog", archive); //TODO remove hardcoded path

		// build home page
		struct buffer* home = buf_new(10240);
		buf_append_buf(home, header1);
		buf_append_buf(home, header2);

		for (int i=0; i<posts->count; i++) {
			if (i > 0) {
				buf_append_str(home, "<hr>\n");
			}
			buf_append_str(home, "<article>");
			buf_append_format(home, "<h1><a href=\"%s\">%s</a></h1>", posts->data[i].server_path, posts->data[i].title);
			buf_append_format(home, "<h2>%s</h2>", posts->data[i].date);
			buf_append_buf(home, posts->data[i].content);
			buf_append_str(home, "</article>\n");
		}

		buf_append_buf(home, footer);
		routes_add(routes, RT_BUFFER, "/", home);

		// build log page
		struct buffer* log = buf_new(10240);
		buf_append_buf(log, header1);
		buf_append_buf(log, header2);

		for (int i=posts->count-1; i>=0; i--) {
			if (i < posts->count-1) {
				buf_append_str(log, "<hr>\n");
			}
			buf_append_str(log, "<article>");
			buf_append_format(log, "<h1><a href=\"%s\">%s</a></h1>", posts->data[i].server_path, posts->data[i].title);
			buf_append_format(log, "<h2>%s</h2>", posts->data[i].date);
			buf_append_buf(log, posts->data[i].content);
			buf_append_str(log, "</article>\n");
		}

		buf_append_buf(log, footer);
		routes_add(routes, RT_BUFFER, "/log", log); // TODO remove hardcoded path?
	}

	// clean up
	//posts_list_free(posts); // There is a problem with how routes store the paths that means I can't free this...yet
	buf_free(header1);
	buf_free(header2);
	buf_free(footer);

	// success?
	return ok;
}

// ================ Main loop etc ================
int main(int argc, char* argv[]) {
	// validate arguments
	if (argc != 3) {
		fprintf(stderr, "Usage: %s port content_directory\n", argv[0]);
		return EXIT_FAILURE;
	}

	// change working directory to content directory
	if (chdir(argv[2]) != 0) {
		perror("content_directory");
		return EXIT_FAILURE;
	}

	// build routes
	struct routes* routes = routes_new();
	routes_add_static(routes);
	
	if (!blog_build(routes)) {
		fprintf(stderr, "Error loading blog\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	/*struct route* route = routes->head;
	while (route != NULL) {

		switch(route->type) {
			case RT_FILE: 
				printf("\"%s\" -> \"%s\"\n", route->from, route->to);
				break;
			case RT_REDIRECT: 
				printf("\"%s\" RD \"%s\"\n", route->from, route->to);
				break;
			case RT_BUFFER: 
				printf("\"%s\" buffer %d\n", route->from, ((struct buffer*)route->to)->length);
				break;
			default:
				printf("\"%s\" ??\n", route->from);
				break;
		}
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
			short revents = sockets->pollfds[i].revents;
			if (sockets->pollfds[i].revents) {
				sockets->listeners[i](sockets, i, routes);
			}
		}
	}

	// tidy up, but we should never get here?
	close(server_socket);
	//routes_free(routes);
	
	return EXIT_SUCCESS;
}
