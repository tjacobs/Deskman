#include "face_tracker.hpp"
#include <iostream>
#include <filesystem>

FaceTracker::FaceTracker(bool show_window) : showWindow(show_window) {
    // Try different possible paths for the face cascade classifier
    vector<string> possible_paths = {
        "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
        "/opt/homebrew/share/opencv4/haarcascades/haarcascade_frontalface_default.xml"
    };
    bool loaded = false;
    for (const auto& path : possible_paths) {
        if (filesystem::exists(path)) {
            if (face_cascade.load(path)) {
                loaded = true;
                cout << "Loaded face cascade from: " << path << endl;
                break;
            }
        }
    }
    if (!loaded) {
        cerr << "Error: Could not find or load face cascade classifier in any of the following paths:" << endl;
        for (const auto& path : possible_paths) {
            cerr << "  " << path << endl;
        }
        throw runtime_error("Failed to load face cascade classifier");
    }

    // Try to initialize camera
    cameraAvailable = camera.initialize();
    if (!cameraAvailable) {
        cerr << "Warning: Could not initialize camera. Face tracking will be disabled." << endl;
        showWindow = false;  // Disable window if camera is not available
    } else if (showWindow) {
        try {
            cv::namedWindow("Camera", cv::WINDOW_NORMAL);
            cv::resizeWindow("Camera", camera.width, camera.height);
        } catch (const exception& e) {
            cerr << "Error creating OpenCV window: " << e.what() << endl;
            showWindow = false;
        }
    }
}

FaceTracker::~FaceTracker() {
    stopTracking();

    // Clean up window on main thread
    if (showWindow) {
        try {
            cout << "Destroying OpenCV window..." << endl;
            cv::destroyWindow("Camera");
            cout << "OpenCV window destroyed successfully" << endl;
        } catch (const exception& e) {
            cerr << "Error destroying OpenCV window: " << e.what() << endl;
        }
    }
}

void FaceTracker::startTracking() {
    if (isTracking()) {
        return;
    }
    cout << "Starting face tracking..." << endl;

    // Only start tracking if camera is available
    if (!cameraAvailable) {
        cout << "Face tracking disabled - camera not available" << endl;
        return;
    }

    // Start tracking thread
    shouldQuit = false;
    trackingThread = thread(&FaceTracker::trackingThreadFunc, this);
}

void FaceTracker::stopTracking() {
    shouldQuit = true;
    if (trackingThread.joinable()) {
        trackingThread.join();
        cout << "Face tracking thread stopped" << endl;
    }
}

bool FaceTracker::getFacePosition(float& x, float& y) {
    if (!cameraAvailable) {
        return false;
    }

    lock_guard<mutex> lock(faceMutex);
    if (!faceTrackingEnabled) {
        return false;
    }

    // Calculate face position relative to screen center
    float centerX = camera.width / 2.0f;
    float centerY = camera.height / 2.0f;
    x = (currentFace.x + currentFace.width/2) - centerX;
    y = (currentFace.y + currentFace.height/2) - centerY;
    
    // Normalize to [-1, 1] range
    x /= centerX;
    y /= centerY;
    
    // Invert coordinates to match screen coordinates
    x = -x;
    y = -y;
    
    return true;
}

void FaceTracker::updateWindow() {
    if (!showWindow || !cameraAvailable) return;

    lock_guard<mutex> lock(frameMutex);
    if (hasNewFrame) {
        try {
            cv::imshow("Camera", currentFrame);
            cv::waitKey(1);  // Process window events
            hasNewFrame = false;
        } catch (const exception& e) {
            cerr << "Error updating window: " << e.what() << endl;
        }
    }
}

void FaceTracker::trackingThreadFunc() {
    try {
        while (!shouldQuit) {
            cv::Mat frame;
            if (!camera.captureFrame(frame)) {
                cerr << "Error: Could not read frame from camera" << endl;
                break;
            }

            // Detect faces
            auto faces = detectFaces(frame);
            
            // Update current face position
            {
                lock_guard<mutex> lock(faceMutex);
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
                    lock_guard<mutex> lock(frameMutex);
                    frame.copyTo(currentFrame);
                    hasNewFrame = true;
                }
            }

            // Add a small delay to reduce CPU usage
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    } catch (const exception& e) {
        cerr << "Face tracking error: " << e.what() << endl;
    }
}

vector<cv::Rect> FaceTracker::detectFaces(const cv::Mat& frame) {
    vector<cv::Rect> faces;
    cv::Mat frame_gray;
    
    // Convert to grayscale
    cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    
    // Equalize histogram to improve detection
    cv::equalizeHist(frame_gray, frame_gray);
    
    // Detect faces with conservative parameters to reduce false positives
    // scaleFactor: 1.2 (less sensitive than 1.1)
    // minNeighbors: 4
    // minSize: 50x50 (focus on larger faces)
    face_cascade.detectMultiScale(frame_gray, faces, 1.2, 4, 0, cv::Size(30, 30));
    
    return faces;
} 