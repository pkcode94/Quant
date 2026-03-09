# libquant-engine — persistence + business logic library
#
# Builds as a static lib for desktop, shared lib for Android.
# Exposes a C API (quant_engine.h) suitable for JNI binding.
cmake_minimum_required(VERSION 3.18)
project(libquant-engine LANGUAGES CXX)

set(ENGINE_SOURCES
    src/quant_engine.cpp
)

set(ENGINE_HEADERS
    include/quant_engine.h
    include/quant_engine_types.h
)

# Quant/ headers are the implementation; engine wraps them
set(QUANT_HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Quant)

if(QUANT_ANDROID)
    add_library(quant-engine SHARED ${ENGINE_SOURCES})
else()
    add_library(quant-engine STATIC ${ENGINE_SOURCES})
endif()

target_include_directories(quant-engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(quant-engine PRIVATE
    ${QUANT_HEADERS_DIR}
    ${QUANT_HEADERS_DIR}/cpp-httplib-master
    ${CMAKE_CURRENT_SOURCE_DIR}/../libquant/include
)

target_compile_features(quant-engine PUBLIC cxx_std_17)

# httplib is only needed by the desktop Quant app, not the engine.
# The engine uses TradeDatabase which needs json.h (nanojson) only.
# No network dependencies.
