#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include "camera.hpp"

using namespace std;

class FaceTracker {
public:
    // Camera resolution constants
    static constexpr int CAMERA_WIDTH = 320;
    static constexpr int CAMERA_HEIGHT = 240;
    static constexpr float CAMERA_CENTER_X = CAMERA_WIDTH / 2.0f;
    static constexpr float CAMERA_CENTER_Y = CAMERA_HEIGHT / 2.0f;

    FaceTracker(bool show_window = false);
    ~FaceTracker();
    
    void startTracking();
    void stopTracking();
    bool isTracking() const { return trackingThread.joinable(); }
    
    // Get the current face position in normalized coordinates (-1 to 1)
    bool getFacePosition(float& x, float& y);
    
    // New method to update window from main thread
    void updateWindow();

private:
    void trackingThreadFunc();
    std::vector<cv::Rect> detectFaces(const cv::Mat& frame);

    cv::CascadeClassifier face_cascade;
    Camera camera;
    cv::Rect currentFace;
    std::mutex faceMutex;
    std::thread trackingThread;
    std::atomic<bool> shouldQuit{false};
    bool faceTrackingEnabled{false};
    bool showWindow;

    // Frame buffer for window updates
    std::mutex frameMutex;
    cv::Mat currentFrame;
    bool hasNewFrame = false;
};

