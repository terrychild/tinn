#ifndef TINN_SCANNER_H
#define TINN_SCANNER_H

typedef struct {
	const char* start;
	const char* current;
	int length;
	int read;
	int line;
	int col;
} Scanner;

typedef enum {
	TOKEN_FIELD,
	TOKEN_EOS,
	TOKEN_ERROR
} TokenType;

typedef struct {
	TokenType type;
	const char* start;
	int length;
	int line;
	int col;
} Token;

Scanner scanner_new(const char* source, const int length);
Token scan_token(Scanner* scanner);

#endif