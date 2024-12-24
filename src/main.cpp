
// Listen to speech, show images based on commands spoken.

#include "image.h"
#include "box.h"
#include "face.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

int mainWhisper(int argc, char ** argv);

extern int screen_width;
extern int screen_height;

int main(int argc, char ** argv) {

    // Create window
    create_window();

    // Listen
    if (false) mainWhisper(argc, argv);

    // Test rendering
    if (true) {
        // Show image
        //show_image("head.jpg");

        Face face = create_face(screen_width, screen_height);

        // Process input
        bool quit = false;
        SDL_Event event;
        while (!quit) {
            // Get events
            while (SDL_PollEvent(&event) != 0) {
                if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) quit = true;

                // Adjust face with keys
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_UP)    update_face(&face, face.eye_height, face.mouth_smile + 1);
                    if (event.key.keysym.sym == SDLK_DOWN)  update_face(&face, face.eye_height, face.mouth_smile - 1);
                    if (event.key.keysym.sym == SDLK_RIGHT) update_face(&face, face.eye_height, 10);
                    if (event.key.keysym.sym == SDLK_LEFT)  update_face(&face, face.eye_height, -10);
                }
            }

            // Clear the screen
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderClear(renderer);

            // Render the face
            render_face(renderer, &face);

            // Present the updated screen
            SDL_RenderPresent(renderer);

            // Wait
            SDL_Delay(16);
        }

        // Done
        close_window();
        return 0;
    }

    return 0;
}