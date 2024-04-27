#include <sys/stat.h>
#include <string.h>

#include "static.h"
#include "console.h"

bool static_content(Request* request, Response* response) {
	TRACE("checking static content");
	DEBUG("the path is \"%s\"", request->target->path + 1);
	DEBUG("the last segment is \"%s\"", request->target->segments[request->target->segments_count-1]);

	/*TODO
	token_is(method, "GET");
	response_simple_status(response, 405, "Oops, that method is not allowed.");
	*/

	struct stat attrib;
    if (stat(request->target->path + 1, &attrib) != 0) {
    	return false;
    }

    /*TODO:
    if (state->h_if_modified_Since>0 && state->h_if_modified_Since>=attrib.st_mtime) {
    	TRACE_DETAIL("not modified");
    	start_headers(state->out, "304", "Not Modified");
    	end_headers(state->out);
    	return true;
    }
    */

    if (S_ISREG(attrib.st_mode)) {
    	DEBUG("found the file");

    	// open file and get content length
		long length;
		FILE *file = fopen(request->target->path + 1, "rb");
		
		if (file == NULL) {
			return false;
		}

		fseek(file, 0, SEEK_END);
		length = ftell(file);
		fseek(file, 0, SEEK_SET);

		// response
		response_reset(response);
		response_status(response, 200);
		response_header(response, "Cache-Control", "no-cache");
		response_date(response, "Last-Modified", attrib.st_mtime);

		char* body = buf_reserve(response_content(response, strrchr(request->target->segments[request->target->segments_count-1], '.')), length);
		fread(body, 1, length, file);
		fclose(file);

		return true;

    } else if (S_ISDIR(attrib.st_mode)) {
    	WARN("it's a directory");
    }

	return false;
}