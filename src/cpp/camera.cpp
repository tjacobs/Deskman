#include "camera.hpp"
#include <iostream>

Camera::Camera() {}

Camera::~Camera() {
    if (cap.isOpened()) {
        cap.release();
    }
}

bool Camera::initialize() {
    // Open the default camera (usually /dev/video0)
    cap.open(0);
    
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera" << std::endl;
        return false;
    }
    
    // Set camera properties
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    
    return true;
}

bool Camera::captureFrame(cv::Mat& frame) {
    if (!cap.isOpened()) {
        return false;
    }
    
    return cap.read(frame);
} 