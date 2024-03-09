#ifndef TINN_ROUTES_H
#define TINN_ROUTES_H

#include "console.h"
#include "buffer.h"

#define RT_FILE 1
#define RT_REDIRECT 2
#define RT_BUFFER 3

typedef struct route {
	unsigned short type;
	char* from;
	void* to;
	struct route* next;
} Route;

typedef struct {
	Route* head;
	Route** tail_next;
} Routes;

Routes* routes_new();
void routes_free(Routes* list);
Buffer* routes_new_buf(Routes* list, char* from, size_t buf_len);
void routes_add_static(Routes* list);

Route* routes_find(Routes* list, char* from);

void routes_log(Routes* list, ConsoleLevel level);
void route_log(Route* route, ConsoleLevel level);

#endif