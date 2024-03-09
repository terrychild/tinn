#ifndef TINN_BUFFER_H
#define TINN_BUFFER_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	long size;
	long length;
	long read_pos;
	char* data;
} Buffer;

Buffer* buf_new(long size);
Buffer* buf_new_file(char* path);
void buf_free(Buffer* buf);

void buf_reset(Buffer* buf);

void buf_grow(Buffer* buf);

void buf_append(Buffer* buf, char* data, long n);
void buf_append_str(Buffer* buf, char* str);
void buf_append_format(Buffer* buf, char* format, ...);
void buf_append_buf(Buffer* target, Buffer* source);
bool buf_append_file(Buffer* buf, char* path);

char* buf_reserve(Buffer* buf, long n);
void buf_consume(Buffer* buf, long n);

char* buf_write_ptr(Buffer* buf);
long buf_write_max(Buffer* buf);
char* buf_advance_write(Buffer* buf, long offset);

char* buf_read_ptr(Buffer* buf);
long buf_read_max(Buffer* buf);
char* buf_advance_read(Buffer* buf, long offset);

char* buf_as_str(Buffer* buf);

#endif