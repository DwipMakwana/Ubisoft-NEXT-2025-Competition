//------------------------------------------------------------------------
// StarField.cpp - High-performance version
//------------------------------------------------------------------------
#include "StarField.h"
#include "../Utilities/Logger.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

StarField::StarField()
    : maxStars(800)            // REDUCED from 1200 for performance
    , spawnDistance(90.0f)     // Slightly smaller area
    , cullDistance(120.0f)     // Tighter culling
    , lastPlayerPos(0, 0, 0)
{
}

void StarField::Init(const Vec3& initialPos) {
    Logger::LogInfo("StarField: Initializing...");

    // Create star mesh
    starMesh = Mesh3D::CreateSphere(1.0f, 4);

    // Spawn initial stars
    SpawnStarsAroundPlayer(initialPos, maxStars);

    lastPlayerPos = initialPos;

    Logger::LogFormat("StarField initialized with %d stars", GetActiveStarCount());
}

float StarField::GetRandomFloat(float min, float max) {
    return min + (rand() % 1000) / 1000.0f * (max - min);
}

void StarField::SpawnStarsAroundPlayer(const Vec3& playerPos, int count) {
    for (int i = 0; i < count; i++) {
        Star star;

        // Random position
        float angle = GetRandomFloat(0.0f, 6.28318f);
        float distance = GetRandomFloat(30.0f, spawnDistance);

        star.position.x = playerPos.x + cosf(angle) * distance;
        star.position.y = playerPos.y + sinf(angle) * distance;
        star.position.z = GetRandomFloat(-40.0f, 40.0f);  // Reduced Z range

        star.brightness = GetRandomFloat(0.4f, 1.0f);
        star.size = GetRandomFloat(0.05f, 0.01f);  // Slightly smaller

        stars.push_back(star);
    }
}

void StarField::Update(const Vec3& playerPos) {
    // Calculate movement
    float dx = playerPos.x - lastPlayerPos.x;
    float dy = playerPos.y - lastPlayerPos.y;
    float movedDist = sqrtf(dx * dx + dy * dy);

    // OPTIMIZATION: Only update every 10 units (was 5)
    if (movedDist < 10.0f) {
        return;
    }

    // Cull distant stars (FAST: single-pass erase-remove)
    stars.erase(
        std::remove_if(stars.begin(), stars.end(),
            [&](const Star& star) {
                float dx = star.position.x - playerPos.x;
                float dy = star.position.y - playerPos.y;
                float distSq = dx * dx + dy * dy;
                return distSq > cullDistance * cullDistance;
            }),
        stars.end()
    );

    // Spawn new stars to maintain count
    int currentCount = GetActiveStarCount();
    int neededStars = maxStars - currentCount;

    if (neededStars > 0) {
        // Directional spawning ahead of movement
        float moveAngle = atan2f(dy, dx);

        for (int i = 0; i < neededStars; i++) {
            Star star;

            float angleVariation = GetRandomFloat(-1.0f, 1.0f);
            float spawnAngle = moveAngle + angleVariation;
            float distance = GetRandomFloat(60.0f, spawnDistance);

            star.position.x = playerPos.x + cosf(spawnAngle) * distance;
            star.position.y = playerPos.y + sinf(spawnAngle) * distance;
            star.position.z = GetRandomFloat(-40.0f, 40.0f);

            star.brightness = GetRandomFloat(0.4f, 1.0f);
            star.size = GetRandomFloat(0.01f, 0.05f);

            stars.push_back(star);
        }
    }

    lastPlayerPos = playerPos;
}

void StarField::Render(const Camera3D& camera) {
    Vec3 camPos = camera.GetPosition();

    // OPTIMIZATION 1: Pre-calculate visible range (screen is ~80x60 units at z=70)
    float visibleRangeX = 50.0f;  // Horizontal visible range
    float visibleRangeY = 40.0f;  // Vertical visible range

    int rendered = 0;
    int culled = 0;

    for (const auto& star : stars) {
        // OPTIMIZATION 2: Fast 2D culling (skip expensive 3D checks)
        float dx = star.position.x - camPos.x;
        float dy = star.position.y - camPos.y;

        // Quick bounds check (much faster than frustum culling)
        if (fabsf(dx) > visibleRangeX || fabsf(dy) > visibleRangeY) {
            culled++;
            continue;
        }

        // OPTIMIZATION 3: Skip stars very far in Z
        if (star.position.z < -60.0f || star.position.z > 60.0f) {
            culled++;
            continue;
        }

        float brightness = 1.0f;

        // Render star (only visible ones reach here)
        float b = star.brightness;
        Renderer3D::DrawMesh(
            starMesh,
            star.position,
            Vec3(0, 0, 0),
            Vec3(star.size, star.size, star.size),
            camera,
            brightness, brightness, brightness,
            true
        );
        rendered++;
    }

    // Minimal logging
    static int frameCount = 0;
    if (++frameCount % 300 == 0) {  // Every 5 seconds
        Logger::LogFormat("Stars: %d rendered, %d culled (%.1f%%)",
            rendered, culled,
            (culled * 100.0f) / (rendered + culled));
    }
}
