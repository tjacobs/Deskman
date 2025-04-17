#pragma once

#include <opencv2/opencv.hpp>

using namespace std;

class Camera {
public:
    Camera();
    ~Camera();
    
    bool initialize();
    bool captureFrame(cv::Mat& frame);

    // Camera parameters
    const int width = 640;
    const int height = 480;
    const int framerate = 30;

private:
    cv::VideoCapture cap;
    bool isRaspberryPi;
    const int cameraIndex = 0;
}; 
