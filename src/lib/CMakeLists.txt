add_library(lib
    cli.c
    log.c
    bytes.c
    slice.c

    mem/arena.c
    mem/allocator.c
    mem/array.c
    mem/pool.c
    mem/buffer.c

    sys/sockets.c
    sys/polling.c

    net/server.c
    net/echo.c
    net/mime.c
    net/http.c    
)

target_include_directories(lib PRIVATE "${CMAKE_SOURCE_DIR}/src")