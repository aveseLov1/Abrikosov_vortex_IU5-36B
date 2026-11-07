
#include "Renderer.h"

#include <cmath>
#include <algorithm>
#include "Magnet.h"
#include "raymath.h"

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

    Camera3D makeCamera(float yaw, float pitch, float distance) {
        Camera3D camera = {0};
        camera.target = {0, 0, 0};
        camera.position = {
            distance * cosf(pitch) * sinf(yaw),
            distance * sinf(pitch),
            distance * cosf(pitch) * cosf(yaw)
        };
        camera.up = {0, 1, 0};
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        return camera;
    }

    bool intersectHorizontalPlane(Ray ray, float planeZ, Vector3& hit) {
        if (fabsf(ray.direction.z) < 0.0001f) return false;
        float t = (planeZ - ray.position.z) / ray.direction.z;
        if (t < 0.0f) return false;
        hit = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
        return true;
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

void Renderer::drawWorld(const Simulation& simulation, const Settings& settings) {
    Camera3D camera = makeCamera(cameraYaw, cameraPitch, cameraDistance);

    BeginMode3D(camera);
    DrawGrid(20, 1.0f);

    // Полупрозрачный объём образца и его границы.
    DrawCubeWires({0, 0, 0}, 20.0f, 20.0f, 12.0f, DARKGRAY);

    // Магнит остаётся видимым даже при выключенном поле, чтобы его можно было
    // свободно размещать. При OFF корпус и полюса становятся приглушёнными.
    {
        Vector3 pos = settings.magnetPosition;
        Vector3 axis = Magnet::getMomentDirection(settings.magnetRotation);
        Color bodyColor = settings.magnetEnabled ? Color{115, 125, 145, 255}
                                                 : Color{75, 80, 90, 180};
        Color northColor = settings.magnetEnabled ? BLUE : Color{70, 80, 100, 150};
        Color southColor = settings.magnetEnabled ? RED : Color{100, 70, 70, 150};
        DrawCubeV(pos, {3.2f, 1.25f, 1.25f}, bodyColor);
        DrawCubeWiresV(pos, {3.2f, 1.25f, 1.25f}, settings.magnetEnabled ? WHITE : GRAY);
        DrawSphere(Vector3Add(pos, Vector3Scale(axis, 0.85f)), 0.38f, northColor);
        DrawSphere(Vector3Subtract(pos, Vector3Scale(axis, 0.85f)), 0.38f, southColor);
        DrawLine3D(pos, Vector3Add(pos, Vector3Scale(axis, 2.5f)), settings.magnetEnabled ? YELLOW : GRAY);

        if (settings.magnetEnabled) {
            // Упрощённая визуализация направления поля внутри сверхпроводника.
            for (int ix = -1; ix <= 1; ix++) {
                for (int iy = -1; iy <= 1; iy++) {
                    for (int iz = -1; iz <= 1; iz++) {
                        Vector3 q = {(float)ix * 6.0f, (float)iy * 6.0f, (float)iz * 3.5f};
                        Vector3 b = Magnet::magneticField(
                            q, settings.magnetPosition,
                            settings.magnetRotation, settings.magnetStrength
                        );
                        float magnitude = Vector3Length(b);
                        if (magnitude > 0.0001f) {
                            Vector3 direction = Vector3Scale(Vector3Normalize(b), 0.85f);
                            DrawLine3D(q, Vector3Add(q, direction), {255, 190, 70, 190});
                        }
                    }
                }
            }
        }
    }

    const auto& vortices = simulation.getVortices();
    for (int i = 0; i < static_cast<int>(vortices.size()); i++) {
        const auto& points = vortices[i].points;
        Color color = getVortexColor(i);
        for (int k = 0; k < static_cast<int>(points.size()) - 1; k++) {
            DrawCylinderEx(points[k], points[k + 1], 0.10f, 0.10f, 8, color);
            DrawSphere(points[k], 0.12f, WHITE);
        }
        if (!points.empty()) DrawSphere(points.back(), 0.12f, WHITE);
    }
    EndMode3D();
}

void Renderer::drawInterface(
    Simulation& simulation,
    Settings& settings,
    bool& paused
) {
    DrawRectangle(0, 0, 365, SCREEN_HEIGHT, {20, 25, 42, 245});
    DrawText("ABRIKOSOV VORTEX", 18, 18, 22, SKYBLUE);
    DrawText("3D SIMULATION", 18, 44, 16, WHITE);
    DrawLine(15, 70, 345, 70, GRAY);

    if (drawButton(18, 80, 155, 34, "MATERIAL")) magnetTab = false;
    if (drawButton(185, 80, 160, 34, "MAGNET")) magnetTab = true;

    if (!magnetTab) {
        bool countActive = drawSlider(0, 20, 130, 205, "Vortex count", countValue, 4.0f, 36.0f);
        if (countActive) {
            int newCount = std::clamp((int)std::round(countValue), 4, 36);
            if (newCount != settings.vortexCount) {
                settings.vortexCount = newCount;
                simulation.initialize(settings);
            }
        }
        drawSlider(1, 20, 190, 205, "Temperature (T/Tc)", settings.temperature, 0.05f, 0.99f);
        drawSlider(2, 20, 250, 205, "Penetration depth", settings.penetrationDepth, 0.5f, 6.0f);
        drawSlider(3, 20, 310, 205, "Interaction strength", settings.interactionStrength, 0.1f, 5.0f);
        drawSlider(4, 20, 370, 205, "Pinning strength", settings.pinningStrength, 0.0f, 5.0f);
        drawSlider(5, 20, 430, 205, "Viscosity", settings.viscosity, 0.2f, 5.0f);
        drawSlider(6, 20, 490, 205, "Line elasticity", settings.elasticity, 0.1f, 3.0f);
    } else {
        DrawText("EXTERNAL BAR MAGNET", 20, 128, 19, YELLOW);
        DrawText("Place anywhere: inside or outside the cube", 20, 151, 12, LIGHTGRAY);
        drawSlider(10, 20, 178, 205, "Position X", settings.magnetPosition.x, -25.0f, 25.0f);
        drawSlider(11, 20, 238, 205, "Position Y", settings.magnetPosition.y, -25.0f, 25.0f);
        drawSlider(12, 20, 298, 205, "Position Z", settings.magnetPosition.z, -25.0f, 25.0f);
        drawSlider(13, 20, 358, 205, "Field strength", settings.magnetStrength, 0.0f, 60.0f);
        drawSlider(14, 20, 418, 205, "Tilt X (degrees)", settings.magnetRotation.x, -180.0f, 180.0f);
        drawSlider(15, 20, 478, 205, "Tilt Y (degrees)", settings.magnetRotation.y, -180.0f, 180.0f);
        drawSlider(16, 20, 538, 205, "Rotation Z (degrees)", settings.magnetRotation.z, -180.0f, 180.0f);
        if (drawButton(20, 600, 325, 36, settings.magnetEnabled ? "MAGNET: ON" : "MAGNET: OFF")) {
            settings.magnetEnabled = !settings.magnetEnabled;
        }
        DrawText("Drag magnet in 3D with left mouse", 20, 646, 13, LIGHTGRAY);
        DrawText("Gold lines: field direction (when ON)", 20, 666, 12, LIGHTGRAY);
    }

    if (drawButton(20, 700, 145, 38, paused ? "RESUME" : "PAUSE")) paused = !paused;
    if (drawButton(185, 700, 160, 38, "RESET VORTICES")) simulation.initialize(settings);

    DrawText(TextFormat("Vortices: %d", settings.vortexCount), 20, 750, 16, WHITE);
}

void Renderer::draw(
    Simulation& simulation,
    Settings& settings,
    bool& paused
) {
    Camera3D interactionCamera = makeCamera(cameraYaw, cameraPitch, cameraDistance);

    // Перетаскивание магнита мышью по плоскости XY на его текущей высоте.
    // Высоту Z можно менять ползунком, а точное положение — тремя ползунками.
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && GetMouseX() > 365) {
        Ray ray = GetScreenToWorldRay(GetMousePosition(), interactionCamera);
        Vector3 half = {1.9f, 1.0f, 1.0f};
        BoundingBox box = {
            Vector3Subtract(settings.magnetPosition, half),
            Vector3Add(settings.magnetPosition, half)
        };
        draggingMagnet = GetRayCollisionBox(ray, box).hit;
    }

    if (draggingMagnet && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Ray ray = GetScreenToWorldRay(GetMousePosition(), interactionCamera);
        Vector3 hit;
        if (intersectHorizontalPlane(ray, settings.magnetPosition.z, hit)) {
            settings.magnetPosition.x = std::clamp(hit.x, -25.0f, 25.0f);
            settings.magnetPosition.y = std::clamp(hit.y, -25.0f, 25.0f);
        }
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) draggingMagnet = false;

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && GetMouseX() > 365 && !draggingMagnet) {
        Vector2 delta = GetMouseDelta();
        cameraYaw -= delta.x * 0.006f;
        cameraPitch += delta.y * 0.006f;
        cameraPitch = std::clamp(cameraPitch, -1.4f, 1.4f);
    }
    cameraDistance -= GetMouseWheelMove() * 1.5f;
    cameraDistance = std::clamp(cameraDistance, 12.0f, 55.0f);

    BeginDrawing();
    ClearBackground({10, 13, 25, 255});
    drawWorld(simulation, settings);
    drawInterface(simulation, settings, paused);
    EndDrawing();

    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) activeSlider = -1;
}
