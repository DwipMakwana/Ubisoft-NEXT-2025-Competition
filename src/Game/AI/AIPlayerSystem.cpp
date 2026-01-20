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

void AIPlayerSystem::Update(float deltaTime, PlanetSystem& planetSystem, AIShipSystem& enemyShips,
    BulletSystem& bulletSystem, Player& humanPlayer)
{
    float dt = deltaTime / 1000.0f; // seconds

    SpawnForActivePlanets(planetSystem);

    for (int i = 0; i < MAX_AIPLAYERS; i++) {
        if (!players[i].active) continue;

        UpdateAI(i, dt, planetSystem, enemyShips, bulletSystem, humanPlayer);

        // Drag (frame-rate independent-ish) - LOWER drag value = LESS drag = FASTER
        players[i].velocity.x *= powf(players[i].drag, dt * 60.0f);
        players[i].velocity.y *= powf(players[i].drag, dt * 60.0f);

        // Integrate with seconds
        players[i].position.x += players[i].velocity.x * dt;
        players[i].position.y += players[i].velocity.y * dt;

        // UPDATE THRUSTER PARTICLES!
        if (players[i].thrusterParticles) {
            float speed = players[i].velocity.Length();
            if (speed > 5.0f) {  // Only emit when moving
                float angleRad = players[i].aimAngle * 3.14159f / 180.0f;
                Vec3 thrusterOffset(-cosf(angleRad) * 1.2f, -sinf(angleRad) * 1.2f, 0);
                Vec3 thrusterPos = players[i].position + thrusterOffset;

                players[i].thrusterParticles->emitVelocity = players[i].velocity * -0.2f;
                players[i].thrusterParticles->EmitContinuous(thrusterPos, 50.0f, dt);
            }
            players[i].thrusterParticles->Update(deltaTime);
        }
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

            // CREATE THRUSTER PARTICLES FOR AI!
            if (players[slot].thrusterParticles == nullptr) {
                players[slot].thrusterParticles = new ParticleSystem3D(300);
                players[slot].thrusterParticles->startColor = Vec3(1.0f, 0.6f, 0.1f);     // Orange
                players[slot].thrusterParticles->endColor = Vec3(0.8f, 0.2f, 0.05f);       // Dark orange
                players[slot].thrusterParticles->startSize = 0.8f;
                players[slot].thrusterParticles->endSize = 0.1f;
                players[slot].thrusterParticles->lifeTime = 0.2f;
            }
        }
    }
}

void AIPlayerSystem::UpdateAI(int index, float dt, PlanetSystem planetSystem,
    AIShipSystem enemyShips, BulletSystem& bullets, Player& humanPlayer) {
    
    AIPlayer& guard = players[index];
    const Planet* planets = planetSystem.GetPlanets();
    const AIShip* ships = enemyShips.GetShips();

    guard.retargetTimer -= dt;
    if (guard.retargetTimer <= 0) {
        TargetInfo target = FindBestTarget(guard.position, guard.homePlanetIndex, 
            enemyShips, humanPlayer);
        
        // Store target info in the guard
        if (target.type != TargetType::NONE) {
            guard.targetShipIndex = target.index;  // Reuse this field
            guard.targetType = (int)target.type;   // Add this field to AIPlayer struct
        } else {
            guard.targetShipIndex = -1;
        }
        
        guard.retargetTimer = 0.1f;
    }

    Vec3 targetPos;
    bool hasTarget = (guard.targetShipIndex >= 0);
    
    // Get target position based on type
    if (hasTarget) {
        TargetType tType = (TargetType)guard.targetType;
        Vec3 targetVel(0, 0, 0);
        
        if (tType == TargetType::SHIP) {
            if (guard.targetShipIndex < enemyShips.GetMaxShips() && 
                ships[guard.targetShipIndex].active) {
                targetPos = ships[guard.targetShipIndex].position;
                targetVel = ships[guard.targetShipIndex].velocity;
            } else {
                hasTarget = false;
            }
        }
        else if (tType == TargetType::GUARD) {
            if (guard.targetShipIndex < MAX_AIPLAYERS && 
                players[guard.targetShipIndex].active) {
                targetPos = players[guard.targetShipIndex].position;
                targetVel = players[guard.targetShipIndex].velocity;
            } else {
                hasTarget = false;
            }
        }
        else if (tType == TargetType::PLAYER) {
            targetPos = humanPlayer.GetPosition();
            targetVel = humanPlayer.GetVelocityVector();
        }
        
        if (hasTarget) {
            // Steer toward target
            ApplyAISteering(&guard, targetPos, dt, 140.0f, 80.0f);
            
            float dist = (targetPos - guard.position).Length();
            
            // Predictive aim
            Vec3 predTarget = targetPos + targetVel * 0.5f;
            guard.aimAngle = atan2f(predTarget.y - guard.position.y, 
                predTarget.x - guard.position.x) * 180.0f / 3.14159f;
            
            // Shoot if in range
            if (guard.shootCooldown <= 0.0f && dist <= 180.0f) {
                float aimRad = guard.aimAngle * 3.14159f / 180.0f;
                Vec3 aimDir(cosf(aimRad), sinf(aimRad), 0);
                bullets.ShootBullet(guard.position, aimDir, guard.homePlanetIndex);
                guard.shootCooldown = 0.08f;
            }
        }
    }
    
    // No target - orbit home planet
    if (!hasTarget) {
        const Planet& home = planets[guard.homePlanetIndex];
        float orbitAngle = fmodf(home.currentRotation * 0.017f + dt * 0.3f, 6.283f);
        targetPos = Vec3(home.position.x + cosf(orbitAngle) * (home.size + 35.0f),
            home.position.y + sinf(orbitAngle) * (home.size + 35.0f), 0);
        ApplyAISteering(&guard, targetPos, dt, 100.0f, 50.0f);
    }

    guard.shootCooldown -= dt;
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

TargetInfo AIPlayerSystem::FindBestTarget(const Vec3& pos, int myHomePlanetIndex,
    AIShipSystem& enemyShips, Player& humanPlayer) {

    TargetInfo best;
    best.type = TargetType::NONE;
    best.index = -1;
    best.distanceSq = 999999.0f;

    const AIShip* ships = enemyShips.GetShips();

    // 1. Check enemy guards (OTHER AI players)
    for (int i = 0; i < MAX_AIPLAYERS; i++) {
        if (!players[i].active) continue;
        if (players[i].homePlanetIndex == myHomePlanetIndex) continue;  // Skip allies

        float dx = players[i].position.x - pos.x;
        float dy = players[i].position.y - pos.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < best.distanceSq && distSq < 40000.0f) {  // 200 unit range
            best.type = TargetType::GUARD;
            best.index = i;
            best.distanceSq = distSq;
            best.position = players[i].position;
            best.velocity = players[i].velocity;
        }
    }

    // 2. Check human player (planet 0)
    if (myHomePlanetIndex != 0) {  // Don't attack player if we're on their team
        float dx = humanPlayer.GetPosition().x - pos.x;
        float dy = humanPlayer.GetPosition().y - pos.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < best.distanceSq && distSq < 40000.0f) {
            best.type = TargetType::PLAYER;
            best.index = 0;
            best.distanceSq = distSq;
            best.position = humanPlayer.GetPosition();
            best.velocity = humanPlayer.GetVelocityVector();
        }
    }

    // 3. Check enemy ships (existing logic)
    for (int i = 0; i < enemyShips.GetMaxShips(); i++) {
        if (!ships[i].active) continue;

        int shipPlanet = ships[i].parentPlanetIndex;
        if (shipPlanet == myHomePlanetIndex || shipPlanet == 0) continue;  // Skip allies

        float dx = ships[i].position.x - pos.x;
        float dy = ships[i].position.y - pos.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < best.distanceSq && distSq < 40000.0f) {
            best.type = TargetType::SHIP;
            best.index = i;
            best.distanceSq = distSq;
            best.position = ships[i].position;
            best.velocity = ships[i].velocity;
        }
    }

    return best;
}

void AIPlayerSystem::Render(const Camera3D& camera) {
    for (int i = 0; i < MAX_AIPLAYERS; i++) {
        if (!players[i].active) continue;

        // Render particles FIRST (behind ship)
        if (players[i].thrusterParticles) {
            players[i].thrusterParticles->Render(camera);
        }

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
            player.RefillHealth(25.0f);
        }

        // CLEANUP PARTICLES!
        if (player.thrusterParticles) {
            delete player.thrusterParticles;
            player.thrusterParticles = nullptr;
        }

        player.active = false;
        Logger::LogFormat("AIPlayer %d DESTROYED!", playerIndex);
    }
}
