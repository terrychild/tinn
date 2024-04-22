#include "request.h"
#include "utils.h"

Request* request_new() {
	Request* request = allocate(NULL, sizeof(*request));
	request->buf = buf_new(1024);
	request_reset_headers(request);
	return request;	
}

void request_free(Request* request) {
	if (request!=NULL) {
		buf_free(request->buf);
		free(request);
	}
}

void request_reset_headers(Request* request) {
	request->h_if_modified_Since = 0;
}