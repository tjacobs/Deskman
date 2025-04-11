#ifndef FACE_H
#define FACE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "vector_renderer.h"

typedef struct {
    // Eye parameters
    int eye_left_x;   // X position of the left eye
    int eye_left_y;   // Y position of the left eye
    int eye_right_x;  // X position of the right eye
    int eye_right_y;  // Y position of the right eye
    int eye_width;    // Width of the eyes
    int eye_height;   // Height of the eyes (changes to simulate squinting)

    // Mouth parameters
    int mouth_x;      // X position of the mouth
    int mouth_y;      // Y position of the mouth
    int mouth_width;  // Width of the mouth
    int mouth_height; // Height of the mouth
    int mouth_smile;  // Curve for the smile (positive for smile, negative for frown)
    char mouth_shape; // Shape of mouth for different phonemes (M=closed, F=slight, T=wide, L=narrow)

    // Font for text rendering
    TTF_Font* font;

    Ellipse* leftEye;
    Ellipse* rightEye;

} Face;

extern Face face;

Face create_face(int center_x, int center_y);
void update_face(Face* face, int eye_squint, int smile_curve);
void cleanup_face(Face* face);

#endif
