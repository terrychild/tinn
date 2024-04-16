#ifndef UTILS_H
#define UTILS_H

#define STRINGIZER(x) #x
#define STR(x) STRINGIZER(x)

#define __USE_XOPEN

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

void* allocate(void* data, size_t size);

#define IMF_DATE_LEN 30 // length of a date in Internet Messaging Format with null terminator
char* to_imf_date(char* buf, size_t max_len, time_t seconds);
time_t from_imf_date(const char* date, size_t len);

#endif