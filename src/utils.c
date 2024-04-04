#include "utils.h"
#include "console.h"

void* allocate(void* data, size_t size) {
	void* new_data = realloc(data, size);
	if (new_data == NULL) {
		PANIC("unable to allocate memory");
	}
	return new_data;
}

// generate a date stamp in Internet Messaging Format
char* to_imf_date(char* buf, size_t max_len, time_t seconds) {
	strftime(buf, max_len, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&seconds));
	return buf;
}

time_t from_imf_date(const char* date, size_t len) {
	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	if (strptime(date, "%a, %d %b %Y %H:%M:%S GMT", &tm) == NULL) {
		ERROR("Invalid IMF date (%.*s)", len, date);
		return 0;
	}
	return mktime(&tm);
}