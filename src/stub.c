#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define QUEUE_SIZE 10
#define BUFFER_SIZE 2560

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sendAll(int sock, char *buf, int len) {
	int total = 0;
	int todo = len;
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

void respond(int sock, char *status, char *statusText, char *body) {
	char headers[BUFFER_SIZE];
	sprintf(headers, "HTTP/1.1 %s %s\nServer: Stub\nContent-Type: text/html\nContent-Length: %d\n\n", status, statusText, strlen(body));
	sendAll(sock, headers, strlen(headers));
	sendAll(sock, body, strlen(body));
}

void sendStatus(int sock, char *status, char *statusText, char *description) {
	char body[BUFFER_SIZE];
	sprintf(body, "<html><body><h1>%s - %s</h1><p>%s</p></body></html>", status, statusText, description);
	respond(sock, status, statusText, body);
}

void sendContent(int sock, char *filename) {
	char *buffer;
	long length;
	FILE *f = fopen(filename, "rb");

	if (f) {
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length+1);
		if (buffer) {
			fread(buffer, 1, length, f);
			buffer[length] = '\0';
		}
		fclose(f);
	}

	respond(sock, "200", "OK", buffer);
	free(buffer);
}

void addSocket(struct pollfd *sockets[], int newSocket, int *count, int *size) {
	if (*count == *size) {
		*size *= 2;

		*sockets = realloc(*sockets, sizeof(**sockets) * (*size));
	}

	(*sockets)[*count].fd = newSocket;
	(*sockets)[*count].events = POLLIN;

	(*count)++;
}

void rmSocket(struct pollfd sockets[], int i, int *count) {
	sockets[i] = sockets[*count-1];

	(*count)--;
}

int main(int argc, char *argv[]) {
	int status;
	struct addrinfo hints, *serverAddress;
	int serverSocket;
	int yes=1;

	struct sockaddr_storage clientAddress;
	socklen_t addressSize;
	char clientAddressString[INET6_ADDRSTRLEN];
	int clientSocket;

	int socketsCount = 0;
	int socketsSize = 10;
	struct pollfd *sockets = malloc(sizeof *sockets * socketsSize);

	char dataIn[BUFFER_SIZE];
	int dataInLen;
	char *method;
	char *path;

	// validate arguments
	if (argc != 2) {
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		return EXIT_FAILURE;
	}

	// get local address info
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(NULL, argv[1], &hints, &serverAddress)) != 0) {
		fprintf(stderr, "gettaddrinfo error: %s\n", gai_strerror(status));
		return EXIT_FAILURE;
	}

	// get socket, bind to it and listen
	if ((serverSocket = socket(serverAddress->ai_family, serverAddress->ai_socktype, serverAddress->ai_protocol)) < 0) {
		fprintf(stderr, "socket error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) !=0 ) {
		fprintf(stderr, "setsockopt error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	if (bind(serverSocket, serverAddress->ai_addr, serverAddress->ai_addrlen) != 0) {
		fprintf(stderr, "bind error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	freeaddrinfo(serverAddress);

	if (listen(serverSocket, QUEUE_SIZE) != 0) {
		fprintf(stderr, "listen error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	addSocket(&sockets, serverSocket, &socketsCount, &socketsSize);

	printf("waiting for connections\n");

	// accept connection and respond
	for (;;) {
		int pollCount = poll(sockets, socketsCount, -1);

		if (pollCount == -1) {
			fprintf(stderr, "poll error: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}

		for (int i = 0; i < socketsCount; i++) {
			if (sockets[i].revents & POLLIN) {
				if (sockets[i].fd == serverSocket) {
					// server listener
					addressSize = sizeof clientAddress;
					if ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addressSize)) < 0) {
						fprintf(stderr, "accept error: %s\n", strerror(errno));
					} else {
						inet_ntop(clientAddress.ss_family, get_in_addr((struct sockaddr *)&clientAddress), clientAddressString, sizeof clientAddressString);

						printf("connection from %s on %d\n", clientAddressString, clientSocket);

						addSocket(&sockets, clientSocket, &socketsCount, &socketsSize);
					}
				} else {
					// client socket
					dataInLen = recv(sockets[i].fd, dataIn, BUFFER_SIZE, 0);

					if (dataInLen <= 0) {
						if (dataInLen < 0) {
							fprintf(stderr, "recv error on %d: %s\n", sockets[i].fd, strerror(errno));
						} else {
							printf("connection on %d closed\n", sockets[i].fd);
						}

					} else {
						method = strtok(dataIn, " ");
						path = strtok(NULL, " ");
						printf("connection: %d, method: %s, path: %s\n", sockets[i].fd, method, path);

						// route
						if (method != NULL && path != NULL) {
							if (strcmp(method, "GET")==0) {
								if (strcmp(path, "/")==0) {
									sendContent(clientSocket, "content.html");
								} else if (strcmp(path, "/code")==0) {
									sendContent(clientSocket, "stub.c");
								} else {
									sendStatus(clientSocket, "404", "Not Found", "Opps, that resource can not be found.");
								}
							} else {
								sendStatus(clientSocket, "501", "Not Implemented", "Opps, that functionality has not been implemented.");
							}
						} else {
							sendStatus(clientSocket, "500", "Internal Server Error", "Opps, something bad has happened, don't worry it's probably not your fault.  Probably.");
							//printf("request:\n%s\n", dataIn);
						}
					}

					close(sockets[i].fd);
					rmSocket(sockets, i, &socketsCount);
				}
			}
		}
	}

	// tidy up?
	close(serverSocket);
	
	return EXIT_SUCCESS;
}