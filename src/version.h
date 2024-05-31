#ifndef VERSION_H
#define VERSION_H

#include "utils.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 11
#define VERSION_PATCH 1

#define VERSION "v" STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_PATCH)
extern const char* BUILD_DATE;

#endif