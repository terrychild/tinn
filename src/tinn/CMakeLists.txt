# build date
set(VERSION_DIR "${CMAKE_CURRENT_BINARY_DIR}/version")
file(MAKE_DIRECTORY ${VERSION_DIR})

add_custom_target(generate_build_date
    COMMAND ${CMAKE_COMMAND} -D TEMPLATE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/version.h.template"
                             -D OUTPUT_FILE="${VERSION_DIR}/version.h"
                             -P "${CMAKE_CURRENT_SOURCE_DIR}/WriteBuildDate.cmake"
    BYPRODUCTS "${VERSION_DIR}/version.h"
    COMMENT "Generating fresh build timestamp..."
)

# main executable
add_executable(tinn
    main.c
    help.c help.h    
)

add_dependencies(tinn generate_build_date)

target_include_directories(tinn PRIVATE ${VERSION_DIR})
target_include_directories(tinn PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tinn PRIVATE lib)