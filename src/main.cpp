// Deskman robot.
// Main program for robot control.
// Thomas Jacobs, 2025.

#include "face.h"
#include "speak.h"
#include "screen.h"
#include "servos.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

void look(bool right);

int main(int argc, char **argv) {
    // Connect to servos
    if (open_servos() != 0) printf("Could not open servos\n");

    // Create window
    if (create_window(true) != 0) printf("Could not create window\n");

    // Create face
    face = create_face(screen_width, screen_height);

    // Process input
    bool quit = false;
    SDL_Event event;
    while (!quit) {
        // Get events
        while (SDL_PollEvent(&event) != 0) {
            // Quit on ESC
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) quit = true;

            // Adjust face with keys
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP)    { move_head(0, 40); update_face(&face, 0, 1); }
                if (event.key.keysym.sym == SDLK_DOWN)  { move_head(0, -40); update_face(&face, 0, -1); }
                if (event.key.keysym.sym == SDLK_RIGHT) { move_head(-40, 0); }
                if (event.key.keysym.sym == SDLK_LEFT)  { move_head(40, 0); }
                if (event.key.keysym.sym == SDLK_SPACE) { move_head(0, -500); update_face(&face, 5, 0); }
                if (event.key.keysym.sym == SDLK_j)     { move_head(900, 0); update_face(&face, 5, 0); }
                if (event.key.keysym.sym == SDLK_l)     { move_head(-900, 0); update_face(&face, -5, 0); }
                if (event.key.keysym.sym == SDLK_i)     { move_head(0, 200); update_face(&face, 5, 0); }
                if (event.key.keysym.sym == SDLK_k)     { move_head(0, -200); update_face(&face, -5, 0); }
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

        // Look
        static int t = 400;
        static bool right = false;
        if (t > 500) {
            look(right);
            speak();
            t = 0;
            right = !right;
        }
        t++;
    }

    // Done
    close_window();
    return 0;
}

void look(bool right) {
    cout << "Looking " << (right ? "right." : "left.") << endl;

    // Move head
    move_head((right ? 1 : -1) * 800, 0);
    this_thread::sleep_for(chrono::milliseconds(1000));
    move_head(0, 100);
    this_thread::sleep_for(chrono::milliseconds(2000));
    move_head((right ? 1 : -1) * -800, 0);
    this_thread::sleep_for(chrono::milliseconds(1000));
    move_head(0, -100);
}

