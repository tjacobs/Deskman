#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <curl/curl.h>

#define TEMP_IMAGE_FILE "downloaded_image.jpg"
#define IMAGE_URL "https://upload.wikimedia.org/wikipedia/commons/thumb/c/c8/Black_Labrador_Retriever_-_Male_IMG_3323.jpg/2560px-Black_Labrador_Retriever_-_Male_IMG_3323.jpg"

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream);
bool download_image(const char* url, const char* filename);

int main() {
    // Download the image
    if (!download_image(IMAGE_URL, TEMP_IMAGE_FILE)) {
        return 1;
    }

    // Initialize SDL objects to NULL
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Surface* image = NULL;
    SDL_Texture* texture = NULL;

    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize SDL_image for multiple image formats
    if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF) == 0) {
        fprintf(stderr, "SDL_image could not initialize: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create SDL window
    window = SDL_CreateWindow("Image Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window could not be created: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create renderer for the window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer could not be created: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load downloaded image
    image = IMG_Load(TEMP_IMAGE_FILE);
    if (!image) {
        fprintf(stderr, "Unable to load image: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create texture from loaded image
    texture = SDL_CreateTextureFromSurface(renderer, image);
    if (!texture) {
        fprintf(stderr, "Unable to create texture: %s\n", SDL_GetError());
        SDL_FreeSurface(image);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Event handling variables
    SDL_Event event;
    bool quit = false;

    // Main event loop
    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                quit = true;
            }
        }

        // Clear renderer
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Render texture
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        // Present renderer
        SDL_RenderPresent(renderer);

        SDL_Delay(16); // Limit frame rate
    }

    // Clean up
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(image);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}


// Callback function for libcurl to write data to a file
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

// Function to download image using libcurl
bool download_image(const char* url, const char* filename) {
    CURL* curl;
    FILE* file;
    CURLcode res;

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Could not initialize libcurl\n");
        return false;
    }

    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Could not open file for writing: %s\n", filename);
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    res = curl_easy_perform(curl);

    fclose(file);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to download image: %s\n", curl_easy_strerror(res));
        return false;
    }

    return true;
}

