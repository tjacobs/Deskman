#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Print OpenCV version and build info
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;
    std::cout << "Build info: " << cv::getBuildInformation() << std::endl;

    // Use a simpler GStreamer pipeline
    std::string pipeline = "libcamerasrc ! video/x-raw,format=BGR ! appsink";
    std::cout << "Opening camera with pipeline: " << pipeline << std::endl;

    cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);
    if (!cap.isOpened()) {
        std::cerr << "Failed to open camera" << std::endl;
        return -1;
    }

    std::cout << "Camera opened successfully" << std::endl;

    // Try to read a frame
    cv::Mat frame;
    for (int i = 0; i < 5; i++) {
        std::cout << "Attempt " << i + 1 << " to read frame..." << std::endl;
        if (cap.read(frame)) {
            std::cout << "Successfully captured frame: " << frame.cols << "x" << frame.rows << std::endl;
            break;
        } else {
            std::cout << "Failed to capture frame" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    if (frame.empty()) {
        std::cerr << "Failed to capture frame after multiple attempts" << std::endl;
        return -1;
    }

    // Save the frame to verify it works
    cv::imwrite("test_frame.jpg", frame);
    std::cout << "Frame saved as test_frame.jpg" << std::endl;

    return 0;
} 