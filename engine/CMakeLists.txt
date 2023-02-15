
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

project(SaneEngine)

file(GLOB_RECURSE SRC_FILES
    src/*.c
    src/*.cpp
)

add_library(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/entt/single_include
    ${CMAKE_SOURCE_DIR}/external/glfw/include
    ${CMAKE_SOURCE_DIR}/external/glm
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/spdlog/include
    ${CMAKE_SOURCE_DIR}/external/
    ${CMAKE_SOURCE_DIR}/external/stb
    ${CMAKE_SOURCE_DIR}/external/tinyobjloader
    ${CMAKE_SOURCE_DIR}/external/tinyxml2
    ${CMAKE_SOURCE_DIR}/external/vulkan/include
)

add_dependencies(${PROJECT_NAME} 
    DataPacker
)

add_custom_command(TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:DataPacker> ${PROJECT_SOURCE_DIR}
    COMMAND powershell -ExecutionPolicy Unrestricted ${PROJECT_SOURCE_DIR}/CopyFiles.ps1 -SRC ${PROJECT_SOURCE_DIR} -DEST ${CMAKE_SOURCE_DIR}/internal/${PROJECT_NAME}
    COMMAND powershell -ExecutionPolicy Unrestricted ${PROJECT_SOURCE_DIR}/CopyExternal.ps1 -SRC ${CMAKE_SOURCE_DIR}/external -DEST ${CMAKE_SOURCE_DIR}/internal/${PROJECT_NAME}
)
