#ifndef TINN_BLOG_H
#define TINN_BLOH_H

#include <stdbool.h>
#include "request.h"
#include "response.h"

#define BLOG_MAX_PATH_LEN 256
#define BLOG_MAX_DATE_LEN 20

struct post {
	char source[BLOG_MAX_PATH_LEN];
	char path[BLOG_MAX_PATH_LEN];
	char title[BLOG_MAX_PATH_LEN];
	char date[BLOG_MAX_DATE_LEN];
	Buffer* content;
};

typedef struct {
	Buffer* header1;
	Buffer* header2;
	Buffer* footer;
	size_t size;
	size_t count;
	struct post* posts;
} Blog;

Blog* blog_new();
void blod_free(Blog* blog);

bool blog_content(void* state, Request* request, Response* Response);

#endif