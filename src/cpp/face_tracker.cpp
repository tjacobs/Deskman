#include "face_tracker.hpp"
#include <iostream>
#include <filesystem>

FaceTracker::FaceTracker() {
    // Try different possible paths for the face cascade classifier
    std::vector<std::string> possible_paths = {
        "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
        "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
        "/opt/homebrew/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
        "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml"
    };

    bool loaded = false;
    for (const auto& path : possible_paths) {
        if (std::filesystem::exists(path)) {
            if (face_cascade.load(path)) {
                loaded = true;
                std::cout << "Successfully loaded face cascade from: " << path << std::endl;
                break;
            }
        }
    }

    if (!loaded) {
        std::cerr << "Error: Could not find or load face cascade classifier in any of the following paths:" << std::endl;
        for (const auto& path : possible_paths) {
            std::cerr << "  " << path << std::endl;
        }
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
    
    // Detect faces with conservative parameters to reduce false positives
    // scaleFactor: 1.2 (less sensitive than 1.1)
    // minNeighbors: 6 (requires more evidence than 4)
    // minSize: 50x50 (focus on larger faces)
    face_cascade.detectMultiScale(frame_gray, faces, 1.2, 6, 0, cv::Size(50, 50));
    
    return faces;
} 