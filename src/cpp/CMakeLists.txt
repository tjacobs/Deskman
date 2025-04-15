cmake_minimum_required(VERSION 3.10)
project(face_tracker)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)
find_package(glog REQUIRED)
find_package(absl REQUIRED)
pkg_check_modules(LIBCAMERA libcamera REQUIRED)

# MediaPipe paths
set(MEDIAPIPE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mediapipe)
set(MEDIAPIPE_INCLUDE_DIR ${MEDIAPIPE_DIR})

# Include directories
include_directories(
    ${MEDIAPIPE_INCLUDE_DIR}
    ${MEDIAPIPE_DIR}/mediapipe
    ${MEDIAPIPE_DIR}/bazel-bin
    ${LIBCAMERA_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
    ${GLOG_INCLUDE_DIRS}
    ${absl_INCLUDE_DIRS}
)

# Add executable
add_executable(face_tracker
    main.cpp
    camera.cpp
    face_tracker.cpp
)

# Link libraries
target_link_libraries(face_tracker PRIVATE
    ${OpenCV_LIBS}
    ${LIBCAMERA_LIBRARIES}
    ${Protobuf_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    glog::glog
    absl::base
    absl::strings
)

