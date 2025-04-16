#include "camera.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

Camera::Camera() : use_libcamera(false), libcamera_handle(nullptr) {
    // Check if we're running on a Raspberry Pi
    isRaspberryPi = false;
    if (std::filesystem::exists("/proc/device-tree/model")) {
        std::ifstream model("/proc/device-tree/model");
        std::string model_str;
        std::getline(model, model_str);
        isRaspberryPi = model_str.find("Raspberry Pi") != std::string::npos;
    }
}

Camera::~Camera() {
    if (cap.isOpened()) {
        cap.release();
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