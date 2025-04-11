// Deskman robot.
// Main program for robot control.
// Thomas Jacobs, 2025.

#include "face.h"
#include "speak.h"
#include "screen.h"
#include "servos.h"
#include "vector_renderer.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

void look(bool right);

// Global vector renderer
VectorRenderer vectorRenderer;

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

    // Create vector shapes for the face
    Ellipse* leftEye = new Ellipse(45, 120, {255, 255, 255, 255}, {255, 255, 255, 255}, 0.0f);
    leftEye->localPosition = Vec3(-120, -100, 0);
    
    Ellipse* rightEye = new Ellipse(45, 120, {255, 255, 255, 255}, {255, 255, 255, 255}, 0.0f);
    rightEye->localPosition = Vec3(120, -100, 0);
    
    // Create mouth with cutout
    Ellipse* mouth = new Ellipse(240, 80, {255, 255, 255, 255}, {255, 255, 255, 255}, 0.0f, -40, 180);
    mouth->localPosition = Vec3(0, 200, 0);
    
    // Add shapes to renderer
    vectorRenderer.addShape(leftEye);
    vectorRenderer.addShape(rightEye);
//    vectorRenderer.addShape(mouth);

    // Animation variables
    float time = 0.0f;
    const float animationSpeed = 0.02f;
    const float maxTilt = 15.0f;
    const float blinkSpeed = 8.0f;
    const float blinkInterval = 10.0f;
    float blinkTimer = 0.0f;
    bool isBlinking = false;

    // Look direction variables
    float lookTimer = 0.0f;
    const float lookInterval = 5.0f;  // Look in new direction every 5 seconds
    const float eyeMoveDuration = 0.5f;  // Time for eyes to move
    const float headMoveDuration = 1.0f;  // Time for head to follow
    bool isLooking = false;
    float targetX = 0.0f;
    float targetY = 0.0f;
    float lookTiltX = 0.0f;
    float lookTiltY = 0.0f;
    float currentHeadX = 0.0f;
    float currentHeadY = 0.0f;
    float lookStartTime = 0.0f;
    int lookDirection = 0;  // 0: left, 1: right, 2: up, 3: down

    // Speech
    bool quit = false;
    bool speak_go = false;
    thread speech_thread([argc, argv, &quit, &speak_go]() {
        while (!speak_go) {
            this_thread::sleep_for(chrono::milliseconds(100));
            if (quit) return;
        }
        cout << "Speaking" << endl;
        speak(quit);
    });

    // Movement
    thread movement_thread([argc, argv, &quit]() {
        while (!quit) {
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
                if (event.key.keysym.sym == SDLK_j)     { move_head(900, 0); update_face(&face, 5, 0); }
                if (event.key.keysym.sym == SDLK_l)     { move_head(-900, 0); update_face(&face, -5, 0); }
                if (event.key.keysym.sym == SDLK_i)     { move_head(0, 200); update_face(&face, 5, 0); }
                if (event.key.keysym.sym == SDLK_k)     { move_head(0, -200); update_face(&face, -5, 0); }

                // Speak
                if (event.key.keysym.sym == SDLK_SPACE) { speak_go = true; }
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

        // Update animation
        time += animationSpeed;
        float tiltX = 0; //sin(time) * maxTilt;
        float tiltY = 0; //cos(time * 0.5f) * maxTilt * 0.3f;

        // Update looking behavior
        lookTimer += 0.016f;  // Assuming ~60fps
        if (!isLooking && lookTimer >= lookInterval) {
            isLooking = true;
            lookTimer = 0.0f;
            lookStartTime = time;
            
            // Choose random look target (-1 to 1 range)
            //targetX = (rand() % 200 - 100) / 100.0f;
            //targetY = (rand() % 200 - 100) / 100.0f;

            // Cycle through directions
            switch(lookDirection) {
                case 0:  // Left
                    targetX = -1.0f;
                    targetY = 0.0f;
                    break;
                case 1:  // Right
                    targetX = 1.0f;
                    targetY = 0.0f;
                    break;
                case 2:  // Up
                    targetX = 0.0f;
                    targetY = -1.0f;
                    break;
                case 3:  // Down
                    targetX = 0.0f;
                    targetY = 1.0f;
                    break;
            }
            lookDirection = (lookDirection + 1) % 4;
            
            // Reset current positions
            lookTiltX = 0.0f;
            lookTiltY = 0.0f;
            currentHeadX = 0.0f;
            currentHeadY = 0.0f;
        }

        // Look
        if (isLooking) {
            float lookTime = time - lookStartTime;
            
            // Tilt face first
            if (lookTime < eyeMoveDuration) {
                float t = lookTime / eyeMoveDuration;
                lookTiltX = targetX * t * 20.0f;  // Up to 20 degrees tilt
                lookTiltY = targetY * t * 20.0f;
            } else {
                lookTiltX = targetX * 20.0f;
                lookTiltY = targetY * 20.0f;
                
                // Then move head
                if (lookTime < (eyeMoveDuration + headMoveDuration)) {
                    float t = (lookTime - eyeMoveDuration) / headMoveDuration;
                    currentHeadX = targetX * t;
                    currentHeadY = targetY * t;
                    
                    // Move servos
                    //move_head(currentHeadX * 100, currentHeadY * 100);

                    // Gradually reduce tilt as head moves
                    lookTiltX *= (1.0f - t);
                    lookTiltY *= (1.0f - t);
                } else {
                    currentHeadX = targetX;
                    currentHeadY = targetY;
                    lookTiltX = 0.0f;
                    lookTiltY = 0.0f;
                    isLooking = false;
                }
            }
        }

        // Apply combined rotation to the face (base animation + looking tilt)
        float finalTiltX = sin(time) * maxTilt + lookTiltX;
        float finalTiltY = cos(time * 0.5f) * maxTilt * 0.3f + lookTiltY;
        vectorRenderer.setFaceRotation(Vec3(lookTiltY, lookTiltX, 0));

        // Update blinking
        blinkTimer += animationSpeed;
        if (!isBlinking && blinkTimer >= blinkInterval) {
            isBlinking = true;
            blinkTimer = 0.0f;
        }

        // Animate eyes
        float blinkProgress = 0.0f;
        if (isBlinking) {
            blinkProgress = sin(blinkTimer * blinkSpeed);
            if (blinkProgress < 0) {
                isBlinking = false;
                blinkProgress = 0.0f;
            }
        }

        // Update eye shapes
        float baseHeight = 120.0f;
        float eyeHeight = baseHeight * (1.0f - blinkProgress * 0.9f);
        leftEye->radiusY = eyeHeight;
        rightEye->radiusY = eyeHeight;

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 81, 74, 63, 255);
        SDL_RenderClear(renderer);

        // Render the face
        vectorRenderer.render(renderer);

        // Draw coordinate text
        char coordText[100];
        snprintf(coordText, sizeof(coordText), "Head X: %.1f  Y: %.1f", currentHeadX * 100, currentHeadY * 100);
        SDL_Color textColor = {0, 0, 0, 255};
        SDL_Surface* textSurface = TTF_RenderText_Solid(face.font, coordText, textColor);
        if (textSurface != NULL) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect = {10, 10, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);
        }

        // Present the updated screen
        SDL_RenderPresent(renderer);

        // Wait
        SDL_Delay(16);
    }

    // Set quit flag and wait for threads to finish
    quit = true;
    if (speech_thread.joinable()) {
        speech_thread.join();
    }
    if (movement_thread.joinable()) {
        movement_thread.join();
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
