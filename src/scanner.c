#include <string.h>

#include "scanner.h"

Scanner scanner_new(const char* source, const size_t length) {
	Scanner scanner = {
		.start = source,
		.current = source,
		.length = length,
		.read = 0
	};
	return scanner;
}

static bool at_end(Scanner* scanner) {
	return scanner->read == scanner->length || *(scanner->current) == '\0';
}

static char peek(Scanner* scanner) {
	return *(scanner->current);
}

static void advance(Scanner* scanner) {
	scanner->current++;
	scanner->read++;
}

Token scan_token(Scanner* scanner, const char* delims) {
	while (!at_end(scanner) && strchr(delims, peek(scanner))==NULL) {
		advance(scanner);
	}
	Token rv = {
		.start = scanner->start,
		.length = (int)(scanner->current - scanner->start)
	};
	while (!at_end(scanner) && strchr(delims, peek(scanner))!=NULL) {
		advance(scanner);
	}
	scanner->start = scanner->current;
	return rv;
}

bool token_is(Token token, const char* str) {
	return strlen(str)==token.length && strncmp(token.start, str, token.length)==0;
}