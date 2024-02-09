#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
	int status;
	struct addrinfo hints, *serverAddress;
	int serverSocket;

	struct sockaddr_storage clientAddress;
	socklen_t addressSize;
	char clientAddressString[INET6_ADDRSTRLEN];
	int clientSocket;

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

	if (bind(serverSocket, serverAddress->ai_addr, serverAddress->ai_addrlen) != 0) {
		fprintf(stderr, "bind error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	freeaddrinfo(serverAddress);

	if (listen(serverSocket, 10) != 0) {
		fprintf(stderr, "listen error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	printf("waiting for connections\n");

	// prep response
	char response[256];
	char body[] = "<html><body><p>It works!</p></body></html>";
	sprintf(response, "HTTP/1.0 200 OK\nServer: Stub\nContent-Type: text/html\nContent-Length: %d\n\n%s", strlen(body), body);

	// accept connection and respond
	for(;;) {
		addressSize = sizeof clientAddress;
		if ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addressSize)) < 0) {
			fprintf(stderr, "accept error: %s\n", strerror(errno));
		}

		inet_ntop(clientAddress.ss_family, get_in_addr((struct sockaddr *)&clientAddress), clientAddressString, sizeof clientAddressString);
		printf("got connection from %s\n", clientAddressString);

		send(clientSocket, &response, strlen(response), 0);

		close(clientSocket);
	}


	// tidy up
	close(serverSocket);
	
	return EXIT_SUCCESS;
}