#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "static.h"
#include "lib/log.h"
#include "lib/mem/allocator.h"
#include "lib/mem/array.h"
#include "lib/net/url.h"
#include "lib/slice.h"

bool staticFileServer(HttpServerExchange* exchange) {
    // build a local path
    URL target = exchange->request->target;
    char local_path[1 + target.path.length + 11 + 1]; // 1 for leading dot, 11 for possible /index.html, 1 for null terminator
    local_path[0] = '.';
    strncpy(local_path + 1, (const char*)target.path.start, target.path.length);
    local_path[1 + target.path.length] = '\0';

    Slice last_segment = *(Slice*)arrayPeek(target.path_segments);

    if (last_segment.length == 0) {
        strcpy(local_path + 1 + target.path.length, "index.html");
        last_segment = sliceFromStr("index.html");
    }

    // ignore dot files
    if (last_segment.length > 0 && last_segment.start[0] == '.') {
        DEBUG("Ignoring dot file \"%s\"", local_path);
        return false;
    }

    // get file information
	struct stat attrib;
	if (stat(local_path, &attrib) != 0) {
		DEBUG("Could not find \"%s\"", local_path);
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
        // check modified date
        if (exchange->request->if_modified_since > 0 && exchange->request->if_modified_since >= attrib.st_mtime) {
            DEBUG("Local file not modified, use cached version of \"%s\"", local_path);
            httpServerSendNotModified(exchange);
            return true;
        }

        // open file and get content length
        long length;
        FILE *file = fopen(local_path, "rb");

        if (file == NULL) {
            ERROR("Unable to open file \"%s\"", local_path);
            return false;
        }
        DEBUG("Serving local file \"%s\"", local_path);

        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);

        // respond
        httpServerAddHeader(exchange, sliceFromStr("Cache-Control"), sliceFromStr("no-cache"));
        httpServerAddDateHeader(exchange, sliceFromStr("Last-Modified"), attrib.st_mtime);

        Slice ext = sliceRightBackStr(last_segment, ".");
        if (sliceIsStr(exchange->request->method, "HEAD")) {
            httpServerSetContentType(exchange, ext);
            //TODO: set content length header
        } else {
            Slice file_content = sliceNew(exchange->scope, length);
            fread((char*)file_content.start, 1, length, file);
            httpServerSetContent(exchange, ext, file_content);
        }
        fclose(file);
        httpServerSend(exchange);

        return true;

    } else if (S_ISDIR(attrib.st_mode)) {
        // check for index
        strcpy(local_path + 1 + target.path.length, "/index.html");
        if (stat(local_path, &attrib) == 0) {
            if (S_ISREG(attrib.st_mode)) {
                Slice new_path = sliceNew(exchange->scope, target.path.length + 1);
                memcpy((char*)new_path.start, target.path.start, target.path.length);
                ((char*)new_path.start)[target.path.length] = '/';

                DEBUG("Found local directory, redirecting to \"%.*s\"", new_path.length, new_path.start);

                httpServerSendRedirect(exchange, new_path);
                return true;
            }
        }
        DEBUG("Found local directory but no index at \"%s\"", local_path);
        return false;

    } else {
		ERROR("Unknown file mode (%d) for \"%s\"", attrib.st_mode, local_path);
		return false;
	}
}