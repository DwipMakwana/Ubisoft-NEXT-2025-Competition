//-----------------------------------------------------------------------------
// TowingSystem.h - Asteroid grab and deposit mechanics
//-----------------------------------------------------------------------------
#ifndef TOWING_SYSTEM_H
#define TOWING_SYSTEM_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Components/Asteroid.h"
#include "../Components/Planet.h"
#include "../Systems/RopeSystem.h"

class TowingSystem {
public:
    // Towing state
    bool isTowing;
    int towedAsteroidIndex;

    // Selection highlighting
    int highlightedAsteroidIndex;
    int highlightedPlanetIndex;

    // Rope simulation
    RopeSystem rope;

    // Settings
    float grabRange;          // How close to grab/deposit (units)
    float ropeSlack;          // Extra rope length for physics sag
    int ropeSegments;         // More = smoother but slower

    // Visual feedback
    float highlightPulse;     // Pulsing highlight effect

    TowingSystem();

    // Update system
    void Update(float deltaTime, const Vec3& playerPos, const Vec3& playerVel,
        AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem);

    // Render highlighting and rope
    void Render(const Camera3D& camera, AsteroidSystem* asteroidSystem,
        PlanetSystem* planetSystem);

    // Input handling
    void HandleInput(const Vec3& playerPos, AsteroidSystem* asteroidSystem,
        PlanetSystem* planetSystem);

    // Query functions
    bool IsTowing() const { return isTowing; }
    int GetTowedAsteroidIndex() const { return towedAsteroidIndex; }

    // AI helpers (non-input)
    bool TryGrabAsteroid(int asteroidIndex, const Vec3& shipPos, AsteroidSystem* asteroidSystem);
    bool TryDepositAtPlanet(int planetIndex, AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem);

    void GrabAsteroid(int asteroidIndex, const Vec3& playerPos, AsteroidSystem* asteroidSystem);
    void DepositAsteroid(int planetIndex, AsteroidSystem* asteroidSystem,PlanetSystem* planetSystem);

    void ReleaseAsteroid();

private:
    // Find nearest interactable object
    int FindNearestAsteroid(const Vec3& playerPos, AsteroidSystem* asteroidSystem);
    int FindNearestPlanet(const Vec3& playerPos, PlanetSystem* planetSystem);

    // Actions

    // Rendering helpers
    void RenderHighlight(const Vec3& position, float size, const Camera3D& camera,
        float r, float g, float b);
    void RenderInteractionPrompt(const Vec3& worldPos, const char* text,
        const Camera3D& camera);
};

#endif // TOWING_SYSTEM_H
