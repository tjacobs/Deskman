cmake_minimum_required(VERSION 3.10)
project(face_tracker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)

# Try to find libcamera
pkg_check_modules(LIBCAMERA libcamera)
if(NOT LIBCAMERA_FOUND)
    message(FATAL_ERROR "libcamera not found. Please install libcamera development package.")
endif()

# Try to find MediaPipe
find_path(MEDIAPIPE_INCLUDE_DIR mediapipe/framework/calculator_graph.h
    PATHS /usr/include /usr/local/include
    PATH_SUFFIXES mediapipe
)
if(NOT MEDIAPIPE_INCLUDE_DIR)
    message(FATAL_ERROR "MediaPipe not found. Please install MediaPipe development package.")
endif()

# Add executable
add_executable(face_tracker
    main.cpp
    camera.cpp
    face_tracker.cpp
)

# Include directories
target_include_directories(face_tracker PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${LIBCAMERA_INCLUDE_DIRS}
    ${MEDIAPIPE_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(face_tracker PRIVATE
    ${OpenCV_LIBS}
    ${LIBCAMERA_LIBRARIES}
    mediapipe
) 