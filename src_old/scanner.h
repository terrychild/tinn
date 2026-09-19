#ifndef TINN_SCANNER_H
#define TINN_SCANNER_H

#include <stdbool.h>

typedef struct {
	const char* start;
	const char* current;
	size_t length;
	size_t read;
} Scanner;

typedef struct {
	const char* start;
	size_t length;
} Token;

Scanner scanner_new(const char* source, const size_t length);
Token scan_token(Scanner* scanner, const char* delims);

bool token_is(Token token, const char* str);

#endif