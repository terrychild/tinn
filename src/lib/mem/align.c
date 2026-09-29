#include <stdlib.h>
#include <unistd.h>

#include "lib/mem/align.h"
#include "lib/log.h"

// align pointers/lengths to page/word boundaries
static bool isPowerOfTwo(U64 ptr) {
    return (ptr & (ptr-1)) == 0;
}
U64 align(U64 ptr, U64 multiple) {
    if (!isPowerOfTwo(multiple)) {
        PANIC("Alignment is not a power of two");
    }

    U64 mod = ptr & (multiple - 1);
    if (mod != 0) {
        ptr += multiple - mod;
    }
    return ptr;
}
U64 alignToPage(U64 ptr) {
    return align(ptr, sysconf(_SC_PAGE_SIZE));
}
U64 alignToWord(U64 ptr) {
    return align(ptr, sizeof(void*));
}