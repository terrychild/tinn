add_executable(test
    main.c
    expect.c

    things.c
    types.c
    arena.c
    memory.c
    array.c
    pool.c
    buffer.c
    slice.c
    maths.c
    crypto.c
    tls.c
)

target_include_directories(test PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(test PRIVATE lib)