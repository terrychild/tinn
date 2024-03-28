#ifndef TINN_SCANNER_H
#define TINN_SCANNER_H

#include <stdbool.h>

typedef struct {
	const char* start;
	const char* current;
	int length;
	int read;
} Scanner;

typedef struct {
	const char* start;
	int length;
} Token;

Scanner scanner_new(const char* source, const int length);
Token scan_token(Scanner* scanner, const char* delims);

bool token_is(Token token, const char* str);

#endif