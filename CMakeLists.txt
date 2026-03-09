# ============================================================
# Root CMakeLists.txt — builds all Quant sub-projects
#
# Desktop:
#   cmake -B build && cmake --build build
#
# Android NDK (from Android Studio or command line):
#   cmake -B build-android \
#     -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
#     -DANDROID_ABI=arm64-v8a \
#     -DQUANT_ANDROID=ON
# ============================================================

cmake_minimum_required(VERSION 3.18)
project(Quant LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(QUANT_CUDA    "Enable CUDA GPU acceleration" OFF)
option(QUANT_ANDROID "Build for Android (JNI shared lib)" OFF)

# ---- Sub-projects ----
add_subdirectory(libquant)
add_subdirectory(libquant-engine)
add_subdirectory(libquant-mcp)
add_subdirectory(quant-ui)

if(QUANT_ANDROID)
    add_subdirectory(quant-android)
else()
    # Desktop CLI + HTTP server
    add_subdirectory(Quant)
    # MCP server for AI assistants
    add_subdirectory(quant-mcp)
endif()
