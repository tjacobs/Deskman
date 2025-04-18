#include "camera.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

Camera::Camera() : use_libcamera(false), libcamera_handle(nullptr) {
    // Check if we're on a Raspberry Pi by looking for Pi-specific files
    if (std::filesystem::exists("/proc/device-tree/model")) {
        std::ifstream model("/proc/device-tree/model");
        std::string model_str;
        std::getline(model, model_str);
        if (model_str.find("Raspberry Pi") != std::string::npos) {
            use_libcamera = true;
            std::cout << "Detected Raspberry Pi: " << model_str << std::endl;
        }
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
            cap.set(cv::CAP_PROP_FRAME_WIDTH, 320);
            cap.set(cv::CAP_PROP_FRAME_HEIGHT, 240);
            
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
    // Use GStreamer pipeline for libcamera with lower framerate
    std::string pipeline = "libcamerasrc ! video/x-raw,width=320,height=240,framerate=10/1,format=BGR ! appsink";
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