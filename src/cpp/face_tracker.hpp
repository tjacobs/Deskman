//#include "mediapipe/framework/calculator_graph.h"

class FaceTracker {
public:
    FaceTracker(Camera* camera);
    ~FaceTracker();
    
    bool initialize();
    bool processFrame(float& face_x, float& face_y);
    
private:
    Camera* camera_;
 //   mediapipe::CalculatorGraph graph_;  // <- This runs your face detection graph

    // Dampener parameters
    float dead_zone_;
    float smoothing_factor_;
    float max_movement_;
    float last_x_;
    float last_y_;
    
    void dampenMovement(float target_x, float target_y, float& out_x, float& out_y);
};

