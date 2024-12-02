#include "face.h"

Face create_face(int center_x, int center_y) {
    Face face;

    // Eyes
    face.eye_left_x = center_x - 50;
    face.eye_left_y = center_y - 50;
    face.eye_right_x = center_x + 50;
    face.eye_right_y = center_y - 50;
    face.eye_width = 30;
    face.eye_height = 15;

    // Mouth
    face.mouth_x = center_x - 100;
    face.mouth_y = center_y + 100;
    face.mouth_width = 200;
    face.mouth_height = 20;
    face.mouth_smile = 10;

    return face;
}

void render_face(SDL_Renderer* renderer, Face* face) {
    // Render eyes
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black for eyes
    SDL_Rect left_eye = {face->eye_left_x - face->eye_width, face->eye_left_y, face->eye_width, face->eye_height};
    SDL_Rect right_eye = {face->eye_right_x, face->eye_right_y, face->eye_width, face->eye_height};
    SDL_RenderFillRect(renderer, &left_eye);
    SDL_RenderFillRect(renderer, &right_eye);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black for mouth line
    int thickness = 5; // Thickness of the mouth line
    for (int t = 0; t < thickness; t++) {
        for (int i = 0; i < face->mouth_width; i++) {
            // Calculate the curve offset for each point on the mouth
            int y_offset = (int)(face->mouth_smile * sin(M_PI * i / face->mouth_width));
            SDL_RenderDrawPoint(renderer, face->mouth_x + i, face->mouth_y + y_offset + t);
        }
    }
}

void update_face(Face* face, int eye_height, int smile_curve) {
    face->eye_height = eye_height;
    face->mouth_smile = smile_curve;
}
