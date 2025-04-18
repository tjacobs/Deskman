#include "face.h"
#include <math.h>
#include <stdio.h>
#include "servos.h"
#include <iostream>
#include "vector_renderer.h"
using namespace std;

// Face
Face face;
extern VectorRenderer vectorRenderer;

Face create_face(int center_x, int center_y) {
    Face face;

    // Create vector shapes for the face
    face.leftEye = new Ellipse(45, 120, {25, 25, 25, 255}, {255, 255, 255, 255}, 0.0f);
    face.leftEye->localPosition = Vec3(-120, -100, 0);
    
    face.rightEye = new Ellipse(45, 120, {25, 25, 25, 255}, {255, 255, 255, 255}, 0.0f);
    face.rightEye->localPosition = Vec3(120, -100, 0);
    
    // Create mouth with cutout
    Ellipse* mouth = new Ellipse(240, 80, {255, 255, 255, 255}, {255, 255, 255, 255}, 0.0f, -40, 180);
    mouth->localPosition = Vec3(0, 200, 0);

    // Add shapes to renderer
    vectorRenderer.addShape(face.leftEye);
    vectorRenderer.addShape(face.rightEye);
    //vectorRenderer.addShape(mouth);

    // Load font
    face.font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 24);
    if (!face.font) {
        face.font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
        if (!face.font) {
            fprintf(stderr, "Failed to load fallback font: %s\n", TTF_GetError());
        }
    }

    return face;
}

void render_face(SDL_Renderer* renderer, Face* face) {
    // Render eyes
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black for eyes
    SDL_Rect left_eye = {face->eye_left_x - face->eye_width / 2, face->eye_left_y - face->eye_height / 2, face->eye_width, face->eye_height};
    SDL_Rect right_eye = {face->eye_right_x - face->eye_width / 2, face->eye_right_y - face->eye_height / 2, face->eye_width, face->eye_height};
    SDL_RenderFillRect(renderer, &left_eye);
    SDL_RenderFillRect(renderer, &right_eye);

    // Render mouth
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black for mouth line
    int thickness = 5; // Thickness of the mouth line
    
    // Different mouth shapes based on phonemes
    switch (face->mouth_shape) {
        case 'M': // Closed mouth
            for (int t = 0; t < thickness; t++) {
                for (int i = 0; i < face->mouth_width; i++) {
                    int y_offset = (int)(face->mouth_smile * sin(M_PI * i / face->mouth_width));
                    SDL_RenderDrawPoint(renderer, face->mouth_x + i, face->mouth_y + y_offset + t);
                }
            }
            break;
            
        case 'F': // Slight opening
            for (int t = 0; t < thickness; t++) {
                // Top lip
                for (int i = 0; i < face->mouth_width; i++) {
                    SDL_RenderDrawPoint(renderer, face->mouth_x + i, face->mouth_y - 5 + t);
                }
                // Bottom lip
                for (int i = 0; i < face->mouth_width; i++) {
                    SDL_RenderDrawPoint(renderer, face->mouth_x + i, face->mouth_y + 15 + t);
                }
            }
            break;
            
        case 'T': // Wide open
            for (int t = 0; t < thickness; t++) {
                // Top lip
                for (int i = 0; i < face->mouth_width; i++) {
                    SDL_RenderDrawPoint(renderer, face->mouth_x + i, face->mouth_y - 25 + t);
                }
                // Bottom lip
                for (int i = 0; i < face->mouth_width; i++) {
                    SDL_RenderDrawPoint(renderer, face->mouth_x + i, face->mouth_y + 25 + t);
                }
            }
            break;
            
        case 'L': // Narrow opening
            for (int t = 0; t < thickness; t++) {
                // Top lip
                for (int i = 0; i < face->mouth_width; i++) {
                    SDL_RenderDrawPoint(renderer, face->mouth_x + i, face->mouth_y - 3 + t);
                }
                // Bottom lip  
                for (int i = 0; i < face->mouth_width; i++) {
                    SDL_RenderDrawPoint(renderer, face->mouth_x + i, face->mouth_y + 5 + t);
                }
            }
            break;
            
        default: // Default closed mouth
            for (int t = 0; t < thickness; t++) {
                for (int i = 0; i < face->mouth_width; i++) {
                    int y_offset = (int)(face->mouth_smile * sin(M_PI * i / face->mouth_width));
                    SDL_RenderDrawPoint(renderer, face->mouth_x + i, face->mouth_y + y_offset + t);
                }
            }
    }
}

void update_face(Face* face, int eye_squint, int smile_curve) {
    // Update face parameters based on input
}

void cleanup_face(Face* face) {
    if (face->font) {
        TTF_CloseFont(face->font);
        face->font = NULL;
    }
    // Note: We don't delete the eye shapes here as they're managed by the vectorRenderer
}

void move_face(int smile) {
    // Move the face
    update_face(&face, 0, smile);
}

