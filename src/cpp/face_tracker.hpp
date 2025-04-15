#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

class FaceTracker {
public:
    FaceTracker();
    std::vector<cv::Rect> detectFaces(const cv::Mat& frame);

private:
    cv::CascadeClassifier face_cascade;
};

