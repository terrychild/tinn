#include <sys/stat.h>
#include <string.h>

#include "static.h"
#include "console.h"

bool static_content(void* state, Request* request, Response* response) {
	(void)state; //un-used

	TRACE("checking static content");

	// build a local path
	char local_path[request->target->path_len + 1 + 11 + 1]; // 1 for leading dot, 11 for possible /index.html, 1 for null terminator
	local_path[0] = '.';
	strcpy(local_path + 1, request->target->path);

	char* last_segment = request->target->segments[request->target->segments_count-1];

	if (strlen(last_segment)==0) {
		last_segment = "index.html";
		strcpy(local_path + 1 + request->target->path_len, last_segment);
	}

	// ignore dot files
	if (last_segment[0]=='.') {
		TRACE("ignoring dot file \"%s\"", local_path);
		return false;
	}

	// get file information
	struct stat attrib;
	if (stat(local_path, &attrib) != 0) {
		TRACE("could not find \"%s\"", local_path);
		return false;
	}

	if (S_ISREG(attrib.st_mode)) {
		TRACE("found \"%s\"", local_path);

		// check this is a GET or HEAD request
		if (!token_is(request->method, "GET") && !token_is(request->method, "HEAD")) {
			TRACE("method not allowed");
			response_error(response, 405);
			response_header(response, "Allow", "GET, HEAD");
			return true;
		}

		// check modified date
		if (request->if_modified_since>0 && request->if_modified_since>=attrib.st_mtime) {
			TRACE("not modified, use cached version");
			response_status(response, 304);
			return true;
		}

		// open file and get content length
		long length;
		FILE *file = fopen(local_path, "rb");
		
		if (file == NULL) {
			ERROR("unable to open file \"%s\"", local_path);
			return false;
		}

		fseek(file, 0, SEEK_END);
		length = ftell(file);
		fseek(file, 0, SEEK_SET);

		// respond
		response_status(response, 200);
		response_header(response, "Cache-Control", "no-cache");
		response_date(response, "Last-Modified", attrib.st_mtime);

		char* ext = strrchr(last_segment, '.');
		if (token_is(request->method, "HEAD")) {
			repsonse_content_headers(response, ext, length);
		} else {
			char* body = buf_reserve(response_content(response, ext), length);
			fread(body, 1, length, file);
		}
		fclose(file);

		return true;

	} else if (S_ISDIR(attrib.st_mode)) {
		TRACE("found a directory \"%s\"", local_path);

		// check for index
		strcpy(local_path + 1 + request->target->path_len, "/index.html");
		if (stat(local_path, &attrib) == 0) {
			if (S_ISREG(attrib.st_mode)) {
				TRACE("found index, redirecting");

				char new_path[request->target->path_len+2];
				strcpy(new_path, request->target->path);
				strcpy(new_path + request->target->path_len, "/");

				response_redirect(response, new_path);
				return true;
			}
		}
		TRACE("no index");
		return false;
	} else {
		ERROR("unknown file mode for \"%s\": %d", local_path, attrib.st_mode);
		return false;
	}
}