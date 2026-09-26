include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ConfigBuildDate.cmake")

add_executable(tinn
    main.c
    "${TIMESTAMP_DIR}/version.c" version.h
    help.c help.h    
)

add_dependencies(tinn UpdateBuildDate)

target_include_directories(tinn PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tinn PRIVATE lib)