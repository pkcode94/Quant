# quant-ui — Virtual router / GUI provider
#
# Renders HTML pages from engine state.  No HTTP dependency.
# Desktop: HTTP server adapter calls qui_get/qui_post.
# Android: Java calls via JNI, renders in WebView.
cmake_minimum_required(VERSION 3.18)
project(quant-ui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QUANT_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../Quant)
set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libquant-engine)

if(QUANT_ANDROID)
    add_library(quant-ui SHARED
        src/quant_ui.cpp
        jni/quant_ui_jni.cpp
    )
    target_link_libraries(quant-ui PRIVATE quant-engine log)
else()
    add_library(quant-ui STATIC
        src/quant_ui.cpp
    )
    target_link_libraries(quant-ui PRIVATE quant-engine)
endif()

target_include_directories(quant-ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(quant-ui PRIVATE
    ${ENGINE_DIR}/include
    ${QUANT_DIR}
    ${QUANT_DIR}/cpp-httplib-master
    ${CMAKE_CURRENT_SOURCE_DIR}/../libquant/include
)

target_compile_features(quant-ui PUBLIC cxx_std_17)
