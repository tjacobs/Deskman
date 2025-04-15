cmake_minimum_required(VERSION 3.10)
project(face_tracker)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)

# Add executable
add_executable(face_tracker
    main.cpp
    camera.cpp
    face_tracker.cpp
)

# Link libraries
target_link_libraries(face_tracker PRIVATE
    ${OpenCV_LIBS}
)

