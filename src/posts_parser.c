#include "posts_parser.h"

PostsParser posts_parser_new(const char* source, const int length) {
	PostsParser parser = {
		.start = source,
		.current = source,
		.length = length,
		.read = 0
	};
	return parser;
}

static bool eos(PostsParser* parser) {
	return parser->read == parser->length || *(parser->current) == '\0';
}

static char advance(PostsParser* parser) {
	parser->current++;
	parser->read++;
	return parser->current[-1];
}

static char peek(PostsParser* parser) {
	return parser->current[0];
}

static bool is_new_line(char c) {
	return c=='\r' || c=='\n';
}

static bool is_field_char(char c) {
	return c!='\t' && !is_new_line(c);
}

static void skip_new_line(PostsParser* parser) {
	while (!eos(parser) && is_new_line(peek(parser))) {
		advance(parser);
	}
}

static void skip_tab(PostsParser* parser) {
	while (!eos(parser) && peek(parser)=='\t') {
		advance(parser);
	}
}

static PostsToken field(PostsParser* parser) {
	while (!eos(parser) && is_field_char(peek(parser))) {
		advance(parser);
	}

	PostsToken token = {
		.start = parser->start,
		.length = (int)(parser->current - parser->start)
	};
	return token;
}

bool read_post(PostsParser* parser, PostsToken* dir, PostsToken* title, PostsToken* date) {
	skip_new_line(parser);
	parser->start = parser->current;
	*dir = field(parser);

	skip_tab(parser);
	parser->start = parser->current;
	*title = field(parser);

	skip_tab(parser);
	parser->start = parser->current;
	*date = field(parser);

	return (dir->length>0 && title->length>0 && date->length>0);
}