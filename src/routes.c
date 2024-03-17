#include <stdlib.h>
#include <string.h>
#include <fts.h> // for file system access stuff

#include "utils.h"
#include "routes.h"

Routes* routes_new() {
	Routes* list = allocate(NULL, sizeof(*list));
	list->head = NULL;
	list->tail_next = &list->head;
	return list;
}
void routes_free(Routes* list) {
	if (list != NULL) {
		Route* route = list->head;
		while (route != NULL) {
			Route* next_route = route->next;

			free(route->from);
			if(route->type == RT_BUFFER) {
				buf_free(route->to);
			} else {
				free(route->to);
			}

			free(route);
			
			route = next_route;
		}	
		free(list);
	}
}

static Route* add(Routes* list) {
	Route* new_route  = allocate(NULL, sizeof(*new_route));
	
	new_route->next = NULL;
	*list->tail_next = new_route;
	list->tail_next = &new_route->next;
	
	return new_route;
}

static Route* add_str(Routes* list, char* from, size_t from_len, char* to, size_t to_len) {
	Route* new_route = add(list);
	new_route->from = allocate(NULL, from_len+1);
	new_route->to = allocate(NULL, to_len+1);

	strncpy(new_route->from, from, from_len);
	new_route->from[from_len] = '\0';

	strncpy(new_route->to, to, to_len);
	((char*)new_route->to)[to_len] = '\0';

	return new_route;
}
static void add_file(Routes* list, char* from, size_t from_len, char* to, size_t to_len) {
	Route* new_route = add_str(list, from, from_len, to, to_len);
	new_route->type = RT_FILE;
}
static void add_redirect(Routes* list, char* from, size_t from_len, char* to, size_t to_len) {
	Route* new_route = add_str(list, from, from_len, to, to_len);
	new_route->type = RT_REDIRECT;
}

static Buffer* add_buf(Routes* list, char* from, size_t from_len, size_t buf_len) {
	Route* new_route = add(list);
	new_route->type = RT_BUFFER;
	new_route->from = allocate(NULL, from_len+1);
	new_route->to = buf_new(buf_len);

	strncpy(new_route->from, from, from_len);
	new_route->from[from_len] = '\0';

	return new_route->to;
}

Buffer* routes_new_buf(Routes* list, char* from, size_t buf_len) {
	return add_buf(list, from, strlen(from), buf_len);
}

void routes_add_static(Routes* list) {
	TRACE("build static routes");

	// read the file system
	FTS* file_system = NULL;
	FTSENT* node = NULL;

	file_system = fts_open((char* const[]){".", NULL}, FTS_LOGICAL, NULL);
	if (file_system == NULL) {
		PANIC("unable to open file system bulding static routes");
	}
	
	while ((node = fts_read(file_system)) != NULL) {
		// ignore dot (hidden) files unless it's the . dir aka the current dir
		if(node->fts_name[0]=='.' && strcmp(node->fts_path, ".")!=0) {
			fts_set(file_system, node, FTS_SKIP);

		} else if (node->fts_info == FTS_F) {
			// a valid file, add route
			add_file(list, node->fts_path+1, node->fts_pathlen-1, node->fts_path, node->fts_pathlen);

			// add directory?
			if (strcmp(node->fts_name, "index.html") == 0) {
				size_t len = node->fts_pathlen - 10;				
				add_file(list, node->fts_path+1, len-1, node->fts_path, node->fts_pathlen);
				if (len>2) {
					add_redirect(list, node->fts_path+1, len-2, node->fts_path+1, len-1);
				}
			}
		}
	}

	fts_close(file_system);
}

Route* routes_find(Routes* list, char* from) {
	Route* route = list->head;
	while (route != NULL) {
		if (strcmp(route->from, from)==0) {
			return route;
		}
		route = route->next;
	}
	return NULL;
}

void routes_log(Routes* list, ConsoleLevel level) {
	console(stdout, level, true, false, "routes");

	Route* route = list->head;
	while (route != NULL) {
		route_log(route, level);
		route = route->next;
	}
}
void route_log(Route* route, ConsoleLevel level) {
	if (route == NULL) {
		console(stdout, level, false, false, "Unknown route");
	} else {
		switch(route->type) {
			case RT_FILE: 
				console(stdout, level, false, false, "\"%s\" -> \"%s\"", route->from, route->to);
				break;
			case RT_REDIRECT: 
				console(stdout, level, false, false, "\"%s\" RD \"%s\"", route->from, route->to);
				break;
			case RT_BUFFER: 
				console(stdout, level, false, false, "\"%s\" buffer (%d)", route->from, ((Buffer*)route->to)->length);
				break;
			default:
				console(stdout, level, false, false, "\"%s\" ??", route->from);
				break;
		}
	}
}