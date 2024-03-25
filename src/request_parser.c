#include <stdbool.h>
#include <string.h>

#include "request_parser.h"
#include "utils.h"
#include "console.h"

RequestParser request_parser_new(const char* source, const int length) {
	RequestParser parser = {
		.start = source,
		.current = source,
		.length = length,
		.read = 0,
		.state = PARSER_STATE_METHOD
	};
	return parser;
}

static RequestToken make_token(RequestParser* parser, TokenType type) {
	RequestToken t = {
		.type = type,
		.start = parser->start,
		.length = (int)(parser->current - parser->start)
	};
	return t;
}

static RequestToken make_error(const char* message) {
	RequestToken t = {
		.type = TOKEN_ERROR,
		.start = message,
		.length = strlen(message)
	};
	return t;
}

static bool eos(RequestParser* parser) {
	return parser->read == parser->length || *(parser->current) == '\0';
}

static char peek(RequestParser* parser) {
	return parser->current[0];
}

static char advance(RequestParser* parser) {
	parser->current++;
	parser->read++;
	return parser->current[-1];
}

/*static bool match(RequestParser* parser, char expected) {
	if (!eos(parser) && parser->current[0] == expected) {
		parser->current++;
		parser->read++;
		return true;
	} return false;
}*/

static bool is_whitespace(char c) {
	return c==' ' || c=='\t'/* || c=='\r' || c=='\n'*/;
}
static void consume_whitespace(RequestParser* parser) {
	while (!eos(parser) && is_whitespace(peek(parser))) {
		advance(parser);
	}
}

static bool is_alpha(char c) {
	return (c>='A' && c<='Z') || (c>='a' && c<='z');
}
static void consume_alpha(RequestParser* parser) {
	while (!eos(parser) && is_alpha(peek(parser))) {
		advance(parser);
	}
}

static bool is_digit(char c) {
	return c>='0' && c<='9';
}
static bool is_unreserved(char c) {
	return is_alpha(c) || is_digit(c) || c=='-' || c=='.' || c=='_' || c=='~';
}
static bool is_path_char(char c) {
	return is_unreserved(c) || c==':' || c=='/' || c=='!' || c=='$' || c=='&' || c=='\'' || c=='(' || c==')' || c=='*' || c=='+' || c==',' || c==';' || c=='=';
}
static void consume_path(RequestParser* parser) {
	while (!eos(parser) && is_path_char(peek(parser))) {
		advance(parser);
	}
}

/*static RequestToken word(RequestParser* parser) {
	while (!eos(parser) && !is_whitespace(peek(parser))) {
		advance(parser);
	}

	return make_token(parser, TOKEN_WORD);
}*/

static bool check_word(RequestParser* parser, const char* word) {
	int len = strlen(word);
	if ((parser->current - parser->start) == len) {
		return memcmp(parser->start, word, len) == 0;
	}
	return false;
}

static RequestToken method(RequestParser* parser) {
	parser->start = parser->current;
	consume_alpha(parser);
	parser->state++;
	if (parser->current == parser->start) {
		return make_error("Empty method");
	}
	if (check_word(parser, "GET")) {
		return make_token(parser, TOKEN_METHOD_GET);
	} else if (check_word(parser, "HEAD")) {
		return make_token(parser, TOKEN_METHOD_HEAD);
	} else if (check_word(parser, "POST")) {
		return make_token(parser, TOKEN_METHOD_POST);
	} else if (check_word(parser, "PUT")) {
		return make_token(parser, TOKEN_METHOD_PUT);
	} else if (check_word(parser, "DELETE")) {
		return make_token(parser, TOKEN_METHOD_DELETE);
	} else if (check_word(parser, "CONNECT")) {
		return make_token(parser, TOKEN_METHOD_CONNECT);
	} else if (check_word(parser, "OPTIONS")) {
		return make_token(parser, TOKEN_METHOD_OPTIONS);
	} else if (check_word(parser, "TRACE")) {
		return make_token(parser, TOKEN_METHOD_TRACE);
	} else if (check_word(parser, "PATCH")) {
		return make_token(parser, TOKEN_METHOD_PATCH);
	}
	return make_error("Unknown method");
}

static RequestToken path(RequestParser* parser) {
	consume_whitespace(parser);
	parser->start = parser->current;
	consume_path(parser);
	parser->state++;
	if (parser->current == parser->start) {
		return make_error("Empty path");
	}
	return make_token(parser, TOKEN_PATH);
}

static RequestToken query(RequestParser* parser) {
	if (peek(parser) == '?') {
		advance(parser); // consume '?'
		parser->start = parser->current;
		consume_path(parser);
	}
	parser->state++;
	return make_token(parser, TOKEN_QUERY);
}

static RequestToken version(RequestParser* parser) {
	consume_whitespace(parser);
	parser->start = parser->current;
	consume_alpha(parser);
	if (check_word(parser, "HTTP")) {
		if (!eos(parser) && advance(parser) == '/') {
			if (!eos(parser) && is_digit(advance(parser))) {
				if (!eos(parser) && advance(parser) == '.') {
					if (!eos(parser) && is_digit(advance(parser))) {
						parser->state++;
						return make_token(parser, TOKEN_VERSION);
					}
				}
			}
		}
	}
	return make_error("Invalid version");
}

RequestToken parser_read_token(RequestParser* parser) {
	switch(parser->state) {
		case PARSER_STATE_METHOD:
			return method(parser);
			break;
		case PARSER_STATE_PATH:
			return path(parser);
			break;
		case PARSER_STATE_QUERY:
			return query(parser);
			break;
		case PARSER_STATE_VERSION:
			return version(parser);
			break;
	}

	
}

char* token_to_str(RequestToken token) {
	char* str = allocate(NULL, token.length+1);
	memcpy(str, token.start, token.length);
	str[token.length] = '\0';
	return str;
}