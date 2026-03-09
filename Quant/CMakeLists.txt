# Quant — Desktop CLI + HTTP server
cmake_minimum_required(VERSION 3.18)
project(Quant LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(quant Quant.cpp)

target_include_directories(quant PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib-master
    ${CMAKE_CURRENT_SOURCE_DIR}/../libquant/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libquant-mcp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libquant-engine/include
)

target_link_libraries(quant PRIVATE pthread quant-mcp-lib)

if(QUANT_CUDA)
    target_compile_definitions(quant PRIVATE QUANT_CUDA)
endif()
