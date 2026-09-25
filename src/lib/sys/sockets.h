#ifndef LIB_SYS_SOCKETS_H
#define LIB_SYS_SOCKETS_H

int listenToSocket(const char* port);
int acceptSocket(int fd, char* text_address);

#endif