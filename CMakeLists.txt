cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(BuildDir ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BuildDir})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BuildDir}/libs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BuildDir}/libs)

set( CMAKE_VERBOSE_MAKEFILE OFF )

project(Sane)

add_subdirectory(data_packer)
add_subdirectory(engine)
add_subdirectory(sandbox)
add_subdirectory(external/glfw)
