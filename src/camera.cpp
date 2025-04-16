#include "camera.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>

Camera::Camera() {
    // Check if we're running on a Raspberry Pi
    isRaspberryPi = false;
    if (std::filesystem::exists("/proc/device-tree/model")) {
        std::ifstream model("/proc/device-tree/model");
        std::string model_str;
        std::getline(model, model_str);
        isRaspberryPi = model_str.find("Raspberry Pi") != std::string::npos;
    }
}

bool Camera::initialize() {
    if (isRaspberryPi) {
        // Use GStreamer pipeline on Raspberry Pi
        std::string pipeline = "libcamerasrc ! video/x-raw,width=640,height=480,framerate=30/1 ! videoconvert ! appsink";
        std::cout << "Initializing Raspberry Pi camera with pipeline: " << pipeline << std::endl;
        
        if (!cap.open(pipeline, cv::CAP_GSTREAMER)) {
            std::cerr << "Failed to open camera with GStreamer pipeline" << std::endl;
            return false;
        }
    } else {
        // Use default camera on other platforms
        std::cout << "Initializing default camera" << std::endl;
        if (!cap.open(cameraIndex)) {
            std::cerr << "Failed to open camera " << cameraIndex << std::endl;
            return false;
        }
        
        // Set camera properties for non-Pi platforms
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        cap.set(cv::CAP_PROP_FPS, 30);
    }

    // Verify camera is opened
    if (!cap.isOpened()) {
        std::cerr << "Camera is not opened after initialization" << std::endl;
        return false;
    }

    // Try to capture a test frame
    cv::Mat testFrame;
    if (!cap.read(testFrame)) {
        std::cerr << "Failed to capture test frame" << std::endl;
        return false;
    }

    std::cout << "Camera initialized successfully" << std::endl;
    return true;
}

bool Camera::captureFrame(cv::Mat& frame) {
    if (!cap.isOpened()) {
        std::cerr << "Camera is not opened" << std::endl;
        return false;
    }
    
    bool success = cap.read(frame);
    if (!success) {
        std::cerr << "Failed to capture frame" << std::endl;
    }
    return success;
}

Camera::~Camera() {
    if (cap.isOpened()) {
        cap.release();
    }
} 