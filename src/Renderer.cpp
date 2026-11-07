
#include "Renderer.h"

#include <cmath>
#include <algorithm>

namespace {
    constexpr int SCREEN_WIDTH = 1200;
    constexpr int SCREEN_HEIGHT = 800;

    Color getVortexColor(int index) {
        const Color colors[] = {
            SKYBLUE,
            BLUE,
            PURPLE,
            MAGENTA,
            GREEN,
            ORANGE,
            YELLOW
        };

        return colors[index % 7];
    }
}

bool Renderer::drawSlider(
    int id,
    int x,
    int y,
    int width,
    const char* name,
    float& value,
    float minValue,
    float maxValue
) {
    DrawText(name, x, y, 17, RAYWHITE);

    int barY = y + 27;

    DrawRectangle(x, barY, width, 8, DARKGRAY);

    float t = (value - minValue) / (maxValue - minValue);
    t = std::clamp(t, 0.0f, 1.0f);

    DrawRectangle(
        x,
        barY,
        static_cast<int>(width * t),
        8,
        SKYBLUE
    );

    int knobX = x + static_cast<int>(width * t);

    DrawCircle(knobX, barY + 4, 8, WHITE);

    DrawText(
        TextFormat("%.2f", value),
        x + width + 12,
        barY - 5,
        17,
        YELLOW
    );

    Rectangle hitbox = {
        static_cast<float>(x - 5),
        static_cast<float>(y),
        static_cast<float>(width + 75),
        45.0f
    };

    Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(mouse, hitbox)) {
        activeSlider = id;
    }

    if (activeSlider == id &&
        IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {

        float position = std::clamp(
            (mouse.x - x) / width,
            0.0f,
            1.0f
        );

        value = minValue +
            position * (maxValue - minValue);
    }

    return activeSlider == id;
}

bool Renderer::drawButton(
    int x,
    int y,
    int width,
    int height,
    const char* name
) {
    Rectangle rect = {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(width),
        static_cast<float>(height)
    };

    bool hover = CheckCollisionPointRec(
        GetMousePosition(), rect
    );

    DrawRectangleRec(
        rect,
        hover ? BLUE : DARKBLUE
    );

    DrawRectangleLinesEx(rect, 1, SKYBLUE);

    DrawText(name, x + 12, y + 10, 17, WHITE);

    return hover &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void Renderer::drawWorld(const Simulation& simulation) {
    Camera3D camera = {0};

    camera.target = {0, 0, 0};

    camera.position = {
        cameraDistance * cosf(cameraPitch) * sinf(cameraYaw),
        cameraDistance * sinf(cameraPitch),
        cameraDistance * cosf(cameraPitch) * cosf(cameraYaw)
    };

    camera.up = {0, 1, 0};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(camera);

    // Координатная сетка
    DrawGrid(20, 1.0f);

    // Образец сверхпроводника
    DrawCubeWires(
        {0, 0, 0},
        20.0f,
        20.0f,
        12.0f,
        DARKGRAY
    );

    const auto& vortices = simulation.getVortices();

    // Отрисовка вихревых линий
    for (int i = 0; i < static_cast<int>(vortices.size()); i++) {
        const auto& points = vortices[i].points;

        Color color = getVortexColor(i);

        for (int k = 0; k < static_cast<int>(points.size()) - 1; k++) {

            DrawCylinderEx(
                points[k],
                points[k + 1],
                0.10f,
                0.10f,
                8,
                color
            );

            DrawSphere(points[k], 0.12f, WHITE);
        }

        if (!points.empty()) {
            DrawSphere(points.back(), 0.12f, WHITE);
        }
    }

    EndMode3D();
}

void Renderer::drawInterface(
    Simulation& simulation,
    Settings& settings,
    bool& paused
) {
    DrawRectangle(
        0, 0, 310, SCREEN_HEIGHT,
        {20, 25, 42, 245}
    );

    DrawText(
        "ABRIKOSOV VORTEX",
        18, 20, 23, SKYBLUE
    );

    DrawText(
        "3D SIMULATION",
        18, 48, 17, WHITE
    );

    DrawLine(15, 78, 290, 78, GRAY);

    bool countActive = drawSlider(
        0, 20, 95, 180,
        "Vortex count",
        countValue,
        4.0f, 36.0f
    );

    if (countActive) {
        int newCount = static_cast<int>(
            std::round(countValue)
        );

        newCount = std::clamp(newCount, 4, 36);

        if (newCount != settings.vortexCount) {
            settings.vortexCount = newCount;
            simulation.initialize(settings);
        }
    }

    drawSlider(
        1, 20, 155, 180,
        "Temperature (T/Tc)",
        settings.temperature,
        0.05f, 0.99f
    );

    drawSlider(
        2, 20, 215, 180,
        "Penetration depth",
        settings.penetrationDepth,
        0.5f, 6.0f
    );

    drawSlider(
        3, 20, 275, 180,
        "Interaction strength",
        settings.interactionStrength,
        0.1f, 5.0f
    );

    drawSlider(
        4, 20, 335, 180,
        "Pinning strength",
        settings.pinningStrength,
        0.0f, 5.0f
    );

    drawSlider(
        5, 20, 395, 180,
        "Viscosity",
        settings.viscosity,
        0.2f, 5.0f
    );

    drawSlider(
        6, 20, 455, 180,
        "Line elasticity",
        settings.elasticity,
        0.1f, 3.0f
    );

    if (drawButton(
        20, 525, 120, 40,
        paused ? "RESUME" : "PAUSE"
    )) {
        paused = !paused;
    }

    if (drawButton(
        155, 525, 135, 40,
        "RESET"
    )) {
        simulation.initialize(settings);
    }

    DrawLine(15, 585, 290, 585, GRAY);

    DrawText(
        TextFormat(
            "Vortices: %d",
            settings.vortexCount
        ),
        20, 605, 18, WHITE
    );

    DrawText(
        TextFormat(
            "Temperature: %.2f Tc",
            settings.temperature
        ),
        20, 632, 17, SKYBLUE
    );

    DrawText(
        paused ? "STATUS: PAUSED" : "STATUS: RUNNING",
        20, 660, 18,
        paused ? ORANGE : GREEN
    );

    DrawText("CONTROLS:", 20, 705, 17, YELLOW);

    DrawText(
        "Right mouse - rotate",
        20, 730, 15, LIGHTGRAY
    );

    DrawText(
        "Mouse wheel - zoom",
        20, 750, 15, LIGHTGRAY
    );
}

void Renderer::draw(
    Simulation& simulation,
    Settings& settings,
    bool& paused
) {
    // Управление камерой
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) &&
        GetMouseX() > 310) {

        Vector2 delta = GetMouseDelta();

        cameraYaw -= delta.x * 0.006f;
        cameraPitch += delta.y * 0.006f;

        cameraPitch = std::clamp(
            cameraPitch, -1.4f, 1.4f
        );
    }

    cameraDistance -= GetMouseWheelMove() * 1.5f;

    cameraDistance = std::clamp(
        cameraDistance, 12.0f, 55.0f
    );

    BeginDrawing();

    ClearBackground({10, 13, 25, 255});

    drawWorld(simulation);

    drawInterface(simulation, settings, paused);

    EndDrawing();

    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        activeSlider = -1;
    }
}
