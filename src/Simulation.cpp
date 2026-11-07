
#include "Simulation.h"
#include "Magnet.h"

#include <cmath>
#include <algorithm>

Simulation::Simulation()
    : generator(std::random_device{}()),
      noise(0.0f, 1.0f) {
}

void Simulation::initialize(const Settings& settings) {
    vortices.clear();

    int count = std::clamp(settings.vortexCount, 4, 36);

    int cols = static_cast<int>(
        std::ceil(std::sqrt(static_cast<float>(count)))
    );

    int rows = (count + cols - 1) / cols;

    float spacing = 2.8f;

    for (int i = 0; i < count; i++) {
        Vortex vortex;

        int col = i % cols;
        int row = i / cols;

        float cx = (col - (cols - 1) / 2.0f) * spacing;
        float cy = (row - (rows - 1) / 2.0f) * spacing;

        // Смещение каждого второго ряда
        if (row % 2 != 0) {
            cx += spacing * 0.5f;
        }

        for (int k = 0; k < NODES; k++) {
            float z = -6.0f +
                12.0f * k / (NODES - 1);

            float offset = 0.12f * noise(generator);

            vortex.points.push_back({
                cx + offset,
                cy + 0.12f * noise(generator),
                z
            });
        }

        vortices.push_back(vortex);
    }
}

void Simulation::calculateForces(
    std::vector<std::vector<Vector3>>& forces,
    const Settings& settings
) {
    int count = static_cast<int>(vortices.size());

    forces.assign(
        count,
        std::vector<Vector3>(
            NODES,
            {0.0f, 0.0f, 0.0f}
        )
    );

    // Взаимное отталкивание вихрей
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {

            for (int k = 0; k < NODES; k++) {

                Vector3 a = vortices[i].points[k];
                Vector3 b = vortices[j].points[k];

                float dx = a.x - b.x;
                float dy = a.y - b.y;

                float r2 = dx * dx + dy * dy + 0.01f;
                float r = std::sqrt(r2);

                float lambda = std::max(
                    settings.penetrationDepth,
                    0.1f
                );

                float force =
                    settings.interactionStrength *
                    std::exp(-r / lambda) / r;

                float fx = force * dx / r;
                float fy = force * dy / r;

                forces[i][k].x += fx;
                forces[i][k].y += fy;

                forces[j][k].x -= fx;
                forces[j][k].y -= fy;
            }
        }
    }

    // Упругость вихревых линий
    for (int i = 0; i < count; i++) {
        for (int k = 1; k < NODES - 1; k++) {

            Vector3 prev = vortices[i].points[k - 1];
            Vector3 next = vortices[i].points[k + 1];
            Vector3 current = vortices[i].points[k];

            float e = settings.elasticity;

            forces[i][k].x +=
                e * (prev.x + next.x - 2.0f * current.x);

            forces[i][k].y +=
                e * (prev.y + next.y - 2.0f * current.y);
        }
    }

    // Внешнее поле прямоугольного магнита.
    if (settings.magnetEnabled && settings.magnetStrength > 0.0f) {
        for (int i = 0; i < count; i++) {
            for (int k = 0; k < NODES; k++) {
                const Vector3 p = vortices[i].points[k];
                Vector3 magneticForce = Magnet::forceOnVortex(
                    p,
                    settings.magnetPosition,
                    settings.magnetRotation,
                    settings.magnetStrength
                );

                forces[i][k].x += magneticForce.x;
                forces[i][k].y += magneticForce.y;
            }
        }
    }

    // Пиннинг и границы образца
    for (int i = 0; i < count; i++) {
        for (int k = 0; k < NODES; k++) {

            Vector3 p = vortices[i].points[k];

            // Периодические центры закрепления
            const float pinSpacing = 2.5f;

            float targetX =
                std::round(p.x / pinSpacing) * pinSpacing;

            float targetY =
                std::round(p.y / pinSpacing) * pinSpacing;

            forces[i][k].x +=
                settings.pinningStrength * (targetX - p.x);

            forces[i][k].y +=
                settings.pinningStrength * (targetY - p.y);

            float limit = settings.materialSize;

            if (p.x > limit)
                forces[i][k].x -= (p.x - limit) * 5.0f;

            if (p.x < -limit)
                forces[i][k].x -= (p.x + limit) * 5.0f;

            if (p.y > limit)
                forces[i][k].y -= (p.y - limit) * 5.0f;

            if (p.y < -limit)
                forces[i][k].y -= (p.y + limit) * 5.0f;
        }
    }
}

void Simulation::applyMovement(
    const std::vector<std::vector<Vector3>>& forces,
    const Settings& settings
) {
    float viscosity = std::max(settings.viscosity, 0.1f);

    float temperatureRatio = std::clamp(
        settings.temperature / settings.criticalTemperature,
        0.0f,
        0.999f
    );

    float thermal = settings.thermalStrength *
        std::sqrt(
            temperatureRatio /
            (1.0f - temperatureRatio)
        );

    float dt = settings.timeStep;

    for (int i = 0; i < static_cast<int>(vortices.size()); i++) {
        for (int k = 0; k < NODES; k++) {

            Vector3& p = vortices[i].points[k];

            float fx = forces[i][k].x / viscosity;
            float fy = forces[i][k].y / viscosity;

            fx = std::clamp(fx, -10.0f, 10.0f);
            fy = std::clamp(fy, -10.0f, 10.0f);

            float randomX = thermal * noise(generator);
            float randomY = thermal * noise(generator);

            p.x += fx * dt + randomX * std::sqrt(dt);
            p.y += fy * dt + randomY * std::sqrt(dt);
        }
    }
}

void Simulation::update(const Settings& settings) {
    std::vector<std::vector<Vector3>> forces;

    calculateForces(forces, settings);

    applyMovement(forces, settings);
}

const std::vector<Vortex>& Simulation::getVortices() const {
    return vortices;
}
