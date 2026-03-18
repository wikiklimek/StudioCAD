cmake_minimum_required(VERSION 3.20)
project(MG1_Project C CXX)

# Ustawiamy standard C++17
set(CMAKE_CXX_STANDARD 17)

# Szukamy pakietów zainstalowanych wcześniej przez DNF na RHEL 10
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_subdirectory("/home/wiktoria/projects/MG1/MathLib" "MathLib_build")

# Wskazujemy, gdzie leżą nagłówki GLAD i ImGui
include_directories(
        external/glad/include
        external/imgui
        external/imgui/backends
)

# Zbieramy pliki źródłowe interfejsu
file(GLOB IMGUI_SOURCES
        "external/imgui/*.cpp"
        "external/imgui/backends/imgui_impl_glfw.cpp"
        "external/imgui/backends/imgui_impl_opengl3.cpp"
)

# Tworzymy plik wykonywalny z naszego main.cpp, GLAD i ImGui
add_executable(${PROJECT_NAME}
        src/main.cpp
        external/glad/src/glad.c
        ${IMGUI_SOURCES}
        src/shader.h
        src/shapes/headers/torus.h
        src/shapes/source/torus.cpp
        src/shapes/headers/matrixesModelViewProjection.h
        src/shapes/source/matrixesModelViewProjection.cpp
        src/shapes/headers/grid.h
        src/shapes/source/grid.cpp
)

# Wskazujemy kompilatorowi dodatkowe foldery z plikami nagłówkowymi (.h)
target_include_directories(${PROJECT_NAME} PRIVATE src/shapes/headers)

# Łączymy projekt z bibliotekami systemowymi
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL glfw MathLib)
