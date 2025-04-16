#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Print OpenCV version and build info
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;
    std::cout << "Build info: " << std::endl << cv::getBuildInformation() << std::endl;
    
    // Use a simpler GStreamer pipeline
    cv::VideoCapture cap;
    std::string pipeline = "libcamerasrc ! video/x-raw,width=640,height=480,framerate=30/1 ! videoconvert ! appsink";
    
    std::cout << "Opening camera with pipeline: " << pipeline << std::endl;
    bool success = cap.open(pipeline, cv::CAP_GSTREAMER);
    
    if (success) {
        std::cout << "Successfully opened camera" << std::endl;
        
        // Try to capture a frame with timeout
        cv::Mat frame;
        std::cout << "Attempting to read frame..." << std::endl;
        
        // Try multiple times to read a frame
        for (int i = 0; i < 5; i++) {
            if (cap.read(frame)) {
                std::cout << "Successfully captured frame from camera" << std::endl;
                std::cout << "Frame size: " << frame.size().width << "x" << frame.size().height << std::endl;
                std::cout << "Frame type: " << frame.type() << std::endl;
                break;
            } else {
                std::cout << "Failed to capture frame, attempt " << (i + 1) << " of 5" << std::endl;
                cv::waitKey(1000); // Wait 1 second between attempts
            }
        }
    } else {
        std::cout << "Failed to open camera" << std::endl;
    }
    
    cap.release();
    return 0;
} 