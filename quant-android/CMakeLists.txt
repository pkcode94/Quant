# quant-android — JNI bridge + shared library for Android
#
# Used by Android Studio via externalNativeBuild in build.gradle:
#   android {
#       externalNativeBuild {
#           cmake { path "../../quant-android/CMakeLists.txt" }
#       }
#   }
cmake_minimum_required(VERSION 3.18)
project(quant-android LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The JNI shared library that Android loads
add_library(quant-jni SHARED
    jni/quant_jni.cpp
)

# Engine and UI includes
set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libquant-engine)
set(UI_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/../quant-ui)
set(QUANT_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../Quant)

target_include_directories(quant-jni PRIVATE
    ${ENGINE_DIR}/include
    ${UI_DIR}/include
    ${QUANT_DIR}
    ${QUANT_DIR}/cpp-httplib-master
    ${CMAKE_CURRENT_SOURCE_DIR}/../libquant/include
)

# Link the engine and UI libraries
target_link_libraries(quant-jni PRIVATE quant-engine quant-ui log)
