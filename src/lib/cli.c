#include <string.h>

bool cliArg(int argc, char* argv[], const char* name) {
    for (int i=0; i<argc; i++) {
        if (strcmp(argv[i], name)==0) {
            return true;
        }
    }
    return false;
}

char* cliValue(int argc, char* argv[], const char* name, const char* default_value) {
    size_t len = strlen(name);
    for (int i=0; i<argc; i++) {
        if (strncmp(argv[i], name, len)==0) {
            if (strlen(argv[i]) > len+1 && argv[i][len]=='=') {
                return &argv[i][len+1];
            }
        }
    }
    return default_value;
}