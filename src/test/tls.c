#include "test/expect.h"

#include "lib/types.h"
#include "lib/cli.h"
#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/net/server.h"
#include "lib/net/tls.h"

typedef struct {
    Memory* mem;
} FakeServer;

static void send(void* context, Slice data) {
    FakeServer* server = context;
    expect("Response", data, sliceFromHex(server->mem, "160303007a020000760303707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f20e0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff130200002e002b0002030400330024001d00209fd7ad6dcff4298dd3f96d5b1b2af910a0535b1488d7f8fabb349a982880b615"));
}

static void addListener(void* server_context, ServerConnectedFunc onConnect, void* context) {
    FakeServer* server = server_context;
    ServerInbound interface = onConnect(context, (ServerOutbound) {
        .mem = server->mem,
        .send = send,
        .context = server_context,
    });

    serverRecived(interface, sliceFromHex(server->mem, "16030100f8010000f40303000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20e0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff000813021303130100ff010000a30000001800160000136578616d706c652e756c666865696d2e6e6574000b000403000102000a00160014001d0017001e0019001801000101010201030104002300000016000000170000000d001e001c040305030603080708080809080a080b080408050806040105010601002b0003020304002d00020101003300260024001d0020358072d6365880d1aeea329adf9121383851ed21a28e3b75e965d0d2cd166254"));
}

void testTLS() {
    PRINT(CC_BLUE, "================\n TLS tests\n================\n");
    ScratchMemory scratch = scratchMemory(NULL);
    Memory* mem = scratch.mem;

    FakeServer fake_server = {
        .mem = mem,
    };
    tlsServer(mem, (ServerInterface) {
        .addListener = addListener,
        .context = &fake_server,
    });

    scratchRelease(scratch);
}