#include "camera.hpp"
#include <iostream>

Camera::Camera() {}

Camera::~Camera() {
    if (cap.isOpened()) {
        cap.release();
    }
}

bool Camera::initialize() {
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
    std::cerr << "Please check:" << std::endl;
    std::cerr << "1. Camera permissions in System Settings > Privacy & Security > Camera" << std::endl;
    std::cerr << "2. No other application is using the camera" << std::endl;
    return false;
}

bool Camera::captureFrame(cv::Mat& frame) {
    if (!cap.isOpened()) {
        return false;
    }
    
    return cap.read(frame);
} 