add_library(lib
    cli.c
    log.c
    bits.c
    bytes.c

    mem/align.c
    mem/arena.c
    mem/mfile.c
    mem/memory.c
    mem/array.c
    mem/pool.c
    mem/buffer.c
    mem/slice.c

    sys/sockets.c
    sys/polling.c

    net/server.c
    net/echo.c
    net/url.c
    net/http.c
    net/tls.c

    maths/bigint.c

    crypto/sha.c
    crypto/ec.c
    crypto/mc.c
    crypto/x25519.c
)

target_include_directories(lib PRIVATE "${CMAKE_SOURCE_DIR}/src")