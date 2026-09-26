add_library(lib
    types.h
    macros.h
    cli.c cli.h
    log.c log.h
    bytes.c bytes.h
    slice.c slice.h

    mem/arena.c mem/arena.h
    mem/allocator.c mem/allocator.h
    mem/array.c mem/array.h
    mem/pool.c mem/pool.h
    mem/buffer.c mem/buffer.h

    sys/sockets.c sys/sockets.h
    sys/polling.c sys/polling.h

    net/server.c net/server.h
    net/echo.c net/echo.h
    net/http.c net/http.h
)

target_include_directories(lib PRIVATE "${CMAKE_SOURCE_DIR}/src")