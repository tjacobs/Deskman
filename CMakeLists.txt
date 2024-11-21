# Project
cmake_minimum_required(VERSION 3.10)
project("Images" C CXX)
project("Images" VERSION 1.0.0)

# Set
set(SOVERSION 1)
set(CMAKE_WARN_UNUSED_CLI YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/whisper/cmake/")

# General
option(BUILD_SHARED_LIBS "build shared libraries" ${BUILD_SHARED_LIBS_DEFAULT})
option(WHISPER_CCACHE "whisper: use ccache if available" ON)

# Debug
option(WHISPER_ALL_WARNINGS          "whisper: enable all compiler warnings" ON)

# Sanitizers
option(WHISPER_SANITIZE_THREAD       "whisper: enable thread sanitizer"      OFF)
option(WHISPER_SANITIZE_ADDRESS      "whisper: enable address sanitizer"     OFF)
option(WHISPER_SANITIZE_UNDEFINED    "whisper: enable undefined sanitizer"   OFF)

# Third party libs
option(WHISPER_CURL                  "whisper: use libcurl"                  ON)
option(WHISPER_SDL2                  "whisper: support for libSDL2"          ON)
option(WHISPER_COREML                "whisper: enable Core ML framework"     OFF)
option(WHISPER_COREML_ALLOW_FALLBACK "whisper: allow non-CoreML fallback"    OFF)
option(WHISPER_OPENVINO              "whisper: support for OpenVINO"         OFF)
option(WHISPER_FFMPEG                "whisper: support ffmpeg  (avcodec)"    OFF)

# Override GGML options
set(GGML_CCACHE             ${WHISPER_CCACHE})
set(GGML_SANITIZE_THREAD    ${WHISPER_SANITIZE_THREAD})
set(GGML_SANITIZE_ADDRESS   ${WHISPER_SANITIZE_ADDRESS})
set(GGML_SANITIZE_UNDEFINED ${WHISPER_SANITIZE_UNDEFINED})
set(GGML_ALL_WARNINGS       ${WHISPER_ALL_WARNINGS})
set(GGML_FATAL_WARNINGS     ${WHISPER_FATAL_WARNINGS})

# Build
add_subdirectory(whisper/ggml)
add_subdirectory(whisper)
add_subdirectory(listen)
