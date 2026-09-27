cmake_minimum_required(VERSION 3.20)
project(tinn C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Auto-create a .gitignore in the specified build directory.
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)  
  file(GENERATE OUTPUT .gitignore CONTENT "*")
endif()

# Save the executable in the build/bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src/lib)
add_subdirectory(src/test)
add_subdirectory(src/tinn)