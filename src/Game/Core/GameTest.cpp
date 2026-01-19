//------------------------------------------------------------------------
// GameTest.cpp - Add planet system
//------------------------------------------------------------------------
#include "../ContestAPI/App.h"
#include "Rendering/Camera3D.h"
#include "Rendering/Renderer3D.h"
#include "Rendering/Mesh3D.h"
#include "Utilities/MathUtils3D.h"
#include "Utilities/Logger.h"
#include "Components/Player.h"
#include "Components/Asteroid.h"
#include "Components/StarField.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <Components/Planet.h>
#include <Utilities/WorldText3D.h>
#include <Systems/TowingSystem.h>
#include <UI/UIManager.h>
#include <Systems/AIShipSystem.h>

Camera3D camera3D;
Player player;
AsteroidSystem asteroidSystem;
StarField starField;
PlanetSystem planetSystem;
TowingSystem towingSystem;
UIManager uiManager;
AIShipSystem aiShips;

void Init() {
    Logger::LogInfo("=== GAME INITIALIZATION START ===");

    srand((unsigned int)time(nullptr));

    player.Init();
    planetSystem.Init();
	uiManager.Init();
    aiShips.Init();

    asteroidSystem.SetPlanetSystem(&planetSystem);
    asteroidSystem.Init();

    camera3D.SetPosition(Vec3(0, 0, 50));
    camera3D.SetTarget(Vec3(0, 0, 0));
    camera3D.SetUp(Vec3(0, 1, 0));

    starField.Init(player.GetPosition());

    Logger::LogInfo("=== GAME INITIALIZATION COMPLETE ===");
}

void Update(float deltaTime) {
    float dt = deltaTime / 1000.0f;

    player.Update(deltaTime);

    Vec3 newPlayerPos = player.GetPosition();
    Vec3 playerVel = player.GetVelocityVector();

    // Update towing system
    towingSystem.Update(deltaTime, newPlayerPos, playerVel,
        &asteroidSystem, &planetSystem);

    // === CRITICAL: Mark towed asteroid to disable AI ===
    Asteroid* asteroids = asteroidSystem.GetAsteroids();
    int maxAsteroids = asteroidSystem.GetMaxAsteroids();

    for (int i = 0; i < maxAsteroids; i++) {
        if (asteroids[i].active) {
            // Set towed flag (disables AI)
            asteroids[i].isTowed = (towingSystem.IsTowing() &&
                i == towingSystem.GetTowedAsteroidIndex());

            if (towingSystem.IsTowing()) {
                int towedIdx = towingSystem.GetTowedAsteroidIndex();
                if (towedIdx >= 0 && towedIdx < asteroidSystem.GetMaxAsteroids()) {
                    asteroids[towedIdx].isPersistent = true;  // Already set on grab?  
                    // Update sector live  
                    int newSX, newSY;
                    asteroidSystem.GetSectorCoords(asteroids[towedIdx].position, newSX, newSY);
                    asteroids[towedIdx].sectorX = newSX;
                    asteroids[towedIdx].sectorY = newSY;
                }
            }
        }
    }

    // Handle 'E' key for grab/deposit
    static bool eKeyWasPressed = false;
    bool eKeyPressed = App::IsKeyPressed(App::KEY_E);

    if (eKeyPressed && !eKeyWasPressed) {
        towingSystem.HandleInput(newPlayerPos, &asteroidSystem, &planetSystem);
    }
    eKeyWasPressed = eKeyPressed;

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
    planetSystem.Update(deltaTime, playerPos);
    asteroidSystem.Update(deltaTime, playerPos, &planetSystem);
    starField.Update(playerPos);
    uiManager.Update(deltaTime);

    aiShips.Update(deltaTime, planetSystem, asteroidSystem);

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
}

void Render() {
    // Render stars (background)
    starField.Render(camera3D);

    // Render game objects
    planetSystem.Render(camera3D);
    asteroidSystem.Render(camera3D);
    player.Render(camera3D);

    towingSystem.Render(camera3D, &asteroidSystem, &planetSystem);

    aiShips.Render(camera3D, asteroidSystem, planetSystem);


    // Player position label (world space)
    Vec3 playerPos = player.GetPosition();

    char posText[64];
    sprintf(posText, "Position: X:%.1f Y:%.1f", playerPos.x, playerPos.y);

    uiManager.AddText("player_pos", 10.0f, 720.0f, posText,
        1.0f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_12);

    if (towingSystem.IsTowing()) {
        float screenCenterX = APP_VIRTUAL_WIDTH / 2.0f - 40.0f;
        float bottomY = (APP_VIRTUAL_HEIGHT / 2.0f) - 340.0f;  // 80 pixels from bottom

        uiManager.AddText("towing_title", screenCenterX, bottomY + 20.0f, "TOWING ASTEROID!",
            1.0f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_12);
        uiManager.AddText("towing_hint", screenCenterX - 70.0f, bottomY, "Fly to your planet and press [E] to replenish resource",
            0.8f, 0.8f, 0.8f, GLUT_BITMAP_HELVETICA_10);
    }
    else {
        uiManager.RemoveText("towing_title");
        uiManager.RemoveText("towing_hint");
    }

    uiManager.Render();
}

void Shutdown() {
    Logger::LogInfo("Shutdown");
}
