
// Listen to speech, show images based on commands spoken.

#include "image.h"
#include "box.h"
#include "face.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

int main(int argc, char ** argv) {

    // Create window
    create_window();

    // Test rendering
    if (true) {
        // Show image
        show_image("head.jpg");

        Box box = create_box(100, 100, 50, 50, 5, 5); // Initial position, size, and speed

        // Process input
        bool quit = false;
        SDL_Event event;
        while (!quit) {
            while (SDL_PollEvent(&event) != 0)
                if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) quit = true;

            // Move the box
            move_box(&box, 800, 600); // Assuming 800x600 screen size

            // Clear the screen
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderClear(renderer);

            // Render the box
            render_box(renderer, &box);

            // Present the updated screen
            SDL_RenderPresent(renderer);

            SDL_Delay(16);
        }

        // Done
        close_window();
        return 0;
    }

    return 0;
}