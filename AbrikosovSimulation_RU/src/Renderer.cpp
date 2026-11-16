
#include "Renderer.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include "Magnet.h"
#include "raymath.h"

namespace {
    constexpr int SCREEN_WIDTH = 1200;
    constexpr int SCREEN_HEIGHT = 800;

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


Renderer::Renderer() {
    // Встроенный шрифт DejaVu Sans содержит кириллицу. Загружаем
    // расширенный набор символов, чтобы русский текст не превращался в «????».
    std::vector<int> codepoints;
    for (int cp = 32; cp <= 0x04FF; ++cp) codepoints.push_back(cp);
    uiFont = LoadFontEx("assets/DejaVuSans.ttf", 32, codepoints.data(),
                        static_cast<int>(codepoints.size()));
    if (uiFont.texture.id != 0) SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
}

Renderer::~Renderer() {
    if (uiFont.texture.id != 0) UnloadFont(uiFont);
}

void Renderer::drawText(const char* text, float x, float y, float size, Color color) {
    if (uiFont.texture.id != 0) {
        DrawTextEx(uiFont, text, {x, y}, size, 1.0f, color);
    } else {
        DrawText(text, static_cast<int>(x), static_cast<int>(y), static_cast<int>(size), color);
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
    drawText(name, (float)x, (float)y, 17, RAYWHITE);

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

    drawText(TextFormat("%.2f", value), (float)(x + width + 12),
             (float)(barY - 5), 17, YELLOW);

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

    drawText(name, (float)(x + 12), (float)(y + 10), 17, WHITE);

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

    // Вихри отображаются только точечными маркерами.
    // Каждая цепочка точек — дискретное представление одной вихревой линии
    // (квантованной трубки магнитного потока) внутри объёма сверхпроводника.
    // Соединительные цилиндры намеренно не рисуются.
    const auto& vortices = simulation.getVortices();
    const Color pointColor = {225, 225, 225, 255};
    for (const auto& vortex : vortices) {
        for (const Vector3& point : vortex.points) {
            // Однотонные маркеры: без индивидуальной окраски и без свечения.
            DrawSphere(point, 0.13f, pointColor);
        }
    }
    EndMode3D();
}

void Renderer::drawInterface(
    Simulation& simulation,
    Settings& settings,
    bool& paused
) {
    DrawRectangle(0, 0, 365, GetScreenHeight(), {20, 25, 42, 245});
    drawText("ВИХРИ АБРИКОСОВА", 18, 18, 22, SKYBLUE);
    drawText("ТРЁХМЕРНАЯ МОДЕЛЬ", 18, 44, 16, WHITE);
    DrawLine(15, 70, 345, 70, GRAY);

    if (drawButton(18, 80, 155, 34, "ОБРАЗЕЦ")) magnetTab = false;
    if (drawButton(185, 80, 160, 34, "МАГНИТ")) magnetTab = true;

    if (!magnetTab) {
        // Выбор материала циклической кнопкой. Значения Tc — справочные,
        // температура ползунка ограничивается критической температурой материала.
        static const char* materialNames[] = {
            "Ниобий-титан (NbTi)",
            "Ниобий-олово (Nb3Sn)",
            "Диборид магния (MgB2)",
            "Керамика YBCO"
        };
        static const float criticalTemperatures[] = {9.2f, 18.0f, 39.0f, 92.0f};

        const int material = std::clamp(settings.materialIndex, 0, 3);
        settings.materialIndex = material;
        settings.criticalTemperature = criticalTemperatures[material];

        if (drawButton(20, 125, 325, 34, TextFormat("Материал: %s", materialNames[material]))) {
            settings.materialIndex = (settings.materialIndex + 1) % 4;
            settings.criticalTemperature = criticalTemperatures[settings.materialIndex];
            if (settings.temperature > settings.criticalTemperature) {
                settings.temperature = settings.criticalTemperature;
            }
        }
        drawText(TextFormat("Критическая температура: %.1f К", settings.criticalTemperature),
                 20, 164, 13, LIGHTGRAY);

        bool countActive = drawSlider(0, 20, 190, 205, "Количество вихрей", countValue, 4.0f, 36.0f);
        if (countActive) {
            int newCount = std::clamp((int)std::round(countValue), 4, 36);
            if (newCount != settings.vortexCount) {
                settings.vortexCount = newCount;
                simulation.initialize(settings);
            }
        }
        drawSlider(1, 20, 250, 205, "Температура (К)", settings.temperature, 4.2f, settings.criticalTemperature);
        drawSlider(2, 20, 310, 205, "Глубина проникновения", settings.penetrationDepth, 0.5f, 6.0f);
        drawSlider(4, 20, 370, 205, "Сила закрепления", settings.pinningStrength, 0.0f, 5.0f);
        drawSlider(5, 20, 430, 205, "Вязкость", settings.viscosity, 0.2f, 5.0f);
        drawText("Вихри возникают при поле выше Hc1", 20, 490, 12, LIGHTGRAY);
        drawText("Внешний ток не требуется", 20, 515, 12, LIGHTGRAY);
        drawText("Взаимодействие вихрей и упругость отключены", 20, 540, 10, GRAY);
    } else {
        drawText("ВНЕШНИЙ ПОЛОСОВОЙ МАГНИТ", 20, 128, 19, YELLOW);
        drawText("Разместите внутри или снаружи куба", 20, 151, 12, LIGHTGRAY);
        drawSlider(10, 20, 178, 205, "Координата X", settings.magnetPosition.x, -25.0f, 25.0f);
        drawSlider(11, 20, 238, 205, "Координата Y", settings.magnetPosition.y, -25.0f, 25.0f);
        drawSlider(12, 20, 298, 205, "Координата Z", settings.magnetPosition.z, -25.0f, 25.0f);
        drawSlider(13, 20, 358, 205, "Сила магнитного поля", settings.magnetStrength, 0.0f, 60.0f);
        drawSlider(14, 20, 418, 205, "Наклон X (градусы)", settings.magnetRotation.x, -180.0f, 180.0f);
        drawSlider(15, 20, 478, 205, "Наклон Y (градусы)", settings.magnetRotation.y, -180.0f, 180.0f);
        drawSlider(16, 20, 538, 205, "Поворот Z (градусы)", settings.magnetRotation.z, -180.0f, 180.0f);
        if (drawButton(20, 600, 325, 36, settings.magnetEnabled ? "МАГНИТ: ВКЛ." : "МАГНИТ: ВЫКЛ.")) {
            settings.magnetEnabled = !settings.magnetEnabled;
        }
        drawText("Перетаскивайте магнит левой кнопкой мыши", 20, 646, 13, LIGHTGRAY);
        drawText("Золотые линии: направление поля", 20, 666, 12, LIGHTGRAY);
        drawText("Поле выше Hc1 создаёт вихри", 20, 684, 11, LIGHTGRAY);
    }

    if (drawButton(20, 700, 145, 38, paused ? "ПРОДОЛЖИТЬ" : "ПАУЗА")) paused = !paused;
    if (drawButton(185, 700, 160, 38, "СБРОСИТЬ ВИХРИ")) simulation.initialize(settings);

    drawText(TextFormat("Вихрей: %d", (int)simulation.getVortices().size()), 20, 750, 16, WHITE);

    // Компактное окно с текущими оценками. Поле модели не имеет калибровки в теслах,
    // поэтому физическая формула показана отдельно и не выдаёт ложное численное значение.
    const int panelW = 315;
    const int panelH = 174;
    const int panelX = std::max(375, GetScreenWidth() - panelW - 18);
    const int panelY = 18;
    DrawRectangle(panelX, panelY, panelW, panelH, {20, 25, 42, 225});
    DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
    drawText("РАСЧЁТ ВИХРЕЙ", panelX + 12, panelY + 10, 17, SKYBLUE);
    DrawLine(panelX + 10, panelY + 36, panelX + panelW - 10, panelY + 36, GRAY);

    const float averageField = simulation.getAverageField(settings);
    const int targetCount = simulation.getTargetVortexCount(settings);
    drawText(TextFormat("Среднее |Bz| модели: %.5f", averageField),
             panelX + 12, panelY + 46, 13, WHITE);
    drawText("Порог Hc1: 0.00150  |  насыщение: 0.018",
             panelX + 12, panelY + 67, 10, LIGHTGRAY);
    drawText(TextFormat("Доля заполнения: %.1f%%", averageField <= 0.0015f ? 0.0f :
             std::clamp((averageField - 0.0015f) / (0.018f - 0.0015f), 0.0f, 1.0f) * 100.0f),
             panelX + 12, panelY + 86, 12, WHITE);
    drawText(TextFormat("Целевое число по модели: %d из %d", targetCount, settings.vortexCount),
             panelX + 12, panelY + 106, 12, YELLOW);
    drawText("Физическая оценка: N = B * S / Phi0", panelX + 12, panelY + 128, 12, SKYBLUE);
    drawText("Phi0 = 2.068e-15 Wb; нужны B (Тл) и S (м²)", panelX + 12, panelY + 148, 9, LIGHTGRAY);
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
