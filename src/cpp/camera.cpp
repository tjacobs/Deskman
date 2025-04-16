#include "camera.hpp"
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>

Camera::Camera() : use_libcamera(false), libcamera_handle(nullptr) {
    // Check if we're on a Raspberry Pi
    if (std::filesystem::exists("/dev/video0")) {
        camera_device = "/dev/video0";
    } else if (std::filesystem::exists("/dev/video10")) {
        // libcamera might use this device
        camera_device = "/dev/video10";
        use_libcamera = true;
    }
}

Camera::~Camera() {
    if (cap.isOpened()) {
        cap.release();
    }
}

bool Camera::initialize() {
    if (use_libcamera) {
        return initializeLibCamera();
    }
    
    // Try different camera indices
    for (int i = 0; i < 10; i++) {
        std::cout << "Trying to open camera " << i << std::endl;
        cap.open(i);
        
        if (cap.isOpened()) {
            std::cout << "Successfully opened camera " << i << std::endl;
            
            // Set camera properties
            cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
            cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
            
            // Test if we can actually read a frame
            cv::Mat test_frame;
            if (cap.read(test_frame)) {
                std::cout << "Successfully captured test frame from camera " << i << std::endl;
                return true;
            } else {
                std::cerr << "Could not read frame from camera " << i << std::endl;
                cap.release();
            }
        }
    }
    
    std::cerr << "Error: Could not open any camera" << std::endl;
    return false;
}

bool Camera::captureFrame(cv::Mat& frame) {
    if (!cap.isOpened()) {
        return false;
    }
    
    return cap.read(frame);
}

bool Camera::initializeLibCamera() {
    // Use GStreamer pipeline for libcamera
    std::string pipeline = "libcamerasrc ! video/x-raw,format=BGR ! appsink";
    std::cout << "Opening camera with pipeline: " << pipeline << std::endl;
    
    cap.open(pipeline, cv::CAP_GSTREAMER);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open libcamera device" << std::endl;
        return false;
    }

    // Test if we can read a frame
    cv::Mat test_frame;
    for (int i = 0; i < 5; i++) {
        std::cout << "Attempt " << i + 1 << " to read test frame..." << std::endl;
        if (cap.read(test_frame)) {
            std::cout << "Successfully captured test frame: " << test_frame.cols << "x" << test_frame.rows << std::endl;
            return true;
        } else {
            std::cout << "Failed to capture test frame, retrying..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    std::cerr << "Error: Could not read frames from libcamera device" << std::endl;
    return false;
} 