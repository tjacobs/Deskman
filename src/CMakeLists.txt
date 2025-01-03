# Stream
set(TARGET stream)
add_executable(${TARGET} stream.cpp image.cpp box.cpp face.cpp main.cpp servos.cpp 
                        ../servos/SMS_STS.cpp ../servos/SCS.cpp ../servos/SCSerial.cpp)

include(DefaultTargetOptions)

target_link_libraries(${TARGET} PRIVATE common common-sdl whisper ${CMAKE_THREAD_LIBS_INIT})
