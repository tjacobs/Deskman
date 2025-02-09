# Speak
cmake_minimum_required(VERSION 3.10)
project(speak LANGUAGES CXX)
set(TARGET_NAME speak)

# Create the executable from source files
add_executable(${TARGET_NAME}
    speak.cpp
    ../src/image.cpp
    ../src/face.cpp
    ../src/servos.cpp
    ../servos/SMS_STS.cpp
    ../servos/SCS.cpp
    ../servos/SCSerial.cpp
)

# Include headers from include
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Link the libraries needed
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(${TARGET_NAME}
    PRIVATE
        SDL2 SDL2_image curl
        websockets
        pv_porcupine
        asound
)
