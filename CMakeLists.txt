cmake_minimum_required(VERSION 3.20)
project(tinn C)

# Auto-create a .gitignore in the specified build directory.
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)  
  file(GENERATE OUTPUT .gitignore CONTENT "*")
endif()

# Save the executable in the build/bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src/lib)
add_subdirectory(src/tinn)