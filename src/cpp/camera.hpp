#pragma once

#include <opencv2/opencv.hpp>
#include <string>

class Camera {
public:
    Camera();
    ~Camera();
    
    bool initialize();
    bool captureFrame(cv::Mat& frame);

private:
    cv::VideoCapture cap;
    std::string camera_device;
    
    // For libcamera
    bool use_libcamera;
    void* libcamera_handle;
    bool initializeLibCamera();
    bool captureFrameLibCamera(cv::Mat& frame);
}; 
