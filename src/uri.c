#include <string.h>

#include "uri.h"
#include "utils.h"
#include "console.h"

static const char* valid_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~%!$&\"()*+,;=:@";

static bool remove_dot_segment(URI* uri, bool last) {
	if (uri->segments_count>0) {
		if (strcmp(uri->segments[uri->segments_count-1], ".")==0) {
			if (last) {
				uri->segments[uri->segments_count-1][0] = '\0';
			} else {
				uri->segments_count--;
			}
		} else if (strcmp(uri->segments[uri->segments_count-1], "..")==0) {
			if (uri->segments_count>1) {
				if (last) {
					uri->segments_count--;
					uri->segments[uri->segments_count-1][0] = '\0';
				} else {
					uri->segments_count -= 2;
				}
			} else {
				uri->valid = false;
				return false;
			}
		}
	}
	return true;
}

URI* uri_new(Token token) {
	URI* uri = allocate(NULL, sizeof(*uri));

	uri->data = allocate(NULL, token.length + 1);
	memcpy(uri->data, token.start, token.length);
	uri->data[token.length] = '\0';

	uri->path = NULL;
	uri->path_len = 0;
	uri->query = uri->data + token.length;
	uri->query_len = 0;

	size_t max_segments = 8;
	uri->segments = allocate(NULL, max_segments * sizeof(*uri->segments));
	uri->segments_count = 0;

	uri->valid = true;

	// validate URI starts with a forward slash
	if (token.length==0 || uri->data[0]!='/') {
		uri->valid = false;
		return uri;
	}

	// scan the URI looking for segments (directories), the start of the query and invalid characters
	bool in_path = true;
	for (size_t i=0; i<token.length; i++) {
		switch (uri->data[i]) {
			case '/':
				if (in_path) {
					if (uri->segments_count == max_segments) {
						max_segments *= 2;
						uri->segments = allocate(uri->segments, max_segments * sizeof(*uri->segments));
					}
					uri->data[i] = '\0';
					if (!remove_dot_segment(uri, false)) {
						return uri;
					}
					uri->segments[uri->segments_count++] = uri->data + i + 1;
				}
				break;
			case '?':
				if (in_path) {
					uri->data[i] = '\0';
					uri->query = uri->data + i + 1;
					uri->query_len = token.length - i + 1;
					in_path = false;
				}
				break;
			default:
				if (strchr(valid_chars, uri->data[i])==NULL) {
					uri->valid = false;
					return uri;
				}
		}
	}

	// process last segement
	if (!remove_dot_segment(uri, true)) {
		return uri;
	}

	// build complete and rationalised path
	size_t lens[uri->segments_count];
	for (size_t i=0; i<uri->segments_count; i++) {
		lens[i] = strlen(uri->segments[i]);
		uri->path_len += 1 + lens[i];
	}
	uri->path = allocate(NULL, uri->path_len + 1);

	size_t pos = 0;
	for (size_t i=0; i<uri->segments_count; i++) {
		uri->path[pos] = '/';
		strcpy(uri->path + pos + 1, uri->segments[i]);
		pos += 1 + lens[i];
	}
	uri->path[pos] = '\0';

	// return URI
	return uri;
}
void uri_free(URI* uri) {
	if (uri != NULL) {
		free(uri->data);
		if (uri->path != NULL) {
			free(uri->path);
		}
		free(uri->segments);

		free(uri);
	}
}