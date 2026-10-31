add_library(lib
    cli.c
    log.c
    bytes.c

    mem/align.c
    mem/arena.c
    mem/mfile.c
    mem/allocator.c
    mem/array.c
    mem/pool.c
    mem/buffer.c
    mem/slice.c

    maths/mpa.c
    maths/bigint.c

    sys/sockets.c
    sys/polling.c

    net/server.c
    net/echo.c
    net/http.c
    net/url.c

    crypto/ec.c
    crypto/mc.c
)

target_include_directories(lib PRIVATE "${CMAKE_SOURCE_DIR}/src")