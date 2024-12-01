# Stream
set(TARGET stream)
add_executable(${TARGET} stream.cpp image.cpp box.cpp main.cpp)

include(DefaultTargetOptions)

target_link_libraries(${TARGET} PRIVATE common common-sdl whisper ${CMAKE_THREAD_LIBS_INIT})
