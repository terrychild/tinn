#include "response.h"
#include "utils.h"

Response* response_new() {
	Response* response = allocate(NULL, sizeof(*response));
	response->buf = buf_new(1024);
	return response;	
}

void response_free(Response* response) {
	if (response!=NULL) {
		buf_free(response->buf);
		free(response);
	}
}