cmake_minimum_required(VERSION 3.16)

project(CMakeTemplate)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")
add_executable(CMakeTemplate "${SOURCES}" "Main.cpp")
target_include_directories(
    CMakeTemplate
    PUBLIC
    "."
    "src"
)

# PCH
target_precompile_headers(
    CMakeTemplate
    PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pch.h>"
)

target_compile_features(CMakeTemplate PUBLIC cxx_std_17)