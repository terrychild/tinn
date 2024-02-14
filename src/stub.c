#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <dirent.h> // for reading directories

#define QUEUE_SIZE 10
#define BUFFER_SIZE 2560
#define TIMEOUT 2500

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

void sendHeader(int sock, char *status, char *statusText, char *type, int len) {
	char headers[BUFFER_SIZE];
	sprintf(headers, "HTTP/1.1 %s %s\r\nServer: Stub\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", status, statusText, type, len);
	sendAll(sock, headers, strlen(headers));
}

void sendStatus(int sock, char *status, char *statusText, char *description) {
	char body[BUFFER_SIZE];
	sprintf(body, "<html><body><h1>%s - %s</h1><p>%s</p></body></html>", status, statusText, description);
	sendHeader(sock, status, statusText, "text/html", strlen(body));
	sendAll(sock, body, strlen(body));
}

char *getContentType(char *filename) {
	char *ext;
	ext = strrchr(filename, '.');

	if (ext) {
		if (strcmp(ext, ".html")==0 || strcmp(ext, ".htm")==0) {
			return "text/html; charset=utf-8";
		} else if (strcmp(ext, ".css")==0) {
			return "text/css; charset=utf-8";
		} else if (strcmp(ext, ".js")==0) {
			return "text/javascript; charset=utf-8";
		} else if (strcmp(ext, ".jpeg")==0 || strcmp(ext, ".jpg")==0) {
			return "image/jpeg";
		} else if (strcmp(ext, ".png")==0) {
			return "image/png";
		} else if (strcmp(ext, ".gif")==0) {
			return "image/gif";
		} else if (strcmp(ext, ".bmp")==0) {
			return "image/bmp";
		} else if (strcmp(ext, ".svg")==0) {
			return "image/svg+xml";
		} else if (strcmp(ext, ".ico")==0) {
			return "image/vnd.microsoft.icon";
		}
	}
	return "text/plain; charset=utf-8";
}

void sendContent(int sock, char *filename) {
	char *buffer;
	long length;
	FILE *f = fopen(filename, "rb");

	if (f) {
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer) {
			fread(buffer, 1, length, f);
		}
		fclose(f);
	}

	sendHeader(sock, "200", "OK", getContentType(filename), length);
	sendAll(sock, buffer, length);
	free(buffer);
}

void addSocket(struct pollfd *sockets[], int newSocket, int *count, int *size) {
	if (*count == *size) {
		*size *= 2;

		*sockets = realloc(*sockets, sizeof(**sockets) * (*size));
	}

	(*sockets)[*count].fd = newSocket;
	(*sockets)[*count].events = POLLIN;
	(*sockets)[*count].revents = 0;

	(*count)++;
}

void rmSocket(struct pollfd sockets[], int i, int *count) {
	sockets[i] = sockets[*count-1];

	(*count)--;
}

void findFiles(char files[BUFFER_SIZE][BUFFER_SIZE], int *count, char *path) {
	DIR *d;
	struct dirent *dir;
	d = opendir(path);
	if (d) {
		while ((dir = readdir(d))!=NULL && *count<BUFFER_SIZE) {
			if (dir->d_name[0] != '.') {
				if (dir->d_type == DT_REG) {
					sprintf(files[*count], "%s/%s", path, dir->d_name);
					(*count)++;
				} else if (dir->d_type == DT_DIR) {
					char d_path[BUFFER_SIZE];
					sprintf(d_path, "%s/%s", path, dir->d_name);
					findFiles(files, count, d_path);
				}
			}
		}
		closedir(d);
	}
}

int validFile(char files[BUFFER_SIZE][BUFFER_SIZE], int count, char *path) {
	for (int i=0; i<count; i++) {
		if (strcmp(files[i], path)==0) {
			return 0;
		}
	}
	return -1;
}

int main(int argc, char *argv[]) {
	// get list of files
	int filesCount = 0;
	char files[BUFFER_SIZE][BUFFER_SIZE];

	findFiles(files, &filesCount, ".");

	printf("available content\n");
	for (int i=0; i<filesCount; i++) {
		printf("%s\n", files[i]);
	}

	// network stuff
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
	char path[BUFFER_SIZE];

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
		int pollCount = poll(sockets, socketsCount, TIMEOUT);
		
		if (pollCount == -1) {
			fprintf(stderr, "poll error: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}

		if (pollCount == 0 ) {
			// no events, time to close client sockets
			while (socketsCount>1) {
				printf("close %d\n", sockets[1].fd);
				close(sockets[1].fd);
				rmSocket(sockets, 1, &socketsCount);
			}
		} else {
			// there where events!
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

							close(sockets[i].fd);
							rmSocket(sockets, i, &socketsCount);
							i--;

						} else {
							// parse the request
							/*for (int j=0; j<dataInLen; j++) {
								if (j>2 && dataIn[j-3]=='\r' && dataIn[j-2]=='\n' && dataIn[j-1]=='\r' && dataIn[j]=='\n') {
									printf("found end at %d\n", j);
								}
							}*/

							method = strtok(dataIn, " ");
							sprintf(path, ".%s", strtok(NULL, " "));
							printf("connection: %d, method: %s, path: %s\n", sockets[i].fd, method, path);

							// route
							if (method != NULL && path != NULL) {
								if (strcmp(method, "GET")==0) {
									if (strcmp(path, "./")==0) {
										sendContent(clientSocket, "content.html");
									} else if (strcmp(path, "./blog")==0 || strcmp(path, "./blog/")==0) {
										sendContent(clientSocket, "./blog/all.html");
									} else if (validFile(files, filesCount, path)==0) {
										sendContent(clientSocket, path);
									} else if (strncmp(path, "./blog/", 7)==0) { // hack for now, will write a better blog module later
										strcat(path, ".html");
										if (validFile(files, filesCount, path)==0) {
											sendContent(clientSocket, path);
										} else {
											sendStatus(clientSocket, "404", "Not Found", "Opps, that resource can not be found.");
										}
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


					}
				}
			}
		}
	}

	// tidy up?
	close(serverSocket);
	
	return EXIT_SUCCESS;
}