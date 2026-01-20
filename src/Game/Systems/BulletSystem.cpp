//-----------------------------------------------------------------------------
// BulletSystem.cpp - Player bullet shooting system
//-----------------------------------------------------------------------------
#include "BulletSystem.h"
#include "../Rendering/Renderer3D.h"
#include "../Utilities/Logger.h"
#include <cmath>
#include "../AI/AIShipSystem.h"
#include "../AI/AIPlayerSystem.h"

BulletSystem::BulletSystem()
{
    for (int i = 0; i < MAX_BULLETS; i++)
        bullets[i].active = false;
}

void BulletSystem::Init()
{
    //Logger::LogInfo("BulletSystem: Initialized");
}

int BulletSystem::FindFreeBullet()
{
    for (int i = 0; i < MAX_BULLETS; i++)
        if (!bullets[i].active) return i;
    return -1;
}

void BulletSystem::ShootBullet(const Vec3& playerPos, const Vec3& direction, int playerHomePlanet)
{
    // COOLDOWN CHECK (machine gun rate)
    if (fireRateCooldown > 0.0f) return;

    int slot = FindFreeBullet();
    if (slot < 0) return;

    // Direction already normalized by caller
    bullets[slot].active = true;
    bullets[slot].position = playerPos;
    bullets[slot].velocity = direction * 100.0f;  // Fast laser
    bullets[slot].killerHomePlanetIndex = playerHomePlanet;
    bullets[slot].lifetime = 0.0f;
    bullets[slot].radius = 1.0f;  // Hit detection (rectangle bounding)

    fireRateCooldown = fireRateInterval;  // Reset cooldown
}

void BulletSystem::Update(float deltaTime, AIShipSystem* aiShips, AIPlayerSystem* aiPlayers,
    AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem)
{
    float dt = deltaTime / 1000.0f;

    // Update fire rate cooldown
    if (fireRateCooldown > 0.0f)
        fireRateCooldown -= dt;

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets[i].active) continue;

        // Move bullet
        bullets[i].position = bullets[i].position + bullets[i].velocity * dt;
        bullets[i].lifetime += dt;

        // Despawn old bullets
        if (bullets[i].lifetime > bullets[i].maxLifetime)
        {
            bullets[i].active = false;
            continue;
        }
    }

    // Check collisions
    CheckCollisions(aiShips, aiPlayers, asteroidSystem, planetSystem);
}

void BulletSystem::CheckCollisions(AIShipSystem* aiShips, AIPlayerSystem* aiPlayers,
    AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem) {
    if (!aiShips || !aiPlayers || !asteroidSystem) return;

    AIShip* ships = aiShips->GetShips();
    int maxShips = aiShips->GetMaxShips();

    AIPlayer* players = aiPlayers->GetPlayers();
    int maxPlayers = aiPlayers->GetMaxPlayers();

    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!bullets[b].active) continue;

        // 1. Ship collisions (existing)
        for (int s = 0; s < maxShips; s++) {
            if (!ships[s].active) continue;
            float dx = bullets[b].position.x - ships[s].position.x;
            float dy = bullets[b].position.y - ships[s].position.y;
            float dz = bullets[b].position.z - ships[s].position.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float hitDist = bullets[b].radius + ships[s].radius;
            if (distSq < hitDist * hitDist) {
                aiShips->OnShipHit(s, *asteroidSystem);
                bullets[b].active = false;
                break;
            }
        }

        if (!bullets[b].active) continue;

        // 2. Player collisions (NEW - exact copy)
        for (int p = 0; p < maxPlayers; p++) {
            if (!players[p].active) continue;
            float dx = bullets[b].position.x - players[p].position.x;
            float dy = bullets[b].position.y - players[p].position.y;
            float dz = bullets[b].position.z - players[p].position.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float hitDist = bullets[b].radius + 1.5f;  // Player collision radius
            if (distSq < hitDist * hitDist) {
                //Logger::LogFormat("BULLET HIT GUARD %d!", p);  // DEBUG
                aiPlayers->OnPlayerHit(p, asteroidSystem, planetSystem, bullets[b].killerHomePlanetIndex);
                bullets[b].active = false;
                break;
            }
        }
    }
}

void BulletSystem::Render(const Camera3D& camera)
{
    static Mesh3D bulletMesh = Mesh3D::CreateCube(0.5f); // Slightly bigger

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets[i].active) continue;

        // Draw bullet using MESH3D (same as asteroids/planets)
        Vec3 bulletScale(1.0f, 1.0f, 1.0f);  // Use mesh natural size
        Vec3 bulletRot(0, 0, 0);  // No rotation needed

        Renderer3D::DrawMesh(bulletMesh, bullets[i].position, bulletRot, bulletScale,
            camera, 1.0f, 0.0f, 0.0f, false);  // Bright red
    }
}

