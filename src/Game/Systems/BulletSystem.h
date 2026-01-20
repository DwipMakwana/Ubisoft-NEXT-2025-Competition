//-----------------------------------------------------------------------------
// BulletSystem.h - Player bullet shooting system
//-----------------------------------------------------------------------------
#ifndef BULLETSYSTEM_H
#define BULLETSYSTEM_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Components/Planet.h"

class AIShipSystem;
class AIPlayerSystem;
class AsteroidSystem;

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
        AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem);
    void Render(const Camera3D& camera);

    // Shooting
    void ShootBullet(const Vec3& playerPos, const Vec3& direction, int playerHomePlanet);

private:
    static const int MAX_BULLETS = 1000;
    Bullet bullets[MAX_BULLETS];

    // Helpers
    int FindFreeBullet();
    void CheckCollisions(AIShipSystem* aiShips, AIPlayerSystem* aiPlayers,
        AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem);

    float fireRateCooldown = 0.0f;
    float fireRateInterval = 0.15f;
};

#endif
