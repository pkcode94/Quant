# libquant — pure math library (header-only)
cmake_minimum_required(VERSION 3.18)
project(libquant LANGUAGES CXX)

add_library(libquant INTERFACE)
target_include_directories(libquant INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
