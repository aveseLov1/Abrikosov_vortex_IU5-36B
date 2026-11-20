
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

    // Краткое окно текущего расчёта и кнопка подробного отчёта.
    const float averageField = simulation.getAverageField(settings);
    const int targetCount = simulation.getTargetVortexCount(settings);
    constexpr float Hc1 = 0.0015f;
    constexpr float Hfull = 0.018f;
    constexpr float Phi0 = 2.068e-15f;
    const float fraction = averageField <= Hc1 ? 0.0f :
        std::clamp((averageField - Hc1) / (Hfull - Hc1), 0.0f, 1.0f);
    const int panelW = 315;
    const int panelH = 174;
    const int panelX = std::max(375, GetScreenWidth() - panelW - 18);
    const int panelY = 18;
    DrawRectangle(panelX, panelY, panelW, panelH, {20, 25, 42, 225});
    DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
    drawText("ТЕКУЩИЙ РАСЧЁТ", panelX + 12, panelY + 10, 17, SKYBLUE);
    DrawLine(panelX + 10, panelY + 36, panelX + panelW - 10, panelY + 36, GRAY);
    drawText(TextFormat("Среднее |Bz|: %.5f усл. ед.", averageField),
             panelX + 12, panelY + 46, 12, WHITE);
    drawText(TextFormat("Заполнение: %.1f%%", fraction * 100.0f),
             panelX + 12, panelY + 67, 12, WHITE);
    drawText(TextFormat("Целевое число: %d / %d", targetCount, settings.vortexCount),
             panelX + 12, panelY + 88, 12, YELLOW);
    drawText(TextFormat("Фактически создано: %d", (int)simulation.getVortices().size()),
             panelX + 12, panelY + 109, 12, WHITE);
    if (drawButton(panelX + 10, panelY + 132, panelW - 20, 32,
                   showCalculations ? "СКРЫТЬ ПОДРОБНЫЕ РАСЧЁТЫ" : "ПОКАЗАТЬ ВСЕ РАСЧЁТЫ")) {
        showCalculations = !showCalculations;
    }

    if (showCalculations) {
        // Подробный отчёт поверх сцены. Все значения пересчитываются каждый кадр
        // из текущих настроек образца и магнита.
        const int w = std::min(760, GetScreenWidth() - 40);
        const int h = std::min(650, GetScreenHeight() - 40);
        const int x = (GetScreenWidth() - w) / 2;
        const int y = (GetScreenHeight() - h) / 2;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 175});
        DrawRectangle(x, y, w, h, {17, 23, 40, 250});
        DrawRectangleLinesEx({(float)x, (float)y, (float)w, (float)h}, 2, SKYBLUE);
        drawText("ПОДРОБНЫЕ РАСЧЁТЫ И ПАРАМЕТРЫ", x + 22, y + 16, 21, SKYBLUE);
        if (drawButton(x + w - 122, y + 12, 100, 34, "ЗАКРЫТЬ")) {
            showCalculations = false;
        }
        DrawLine(x + 20, y + 55, x + w - 20, y + 55, GRAY);

        static const char* materialNames[] = {
            "Ниобий-титан (NbTi)", "Ниобий-олово (Nb3Sn)",
            "Диборид магния (MgB2)", "Керамика YBCO"
        };
        const int material = std::clamp(settings.materialIndex, 0, 3);
        const float tempRatio = std::clamp(settings.temperature /
            std::max(settings.criticalTemperature, 0.001f), 0.0f, 0.999f);
        const float thermalFactor = settings.thermalStrength *
            std::sqrt(tempRatio / std::max(1.0f - tempRatio, 0.001f));
        const float pinSpacing = 2.5f;
        const int actualCount = (int)simulation.getVortices().size();

        int ty = y + 70;
        const int left = x + 24;
        const int right = x + w / 2 + 8;
        const int line = 25;
        drawText("ОБРАЗЕЦ И МАТЕРИАЛ", left, ty, 15, YELLOW); ty += 25;
        drawText(TextFormat("Материал: %s", materialNames[material]), left, ty, 12, WHITE); ty += line;
        drawText(TextFormat("Температура: %.2f K", settings.temperature), left, ty, 12, WHITE); ty += line;
        drawText(TextFormat("Критическая температура Tc: %.2f K", settings.criticalTemperature), left, ty, 12, WHITE); ty += line;
        drawText(TextFormat("T/Tc = %.4f (%.1f%% от Tc)", tempRatio, tempRatio * 100.0f), left, ty, 12, WHITE); ty += line;
        drawText(TextFormat("Тепловой множитель: %.5f", thermalFactor), left, ty, 12, WHITE); ty += line;
        drawText(TextFormat("Глубина проникновения: %.2f усл. ед.", settings.penetrationDepth), left, ty, 12, WHITE); ty += line;
        drawText("В текущей модели глубина не меняет число вихрей.", left, ty, 10, LIGHTGRAY);

        ty = y + 70;
        drawText("МАГНИТ И МАГНИТНОЕ ПОЛЕ", right, ty, 15, YELLOW); ty += 25;
        drawText(TextFormat("Магнит: %s", settings.magnetEnabled ? "ВКЛЮЧЁН" : "ВЫКЛЮЧЕН"), right, ty, 12, WHITE); ty += line;
        drawText(TextFormat("Сила магнита: %.2f усл. ед.", settings.magnetStrength), right, ty, 12, WHITE); ty += line;
        drawText(TextFormat("Положение: X %.1f | Y %.1f | Z %.1f", settings.magnetPosition.x, settings.magnetPosition.y, settings.magnetPosition.z), right, ty, 10, WHITE); ty += line;
        drawText(TextFormat("Поворот: X %.1f | Y %.1f | Z %.1f", settings.magnetRotation.x, settings.magnetRotation.y, settings.magnetRotation.z), right, ty, 10, WHITE); ty += line;
        drawText(TextFormat("Среднее |Bz| по 125 точкам: %.6f", averageField), right, ty, 11, WHITE); ty += line;
        drawText("Сетка: 5 x 5 x 5; шаг координат: 4", right, ty, 11, LIGHTGRAY); ty += line;
        drawText("Bср = сумма |Bz(i)| / 125", right, ty, 11, SKYBLUE);

        ty = y + 265;
        DrawLine(x + 20, ty, x + w - 20, ty, GRAY); ty += 12;
        drawText("РАСЧЁТ ЦЕЛЕВОГО ЧИСЛА ВИХРЕЙ", left, ty, 15, YELLOW); ty += 27;
        drawText(TextFormat("Hc1 = %.4f; Hfull = %.3f (условные единицы)", Hc1, Hfull), left, ty, 12, WHITE); ty += line;
        drawText(averageField <= Hc1 ? "Bср <= Hc1: вихри не создаются" :
            TextFormat("f = (Bср - Hc1) / (Hfull - Hc1) = %.4f", fraction), left, ty, 12, WHITE); ty += line;
        drawText(TextFormat("f = %.1f%%; максимум Nmax = %d", fraction * 100.0f, settings.vortexCount), left, ty, 12, WHITE); ty += line;
        drawText(TextFormat("Nцель = ceil(f * Nmax) = %d", targetCount), left, ty, 12, SKYBLUE); ty += line;
        drawText(TextFormat("Сейчас создано: %d; разница: %+d", actualCount, targetCount - actualCount), left, ty, 12, WHITE); ty += line;
        drawText("Обновление: максимум 1 вихрь за 0.12 модельного времени", left, ty, 10, LIGHTGRAY);

        ty = y + 265;
        drawText("ДВИЖЕНИЕ И ЗАКРЕПЛЕНИЕ", right, ty, 15, YELLOW); ty += 27;
        drawText(TextFormat("Вязкость: %.2f", settings.viscosity), right, ty, 12, WHITE); ty += line;
        drawText(TextFormat("Закрепление: %.2f", settings.pinningStrength), right, ty, 12, WHITE); ty += line;
        drawText(TextFormat("Шаг времени dt: %.4f", settings.timeStep), right, ty, 12, WHITE); ty += line;
        drawText(TextFormat("Шаг центров закрепления: %.1f", pinSpacing), right, ty, 12, WHITE); ty += line;
        drawText("Скорость движения пропорциональна", right, ty, 11, LIGHTGRAY); ty += 18;
        drawText("силе / вязкости; шум зависит от T/Tc.", right, ty, 11, LIGHTGRAY); ty += 22;
        drawText("Пиннинг тянет узлы к ближайшему центру.", right, ty, 11, LIGHTGRAY);

        ty = y + h - 82;
        DrawLine(x + 20, ty - 10, x + w - 20, ty - 10, GRAY);
        drawText("ФИЗИЧЕСКАЯ ОЦЕНКА", left, ty, 14, YELLOW); ty += 23;
        drawText("N ≈ B * S / Phi0; Phi0 = 2.068e-15 Вб.", left, ty, 11, SKYBLUE);
        drawText("Число не вычисляется: модельное B не откалибровано в теслах, S не задана в м².", left, ty + 19, 10, LIGHTGRAY);
    }
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
