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
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

// Functions
void look(bool right);
void enable_raw_mode();
void disable_raw_mode();
void read_arrow_keys();

// Main
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
            }
            t++;
            this_thread::sleep_for(chrono::milliseconds(10));
        }
    });
    movement_thread.detach();

    //enable_raw_mode();

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

        // Read arrow keys from terminal
        //read_arrow_keys();

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
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
    disable_raw_mode();
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

void enable_raw_mode() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~(ICANON); // | ECHO); // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    // Set stdin to non-blocking mode
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void disable_raw_mode() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= (ICANON | ECHO); // Re-enable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    // Restore stdin to blocking mode
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

void read_arrow_keys() {
    char ch;
    ch = getchar();
    if (ch == '\x1B') { // Start of escape sequence
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) > 0 &&
                read(STDIN_FILENO, &seq[1], 1) > 0) {
                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'A': std::cout << "Up arrow pressed\n"; break;
                        case 'B': std::cout << "Down arrow pressed\n"; break;
                        case 'C': std::cout << "Right arrow pressed\n"; break;
                        case 'D': std::cout << "Left arrow pressed\n"; break;
                    }
                }
            }
    } else if (ch > 0) {
            std::cout << "Key pressed: " << ch << "\n";
    }
    if (ch == 'q') exit(0);
}

