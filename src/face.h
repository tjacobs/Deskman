#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef struct {
    // Eye parameters
    int eye_left_x;  // X position of the left eye
    int eye_left_y;  // Y position of the left eye
    int eye_right_x; // X position of the right eye
    int eye_right_y; // Y position of the right eye
    int eye_width;   // Width of the eyes
    int eye_height;  // Height of the eyes (changes to simulate squinting)

    // Mouth parameters
    int mouth_x;     // X position of the mouth
    int mouth_y;     // Y position of the mouth
    int mouth_width; // Width of the mouth
    int mouth_height; // Height of the mouth
    int mouth_smile;  // Curve for the smile (positive for smile, negative for frown)

} Face;

Face create_face(int center_x, int center_y);
void render_face(SDL_Renderer* renderer, Face* face);
void update_face(Face* face, int eye_squint, int smile_curve);