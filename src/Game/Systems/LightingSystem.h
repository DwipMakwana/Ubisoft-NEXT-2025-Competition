//-----------------------------------------------------------------------------
// LightingSystem.h
// Simple single directional light
//-----------------------------------------------------------------------------
#ifndef LIGHTING_SYSTEM_H
#define LIGHTING_SYSTEM_H

#include "../Utilities/MathUtils3D.h"

class LightingSystem {
public:
    static void Init();

    // Calculate final color with lighting
    static Vec3 CalculateLighting(
        const Vec3& worldPos,
        const Vec3& normal,
        const Vec3& baseColor
    );

    // Simple settings
    static Vec3 lightDirection;
    static Vec3 lightColor;
    static float lightIntensity;
    static Vec3 ambientColor;
};

#endif
