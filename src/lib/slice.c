#include "lib/slice.h"

I8 sliceCmpStr(const Slice a, const char* b) {
    for (U64 i = 0; i<a.length; i++) {
        if (b[i] == '\0') {
            return 1;
        }
        if (a.start[i] == b[i]) {
            continue;
        }
        return b[i] - a.start[i];
    }
    if (b[a.length] != '\0') {
        return -1;
    }
    return 0;
}