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
		perror("routes_build");
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

				routes_add(list, RT_FILE, file_path+pathlen, file_path);

				// add directory?
				if (strcmp(node->fts_name, "index.html") == 0) {
					size_t len = node->fts_pathlen - pathlen - 10;
					char* dir_path = malloc(len+1);
					strncpy(dir_path, node->fts_path+pathlen, len);
					dir_path[len] = '\0';

					routes_add(list, RT_FILE, dir_path, file_path);

					if (len>1) {
						char* redirect_path = malloc(len);
						strncpy(redirect_path, node->fts_path+pathlen, len-1);
						redirect_path[len-1] = '\0';

						routes_add(list, RT_REDIRECT, redirect_path, dir_path);
					}
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
	free(buf->data);
	free(buf);
}

int buf_extend(struct buffer* buf, long new_size) {
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

int buf_ensure(struct buffer* buf, long n) {
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

int buf_append(struct buffer* buf, char* data, long n) {
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

int buf_append_format(struct buffer* buf, long max, char* format, ...) {
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
int buf_append_buf(struct buffer* target, struct buffer* source) {
	long len = source->length;
	if (buf_ensure(target, len) < 0) {
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
		if (buf_extend(buf, buf->size + 1) < 0) {
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
	buf_append_format(buf, 12 + strlen(status_code) + strlen(status_text), "HTTP/1.1 %s %s\r\n", status_code, status_text);

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
	buf_append_format(buf, 30, "Content-Length: %ld\r\n", length);
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

	return length;
}

void blog_build(char* base_path, struct routes* routes) {
	const size_t max_len = 256;
	const char blog_dir[] = "/blog/";
	const char blog_file[] = "/.post.html";

	// create arrays for paths
	size_t bpl = strlen(base_path);
	size_t spl = sizeof(blog_dir)-1;
	if (base_path[bpl-1] == '/') {
		bpl--;
	}	
	char path[bpl + spl + max_len + sizeof(blog_file)];
	strcpy(path, base_path);
	strcpy(path + bpl, blog_dir);
	bpl += spl;
			
	// load header part 1
	strcpy(path+bpl, ".header1.html");
	struct buffer* header1 = buf_new(256);
	if (buf_append_file(header1, path) < 0) {
		fprintf(stderr, "Error reading %s\n", path);
		return;
	}

	// load header part 2
	strcpy(path+bpl, ".header2.html");
	struct buffer* header2 = buf_new(256);
	if (buf_append_file(header2, path) < 0) {
		fprintf(stderr, "Error reading %s\n", path);
		return;
	}

	// load footer
	strcpy(path+bpl, ".footer.html");
	struct buffer* footer = buf_new(256);
	if (buf_append_file(footer, path) < 0) {
		fprintf(stderr, "Error reading %s\n", path);
		return;
	}

	// start index
	struct buffer* index = buf_new(10240);
	buf_append_buf(index, header1);
	buf_append_buf(index, header2);

	// start archive
	struct buffer* archive = buf_new(10240);
	buf_append_buf(archive, header1);
	buf_append_str(archive, " - Blog");
	buf_append_buf(archive, header2);
	buf_append_str(archive, "<h1>Blog Archive</h1>\n");

	// read blog list
	strcpy(path+bpl, ".posts.txt");
	FILE *posts = fopen(path, "r");	
	if (posts == NULL) {
		fprintf(stderr, "Error reading %s\n", path);
		return;
	}

	int count = 0;
	char post_path[max_len];
	char post_title[max_len];
	char post_date[20];

	struct buffer* next_post;
	char* next_post_path = NULL;
	char* next_next_post_path = NULL;

	char archive_date[15] = "";

	// erm, this is a going to be fun to read later, google scansets
	// also hardcoded the maximums....
	while (fscanf(posts, "%256[^\t]%*[\t]%256[^\t]%*[\t]%18[^\n]%*[\n]", post_path, post_title, post_date) == 3) {
		count++;

		strcpy(path+bpl, post_path);
		strcpy(path+bpl+strlen(post_path), blog_file);

		// post page
		struct buffer* post = buf_new(10240);
		buf_append_buf(post, header1);
		buf_append_str(post, " - ");
		buf_append_str(post, post_title);
		buf_append_buf(post, header2);
		buf_append_str(post, "<article><h1>");
		buf_append_str(post, post_title);
		buf_append_str(post, "</h1><h2>");
		buf_append_str(post, post_date);
		buf_append_str(post, "</h2>\n");
		if (buf_append_file(post, path) < 0) {
			fprintf(stderr, "Error reading %s\n", path);
			buf_free(post);
			continue;
		}

		char* server_path = malloc(spl + strlen(post_path) + 1);
		strcpy(server_path, blog_dir);
		strcpy(server_path+spl, post_path);
		routes_add(routes, RT_BUFFER, server_path, post);

		//printf("\"%s\" \"%s\" \"%s\"\n", server_path, next_post_path, next_next_post_path);

		// add navigation and close off the next post (which we already saw because blogs are backwards)
		if (count > 1) {
			buf_append_str(next_post, "<nav><a href=\"");
			buf_append_str(next_post, server_path);
			buf_append_str(next_post, "\">prev</a>");
			if (count > 2) {
				buf_append_str(next_post, "<a href=\"");
				buf_append_str(next_post, next_next_post_path);
				buf_append_str(next_post, "\">next</a>");
			}
			buf_append_str(next_post, "</nav></article>\n");
			buf_append_buf(next_post, footer);
		}

		next_post = post;
		next_next_post_path = next_post_path;
		next_post_path = server_path;

		// index
		if (count > 1) {
			buf_append_str(index, "<hr>\n");
		}
		buf_append_str(index, "<article><h1><a href=\"");
		buf_append_str(index, server_path);
		buf_append_str(index, "\">");
		buf_append_str(index, post_title);
		buf_append_str(index, "</a></h1><h2>");
		buf_append_str(index, post_date);
		buf_append_str(index, "</h2>");
		buf_append_file(index, path);
		buf_append_str(index, "</article>\n");

		// archive
		if (strcmp(archive_date, strchr(post_date, ' ')+1) != 0) {
			buf_append_str(archive, "<hr>\n");

			strcpy(archive_date, strchr(post_date, ' ')+1);
			buf_append_str(archive, "<h3>");
			buf_append_str(archive, archive_date);
			buf_append_str(archive, "</h3>\n");
		}		
		buf_append_str(archive, "<p><a href=\"");
		buf_append_str(archive, server_path);
		buf_append_str(archive, "\">");
		buf_append_str(archive, post_title);
		buf_append_str(archive, "</a></p>\n");
	}

	// close off first post (which is last becuase blogs are backwards)
	if (count > 1) {
		buf_append_str(next_post, "<nav><span>&nbsp;</span><a href=\"");
		buf_append_str(next_post, next_next_post_path);
		buf_append_str(next_post, "\">next</a></nav>");
	}
	if (count > 0) {
		buf_append_str(next_post, "</article>\n");
		buf_append_buf(next_post, footer);
	}

	// close index
	buf_append_buf(index, footer);
	routes_add(routes, RT_BUFFER, "/", index);

	// close archive
	buf_append_buf(archive, footer);
	routes_add(routes, RT_BUFFER, "/blog", archive);

	// clean up
	buf_free(header1);
	buf_free(header2);
	buf_free(footer);
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
	blog_build(argv[2], routes);

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
