#ifndef TINN_BLOG_H
#define TINN_BLOH_H

#include <stdbool.h>
#include "request.h"
#include "response.h"

#define BLOG_MAX_PATH_LEN 256
#define BLOG_MAX_DATE_LEN 20

struct html_fragment {
	const char* path;
	time_t mod_date;
	Buffer* buf;
};
#define HF_HEADER_1	0
#define HF_HEADER_2	1
#define HF_FOOTER	2
#define HF_COUNT	3

struct post {
	char source[BLOG_MAX_PATH_LEN];
	char path[BLOG_MAX_PATH_LEN];
	char title[BLOG_MAX_PATH_LEN];
	char date[BLOG_MAX_DATE_LEN];
	time_t mod_date;
	Buffer* content;
};

typedef struct {
	time_t mod_date;
	struct html_fragment fragments[HF_COUNT];
	size_t size;
	size_t count;
	struct post* posts;
} Blog;

Blog* blog_new();
void blog_free(Blog* blog);

bool blog_content(void* state, Request* request, Response* Response);

#endif