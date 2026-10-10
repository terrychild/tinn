#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "static.h"
#include "lib/log.h"
#include "lib/mem/allocator.h"
#include "lib/mem/array.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"
#include "lib/net/url.h"

bool staticContent(HttpServerExchange* exchange) {
    // build a local path
    URL* target = exchange->request->target;
    Buffer* local_path = bufNew(exchange->scope, 1 + target->path.length + 11 + 1); // 1 for leading dot, 11 for possible /index.html, 1 for null terminator
    bufAppendStr(local_path, ".");
    bufAppendSlice(local_path, target->path);

    Slice last_segment = *(Slice*)arrayPeek(target->path_segments);

    if (last_segment.length == 0) {
        bufAppendStr(local_path, "index.html");
        last_segment = sliceFromStr("index.html");
    }

    // ignore dot files
    if (last_segment.length > 0 && last_segment.start[0] == '.') {
        DEBUG("Static: Ignoring dot file %s", local_path + 1);
        return false;
    }

    // get file information
	struct stat attrib;
    char* local_path_str = bufAsStr(local_path);
	if (stat(local_path_str, &attrib) != 0) {
		return false;
	}

    // check this is a GET or HEAD request
    if (!sliceIsStr(exchange->request->method, "GET") && !sliceIsStr(exchange->request->method, "HEAD")) {
        DEBUG("Method not allowed");
        httpServerAddHeader(exchange, sliceFromStr("Allow"), sliceFromStr("GET, HEAD"));
        httpServerSendError(exchange, HTTP_METHOD_NOT_ALLOWED);
        return true;
    }

    // is it a file or directory?
    if (S_ISREG(attrib.st_mode)) {
        LOG("Static: Serving %.*s to %s", target->path.length, target->path.start, exchange->connection->address);
        DEBUG("Static: Local file path is %s", local_path_str + 1);

        // check modified date
        if (exchange->request->if_modified_since > 0 && exchange->request->if_modified_since >= attrib.st_mtime) {
            DEBUG("Static: Use cached version");
            httpServerSendNotModified(exchange);
            return true;
        }

        // open file and get content length
        long length;
        FILE *file = fopen(local_path_str, "rb");

        if (file == NULL) {
            ERROR("Static: Unable to open file %s", local_path_str + 1);
            return false;
        }

        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);

        // respond
        httpServerAddHeader(exchange, sliceFromStr("Cache-Control"), sliceFromStr("no-cache"));
        httpServerAddDateHeader(exchange, sliceFromStr("Last-Modified"), attrib.st_mtime);

        Slice ext = sliceRightBackStr(last_segment, ".");
        if (sliceIsStr(exchange->request->method, "HEAD")) {
            httpServerSetContentHeaders(exchange, ext, length);
        } else {
            U8* file_content = allocate(exchange->scope, length);
            fread(file_content, 1, length, file);
            httpServerSetContent(exchange, ext, sliceNew(file_content, length));
        }
        fclose(file);
        httpServerSend(exchange);

        return true;

    } else if (S_ISDIR(attrib.st_mode)) {
        // check for index
        bufAppendStr(local_path, "/index.html");
        if (stat(local_path_str, &attrib) == 0) {
            if (S_ISREG(attrib.st_mode)) {
                Slice new_path = bufSlice(local_path, 1, target->path.length + 2);

                DEBUG("Static: Found local directory, redirecting to %.*s", new_path.length, new_path.start);

                httpServerSendRedirect(exchange, new_path);
                return true;
            }
        }
        DEBUG("Static: Found local directory but no index at %s", local_path_str + 1);
        return false;

    } else {
		ERROR("Unknown file mode (%d) for %s", attrib.st_mode, local_path_str + 1);
		return false;
	}
}