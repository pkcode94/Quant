# quant-mcp — MCP (Model Context Protocol) server for Quant Ledger
#
# Exposes the Quant engine as MCP tools that AI assistants can call.
# Transport: stdio (JSON-RPC 2.0 over stdin/stdout)
#
# Build:
#   cmake -B build && cmake --build build --target quant-mcp
#
# Run:
#   ./quant-mcp --db ./db
#
# Claude Desktop config (~/.config/claude/claude_desktop_config.json):
#   {
#     "mcpServers": {
#       "quant": {
#         "command": "/path/to/quant-mcp",
#         "args": ["--db", "/path/to/db"]
#       }
#     }
#   }

cmake_minimum_required(VERSION 3.18)
project(quant-mcp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(quant-mcp main.cpp)

target_include_directories(quant-mcp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libquant-mcp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libquant-engine/include
)

target_link_libraries(quant-mcp PRIVATE quant-mcp-lib)
