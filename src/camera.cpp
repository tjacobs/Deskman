#include "camera.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace std;

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
        // Use GStreamer pipeline on Raspberry Pi with resolution and framerate from class
        string pipeline = "libcamerasrc ! video/x-raw,width=" + to_string(width) + 
                              ",height=" + to_string(height) + 
                              ",framerate=" + to_string(framerate) + "/1,format=BGR ! appsink";
        cout << "Initializing Raspberry Pi camera with pipeline: " << pipeline << endl;
        
        cap.open(pipeline, cv::CAP_GSTREAMER);
        if (!cap.isOpened()) {
            cerr << "Failed to open camera with GStreamer pipeline" << endl;
            return false;
        }

        // Test if we can read a frame with multiple retries
        cv::Mat testFrame;
        for (int i = 0; i < 5; i++) {
            cout << "Attempt " << i + 1 << " to read test frame..." << endl;
            if (cap.read(testFrame)) {
                cout << "Successfully captured test frame: " << testFrame.cols << "x" << testFrame.rows << endl;
                return true;
            } else {
                cout << "Failed to capture test frame, retrying..." << endl;
                this_thread::sleep_for(chrono::seconds(1));
            }
        }
        
        cerr << "Error: Could not read frames from camera" << endl;
        return false;
    } else {
        // Use default camera on other platforms
        cout << "Initializing default camera" << endl;
        if (!cap.open(cameraIndex)) {
            cerr << "Failed to open camera " << cameraIndex << endl;
            return false;
        }
        
        // Set camera properties for non-Pi platforms
        cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
        cap.set(cv::CAP_PROP_FPS, framerate);

        // Try to capture a test frame
        cv::Mat testFrame;
        if (!cap.read(testFrame)) {
            cerr << "Failed to capture test frame" << endl;
            return false;
        }
    }

    cout << "Camera initialized successfully" << endl;
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