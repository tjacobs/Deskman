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
    // Try to open the camera
    std::cout << "Trying to open camera " << cameraIndex << std::endl;
    
    if (isRaspberryPi) {
        // Use GStreamer pipeline on Raspberry Pi
        std::string pipeline = "libcamerasrc ! video/x-raw,width=640,height=480,framerate=30/1 ! videoconvert ! appsink";
        cap.open(pipeline, cv::CAP_GSTREAMER);
    } else {
        // Use default camera on other platforms
        cap.open(cameraIndex);
    }

    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera " << cameraIndex << std::endl;
        return false;
    }

    // Set camera properties
    if (!isRaspberryPi) {
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        cap.set(cv::CAP_PROP_FPS, 30);
    }

    // Try to capture a test frame
    cv::Mat testFrame;
    if (!cap.read(testFrame)) {
        std::cerr << "Error: Could not read frame from camera " << cameraIndex << std::endl;
        return false;
    }

    std::cout << "Successfully opened camera " << cameraIndex << std::endl;
    std::cout << "Successfully captured test frame from camera " << cameraIndex << std::endl;
    return true;
}

bool Camera::captureFrame(cv::Mat& frame) {
    return cap.read(frame);
}

Camera::~Camera() {
    if (cap.isOpened()) {
        cap.release();
    }
} 