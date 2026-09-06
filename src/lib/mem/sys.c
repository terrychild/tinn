#include <unistd.h>
#include <sys/mman.h>

#include "lib/types.h"
#include "lib/console.h"

// align pointers/lengths to page/word boundaries
static bool isPowerOfTwo(U64 ptr) {
    return (ptr & (ptr-1)) == 0;
}
static U64 align(U64 ptr, U64 multiple) {
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

// system calls
// TODO: support more than linux?
void* sysMemReserve(U64 size) {
    void* mem = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        PANIC("Unable to allocate memory");
    }
    return mem;
}

void sysMemCommit(void* memory, U64 size) {
    if (mprotect(memory, size, PROT_READ | PROT_WRITE)) {
        PANIC("Unable to commit memory");
    }
}

void sysMemUncommit(void* memory, U64 size) {
    if (mprotect(memory, size, PROT_NONE)) {
        PANIC("Unable to uncommit memory");
    }
}

void sysMemRelease(void* memory, U64 size) {
    if (munmap(memory, size)) {
        ERROR("Unable to free memory");
    }
}