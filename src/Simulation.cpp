
#include "Simulation.h"
#include "Magnet.h"

#include <cmath>
#include <algorithm>
#include <utility>

Simulation::Simulation()
    : generator(std::random_device{}()),
      noise(0.0f, 1.0f) {
}

void Simulation::initialize(const Settings& settings) {
    // Вихри изначально отсутствуют: они будут появляться только тогда,
    // когда поле внешнего магнита в образце превысит условный Hc1.
    vortices.clear();
    vortexSpawnAccumulator = 0.0f;
}

int Simulation::calculateFieldDrivenVortexCount(const Settings& settings) const {
    if (!settings.magnetEnabled || settings.magnetStrength <= 0.0f) return 0;

    // Оцениваем среднее |Bz| в объёме сверхпроводника.
    float sum = 0.0f;
    int samples = 0;
    for (int ix = -2; ix <= 2; ++ix) {
        for (int iy = -2; iy <= 2; ++iy) {
            for (int iz = -2; iz <= 2; ++iz) {
                Vector3 q = {ix * 4.0f, iy * 4.0f, iz * 4.0f};
                sum += std::fabs(Magnet::fieldZ(
                    q, settings.magnetPosition, settings.magnetRotation,
                    settings.magnetStrength));
                ++samples;
            }
        }
    }
    const float averageField = sum / static_cast<float>(samples);

    // Условный порог проникновения и масштаб насыщения для единиц сцены.
    // Это качественная демонстрационная аппроксимация Hc1, не расчёт в теслах.
    constexpr float Hc1 = 0.0015f;
    constexpr float Hfull = 0.018f;
    if (averageField <= Hc1) return 0;

    float fraction = std::clamp((averageField - Hc1) / (Hfull - Hc1), 0.0f, 1.0f);
    int maximum = std::clamp(settings.vortexCount, 4, 36);
    return std::clamp(static_cast<int>(std::ceil(fraction * maximum)), 0, maximum);
}

void Simulation::addVortex(const Settings& settings) {
    Vortex vortex;

    // Ищем область с наибольшим |Bz| на верхней поверхности образца:
    // новые вихри входят там, где поле сильнее.
    Vector3 best = {0.0f, 0.0f, 0.0f};
    float bestField = -1.0f;
    for (int ix = -5; ix <= 5; ++ix) {
        for (int iy = -5; iy <= 5; ++iy) {
            Vector3 q = {ix * 1.8f, iy * 1.8f, 6.0f};
            float b = std::fabs(Magnet::fieldZ(q, settings.magnetPosition,
                settings.magnetRotation, settings.magnetStrength));
            if (b > bestField) { bestField = b; best = q; }
        }
    }

    // Небольшое распределение точек вокруг области входа потока.
    best.x += 0.35f * noise(generator);
    best.y += 0.35f * noise(generator);
    for (int k = 0; k < NODES; ++k) {
        float z = -6.0f + 12.0f * k / (NODES - 1);
        vortex.points.push_back({best.x + 0.08f * noise(generator),
                                 best.y + 0.08f * noise(generator), z});
    }
    vortices.push_back(std::move(vortex));
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

    // Точки независимы: взаимное отталкивание и упругая связь между
    // соседними узлами вихревой линии отключены намеренно.

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
    const int targetCount = calculateFieldDrivenVortexCount(settings);

    // Появление и исчезновение вихрей происходит постепенно, а не мгновенно.
    vortexSpawnAccumulator += settings.timeStep;
    if (vortexSpawnAccumulator >= 0.12f) {
        vortexSpawnAccumulator = 0.0f;
        if (static_cast<int>(vortices.size()) < targetCount) {
            addVortex(settings);
        } else if (static_cast<int>(vortices.size()) > targetCount) {
            vortices.pop_back();
        }
    }

    std::vector<std::vector<Vector3>> forces;

    calculateForces(forces, settings);

    applyMovement(forces, settings);
}

const std::vector<Vortex>& Simulation::getVortices() const {
    return vortices;
}
