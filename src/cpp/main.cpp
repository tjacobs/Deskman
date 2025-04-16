#include "camera.hpp"
#include "face_tracker.hpp"
#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    try {
        std::cout << "Starting face tracker..." << std::endl;
        
        Camera camera;
        std::cout << "Camera object created" << std::endl;
        
        FaceTracker tracker;
        std::cout << "FaceTracker object created" << std::endl;
        
        if (!camera.initialize()) {
            std::cerr << "Failed to initialize camera" << std::endl;
            return 1;
        }
        std::cout << "Camera initialized successfully" << std::endl;
        
        cv::namedWindow("Face Tracker", cv::WINDOW_NORMAL);
        cv::resizeWindow("Face Tracker", 640, 480);
        std::cout << "Window created" << std::endl;
        
        while (true) {
            cv::Mat frame;
            if (!camera.captureFrame(frame)) {
                std::cerr << "Failed to capture frame" << std::endl;
                break;
            }
            
            // Detect faces
            auto faces = tracker.detectFaces(frame);
            std::cout << "Detected " << faces.size() << " faces" << std::endl;
            
            // Draw rectangles around detected faces
            for (const auto& face : faces) {
                cv::rectangle(frame, face, cv::Scalar(0, 255, 0), 2);
            }
            
            // Display the frame
            cv::imshow("Face Tracker", frame);
            
            // Break loop if 'q' is pressed
            if (cv::waitKey(1) == 'q') {
                break;
            }
        }
        
        cv::destroyAllWindows();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
} 
