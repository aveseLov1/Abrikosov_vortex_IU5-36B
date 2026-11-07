
#pragma once

#include "raylib.h"

struct Settings {
    int vortexCount = 16;

    float temperature = 0.35f;
    float criticalTemperature = 1.0f;

    float penetrationDepth = 3.0f;
    float interactionStrength = 2.0f;
    float pinningStrength = 0.5f;
    float viscosity = 1.0f;

    float elasticity = 0.8f;
    float thermalStrength = 0.025f;

    float timeStep = 0.015f;

    float materialSize = 10.0f;

    // Внешний прямоугольный магнит (координаты в сцене)
    bool magnetEnabled = true;
    Vector3 magnetPosition = {0.0f, 0.0f, 16.0f};
    Vector3 magnetRotation = {0.0f, 0.0f, 0.0f}; // градусы X/Y/Z
    float magnetStrength = 18.0f;
};
