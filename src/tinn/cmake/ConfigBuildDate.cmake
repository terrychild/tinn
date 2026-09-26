set(TIMESTAMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/timestamp")
file(MAKE_DIRECTORY ${TIMESTAMP_DIR})
set(STAMP_FILE "${TIMESTAMP_DIR}/timestamp.stamp")

add_custom_command(
    OUTPUT "${TIMESTAMP_DIR}/version.c"
    COMMAND ${CMAKE_COMMAND} -D TEMPLATE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/version.c.template"
                             -D OUTPUT_FILE="${TIMESTAMP_DIR}/version.c"
                             -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBuildDate.cmake"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/version.c.template" ${STAMP_FILE}
    COMMENT "Updating build date"
)

add_custom_target(UpdateBuildDate 
    COMMAND ${CMAKE_COMMAND} -E touch ${STAMP_FILE}
)

configure_file(version.h "${TIMESTAMP_DIR}/version.h" COPYONLY)