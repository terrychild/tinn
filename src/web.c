#include <stdlib.h>

#include "web.h"
#include "console.h"
#include "version.h"

int hostWebServer(int argc, char* argv[]) {
    LOG("Tinn Web Server %s (%s)", VERSION, BUILD_DATE);
    return EXIT_FAILURE;
}