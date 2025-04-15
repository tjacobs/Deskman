#include "face_tracker.hpp"
#include <iostream>

FaceTracker::FaceTracker(Camera* camera)
    : camera_(camera),
      dead_zone_(0.01f), smoothing_factor_(0.1f),
      max_movement_(0.5f), last_x_(0.5f), last_y_(0.5f) {}

FaceTracker::~FaceTracker() {
    // MediaPipe graph will be automatically cleaned up
}

bool FaceTracker::setupMediaPipeGraph() {
    // Configure MediaPipe graph
    mediapipe::CalculatorGraphConfig config;
    config.ParseFromString(R"(
        input_stream: "input_video"
        output_stream: "face_detections"
        node {
            calculator: "FaceDetectionShortRangeCpu"
            input_stream: "IMAGE:input_video"
            output_stream: "DETECTIONS:face_detections"
        }
    )");

    // Initialize the graph
    auto status = graph_.Initialize(config);
    if (!status.ok()) {
        std::cerr << "Failed to initialize MediaPipe graph: " << status.message() << std::endl;
        return false;
    }

    // Start the graph
    status = graph_.StartRun({});
    if (!status.ok()) {
        std::cerr << "Failed to start MediaPipe graph: " << status.message() << std::endl;
        return false;
    }

    return true;
}

bool FaceTracker::initialize() {
    if (!camera_->initialize()) {
        std::cerr << "Failed to initialize camera" << std::endl;
        return false;
    }

    if (!camera_->startStream()) {
        std::cerr << "Failed to start camera stream" << std::endl;
        return false;
    }

    return setupMediaPipeGraph();
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
    // Get frame data directly from camera
    uint8_t* frame_data;
    size_t frame_size;
    if (!camera_->getNextFrame(frame_data, frame_size)) {
        return false;
    }

    // Create MediaPipe ImageFrame
    auto input_frame = absl::make_unique<mediapipe::ImageFrame>(
        mediapipe::ImageFormat::SRGB,
        camera_->getWidth(),
        camera_->getHeight(),
        mediapipe::ImageFrame::kDefaultAlignmentBoundary
    );

    // Copy frame data
    std::memcpy(input_frame->MutablePixelData(), frame_data, frame_size);

    // Send frame to MediaPipe
    auto status = graph_.AddPacketToInputStream(
        "input_video",
        mediapipe::Adopt(input_frame.release()).At(mediapipe::Timestamp(0))
    );

    if (!status.ok()) {
        std::cerr << "Failed to send frame to MediaPipe: " << status.message() << std::endl;
        return false;
    }

    // Get face detections
    mediapipe::Packet packet;
    if (!graph_.GetNextPacket("face_detections", &packet)) {
        return false;
    }

    const auto& detections = packet.Get<std::vector<mediapipe::Detection>>();
    if (detections.empty()) {
        return false;
    }

    // Find largest face
    const auto& detection = *std::max_element(
        detections.begin(),
        detections.end(),
        [](const auto& a, const auto& b) {
            return a.location_data().relative_bounding_box().width() * 
                   a.location_data().relative_bounding_box().height() <
                   b.location_data().relative_bounding_box().width() * 
                   b.location_data().relative_bounding_box().height();
        }
    );

    // Get face center
    const auto& bbox = detection.location_data().relative_bounding_box();
    float x_center = bbox.xmin() + bbox.width()/2;
    float y_center = bbox.ymin() + bbox.height()/2;

    // Apply damping
    dampenMovement(x_center, 1.0f - y_center, face_x, face_y);

    return true;
} 