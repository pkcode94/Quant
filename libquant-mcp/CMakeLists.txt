cmake_minimum_required(VERSION 3.18)
project(quant-mcp-lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(quant-mcp-lib STATIC src/quant_mcp.cpp)

target_include_directories(quant-mcp-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(quant-mcp-lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Quant
    ${CMAKE_CURRENT_SOURCE_DIR}/../libquant/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libquant-engine/include
)

target_link_libraries(quant-mcp-lib PUBLIC quant-engine)
