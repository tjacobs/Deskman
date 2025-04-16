cmake_minimum_required(VERSION 3.10)
project(face_tracker)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)

# Add executables
add_executable(face_tracker
    main.cpp
    camera.cpp
    face_tracker.cpp
)

add_executable(test_libcamera
    test_libcamera.cpp
)

# Link libraries
target_link_libraries(face_tracker PRIVATE
    ${OpenCV_LIBS}
)

target_link_libraries(test_libcamera PRIVATE
    ${OpenCV_LIBS}
)

