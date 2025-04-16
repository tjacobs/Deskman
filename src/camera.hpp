#pragma once

#include <opencv2/opencv.hpp>

class Camera {
public:
    Camera();
    ~Camera();
    
    bool initialize();
    bool captureFrame(cv::Mat& frame);

private:
    cv::VideoCapture cap;
    bool isRaspberryPi;
    const int cameraIndex = 0;
}; 
