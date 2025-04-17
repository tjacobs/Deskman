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
    }
}

bool Camera::initialize() {
    if (isRaspberryPi) {
        // Use GStreamer pipeline on Raspberry Pi because only libcamera works, no v4l2
        string pipeline = "libcamerasrc ! video/x-raw,width=" + to_string(width) + 
                              ",height=" + to_string(height) + 
                              ",framerate=" + to_string(framerate) + "/1,format=BGR ! appsink";
        cout << "Initializing Raspberry Pi camera with pipeline: " << pipeline << endl;        
        cap.open(pipeline, cv::CAP_GSTREAMER);
        if (!cap.isOpened()) {
            cerr << "Failed to open camera with GStreamer pipeline" << endl;
            return false;
        }
        return true;
    } else {
        // Use default camera on other platforms
        cout << "Initializing camera" << endl;
        if (!cap.open(cameraIndex)) {
            cerr << "Failed to open camera " << cameraIndex << endl;
            return false;
        }
        
        // Set camera properties for other platforms
        cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
        cap.set(cv::CAP_PROP_FPS, framerate);
    }
    return true;
}

bool Camera::captureFrame(cv::Mat& frame) {
    if (!cap.isOpened()) {
        cerr << "Camera is not opened" << endl;
        return false;
    }
    bool success = cap.read(frame);
    if (!success) {
        cerr << "Failed to capture frame" << endl;
    }
    return success;
}

Camera::~Camera() {
    if (cap.isOpened()) {
        cap.release();
    }
}
