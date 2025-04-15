#pragma once

#include <opencv2/opencv.hpp>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/stream.h>
#include <libcamera/framebuffer.h>


class Camera {
public:
    Camera(int width = 640, int height = 480);
    ~Camera();
    
    bool initialize();
    bool captureFrame(cv::Mat& frame);
    
    // New methods for direct buffer access
    bool startStream();
    bool getNextFrame(uint8_t*& data, size_t& size);
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
private:
    int width_;
    int height_;
    libcamera::Camera* camera_;
    libcamera::FrameBufferAllocator* allocator_;
    libcamera::Request* current_request_;
    
    bool setupStream();
    void cleanup();
}; 
