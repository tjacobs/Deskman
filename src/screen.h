#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// Screen
extern int screen_width;
extern int screen_height;
extern SDL_Renderer* renderer;

bool create_window(bool fullscreen);
bool close_window();

