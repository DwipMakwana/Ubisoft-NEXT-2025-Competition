#include "AIPlayerSystem.h"
#include "../Utilities/Logger.h"
#include "App.h"
#include <cmath>
#include <algorithm>

AIPlayerSystem::AIPlayerSystem() {
    for (int i = 0; i < MAX_AIPLAYERS; i++) players[i].active = false;
}

void AIPlayerSystem::Init(PlanetSystem& planetSystem) {
    // Clear existing
    for (int i = 0; i < MAX_AIPLAYERS; i++) {
        players[i].active = false;
        players[i].shootCooldown = 0.0f;
    }
}

void AIPlayerSystem::Update(float deltaTime, PlanetSystem& planetSystem,
    AIShipSystem& enemyShips, BulletSystem& bullets)
{
    float dt = deltaTime / 1000.0f; // seconds

    SpawnForActivePlanets(planetSystem);

    for (int i = 0; i < MAX_AIPLAYERS; i++) {
        if (!players[i].active) continue;

        UpdateAI(i, dt, planetSystem, enemyShips, bullets); // pass seconds now

        // Drag (frame-rate independent-ish)
        players[i].velocity.x *= powf(players[i].drag, dt * 60.0f);
        players[i].velocity.y *= powf(players[i].drag, dt * 60.0f);

        // Clamp speed
        float speed = sqrtf(players[i].velocity.x * players[i].velocity.x +
            players[i].velocity.y * players[i].velocity.y);
        if (speed > players[i].maxSpeed && speed > 0.0001f) {
            players[i].velocity.x = (players[i].velocity.x / speed) * players[i].maxSpeed;
            players[i].velocity.y = (players[i].velocity.y / speed) * players[i].maxSpeed;
        }

        // Integrate with seconds
        players[i].position.x += players[i].velocity.x * dt;
        players[i].position.y += players[i].velocity.y * dt;
    }
}

void AIPlayerSystem::SpawnForActivePlanets(PlanetSystem& planetSystem) {
    const Planet* planets = planetSystem.GetPlanets();
    int maxPlanets = planetSystem.GetMaxPlanets();

    for (int p = 0; p < maxPlanets; p++) {
        if (!planets[p].active || planets[p].isHomePlanet) continue;  // SKIP HOME!

        // 1-3 guards per planet based on desperation
        float planetAvg = (planets[p].ironLevel + planets[p].waterLevel +
            planets[p].carbonLevel + planets[p].energyLevel) / 400.0f;
        int targetGuards = 1 + int(2.0f * (1.0f - planetAvg));
        targetGuards = std::clamp(targetGuards, 1, 3);

        // Count existing guards for this planet
        int currentGuards = 0;
        for (int i = 0; i < MAX_AIPLAYERS; i++) {
            if (players[i].active && players[i].homePlanetIndex == p)
                currentGuards++;
        }

        // Spawn missing guards
        for (int needed = 0; needed < targetGuards - currentGuards; needed++) {
            int slot = -1;
            for (int i = 0; i < MAX_AIPLAYERS; i++) {
                if (!players[i].active) { slot = i; break; }
            }
            if (slot == -1) break;

            // Planet color (computed like AIShips)
            float avgResource = planetAvg;
            float hue = avgResource * 300.0f;
            float h = fmodf(hue / 60.0f, 6.0f);
            float s = 0.8f - avgResource * 0.2f;
            float v = 0.6f + avgResource * 0.4f;
            // HSV → RGB (copy from AIShipSystem::SpawnShip)
            float c = v * s, x = c * (1 - fabsf(fmodf(h, 2.0f) - 1));
            float m = v - c;
            // ... set players[slot].r,g,b ...

            // Bottom spawn around THIS planet
            float angle = (needed * 2.0944f) + 4.71239f;  // 120° spacing, bottom
            float orbitDist = planets[p].size + 18.0f;
            players[slot].position = planets[p].position +
                Vec3(cosf(angle) * orbitDist, sinf(angle) * orbitDist, 0);

            players[slot].mesh = Mesh3D::CreatePyramid(2.0f, 3.0f);
            players[slot].homePlanetIndex = p;
            players[slot].active = true;

			players[slot].r = planets[p].r;
			players[slot].g = planets[p].g;
			players[slot].b = planets[p].b;
        }
    }
}

void AIPlayerSystem::UpdateAI(int index, float dt, PlanetSystem& planetSystem,
    AIShipSystem& enemyShips, BulletSystem& bullets) {
    AIPlayer& player = players[index];
    const Planet* planets = planetSystem.GetPlanets();
    const AIShip* ships = enemyShips.GetShips();

    if (player.targetShipIndex >= 0) {
        const AIShip& target = ships[player.targetShipIndex];

        // Aim
        Vec3 toTarget = target.position - player.position;
        float dist = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
        toTarget.x /= dist; toTarget.y /= dist;
        player.aimAngle = atan2f(toTarget.y, toTarget.x) * 180.0f / 3.14159f;

        // SHOOT LOGIC - BULLETSYSTEM.SHOOTBULLET
        if (player.shootCooldown <= 0.0f && dist < 120.0f) {  // Range check
            float aimRad = player.aimAngle * 3.14159f / 180.0f;
            Vec3 aimDir(cosf(aimRad), sinf(aimRad), 0.0f);
            bullets.ShootBullet(player.position, aimDir);
            player.shootCooldown = 0.15f;  // 6 shots/sec
        }
    }

    // SCAN FOR NEW TARGETS frequently
    if (player.targetShipIndex < 0 || player.retargetTimer <= 0) {
        player.targetShipIndex = FindNearestEnemyShip(player.position, player.homePlanetIndex, enemyShips);
        player.retargetTimer = 0.3f;  // SHORT 0.3s scan cycle
    }

    if (player.targetShipIndex >= 0 && ships[player.targetShipIndex].active) {
        const AIShip& target = ships[player.targetShipIndex];

        // Aim
        Vec3 targetDelta(target.position.x - player.position.x,
            target.position.y - player.position.y, 0);
        float len = sqrtf(targetDelta.x * targetDelta.x + targetDelta.y * targetDelta.y);
        if (len > 0.001f) {
            Vec3 targetDir(targetDelta.x / len, targetDelta.y / len, 0);
            player.aimAngle = atan2f(targetDir.y, targetDir.x) * 180.0f / 3.14159f;
        }

        // SHOOT BURST - PER GUARD counter
        static int burstCount[MAX_AIPLAYERS] = {};  // FIXED: per-guard!
        if (player.shootCooldown <= 0 && burstCount[index] < 3) {
            float aimRad = player.aimAngle * 3.14159f / 180.0f;
            Vec3 aimDir(cosf(aimRad), sinf(aimRad), 0.0f);
            bullets.ShootBullet(player.position, aimDir);
            player.shootCooldown = 0.15f;  // Seconds
            burstCount[index]++;
        }
        else if (burstCount[index] >= 3) {
            player.shootCooldown = 2.0f;  // 2s cooldown
            burstCount[index] = 0;
        }

        // Steering (seconds-tuned)

        // In targeting block - bigger engage range
        float dx = target.position.x - player.position.x;
        float dy = target.position.y - player.position.y;
        float distSq = dx * dx + dy * dy;
        if (distSq > 16000.0f) {  // 126u vs 80u - chase farther!
            player.targetShipIndex = -1;
            player.retargetTimer = 0.1f;  // Instant rescan
            return;
        }
    }
    else {
        // STRONGER PATROL ORBIT - copy AIShip idle logic [file:5]
        const Planet& home = planets[player.homePlanetIndex];

        // Orbit using planet rotation + time
        float orbitAngle = fmodf(home.currentRotation * 0.0174533f + dt * 0.5f, 6.28318f);
        Vec3 patrolTarget(home.position.x + cosf(orbitAngle) * (home.size + 22.0f),
            home.position.y + sinf(orbitAngle) * (home.size + 22.0f), 0);

        Vec3 toPatrol(patrolTarget.x - player.position.x,
            patrolTarget.y - player.position.y, 0);
        float patrolDist = sqrtf(toPatrol.x * toPatrol.x + toPatrol.y * toPatrol.y);

        if (patrolDist > 1.0f) {
            toPatrol.x /= patrolDist;
            toPatrol.y /= patrolDist;

            // STRONG STEERING like AIShip ApplySteering
            player.velocity.x += toPatrol.x * 20.0f * dt;
            player.velocity.y += toPatrol.y * 20.0f * dt;
        }

        // Idle spin
        player.aimAngle += dt * 60.0f;

        // FORCE minimum speed if stopped
        float speedSq = player.velocity.x * player.velocity.x + player.velocity.y * player.velocity.y;
        if (speedSq < 1.0f) {
            float nudgeAngle = dt * 2.0f;
            player.velocity.x += cosf(nudgeAngle) * 8.0f * dt;
            player.velocity.y += sinf(nudgeAngle) * 8.0f * dt;
        }
    }
}

int AIPlayerSystem::FindFreePlayer()
{
    return 0;
}

int AIPlayerSystem::FindNearestEnemyShip(const Vec3& pos, int myHomePlanetIndex, AIShipSystem& enemyShips) {
    const AIShip* ships = enemyShips.GetShips();
    int bestGuardIdx = -1, bestWorkerIdx = -1;
    float bestGuardDistSq = 999999.0f, bestWorkerDistSq = 999999.0f;

    for (int i = 0; i < enemyShips.GetMaxShips(); i++) {
        if (!ships[i].active) continue;

        int shipPlanet = ships[i].parentPlanetIndex;
        Logger::LogFormat("Ship%d planet=%d", i, shipPlanet);

        // BULLETPROOF: Skip own + Home + invalid
        if (shipPlanet == myHomePlanetIndex || shipPlanet == 0 || shipPlanet < 0) {
            Logger::LogFormat("Ship%d SKIPPED (planet=%d)", i, shipPlanet);
            continue;
        }

        float dx = ships[i].position.x - pos.x;
        float dy = ships[i].position.y - pos.y;
        float distSq = dx * dx + dy * dy;

        // PRIORITY 1: Enemy Guards (non-Home workers)
        if (shipPlanet != 0) {  // Assume guards have planet != 0
            if (distSq < bestGuardDistSq) {
                bestGuardDistSq = distSq;
                bestGuardIdx = i;
                Logger::LogFormat("New best GUARD: %d dist=%.1f", i, sqrtf(distSq));
            }
        }
        // PRIORITY 2: Enemy Workers (any non-Home)
        else {
            if (distSq < bestWorkerDistSq) {
                bestWorkerDistSq = distSq;
                bestWorkerIdx = i;
                Logger::LogFormat("New best WORKER: %d dist=%.1f", i, sqrtf(distSq));
            }
        }
    }

    // Return guards first, then workers
    int target = (bestGuardIdx != -1 && bestGuardDistSq < 4000.0f) ? bestGuardIdx : bestWorkerIdx;
    if (target != -1 && bestWorkerDistSq < 4000.0f) target = bestWorkerIdx;

    Logger::LogFormat("FINAL target=%d", target);
    return target;
}

void AIPlayerSystem::Render(const Camera3D& camera) {
    for (int i = 0; i < MAX_AIPLAYERS; i++) {
        if (!players[i].active) continue;

        Vec3 rotation(0, 0, players[i].aimAngle - 90.0f);
        Vec3 scale(players[i].size, players[i].size, players[i].size);

        Renderer3D::DrawMesh(players[i].mesh, players[i].position, rotation, scale,
            camera, players[i].r, players[i].g, players[i].b, false);
    }
}

void AIPlayerSystem::OnPlayerHit(int playerIndex) {
    if (playerIndex < 0 || playerIndex >= MAX_AIPLAYERS) return;
    AIPlayer& player = players[playerIndex];
    if (!player.active) return;

    player.health -= 25.0f;  // Matches ship damage per hit
    Logger::LogFormat("AIPlayer %d hit! Health: %.0f", playerIndex, player.health);

    if (player.health <= 0.0f) {
        player.active = false;
        Logger::LogFormat("AIPlayer %d destroyed!", playerIndex);
        // Optional: ParticleSystem3D::EmitExplosion(player.position, 20);
    }
}

