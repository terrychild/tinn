#include "test/expect.h"

#include "lib/types.h"
#include "lib/cli.h"
#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/net/server.h"
#include "lib/net/tls.h"

void testTLS() {
    PRINT(CC_BLUE, "================\n TLS tests\n================\n");
    ScratchMemory scratch = scratchMemory(NULL);
    Memory* mem = scratch.mem;




    scratchRelease(scratch);
}