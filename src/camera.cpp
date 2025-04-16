#include "camera.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

Camera::Camera() {
    // Check if we're running on a Raspberry Pi
    isRaspberryPi = false;
    if (std::filesystem::exists("/proc/device-tree/model")) {
        std::ifstream model("/proc/device-tree/model");
        std::string model_str;
        std::getline(model, model_str);
        isRaspberryPi = model_str.find("Raspberry Pi") != std::string::npos;
        if (isRaspberryPi) {
            std::cout << "Detected Raspberry Pi: " << model_str << std::endl;
        }
    }
}

bool Camera::initialize() {
    if (isRaspberryPi) {
        // Use GStreamer pipeline on Raspberry Pi with lower resolution and framerate
        std::string pipeline = "libcamerasrc ! video/x-raw,width=320,height=240,framerate=10/1,format=BGR ! appsink";
        std::cout << "Initializing Raspberry Pi camera with pipeline: " << pipeline << std::endl;
        
        cap.open(pipeline, cv::CAP_GSTREAMER);
        if (!cap.isOpened()) {
            std::cerr << "Failed to open camera with GStreamer pipeline" << std::endl;
            return false;
        }

        // Test if we can read a frame with multiple retries
        cv::Mat testFrame;
        for (int i = 0; i < 5; i++) {
            std::cout << "Attempt " << i + 1 << " to read test frame..." << std::endl;
            if (cap.read(testFrame)) {
                std::cout << "Successfully captured test frame: " << testFrame.cols << "x" << testFrame.rows << std::endl;
                return true;
            } else {
                std::cout << "Failed to capture test frame, retrying..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        
        std::cerr << "Error: Could not read frames from camera" << std::endl;
        return false;
    } else {
        // Use default camera on other platforms
        std::cout << "Initializing default camera" << std::endl;
        if (!cap.open(cameraIndex)) {
            std::cerr << "Failed to open camera " << cameraIndex << std::endl;
            return false;
        }
        
        // Set camera properties for non-Pi platforms
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 320);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 240);
        cap.set(cv::CAP_PROP_FPS, 30);

        // Try to capture a test frame
        cv::Mat testFrame;
        if (!cap.read(testFrame)) {
            std::cerr << "Failed to capture test frame" << std::endl;
            return false;
        }
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