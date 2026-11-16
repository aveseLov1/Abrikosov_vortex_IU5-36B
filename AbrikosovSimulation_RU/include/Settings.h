
#pragma once

#include "raylib.h"

struct Settings {
    int vortexCount = 16;

    int materialIndex = 3; // 0: NbTi, 1: Nb3Sn, 2: MgB2, 3: YBCO
    float temperature = 4.2f; // температура в Кельвинах
    float criticalTemperature = 92.0f; // критическая температура выбранного материала, К

    float penetrationDepth = 3.0f;
    float interactionStrength = 2.0f;
    float pinningStrength = 0.5f;
    float viscosity = 1.0f;

    float elasticity = 0.8f;
    float thermalStrength = 0.025f;

    float timeStep = 0.015f;

    float materialSize = 10.0f;

    // Прямоугольный магнит: можно располагать в любой точке рабочей области
    bool magnetEnabled = false;
    Vector3 magnetPosition = {0.0f, 0.0f, 22.0f};
    Vector3 magnetRotation = {0.0f, 0.0f, 0.0f}; // градусы X/Y/Z
    float magnetStrength = 18.0f;
};
