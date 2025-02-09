#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

bool create_window();
bool close_window();
bool create_image(const char* prompt);
bool show_image(const char* image_file);
bool generate_image(const char *prompt, char *image_id);
bool get_image_url(const char *image_id, char *image_url);
bool download_image(const char* url, const char* filename);
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);

extern SDL_Renderer* renderer;
