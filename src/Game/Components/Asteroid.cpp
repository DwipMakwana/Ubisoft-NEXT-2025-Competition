//------------------------------------------------------------------------
// Asteroid.cpp - Fragmentation physics system
//------------------------------------------------------------------------
#include "Asteroid.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>

AsteroidSystem::AsteroidSystem()
    : spawnTimer(0.0f)
    , spawnInterval(0.05f)
    , spawnDistance(70.0f)
    , minSize(0.5f)
    , maxSize(2.0f)
    , minSpeed(1.0f)
    , maxSpeed(3.0f)
    , restitution(0.7f)
    , fragmentMinSize(0.6f)    // Asteroids smaller than this = direct destruction
    , fragmentCount(4)         // Break into 4 pieces
    , fragmentSpeed(8.0f)      // Explosion velocity
    , cellSize(10.0f)
    , nextFreeIndex(0)
{
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].active = false;
        asteroids[i].isFragment = false;
    }
}

void AsteroidSystem::Init() {
    asteroidMesh = Mesh3D::CreateSphere(1.0f, 5);
    printf("AsteroidSystem initialized with FRAGMENTATION\n");
    printf("- Fragments per break: %d\n", fragmentCount);
    printf("- Fragment min size: %.2f\n", fragmentMinSize);
}

void AsteroidSystem::SpawnAsteroid(const Vec3& earthPos) {
    // VAMPIRE SURVIVORS STYLE: Spawn SINGLE asteroid continuously
    // High spawn rate creates overwhelming swarm

    // Random angle around Earth (full 360 degrees)
    float spawnAngle = (rand() % 1000) / 1000.0f * 6.28318f;

    // Slight distance variation (creates depth)
    float distanceVariation = ((rand() % 1000) / 1000.0f - 0.5f) * 8.0f;  // ±4 units
    float actualSpawnDistance = spawnDistance + distanceVariation;

    // Spawn position
    Vec3 spawnPos;
    spawnPos.x = earthPos.x + cosf(spawnAngle) * actualSpawnDistance;
    spawnPos.y = earthPos.y + sinf(spawnAngle) * actualSpawnDistance;
    spawnPos.z = 0.0f;

    // Random speed toward Earth
    float speed = minSpeed + (rand() % 1000) / 1000.0f * (maxSpeed - minSpeed);

    // Direction toward Earth (with slight angular drift)
    Vec3 toEarth = earthPos - spawnPos;
    float distance = sqrtf(toEarth.x * toEarth.x + toEarth.y * toEarth.y + toEarth.z * toEarth.z);
    toEarth.x /= distance;
    toEarth.y /= distance;
    toEarth.z /= distance;

    // Add slight drift (makes movement less predictable)
    float driftAngle = ((rand() % 1000) / 1000.0f - 0.5f) * 0.4f;  // ±0.2 radians
    Vec3 perpendicular;
    perpendicular.x = -toEarth.y;
    perpendicular.y = toEarth.x;
    perpendicular.z = 0.0f;

    Vec3 velocity;
    velocity.x = (toEarth.x + perpendicular.x * driftAngle) * speed;
    velocity.y = (toEarth.y + perpendicular.y * driftAngle) * speed;
    velocity.z = 0.0f;

    // Find free slot and spawn
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) {
            asteroids[i].active = true;
            asteroids[i].isFragment = false;

            asteroids[i].position = spawnPos;

            // Size and mass (varied)
            asteroids[i].size = minSize + (rand() % 1000) / 1000.0f * (maxSize - minSize);
            asteroids[i].mass = asteroids[i].size * asteroids[i].size * asteroids[i].size * 4.0f;

            asteroids[i].velocity = velocity;

            // Rotation
            asteroids[i].rotationSpeed = -50.0f + (rand() % 1000) / 1000.0f * 100.0f;
            asteroids[i].currentRotation = (rand() % 360);

            // Fragment properties (unused for full asteroids)
            asteroids[i].lifetime = 0.0f;
            asteroids[i].maxLifetime = 0.0f;
            asteroids[i].alpha = 1.0f;

            // Color variation (yellow to red-orange)
            float colorVariation = (rand() % 1000) / 1000.0f;

            if (colorVariation < 0.33f) {
                // Yellow
                asteroids[i].r = 1.0f;
                asteroids[i].g = 0.8f + colorVariation * 0.2f;
                asteroids[i].b = 0.0f;
            }
            else if (colorVariation < 0.66f) {
                // Orange
                asteroids[i].r = 1.0f;
                asteroids[i].g = 0.4f + (colorVariation - 0.33f) * 0.6f;
                asteroids[i].b = 0.0f;
            }
            else {
                // Red-orange
                asteroids[i].r = 1.0f;
                asteroids[i].g = 0.2f + (colorVariation - 0.66f) * 0.3f;
                asteroids[i].b = 0.0f;
            }

            return;  // Successfully spawned
        }
    }

    // Pool full - this is expected with high spawn rate
}

void AsteroidSystem::BreakAsteroidIntoFragments(int index, const Vec3& impactPoint, const Vec3& impactVel) {
    Asteroid& parent = asteroids[index];

    printf("💥 Breaking asteroid at (%.1f, %.1f) size=%.2f into %d fragments\n",
        parent.position.x, parent.position.y, parent.size, fragmentCount);

    // Calculate fragment size (each piece is smaller)
    float fragmentSize = parent.size * 0.4f;  // 40% of original size

    // Parent asteroid is destroyed
    parent.active = false;

    // Create fragments
    int created = 0;
    for (int f = 0; f < fragmentCount; f++) {
        // Find free slot
        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (!asteroids[i].active) {
                asteroids[i].active = true;
                asteroids[i].isFragment = true;

                // Position: slightly offset from parent
                float offsetAngle = (f / (float)fragmentCount) * 6.28318f + (rand() % 100) / 500.0f;
                float offsetDist = parent.size * 0.3f;
                asteroids[i].position.x = parent.position.x + cosf(offsetAngle) * offsetDist;
                asteroids[i].position.y = parent.position.y + sinf(offsetAngle) * offsetDist;
                asteroids[i].position.z = parent.position.z;

                // Size and mass
                asteroids[i].size = fragmentSize + (rand() % 100) / 500.0f;  // Slight variation
                asteroids[i].mass = asteroids[i].size * asteroids[i].size * asteroids[i].size * 2.0f;

                // Velocity: parent velocity + explosion direction
                Vec3 explosionDir;
                explosionDir.x = cosf(offsetAngle);
                explosionDir.y = sinf(offsetAngle);
                explosionDir.z = 0.0f;

                float explosionMagnitude = fragmentSpeed * (0.8f + (rand() % 100) / 250.0f);  // 80-120%

                asteroids[i].velocity.x = parent.velocity.x + explosionDir.x * explosionMagnitude;
                asteroids[i].velocity.y = parent.velocity.y + explosionDir.y * explosionMagnitude;
                asteroids[i].velocity.z = 0.0f;

                // Rotation (faster spin for fragments)
                asteroids[i].rotationSpeed = -150.0f + (rand() % 1000) / 1000.0f * 300.0f;
                asteroids[i].currentRotation = (rand() % 360);

                // Fragment lifetime
                asteroids[i].lifetime = 0.0f;
                asteroids[i].maxLifetime = 2.0f + (rand() % 100) / 100.0f;  // 2-3 seconds
                asteroids[i].alpha = 1.0f;

                float grayValue = 0.3f + (rand() % 100) / 500.0f;  // 0.3 - 0.5 (dark gray)
                asteroids[i].r = grayValue;
                asteroids[i].g = grayValue;
                asteroids[i].b = grayValue;

                created++;
                break;
            }
        }
    }
}

void AsteroidSystem::UpdateFragment(int index, float deltaTime) {
    Asteroid& frag = asteroids[index];

    frag.lifetime += deltaTime;

    // Calculate fade alpha (linear fade in last 1 second)
    if (frag.lifetime > frag.maxLifetime - 1.0f) {
        frag.alpha = (frag.maxLifetime - frag.lifetime);  // 1.0 → 0.0
        if (frag.alpha < 0.0f) frag.alpha = 0.0f;
    }

    // Delete when invisible
    if (frag.lifetime >= frag.maxLifetime) {
        frag.active = false;
    }
}

void AsteroidSystem::GetGridCell(const Vec3 & pos, int& cellX, int& cellY) {
    cellX = (int)((pos.x + 50.0f) / cellSize);  // Offset to handle negative coords
    cellY = (int)((pos.y + 50.0f) / cellSize);

    // Clamp to grid bounds
    if (cellX < 0) cellX = 0;
    if (cellX >= GRID_SIZE) cellX = GRID_SIZE - 1;
    if (cellY < 0) cellY = 0;
    if (cellY >= GRID_SIZE) cellY = GRID_SIZE - 1;
}

void AsteroidSystem::UpdateSpatialGrid() {
    // Clear grid
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            spatialGrid[x][y].clear();
        }
    }

    // Add asteroids to grid cells
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) continue;
        if (asteroids[i].isFragment) continue;  // Skip fragments for collision

        int cellX, cellY;
        GetGridCell(asteroids[i].position, cellX, cellY);
        spatialGrid[cellX][cellY].push_back(i);
    }
}

void AsteroidSystem::GetNearbyAsteroids(int cellX, int cellY, std::vector<int>&nearby) {
    nearby.clear();

    // Check 3x3 grid around cell (current + 8 neighbors)
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

void AsteroidSystem::Update(float deltaTime, const Vec3 & earthPos) {
    float dt = deltaTime / 1000.0f;

    // Spawn timer
    spawnTimer += dt;
    if (spawnTimer >= spawnInterval) {
        SpawnAsteroid(earthPos);
        spawnTimer = 0.0f;
    }

    // Define zones
    const float VISIBLE_RADIUS_SQ = 4900.0f;   // 70^2 - visible area
    const float CLEANUP_RADIUS_SQ = 22500.0f;  // 150^2 - deletion boundary

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) continue;

        // Fragment update
        if (asteroids[i].isFragment) {
            UpdateFragment(i, dt);
        }

        // Always update position
        asteroids[i].position.x += asteroids[i].velocity.x * dt;
        asteroids[i].position.y += asteroids[i].velocity.y * dt;

        // Calculate distance from origin
        float distSq = asteroids[i].position.x * asteroids[i].position.x +
            asteroids[i].position.y * asteroids[i].position.y;

        // === ZONE-BASED PROCESSING ===

        if (distSq < VISIBLE_RADIUS_SQ) {
            // ZONE 1: VISIBLE - Full update
            asteroids[i].currentRotation += asteroids[i].rotationSpeed * dt;
            if (asteroids[i].currentRotation > 360.0f) asteroids[i].currentRotation -= 360.0f;

            // Earth collision check
            if (!asteroids[i].isFragment) {
                float distToEarthSq = (asteroids[i].position.x - earthPos.x) * (asteroids[i].position.x - earthPos.x) +
                    (asteroids[i].position.y - earthPos.y) * (asteroids[i].position.y - earthPos.y);
                if (distToEarthSq < 72.25f) {
                    asteroids[i].active = false;
                    continue;
                }
            }
        }
        else if (distSq < CLEANUP_RADIUS_SQ) {
            // ZONE 2: OFF-SCREEN but still alive
            // Minimal update (position only - already done above)
            // Check if moving away
            float dotProduct = asteroids[i].position.x * asteroids[i].velocity.x +
                asteroids[i].position.y * asteroids[i].velocity.y;

            if (dotProduct > 0) {
                // Moving away from center - will never return
                asteroids[i].active = false;
                continue;
            }
            // If moving toward center, keep it (might come back)
        }
        else {
            // ZONE 3: VERY FAR - Delete
            asteroids[i].active = false;
            continue;
        }
    }

    // Collision detection only for visible asteroids
    UpdateSpatialGrid();

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active || asteroids[i].isFragment) continue;

        // Only process collisions if in visible zone
        float distSq = asteroids[i].position.x * asteroids[i].position.x +
            asteroids[i].position.y * asteroids[i].position.y;
        if (distSq > VISIBLE_RADIUS_SQ) continue;

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

            float minDistSq = (asteroids[i].size + asteroids[j].size) * (asteroids[i].size + asteroids[j].size);

            if (distSq < minDistSq && distSq > 0.001f) {
                ResolveCollision(i, j);
            }
        }
    }
}

int AsteroidSystem::FindFreeSlot() {
    // Start search from last known free index
    for (int i = nextFreeIndex; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) {
            nextFreeIndex = i + 1;
            return i;
        }
    }

    // Wrap around if needed
    for (int i = 0; i < nextFreeIndex; i++) {
        if (!asteroids[i].active) {
            nextFreeIndex = i + 1;
            return i;
        }
    }

    return -1;  // Pool full
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

    float overlap = (a1.size + a2.size) - distance;
    float separationRatio = overlap / 2.0f;

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

    float totalMass = a1.mass + a2.mass;
    float impulse = -(1.0f + restitution) * velAlongNormal / totalMass;

    Vec3 impulseVec;
    impulseVec.x = impulse * normal.x;
    impulseVec.y = impulse * normal.y;
    impulseVec.z = impulse * normal.z;

    a1.velocity.x -= impulseVec.x * a2.mass;
    a1.velocity.y -= impulseVec.y * a2.mass;
    a2.velocity.x += impulseVec.x * a1.mass;
    a2.velocity.y += impulseVec.y * a1.mass;
}

int AsteroidSystem::CheckBulletCollision(const Vec3& bulletPos, const Vec3& bulletVel, float bulletRadius) {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) continue;

        // Fragments don't get hit by bullets (too small, already breaking apart)
        if (asteroids[i].isFragment) continue;

        float dx = asteroids[i].position.x - bulletPos.x;
        float dy = asteroids[i].position.y - bulletPos.y;
        float dz = asteroids[i].position.z - bulletPos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        float collisionRadius = asteroids[i].size + bulletRadius;
        float collisionRadiusSq = collisionRadius * collisionRadius;

        if (distSq < collisionRadiusSq) {
            // HIT! Check if should fragment or destroy
            if (asteroids[i].size >= fragmentMinSize) {
                // Large asteroid - break into fragments
                BreakAsteroidIntoFragments(i, bulletPos, bulletVel);
            }
            else {
                // Small asteroid - direct destruction
                printf("💥 Small asteroid destroyed directly\n");
                asteroids[i].active = false;
            }
            return i;
        }
    }

    return -1;
}

void AsteroidSystem::CheckPlayerCollision(const Vec3& playerPos, float playerRadius, const Vec3& playerVel) {
    int collisionCount = 0;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) continue;

        // Fragments don't collide with player (harmless debris)
        if (asteroids[i].isFragment) continue;

        // Distance to player
        float dx = asteroids[i].position.x - playerPos.x;
        float dy = asteroids[i].position.y - playerPos.y;
        float dz = asteroids[i].position.z - playerPos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        float collisionDist = asteroids[i].size + playerRadius;
        float collisionDistSq = collisionDist * collisionDist;

        if (distSq < collisionDistSq && distSq > 0.001f) {
            collisionCount++;

            float distance = sqrtf(distSq);

            // Normal pointing from player to asteroid
            Vec3 normal;
            normal.x = dx / distance;
            normal.y = dy / distance;
            normal.z = dz / distance;

            // Push asteroid away from player (separate overlap)
            float overlap = collisionDist - distance;
            asteroids[i].position.x += normal.x * overlap;
            asteroids[i].position.y += normal.y * overlap;
            asteroids[i].position.z += normal.z * overlap;

            // === PLAYER VELOCITY IMPACT ===

            // Player velocity magnitude
            float playerSpeed = sqrtf(playerVel.x * playerVel.x +
                playerVel.y * playerVel.y +
                playerVel.z * playerVel.z);

            // Impact force based on player speed
            float playerImpactForce = 5.0f;
            float impactMagnitude = playerSpeed * playerImpactForce / asteroids[i].mass;

            // Direction of impact
            Vec3 playerVelNorm;
            if (playerSpeed > 0.1f) {
                playerVelNorm.x = playerVel.x / playerSpeed;
                playerVelNorm.y = playerVel.y / playerSpeed;
                playerVelNorm.z = playerVel.z / playerSpeed;
            }
            else {
                playerVelNorm = normal;
            }

            Vec3 impactDir;
            impactDir.x = playerVelNorm.x * 0.7f + normal.x * 0.3f;
            impactDir.y = playerVelNorm.y * 0.7f + normal.y * 0.3f;
            impactDir.z = playerVelNorm.z * 0.7f + normal.z * 0.3f;

            // Normalize impact direction
            float impactDirLen = sqrtf(impactDir.x * impactDir.x +
                impactDir.y * impactDir.y +
                impactDir.z * impactDir.z);
            if (impactDirLen > 0.001f) {
                impactDir.x /= impactDirLen;
                impactDir.y /= impactDirLen;
                impactDir.z /= impactDirLen;
            }

            // Apply player velocity impact
            asteroids[i].velocity.x += impactDir.x * impactMagnitude;
            asteroids[i].velocity.y += impactDir.y * impactMagnitude;
            asteroids[i].velocity.z += impactDir.z * impactMagnitude;

            // === BOUNCE (collision response) ===

            // Relative velocity
            Vec3 relVel;
            relVel.x = asteroids[i].velocity.x - playerVel.x;
            relVel.y = asteroids[i].velocity.y - playerVel.y;
            relVel.z = asteroids[i].velocity.z - playerVel.z;

            float velDotNormal = relVel.x * normal.x +
                relVel.y * normal.y +
                relVel.z * normal.z;

            // Only reflect if moving toward player
            if (velDotNormal < 0) {
                float reflectionFactor = -(1.0f + restitution) * velDotNormal;

                asteroids[i].velocity.x += reflectionFactor * normal.x;
                asteroids[i].velocity.y += reflectionFactor * normal.y;
                asteroids[i].velocity.z += reflectionFactor * normal.z;
            }

            printf("⚡ PLAYER COLLISION! PlayerSpeed=%.1f, AsteroidVel=(%.1f, %.1f)\n",
                playerSpeed, asteroids[i].velocity.x, asteroids[i].velocity.y);
        }
    }

    if (collisionCount > 0) {
        printf("🛡️ Player pushed %d asteroids!\n", collisionCount);
    }
}

void AsteroidSystem::Render(const Camera3D& camera) {
    // Frustum culling bounds (adjust based on your camera)
    float visibleMinX = -60.0f;
    float visibleMaxX = 60.0f;
    float visibleMinY = -60.0f;
    float visibleMaxY = 60.0f;

    int culled = 0;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) continue;

        // Quick bounds check
        if (asteroids[i].position.x < visibleMinX || asteroids[i].position.x > visibleMaxX ||
            asteroids[i].position.y < visibleMinY || asteroids[i].position.y > visibleMaxY) {
            culled++;
            continue;  // Skip rendering
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

        Renderer3D::DrawMesh(asteroidMesh,
            asteroids[i].position,
            rotation,
            Vec3(asteroids[i].size, asteroids[i].size, asteroids[i].size),
            camera,
            r, g, b,
            false);
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
}
