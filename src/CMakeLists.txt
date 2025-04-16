# Deskman robot
cmake_minimum_required(VERSION 3.10)
project(robot LANGUAGES CXX)
set(TARGET_NAME robot)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Create the executable from source files
add_executable(${TARGET_NAME}
    main.cpp
    face.cpp
    speak.cpp
    screen.cpp
    servos.cpp
    vector_renderer.cpp
    face_tracker.cpp
    camera.cpp
    ../servos/SMS_STS.cpp
    ../servos/SCS.cpp
    ../servos/SCSerial.cpp
)

# Include headers from include
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(/usr/local/include)

# Platform-specific includes and libraries
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux specific includes and libraries
    include_directories(/usr/include/SDL2)
    target_link_libraries(${TARGET_NAME} PRIVATE asound)
    target_link_libraries(${TARGET_NAME} PRIVATE SDL2 SDL2_image SDL2_ttf)
    
    # GStreamer for libcamera
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
    include_directories(${GSTREAMER_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} PRIVATE ${GSTREAMER_LIBRARIES})
elseif(APPLE)
    # macOS specific includes and libraries
    include_directories(/opt/homebrew/include)
    include_directories(/opt/homebrew/include/SDL2)
    include_directories(/opt/homebrew/Cellar/portaudio/19.7.0/include)
    link_directories(/opt/homebrew/lib)
    target_link_libraries(${TARGET_NAME} PRIVATE
        /opt/homebrew/lib/libSDL2.dylib
        /opt/homebrew/lib/libSDL2_image.dylib
        /opt/homebrew/lib/libSDL2_ttf.dylib
        -L/opt/homebrew/Cellar/portaudio/19.7.0/lib -lportaudio
        -Wl,-rpath,/opt/homebrew/lib
    )
endif()

# Common libraries
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(${TARGET_NAME}
    PRIVATE
        curl
        pv_porcupine
        -L${CMAKE_CURRENT_SOURCE_DIR}/lib -lwebsockets
        -Wl,-rpath,${CMAKE_CURRENT_SOURCE_DIR}/lib
        ${OpenCV_LIBS}
        )
