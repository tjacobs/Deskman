#include "face_tracker.hpp"
#include <iostream>

FaceTracker::FaceTracker(Camera* camera)
    : camera_(camera), face_detector_(NULL),
      dead_zone_(0.01f), smoothing_factor_(0.1f),
      max_movement_(0.5f), last_x_(0.5f), last_y_(0.5f) {}

FaceTracker::~FaceTracker() {
    if (face_detector_) {
        delete face_detector_;
    }
}

bool FaceTracker::initialize() {
    try {
        face_detector_ = new mediapipe::FaceDetection();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Face detector initialization error: " << e.what() << std::endl;
        return false;
    }
}

void FaceTracker::dampenMovement(float target_x, float target_y, float& out_x, float& out_y) {
    // Calculate distance
    float dx = target_x - last_x_;
    float dy = target_y - last_y_;
    
    // Apply dead zone
    if (std::abs(dx) < dead_zone_) dx = 0;
    if (std::abs(dy) < dead_zone_) dy = 0;
    
    // Limit maximum movement
    dx = std::max(std::min(dx, max_movement_), -max_movement_);
    dy = std::max(std::min(dy, max_movement_), -max_movement_);
    
    // Apply smoothing
    out_x = last_x_ + dx * smoothing_factor_;
    out_y = last_y_ + dy * smoothing_factor_;
    
    // Update last positions
    last_x_ = out_x;
    last_y_ = out_y;
}

bool FaceTracker::processFrame(float& face_x, float& face_y) {
    cv::Mat frame;
    if (!camera_->captureFrame(frame)) {
        return false;
    }
    
    // Convert to RGB for MediaPipe
    cv::Mat rgb_frame;
    cv::cvtColor(frame, rgb_frame, cv::COLOR_BGR2RGB);
    
    // Process frame with MediaPipe
    auto results = face_detector_->process(rgb_frame);
    
    if (results.detections.empty()) {
        return false;
    }
    
    // Find largest face
    const auto& detection = *std::max_element(
        results.detections.begin(),
        results.detections.end(),
        [](const auto& a, const auto& b) {
            return a.location_data.relative_bounding_box.width * 
                   a.location_data.relative_bounding_box.height <
                   b.location_data.relative_bounding_box.width * 
                   b.location_data.relative_bounding_box.height;
        }
    );
    
    // Get face center
    const auto& bbox = detection.location_data.relative_bounding_box;
    float x_center = bbox.xmin + bbox.width/2;
    float y_center = bbox.ymin + bbox.height/2;
    
    // Apply damping
    dampenMovement(x_center + 0.1f, 1.0f - y_center, face_x, face_y);
    
    return true;
} 