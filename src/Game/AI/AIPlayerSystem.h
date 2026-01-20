#ifndef AIPLAYERSYSTEM_H
#define AIPLAYERSYSTEM_H

#include "../Components/Player.h"
#include "../Components/Planet.h"
#include "AIShipSystem.h"
#include "../Rendering/Camera3D.h"
#include <vector>
#include "../Systems/BulletSystem.h"

struct AIPlayer {
    Vec3 position;
    Vec3 velocity;
    float aimAngle;
    Mesh3D mesh;
    float size = 0.8f;
    float maxSpeed = 120.0f;
    float drag = 0.995f;

    // AI state
    int homePlanetIndex = 0;
    int targetShipIndex = -1;
    int targetType = 0;  // NEW: 0=none, 1=ship, 2=guard, 3=player
    float retargetTimer = 0.0f;
    float shootCooldown = 0.0f;
    Vec3 patrolTarget;
    bool active = false;

    // Colors
    float r = 1.0f, g = 1.0f, b = 1.0f;

    float health = 100.0f;
    int consecutiveHits = 0;
    float hitComboTimer = 0.0f;

    ParticleSystem3D* thrusterParticles = nullptr;
};

// Forward declarations for targeting system
enum class TargetType {
    NONE,
    SHIP,
    GUARD,
    PLAYER
};

struct TargetInfo {
    TargetType type;
    int index;
    float distanceSq;
    Vec3 position;
    Vec3 velocity;
};

class AIPlayerSystem {
public:
    AIPlayerSystem();
    void Init(PlanetSystem& planetSystem);
    void SpawnForActivePlanets(PlanetSystem& planetSystem);
    void Update(float deltaTime, PlanetSystem& planetSystem, AIShipSystem& enemyShips,
        BulletSystem& bulletSystem, Player& humanPlayer);
    void Render(const Camera3D& camera);

    void OnPlayerHit(int playerIndex, AsteroidSystem* asteroidSystem,
        PlanetSystem* planetSystem, int killerHomePlanet);
    AIPlayer* GetPlayers() { return players; }
    int GetMaxPlayers() const { return MAX_AIPLAYERS; }

private:
    static const int MAX_AIPLAYERS = 8;
    AIPlayer players[MAX_AIPLAYERS];

    void ApplyAISteering(AIPlayer* player, const Vec3 targetPos, float dt,
        float maxSpeed, float accel, bool updatePosition = true);

    int FindFreePlayer();
    TargetInfo FindBestTarget(const Vec3& pos, int myHomePlanetIndex,
        AIShipSystem& enemyShips, Player& humanPlayer);
    void UpdateAI(int index, float dt, PlanetSystem planetSystem,
        AIShipSystem enemyShips, BulletSystem& bullets, Player& humanPlayer);
};

#endif