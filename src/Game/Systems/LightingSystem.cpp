//-----------------------------------------------------------------------------
// LightingSystem.cpp
// Simple directional lighting
//-----------------------------------------------------------------------------
#include "LightingSystem.h"
#include <algorithm>
#include <cmath>

// Static member initialization
Vec3 LightingSystem::lightDirection(0.3f, -1.0f, 0.5f);
Vec3 LightingSystem::lightColor(1.0f, 0.98f, 0.95f);
float LightingSystem::lightIntensity = 0.8f;
Vec3 LightingSystem::ambientColor(0.3f, 0.3f, 0.35f);

void LightingSystem::Init() {
    lightDirection = Vec3(-0.5f, 0.8f, -0.3f);
    lightDirection = lightDirection.Normalized();
    lightColor = Vec3(1.0f, 0.8f, 0.6f);
    lightIntensity = 0.4f;
    ambientColor = Vec3(0.3f, 0.3f, 0.35f);
}

Vec3 LightingSystem::CalculateLighting(
    const Vec3& worldPos,
    const Vec3& normal,
    const Vec3& baseColor
) {
    // Normalize inputs
    Vec3 N = normal.Normalized();
    Vec3 L = (lightDirection * -1.0f).Normalized();  // Light rays go opposite to direction

    // Diffuse lighting (Lambert)
    float diff = N.Dot(L);
    if (diff < 0.0f) diff = 0.0f;

    // Combine ambient + diffuse
    Vec3 ambient = ambientColor;
    Vec3 diffuse = lightColor * (diff * lightIntensity);

    Vec3 finalColor = ambient + diffuse;

    // Multiply by base color
    finalColor.x *= baseColor.x;
    finalColor.y *= baseColor.y;
    finalColor.z *= baseColor.z;

    // Clamp
    if (finalColor.x > 1.0f) finalColor.x = 1.0f;
    if (finalColor.y > 1.0f) finalColor.y = 1.0f;
    if (finalColor.z > 1.0f) finalColor.z = 1.0f;

    return finalColor;
}
