//-----------------------------------------------------------------------------
// AIShipSystem.h - Planet-based AI towing ships
//-----------------------------------------------------------------------------
#ifndef AISHIPSYSTEM_H
#define AISHIPSYSTEM_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Components/Planet.h"
#include "../Components/Asteroid.h"
#include "../Systems/TowingSystem.h"
#include <vector>

struct AIShip
{
    bool active = false;

    // Parent planet this ship belongs to
    int parentPlanetIndex = -1;

    // Ship motion
    Vec3 position = Vec3(0, 0, 0);
    Vec3 velocity = Vec3(0, 0, 0);

    // Visuals
    float radius = 1.2f;          // Match player collision radius used in GameTest [file:9]
    float r = 0.9f, g = 0.9f, b = 0.9f;

    // Decision state
    int targetAsteroidIndex = -1;
    AsteroidMineralType desiredMineral = MINERAL_IRON;

    // Towing behavior (same system as player)
    TowingSystem towing;

    // Timers to avoid heavy scanning every frame
    float retargetTimer = 0.0f;

    int consecutiveHits = 0;
    float hitComboTimer = 0.0f;
};

class AIShipSystem
{
public:
    AIShipSystem();

    void Init();
    void Update(float deltaTime, PlanetSystem& planetSystem, AsteroidSystem& asteroidSystem);
    void Render(const Camera3D& camera, AsteroidSystem& asteroidSystem, PlanetSystem& planetSystem);

    int GetMaxShips() const { return MAX_AI_SHIPS; }
    AIShip* GetShips() { return ships; }
    const AIShip* GetShips() const { return ships; }

    void OnShipHit(int shipIndex);

    // Add to public section
    void ResolveCollisionsWithAsteroids(AsteroidSystem& asteroidSystem);

private:
    static const int MAX_AI_SHIPS = 84;

    AIShip ships[MAX_AI_SHIPS];

    // Helpers
    void SpawnForActivePlanets(PlanetSystem& planetSystem);
    int FindShipForPlanet(int planetIndex) const;
    void SpawnShip(int slot, int planetIndex, const PlanetSystem& planetSystem);

    AsteroidMineralType ChooseDesiredMineral(const Planet& planet) const;
    int FindBestAsteroidTarget(const AIShip& ship, const Planet& parent,
        AsteroidSystem& asteroidSystem) const;

    bool IsPlanetFrozen(int planetIndex) const;

    void UpdateAI(int shipIndex, AIShip& ship, float dt, PlanetSystem& planetSystem, AsteroidSystem& asteroidSystem);
    void ApplySteering(AIShip& ship, const Vec3& targetPos, float dt, float maxSpeed, float accel);

    void FreezePlanetFleet(int planetIndex);

    bool IsNear(const Vec3& a, const Vec3& b, float dist) const;

    struct PlanetFreezeState {
        int planetIndex = -1;
        float freezeTimer = 0.0f;
    };
    PlanetFreezeState frozenPlanets[20];  // Max planets

};

#endif
