//------------------------------------------------------------------------
// Planet.cpp - Persistent planet system
//------------------------------------------------------------------------
#include "Planet.h"
#include "../Utilities/Logger.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include "../Utilities/WorldText3D.h"

PlanetSystem::PlanetSystem()
    : lastPlayerPos(0, 0, 0)
{
    for (int i = 0; i < MAX_PLANETS; i++) {
        planets[i].active = false;
    }
}

void PlanetSystem::GeneratePlanetName(int sectorX, int sectorY, char* outName, int maxLength) {
    // Arrays of name parts for procedural generation
    const char* prefixes[] = {
        "Alpha", "Beta", "Gamma", "Delta", "Epsilon",
        "Zeta", "Theta", "Nova", "Terra", "Xeno",
        "Kepler", "Proxima", "Vega", "Sirius", "Rigel"
    };

    const char* suffixes[] = {
        "Prime", "Secundus", "Minor", "Major", "Centauri",
        "VII", "IX", "Omega", "Tertius", "Nexus"
    };

    int numPrefixes = sizeof(prefixes) / sizeof(prefixes[0]);
    int numSuffixes = sizeof(suffixes) / sizeof(suffixes[0]);

    // Use sector coords as seed for consistent names per sector
    int seed = sectorX * 73856093 ^ sectorY * 19349663;

    // Select prefix and suffix based on seed
    int prefixIdx = (seed & 0xFFFF) % numPrefixes;
    int suffixIdx = ((seed >> 16) & 0xFFFF) % numSuffixes;

    // Generate a number based on coordinates
    int number = abs(sectorX * 10 + sectorY);

    // Format the name
    snprintf(outName, maxLength, "%s-%s %d",
        prefixes[prefixIdx],
        suffixes[suffixIdx],
        number);
}

void PlanetSystem::DrawResourceGauge(const Planet& planet, const Camera3D& camera) {
    Vec3 camPos = camera.GetPosition();

    // Check distance - only show within 30 units
    float dx = planet.position.x - camPos.x;
    float dy = planet.position.y - camPos.y;
    float distSq = dx * dx + dy * dy;

    if (distSq > 2500.0f) return;  // 50*50 = 2500

    // Define resources with colors and angles (like speedometer positions)
    struct ResourceInfo {
        const char* label;
        float value;
        float r, g, b;
        float angle;  // Angle in radians (0 = right, -PI/2 = top, etc.)
    };

    // Arrange in top-right quadrant (angles from -45° to +45° from top-right)
    // Think of it like a speedometer arc from 10 o'clock to 2 o'clock
    ResourceInfo resources[4] = {
        { "Water",  planet.waterLevel,  0.3f, 0.6f, 1.0f, -0.7854f },   // ~45° up-right (10:30 position)
        { "Energy", planet.energyLevel, 1.0f, 1.0f, 0.3f, -0.3927f },   // ~22° up-right (11:30 position)
        { "Carbon", planet.carbonLevel, 0.3f, 1.0f, 0.3f,  0.0f },      // 0° right (12:00 position)
        { "Iron",   planet.ironLevel,   1.0f, 0.3f, 0.3f,  0.3927f }    // ~22° down-right (1:30 position)
    };

    // Distance from planet center for text
    float textRadius = planet.size + 2.0f;  // units beyond planet surface

    for (int i = 0; i < 4; i++) {
        // Calculate position along circumference
        Vec3 textPos = planet.position;
        textPos.x += cosf(resources[i].angle) * textRadius;
        textPos.y += sinf(resources[i].angle) * textRadius;

        // Format: "Water: 85"
        char labelBuffer[32];
        snprintf(labelBuffer, sizeof(labelBuffer), "%s: %.0f",
            resources[i].label, resources[i].value);

        // Draw label using WorldText3D
        WorldText3D::Print(textPos, labelBuffer, camera,
            resources[i].r, resources[i].g, resources[i].b,
            GLUT_BITMAP_HELVETICA_10);

        // Draw resource bar below the label
        DrawResourceBar(textPos, resources[i].value, camera,
            resources[i].r, resources[i].g, resources[i].b);
    }
}

void PlanetSystem::DrawResourceBar(const Vec3& worldPos, float value,
    const Camera3D& camera,
    float r, float g, float b) {
    // Bar position slightly below the text
    Vec3 barPos = worldPos;
    barPos.y -= 0.6f;  // Closer to text

    // Convert to NDC for screen-space bar drawing
    Vec4 ndc = camera.WorldToNDC(barPos);

    // Check if on screen
    if (ndc.w < camera.GetNearPlane()) return;
    if (ndc.x < -1.5f || ndc.x > 1.5f || ndc.y < -1.5f || ndc.y > 1.5f) return;

    // Convert to screen coordinates
    float screenX = (ndc.x + 1.0f) * (APP_VIRTUAL_WIDTH * 0.5f);
    float screenY = (ndc.y + 1.0f) * (APP_VIRTUAL_HEIGHT * 0.5f);

    // Bar dimensions (THINNER)
    float barWidth = 60.0f;
    float barHeight = 3.0f;  // CHANGED: Thinner (was 6.0f)
    float fillWidth = (value / 100.0f) * barWidth;

    // Draw filled portion only (NO BACKGROUND)
    for (float y = 0; y < barHeight; y++) {
        App::DrawLine(screenX, screenY + y,
            screenX + fillWidth, screenY + y,
            r, g, b);
    }

    // Draw thin border outline
    //App::DrawLine(screenX, screenY, screenX + barWidth, screenY, 0.4f, 0.4f, 0.4f);  // Bottom
    App::DrawLine(screenX, screenY + barHeight, screenX + barWidth, screenY + barHeight, 0.4f, 0.4f, 0.4f);  // Top
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

void PlanetSystem::WorldToSector(const Vec3& worldPos, int& sectorX, int& sectorY) const {
    const_cast<PlanetSystem*>(this)->GetSectorCoords(worldPos, sectorX, sectorY);
}

bool PlanetSystem::HasPlanetInSector(int sectorX, int sectorY) const {
    // Check if any active planet is in this sector
    for (int i = 0; i < MAX_PLANETS; i++) {
        if (planets[i].active &&
            planets[i].sectorX == sectorX &&
            planets[i].sectorY == sectorY) {
            return true;
        }
    }
    return false;
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

    // Initialize random resource levels
    planets[slot].waterLevel = 30.0f + (rand() % 1000) / 1000.0f * 70.0f;   // 30-100
    planets[slot].energyLevel = 30.0f + (rand() % 1000) / 1000.0f * 70.0f;  // 30-100
    planets[slot].carbonLevel = 30.0f + (rand() % 1000) / 1000.0f * 70.0f;  // 30-100
    planets[slot].ironLevel = 30.0f + (rand() % 1000) / 1000.0f * 70.0f;    // 30-100

    GeneratePlanetName(sectorX, sectorY, planets[slot].name, sizeof(planets[slot].name));

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

        // Slowly degenerate resources (random per second)
        if (planets[i].waterLevel > 0.0f) {
            planets[i].waterLevel -= 2.0f * dt;
            if (planets[i].waterLevel > 100.0f) {
                planets[i].waterLevel = 100.0f;
            }
        }
        if (planets[i].ironLevel > 0.0f) {
            planets[i].ironLevel -= 1.0f * dt;
            if (planets[i].ironLevel > 100.0f) {
                planets[i].ironLevel = 100.0f;
            }
        }
        if (planets[i].energyLevel > 0.0f) {
            planets[i].energyLevel -= 1.5f * dt;
            if (planets[i].energyLevel > 100.0f) {
                planets[i].energyLevel = 100.0f;
            }
        }
        if (planets[i].carbonLevel > 0.0f) {
            planets[i].carbonLevel -= 2.5f * dt;
            if (planets[i].carbonLevel > 100.0f) {
                planets[i].carbonLevel = 100.0f;
            }
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

        // BRIGHT PALETTE based on resources [Replace existing color code]
        float avgResource = (planets[i].ironLevel + planets[i].waterLevel +
            planets[i].carbonLevel + planets[i].energyLevel) / 400.0f;

        float baseBright = 0.6f + avgResource * 0.4f;  // 0.6?1.0 (bright!)

        float r, g, b;
        // Hue cycle for variety (bright rainbow)
        float hue = avgResource * 300.0f;  // 0?300 degrees
        float h = fmodf(hue / 60.0f, 6.0f);
        float s = 0.8f + avgResource * 0.2f;
        float v = baseBright;

        // HSV ? RGB (bright colors)
        float c = v * s;
        float x = c * (1.0f - fabsf(fmodf(h, 2.0f) - 1.0f));
        float m = v - c;

        if (h <= 1.0f) { r = c; g = x; b = 0; }
        else if (h <= 2.0f) { r = x; g = c; b = 0; }
        else if (h <= 3.0f) { r = 0; g = c; b = x; }
        else if (h <= 4.0f) { r = 0; g = x; b = c; }
        else if (h <= 5.0f) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }

        r += m; g += m; b += m;

        Renderer3D::DrawMesh(planetMesh, planets[i].position, rotation,
            Vec3(planets[i].size, planets[i].size, planets[i].size),
            camera, r, g, b, false);

        float labelOffset = -(planets[i].size + 3.0f);  // Negative = below

        WorldText3D::PrintWithOffset(planets[i].position,
            labelOffset,
            planets[i].name,  // Use stored name
            camera,
            1.0f, 1.0f, 1.0f,
            GLUT_BITMAP_HELVETICA_12);

        DrawResourceGauge(planets[i], camera);
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
