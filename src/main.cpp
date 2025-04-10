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
    bool fullscreen = false;
    #ifdef __linux__
    fullscreen = true;
    #endif
    if (create_window(fullscreen) != 0) printf("Could not create window\n");

    // Create face
    face = create_face(screen_width, screen_height);

    // Speech
    bool quit = false;
    thread speech_thread([argc, argv, &quit]() {
        speak(quit);
    });
    speech_thread.detach();

    // Movement
    thread movement_thread([argc, argv, quit]() {
        while (!quit) {
            // Look
            static int t = 400;
            static bool right = false;
            if (t > 30) {
                t = 0;

//                look(right);
//                right = !right;

                if      (face.mouth_shape == '_' ) face.mouth_shape = 'M';
                else if (face.mouth_shape == 'M' ) face.mouth_shape = 'F';
                else if (face.mouth_shape == 'F' ) face.mouth_shape = 'T';
                else if (face.mouth_shape == 'T' ) face.mouth_shape = 'L';
                else if (face.mouth_shape == 'L' ) face.mouth_shape = '_';
                cout << face.mouth_shape << endl;
            }
            t++;
            this_thread::sleep_for(chrono::milliseconds(10));
        }
    });
    movement_thread.detach();

    // Process keyboard input on main thread
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

        // Check for keyboard input from stdin
        if (cin.rdbuf()->in_avail()) {
            char input = cin.get();
            switch(input) {
                case 'A': // Up arrow
                    move_head(0, 40); 
                    update_face(&face, 0, 1);
                    break;
                case 'B': // Down arrow
                    move_head(0, -40);
                    update_face(&face, 0, -1);
                    break;
                case 'C': // Right arrow
                    move_head(-40, 0);
                    break;
                case 'D': // Left arrow
                    move_head(40, 0);
                    break;
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

