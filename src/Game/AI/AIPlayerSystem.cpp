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

        UpdateAI(i, dt, planetSystem, enemyShips, bullets);

        // Drag (frame-rate independent-ish) - LOWER drag value = LESS drag = FASTER
        players[i].velocity.x *= powf(players[i].drag, dt * 60.0f);
        players[i].velocity.y *= powf(players[i].drag, dt * 60.0f);

        // REMOVE speed clamp temporarily for max speed - guards go FAST
        /*
        float speed = sqrtf(players[i].velocity.x * players[i].velocity.x +
            players[i].velocity.y * players[i].velocity.y);
        if (speed > players[i].maxSpeed && speed > 0.0001f) {
            players[i].velocity.x = (players[i].velocity.x / speed) * players[i].maxSpeed;
            players[i].velocity.y = (players[i].velocity.y / speed) * players[i].maxSpeed;
        }
        */

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
            players[slot].r = planets[p].r;
            players[slot].g = planets[p].g;
            players[slot].b = planets[p].b;

            // Bottom spawn around THIS planet
            float angle = (needed * 2.0944f) + 4.71239f;  // 120° spacing, bottom
            float orbitDist = planets[p].size + 18.0f;
            players[slot].position = planets[p].position +
                Vec3(cosf(angle) * orbitDist, sinf(angle) * orbitDist, 0);

            players[slot].mesh = Mesh3D::CreatePyramid(2.0f, 3.0f);
            players[slot].homePlanetIndex = p;
            players[slot].active = true;
            players[slot].velocity = Vec3(0, 0, 0);
            players[slot].maxSpeed = 180.0f;        // *** HIGH SPEED ***
            players[slot].drag = 0.985f;            // *** LOW DRAG = FAST ***
            players[slot].size = 1.0f;              // Larger & visible
            players[slot].shootCooldown = 0.0f;
            players[slot].aimAngle = 0.0f;
            players[slot].health = 100.0f;
        }
    }
}

void AIPlayerSystem::UpdateAI(int index, float dt, PlanetSystem planetSystem,
    AIShipSystem enemyShips, BulletSystem bullets) {
    AIPlayer& player = players[index];
    const Planet* planets = planetSystem.GetPlanets();
    const AIShip* ships = enemyShips.GetShips();

    player.retargetTimer -= dt;
    if (player.retargetTimer <= 0) {
        player.targetShipIndex = FindNearestEnemyShip(player.position, player.homePlanetIndex, enemyShips);
        player.retargetTimer = 0.1f;
    }

    Vec3 targetPos;
    bool hasTarget = (player.targetShipIndex >= 0 && player.targetShipIndex < enemyShips.GetMaxShips() && ships[player.targetShipIndex].active);

    if (hasTarget) {
        const AIShip& targetShip = ships[player.targetShipIndex];
        targetPos = Vec3(targetShip.position.x, targetShip.position.y, 0);

        // *** STEER BUT NO POSITION INTEGRATION ***
        ApplyAISteering(&player, targetPos, dt, 140.0f, 80.0f);  // false = no position update

        float dist = (targetPos - player.position).Length();

        // *** PREDICTIVE AIM ***
        Vec3 predTarget = targetPos + Vec3(targetShip.velocity.x * 0.5f, targetShip.velocity.y * 0.5f, 0);
        player.aimAngle = atan2f(predTarget.y - player.position.y, predTarget.x - player.position.x) * 180.0f / 3.14159f;

        if (player.shootCooldown <= 0.0f && dist <= 180.0f) {
            float aimRad = player.aimAngle * 3.14159f / 180.0f;
            Vec3 aimDir(cosf(aimRad), sinf(aimRad), 0);
            bullets.ShootBullet(player.position, aimDir, player.homePlanetIndex);  // *** 3 ARGS ***
            player.shootCooldown = 0.08f;
            //Logger::LogFormat("GUARD %d SHOOTS ship %d (dist %.1f)", index, player.targetShipIndex, dist);
        }
    }
    else {
        const Planet& home = planets[player.homePlanetIndex];
        float orbitAngle = fmodf(home.currentRotation * 0.017f + dt * 0.3f, 6.283f);
        targetPos = Vec3(home.position.x + cosf(orbitAngle) * (home.size + 35.0f),
            home.position.y + sinf(orbitAngle) * (home.size + 35.0f), 0);
        ApplyAISteering(&player, targetPos, dt, 100.0f, 50.0f);  // *** NO DOUBLE POS ***
    }

    player.shootCooldown -= dt;
}

void AIPlayerSystem::ApplyAISteering(AIPlayer* player, const Vec3 targetPos, float dt,
    float maxSpeed, float accel, bool updatePosition)
{
    Vec3 toTarget(targetPos - player->position);
    float len = toTarget.Length();
    if (len < 12.0f) return;  // *** STOP AT 12u - NO OVERSHOOT ***

    Vec3 desiredVel(toTarget.x / len, toTarget.y / len, toTarget.z / len);
    desiredVel.x *= maxSpeed;
    desiredVel.y *= maxSpeed;
    desiredVel.z *= maxSpeed;

    Vec3 dv(desiredVel.x - player->velocity.x,
        desiredVel.y - player->velocity.y,
        desiredVel.z - player->velocity.z);

    float dvLen = dv.Length();
    if (dvLen > accel * dt && dvLen > 0.001f) {
        dv.x *= (accel * dt) / dvLen;
        dv.y *= (accel * dt) / dvLen;
        dv.z *= (accel * dt) / dvLen;
    }

    player->velocity.x += dv.x;
    player->velocity.y += dv.y;
    player->velocity.z += dv.z;

    // *** OPTIONAL POSITION UPDATE ***
    if (updatePosition) {
        player->position.x += player->velocity.x * dt;
        player->position.y += player->velocity.y * dt;
        player->position.z += player->velocity.z * dt;
    }

    // Aim toward velocity
    if (player->velocity.LengthSquared() > 0.01f) {
        Vec3 velDir = player->velocity.Normalized();
        player->aimAngle = atan2f(velDir.y, velDir.x) * 180.0f / 3.14159f;
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

        // *** FIXED: Skip OWN planet + ALL HOME PLANET SHIPS (0) ***
        if (shipPlanet == myHomePlanetIndex || shipPlanet == 0) {
            //Logger::LogFormat("Guard SKIP ship %d (planet=%d)", i, shipPlanet);
            continue;
        }

        float dx = ships[i].position.x - pos.x;
        float dy = ships[i].position.y - pos.y;
        float distSq = dx * dx + dy * dy;

        if (shipPlanet > 0) {  // Enemy planet ships = priority guards
            if (distSq < bestGuardDistSq) {
                bestGuardDistSq = distSq;
                bestGuardIdx = i;
            }
        }
        else if (distSq < bestWorkerDistSq) {
            bestWorkerDistSq = distSq;
            bestWorkerIdx = i;
        }
    }

    int target = (bestGuardIdx != -1 && bestGuardDistSq < 40000.0f) ? bestGuardIdx :
        (bestWorkerIdx != -1 && bestWorkerDistSq < 40000.0f) ? bestWorkerIdx : -1;

    //Logger::LogFormat("GUARD target=%d (guard %.1f, worker %.1f)", target,
        //bestGuardIdx != -1 ? sqrtf(bestGuardDistSq) : -1.0f,
        //bestWorkerIdx != -1 ? sqrtf(bestWorkerDistSq) : -1.0f);
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

void AIPlayerSystem::OnPlayerHit(int playerIndex, AsteroidSystem* asteroidSystem,
    PlanetSystem* planetSystem, int killerHomePlanet) {
    if (playerIndex < 0 || playerIndex >= MAX_AIPLAYERS) return;
    AIPlayer& player = players[playerIndex];
    if (!player.active) return;

    player.health -= 25.0f;  // Identical ship damage
    player.consecutiveHits++;  // Combo counter
    player.hitComboTimer = 2.0f;  // 2s window

    //Logger::LogFormat("AIPlayer %d hit! Health: %.0f", playerIndex, player.health);

    if (player.hitComboTimer <= 0.0f) {
        player.consecutiveHits = 0;  // Reset if no hits in 2s
    }

    if (player.health <= 0.0f) {

        // DEAD: Lose 10% from victim's planet
        int victimPlanet = player.homePlanetIndex;
        Planet& victimP = planetSystem->GetPlanets()[victimPlanet];
        float stolen = 0.10f;  // 10%
        victimP.ironLevel *= (1.0f - stolen);
        victimP.waterLevel *= (1.0f - stolen);
        victimP.carbonLevel *= (1.0f - stolen);
        victimP.energyLevel *= (1.0f - stolen);
        //Logger::LogFormat("Victim Planet %d loses 10%% resources", victimPlanet);

        // KILLER: Gain 20% of stolen (double reward)
        Planet& killerP = planetSystem->GetPlanets()[killerHomePlanet];
        float reward = stolen * 0.20f;
        killerP.ironLevel += reward * 100.0f;    // Scale to 0-100 range
        killerP.waterLevel += reward * 100.0f;
        killerP.carbonLevel += reward * 100.0f;
        killerP.energyLevel += reward * 100.0f;
        killerP.ironLevel = fminf(killerP.ironLevel, 100.0f);
        killerP.waterLevel = fminf(killerP.waterLevel, 100.0f);
        killerP.carbonLevel = fminf(killerP.carbonLevel, 100.0f);
        killerP.energyLevel = fminf(killerP.energyLevel, 100.0f);
        //Logger::LogFormat("Killer Planet %d gains %.1f%% resources", killerHomePlanet, reward * 100);

        // *** PLAYER REFUEL ONLY if killer is HOME (us) ***
        if (killerHomePlanet == 0) {  // Player home planet
            extern Player player;  // GameTest global ref
            player.RefillFuel(25.0f);
            Logger::LogFormat("PLAYER REFUELED +25 fuel! (guard %d)", playerIndex);
        }

        player.active = false;
        Logger::LogFormat("AIPlayer %d DESTROYED!", playerIndex);
    }
}
