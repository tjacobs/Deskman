#pragma once

#include "camera.hpp"
#include <mediapipe/framework/calculator_graph.h>
#include <mediapipe/framework/formats/image_frame.h>
#include <mediapipe/framework/formats/landmark.pb.h>
#include <mediapipe/framework/port/opencv_core_inc.h>
#include <mediapipe/framework/port/opencv_imgproc_inc.h>
#include <mediapipe/framework/port/status.h>

class FaceTracker {
public:
    FaceTracker(Camera* camera);
    ~FaceTracker();
    
    bool initialize();
    bool processFrame(float& face_x, float& face_y);
    
private:
    Camera* camera_;
    mediapipe::CalculatorGraph graph_;
    
    // Dampener parameters
    float dead_zone_;
    float smoothing_factor_;
    float max_movement_;
    float last_x_;
    float last_y_;
    
    void dampenMovement(float target_x, float target_y, float& out_x, float& out_y);
    bool setupMediaPipeGraph();
};

