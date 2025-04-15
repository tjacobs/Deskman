cmake_minimum_required(VERSION 3.10)
project(face_tracker)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Manually set MediaPipe paths
set(MEDIAPIPE_DIR ../mediapipe)
set(MEDIAPIPE_INCLUDE_DIR ${MEDIAPIPE_DIR})

include_directories(
    ${MEDIAPIPE_INCLUDE_DIR}
    ${MEDIAPIPE_DIR}/third_party
    ${MEDIAPIPE_DIR}/bazel-bin
)

# Add OpenCV + libcamera
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCAMERA libcamera REQUIRED)
include_directories(${LIBCAMERA_INCLUDE_DIRS})

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
    # These are example static libs from MediaPipe; adjust as needed:
    ${CMAKE_SOURCE_DIR}/../mediapipe/bazel-bin/mediapipe/framework/libcalculator_graph.a
    ${CMAKE_SOURCE_DIR}/../mediapipe/bazel-bin/mediapipe/framework/port/libstatus.a
    # You may need more depending on what you use
)

