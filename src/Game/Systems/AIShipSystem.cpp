//-----------------------------------------------------------------------------
// AIShipSystem.cpp - Planet-based AI towing ships
//-----------------------------------------------------------------------------
#include "AIShipSystem.h"
#include "../Utilities/Logger.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

AIShipSystem::AIShipSystem()
{
    for (int i = 0; i < MAX_AI_SHIPS; i++)
        ships[i].active = false;
}

void AIShipSystem::Init()
{
    Logger::LogInfo("AIShipSystem: Initialized");
}

void AIShipSystem::Update(float deltaTime, PlanetSystem& planetSystem, AsteroidSystem& asteroidSystem)
{
    float dt = deltaTime / 1000.0f;

    // Ensure ships exist for currently active planets
    SpawnForActivePlanets(planetSystem);

    for (int i = 0; i < 20; i++) {
        if (frozenPlanets[i].freezeTimer > 0.0f) {
            frozenPlanets[i].freezeTimer -= dt;
        }
    }

    for (int i = 0; i < MAX_AI_SHIPS; i++)
    {
        if (!ships[i].active) continue;

        if (ships[i].active && ships[i].hitComboTimer > 0.0f) {
            ships[i].hitComboTimer -= dt;
            if (ships[i].hitComboTimer <= 0.0f) {
                ships[i].consecutiveHits = 0;  // Reset combo if no hits in 2s
            }
        }

        Vec3 shipPush(0, 0, 0);
        asteroidSystem.ResolveExternalCollision(
            ships[i].position,
            ships[i].radius,  // 1.2f matches AIShip.radius
            ships[i].velocity,
            shipPush
        );

        if (shipPush.LengthSquared() > 0.01f) {
            ships[i].position.x += shipPush.x;
            ships[i].position.y += shipPush.y;
            ships[i].position.z += shipPush.z;
            // Optional: reflect ship velocity like asteroids do
        }

        UpdateAI(i, ships[i], dt, planetSystem, asteroidSystem);
    }
}

void AIShipSystem::Render(const Camera3D& camera, AsteroidSystem& asteroidSystem, PlanetSystem& planetSystem)
{
    Mesh3D shipMesh = Mesh3D::CreateCube(1.0f);

    for (int i = 0; i < MAX_AI_SHIPS; i++)
    {
        if (!ships[i].active) continue;

        AIShip& ship = ships[i];

        float facingAngleDeg = 0.0f;
        if (ship.velocity.LengthSquared() > 0.01f)
        {
            Vec3 forward = ship.velocity.Normalized();
            float angleRad = atan2f(forward.y, forward.x);  // Radians (-PI to PI)
            facingAngleDeg = angleRad * (180.0f / 3.14159265359f);  // Convert to DEGREES
            facingAngleDeg = fmodf(facingAngleDeg + 360.0f, 360.0f); // Normalize 0-360
        }
        else
        {
            facingAngleDeg = fmodf(ship.position.Length() * 3.0f, 360.0f);  // Idle spin
        }
        Vec3 shipRot(0, 0, facingAngleDeg - 90.0f);  // Degrees

        // 3D MESH SHIP (player scale + rotation)
        Vec3 shipScale(1.0, 2.0f, 1.0f);  // Match player size

        // Draw with Mesh3D (matches your Planet/Asteroid rendering style)
        Renderer3D::DrawMesh(shipMesh, ship.position, shipRot, shipScale,
            camera, ship.r, ship.g, ship.b, false);

        // Towing ropes/highlights
        ship.towing.Render(camera, &asteroidSystem, &planetSystem);
    }
}

void AIShipSystem::SpawnForActivePlanets(PlanetSystem& planetSystem)
{
    const Planet* planets = planetSystem.GetPlanets();
    int maxPlanets = planetSystem.GetMaxPlanets();

    for (int p = 0; p < maxPlanets; p++)
    {
        if (!planets[p].active) continue;

        // SKIP HOME PLANET (DOUBLE CHECK!)
        //if (planets[p].isHomePlanet) continue;

        // PER-PLANET DESPERATION (not global average!)
        float planetAvg = (planets[p].ironLevel + planets[p].waterLevel +
            planets[p].carbonLevel + planets[p].energyLevel) / 400.0f;

        int targetShips = 3 + (int)((1.0f - planetAvg) * 4.0f);
        targetShips = std::clamp(targetShips, 3, 7);

        // Count EXISTING ships for THIS planet
        int currentShips = 0;
        for (int i = 0; i < MAX_AI_SHIPS; i++)
        {
            if (ships[i].active && ships[i].parentPlanetIndex == p)
                currentShips++;
        }

        // Spawn ONLY missing ships for THIS planet
        for (int needed = currentShips; needed < targetShips; needed++)
        {
            int slot = -1;
            for (int i = 0; i < MAX_AI_SHIPS; i++)
            {
                if (!ships[i].active) { slot = i; break; }
            }
            if (slot < 0) break;

            SpawnShip(slot, p, planetSystem);
        }
    }
}

int AIShipSystem::FindShipForPlanet(int planetIndex) const
{
    for (int i = 0; i < MAX_AI_SHIPS; i++)
    {
        if (ships[i].active && ships[i].parentPlanetIndex == planetIndex)
            return i;
    }
    return -1;
}

void AIShipSystem::SpawnShip(int slot, int planetIndex, const PlanetSystem& planetSystem)
{
    const Planet* planets = planetSystem.GetPlanets();
    const Planet& parent = planets[planetIndex];

    ships[slot].active = true;
    ships[slot].parentPlanetIndex = planetIndex;
    ships[slot].velocity = Vec3(0, 0, 0);
    ships[slot].targetAsteroidIndex = -1;

    // Orbit spawn position
    static float globalAngleOffset = 0.0f;
    float shipAngle = globalAngleOffset + ((rand() % 1000) / 1000.0f) * 1.57f;
    globalAngleOffset += 0.8f;
    globalAngleOffset = fmodf(globalAngleOffset, 6.283185f);
    float spawnDist = parent.size + 10.0f + ((rand() % 500) / 500.0f * 4.0f);
    ships[slot].position = parent.position + Vec3(cosf(shipAngle) * spawnDist,
        sinf(shipAngle) * spawnDist, 0.0f);

    ships[slot].r = parent.r;
    ships[slot].g = parent.g;
    ships[slot].b = parent.b;

    ships[slot].retargetTimer = 0.0f;
}

AsteroidMineralType AIShipSystem::ChooseDesiredMineral(const Planet& planet) const
{
    // Choose the lowest resource as the "most needed"
    float iron = planet.ironLevel;
    float water = planet.waterLevel;
    float carbon = planet.carbonLevel;
    float energy = planet.energyLevel;

    float minVal = iron;
    AsteroidMineralType best = MINERAL_IRON;

    if (water < minVal) { minVal = water; best = MINERAL_WATER; }
    if (carbon < minVal) { minVal = carbon; best = MINERAL_CARBON; }
    if (energy < minVal) { minVal = energy; best = MINERAL_ENERGY; }

    return best;
}

int AIShipSystem::FindBestAsteroidTarget(const AIShip& ship, const Planet& parent,
    AsteroidSystem& asteroidSystem) const
{
    const Asteroid* asteroids = asteroidSystem.GetAsteroids();
    int maxA = asteroidSystem.GetMaxAsteroids();

    float bestDistSq = 999999999.0f;
    int bestIdx = -1;

    for (int i = 0; i < maxA; i++)
    {
        const Asteroid& a = asteroids[i];
        if (!a.active) continue;
        if (a.isFragment) continue;
        if (a.isTowed) continue;              // Player towing
        if (a.claimedByAIShip != -1) continue; // AI claimed
        if (a.mineralType != ship.desiredMineral) continue;

        float dx = a.position.x - ship.position.x;
        float dy = a.position.y - ship.position.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            bestIdx = i;
        }
    }

    return bestIdx;
}

bool AIShipSystem::IsNear(const Vec3& a, const Vec3& b, float dist) const
{
    Vec3 d = b - a;
    return d.LengthSquared() <= dist * dist;
}

void AIShipSystem::ApplySteering(AIShip& ship, const Vec3& targetPos, float dt, float maxSpeed, float accel)
{
    Vec3 toTarget = targetPos - ship.position;
    float len = toTarget.Length();
    if (len < 0.001f) return;

    Vec3 desiredVel = toTarget * (1.0f / len);
    desiredVel = desiredVel * maxSpeed;

    Vec3 dv = desiredVel - ship.velocity;

    // Clamp acceleration
    float dvLen = dv.Length();
    if (dvLen > accel)
        dv = dv * (accel / dvLen);

    ship.velocity = ship.velocity + dv;
    ship.position = ship.position + ship.velocity * dt;
}

void AIShipSystem::OnShipHit(int shipIndex) {
    if (shipIndex < 0 || shipIndex >= MAX_AI_SHIPS) return;

    AIShip& ship = ships[shipIndex];
    if (!ship.active) return;

    ship.consecutiveHits++;
    ship.hitComboTimer = 2.0f;  // 2s window for combo

    Logger::LogFormat("Ship %d hit! Combo: %d", shipIndex, ship.consecutiveHits);

    // 3+ hits = FREEZE ENTIRE PLANET FLEET!
    if (ship.consecutiveHits >= 3) {
        FreezePlanetFleet(ship.parentPlanetIndex);
        ship.consecutiveHits = 0;  // Reset combo
    }
}

void AIShipSystem::ResolveCollisionsWithAsteroids(AsteroidSystem& asteroidSystem) {
    for (int i = 0; i < MAX_AI_SHIPS; i++) {
        if (!ships[i].active) continue;

        Vec3 shipPush(0, 0, 0);
        asteroidSystem.ResolveExternalCollision(
            ships[i].position,
            ships[i].radius,  // Exactly 1.2f like player
            ships[i].velocity,
            shipPush
        );

        if (shipPush.LengthSquared() > 0.01f) {
            ships[i].position.x += shipPush.x;
            ships[i].position.y += shipPush.y;
            ships[i].position.z += shipPush.z;
            // Asteroids auto-reflect velocity via ResolveExternalCollision
        }
    }
}

void AIShipSystem::FreezePlanetFleet(int planetIndex) {
    // Find freeze slot
    int slot = -1;
    for (int i = 0; i < 20; i++) {
        if (frozenPlanets[i].planetIndex == planetIndex) {
            slot = i;
            break;
        }
        if (frozenPlanets[i].freezeTimer <= 0.0f) {
            slot = i;
            break;
        }
    }

    if (slot >= 0) {
        frozenPlanets[slot].planetIndex = planetIndex;
        frozenPlanets[slot].freezeTimer = 3.0f;  // 3 second freeze
        Logger::LogFormat("Planet %d fleet FROZEN for 3s!", planetIndex);
    }
}

bool AIShipSystem::IsPlanetFrozen(int planetIndex) const {
    for (int i = 0; i < 20; i++) {
        if (frozenPlanets[i].planetIndex == planetIndex &&
            frozenPlanets[i].freezeTimer > 0.0f) {
            return true;
        }
    }
    return false;
}

void AIShipSystem::UpdateAI(int shipIndex, AIShip& ship, float dt, PlanetSystem& planetSystem, AsteroidSystem& asteroidSystem)
{
    if (IsPlanetFrozen(ship.parentPlanetIndex)) {
        if (ship.targetAsteroidIndex >= 0) {
            Asteroid* ast = &asteroidSystem.GetAsteroids()[ship.targetAsteroidIndex];
            ast->velocity = Vec3(0, 0, 0);  // Stop movement
        }
        return;  // Skip all AI: no movement, targeting, or towing during freeze
    }

    Planet* planets = planetSystem.GetPlanets();
    if (ship.parentPlanetIndex < 0) return;

    Planet& parent = planets[ship.parentPlanetIndex];
    if (!parent.active)
    {
        ship.active = false;
        return;
    }

    // CRITICAL: PAUSE TARGETING WHILE TOWING
    if (ship.towing.IsTowing())
    {
        Vec3 astPos = asteroidSystem.GetAsteroids()[ship.targetAsteroidIndex].position;
        float ropeDistSq = (ship.position - astPos).LengthSquared();

        if (ropeDistSq > 2500.0f)  // >50 units away = skip physics (like planets) [file:10]
        {
            // Just fly to planet
            ApplySteering(ship, parent.position, dt, 16.0f, 0.7f);
            return;
        }

        ship.towing.Update(dt * 1000.0f, ship.position, ship.velocity, &asteroidSystem, &planetSystem);
        
        ApplySteering(ship, parent.position, dt, 16.0f, 0.7f);

        // Deposit only
        if (IsNear(ship.position, parent.position, parent.size + 6.0f))
        {
            ship.towing.TryDepositAtPlanet(ship.parentPlanetIndex, &asteroidSystem, &planetSystem);

            if (ship.targetAsteroidIndex >= 0)
            {
                // Release claim
                Asteroid* asteroids = asteroidSystem.GetAsteroids();
                asteroids[ship.targetAsteroidIndex].claimedByAIShip = -1;
            }
            ship.targetAsteroidIndex = -1;
        }

        return;  // NO TARGETING WHILE TOWING
    }

    // ONLY target/retarget when NOT towing
    ship.retargetTimer -= dt;
    if (ship.retargetTimer <= 0.0f)
    {
        ship.desiredMineral = ChooseDesiredMineral(parent);
        ship.retargetTimer = 1.2f;  // SLOWER retarget (2s)
    }

    // Find target (only when free)
    if (ship.targetAsteroidIndex < 0 || !asteroidSystem.GetAsteroids()[ship.targetAsteroidIndex].active)
    {
        ship.targetAsteroidIndex = FindBestAsteroidTarget(ship, parent, asteroidSystem);
    }

    if (ship.targetAsteroidIndex >= 0)
    {
        Asteroid* asteroids = asteroidSystem.GetAsteroids();
        Asteroid& targetAst = asteroids[ship.targetAsteroidIndex];

        // CLAIM + SAFETY CHECK
        if (targetAst.claimedByAIShip == -1)
        {
            targetAst.claimedByAIShip = shipIndex;
        }
        else if (targetAst.claimedByAIShip != shipIndex)
        {
            ship.targetAsteroidIndex = -1;  // Someone else claimed it
            return;
        }

        // Fly + grab
        ApplySteering(ship, targetAst.position, dt, 18.0f, 0.8f);

        if (IsNear(ship.position, targetAst.position, 5.0f))
        {
            ship.towing.TryGrabAsteroid(ship.targetAsteroidIndex, ship.position, &asteroidSystem);
        }
    }
    else
    {
        // Idle orbit
        float angle = fmodf(parent.currentRotation * 0.0174533f, 6.2831853f);
        Vec3 idlePos = parent.position + Vec3(cosf(angle) * (parent.size + 12.0f),
            sinf(angle) * (parent.size + 12.0f), 0.0f);
        ApplySteering(ship, idlePos, dt, 10.0f, 0.6f);
    }

    // Update towing (highlights only when not towing)
    ship.towing.Update(dt * 1000.0f, ship.position, ship.velocity, &asteroidSystem, &planetSystem);
}
