
project(Sandbox)

file(GLOB_RECURSE SRC_FILES
    src/*.c
    src/*.cpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
endif()

add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/internal
)

target_link_libraries(${PROJECT_NAME} 
    SaneEngine
    ${CMAKE_SOURCE_DIR}/external/vulkan/Lib/vulkan-1.lib
    glfw
)

add_dependencies(${PROJECT_NAME} 
    DataPacker
)

add_custom_command(TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMAND powershell -ExecutionPolicy Unrestricted ${PROJECT_SOURCE_DIR}/CopyFiles.ps1 -SRC ${PROJECT_SOURCE_DIR}/resources/ -DEST $<TARGET_FILE_DIR:Sandbox>/resources/
)
