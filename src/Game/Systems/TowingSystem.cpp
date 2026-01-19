//-----------------------------------------------------------------------------
// TowingSystem.cpp - Asteroid towing implementation
//-----------------------------------------------------------------------------
#include "TowingSystem.h"
#include "../Utilities/Logger.h"
#include "../Utilities/WorldText3D.h"
#include "../ContestAPI/App.h"
#include <cmath>

TowingSystem::TowingSystem()
    : isTowing(false)
    , towedAsteroidIndex(-1)
    , highlightedAsteroidIndex(-1)
    , highlightedPlanetIndex(-1)
    , grabRange(5.0f)
    , ropeSlack(1.0f)
    , ropeSegments(2)
    , highlightPulse(0.0f)
{
}

void TowingSystem::Update(float deltaTime, const Vec3& playerPos, const Vec3& playerVel,
    AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem) {
    float dt = deltaTime / 1000.0f;

    // Update highlight pulse animation
    highlightPulse += dt * 3.0f;
    if (highlightPulse > 6.28318f) highlightPulse -= 6.28318f;

    // Find nearest interactables
    if (!isTowing) {
        highlightedAsteroidIndex = FindNearestAsteroid(playerPos, asteroidSystem);
        highlightedPlanetIndex = -1;
    }
    else {
        highlightedAsteroidIndex = -1;
        highlightedPlanetIndex = FindNearestPlanet(playerPos, planetSystem);
    }

    // Update rope physics if towing
    if (isTowing && towedAsteroidIndex >= 0) {
        Asteroid* ast = &asteroidSystem->GetAsteroids()[towedAsteroidIndex];

        if (ast->active) {
            // === CHANGED: Fixed-length rope (no stretching) ===
            float fixedRopeLength = 4.0f;  // Fixed distance
            Vec3 toAsteroid = ast->position - playerPos;
            float currentDistance = toAsteroid.Length();

            // If too far, pull asteroid to exact rope length
            if (currentDistance > fixedRopeLength) {
                Vec3 direction = toAsteroid.Normalized();
                Vec3 targetPos = playerPos + direction * fixedRopeLength;

                // Smoothly move asteroid to target position
                Vec3 correction = targetPos - ast->position;
                ast->position.x += correction.x * 0.5f;
                ast->position.y += correction.y * 0.5f;
                ast->position.z += correction.z * 0.5f;

                // Match velocity to player
                ast->velocity = playerVel * 0.9f;
            }
            else {
                // Within rope length, just match velocity smoothly
                Vec3 targetVel = playerVel * 0.9f;
                Vec3 velDiff = targetVel - ast->velocity;

                ast->velocity.x += velDiff.x * dt * 5.0f;
                ast->velocity.y += velDiff.y * dt * 5.0f;
                ast->velocity.z += velDiff.z * dt * 5.0f;
            }

            // Update rope anchor points
            rope.SetAnchorPoints(playerPos, ast->position);
            rope.Update(deltaTime);
        }
        else {
            // Asteroid was destroyed, release tow
            ReleaseAsteroid();
        }
    }
}

void TowingSystem::HandleInput(const Vec3& playerPos, AsteroidSystem* asteroidSystem,
    PlanetSystem* planetSystem) {
    // Check for 'E' key press
    if (!App::IsKeyPressed(App::KEY_E)) return;

    if (!isTowing) {
        // Try to grab nearest asteroid
        if (highlightedAsteroidIndex >= 0) {
            GrabAsteroid(highlightedAsteroidIndex, playerPos, asteroidSystem);
        }
    }
    else {
        // === CHANGED: Check if near planet first ===
        if (highlightedPlanetIndex >= 0) {
            // Near planet → deposit
            DepositAsteroid(highlightedPlanetIndex, asteroidSystem, planetSystem);
        }
        else {
            // Not near planet → manual release
            ReleaseAsteroid();
        }
    }
}

bool TowingSystem::TryGrabAsteroid(int asteroidIndex, const Vec3& shipPos, AsteroidSystem* asteroidSystem)
{
    if (isTowing) return false;
    if (!asteroidSystem) return false;
    GrabAsteroid(asteroidIndex, shipPos, asteroidSystem);
    return isTowing;
}

bool TowingSystem::TryDepositAtPlanet(int planetIndex, AsteroidSystem* asteroidSystem, PlanetSystem* planetSystem)
{
    if (!isTowing) return false;
    if (!asteroidSystem || !planetSystem) return false;
    DepositAsteroid(planetIndex, asteroidSystem, planetSystem);
    return true;
}

int TowingSystem::FindNearestAsteroid(const Vec3& playerPos, AsteroidSystem* asteroidSystem) {
    if (!asteroidSystem) return -1;

    int nearestIndex = -1;
    float nearestDistSq = grabRange * grabRange;

    Asteroid* asteroids = asteroidSystem->GetAsteroids();
    int maxAsteroids = asteroidSystem->GetMaxAsteroids();

    for (int i = 0; i < maxAsteroids; i++) {
        if (!asteroids[i].active) continue;
        if (asteroids[i].isFragment) continue;  // Can't grab fragments

        float dx = asteroids[i].position.x - playerPos.x;
        float dy = asteroids[i].position.y - playerPos.y;
        float dz = asteroids[i].position.z - playerPos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

int TowingSystem::FindNearestPlanet(const Vec3& playerPos, PlanetSystem* planetSystem) {
    if (!planetSystem) return -1;

    int nearestIndex = -1;
    float nearestDistSq = 999999.0f;

    const Planet* planets = planetSystem->GetPlanets();
    int maxPlanets = planetSystem->GetMaxPlanets();

    for (int i = 0; i < maxPlanets; i++) {
        if (!planets[i].active) continue;
        if (!planets[i].isHomePlanet) continue;

        float dx = planets[i].position.x - playerPos.x;
        float dy = planets[i].position.y - playerPos.y;
        float dz = planets[i].position.z - playerPos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        // Adjust for planet size
        float interactionDist = planets[i].size + grabRange;
        float interactionDistSq = interactionDist * interactionDist;

        if (distSq < interactionDistSq && distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

void TowingSystem::GrabAsteroid(int asteroidIndex, const Vec3& playerPos,
    AsteroidSystem* asteroidSystem) {
    Asteroid* ast = &asteroidSystem->GetAsteroids()[asteroidIndex];

    if (!ast->active) return;

    isTowing = true;
    towedAsteroidIndex = asteroidIndex;

    // Create rope with VERY stiff settings (minimal stretch)
    Vec3 asteroidPos = ast->position;

    rope.CreateRope(playerPos, asteroidPos, ropeSegments);
    rope.stiffness = 1.0f;  // MAXIMUM stiffness (no stretch)
    rope.damping = 0.99f;   // Very high damping
    rope.gravity = Vec3(0, -0.5f, 0);  // Very light gravity (minimal sag)
    rope.solverIterations = 5;  // More iterations = more rigid

    const char* mineralNames[] = { "Iron", "Water", "Carbon", "Energy" };
    Logger::LogFormat("Grabbed %s asteroid! Resources: %.0f\n",
        mineralNames[ast->mineralType], ast->resourceAmount);
}

void TowingSystem::DepositAsteroid(int planetIndex, AsteroidSystem* asteroidSystem,
    PlanetSystem* planetSystem) {
    if (!isTowing || towedAsteroidIndex < 0) return;

    Asteroid* ast = &asteroidSystem->GetAsteroids()[towedAsteroidIndex];
    Planet* planet = &planetSystem->GetPlanets()[planetIndex];

    if (!ast->active || !planet->active) return;

    // Much smaller resource amounts (1-10 range)
    // Asteroid sizes typically range from 0.5 to 2.0
    float sizeMultiplier = ast->size / 2.0f;  // Normalize (0.25 to 1.0)
    float resourceAmount = 10.0f + (sizeMultiplier * 50.0f);  // 1 to 10 range

    // Clamp to ensure 1-10 range
    if (resourceAmount < 1.0f) resourceAmount = 1.0f;
    if (resourceAmount > 10.0f) resourceAmount = 10.0f;

    // Transfer resources based on asteroid type
    switch (ast->mineralType) {
    case MINERAL_IRON:
        planet->ironLevel += resourceAmount;
        if (planet->ironLevel > 100.0f) planet->ironLevel = 100.0f;
        break;

    case MINERAL_WATER:
        planet->waterLevel += resourceAmount;
        if (planet->waterLevel > 100.0f) planet->waterLevel = 100.0f;
        break;

    case MINERAL_CARBON:
        planet->carbonLevel += resourceAmount;
        if (planet->carbonLevel > 100.0f) planet->carbonLevel = 100.0f;
        break;

    case MINERAL_ENERGY:
        planet->energyLevel += resourceAmount;
        if (planet->energyLevel > 100.0f) planet->energyLevel = 100.0f;
        break;
    }

    // Destroy asteroid (consumed by planet)
    asteroidSystem->DestroyAsteroid(towedAsteroidIndex);

    // Release tow
    ReleaseAsteroid();
}

void TowingSystem::ReleaseAsteroid() {
    isTowing = false;
    towedAsteroidIndex = -1;
    rope.Clear();
}

void TowingSystem::Render(const Camera3D& camera, AsteroidSystem* asteroidSystem,
    PlanetSystem* planetSystem) {
    // Render rope if towing
    if (isTowing && towedAsteroidIndex >= 0) {
        rope.Render(camera, 1.0f, 1.0f, 1.0f);  // Green rope color
    }

    // Render highlighted asteroid
    if (highlightedAsteroidIndex >= 0 && asteroidSystem) {
        Asteroid* ast = &asteroidSystem->GetAsteroids()[highlightedAsteroidIndex];
        if (ast->active) {
            RenderHighlight(ast->position, ast->size * 1.3f, camera, 0.3f, 0.3f, 0.3f);

            // === UPDATED: Show smaller resource value ===
            float sizeMultiplier = ast->size / 2.0f;
            float resourceValue = 1.0f + (sizeMultiplier * 9.0f);
            if (resourceValue < 1.0f) resourceValue = 1.0f;
            if (resourceValue > 10.0f) resourceValue = 10.0f;

            // Show resource amount and type
            const char* mineralNames[] = { "Iron", "Water", "Carbon", "Energy" };
            char infoText[64];
            sprintf(infoText, "[E] Grab - %s: +%.0f",
                mineralNames[ast->mineralType], resourceValue);

            RenderInteractionPrompt(ast->position, infoText, camera);
        }
    }


    // Render highlighted planet
    if (highlightedPlanetIndex >= 0 && planetSystem) {
        const Planet* planet = &planetSystem->GetPlanets()[highlightedPlanetIndex];
        if (planet->active && strcmp(planet->name, "Home Planet") == 0) {
            RenderHighlight(planet->position, planet->size * 1.2f, camera, 0.7f, 0.7f, 0.7f);

            // Show interaction prompt
            RenderInteractionPrompt(planet->position + Vec3(0.0f, planet->size, 0.0f), "[E] Deposit", camera);
        }
    }
}

void TowingSystem::RenderHighlight(const Vec3& position, float size,
    const Camera3D& camera, float r, float g, float b) {

    // Draw multiple wireframe spheres for thicker, more visible border
    Renderer3D::DrawSphere(position, size, camera, r, g, b, 1, true);
}

void TowingSystem::RenderInteractionPrompt(const Vec3& worldPos, const char* text,
    const Camera3D& camera) {
    // Show prompt above object
    WorldText3D::PrintWithOffset(worldPos, 3.0f, text, camera, 1.0f, 1.0f, 0.3f,
        GLUT_BITMAP_HELVETICA_12);
}
