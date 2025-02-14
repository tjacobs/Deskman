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
include_directories(/opt/homebrew/Cellar/portaudio/19.7.0/include)

# Link the libraries needed
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(${TARGET_NAME}
    PRIVATE
        SDL2 SDL2_image curl
        pv_porcupine
        #asound
        -L${CMAKE_CURRENT_SOURCE_DIR}/lib -lwebsockets
        -L/opt/homebrew/Cellar/portaudio/19.7.0/lib -lportaudio
        -Wl,-rpath,${CMAKE_CURRENT_SOURCE_DIR}/lib
        )
