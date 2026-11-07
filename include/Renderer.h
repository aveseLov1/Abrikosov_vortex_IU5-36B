
#pragma once

#include "Simulation.h"
#include "Settings.h"

class Renderer {
private:
    float cameraYaw = 0.8f;
    float cameraPitch = 0.35f;
    float cameraDistance = 30.0f;

    int activeSlider = -1;
    float countValue = 16.0f;
    bool magnetTab = false;
    bool draggingMagnet = false;

    bool drawSlider(
        int id,
        int x,
        int y,
        int width,
        const char* name,
        float& value,
        float minValue,
        float maxValue
    );

    bool drawButton(
        int x,
        int y,
        int width,
        int height,
        const char* name
    );

    void drawWorld(const Simulation& simulation, const Settings& settings);

    void drawInterface(
        Simulation& simulation,
        Settings& settings,
        bool& paused
    );

public:
    void draw(
        Simulation& simulation,
        Settings& settings,
        bool& paused
    );
};
