
#include "raylib.h"

#include "Settings.h"
#include "Simulation.h"
#include "Renderer.h"

int main() {
    const int SCREEN_WIDTH = 1200;
    const int SCREEN_HEIGHT = 800;

    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "Abrikosov Vortex Simulation"
    );

    SetTargetFPS(60);

    Settings settings;

    Simulation simulation;
    Renderer renderer;

    simulation.initialize(settings);

    bool paused = false;

    while (!WindowShouldClose()) {

        if (!paused) {
            simulation.update(settings);
        }

        renderer.draw(
            simulation,
            settings,
            paused
        );
    }

    CloseWindow();

    return 0;
}
