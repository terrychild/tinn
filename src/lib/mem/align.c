#include <stdlib.h>
#include <unistd.h>

#include "lib/mem/align.h"
#include "lib/log.h"

// align pointers/lengths to page/word boundaries
static bool isPowerOfTwo(Size ptr) {
    return (ptr & (ptr-1)) == 0;
}
Size align(Size ptr, Size multiple) {
    if (!isPowerOfTwo(multiple)) {
        PANIC("Alignment is not a power of two");
    }

    Size mod = ptr & (multiple - 1);
    if (mod != 0) {
        ptr += multiple - mod;
    }
    return ptr;
}
Size alignToPage(Size ptr) {
    return align(ptr, sysconf(_SC_PAGE_SIZE));
}
Size alignToWord(Size ptr) {
    return align(ptr, sizeof(void*));
}