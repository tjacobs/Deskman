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
    face.mouth_x = center_x - 50;
    face.mouth_y = center_y + 50;
    face.mouth_width = 100;
    face.mouth_height = 20;
    face.mouth_smile = 0;

    return face;
}

void render_face(SDL_Renderer* renderer, Face* face) {
    // Render eyes
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black for eyes

    SDL_Rect left_eye = {face->eye_left_x, face->eye_left_y, face->eye_width, face->eye_height};
    SDL_Rect right_eye = {face->eye_right_x, face->eye_right_y, face->eye_width, face->eye_height};

    SDL_RenderFillRect(renderer, &left_eye);
    SDL_RenderFillRect(renderer, &right_eye);

    // Render mouth
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for mouth

    SDL_Rect mouth = {face->mouth_x, face->mouth_y, face->mouth_width, face->mouth_height};
    SDL_RenderFillRect(renderer, &mouth);

    // Add mouth curvature (smile or frown)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black outline for smile curve
    for (int i = 0; i < face->mouth_width; i++) {
        int y_offset = (int)(face->mouth_smile * sin(M_PI * i / face->mouth_width));
        SDL_RenderDrawPoint(renderer, face->mouth_x + i, face->mouth_y + face->mouth_height / 2 + y_offset);
    }
}

void update_face(Face* face, int eye_squint, int smile_curve) {
    // Eye muscles control the height (squinting)
    face->eye_height = 15 - eye_squint; // Reduce eye height for squint

    // Mouth muscles control the curvature
    face->mouth_smile = smile_curve; // Set curvature for smile or frown
}
