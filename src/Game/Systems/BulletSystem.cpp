//-----------------------------------------------------------------------------
// BulletSystem.cpp - Player bullet shooting system (FIXED: Per-shooter cooldown)
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

void BulletSystem::Init() {
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    nextBulletSlot = 0;  // Reset
    Logger::LogInfo("BulletSystem Initialized - Round-robin slots");
}


int BulletSystem::FindFreeBullet() {
    // ROUND-ROBIN: Start from last used slot
    for (int i = 0; i < MAX_BULLETS; i++) {
        int slot = (nextBulletSlot + i) % MAX_BULLETS;
        if (!bullets[slot].active) {
            nextBulletSlot = (slot + 1) % MAX_BULLETS;  // Next starts after
            return slot;
        }
    }
    return -1;
}

// CHANGED: Removed global cooldown check - each shooter manages their own cooldown
void BulletSystem::ShootBullet(const Vec3& shooterPos, const Vec3& direction, int shooterHomePlanet)
{
    int slot = FindFreeBullet();
    if (slot < 0) return;

    // Direction already normalized by caller
    bullets[slot].active = true;
    bullets[slot].position = shooterPos;
    bullets[slot].velocity = direction * 100.0f;  // Fast laser
    bullets[slot].killerHomePlanetIndex = shooterHomePlanet;
    bullets[slot].lifetime = 0.0f;
    bullets[slot].radius = 1.0f;  // Hit detection (rectangle bounding)
}

void BulletSystem::Update(float deltaTime, AIShipSystem* aiShips, AIPlayerSystem* aiPlayers,
    AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem, Player* humanPlayer)
{
    float dt = deltaTime / 1000.0f;

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
    CheckCollisions(aiShips, aiPlayers, asteroidSystem, planetSystem, humanPlayer);

}

void BulletSystem::CheckCollisions(AIShipSystem* aiShips, AIPlayerSystem* aiPlayers,
    AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem, Player* humanPlayer) {
    if (!aiShips || !aiPlayers || !asteroidSystem) return;

    AIShip* ships = aiShips->GetShips();
    int maxShips = aiShips->GetMaxShips();

    AIPlayer* players = aiPlayers->GetPlayers();
    int maxPlayers = aiPlayers->GetMaxPlayers();

    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!bullets[b].active) continue;

        // 1. Ship collisions (FIXED: Don't hit own team!)
        for (int s = 0; s < maxShips; s++) {
            if (!ships[s].active) continue;

            // CRITICAL: Skip if bullet is from same planet as ship
            if (bullets[b].killerHomePlanetIndex == ships[s].parentPlanetIndex) {
                continue;  // Friendly fire OFF
            }

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

        // 2. AI Guard collisions (FIXED: Don't hit own team!)
        for (int p = 0; p < maxPlayers; p++) {
            if (!players[p].active) continue;

            // CRITICAL: Skip if bullet is from same planet as guard
            if (bullets[b].killerHomePlanetIndex == players[p].homePlanetIndex) {
                continue;  // Friendly fire OFF
            }

            float dx = bullets[b].position.x - players[p].position.x;
            float dy = bullets[b].position.y - players[p].position.y;
            float dz = bullets[b].position.z - players[p].position.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float hitDist = bullets[b].radius + 1.5f;  // Player collision radius
            if (distSq < hitDist * hitDist) {
                aiPlayers->OnPlayerHit(p, asteroidSystem, planetSystem, bullets[b].killerHomePlanetIndex);
                bullets[b].active = false;
                break;
            }
        }

        if (!bullets[b].active) continue;

        // 3. HUMAN PLAYER collisions (NEW!)
        if (humanPlayer) {
            // Skip if bullet is from player's own planet (0)
            if (bullets[b].killerHomePlanetIndex == 0) {
                continue;  // Player can't shoot themselves
            }

            Vec3 playerPos = humanPlayer->GetPosition();
            float dx = bullets[b].position.x - playerPos.x;
            float dy = bullets[b].position.y - playerPos.y;
            float dz = bullets[b].position.z - playerPos.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float hitDist = bullets[b].radius + 1.2f;  // Human player collision radius

            if (distSq < hitDist * hitDist) {
                humanPlayer->TakeDamage(25.0f);

                // For now, just destroy the bullet
                bullets[b].active = false;

                // Optional: Add knockback
                Vec3 knockback(dx, dy, dz);
                float len = sqrtf(distSq);
                if (len > 0.001f) {
                    knockback = knockback * (5.0f / len);  // Knockback strength
                    humanPlayer->ApplyKnockback(knockback);
                }
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