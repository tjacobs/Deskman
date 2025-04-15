#include "camera.hpp"
#include <iostream>

Camera::Camera(int width, int height)
    : width_(width), height_(height) {}

Camera::~Camera() {
    cleanup();
}

void Camera::cleanup() {
    if (camera_) {
        camera_->stop();
        camera_->release();
    }
    camera_.reset();
    manager_.reset();
}

bool Camera::setupStream() {
    try {
        // Initialize libcamera
        manager_ = std::make_shared<libcamera::CameraManager>();
        if (manager_->start()) {
            std::cerr << "Failed to start camera manager" << std::endl;
            return false;
        }

        // Get the first available camera
        auto cameras = manager_->cameras();
        if (cameras.empty()) {
            std::cerr << "No cameras available" << std::endl;
            return false;
        }

        camera_ = cameras[0];
        if (camera_->acquire()) {
            std::cerr << "Failed to acquire camera" << std::endl;
            return false;
        }

        // Configure camera
        config_ = camera_->generateConfiguration({libcamera::StreamRole::Viewfinder});
        if (!config_) {
            std::cerr << "Failed to generate camera configuration" << std::endl;
            return false;
        }

        auto& streamConfig = config_->at(0);
        streamConfig.size.width = width_;
        streamConfig.size.height = height_;
        streamConfig.pixelFormat = libcamera::formats::RGB888;

        if (camera_->configure(config_.get())) {
            std::cerr << "Failed to configure camera" << std::endl;
            return false;
        }

        // Start camera
        if (camera_->start()) {
            std::cerr << "Failed to start camera" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Camera setup error: " << e.what() << std::endl;
        return false;
    }
}

bool Camera::initialize() {
    return setupStream();
}

bool Camera::captureFrame(cv::Mat& frame) {
    // Create request
    current_request_ = camera_->createRequest();
    if (!current_request_) {
        std::cerr << "Failed to create request" << std::endl;
        return false;
    }

    // Queue request
    if (camera_->queueRequest(current_request_.get())) {
        std::cerr << "Failed to queue request" << std::endl;
        return false;
    }

    // Wait for completion
    libcamera::Request* completed = nullptr;
    camera_->requestCompleted.connect([&](libcamera::Request* request) {
        completed = request;
    });

    while (!completed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (completed->status() != libcamera::Request::Status::Complete) {
        std::cerr << "Request failed" << std::endl;
        return false;
    }

    // Get frame data
    const auto& buffers = completed->buffers();
    if (buffers.empty()) {
        std::cerr << "No buffers in completed request" << std::endl;
        return false;
    }

    auto buffer = buffers.begin()->second;
    const auto& planes = buffer->planes();
    if (planes.empty()) {
        std::cerr << "No planes in buffer" << std::endl;
        return false;
    }

    const auto& plane = planes[0];
    frame = cv::Mat(height_, width_, CV_8UC3, const_cast<uint8_t*>(plane.data().data()));
    return true;
} 