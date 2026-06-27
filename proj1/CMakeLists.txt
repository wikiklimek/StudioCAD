cmake_minimum_required(VERSION 3.20)
project(MG1_Proj1 C CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_subdirectory("/home/wiktoria/projects/MG1/MathLib" "MathLib_build")

include_directories(
        external/glad/include
        external/imgui
        external/imgui/backends
)

file(GLOB IMGUI_SOURCES
        "external/imgui/*.cpp"
        "external/imgui/backends/imgui_impl_glfw.cpp"
        "external/imgui/backends/imgui_impl_opengl3.cpp"
)

add_executable(${PROJECT_NAME}
        src/main.cpp
        external/glad/src/glad.c
        ${IMGUI_SOURCES}
        src/shader.h
        olds/old_main.txt
)

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL glfw MathLib)