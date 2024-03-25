#ifndef TINN_REQUEST_PARSER_H
#define TINN_REQUEST_PARSER_H

typedef enum {
	PARSER_STATE_METHOD,
	PARSER_STATE_PATH,
	PARSER_STATE_QUERY,
	PARSER_STATE_VERSION,
	PARSER_STATE_HEADERS
} ParserState;

typedef struct {
	const char* start;
	const char* current;
	int length;
	int read;
	ParserState state;
} RequestParser;

typedef enum {
	TOKEN_METHOD_GET,
	TOKEN_METHOD_HEAD,
	TOKEN_METHOD_POST,
	TOKEN_METHOD_PUT,
	TOKEN_METHOD_DELETE,
	TOKEN_METHOD_CONNECT,
	TOKEN_METHOD_OPTIONS,
	TOKEN_METHOD_TRACE,
	TOKEN_METHOD_PATCH,
	TOKEN_PATH,
	TOKEN_QUERY,
	TOKEN_VERSION,
	TOKEN_ERROR
} TokenType;

typedef struct {
	TokenType type;
	const char* start;
	int length;
} RequestToken;

RequestParser request_parser_new(const char* source, const int length);
RequestToken parser_read_token(RequestParser* parser);
char* token_to_str(RequestToken token);

#endif