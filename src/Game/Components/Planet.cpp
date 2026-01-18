//------------------------------------------------------------------------
// Planet.cpp - Persistent planet system
//------------------------------------------------------------------------
#include "Planet.h"
#include "../Utilities/Logger.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>

PlanetSystem::PlanetSystem()
    : lastPlayerPos(0, 0, 0)
{
    for (int i = 0; i < MAX_PLANETS; i++) {
        planets[i].active = false;
    }
}

void PlanetSystem::Init() {
    // Higher detail sphere for planets (they're larger)
    planetMesh = Mesh3D::CreateSphere(1.0f, 16);

    Logger::LogInfo("PlanetSystem: Initialized");

    // Load initial sectors (3x3 grid around origin = up to 9 planets)
    LoadSectorsAroundPlayer(Vec3(0, 0, 0));
    lastPlayerPos = Vec3(0, 0, 0);

    Logger::LogFormat("Initial planets loaded: %d", GetActivePlanetCount());
}

void PlanetSystem::GetSectorCoords(const Vec3& pos, int& sectorX, int& sectorY) {
    sectorX = (int)floorf(pos.x / SECTOR_SIZE);
    sectorY = (int)floorf(pos.y / SECTOR_SIZE);
}

void PlanetSystem::LoadSectorsAroundPlayer(const Vec3& playerPos) {
    int playerSectorX, playerSectorY;
    GetSectorCoords(playerPos, playerSectorX, playerSectorY);

    // Load 3x3 grid of sectors (up to 9 planets visible)
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int sectorX = playerSectorX + dx;
            int sectorY = playerSectorY + dy;

            PlanetSectorKey key = { sectorX, sectorY };

            if (loadedSectors.find(key) == loadedSectors.end()) {
                GenerateSectorPlanet(sectorX, sectorY);
                loadedSectors[key] = true;
            }
        }
    }
}

void PlanetSystem::GenerateSectorPlanet(int sectorX, int sectorY) {
    // Deterministic seed based on sector
    unsigned int seed = (unsigned int)(sectorX * 73856093) ^ (unsigned int)(sectorY * 19349663);
    unsigned int oldSeed = rand();
    srand(seed);

    // Not every sector has a planet (40% chance)
    float spawnChance = (rand() % 1000) / 1000.0f;
    if (spawnChance > 0.4f) {
        srand(oldSeed);
        return;  // No planet in this sector
    }

    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_PLANETS; i++) {
        if (!planets[i].active) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        Logger::LogWarning("No free planet slots!");
        srand(oldSeed);
        return;
    }

    // Planet position (center of sector with small offset)
    float centerX = (sectorX + 0.5f) * SECTOR_SIZE;
    float centerY = (sectorY + 0.5f) * SECTOR_SIZE;

    float offsetX = ((rand() % 1000) / 1000.0f - 0.5f) * 30.0f;  // Small offset
    float offsetY = ((rand() % 1000) / 1000.0f - 0.5f) * 30.0f;

    planets[slot].position.x = centerX + offsetX;
    planets[slot].position.y = centerY + offsetY;
    planets[slot].position.z = 0.0f;

    // Size: 6.0 to 15.0 (asteroids are max 2.0, so 3x-7.5x bigger)
    planets[slot].size = 6.0f + (rand() % 1000) / 1000.0f * 9.0f;

    // Slow rotation
    planets[slot].rotationSpeed = -10.0f + (rand() % 1000) / 1000.0f * 20.0f;
    planets[slot].currentRotation = (rand() % 360);

    // Sector tracking
    planets[slot].sectorX = sectorX;
    planets[slot].sectorY = sectorY;
    planets[slot].active = true;

    Logger::LogFormat("Planet spawned in sector (%d, %d) - size: %.1f",
        sectorX, sectorY, planets[slot].size);

    srand(oldSeed);
}

void PlanetSystem::UnloadDistantSectors(const Vec3& playerPos) {
    int playerSectorX, playerSectorY;
    GetSectorCoords(playerPos, playerSectorX, playerSectorY);

    std::vector<PlanetSectorKey> toUnload;

    // Unload sectors more than 2 sectors away
    for (auto& pair : loadedSectors) {
        int dx = pair.first.x - playerSectorX;
        int dy = pair.first.y - playerSectorY;

        if (abs(dx) > 2 || abs(dy) > 2) {
            toUnload.push_back(pair.first);
        }
    }

    // Remove planets from unloaded sectors
    for (const auto& key : toUnload) {
        for (int i = 0; i < MAX_PLANETS; i++) {
            if (planets[i].active &&
                planets[i].sectorX == key.x &&
                planets[i].sectorY == key.y) {
                planets[i].active = false;
                Logger::LogFormat("Planet unloaded from sector (%d, %d)", key.x, key.y);
            }
        }
        loadedSectors.erase(key);
    }
}

void PlanetSystem::Update(float deltaTime, const Vec3& playerPos) {
    float dt = deltaTime / 1000.0f;

    // Check if player moved significantly
    float dx = playerPos.x - lastPlayerPos.x;
    float dy = playerPos.y - lastPlayerPos.y;
    float movedDist = sqrtf(dx * dx + dy * dy);

    if (movedDist > 20.0f) {
        LoadSectorsAroundPlayer(playerPos);
        UnloadDistantSectors(playerPos);
        lastPlayerPos = playerPos;
    }

    // Update planet rotations
    for (int i = 0; i < MAX_PLANETS; i++) {
        if (!planets[i].active) continue;

        planets[i].currentRotation += planets[i].rotationSpeed * dt;
        if (planets[i].currentRotation > 360.0f) {
            planets[i].currentRotation -= 360.0f;
        }
        if (planets[i].currentRotation < 0.0f) {
            planets[i].currentRotation += 360.0f;
        }
    }
}

void PlanetSystem::Render(const Camera3D& camera) {
    Vec3 camPos = camera.GetPosition();

    // Culling bounds (planets are large, use bigger range)
    float visibleRangeX = 70.0f;
    float visibleRangeY = 60.0f;

    for (int i = 0; i < MAX_PLANETS; i++) {
        if (!planets[i].active) continue;

        float dx = planets[i].position.x - camPos.x;
        float dy = planets[i].position.y - camPos.y;

        // Cull if outside visible range (accounting for planet size)
        if (fabsf(dx) > visibleRangeX + planets[i].size ||
            fabsf(dy) > visibleRangeY + planets[i].size) {
            continue;
        }

        Vec3 rotation(0, 0, planets[i].currentRotation);

        // White color
        float white = 1.0f;

        Renderer3D::DrawMesh(planetMesh, planets[i].position, rotation,
            Vec3(planets[i].size, planets[i].size, planets[i].size),
            camera, white, white, white, false);
    }
}

bool PlanetSystem::CheckAsteroidCollision(const Vec3& asteroidPos, float asteroidRadius, Vec3& pushDirection) {
    bool collided = false;
    pushDirection = Vec3(0, 0, 0);

    for (int i = 0; i < MAX_PLANETS; i++) {
        if (!planets[i].active) continue;

        float dx = asteroidPos.x - planets[i].position.x;
        float dy = asteroidPos.y - planets[i].position.y;
        float dz = asteroidPos.z - planets[i].position.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        float collisionDist = asteroidRadius + planets[i].size;
        float collisionDistSq = collisionDist * collisionDist;

        if (distSq < collisionDistSq && distSq > 0.001f) {
            float dist = sqrtf(distSq);

            // Calculate overlap (how much inside planet)
            float overlap = collisionDist - dist;

            // Normal pointing away from planet
            Vec3 normal;
            normal.x = dx / dist;
            normal.y = dy / dist;
            normal.z = dz / dist;

            // Accumulate push direction (normalized push * overlap)
            pushDirection.x += normal.x * overlap;
            pushDirection.y += normal.y * overlap;
            pushDirection.z += normal.z * overlap;

            collided = true;
        }
    }

    return collided;
}

bool PlanetSystem::CheckPlayerCollision(const Vec3& playerPos, float playerRadius, Vec3& pushDirection) {
    bool collided = false;
    pushDirection = Vec3(0, 0, 0);

    for (int i = 0; i < MAX_PLANETS; i++) {
        if (!planets[i].active) continue;

        float dx = playerPos.x - planets[i].position.x;
        float dy = playerPos.y - planets[i].position.y;
        float dz = playerPos.z - planets[i].position.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        float collisionDist = playerRadius + planets[i].size;
        float collisionDistSq = collisionDist * collisionDist;

        if (distSq < collisionDistSq && distSq > 0.001f) {
            float dist = sqrtf(distSq);

            // Calculate overlap
            float overlap = collisionDist - dist;

            // Normal pointing away from planet
            Vec3 normal;
            normal.x = dx / dist;
            normal.y = dy / dist;
            normal.z = dz / dist;

            // Accumulate push direction
            pushDirection.x += normal.x * overlap;
            pushDirection.y += normal.y * overlap;
            pushDirection.z += normal.z * overlap;

            collided = true;
        }
    }

    return collided;
}

int PlanetSystem::GetActivePlanetCount() const {
    int count = 0;
    for (int i = 0; i < MAX_PLANETS; i++) {
        if (planets[i].active) count++;
    }
    return count;
}

void PlanetSystem::Clear() {
    for (int i = 0; i < MAX_PLANETS; i++) {
        planets[i].active = false;
    }
    loadedSectors.clear();
}
