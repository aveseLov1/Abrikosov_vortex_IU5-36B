#include "Magnet.h"
#include "raymath.h"

#include <cmath>
#include <algorithm>

namespace {
    constexpr float DEG_TO_RAD = 0.017453292519943295769f;
    constexpr float SOFTENING = 0.65f;

    Vector3 rotateDirection(Vector3 v, Vector3 angles) {
        const float x = angles.x * DEG_TO_RAD;
        const float y = angles.y * DEG_TO_RAD;
        const float z = angles.z * DEG_TO_RAD;

        // Повороты вокруг X, затем Y, затем Z.
        Vector3 rx = {
            v.x,
            v.y * std::cos(x) - v.z * std::sin(x),
            v.y * std::sin(x) + v.z * std::cos(x)
        };
        Vector3 ry = {
            rx.x * std::cos(y) + rx.z * std::sin(y),
            rx.y,
            -rx.x * std::sin(y) + rx.z * std::cos(y)
        };
        return {
            ry.x * std::cos(z) - ry.y * std::sin(z),
            ry.x * std::sin(z) + ry.y * std::cos(z),
            ry.z
        };
    }

    float lengthSquared(Vector3 v) {
        return v.x*v.x + v.y*v.y + v.z*v.z;
    }
}

Vector3 Magnet::getMomentDirection(Vector3 rotationDegrees) {
    return Vector3Normalize(rotateDirection({0.0f, 0.0f, -1.0f}, rotationDegrees));
}

Vector3 Magnet::magneticField(Vector3 point, Vector3 position,
                              Vector3 rotationDegrees, float strength) {
    Vector3 r = Vector3Subtract(point, position);
    float r2 = lengthSquared(r) + SOFTENING * SOFTENING;
    float r1 = std::sqrt(r2);
    float r3 = r2 * r1;
    float r5 = r3 * r2;
    Vector3 m = getMomentDirection(rotationDegrees);
    float mr = Vector3DotProduct(m, r);

    Vector3 field = Vector3Subtract(
        Vector3Scale(r, 3.0f * mr / r5),
        Vector3Scale(m, 1.0f / r3)
    );
    return Vector3Scale(field, strength);
}

float Magnet::fieldZ(Vector3 point, Vector3 position,
                     Vector3 rotationDegrees, float strength) {
    return magneticField(point, position, rotationDegrees, strength).z;
}

Vector3 Magnet::forceOnVortex(Vector3 point, Vector3 position,
                              Vector3 rotationDegrees, float strength) {
    // Вихрь направлен преимущественно вдоль Z. Энергия взаимодействия
    // пропорциональна -Bz; сила в плоскости XY пропорциональна grad(Bz).
    constexpr float h = 0.12f;
    Vector3 dx = {h, 0.0f, 0.0f};
    Vector3 dy = {0.0f, h, 0.0f};

    float gx = (fieldZ(Vector3Add(point, dx), position, rotationDegrees, strength) -
                fieldZ(Vector3Subtract(point, dx), position, rotationDegrees, strength)) / (2.0f*h);
    float gy = (fieldZ(Vector3Add(point, dy), position, rotationDegrees, strength) -
                fieldZ(Vector3Subtract(point, dy), position, rotationDegrees, strength)) / (2.0f*h);

    // Коэффициент выбран для устойчивой и наглядной демонстрации в единицах сцены.
    return {gx * 2.2f, gy * 2.2f, 0.0f};
}
