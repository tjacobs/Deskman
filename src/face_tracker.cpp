#include "face_tracker.hpp"
#include <iostream>
#include <filesystem>

FaceTracker::FaceTracker(bool show_window) : showWindow(show_window) {
    std::cout << "Initializing FaceTracker..." << std::endl;
    
    // Try different possible paths for the face cascade classifier
    std::vector<std::string> possible_paths = {
        "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
        "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
        "/opt/homebrew/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
        "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml"
    };

    bool loaded = false;
    for (const auto& path : possible_paths) {
        if (std::filesystem::exists(path)) {
            if (face_cascade.load(path)) {
                loaded = true;
                std::cout << "Successfully loaded face cascade from: " << path << std::endl;
                break;
            }
        }
    }

    if (!loaded) {
        std::cerr << "Error: Could not find or load face cascade classifier in any of the following paths:" << std::endl;
        for (const auto& path : possible_paths) {
            std::cerr << "  " << path << std::endl;
        }
        throw std::runtime_error("Failed to load face cascade classifier");
    }

    // Create window on main thread if needed
    if (showWindow) {
        try {
            std::cout << "Creating OpenCV window..." << std::endl;
            cv::namedWindow("Camera", cv::WINDOW_NORMAL);
            cv::resizeWindow("Camera", 640, 480);
            std::cout << "OpenCV window created successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error creating OpenCV window: " << e.what() << std::endl;
            showWindow = false;  // Disable window if creation fails
        }
    }
}

FaceTracker::~FaceTracker() {
    std::cout << "Cleaning up FaceTracker..." << std::endl;
    stopTracking();
    // Clean up window on main thread
    if (showWindow) {
        try {
            std::cout << "Destroying OpenCV window..." << std::endl;
            cv::destroyWindow("Camera");
            std::cout << "OpenCV window destroyed successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error destroying OpenCV window: " << e.what() << std::endl;
        }
    }
}

void FaceTracker::startTracking() {
    if (isTracking()) {
        return;
    }

    std::cout << "Starting face tracking..." << std::endl;

    // Initialize camera
    if (!camera.initialize()) {
        throw std::runtime_error("Failed to initialize camera");
    }

    shouldQuit = false;
    trackingThread = std::thread(&FaceTracker::trackingThreadFunc, this);
    std::cout << "Face tracking thread started" << std::endl;
}

void FaceTracker::stopTracking() {
    std::cout << "Stopping face tracking..." << std::endl;
    shouldQuit = true;
    if (trackingThread.joinable()) {
        trackingThread.join();
        std::cout << "Face tracking thread stopped" << std::endl;
    }
}

bool FaceTracker::getFacePosition(float& x, float& y) {
    std::lock_guard<std::mutex> lock(faceMutex);
    if (!faceTrackingEnabled) {
        return false;
    }

    // Calculate face position relative to screen center
    x = (currentFace.x + currentFace.width/2) - CAMERA_CENTER_X;
    y = (currentFace.y + currentFace.height/2) - CAMERA_CENTER_Y;
    
    // Normalize to [-1, 1] range
    x /= CAMERA_CENTER_X;
    y /= CAMERA_CENTER_Y;
    
    // Invert coordinates to match screen coordinates
    x = -x;
    y = -y;
    
    return true;
}

void FaceTracker::updateWindow() {
    if (!showWindow) return;

    std::lock_guard<std::mutex> lock(frameMutex);
    if (hasNewFrame) {
        try {
            cv::imshow("Camera", currentFrame);
            cv::waitKey(1);  // Process window events
            hasNewFrame = false;
        } catch (const std::exception& e) {
            std::cerr << "Error updating window: " << e.what() << std::endl;
        }
    }
}

void FaceTracker::trackingThreadFunc() {
    try {
        std::cout << "Face tracking thread started" << std::endl;
        while (!shouldQuit) {
            cv::Mat frame;
            if (!camera.captureFrame(frame)) {
                std::cerr << "Error: Could not read frame from camera" << std::endl;
                break;
            }

            // Detect faces
            auto faces = detectFaces(frame);
            
            // Update current face position
            {
                std::lock_guard<std::mutex> lock(faceMutex);
                if (!faces.empty()) {
                    currentFace = faces[0];  // Track the first face found
                    faceTrackingEnabled = true;
                } else {
                    faceTrackingEnabled = false;
                }
            }

            // Draw faces and update frame buffer if window is enabled
            if (showWindow) {
                // Draw rectangles around faces
                for (const auto& face : faces) {
                    cv::rectangle(frame, face, cv::Scalar(0, 255, 0), 2);
                }
                
                // Update frame buffer
                {
                    std::lock_guard<std::mutex> lock(frameMutex);
                    frame.copyTo(currentFrame);
                    hasNewFrame = true;
                }
            }

            // Add a small delay to reduce CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception& e) {
        std::cerr << "Face tracking error: " << e.what() << std::endl;
    }
}

std::vector<cv::Rect> FaceTracker::detectFaces(const cv::Mat& frame) {
    std::vector<cv::Rect> faces;
    cv::Mat frame_gray;
    
    // Convert to grayscale
    cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    
    // Equalize histogram to improve detection
    cv::equalizeHist(frame_gray, frame_gray);
    
    // Detect faces with conservative parameters to reduce false positives
    // scaleFactor: 1.2 (less sensitive than 1.1)
    // minNeighbors: 6 (requires more evidence than 4)
    // minSize: 50x50 (focus on larger faces)
    face_cascade.detectMultiScale(frame_gray, faces, 1.2, 6, 0, cv::Size(50, 50));
    
    return faces;
} 