#include <string.h>

#include "lib/net/url.h"
#include "lib/mem/array.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"

enum parse_state {
    PARSE_PATH,
    PARSE_QUERY,
    PARSE_FRAGMENT
};

static const char* valid_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~%!$&'()*+,;=:@";

static void addPathSegment(URL* url, Slice source, U64 start, U64 end, bool is_last) {
    if (end >= start) {
        Slice segment = slice(source, start, end);
        if (sliceIsStr(segment, ".")) {
            if (is_last) {
                segment.length = 0;
                arrayPush(url->path_segments, &segment);
            }
        } else if (sliceIsStr(segment, "..")) {
            if (url->path_segments->count > 0) {
                arrayPop(url->path_segments);
                if (is_last) {
                    segment.length = 0;
                    arrayPush(url->path_segments, &segment);
                }
            } else {
                url->valid = false;
            }
        } else {
            arrayPush(url->path_segments, &segment);
        }
    }
}
static void addPath(Allocator* allocator, URL* url, U64 length) {
    Buffer* path = bufNew(allocator, length);
    for (U64 i=0; i<url->path_segments->count; i++) {
        bufAppendStr(path, "/");
        bufAppendSlice(path, *(Slice*)arrayGet(url->path_segments, i));
    }
    url->path = bufAsSlice(path);
}

URL urlParseOrigin(Allocator* allocator, Slice source) {
    URL url = {0};
    url.path_segments = arrayNew(allocator, sizeof(Slice), 8);
    url.valid = true;

    // validate path starts with a forward slash
	if (source.length == 0 || source.start[0] != '/') {
		url.valid = false;
		return url;
	}

    // scan the URL looking for segments (directories), the start of the query and invalid characters
    enum parse_state state = PARSE_PATH;
    U64 component_start = 0;
    U64 segment_start = 1;
	for (U64 i=1; i<source.length; i++) {
		switch (source.start[i]) {
			case '/':
				if (state == PARSE_PATH) {
                    addPathSegment(&url, source, segment_start, i, false);
                    segment_start = i + 1;
                }
				break;
			case '?':
				if (state == PARSE_PATH) {
                    addPathSegment(&url, source, segment_start, i, true);
                    addPath(allocator, &url, i - component_start);
                    component_start = i + 1;
					state = PARSE_QUERY;
				}
				break;
            case '#':
                if (state == PARSE_PATH) {
                    addPathSegment(&url, source, segment_start, i, true);
                    addPath(allocator, &url, i - component_start);
                } else if (state == PARSE_QUERY) {
                    url.query = slice(source, component_start, i);
                }
                component_start = i + 1;
                state = PARSE_FRAGMENT;
                break;
			default:
				if (strchr(valid_chars, source.start[i]) == NULL) {
					url.valid = false;
					return url;
				}
		}
	}
    // finished parse last segment
    if (state == PARSE_PATH) {
        addPathSegment(&url, source, segment_start, -1, true);
        addPath(allocator, &url, source.length - component_start);
    } else if (state == PARSE_QUERY) {
        url.query = slice(source, component_start, -1);
    }

    return url;
}