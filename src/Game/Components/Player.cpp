#include "Player.h"
#include "Player.h"
#include "Player.h"
//------------------------------------------------------------------------
// Player.cpp - Free movement implementation
//------------------------------------------------------------------------
#include "Player.h"
#include "../../ContestAPI/App.h"
#include <cstdio>
#include <cmath>
#include <Utilities/WorldText3D.h>
#include "Planet.h"
#include <Utilities/Logger.h>

Player::Player()
    : position(0.0f, -15.0f, 0.0f)  // Start below Earth
    , velocity(0.0f, 0.0f, 0.0f)
    , aimAngle(90.0f)
    , acceleration(120.0f)      // INCREASED from 50.0f (much faster)
    , maxSpeed(50.0f)           // INCREASED from 25.0f (way faster)
    , drag(0.98f)
    , size(0.8f)
	, r(1.0f), g(1.0f), b(1.0f)  // White color
{
}

void Player::Init() {
    mesh = Mesh3D::CreatePyramid(2.0f, 3.0f);
}

void Player::TakeDamage(float damage) {
    if (isDead) return;  // Can't damage a dead player

    health -= damage;
    Logger::LogFormat("Player took %.0f damage! Health: %.0f/%.0f\n", damage, health, maxHealth);

    if (health <= 0.0f) {
        health = 0.0f;
        Die();
    }
}

void Player::Die() {
    isDead = true;
    respawnTimer = respawnDelay;
    velocity = Vec3(0, 0, 0);  // Stop moving
    Logger::LogInfo("PLAYER DIED! Respawning in 3 seconds...");
}

void Player::Respawn() {
    isDead = false;
    health = maxHealth;
    fuel = maxFuel;
    position = Vec3(0, -15.0f, 0.0f);  // Reset to spawn position
    velocity = Vec3(0, 0, 0);
    Logger::LogInfo("Player respawned!");
}

void Player::UpdateRespawn(float deltaTime) {
    if (!isDead) return;

    float dt = deltaTime / 1000.0f;
    respawnTimer -= dt;

    if (respawnTimer <= 0.0f) {
        Respawn();
    }
}

void Player::Update(float deltaTime) {
    float dt = deltaTime / 1000.0f;

    // Handle respawn countdown
    if (isDead) {
        UpdateRespawn(deltaTime);
        return;  // Skip all other updates while dead
    }

    // === WASD MOVEMENT ===
    Vec3 inputDir(0.0f, 0.0f, 0.0f);
    bool isThrusting = false;
    bool canThrust = (fuel > 0.0f);

    if (canThrust) {
        if (App::IsKeyPressed(App::KEY_W)) {
            inputDir.y += 1.0f;  // Up
            isThrusting = true;
        }
        if (App::IsKeyPressed(App::KEY_S)) {
            inputDir.y -= 1.0f;  // Down
            isThrusting = true;
        }
        if (App::IsKeyPressed(App::KEY_A)) {
            inputDir.x -= 1.0f;  // Left
            isThrusting = true;
        }
        if (App::IsKeyPressed(App::KEY_D)) {
            inputDir.x += 1.0f;  // Right
            isThrusting = true;
        }
    }

    // Normalize diagonal movement
    float inputMag = sqrtf(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
    if (inputMag > 0.01f) {
        inputDir.x /= inputMag;
        inputDir.y /= inputMag;

        // Apply acceleration
        velocity.x += inputDir.x * acceleration * dt;
        velocity.y += inputDir.y * acceleration * dt;
    }

    // Apply drag
    velocity.x *= powf(drag, dt * 60.0f);
    velocity.y *= powf(drag, dt * 60.0f);

    // Clamp to max speed
    float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y);
    if (speed > maxSpeed) {
        velocity.x = (velocity.x / speed) * maxSpeed;
        velocity.y = (velocity.y / speed) * maxSpeed;
    }

    // Update position
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    // === MOUSE AIM ===
    float mouseX, mouseY;
    App::GetMousePos(mouseX, mouseY);

    // Convert mouse screen coords to world coords
    // Screen: (0,0) top-left, (1024, 768) bottom-right
    // World: centered at (0,0), visible area ~80x60 units
    
    // Calculate world offset from screen center
    float screenCenterX = 1024.0f / 2.0f;
    float screenCenterY = 768.0f / 2.0f;

    float mouseOffsetX = (mouseX - screenCenterX) / 1024.0f * 80.0f;  // Screen to world scale
    float mouseOffsetY = -(mouseY - screenCenterY) / 768.0f * 60.0f;  // Flip Y, scale

    UpdateFuel(isThrusting, dt);

    // World position = player position + mouse offset
    float worldMouseX = position.x + mouseOffsetX;
    float worldMouseY = position.y + mouseOffsetY;

    // Calculate aim angle
    float dx = worldMouseX - position.x;
    float dy = worldMouseY - position.y;
    aimAngle = atan2f(dy, dx) * 180.0f / 3.14159f;
}

void Player::DrawFuelBarWorldSpace(Vec3 playerPos, float fuel, float maxFuel, const Camera3D& camera)
{
    float fuelPct = (maxFuel > 0.0f) ? (fuel / maxFuel) : 0.0f;

    // Position bar to top-right of player (world space offset)
    Vec3 barWorldPos(playerPos.x + 1.5f, playerPos.y - 2.5f, playerPos.z);  // Offset right+up

    DrawResourceBar(barWorldPos, fuelPct * 100.0f, camera, 1.0f, 1.0f, 0.0f);  // Yellow

    // Planet-style label ABOVE bar
    char fuelText[32];
    snprintf(fuelText, sizeof(fuelText), "Fuel: %.0f%%", fuelPct * 100.0f);
    Vec3 labelPos = barWorldPos;
    labelPos.y += 1.2f;  // Above bar
    WorldText3D::Print(labelPos, fuelText, camera, 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
}

void Player::DrawHealthBarWorldSpace(Vec3 playerPos, float health, float maxHealth, const Camera3D& camera)
{
    float healthPct = (maxHealth > 0.0f) ? (health / maxHealth) : 0.0f;

    // Position bar to TOP-LEFT of player (mirrored from fuel)
    Vec3 barWorldPos(playerPos.x - 6.5f, playerPos.y - 2.5f, playerPos.z);  // LEFT side (negative x)

    DrawResourceBar(barWorldPos, healthPct * 100.0f, camera, 0.0f, 1.0f, 0.0f);  // Red

    // Label ABOVE bar
    char healthText[32];
    snprintf(healthText, sizeof(healthText), "Health: %.0f%%", healthPct * 100.0f);
    Vec3 labelPos = barWorldPos;
    labelPos.y += 1.2f;  // Above bar
    WorldText3D::Print(labelPos, healthText, camera, 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
}

void Player::RefillFuel(float amount) {
    fuel += amount;
    if (fuel > 100.0f) fuel = 100.0f;  // Hard cap
}

void Player::UpdateFuel(bool isThrusting, float dtSeconds) {
    if (!isThrusting) return;  // NO COST when coasting/idling

    // Thrust costs 5% per second (full throttle)
    float thrustCost = 10.0f * dtSeconds;
    fuel -= thrustCost;

    if (fuel < 0.0f) fuel = 0.0f;
}


void Player::DrawResourceBar(const Vec3& worldPos, float value, const Camera3D& camera, float r, float g, float b)
{
    // Bar position slightly below the text
    Vec3 barPos = worldPos;
    barPos.y += 0.4f;  // Closer to text

    // Convert to NDC for screen-space bar drawing
    Vec4 ndc = camera.WorldToNDC(barPos);

    // Check if on screen
    if (ndc.w < camera.GetNearPlane()) return;
    if (ndc.x < -1.5f || ndc.x > 1.5f || ndc.y < -1.5f || ndc.y > 1.5f) return;

    // Convert to screen coordinates
    float screenX = (ndc.x + 1.0f) * (APP_VIRTUAL_WIDTH * 0.5f);
    float screenY = (ndc.y + 1.0f) * (APP_VIRTUAL_HEIGHT * 0.5f);

    // Bar dimensions (THINNER)
    float barWidth = 60.0f;
    float barHeight = 3.0f;  // CHANGED: Thinner (was 6.0f)
    float fillWidth = (value / 100.0f) * barWidth;

    // Draw filled portion only (NO BACKGROUND)
    for (float y = 0; y < barHeight; y++) {
        App::DrawLine(screenX, screenY + y,
            screenX + fillWidth, screenY + y,
            r, g, b);
    }

    // Draw thin border outline
    //App::DrawLine(screenX, screenY, screenX + barWidth, screenY, 0.4f, 0.4f, 0.4f);  // Bottom
    App::DrawLine(screenX, screenY + barHeight, screenX + barWidth, screenY + barHeight, 0.4f, 0.4f, 0.4f);  // Top
}

void Player::Render(const Camera3D& camera) {
    // Rotation to face mouse
    Vec3 rotation;
    rotation.x = 0.0f;
    rotation.y = 0.0f;
    rotation.z = aimAngle - 90.0f;  // Pyramid tip points toward mouse

    // Draw player (green pyramid)
    Renderer3D::DrawMesh(mesh, position, rotation,
        Vec3(size, size, size),
        camera,
        r, g, b,
        false);

    DrawHealthBarWorldSpace(position, health, maxHealth, camera);
    DrawFuelBarWorldSpace(position, fuel, maxFuel, camera);
}

Vec3 Player::GetAimDirection() const {
    float angleRad = aimAngle * 3.14159f / 180.0f;
    Vec3 dir;
    dir.x = cosf(angleRad);
    dir.y = sinf(angleRad);
    dir.z = 0.0f;
    return dir;
}
