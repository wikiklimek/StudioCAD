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
        src/shapes/headers/shader.h
        src/shapes/headers/torusGrid.h
        src/shapes/source/torusGrid.cpp
        src/shapes/headers/matrixesModelViewProjection.h
        src/shapes/source/matrixesModelViewProjection.cpp
        src/shapes/headers/axis.h
        src/shapes/source/axis.cpp
        src/shapes/headers/transformations.h
        src/shapes/headers/sceneObject.h
        src/shapes/headers/sceneTorus.h
        src/shapes/headers/scenePoint.h
        src/shapes/headers/cursor.h
        src/shapes/headers/previewFunctions.h
        src/shapes/source/previewFunctions.cpp
        src/shapes/source/scenePoint.cpp
        src/shapes/source/sceneTorus.cpp
        src/shapes/headers/screenInteractions.h
        src/shapes/source/screenInteractions.cpp
        src/shapes/headers/quaternion.h
        src/shapes/headers/bakeTransform.h
        src/shapes/source/cursor.cpp
        src/shapes/source/quaternion.cpp
        src/shapes/headers/camera.h
        src/shapes/headers/sceneBezierC0.h
        src/shapes/source/sceneBezierC0.cpp
        src/shapes/headers/guiManager.h
        src/shapes/headers/enums.h
        src/shapes/headers/previewContext.h
        src/shapes/headers/transformManager.h
        src/shapes/source/guiManager.cpp
        src/shapes/source/transformManager.cpp
        src/shapes/headers/appState.h
        src/shapes/headers/sceneBezier.h
        src/shapes/source/sceneBezier.cpp
        src/shapes/headers/sceneBezierC2.h
        src/shapes/source/sceneBezierC2.cpp
        src/shapes/headers/sceneSplineInterpolating.h
        src/shapes/source/sceneSplineInterpolating.cpp
        src/shapes/headers/selectionManager.h
        src/shapes/source/selectionManager.cpp
        src/shapes/headers/VirtualPoints.h
)

# Wskazujemy kompilatorowi dodatkowe foldery z plikami nagłówkowymi (.h)
target_include_directories(${PROJECT_NAME} PRIVATE src/shapes/headers)

# Łączymy projekt z bibliotekami systemowymi
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL glfw MathLib)
