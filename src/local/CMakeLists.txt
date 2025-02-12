# Stream
set(TARGET stream)
add_executable(${TARGET} image.cpp box.cpp face.cpp main.cpp servos.cpp bark.cpp # stream.cpp 
                        ../servos/SMS_STS.cpp ../servos/SCS.cpp ../servos/SCSerial.cpp)

# Options
include(DefaultTargetOptions)

# SDL
target_link_libraries(${TARGET} PRIVATE common-sdl)

# Whisper
#target_link_libraries(${TARGET} PRIVATE common whisper ${CMAKE_THREAD_LIBS_INIT})

# Bark
target_link_libraries(${TARGET} PRIVATE bark common2)
target_compile_features(${TARGET} PRIVATE cxx_std_11)

