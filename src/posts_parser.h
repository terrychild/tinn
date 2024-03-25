#ifndef TINN_POSTS_PARSER_H
#define TINN_POSTS_PARSER_H

#include <stdbool.h>

typedef struct {
	const char* start;
	const char* current;
	int length;
	int read;
} PostsParser;

typedef struct {
	const char* start;
	int length;
} PostsToken;

PostsParser posts_parser_new(const char* source, const int length);
bool read_post(PostsParser* parser, PostsToken* dir, PostsToken* title, PostsToken* date);

#endif