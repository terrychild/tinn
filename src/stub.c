#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

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
			return;
		}
		total += sent;
	}
}

void respond(int sock, char *status, char *statusText, char *body) {
	char buf[BUFFER_SIZE];
	sprintf(buf, "HTTP/1.0 %s %s\nServer: Stub\nContent-Type: text/html\nContent-Length: %d\n\n<html><body>%s</body></html>", status, statusText, strlen(body)+26, body);
	
	sendAll(sock, buf, strlen(buf));
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

	printf("waiting for connections\n");

	// accept connection and respond
	for (;;) {
		addressSize = sizeof clientAddress;
		if ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addressSize)) < 0) {
			fprintf(stderr, "accept error: %s\n", strerror(errno));
		}

		if (!fork()) {
			// child process does not need server socket
			close(serverSocket);

			inet_ntop(clientAddress.ss_family, get_in_addr((struct sockaddr *)&clientAddress), clientAddressString, sizeof clientAddressString);
			
			// read request
			if ((dataInLen = recv(clientSocket, &dataIn, BUFFER_SIZE, 0)) < 0) {
				fprintf(stderr, "recv error: %s\n", strerror(errno));
			}

			method = strtok(dataIn, " ");
			path = strtok(NULL, " ");
			printf("address: %s, method: %s, path: %s\n", clientAddressString, method, path);

			// route
			if (method != NULL && path != NULL) {
				if (strcmp(method, "GET")==0) {
					if (strcmp(path, "/")==0) {
						respond(clientSocket, "200", "OK", "<p>It still works!</p>");
					} else {
						respond(clientSocket, "404", "Not Found", "<h1>404 - Not Found</h1><p>Opps, that resource can not be found.</p>");
					}
				} else {
					respond(clientSocket, "501", "Not Implemented", "<h1>501 - Not Implemented</h1><p>Opps, that functionality has not been implemented.</p>");
				}
			} else {
				respond(clientSocket, "500", "Internal Server Error", "<h1>500 - Internal Server Error</h1><p>Opps, something bad has happened, don't worry it's probably not your fault.  Probably.</p>");
				//printf("request:\n%s\n", dataIn);
			}

			close(clientSocket);
			exit(0);
		}

		close(clientSocket);
	}


	// tidy up
	close(serverSocket);
	
	return EXIT_SUCCESS;
}