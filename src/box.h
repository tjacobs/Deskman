#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// Box
typedef struct {
    SDL_Rect rect;
    int dx; // Speed in x direction
    int dy; // Speed in y direction
} Box;
Box create_box(int x, int y, int width, int height, int dx, int dy);
void render_box(SDL_Renderer* renderer, Box* box);
void move_box(Box* box, int screen_width, int screen_height);
