cmake_minimum_required(VERSION 3.20)

project(MathLib CXX)

set(CMAKE_CXX_STANDARD 17)

file(GLOB MATH_SOURCES "source/*.cpp")
file(GLOB MATH_HEADERS "include/MG1Math/*.h")

add_library(MathLib STATIC ${MATH_SOURCES} ${MATH_HEADERS})

target_include_directories(MathLib PUBLIC include)
