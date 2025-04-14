#include "camera.hpp"
#include <iostream>

Camera::Camera(int width, int height)
    : width_(width), height_(height), camera_(NULL), allocator_(NULL) {}

Camera::~Camera() {
    if (camera_) {
        camera_->stop();
        camera_->release();
        delete camera_;
    }
    if (allocator_) {
        delete allocator_;
    }
}

bool Camera::initialize() {
    try {
        // Initialize libcamera
        libcamera::CameraManager* manager = new libcamera::CameraManager();
        if (manager->start()) {
            std::cerr << "Failed to start camera manager" << std::endl;
            delete manager;
            return false;
        }

        // Get the first available camera
        auto cameras = manager->cameras();
        if (cameras.empty()) {
            std::cerr << "No cameras available" << std::endl;
            delete manager;
            return false;
        }

        camera_ = new libcamera::Camera(cameras[0]);
        if (camera_->acquire()) {
            std::cerr << "Failed to acquire camera" << std::endl;
            delete camera_;
            camera_ = NULL;
            delete manager;
            return false;
        }

        // Configure camera
        auto config = camera_->generateConfiguration({libcamera::StreamRole::Viewfinder});
        if (!config) {
            std::cerr << "Failed to generate camera configuration" << std::endl;
            delete camera_;
            camera_ = NULL;
            delete manager;
            return false;
        }

        auto& streamConfig = config->at(0);
        streamConfig.size.width = width_;
        streamConfig.size.height = height_;
        streamConfig.pixelFormat = libcamera::formats::RGB888;

        if (camera_->configure(config.get())) {
            std::cerr << "Failed to configure camera" << std::endl;
            delete camera_;
            camera_ = NULL;
            delete manager;
            return false;
        }

        // Allocate buffers
        allocator_ = new libcamera::FrameBufferAllocator(camera_);
        for (auto& stream : config->streams()) {
            if (allocator_->allocate(stream.get())) {
                std::cerr << "Failed to allocate buffers" << std::endl;
                delete allocator_;
                allocator_ = NULL;
                delete camera_;
                camera_ = NULL;
                delete manager;
                return false;
            }
        }

        // Start camera
        if (camera_->start()) {
            std::cerr << "Failed to start camera" << std::endl;
            delete allocator_;
            allocator_ = NULL;
            delete camera_;
            camera_ = NULL;
            delete manager;
            return false;
        }

        delete manager;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Camera initialization error: " << e.what() << std::endl;
        return false;
    }
}

bool Camera::captureFrame(cv::Mat& frame) {
    try {
        auto request = camera_->createRequest();
        if (!request) {
            std::cerr << "Failed to create request" << std::endl;
            return false;
        }

        // Queue request and wait for frame
        if (camera_->queueRequest(request.get())) {
            std::cerr << "Failed to queue request" << std::endl;
            return false;
        }

        auto completed = request->wait();
        if (completed->status() != libcamera::Request::Status::Completed) {
            std::cerr << "Request failed" << std::endl;
            return false;
        }

        // Get frame data
        auto buffer = completed->buffers().begin()->second;
        const auto& planes = buffer->planes();
        const auto& plane = planes[0];

        // Create OpenCV Mat from frame data
        frame = cv::Mat(height_, width_, CV_8UC3, plane->data(), plane->stride());
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Frame capture error: " << e.what() << std::endl;
        return false;
    }
} 