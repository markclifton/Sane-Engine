
project(DataPacker)

file(GLOB_RECURSE SRC_FILES
    src/*.c
    src/*.cpp
)

add_executable(${PROJECT_NAME} ${SRC_FILES})
