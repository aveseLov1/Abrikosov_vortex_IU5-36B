#pragma once

#include "raylib.h"

struct Magnet {
    static Vector3 getMomentDirection(Vector3 rotationDegrees);
    static Vector3 magneticField(Vector3 point, Vector3 position,
                                 Vector3 rotationDegrees, float strength);
    static float fieldZ(Vector3 point, Vector3 position,
                        Vector3 rotationDegrees, float strength);
    static Vector3 forceOnVortex(Vector3 point, Vector3 position,
                                 Vector3 rotationDegrees, float strength);
};
