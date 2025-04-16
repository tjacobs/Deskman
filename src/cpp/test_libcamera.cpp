#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Print OpenCV version and build info
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;
    std::cout << "Build info: " << std::endl << cv::getBuildInformation() << std::endl;
    
    // Use GStreamer pipeline with libcamera
    cv::VideoCapture cap;
    std::string pipeline = "libcamerasrc ! video/x-raw,width=1280,height=720,framerate=30/1 ! videoconvert ! appsink";
    bool success = cap.open(pipeline, cv::CAP_GSTREAMER);
    
    if (success) {
        std::cout << "Successfully opened camera" << std::endl;
        
        // Try to capture a frame
        cv::Mat frame;
        if (cap.read(frame)) {
            std::cout << "Successfully captured frame from camera" << std::endl;
            std::cout << "Frame size: " << frame.size().width << "x" << frame.size().height << std::endl;
        } else {
            std::cout << "Failed to capture frame from camera" << std::endl;
        }
    } else {
        std::cout << "Failed to open camera" << std::endl;
    }
    
    cap.release();
    return 0;
} 