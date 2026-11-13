#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "static.h"
#include "lib/log.h"
#include "lib/mem/memory.h"
#include "lib/mem/array.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"
#include "lib/net/url.h"

bool staticContent(Memory* exchange_mem, HttpConnection* connection, HttpServerRequest* request, __attribute__((unused)) void* context) {
    // build a local path
    URL* target = request->target;
    Buffer* local_path = bufNew(exchange_mem, 1 + target->path.length + 11 + 1); // 1 for leading dot, 11 for possible /index.html, 1 for null terminator
    bufAppend(local_path, ".");
    bufAppend(local_path, target->path);

    Slice last_segment = *(Slice*)arrayPeek(target->path_segments);

    if (last_segment.length == 0) {
        bufAppend(local_path, "index.html");
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
    if (!sliceIsStr(request->method, "GET") && !sliceIsStr(request->method, "HEAD")) {
        DEBUG("Method not allowed");
        httpServerAddHeader(connection, sliceFromStr("Allow"), sliceFromStr("GET, HEAD"));
        httpServerSendError(connection, HTTP_METHOD_NOT_ALLOWED);
        return true;
    }

    // is it a file or directory?
    if (S_ISREG(attrib.st_mode)) {
        LOG("Static: Serving %.*s", target->path.length, target->path.start);
        DEBUG("Static: Local file path is %s", local_path_str + 1);

        // check modified date
        if (request->if_modified_since > 0 && request->if_modified_since >= attrib.st_mtime) {
            DEBUG("Static: Use cached version");
            httpServerSendNotModified(connection);
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
        httpServerAddHeader(connection, sliceFromStr("Cache-Control"), sliceFromStr("no-cache"));
        httpServerAddDateHeader(connection, sliceFromStr("Last-Modified"), attrib.st_mtime);

        Slice ext = sliceRightBackStr(last_segment, ".");
        if (sliceIsStr(request->method, "HEAD")) {
            httpServerSetContentHeaders(connection, ext, length);
        } else {
            U8* file_content = allocate(exchange_mem, length);
            fread(file_content, 1, length, file);
            httpServerSetContent(connection, ext, sliceNew(file_content, length));
        }
        fclose(file);
        httpServerSend(connection);

        return true;

    } else if (S_ISDIR(attrib.st_mode)) {
        // check for index
        bufAppend(local_path, "/index.html");
        if (stat(local_path_str, &attrib) == 0) {
            if (S_ISREG(attrib.st_mode)) {
                Slice new_path = slice(local_path, 1, target->path.length + 2);

                DEBUG("Static: Found local directory, redirecting to %.*s", new_path.length, new_path.start);

                httpServerSendRedirect(connection, new_path);
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