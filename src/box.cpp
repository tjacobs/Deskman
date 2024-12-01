#include "box.h"

Box create_box(int x, int y, int width, int height, int dx, int dy) {
    Box box;
    box.rect.x = x;
    box.rect.y = y;
    box.rect.w = width;
    box.rect.h = height;
    box.dx = dx;
    box.dy = dy;
    return box;
}

void render_box(SDL_Renderer* renderer, Box* box) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red box
    SDL_RenderFillRect(renderer, &box->rect);
}

void move_box(Box* box, int screen_width, int screen_height) {
    box->rect.x += box->dx;
    box->rect.y += box->dy;

    // Check boundaries
    if (box->rect.x < 0 || box->rect.x + box->rect.w > screen_width) {
        box->dx = -box->dx; // Reverse direction
    }
    if (box->rect.y < 0 || box->rect.y + box->rect.h > screen_height) {
        box->dy = -box->dy; // Reverse direction
    }
}
