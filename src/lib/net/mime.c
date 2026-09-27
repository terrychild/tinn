#include <string.h>

const char* mimeFromExt(char* ext) {
	if (ext != NULL && strlen(ext) > 0) {
		if (ext[0] == '.') {
			ext += 1;
		}

		if (strcmp(ext, "html")==0 || strcmp(ext, "htm")==0) {
			return "text/html; charset=utf-8";
		} else if (strcmp(ext, "css")==0) {
			return "text/css; charset=utf-8";
		} else if (strcmp(ext, "js")==0) {
			return "text/javascript; charset=utf-8";
		} else if (strcmp(ext, "jpeg")==0 || strcmp(ext, "jpg")==0) {
			return "image/jpeg";
		} else if (strcmp(ext, "png")==0) {
			return "image/png";
		} else if (strcmp(ext, "gif")==0) {
			return "image/gif";
		} else if (strcmp(ext, "bmp")==0) {
			return "image/bmp";
		} else if (strcmp(ext, "svg")==0) {
			return "image/svg+xml";
		} else if (strcmp(ext, "ico")==0) {
			return "image/vnd.microsoft.icon";
		} else if (strcmp(ext, "mp3")==0) {
			return "audio/mpeg";
		}
	}

	return "text/plain; charset=utf-8";
}