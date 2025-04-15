#include "face_tracker.hpp"
#include <iostream>
#include <mediapipe/framework/calculator_graph.h>
#include <mediapipe/framework/formats/image_frame.h>
#include <mediapipe/framework/formats/landmark.pb.h>
#include <mediapipe/framework/port/opencv_core_inc.h>
#include <mediapipe/framework/port/opencv_imgproc_inc.h>
#include <mediapipe/framework/port/status.h>

FaceTracker::FaceTracker(Camera* camera)
    : camera_(camera),
      dead_zone_(0.05f),
      smoothing_factor_(0.1f),
      max_movement_(0.1f),
      last_x_(0.5f),
      last_y_(0.5f) {}

FaceTracker::~FaceTracker() {
    // Cleanup MediaPipe resources
}

bool FaceTracker::initialize() {
    return setupMediaPipeGraph();
}

bool FaceTracker::setupMediaPipeGraph() {
    // Create MediaPipe graph
    mediapipe::CalculatorGraphConfig config;
    config.ParseFromString(R"(
        input_stream: "input_video"
        output_stream: "output_video"
        output_stream: "face_detections"
        node {
            calculator: "FaceDetectionShortRangeCpu"
            input_stream: "IMAGE:input_video"
            output_stream: "DETECTIONS:face_detections"
        }
    )");

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

bool FaceTracker::processFrame(float& face_x, float& face_y) {
    cv::Mat frame;
    if (!camera_->captureFrame(frame)) {
        return false;
    }

    // Convert to RGB for MediaPipe
    cv::Mat rgb_frame;
    cv::cvtColor(frame, rgb_frame, cv::COLOR_BGR2RGB);

    // Create MediaPipe ImageFrame
    auto input_frame = absl::make_unique<mediapipe::ImageFrame>(
        mediapipe::ImageFormat::SRGB, rgb_frame.cols, rgb_frame.rows,
        mediapipe::ImageFrame::kDefaultAlignmentBoundary);
    cv::Mat input_mat = mediapipe::formats::MatView(input_frame.get());
    rgb_frame.copyTo(input_mat);

    // Send frame to MediaPipe
    auto status = graph_.AddPacketToInputStream(
        "input_video",
        mediapipe::Adopt(input_frame.release())
            .At(mediapipe::Timestamp(0)));
    if (!status.ok()) {
        std::cerr << "Failed to send frame to MediaPipe: " << status.message() << std::endl;
        return false;
    }

    // Get face detections
    mediapipe::Packet packet;
    if (!graph_.GetNextPacket(&packet)) {
        return false;
    }

    const auto& detections = packet.Get<std::vector<mediapipe::Detection>>();
    if (detections.empty()) {
        return false;
    }

    // Find largest face
    float max_area = 0;
    const mediapipe::Detection* largest_face = nullptr;
    for (const auto& detection : detections) {
        const auto& bbox = detection.location_data().relative_bounding_box();
        float area = bbox.width() * bbox.height();
        if (area > max_area) {
            max_area = area;
            largest_face = &detection;
        }
    }

    if (!largest_face) {
        return false;
    }

    // Get face center
    const auto& bbox = largest_face->location_data().relative_bounding_box();
    float target_x = bbox.xmin() + bbox.width() / 2;
    float target_y = 1.0f - (bbox.ymin() + bbox.height() / 2);

    // Apply dampening
    dampenMovement(target_x, target_y, face_x, face_y);
    return true;
}

void FaceTracker::dampenMovement(float target_x, float target_y, float& out_x, float& out_y) {
    // Calculate movement
    float dx = (target_x - last_x_) * smoothing_factor_;
    float dy = (target_y - last_y_) * smoothing_factor_;

    // Apply dead zone
    if (std::abs(dx) < dead_zone_) dx = 0;
    if (std::abs(dy) < dead_zone_) dy = 0;

    // Limit maximum movement
    dx = std::max(std::min(dx, max_movement_), -max_movement_);
    dy = std::max(std::min(dy, max_movement_), -max_movement_);

    // Update positions
    out_x = last_x_ + dx;
    out_y = last_y_ + dy;

    // Clamp to [0,1] range
    out_x = std::max(0.0f, std::min(1.0f, out_x));
    out_y = std::max(0.0f, std::min(1.0f, out_y));

    // Update last positions
    last_x_ = out_x;
    last_y_ = out_y;
} 