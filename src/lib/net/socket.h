#ifndef LIB_NET_SOCKET_H
#define LIB_NET_SOCKET_H

int listenToSocket(char* port);
int acceptSocket(int fd, char* text_address);

#endif