#include "face_tracker.hpp"
#include <iostream>

FaceTracker::FaceTracker() {
    // Load the pre-trained face detection classifier
    if (!face_cascade.load("/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml")) {
        std::cerr << "Error loading face cascade classifier" << std::endl;
        throw std::runtime_error("Failed to load face cascade classifier");
    }
}

std::vector<cv::Rect> FaceTracker::detectFaces(const cv::Mat& frame) {
    std::vector<cv::Rect> faces;
    cv::Mat frame_gray;
    
    // Convert to grayscale
    cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    
    // Equalize histogram to improve detection
    cv::equalizeHist(frame_gray, frame_gray);
    
    // Detect faces
    face_cascade.detectMultiScale(frame_gray, faces, 1.1, 3, 0, cv::Size(30, 30));
    
    return faces;
} 