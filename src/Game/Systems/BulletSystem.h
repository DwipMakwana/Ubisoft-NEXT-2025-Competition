//-----------------------------------------------------------------------------
// BulletSystem.h - Player bullet shooting system (FIXED)
//-----------------------------------------------------------------------------
#ifndef BULLETSYSTEM_H
#define BULLETSYSTEM_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Components/Planet.h"

class AIShipSystem;
class AIPlayerSystem;
class AsteroidSystem;
class Player;

struct Bullet
{
    bool active = false;
    Vec3 position;
    Vec3 velocity;
    float lifetime = 0.0f;
    float maxLifetime = 2.0f;  // 2 seconds before despawn
    float radius = 0.3f;
    int killerHomePlanetIndex = 0;
};

class BulletSystem
{
public:
    BulletSystem();

    void Init();
    void Update(float deltaTime, AIShipSystem* aiShips, AIPlayerSystem* aiPlayers,
        AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem, Player* humanPlayer);
    void Render(const Camera3D& camera);

    // Shooting (no cooldown - caller manages their own cooldown)
    void ShootBullet(const Vec3& shooterPos, const Vec3& direction, int shooterHomePlanet);

private:
    static const int MAX_BULLETS = 2000;
    int nextBulletSlot = 0;
    Bullet bullets[MAX_BULLETS];

    // Helpers
    int FindFreeBullet();
    void CheckCollisions(AIShipSystem* aiShips, AIPlayerSystem* aiPlayers,
        AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem, Player* humanPlayer);
};

#endif