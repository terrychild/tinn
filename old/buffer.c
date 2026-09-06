#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "utils.h"
#include "buffer.h"
#include "console.h"

Buffer* buf_new(long size) {
	Buffer* buf = allocate(NULL, sizeof(*buf));
	buf->size = size;
	buf->length = 0;
	buf->read_pos = 0;
	buf->data = allocate(NULL, buf->size);
	return buf;
}
Buffer* buf_new_file(const char* path) {
	Buffer* buf = buf_new(0);
	if (!buf_append_file(buf, path)) {
		ERROR("reading %s\n", path);
		buf_free(buf);
		return NULL;
	}
	return buf;
}

void buf_free(Buffer* buf) {
	if (buf != NULL) {
		free(buf->data);
		free(buf);
	}
}

void buf_reset(Buffer* buf) {
	buf->length = 0;
	buf->read_pos = 0;
}

static void ensure(Buffer* buf, long n) {
	long new_size = buf->size || 1;
	while (new_size < buf->length + n) {
		new_size *= 2;
	}
	if (new_size > buf->size) {
		buf->size = new_size;
		buf->data = allocate(buf->data, new_size);
	}
}
void buf_grow(Buffer* buf) {
	buf->size *= 2;
	buf->data = allocate(buf->data, buf->size);
}

void buf_append(Buffer* buf, const char* data, long n) {
	ensure(buf, n);

	memcpy(buf->data + buf->length, data, n);
	buf->length += n;
}
void buf_append_str(Buffer* buf, const char* str) {
	buf_append(buf, str, strlen(str));	
}
void buf_append_format(Buffer* buf, const char* format, ...) {
	long max = buf_write_max(buf);

	va_list args;
	va_start(args, format);
	long len = vsnprintf(buf_write_ptr(buf), max, format, args);
	va_end(args);

	if (len >= max) {
		ensure(buf, len+1);

		va_start(args, format);
		len = vsnprintf(buf_write_ptr(buf), len+1, format, args);
		va_end(args);
	}

	if (len < 0) {
		ERROR("unable to format string for buffer");
		return;
	}
	buf->length += len;
}
void buf_append_buf(Buffer* target, Buffer* source) {
	long len = source->length;
	ensure(target, len);

	memcpy(target->data + target->length, source->data, len);
	target->length += len;
}
bool buf_append_file(Buffer* buf, const char* path) {
	FILE *file = fopen(path, "rb");	
	if (file == NULL) {
		return false;
	}

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buf_ptr = buf_reserve(buf, length);
	if (buf_ptr == NULL) {
		return false;
	}

	fread(buf_ptr, 1, length, file);
	fclose(file);

	return true;
}

char* buf_reserve(Buffer* buf, long n) {
	ensure(buf, n);
	
	char* rv = buf->data + buf->length;
	buf->length += n;
	return rv;
}
void buf_consume(Buffer* buf, long n) {
	long new_len = buf->length - n;
	if (new_len > 0) {
		memmove(buf->data, buf->data+n, new_len);
		buf->length = new_len;
		buf_advance_read(buf, -n);
	} else {
		buf_reset(buf);
	}
}

char* buf_write_ptr(Buffer* buf) {
	return buf->data + buf->length;
}
long buf_write_max(Buffer* buf) {
	return buf->size - buf->length;
}
char* buf_advance_write(Buffer* buf, long offset) {
	if (buf->length + offset < 0) {
		buf->length = 0;
	} else if (buf->length + offset > buf->size) {
		buf->length = buf->size;
	} else {
		buf-> length += offset;
	}
	return buf_write_ptr(buf);
}

char* buf_read_ptr(Buffer* buf) {
	return buf->data + buf->read_pos;
}
long buf_read_max(Buffer* buf) {
	return buf->length - buf->read_pos;
}
char* buf_advance_read(Buffer* buf, long offset) {
	if (buf->read_pos + offset < 0) {
		buf->read_pos = 0;
	} else if (buf->read_pos + offset > buf->length) {
		buf->read_pos = buf->length;
	} else {
		buf-> read_pos += offset;
	}
	return buf_read_ptr(buf);
}

char* buf_as_str(Buffer* buf) {
	ensure(buf, 1);
	buf->data[buf->length] = '\0';
	return buf->data;
}