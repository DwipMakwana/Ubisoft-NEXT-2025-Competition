//------------------------------------------------------------------------
// Asteroid.cpp - Stationary / physics-driven asteroid field
//------------------------------------------------------------------------

#include "Asteroid.h"
#include "../Utilities/Logger.h"
#include "../Components/Planet.h"

#include <cstdlib>
#include <cmath>

AsteroidSystem::AsteroidSystem()
    : planetSystemRef(nullptr)
    , nextFragmentSlot(0)
    , spawnDistance(200.0f)
    , cullDistance(260.0f)
    , cellSize(40.0f)
    , lastPlayerPos(0, 0, 0)
{
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].active = false;
    }

    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            spatialGrid[x][y].clear();
        }
    }

    for (int i = 0; i < MAX_TRACKED_FRAGMENTS; i++) {
        trackedFragments[i].lifetime = 0.0f;
        trackedFragments[i].collected = false;
    }
}

void AsteroidSystem::Init() {
    asteroidMesh = Mesh3D::CreateRock(1.0f, 1);
    Logger::LogInfo("AsteroidSystem: Initialized (physics-only)");

    // Load initial sectors around origin
    LoadSectorsAroundPlayer(Vec3(0, 0, 0));
    lastPlayerPos = Vec3(0, 0, 0);
}

int AsteroidSystem::FindFreeSlot() {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) return i;
    }
    return -1;
}

void AsteroidSystem::GetSectorCoords(const Vec3& pos, int& sectorX, int& sectorY) {
    sectorX = (int)floorf(pos.x / SECTOR_SIZE);
    sectorY = (int)floorf(pos.y / SECTOR_SIZE);
}

void AsteroidSystem::LoadSectorsAroundPlayer(const Vec3& playerPos) {
    int playerSectorX, playerSectorY;
    GetSectorCoords(playerPos, playerSectorX, playerSectorY);
    for (int dx = -2; dx <= 2; ++dx) {  // Changed from -1 to 1  
        for (int dy = -2; dy <= 2; ++dy) {
            int sx = playerSectorX + dx;
            int sy = playerSectorY + dy;
            SectorKey key{ sx, sy };
            if (loadedSectors.find(key) == loadedSectors.end()) {
                GenerateSectorAsteroids(sx, sy);
                loadedSectors[key] = true;
            }
        }
    }
}

void AsteroidSystem::UnloadDistantSectors(const Vec3& playerPos) {
    int playerSectorX, playerSectorY;
    GetSectorCoords(playerPos, playerSectorX, playerSectorY);

    std::vector<SectorKey> toUnload;

    for (auto& pair : loadedSectors) {
        int dx = pair.first.x - playerSectorX;
        int dy = pair.first.y - playerSectorY;

        if (std::abs(dx) > 2 || std::abs(dy) > 2) {
            toUnload.push_back(pair.first);
        }
    }

    for (const auto& key : toUnload) {
        // deactivate asteroids in those sectors (unless persistent or towed)
        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            Asteroid& a = asteroids[i];
            if (!a.active) continue;
            if (a.isPersistent || a.isTowed) continue;

            if (a.sectorX == key.x && a.sectorY == key.y) {
                a.active = false;
            }
        }
        loadedSectors.erase(key);
    }
}

void AsteroidSystem::GenerateSectorAsteroids(int sectorX, int sectorY) {
    unsigned int seed = (unsigned int)(sectorX * 73856093) ^ (unsigned int)(sectorY * 19349663);
    unsigned int oldSeed = rand();
    srand(seed);

    int count = ASTEROIDS_PER_SECTOR;

    float centerX = (sectorX + 0.5f) * SECTOR_SIZE;
    float centerY = (sectorY + 0.5f) * SECTOR_SIZE;

    for (int n = 0; n < count; n++) {
        int slot = FindFreeSlot();
        if (slot == -1) {
            Logger::LogWarning("AsteroidSystem: No free asteroid slots!");
            break;
        }

        Asteroid& a = asteroids[slot];

        a.active = true;
        a.isFragment = false;
        a.isPersistent = true;
        a.isTowed = false;

        a.sectorX = sectorX;
        a.sectorY = sectorY;

        // Random position within the sector
        float offsetX = ((rand() % 1000) / 1000.0f - 0.5f) * (SECTOR_SIZE - 10.0f);
        float offsetY = ((rand() % 1000) / 1000.0f - 0.5f) * (SECTOR_SIZE - 10.0f);
        float offsetZ = 0.0f;

        a.position.x = centerX + offsetX;
        a.position.y = centerY + offsetY;
        a.position.z = offsetZ;

        // In GenerateSectorAsteroids, after setting a.position
        bool tooCloseToPlanet = false;
        for (int p = 0; p < planetSystemRef->GetMaxPlanets(); p++) {
            const Planet* planet = &planetSystemRef->GetPlanets()[p];
            if (!planet->active) continue;

            float dx = a.position.x - planet->position.x;
            float dy = a.position.y - planet->position.y;
            float distSq = dx * dx + dy * dy;

            float safeDist = planet->size + 15.0f; // 15 units clear zone
            if (distSq < safeDist * safeDist) {
                tooCloseToPlanet = true;
                break;
            }
        }

        if (tooCloseToPlanet) {
            // Skip this spawn position, try again or reduce count
            continue;
        }

        // Small initial drift
        a.velocity.x = ((rand() % 1000) / 1000.0f - 0.5f) * 0.5f;
        a.velocity.y = ((rand() % 1000) / 1000.0f - 0.5f) * 0.5f;
		a.velocity.z = 0.0f;

        a.size = 0.6f + ((rand() % 1000) / 1000.0f) * 1.0f; // ~0.6 to 1.6
        a.mass = a.size * a.size;

        a.rotationSpeed = ((rand() % 1000) / 1000.0f - 0.5f) * 40.0f;
        a.currentRotation = (rand() % 360);
        a.alpha = 1.0f;

        // Mineral distribution
        int typeRoll = rand() % 100;
        if (typeRoll < 25) {
            a.mineralType = MINERAL_IRON;
            float brightness = 0.4f + (rand() % 1000) / 1000.0f * 0.4f;
            a.r = brightness;
            a.g = brightness * 0.2f;
            a.b = brightness * 0.2f;
        }
        else if (typeRoll < 50) {
            a.mineralType = MINERAL_WATER;
            float brightness = 0.4f + (rand() % 1000) / 1000.0f * 0.4f;
            a.r = brightness * 0.2f;
            a.g = brightness * 0.5f;
            a.b = brightness;
        }
        else if (typeRoll < 75) {
            a.mineralType = MINERAL_CARBON;
            float brightness = 0.4f + (rand() % 1000) / 1000.0f * 0.4f;
            a.r = brightness * 0.2f;
            a.g = brightness;
            a.b = brightness * 0.3f;
        }
        else {
            a.mineralType = MINERAL_ENERGY;
            float brightness = 0.4f + (rand() % 1000) / 1000.0f * 0.3f;
            a.r = brightness;
            a.g = brightness;
            a.b = brightness * 0.3f;
        }

        // Resource amount scales with size
        a.resourceAmount = 20.0f + a.size * 20.0f;

        a.lifetime = 0.0f;
        a.maxLifetime = 0.0f;
    }

    srand(oldSeed);
}

void AsteroidSystem::ClearSpatialGrid() {
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            spatialGrid[x][y].clear();
        }
    }
}

void AsteroidSystem::GetGridCell(const Vec3& pos, int& cellX, int& cellY) {
    cellX = (int)floorf((pos.x + (GRID_SIZE * cellSize * 0.5f)) / cellSize);
    cellY = (int)floorf((pos.y + (GRID_SIZE * cellSize * 0.5f)) / cellSize);

    if (cellX < 0) cellX = 0;
    if (cellY < 0) cellY = 0;
    if (cellX >= GRID_SIZE) cellX = GRID_SIZE - 1;
    if (cellY >= GRID_SIZE) cellY = GRID_SIZE - 1;
}

void AsteroidSystem::UpdateSpatialGrid() {
    ClearSpatialGrid();

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active || asteroids[i].isFragment) continue;

        int cellX, cellY;
        GetGridCell(asteroids[i].position, cellX, cellY);
        spatialGrid[cellX][cellY].push_back(i);
    }
}

void AsteroidSystem::GetNearbyAsteroids(int cellX, int cellY, std::vector<int>& nearby) {
    nearby.clear();
    for (int x = cellX - 1; x <= cellX + 1; x++) {
        for (int y = cellY - 1; y <= cellY + 1; y++) {
            if (x < 0 || y < 0 || x >= GRID_SIZE || y >= GRID_SIZE) continue;
            for (int idx : spatialGrid[x][y]) {
                nearby.push_back(idx);
            }
        }
    }
}

void AsteroidSystem::ResolveCollision(int index1, int index2) {
    Asteroid& a = asteroids[index1];
    Asteroid& b = asteroids[index2];

    Vec3 delta = b.position - a.position;
    float distSq = delta.LengthSquared();
    if (distSq <= 0.0001f) return;

    float minDist = a.size + b.size;
    if (distSq >= minDist * minDist) return;

    float dist = sqrtf(distSq);
    Vec3 normal = delta / dist;

    float penetration = minDist - dist;

    // Separate positions
    float totalMass = a.mass + b.mass;
    float moveA = (b.mass / totalMass) * penetration;
    float moveB = (a.mass / totalMass) * penetration;

    a.position = a.position - normal * moveA;
    b.position = b.position + normal * moveB;

    // Simple elastic collision
    float va = a.velocity.Dot(normal);
    float vb = b.velocity.Dot(normal);
    float relVel = vb - va;

    if (relVel < 0.0f) {
        float e = 0.3f; // restitution
        float j = -(1.0f + e) * relVel / (1.0f / a.mass + 1.0f / b.mass);

        Vec3 impulse = normal * j;
        a.velocity = a.velocity - impulse / a.mass;
        b.velocity = b.velocity + impulse / b.mass;
    }
}

void AsteroidSystem::ResolveAsteroidCollisions(const Vec3& playerPos) {
    UpdateSpatialGrid();

    float visibleRangeSq = 90.0f * 90.0f;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        Asteroid& a = asteroids[i];
        if (!a.active || a.isFragment) continue;

        float dx = a.position.x - playerPos.x;
        float dy = a.position.y - playerPos.y;
        float distSq = dx * dx + dy * dy;
        if (distSq > visibleRangeSq) continue;

        int cellX, cellY;
        GetGridCell(a.position, cellX, cellY);
        std::vector<int> nearby;
        GetNearbyAsteroids(cellX, cellY, nearby);

        for (int j : nearby) {
            if (j <= i) continue;
            if (!asteroids[j].active || asteroids[j].isFragment) continue;
            ResolveCollision(i, j);
        }
    }
}

void AsteroidSystem::Update(float deltaTime,
    const Vec3& playerPos,
    PlanetSystem* planetSystem) {
    float dt = deltaTime / 1000.0f;
    planetSystemRef = planetSystem;

    // Sector management
    float dx = playerPos.x - lastPlayerPos.x;
    float dy = playerPos.y - lastPlayerPos.y;
    float movedDist = sqrtf(dx * dx + dy * dy);

    if (movedDist > 50.0f) {
        LoadSectorsAroundPlayer(playerPos);
        UnloadDistantSectors(playerPos);
        lastPlayerPos = playerPos;
    }

    // Update asteroids (physics only)
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        Asteroid& a = asteroids[i];
        if (!a.active) continue;

        if (a.isFragment) {
            UpdateFragment(i, dt);
            continue;
        }

        if (a.isTowed) {
            // Position is controlled by towing system, skip physics
            continue;
        }

        // Light drag
        a.velocity = a.velocity * 0.998f;

        // Integrate motion
        a.position.x += a.velocity.x * dt;
        a.position.y += a.velocity.y * dt;
        a.position.z += a.velocity.z * dt;

        // Rotation
        a.currentRotation += a.rotationSpeed * dt;
        if (a.currentRotation > 360.0f) a.currentRotation -= 360.0f;
        if (a.currentRotation < 0.0f)   a.currentRotation += 360.0f;
    }

    // Handle asteroid-asteroid collisions
    ResolveAsteroidCollisions(playerPos);

    // Update fragment tracking (if used for AI hints later)
    UpdateFragmentTracking(dt);
}

void AsteroidSystem::Render(const Camera3D& camera) {
    Vec3 camPos = camera.GetPosition();

    float visibleRangeX = 80.0f;
    float visibleRangeY = 70.0f;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        const Asteroid& a = asteroids[i];
        if (!a.active) continue;

        float dx = a.position.x - camPos.x;
        float dy = a.position.y - camPos.y;

        if (fabsf(dx) > visibleRangeX + a.size ||
            fabsf(dy) > visibleRangeY + a.size) {
            continue;
        }

        Vec3 rotation(0, 0, a.currentRotation);

        Renderer3D::DrawMesh(
            asteroidMesh,
            a.position,
            rotation,
            Vec3(a.size, a.size, a.size),
            camera,
            a.r, a.g, a.b,
            false
        );
    }
}

int AsteroidSystem::GetActiveAsteroidCount() const {
    int count = 0;
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) count++;
    }
    return count;
}

Vec3 AsteroidSystem::CheckPlayerCollision(const Vec3& playerPos,
    float playerRadius,
    const Vec3& playerVel) {
    Vec3 totalPush(0, 0, 0);

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        Asteroid& a = asteroids[i];
        if (!a.active || a.isFragment) continue;

        // FIXED (pushing)
        float dx = playerPos.x - a.position.x;
        float dy = playerPos.y - a.position.y;
        float dz = playerPos.z - a.position.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        float minDist = playerRadius + a.size;
        if (distSq < minDist * minDist && distSq > 0.0001f) {
            float dist = sqrtf(distSq);
            float overlap = minDist - dist;

            // CORRECT: normal points FROM player TO asteroid = pushes player away
            Vec3 normal(-dx / dist, -dy / dist, -dz / dist);
            totalPush = totalPush + normal * overlap;
        }
    }

    return totalPush;
}

void AsteroidSystem::DestroyAsteroid(int index) {
    if (index < 0 || index >= MAX_ASTEROIDS) return;
    asteroids[index].active = false;
}

void AsteroidSystem::Clear() {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].active = false;
    }
    loadedSectors.clear();
}

// Fragments – keep simple / minimal; you can reuse your old logic if needed.

void AsteroidSystem::BreakAsteroidIntoFragments(int index,
    const Vec3& impactPoint,
    const Vec3& impactVel) {
    Asteroid& src = asteroids[index];

    // Example: create a few small permanent fragments
    int fragmentCount = 3;
    for (int i = 0; i < fragmentCount; i++) {
        int slot = FindFreeSlot();
        if (slot == -1) break;

        Asteroid& f = asteroids[slot];
        f.active = true;
        f.isFragment = true;
        f.isPersistent = true;
        f.isTowed = false;

        f.position = impactPoint;
        float angle = (float)(rand() % 1000) / 1000.0f * 6.28318f;
        float speed = 4.0f + (float)(rand() % 1000) / 1000.0f * 3.0f;

        f.velocity.x = impactVel.x * 0.3f + cosf(angle) * speed;
        f.velocity.y = impactVel.y * 0.3f + sinf(angle) * speed;
        f.velocity.z = impactVel.z * 0.3f;

        f.size = src.size * 0.3f;
        f.mass = f.size * f.size;

        f.rotationSpeed = src.rotationSpeed * 1.5f;
        f.currentRotation = src.currentRotation;
        f.alpha = 1.0f;

        f.mineralType = src.mineralType;
        f.resourceAmount = src.resourceAmount * 0.3f;

        f.lifetime = 0.0f;
        f.maxLifetime = 0.0f; // 0 = never expires (or set to >0 to fade out)
        f.sectorX = src.sectorX;
        f.sectorY = src.sectorY;
    }

    src.active = false;
}

void AsteroidSystem::UpdateFragment(int index, float deltaTime) {
    Asteroid& f = asteroids[index];

    // If maxLifetime == 0, fragment is persistent
    if (f.maxLifetime == 0.0f) {
        f.velocity = f.velocity * 0.98f;
        return;
    }

    f.lifetime += deltaTime;
    if (f.lifetime > f.maxLifetime) {
        f.active = false;
        return;
    }

    if (f.lifetime > f.maxLifetime - 1.0f) {
        float t = (f.maxLifetime - f.lifetime);
        if (t < 0.0f) t = 0.0f;
        f.alpha = t / 1.0f;
    }
}

void AsteroidSystem::UpdateFragmentTracking(float deltaTime) {
    // Optional: you can keep this empty or reuse your existing logic
    // if you want other systems (AI, etc.) to react to fragments.
}
