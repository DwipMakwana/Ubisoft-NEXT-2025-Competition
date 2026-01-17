//------------------------------------------------------------------------
// GameTest.cpp - Cleaned up with Player class
//------------------------------------------------------------------------
#include "../ContestAPI/App.h"
#include "Rendering/Camera3D.h"
#include "Rendering/Renderer3D.h"
#include "Rendering/Mesh3D.h"
#include "Utilities/MathUtils3D.h"
#include "Components/Bullet.h"
#include "Components/Player.h"
#include <cstdlib>
#include <ctime>
#include <Components/Asteroid.h>

Camera3D camera3D;
Mesh3D starMesh, earthMesh;
BulletSystem bulletSystem;
Player player;
AsteroidSystem asteroidSystem;

Vec3 earthPosition(0, 0, 0);
float earthRadius = 8.0f;

const int STAR_COUNT = 500;
struct Star {
    Vec3 position;
    float brightness;
    float size;
};
Star stars[STAR_COUNT];

// Shooting
float shootCooldown = 0.0f;

// Camera
float cameraYaw = 0.0f;

void DrawCrosshair() {
    float mouseX, mouseY;
    App::GetMousePos(mouseX, mouseY);

    mouseY = 768.0f - mouseY;

    // Crosshair settings
    float size = 15.0f;
    float gap = 5.0f;
    float circleRadius = 10.0f;

    // Draw circle (24 segments)
    int segments = 24;
    for (int i = 0; i < segments; i++) {
        float angle1 = (i / (float)segments) * 6.28318f;
        float angle2 = ((i + 1) / (float)segments) * 6.28318f;

        float x1 = mouseX + cosf(angle1) * circleRadius;
        float y1 = mouseY + sinf(angle1) * circleRadius;
        float x2 = mouseX + cosf(angle2) * circleRadius;
        float y2 = mouseY + sinf(angle2) * circleRadius;

        App::DrawLine(x1, y1, x2, y2, 1.0f, 1.0f, 1.0f);
    }

    // Draw cross lines
    // Horizontal left
    App::DrawLine(mouseX - size, mouseY, mouseX - gap, mouseY, 1.0f, 1.0f, 1.0f);
    // Horizontal right
    App::DrawLine(mouseX + gap, mouseY, mouseX + size, mouseY, 1.0f, 1.0f, 1.0f);
    // Vertical top
    App::DrawLine(mouseX, mouseY - size, mouseX, mouseY - gap, 1.0f, 1.0f, 1.0f);
    // Vertical bottom
    App::DrawLine(mouseX, mouseY + gap, mouseX, mouseY + size, 1.0f, 1.0f, 1.0f);

    // Optional: Add center dot for precision
    App::DrawLine(mouseX - 1, mouseY, mouseX + 1, mouseY, 1.0f, 1.0f, 1.0f);
    App::DrawLine(mouseX, mouseY - 1, mouseX, mouseY + 1, 1.0f, 1.0f, 1.0f);
}

void Init() {
    srand((unsigned int)time(nullptr));

    starMesh = Mesh3D::CreateSphere(1.0f, 4);
    earthMesh = Mesh3D::CreateSphere(1.0f, 24);

    bulletSystem.Init();
    player.Init();
    asteroidSystem.Init();

    printf("=== ORBITAL DEFENDER ===\n");

    // Generate stars
    for (int i = 0; i < STAR_COUNT; i++) {
        float theta = (rand() % 1000) / 1000.0f * 6.28318f;
        float phi = (rand() % 1000) / 1000.0f * 3.14159f;
        float distance = 50.0f + (rand() % 1000) / 1000.0f * 50.0f;

        stars[i].position.x = sinf(phi) * cosf(theta) * distance;
        stars[i].position.y = cosf(phi) * distance;
        stars[i].position.z = sinf(phi) * sinf(theta) * distance;

        stars[i].brightness = 0.4f + (rand() % 100) / 100.0f * 0.6f;
        stars[i].size = 0.1f + (rand() % 100) / 100.0f * 0.12f;
    }
}

void Update(float deltaTime) {
    float dt = deltaTime / 1000.0f;

    // Update player
    player.Update(deltaTime);

    // === MOUSE SHOOTING (Left Click) ===
    if (App::IsMousePressed(GLUT_LEFT_BUTTON)) {
        if (shootCooldown <= 0.0f) {
            // Shoot in direction player is facing
            bulletSystem.SpawnBulletDirectional(player.GetPosition(), player.GetAimDirection());
            shootCooldown = 0.08f;  // Fire rate
        }
    }

    // Update shoot cooldown
    if (shootCooldown > 0.0f) {
        shootCooldown -= dt;
    }

    // Update camera (fixed top-down view)
    camera3D.SetPosition(Vec3(0, 0, 70));
    camera3D.SetTarget(Vec3(0, 0, 0));
    camera3D.SetUp(Vec3(0, 1, 0));  // Fixed up direction

    // Update bullets
    bulletSystem.Update(deltaTime);
    asteroidSystem.Update(deltaTime, earthPosition);

    // PLAYER-ASTEROID COLLISION (FIXED RADIUS)
    float playerCollisionRadius = 1.2f;
    Vec3 playerVelocity = player.GetVelocityVector();
    asteroidSystem.CheckPlayerCollision(player.GetPosition(), playerCollisionRadius, playerVelocity);

    // BULLET-ASTEROID COLLISION (DESTRUCTION)
    for (int b = 0; b < 100; b++) {
        Vec3 bulletPos, bulletVel;
        float bulletSize;

        if (bulletSystem.GetBulletData(b, bulletPos, bulletVel, bulletSize)) {
            int hitIndex = asteroidSystem.CheckBulletCollision(bulletPos, bulletVel, bulletSize);

            if (hitIndex >= 0) {
                // Bullet destroyed asteroid - remove bullet too
                bulletSystem.DestroyBullet(b);
            }
        }
    }
}

void Render() {
    int culled = 0;
    Vec3 camForward = (earthPosition - camera3D.GetPosition()).Normalized();

    // Render stars
    for (int i = 0; i < STAR_COUNT; i++) {
        Vec3 toStar = stars[i].position - camera3D.GetPosition();
        if (toStar.Dot(camForward) < -10.0f) {
            culled++;
            continue;
        }

        float b = stars[i].brightness;
        Renderer3D::DrawMesh(starMesh, stars[i].position, Vec3(0, 0, 0),
            Vec3(stars[i].size, stars[i].size, stars[i].size),
            camera3D, b, b, b, false);
    }

    // Earth
    Renderer3D::DrawMesh(earthMesh, earthPosition, Vec3(0, 0, 0),
        Vec3(earthRadius, earthRadius, earthRadius),
        camera3D, 0.2f, 0.5f, 0.9f, false);

    // Asteroids (render before player so player is on top)
    asteroidSystem.Render(camera3D);

    // Player
    player.Render(camera3D);

    // Bullets
    bulletSystem.Render(camera3D);

    //Crosshair
	DrawCrosshair();
}

void Shutdown() {
    printf("Shutdown\n");
}
