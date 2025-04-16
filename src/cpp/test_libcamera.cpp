#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Print OpenCV version and build info
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;
    std::cout << "Build info: " << cv::getBuildInformation() << std::endl;
    
    // Try to open libcamera device
    cv::VideoCapture cap;
    bool success = cap.open("/dev/video10", cv::CAP_LIBCAMERA);
    
    if (success) {
        std::cout << "Successfully opened libcamera device" << std::endl;
        
        // Try to read a frame
        cv::Mat frame;
        if (cap.read(frame)) {
            std::cout << "Successfully captured frame from libcamera" << std::endl;
            std::cout << "Frame size: " << frame.cols << "x" << frame.rows << std::endl;
        } else {
            std::cout << "Failed to capture frame from libcamera" << std::endl;
        }
    } else {
        std::cout << "Failed to open libcamera device" << std::endl;
        std::cout << "Trying V4L2 device..." << std::endl;
        
        // Try V4L2 as fallback
        success = cap.open(0);
        if (success) {
            std::cout << "Successfully opened V4L2 device" << std::endl;
        } else {
            std::cout << "Failed to open V4L2 device" << std::endl;
        }
    }
    
    return 0;
} 