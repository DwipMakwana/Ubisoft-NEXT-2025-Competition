//------------------------------------------------------------------------
// GameTest.cpp - Add planet system
//------------------------------------------------------------------------
#include "../ContestAPI/App.h"
#include "Rendering/Camera3D.h"
#include "Rendering/Renderer3D.h"
#include "Rendering/Mesh3D.h"
#include "Utilities/MathUtils3D.h"
#include "Utilities/Logger.h"
#include "Components/Bullet.h"
#include "Components/Player.h"
#include "Components/Asteroid.h"
#include "Components/StarField.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <Components/Planet.h>

Camera3D camera3D;
BulletSystem bulletSystem;
Player player;
AsteroidSystem asteroidSystem;
StarField starField;
PlanetSystem planetSystem;

float shootCooldown = 0.0f;

void DrawCrosshair() {
    // ... existing crosshair code ...
}

void Init() {
    Logger::LogInfo("=== GAME INITIALIZATION START ===");

    srand((unsigned int)time(nullptr));

    bulletSystem.Init();
    player.Init();
    asteroidSystem.Init();
    planetSystem.Init();

    camera3D.SetPosition(Vec3(0, 0, 70));
    camera3D.SetTarget(Vec3(0, 0, 0));
    camera3D.SetUp(Vec3(0, 1, 0));

    starField.Init(player.GetPosition());

    Logger::LogInfo("=== GAME INITIALIZATION COMPLETE ===");
}

void Update(float deltaTime) {
    float dt = deltaTime / 1000.0f;

    player.Update(deltaTime);

    Vec3 newPlayerPos = player.GetPosition();

    // Shooting
    if (App::IsMousePressed(GLUT_LEFT_BUTTON)) {
        if (shootCooldown <= 0.0f) {
            bulletSystem.SpawnBulletDirectional(player.GetPosition(), player.GetAimDirection());
            shootCooldown = 0.08f;
        }
    }
    if (shootCooldown > 0.0f) {
        shootCooldown -= dt;
    }

    // Camera follows player
    Vec3 playerPos = player.GetPosition();
    Vec3 targetCameraPos(playerPos.x, playerPos.y, 70.0f);
    Vec3 currentCameraPos = camera3D.GetPosition();
    float smoothFactor = 0.1f;

    Vec3 newCameraPos;
    newCameraPos.x = currentCameraPos.x + (targetCameraPos.x - currentCameraPos.x) * smoothFactor;
    newCameraPos.y = currentCameraPos.y + (targetCameraPos.y - currentCameraPos.y) * smoothFactor;
    newCameraPos.z = 70.0f;

    camera3D.SetPosition(newCameraPos);
    camera3D.SetTarget(Vec3(newCameraPos.x, newCameraPos.y, 0));
    camera3D.SetUp(Vec3(0, 1, 0));

    // Update systems
    bulletSystem.Update(deltaTime);
    asteroidSystem.Update(deltaTime, playerPos, &planetSystem);
    planetSystem.Update(deltaTime, playerPos);
    starField.Update(playerPos);

    // === PLANET-ASTEROID COLLISION (RIGID WALL) ===
    for (int i = 0; i < asteroidSystem.GetMaxAsteroids(); i++) {
        Asteroid* ast = &asteroidSystem.GetAsteroids()[i];
        if (!ast->active || ast->isFragment) continue;

        Vec3 pushDir;
        if (planetSystem.CheckAsteroidCollision(ast->position, ast->size, pushDir)) {
            // RIGID WALL: Just push asteroid out, stop velocity toward planet
            ast->position.x += pushDir.x;
            ast->position.y += pushDir.y;
            ast->position.z += pushDir.z;

            // Stop velocity component moving toward planet
            float pushLen = sqrtf(pushDir.x * pushDir.x + pushDir.y * pushDir.y + pushDir.z * pushDir.z);
            if (pushLen > 0.001f) {
                Vec3 pushNormal;
                pushNormal.x = pushDir.x / pushLen;
                pushNormal.y = pushDir.y / pushLen;
                pushNormal.z = pushDir.z / pushLen;

                // Remove velocity component going into planet
                float velDot = ast->velocity.x * pushNormal.x +
                    ast->velocity.y * pushNormal.y +
                    ast->velocity.z * pushNormal.z;

                if (velDot < 0) {  // Moving toward planet
                    ast->velocity.x -= velDot * pushNormal.x;
                    ast->velocity.y -= velDot * pushNormal.y;
                    ast->velocity.z -= velDot * pushNormal.z;
                }
            }
        }
    }

    // == = PLANET - PLAYER COLLISION(FIXED - NO STICKING) == =
    Vec3 planetPush;
    if (planetSystem.CheckPlayerCollision(playerPos, 1.2f, planetPush)) {
        // ONLY push player out, don't touch velocity at all
        playerPos.x += planetPush.x;
        playerPos.y += planetPush.y;
        playerPos.z += planetPush.z;

        player.SetPosition(playerPos);

        // That's it! No velocity changes = smooth sliding
    }

    // Player-asteroid collision
    float playerCollisionRadius = 1.2f;
    Vec3 playerVelocity = player.GetVelocityVector();
    Vec3 knockback = asteroidSystem.CheckPlayerCollision(playerPos, playerCollisionRadius, playerVelocity);

    if (knockback.x != 0.0f || knockback.y != 0.0f) {
        player.ApplyKnockback(knockback);
    }

    // Bullet-asteroid collision
    for (int b = 0; b < 100; b++) {
        Vec3 bulletPos, bulletVel;
        float bulletSize;
        if (bulletSystem.GetBulletData(b, bulletPos, bulletVel, bulletSize)) {
            int hitIndex = asteroidSystem.CheckBulletCollision(bulletPos, bulletVel, bulletSize);
            if (hitIndex >= 0) {
                bulletSystem.DestroyBullet(b);
            }
        }
    }
}

void Render() {
    // Render stars (background)
    starField.Render(camera3D);

    // Render game objects
    planetSystem.Render(camera3D);
    asteroidSystem.Render(camera3D);
    player.Render(camera3D);
    bulletSystem.Render(camera3D);

    // Crosshair
    DrawCrosshair();
}

void Shutdown() {
    Logger::LogInfo("Shutdown");
}
