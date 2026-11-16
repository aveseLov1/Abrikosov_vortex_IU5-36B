
#pragma once

#include "Vortex.h"
#include "Settings.h"

#include <vector>
#include <random>

class Simulation {
private:
    std::vector<Vortex> vortices;

    std::mt19937 generator;
    std::normal_distribution<float> noise;
    float vortexSpawnAccumulator = 0.0f;

    int calculateFieldDrivenVortexCount(const Settings& settings) const;
    void addVortex(const Settings& settings);

    static constexpr int NODES = 24;

    void calculateForces(
        std::vector<std::vector<Vector3>>& forces,
        const Settings& settings
    );

    void applyMovement(
        const std::vector<std::vector<Vector3>>& forces,
        const Settings& settings
    );

public:
    Simulation();

    void initialize(const Settings& settings);

    void update(const Settings& settings);

    const std::vector<Vortex>& getVortices() const;
    float getAverageField(const Settings& settings) const;
    int getTargetVortexCount(const Settings& settings) const;
};
