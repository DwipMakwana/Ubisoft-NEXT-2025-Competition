//------------------------------------------------------------------------
// Asteroid.cpp - AI-driven asteroid types
//------------------------------------------------------------------------
#include "Asteroid.h"
#include "../Utilities/Logger.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include "Planet.h"

AsteroidSystem::AsteroidSystem()
    : spawnDistance(100.0f)
    , cullDistance(150.0f)
    , minSize(0.5f)
    , maxSize(2.0f)
    , minSpeed(0.5f)
    , maxSpeed(2.0f)
    , restitution(0.7f)
    , fragmentMinSize(0.6f)
    , fragmentCount(4)
    , fragmentSpeed(8.0f)
    , cellSize(10.0f)
    , lastPlayerPos(0, 0, 0)
{
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].active = false;
        asteroids[i].isFragment = false;
        asteroids[i].isPersistent = false;
        asteroids[i].type = WANDERER;
        asteroids[i].orbitTargetIndex = -1;
    }
}

void AsteroidSystem::Init() {
    asteroidMesh = Mesh3D::CreateSphere(1.0f, 5);

    Logger::LogInfo("AsteroidSystem: AI-driven types initialized");

    // Load initial sectors
    LoadSectorsAroundPlayer(Vec3(0, 0, 0));
    lastPlayerPos = Vec3(0, 0, 0);

    Logger::LogFormat("Initial asteroids: %d", GetActiveAsteroidCount());
}

void AsteroidSystem::GetSectorCoords(const Vec3& pos, int& sectorX, int& sectorY) {
    sectorX = (int)floorf(pos.x / SECTOR_SIZE);
    sectorY = (int)floorf(pos.y / SECTOR_SIZE);
}

void AsteroidSystem::LoadSectorsAroundPlayer(const Vec3& playerPos) {
    int playerSectorX, playerSectorY;
    GetSectorCoords(playerPos, playerSectorX, playerSectorY);

    // Load 3x3 grid of sectors around player
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int sectorX = playerSectorX + dx;
            int sectorY = playerSectorY + dy;

            SectorKey key = { sectorX, sectorY };

            // Check if already loaded
            if (loadedSectors.find(key) == loadedSectors.end()) {
                GenerateSectorAsteroids(sectorX, sectorY);
                loadedSectors[key] = true;

                Logger::LogFormat("Loaded sector (%d, %d)", sectorX, sectorY);
            }
        }
    }
}

void AsteroidSystem::GenerateSectorAsteroids(int sectorX, int sectorY) {
    unsigned int seed = (unsigned int)(sectorX * 73856093) ^ (unsigned int)(sectorY * 19349663);
    unsigned int oldSeed = rand();
    srand(seed);

    float centerX = (sectorX + 0.5f) * SECTOR_SIZE;
    float centerY = (sectorY + 0.5f) * SECTOR_SIZE;

    for (int i = 0; i < ASTEROIDS_PER_SECTOR; i++) {
        int slot = -1;
        for (int j = 0; j < MAX_ASTEROIDS; j++) {
            if (!asteroids[j].active) {
                slot = j;
                break;
            }
        }

        if (slot == -1) break;

        // Position
        float offsetX = ((rand() % 1000) / 1000.0f - 0.5f) * SECTOR_SIZE * 0.8f;
        float offsetY = ((rand() % 1000) / 1000.0f - 0.5f) * SECTOR_SIZE * 0.8f;

        asteroids[slot].position.x = centerX + offsetX;
        asteroids[slot].position.y = centerY + offsetY;
        asteroids[slot].position.z = 0.0f;

        // Initial velocity
        asteroids[slot].velocity.x = ((rand() % 1000) / 1000.0f - 0.5f) * maxSpeed * 0.5f;
        asteroids[slot].velocity.y = ((rand() % 1000) / 1000.0f - 0.5f) * maxSpeed * 0.5f;
        asteroids[slot].velocity.z = 0.0f;

        // Size and mass
        asteroids[slot].size = minSize + (rand() % 1000) / 1000.0f * (maxSize - minSize);
        asteroids[slot].mass = asteroids[slot].size * asteroids[slot].size * asteroids[slot].size * 50.0f;

        // Rotation
        asteroids[slot].rotationSpeed = -50.0f + (rand() % 1000) / 1000.0f * 100.0f;
        asteroids[slot].currentRotation = (rand() % 360);

        // Assign type with colors
        float typeRoll = (rand() % 1000) / 1000.0f;
        if (typeRoll < 0.50f) {
            // WANDERER - GRAY/WHITE
            asteroids[slot].type = WANDERER;
            float brightness = 0.5f + (rand() % 1000) / 1000.0f * 0.4f;
            asteroids[slot].r = brightness;
            asteroids[slot].g = brightness;
            asteroids[slot].b = brightness;

            // NEW: Initialize patrol behavior
            float targetOffsetX = ((rand() % 1000) / 1000.0f - 0.5f) * SECTOR_SIZE * 0.7f;
            float targetOffsetY = ((rand() % 1000) / 1000.0f - 0.5f) * SECTOR_SIZE * 0.7f;
            asteroids[slot].patrolTarget.x = centerX + targetOffsetX;
            asteroids[slot].patrolTarget.y = centerY + targetOffsetY;
            asteroids[slot].patrolTarget.z = 0.0f;

            asteroids[slot].patrolWaitTime = 2.0f + (rand() % 1000) / 1000.0f * 6.0f;
            asteroids[slot].patrolWaitTimer = 0.0f;
            asteroids[slot].patrolWaiting = false;

        }
        else if (typeRoll < 0.80f) {
            // ORBITER - BRIGHT BLUE
            asteroids[slot].type = ORBITER;
            asteroids[slot].r = 0.2f + (rand() % 1000) / 1000.0f * 0.2f;
            asteroids[slot].g = 0.4f + (rand() % 1000) / 1000.0f * 0.3f;
            asteroids[slot].b = 0.7f + (rand() % 1000) / 1000.0f * 0.3f;

            asteroids[slot].orbitTargetIndex = -1;
            asteroids[slot].orbitingPlanet = false;
            asteroids[slot].orbitAngle = (rand() % 1000) / 1000.0f * 6.28318f;
            asteroids[slot].orbitRadius = 5.0f + (rand() % 1000) / 1000.0f * 5.0f;
            asteroids[slot].orbitSpeed = 0.05f + (rand() % 1000) / 1000.0f * 0.20f;
            asteroids[slot].orbitClockwise = (rand() % 2) == 0;

        }
        else {
            // FLOCKING - BRIGHT GREEN
            asteroids[slot].type = FLOCKING;
            asteroids[slot].r = 0.2f + (rand() % 1000) / 1000.0f * 0.2f;
            asteroids[slot].g = 0.7f + (rand() % 1000) / 1000.0f * 0.3f;
            asteroids[slot].b = 0.2f + (rand() % 1000) / 1000.0f * 0.2f;

            asteroids[slot].flockingForce = Vec3(0, 0, 0);
        }

        // Common properties
        asteroids[slot].isFragment = false;
        asteroids[slot].isPersistent = true;
        asteroids[slot].sectorX = sectorX;
        asteroids[slot].sectorY = sectorY;
        asteroids[slot].lifetime = 0.0f;
        asteroids[slot].maxLifetime = 0.0f;
        asteroids[slot].alpha = 1.0f;
        asteroids[slot].active = true;
    }

    srand(oldSeed);
}

void AsteroidSystem::UnloadDistantSectors(const Vec3& playerPos) {
    int playerSectorX, playerSectorY;
    GetSectorCoords(playerPos, playerSectorX, playerSectorY);

    // Unload sectors that are too far (more than 2 sectors away)
    std::vector<SectorKey> toUnload;

    for (auto& pair : loadedSectors) {
        int dx = pair.first.x - playerSectorX;
        int dy = pair.first.y - playerSectorY;

        if (abs(dx) > 2 || abs(dy) > 2) {
            toUnload.push_back(pair.first);
        }
    }

    // Remove asteroids from unloaded sectors
    for (const auto& key : toUnload) {
        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (asteroids[i].active &&
                asteroids[i].isPersistent &&
                asteroids[i].sectorX == key.x &&
                asteroids[i].sectorY == key.y) {
                asteroids[i].active = false;
            }
        }

        loadedSectors.erase(key);
        Logger::LogFormat("Unloaded sector (%d, %d)", key.x, key.y);
    }
}

void AsteroidSystem::Update(float deltaTime, const Vec3& playerPos, PlanetSystem* planetSystem) {
    float dt = deltaTime / 1000.0f;

    // Sector management
    float dx = playerPos.x - lastPlayerPos.x;
    float dy = playerPos.y - lastPlayerPos.y;
    float movedDist = sqrtf(dx * dx + dy * dy);

    if (movedDist > 20.0f) {
        LoadSectorsAroundPlayer(playerPos);
        UnloadDistantSectors(playerPos);
        lastPlayerPos = playerPos;
    }

    // Update all active asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) continue;

        if (asteroids[i].isFragment) {
            UpdateFragment(i, dt);
            continue;
        }

        // AI behavior
        switch (asteroids[i].type) {
        case WANDERER:
            UpdateWandererAI(i, dt);
            break;
        case ORBITER:
            UpdateOrbiterAI(i, dt, planetSystem);  // CHANGED: Pass planetSystem
            break;
        case FLOCKING:
            UpdateFlockingAI(i, dt);
            break;
        }

        // Update position
        asteroids[i].position.x += asteroids[i].velocity.x * dt;
        asteroids[i].position.y += asteroids[i].velocity.y * dt;
        asteroids[i].position.z += asteroids[i].velocity.z * dt;

        // Update rotation
        asteroids[i].currentRotation += asteroids[i].rotationSpeed * dt;
        if (asteroids[i].currentRotation > 360.0f) {
            asteroids[i].currentRotation -= 360.0f;
        }

        // Drag
        if (asteroids[i].type == FLOCKING) {
            asteroids[i].velocity.x *= 0.98f;
            asteroids[i].velocity.y *= 0.98f;
            asteroids[i].velocity.z *= 0.98f;
        }
        else {
            asteroids[i].velocity.x *= 0.95f;
            asteroids[i].velocity.y *= 0.95f;
            asteroids[i].velocity.z *= 0.95f;
        }
    }

    // Collision detection (existing code)
    UpdateSpatialGrid();

    float visibleRangeSq = 80.0f * 80.0f;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active || asteroids[i].isFragment) continue;

        float dx = asteroids[i].position.x - playerPos.x;
        float dy = asteroids[i].position.y - playerPos.y;
        float distSq = dx * dx + dy * dy;

        if (distSq > visibleRangeSq) continue;

        int cellX, cellY;
        GetGridCell(asteroids[i].position, cellX, cellY);

        std::vector<int> nearby;
        GetNearbyAsteroids(cellX, cellY, nearby);

        for (int j : nearby) {
            if (j <= i) continue;
            if (!asteroids[j].active || asteroids[j].isFragment) continue;

            float dx = asteroids[j].position.x - asteroids[i].position.x;
            float dy = asteroids[j].position.y - asteroids[i].position.y;
            float dz = asteroids[j].position.z - asteroids[i].position.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            float minDistSq = (asteroids[i].size + asteroids[j].size) *
                (asteroids[i].size + asteroids[j].size);

            if (distSq < minDistSq && distSq > 0.001f) {
                ResolveCollision(i, j);
            }
        }
    }
}

void AsteroidSystem::UpdateFragment(int index, float deltaTime) {
    Asteroid& frag = asteroids[index];
    frag.lifetime += deltaTime;

    if (frag.lifetime > frag.maxLifetime - 1.0f) {
        frag.alpha = (frag.maxLifetime - frag.lifetime) / 1.0f;
        if (frag.alpha < 0.0f) frag.alpha = 0.0f;
    }

    if (frag.lifetime >= frag.maxLifetime) {
        frag.active = false;
    }
}

void AsteroidSystem::GetGridCell(const Vec3& pos, int& cellX, int& cellY) {
    cellX = (int)(pos.x / cellSize) + 50;
    cellY = (int)(pos.y / cellSize) + 50;

    if (cellX < 0) cellX = 0;
    if (cellX >= GRID_SIZE) cellX = GRID_SIZE - 1;
    if (cellY < 0) cellY = 0;
    if (cellY >= GRID_SIZE) cellY = GRID_SIZE - 1;
}

void AsteroidSystem::UpdateSpatialGrid() {
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            spatialGrid[x][y].clear();
        }
    }

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) continue;
        if (asteroids[i].isFragment) continue;

        int cellX, cellY;
        GetGridCell(asteroids[i].position, cellX, cellY);
        spatialGrid[cellX][cellY].push_back(i);
    }
}

void AsteroidSystem::GetNearbyAsteroids(int cellX, int cellY, std::vector<int>& nearby) {
    nearby.clear();

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int nx = cellX + dx;
            int ny = cellY + dy;

            if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
                for (int idx : spatialGrid[nx][ny]) {
                    nearby.push_back(idx);
                }
            }
        }
    }
}

void AsteroidSystem::ResolveCollision(int index1, int index2) {
    Asteroid& a1 = asteroids[index1];
    Asteroid& a2 = asteroids[index2];

    Vec3 delta;
    delta.x = a2.position.x - a1.position.x;
    delta.y = a2.position.y - a1.position.y;
    delta.z = a2.position.z - a1.position.z;

    float distance = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    if (distance < 0.001f) return;

    Vec3 normal;
    normal.x = delta.x / distance;
    normal.y = delta.y / distance;
    normal.z = delta.z / distance;

    // CHANGED: Softer separation (was instant, now gradual)
    float overlap = (a1.size + a2.size) - distance;
    float separationRatio = overlap * 0.3f;  // Only separate 30% per frame

    a1.position.x -= normal.x * separationRatio;
    a1.position.y -= normal.y * separationRatio;
    a2.position.x += normal.x * separationRatio;
    a2.position.y += normal.y * separationRatio;

    Vec3 relVel;
    relVel.x = a2.velocity.x - a1.velocity.x;
    relVel.y = a2.velocity.y - a1.velocity.y;
    relVel.z = a2.velocity.z - a1.velocity.z;

    float velAlongNormal = relVel.x * normal.x + relVel.y * normal.y + relVel.z * normal.z;

    if (velAlongNormal > 0) return;

    // CHANGED: Softer restitution (was 0.7f, now 0.5f)
    float totalMass = a1.mass + a2.mass;
    float impulse = -(1.0f + 0.5f) * velAlongNormal / totalMass;

    Vec3 impulseVec;
    impulseVec.x = impulse * normal.x;
    impulseVec.y = impulse * normal.y;
    impulseVec.z = impulse * normal.z;

    // CHANGED: Dampen impulse application
    a1.velocity.x -= impulseVec.x * a2.mass * 0.8f;
    a1.velocity.y -= impulseVec.y * a2.mass * 0.8f;
    a2.velocity.x += impulseVec.x * a1.mass * 0.8f;
    a2.velocity.y += impulseVec.y * a1.mass * 0.8f;
}

void AsteroidSystem::BreakAsteroidIntoFragments(int index, const Vec3& impactPoint, const Vec3& impactVel) {
    Asteroid& parent = asteroids[index];

    float fragmentSize = parent.size * 0.4f;
    parent.active = false;

    for (int f = 0; f < fragmentCount; f++) {
        int slot = -1;
        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (!asteroids[i].active) {
                slot = i;
                break;
            }
        }

        if (slot == -1) break;

        float offsetAngle = (f / (float)fragmentCount) * 6.28318f + (rand() % 100) / 500.0f;
        float offsetDist = parent.size * 0.3f;

        asteroids[slot].position.x = parent.position.x + cosf(offsetAngle) * offsetDist;
        asteroids[slot].position.y = parent.position.y + sinf(offsetAngle) * offsetDist;
        asteroids[slot].position.z = parent.position.z;

        asteroids[slot].size = fragmentSize + (rand() % 100) / 500.0f;
        asteroids[slot].mass = asteroids[slot].size * asteroids[slot].size * asteroids[slot].size * 2.0f;

        Vec3 explosionDir;
        explosionDir.x = cosf(offsetAngle);
        explosionDir.y = sinf(offsetAngle);
        explosionDir.z = 0.0f;

        float explosionMag = fragmentSpeed * (0.8f + (rand() % 100) / 250.0f);

        asteroids[slot].velocity.x = parent.velocity.x + explosionDir.x * explosionMag;
        asteroids[slot].velocity.y = parent.velocity.y + explosionDir.y * explosionMag;
        asteroids[slot].velocity.z = 0.0f;

        asteroids[slot].rotationSpeed = -150.0f + (rand() % 1000) / 1000.0f * 300.0f;
        asteroids[slot].currentRotation = rand() % 360;

        asteroids[slot].lifetime = 0.0f;
        asteroids[slot].maxLifetime = 2.0f + (rand() % 100) / 100.0f;
        asteroids[slot].alpha = 1.0f;
        asteroids[slot].isFragment = true;
        asteroids[slot].isPersistent = false;  // Fragments are NOT persistent

        float grayValue = 0.3f + (rand() % 100) / 500.0f;
        asteroids[slot].r = grayValue;
        asteroids[slot].g = grayValue;
        asteroids[slot].b = grayValue;

        asteroids[slot].active = true;
    }
}

int AsteroidSystem::CheckBulletCollision(const Vec3& bulletPos, const Vec3& bulletVel, float bulletRadius) {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) continue;
        if (asteroids[i].isFragment) continue;

        float dx = asteroids[i].position.x - bulletPos.x;
        float dy = asteroids[i].position.y - bulletPos.y;
        float dz = asteroids[i].position.z - bulletPos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        float collisionRadius = asteroids[i].size + bulletRadius;
        float collisionRadiusSq = collisionRadius * collisionRadius;

        if (distSq < collisionRadiusSq) {
            if (asteroids[i].size > fragmentMinSize) {
                BreakAsteroidIntoFragments(i, bulletPos, bulletVel);
            }
            else {
                asteroids[i].active = false;
            }
            return i;
        }
    }
    return -1;
}

Vec3 AsteroidSystem::CheckPlayerCollision(const Vec3& playerPos, float playerRadius, const Vec3& playerVel) {
    Vec3 totalKnockback(0, 0, 0);
    int collisionCount = 0;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) continue;
        if (asteroids[i].isFragment) continue;

        float dx = asteroids[i].position.x - playerPos.x;
        float dy = asteroids[i].position.y - playerPos.y;
        float dz = asteroids[i].position.z - playerPos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        float collisionDist = asteroids[i].size + playerRadius;
        float collisionDistSq = collisionDist * collisionDist;

        if (distSq < collisionDistSq && distSq > 0.001f) {
            float distance = sqrtf(distSq);

            Vec3 normal;
            normal.x = dx / distance;
            normal.y = dy / distance;
            normal.z = dz / distance;

            // Push apart
            float overlap = collisionDist - distance;
            asteroids[i].position.x += normal.x * overlap;
            asteroids[i].position.y += normal.y * overlap;
            asteroids[i].position.z += normal.z * overlap;

            // Relative velocity
            Vec3 relVel;
            relVel.x = asteroids[i].velocity.x - playerVel.x;
            relVel.y = asteroids[i].velocity.y - playerVel.y;
            relVel.z = asteroids[i].velocity.z - playerVel.z;

            float velAlongNormal = relVel.x * normal.x + relVel.y * normal.y + relVel.z * normal.z;

            if (velAlongNormal < 0) {
                // SIMPLE: Just transfer player velocity to asteroid
                float transferAmount = 0.7f;  // 70% of player velocity

                asteroids[i].velocity.x += playerVel.x * transferAmount;
                asteroids[i].velocity.y += playerVel.y * transferAmount;
                asteroids[i].velocity.z += playerVel.z * transferAmount;

                // Small knockback
                float playerSpeed = sqrtf(playerVel.x * playerVel.x +
                    playerVel.y * playerVel.y);

                float knockbackMultiplier = 0.4f * (1.0f + asteroids[i].mass * 0.01f);

                totalKnockback.x -= normal.x * knockbackMultiplier;
                totalKnockback.y -= normal.y * knockbackMultiplier;
                totalKnockback.z -= normal.z * knockbackMultiplier;

                collisionCount++;
            }
        }
    }

    if (collisionCount > 1) {
        float dampen = 1.0f / sqrtf((float)collisionCount);
        totalKnockback.x *= dampen;
        totalKnockback.y *= dampen;
        totalKnockback.z *= dampen;
    }

    return totalKnockback;
}

void AsteroidSystem::Render(const Camera3D& camera) {
    Vec3 camPos = camera.GetPosition();
    float visibleRangeX = 60.0f;
    float visibleRangeY = 50.0f;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) continue;

        float dx = asteroids[i].position.x - camPos.x;
        float dy = asteroids[i].position.y - camPos.y;

        if (fabsf(dx) > visibleRangeX || fabsf(dy) > visibleRangeY) {
            continue;
        }

        Vec3 rotation(0, 0, asteroids[i].currentRotation);

        float r = asteroids[i].r;
        float g = asteroids[i].g;
        float b = asteroids[i].b;

        if (asteroids[i].isFragment) {
            r *= asteroids[i].alpha;
            g *= asteroids[i].alpha;
            b *= asteroids[i].alpha;
        }

        Renderer3D::DrawMesh(asteroidMesh, asteroids[i].position, rotation,
            Vec3(asteroids[i].size, asteroids[i].size, asteroids[i].size),
            camera, r, g, b, false);
    }
}

int AsteroidSystem::GetActiveAsteroidCount() const {
    int count = 0;
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active && !asteroids[i].isFragment) count++;
    }
    return count;
}

void AsteroidSystem::DestroyAsteroid(int index) {
    if (index >= 0 && index < MAX_ASTEROIDS) {
        asteroids[index].active = false;
    }
}

void AsteroidSystem::Clear() {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].active = false;
    }
    loadedSectors.clear();
}

void AsteroidSystem::UpdateWandererAI(int index, float deltaTime) {
    Asteroid& wanderer = asteroids[index];

    if (wanderer.patrolWaiting) {
        // Currently waiting at destination
        wanderer.patrolWaitTimer += deltaTime;

        // Slow down while waiting
        wanderer.velocity.x *= 0.9f;
        wanderer.velocity.y *= 0.9f;
        wanderer.velocity.z *= 0.9f;

        // Check if wait time is over
        if (wanderer.patrolWaitTimer >= wanderer.patrolWaitTime) {
            // Pick new random destination within sector
            float sectorCenterX = (wanderer.sectorX + 0.5f) * SECTOR_SIZE;
            float sectorCenterY = (wanderer.sectorY + 0.5f) * SECTOR_SIZE;

            float targetOffsetX = ((rand() % 1000) / 1000.0f - 0.5f) * SECTOR_SIZE * 0.7f;
            float targetOffsetY = ((rand() % 1000) / 1000.0f - 0.5f) * SECTOR_SIZE * 0.7f;

            wanderer.patrolTarget.x = sectorCenterX + targetOffsetX;
            wanderer.patrolTarget.y = sectorCenterY + targetOffsetY;
            wanderer.patrolTarget.z = 0.0f;

            // New random wait time (2-8 seconds)
            wanderer.patrolWaitTime = 2.0f + (rand() % 1000) / 1000.0f * 6.0f;
            wanderer.patrolWaitTimer = 0.0f;
            wanderer.patrolWaiting = false;

            Logger::LogFormat("Wanderer %d: New patrol target (%.1f, %.1f)",
                index, wanderer.patrolTarget.x, wanderer.patrolTarget.y);
        }
    }
    else {
        // Traveling to patrol target
        float dx = wanderer.patrolTarget.x - wanderer.position.x;
        float dy = wanderer.patrolTarget.y - wanderer.position.y;
        float dz = wanderer.patrolTarget.z - wanderer.position.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        float dist = sqrtf(distSq);

        // Check if reached target (within 3 units)
        if (dist < 3.0f) {
            // Start waiting
            wanderer.patrolWaiting = true;
            wanderer.patrolWaitTimer = 0.0f;
        }
        else {
            // Steer toward target
            if (dist > 0.001f) {
                Vec3 direction;
                direction.x = dx / dist;
                direction.y = dy / dist;
                direction.z = dz / dist;

                // Apply steering force (gradual acceleration)
                float steerForce = 1.5f;
                wanderer.velocity.x += direction.x * steerForce * deltaTime;
                wanderer.velocity.y += direction.y * steerForce * deltaTime;
                wanderer.velocity.z += direction.z * steerForce * deltaTime;

                // Limit max speed
                float speedSq = wanderer.velocity.x * wanderer.velocity.x +
                    wanderer.velocity.y * wanderer.velocity.y +
                    wanderer.velocity.z * wanderer.velocity.z;
                float maxSpeed = 3.0f;

                if (speedSq > maxSpeed * maxSpeed) {
                    float speed = sqrtf(speedSq);
                    wanderer.velocity.x = (wanderer.velocity.x / speed) * maxSpeed;
                    wanderer.velocity.y = (wanderer.velocity.y / speed) * maxSpeed;
                    wanderer.velocity.z = (wanderer.velocity.z / speed) * maxSpeed;
                }
            }
        }
    }
}

void AsteroidSystem::UpdateOrbiterAI(int index, float deltaTime, PlanetSystem* planetSystem) {
    Asteroid& orbiter = asteroids[index];

    // Check if has valid target
    if (orbiter.orbitTargetIndex == -1) {
        FindOrbitTarget(index, planetSystem);
    }

    // Verify target still exists
    if (orbiter.orbitTargetIndex != -1 && planetSystem) {
        const Planet* planets = planetSystem->GetPlanets();
        if (!planets[orbiter.orbitTargetIndex].active) {
            orbiter.orbitTargetIndex = -1;
            FindOrbitTarget(index, planetSystem);
        }
    }

    // If has target, orbit it
    if (orbiter.orbitTargetIndex != -1 && planetSystem) {
        const Planet* planets = planetSystem->GetPlanets();
        const Planet& target = planets[orbiter.orbitTargetIndex];

        // Update orbit angle
        float direction = orbiter.orbitClockwise ? -1.0f : 1.0f;
        orbiter.orbitAngle += orbiter.orbitSpeed * direction * deltaTime;

        if (orbiter.orbitAngle > 6.28318f) orbiter.orbitAngle -= 6.28318f;
        if (orbiter.orbitAngle < 0.0f) orbiter.orbitAngle += 6.28318f;

        // Calculate desired position around planet
        Vec3 desiredPos;
        desiredPos.x = target.position.x + cosf(orbiter.orbitAngle) * orbiter.orbitRadius;
        desiredPos.y = target.position.y + sinf(orbiter.orbitAngle) * orbiter.orbitRadius;
        desiredPos.z = target.position.z;

        // Steer toward desired position
        Vec3 toDesired;
        toDesired.x = desiredPos.x - orbiter.position.x;
        toDesired.y = desiredPos.y - orbiter.position.y;
        toDesired.z = desiredPos.z - orbiter.position.z;

        float steerStrength = 2.0f;
        orbiter.velocity.x += toDesired.x * steerStrength * deltaTime;
        orbiter.velocity.y += toDesired.y * steerStrength * deltaTime;
        orbiter.velocity.z += toDesired.z * steerStrength * deltaTime;

        // Planets don't move, so no need to match velocity
    }
    else {
        // No target - drift randomly
        UpdateWandererAI(index, deltaTime);
    }
}

void AsteroidSystem::FindOrbitTarget(int index, PlanetSystem* planetSystem) {
    if (!planetSystem) {
        asteroids[index].orbitTargetIndex = -1;
        return;
    }

    int targetIndex = FindNearestPlanet(asteroids[index].position, 50.0f, planetSystem);

    if (targetIndex != -1) {
        asteroids[index].orbitTargetIndex = targetIndex;
        asteroids[index].orbitingPlanet = true;

        // Count other orbiters on this planet
        int orbitersOnTarget = 0;
        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (i == index) continue;
            if (!asteroids[i].active) continue;
            if (asteroids[i].type != ORBITER) continue;
            if (asteroids[i].orbitTargetIndex == targetIndex && asteroids[i].orbitingPlanet) {
                orbitersOnTarget++;
            }
        }

        // Assign orbital lane
        const Planet* planets = planetSystem->GetPlanets();
        float planetSize = planets[targetIndex].size;
        float baseRadius = planetSize + 3.0f;  // Start 3 units from surface
        float laneSpacing = 2.5f;

        asteroids[index].orbitRadius = baseRadius + (orbitersOnTarget * laneSpacing);
        asteroids[index].orbitAngle = (rand() % 1000) / 1000.0f * 6.28318f;

        Logger::LogFormat("Orbiter %d locked to planet %d (lane %d, radius %.1f)",
            index, targetIndex, orbitersOnTarget, asteroids[index].orbitRadius);
    }
    else {
        asteroids[index].orbitTargetIndex = -1;
        asteroids[index].orbitingPlanet = false;
    }
}

int AsteroidSystem::FindNearestPlanet(const Vec3& pos, float maxDistance, PlanetSystem* planetSystem) {
    if (!planetSystem) return -1;

    const Planet* planets = planetSystem->GetPlanets();
    int maxPlanets = planetSystem->GetMaxPlanets();

    int bestIndex = -1;
    float bestDistSq = maxDistance * maxDistance;

    for (int i = 0; i < maxPlanets; i++) {
        if (!planets[i].active) continue;

        float dx = planets[i].position.x - pos.x;
        float dy = planets[i].position.y - pos.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestIndex = i;
        }
    }

    return bestIndex;
}

void AsteroidSystem::UpdateFlockingAI(int index, float deltaTime) {
    Asteroid& flocker = asteroids[index];

    // Find nearby flockers - INCREASED RADIUS
    std::vector<int> neighbors;
    GetFlockingNeighbors(index, neighbors, 25.0f);  // Was 15.0f, now 25.0f

    if (neighbors.empty()) {
        // No neighbors - add slight attraction to nearest flocker
        int nearest = -1;
        float nearestDist = 100.0f;

        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (i == index) continue;
            if (!asteroids[i].active) continue;
            if (asteroids[i].isFragment) continue;
            if (asteroids[i].type != FLOCKING) continue;

            float dx = asteroids[i].position.x - flocker.position.x;
            float dy = asteroids[i].position.y - flocker.position.y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < nearestDist) {
                nearestDist = dist;
                nearest = i;
            }
        }

        if (nearest != -1) {
            // Drift toward nearest flocker
            float dx = asteroids[nearest].position.x - flocker.position.x;
            float dy = asteroids[nearest].position.y - flocker.position.y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist > 0.001f) {
                flocker.velocity.x += (dx / dist) * 0.5f * deltaTime;
                flocker.velocity.y += (dy / dist) * 0.5f * deltaTime;
            }
        }
        else {
            // No other flockers at all - drift randomly
            UpdateWandererAI(index, deltaTime);
        }
        return;
    }

    // Boids algorithm: 3 rules
    Vec3 separation(0, 0, 0);
    Vec3 alignment(0, 0, 0);
    Vec3 cohesion(0, 0, 0);

    int nearCount = 0;
    int totalCount = (int)neighbors.size();

    for (int neighborIdx : neighbors) {
        Asteroid& neighbor = asteroids[neighborIdx];

        float dx = neighbor.position.x - flocker.position.x;
        float dy = neighbor.position.y - flocker.position.y;
        float dz = neighbor.position.z - flocker.position.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        float dist = sqrtf(distSq);

        // Rule 1: Separation (avoid crowding) - INCREASED DISTANCE
        if (dist < 8.0f && dist > 0.001f) {  // Was 5.0f, now 8.0f
            separation.x -= dx / dist;
            separation.y -= dy / dist;
            separation.z -= dz / dist;
            nearCount++;
        }

        // Rule 2: Alignment (match velocity)
        alignment.x += neighbor.velocity.x;
        alignment.y += neighbor.velocity.y;
        alignment.z += neighbor.velocity.z;

        // Rule 3: Cohesion (move toward average position)
        cohesion.x += neighbor.position.x;
        cohesion.y += neighbor.position.y;
        cohesion.z += neighbor.position.z;
    }

    // Average and apply forces
    if (nearCount > 0) {
        separation.x /= nearCount;
        separation.y /= nearCount;
        separation.z /= nearCount;
    }

    if (totalCount > 0) {
        alignment.x /= totalCount;
        alignment.y /= totalCount;
        alignment.z /= totalCount;

        cohesion.x /= totalCount;
        cohesion.y /= totalCount;
        cohesion.z /= totalCount;

        cohesion.x -= flocker.position.x;
        cohesion.y -= flocker.position.y;
        cohesion.z -= flocker.position.z;
    }

    // INCREASED WEIGHTS for more visible flocking
    float separationWeight = 2.0f;   // Was 1.5f
    float alignmentWeight = 1.2f;    // Was 0.5f
    float cohesionWeight = 1.5f;     // Was 0.8f

    // Apply forces
    flocker.velocity.x += separation.x * separationWeight * deltaTime;
    flocker.velocity.y += separation.y * separationWeight * deltaTime;
    flocker.velocity.z += separation.z * separationWeight * deltaTime;

    flocker.velocity.x += (alignment.x - flocker.velocity.x) * alignmentWeight * deltaTime;
    flocker.velocity.y += (alignment.y - flocker.velocity.y) * alignmentWeight * deltaTime;
    flocker.velocity.z += (alignment.z - flocker.velocity.z) * alignmentWeight * deltaTime;

    flocker.velocity.x += cohesion.x * cohesionWeight * deltaTime;
    flocker.velocity.y += cohesion.y * cohesionWeight * deltaTime;
    flocker.velocity.z += cohesion.z * cohesionWeight * deltaTime;
}

void AsteroidSystem::GetFlockingNeighbors(int index, std::vector<int>& neighbors, float radius) {
    neighbors.clear();

    Vec3 pos = asteroids[index].position;
    float radiusSq = radius * radius;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (i == index) continue;
        if (!asteroids[i].active) continue;
        if (asteroids[i].isFragment) continue;
        if (asteroids[i].type != FLOCKING) continue;

        float dx = asteroids[i].position.x - pos.x;
        float dy = asteroids[i].position.y - pos.y;
        float dz = asteroids[i].position.z - pos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < radiusSq) {
            neighbors.push_back(i);
        }
    }
}

