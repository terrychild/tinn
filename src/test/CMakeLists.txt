add_executable(test
    main.c
    expect.c

    types.c
    arena.c
    allocator.c
    array.c
    pool.c
    buffer.c
)

target_include_directories(test PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(test PRIVATE lib)