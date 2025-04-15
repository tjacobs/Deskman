cmake_minimum_required(VERSION 3.10)
project(face_tracker)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MediaPipe paths
set(MEDIAPIPE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mediapipe)
set(MEDIAPIPE_INCLUDE_DIR ${MEDIAPIPE_DIR})

# Find required packages
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)
pkg_check_modules(LIBCAMERA libcamera REQUIRED)

# Include directories
include_directories(
    ${MEDIAPIPE_INCLUDE_DIR}
    ${MEDIAPIPE_DIR}/mediapipe
    ${MEDIAPIPE_DIR}/bazel-bin
    ${MEDIAPIPE_DIR}/bazel-mediapipe
    ${MEDIAPIPE_DIR}/bazel-mediapipe/external/com_google_absl
    ${MEDIAPIPE_DIR}/bazel-mediapipe/external/com_google_protobuf/src
    ${MEDIAPIPE_DIR}/third_party
    ${LIBCAMERA_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
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
    ${MEDIAPIPE_DIR}/bazel-bin/mediapipe/framework/libcalculator_graph.a
    ${MEDIAPIPE_DIR}/bazel-bin/mediapipe/framework/port/libstatus.a
    ${MEDIAPIPE_DIR}/bazel-bin/mediapipe/modules/face_detection/libface_detection_short_range_cpu.a
    ${MEDIAPIPE_DIR}/bazel-bin/external/com_google_absl/absl/base/libbase.a
    ${MEDIAPIPE_DIR}/bazel-bin/external/com_google_absl/absl/strings/libstrings.a
)

