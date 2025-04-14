#pragma once

#include <opencv2/opencv.hpp>
#include "camera.hpp"

class FaceTracker {
public:
    FaceTracker(Camera* camera);
    ~FaceTracker();
    
    bool initialize();
    bool processFrame(float& face_x, float& face_y);
    
private:
    Camera* camera_;
    mediapipe::FaceDetection* face_detector_;
    
    // Dampener parameters
    float dead_zone_;
    float smoothing_factor_;
    float max_movement_;
    float last_x_;
    float last_y_;
    
    void dampenMovement(float target_x, float target_y, float& out_x, float& out_y);
}; 