# Deskman robot
cmake_minimum_required(VERSION 3.10)
project(robot LANGUAGES CXX)
set(TARGET_NAME robot)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create the executable from source files
add_executable(${TARGET_NAME}
    main.cpp
    face.cpp
    speak.cpp
    screen.cpp
    servos.cpp
    ../servos/SMS_STS.cpp
    ../servos/SCS.cpp
    ../servos/SCSerial.cpp
)

# Include headers from include
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(/usr/local/include)
include_directories(/opt/homebrew/include)
include_directories(/opt/homebrew/include/SDL2)
include_directories(/opt/homebrew/Cellar/portaudio/19.7.0/include)

# Add library search paths
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
link_directories(/opt/homebrew/lib)

# Platform-specific libraries
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux specific libraries
    target_link_libraries(${TARGET_NAME} PRIVATE asound)
endif()

# Link the libraries needed
target_link_libraries(${TARGET_NAME}
    PRIVATE
        /opt/homebrew/lib/libSDL2.dylib
        /opt/homebrew/lib/libSDL2_image.dylib
        curl
        pv_porcupine
        -L${CMAKE_CURRENT_SOURCE_DIR}/lib -lwebsockets
        -L/opt/homebrew/Cellar/portaudio/19.7.0/lib -lportaudio
        -Wl,-rpath,${CMAKE_CURRENT_SOURCE_DIR}/lib
        -Wl,-rpath,/opt/homebrew/lib
        )
