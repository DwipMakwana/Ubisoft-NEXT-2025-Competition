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
#include <Systems/BulletSystem.h>

Camera3D camera3D;
Player player;
AsteroidSystem asteroidSystem;
StarField starField;
PlanetSystem planetSystem;
UIManager uiManager;
AIShipSystem aiShips;
BulletSystem bulletSystem;

void Init() {
    srand((unsigned int)time(nullptr));

    player.Init();
    planetSystem.Init();
	uiManager.Init();
    bulletSystem.Init();

    aiShips.Init();

    asteroidSystem.SetPlanetSystem(&planetSystem);
    asteroidSystem.Init();

    camera3D.SetPosition(Vec3(0, 0, 50));
    camera3D.SetTarget(Vec3(0, 0, 0));
    camera3D.SetUp(Vec3(0, 1, 0));

    starField.Init(player.GetPosition());
}

void Update(float deltaTime) {
    float dt = deltaTime / 1000.0f;

    player.Update(deltaTime);

    Vec3 newPlayerPos = player.GetPosition();
    Vec3 playerVel = player.GetVelocityVector();

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

    // SHOOT (Left Click)
    if (App::IsKeyPressed(App::KEY_SPACE))
    {
        Vec3 shootDir = player.GetAimDirection();  // Uses rotation!
        bulletSystem.ShootBullet(player.GetPosition(), shootDir);
    }

    // Update systems
    planetSystem.Update(deltaTime, playerPos);
    asteroidSystem.Update(deltaTime, playerPos, &planetSystem);
    starField.Update(playerPos);
    uiManager.Update(deltaTime);
    bulletSystem.Update(deltaTime, aiShips);
    aiShips.Update(deltaTime, planetSystem, asteroidSystem);
    aiShips.ResolveCollisionsWithAsteroids(asteroidSystem);

    Vec3 playerPush;
    asteroidSystem.ResolveExternalCollision(player.GetPosition(), 1.2f,
        player.GetVelocityVector(), playerPush);

    if (playerPush.LengthSquared() > 0.01f)
    {
        Vec3 playerPos = player.GetPosition();
        playerPos.x += playerPush.x;
        playerPos.y += playerPush.y;
        playerPos.z += playerPush.z;
        player.SetPosition(playerPos);
    }
}

void Render() {
    // Render stars (background)
    starField.Render(camera3D);

    // Render game objects
    planetSystem.Render(camera3D);
    asteroidSystem.Render(camera3D);
    player.Render(camera3D);
    aiShips.Render(camera3D, asteroidSystem, planetSystem);
    bulletSystem.Render(camera3D);

    // Player position label (world space)
    Vec3 playerPos = player.GetPosition();

    char posText[64];
    sprintf(posText, "Position: X:%.1f Y:%.1f", playerPos.x, playerPos.y);

    uiManager.AddText("player_pos", 10.0f, 720.0f, posText,
        1.0f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_12);

    uiManager.Render();
}

void Shutdown() {
    Logger::LogInfo("Shutdown");
}
