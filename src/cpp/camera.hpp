#pragma once

#include <opencv2/opencv.hpp>
#include <libcamera/libcamera.hpp>

class Camera {
public:
    Camera(int width = 640, int height = 480);
    ~Camera();
    
    bool initialize();
    bool captureFrame(cv::Mat& frame);
    
private:
    int width_;
    int height_;
    libcamera::Camera* camera_;
    libcamera::FrameBufferAllocator* allocator_;
}; 