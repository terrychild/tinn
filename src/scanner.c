#include <stdbool.h>

#include "scanner.h"
#include "utils.h"
#include "console.h"

Scanner scanner_new(const char* source, const int length) {
	Scanner scanner = {
		.start = source,
		.current = source,
		.length = length,
		.read = 0,
		.line = 1,
		.col = 0
	};
	return scanner;
}

static Token make_token(Scanner* scanner, TokenType type) {
	Token t = {
		.type = type,
		.start = scanner->start,
		.length = (int)(scanner->current - scanner->start),
		.line = scanner->line
	};
	t.col = scanner->col - t.length;
	return t;
}

static bool eos(Scanner* scanner) {
	return scanner->read == scanner->length || *(scanner->current) == '\0';
}

static char advance(Scanner* scanner) {
	if (scanner->read>0 && scanner->current[-1]=='\n') {
		scanner->line++;
		scanner->col = 0;
	}
	scanner->current++;
	scanner->read++;
	scanner->col++;
	return scanner->current[-1];
}

static char peek(Scanner* scanner) {
	return scanner->current[0];
}

static bool is_sep(char c) {
	return c=='\t' || c=='\r' || c=='\n';
}

static void skip_sep(Scanner* scanner) {
	while (!eos(scanner) && is_sep(peek(scanner))) {
		advance(scanner);
	}
}

static Token field(Scanner* scanner) {
	while (!eos(scanner) && !is_sep(peek(scanner))) {
		advance(scanner);
	}

	return make_token(scanner, TOKEN_FIELD);
}

Token scan_token(Scanner* scanner) {
	skip_sep(scanner);
	scanner->start = scanner->current;
	
	if (eos(scanner)) {
		TRACE("scanner - eos, read %d/%d", scanner->read, scanner->length);
		return make_token(scanner, TOKEN_EOS);
	}
	TRACE("scanner - char (%c) %d, read %d/%d", *(scanner->current), *(scanner->current), scanner->read, scanner->length);

	return field(scanner);
}
