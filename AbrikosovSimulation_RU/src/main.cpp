
#include "raylib.h"

#include "Settings.h"
#include "Simulation.h"
#include "Renderer.h"

int main() {
    const int SCREEN_WIDTH = 1200;
    const int SCREEN_HEIGHT = 800;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "Моделирование вихрей Абрикосова"
    );
    SetWindowMinSize(1000, 700);

    SetTargetFPS(60);

    Settings settings;

    Simulation simulation;
    Renderer renderer;

    simulation.initialize(settings);

    bool paused = false;

    while (!WindowShouldClose()) {
        // F11 разворачивает окно на весь экран рабочего стола и возвращает прежний размер.
        if (IsKeyPressed(KEY_F11)) {
            if (IsWindowMaximized()) RestoreWindow();
            else MaximizeWindow();
        }

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
