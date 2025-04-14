#include <iostream>
#include "face_tracker.hpp"
#include "camera.hpp"

int main() {
    Camera* camera = NULL;
    FaceTracker* tracker = NULL;
    
    try {
        // Initialize components
        camera = new Camera(640, 480);
        if (!camera->initialize()) {
            std::cerr << "Failed to initialize camera" << std::endl;
            return 1;
        }
        
        tracker = new FaceTracker(camera);
        if (!tracker->initialize()) {
            std::cerr << "Failed to initialize tracker" << std::endl;
            return 1;
        }
        
        // Main loop
        while (true) {
            float face_x, face_y;
            if (tracker->processFrame(face_x, face_y)) {
                // Move servos using your existing functions
                move_servos(face_x, face_y);
            }
            
            // Check for exit condition
            if (cv::waitKey(1) == 27) break; // ESC key
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        if (tracker) delete tracker;
        if (camera) delete camera;
        return 1;
    }
    
    // Cleanup
    if (tracker) delete tracker;
    if (camera) delete camera;
    return 0;
} 