# CMake
cmake_minimum_required(VERSION 3.10)
project(face_tracker)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set custom OpenCV path for Pi
#set(OpenCV_DIR "/home/tom/Documents/opencv/build")

# Find required packages
find_package(OpenCV REQUIRED)

# Add executables
add_executable(face_tracker
    main.cpp
    camera.cpp
    face_tracker.cpp
)
add_executable(test_camera
    test_camera.cpp
)

# Link libraries
target_link_libraries(face_tracker PRIVATE
    ${OpenCV_LIBS}
)
target_link_libraries(test_camera PRIVATE
    ${OpenCV_LIBS}
)

