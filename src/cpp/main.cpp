#include "camera.hpp"
#include "face_tracker.hpp"
#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    try {
        Camera camera;
        FaceTracker tracker;
        
        if (!camera.initialize()) {
            std::cerr << "Failed to initialize camera" << std::endl;
            return 1;
        }
        
        cv::namedWindow("Face Detection", cv::WINDOW_AUTOSIZE);
        
        while (true) {
            cv::Mat frame;
            if (!camera.captureFrame(frame)) {
                std::cerr << "Failed to capture frame" << std::endl;
                break;
            }
            
            // Detect faces
            auto faces = tracker.detectFaces(frame);
            
            // Draw rectangles around detected faces
            for (const auto& face : faces) {
                cv::rectangle(frame, face, cv::Scalar(0, 255, 0), 2);
            }
            
            // Display the frame
            cv::imshow("Face Detection", frame);
            
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
