# ============================================================
# NDK CMakeLists.txt — builds all C++ into one shared library
#
# Compiles: quant_engine + quant_ui + JNI bridges
# Includes: Quant/ headers (Trade, QuantMath, TradeDatabase, etc.)
# Output:   libquant-native.so
# ============================================================
cmake_minimum_required(VERSION 3.22.1)
project(quant-native LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Repo root is 5 levels up from this CMakeLists.txt
# quant-app/app/src/main/cpp/ → repo root
set(REPO ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)

set(QUANT_DIR      ${REPO}/Quant)
set(ENGINE_DIR     ${REPO}/libquant-engine)
set(UI_DIR         ${REPO}/quant-ui)
set(LIBQUANT_DIR   ${REPO}/libquant)
set(MCP_DIR        ${REPO}/libquant-mcp)

# ---- Single shared library with everything ----
add_library(quant-native SHARED
    ${ENGINE_DIR}/src/quant_engine.cpp
    ${UI_DIR}/src/quant_ui.cpp
    ${UI_DIR}/jni/quant_ui_jni.cpp
    ${UI_DIR}/jni/quant_mcp_jni.cpp
    ${MCP_DIR}/src/quant_mcp.cpp
    ${REPO}/quant-android/jni/quant_jni.cpp
)

target_include_directories(quant-native PRIVATE
    ${ENGINE_DIR}/include
    ${UI_DIR}/include
    ${MCP_DIR}/include
    ${QUANT_DIR}
    ${QUANT_DIR}/cpp-httplib-master
    ${LIBQUANT_DIR}/include
)

target_compile_definitions(quant-native PRIVATE QUANT_PUBLIC_BUILD)

target_link_libraries(quant-native
    log
)
