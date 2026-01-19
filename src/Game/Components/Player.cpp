//------------------------------------------------------------------------
// Player.cpp - Free movement implementation
//------------------------------------------------------------------------
#include "Player.h"
#include "../../ContestAPI/App.h"
#include <cstdio>
#include <cmath>

Player::Player()
    : position(0.0f, -15.0f, 0.0f)  // Start below Earth
    , velocity(0.0f, 0.0f, 0.0f)
    , aimAngle(90.0f)
    , acceleration(120.0f)      // INCREASED from 50.0f (much faster)
    , maxSpeed(50.0f)           // INCREASED from 25.0f (way faster)
    , drag(0.98f)
    , size(0.8f)
{
}

void Player::Init() {
    mesh = Mesh3D::CreatePyramid(1.0f, 1.5f);
    printf("Player initialized with FREE MOVEMENT\n");
    printf("Controls: WASD = Move, Mouse = Aim, Left Click = Shoot\n");
}

void Player::Update(float deltaTime) {
    float dt = deltaTime / 1000.0f;

    // === WASD MOVEMENT ===
    Vec3 inputDir(0.0f, 0.0f, 0.0f);

    if (App::IsKeyPressed(App::KEY_W)) {
        inputDir.y += 1.0f;  // Up
    }
    if (App::IsKeyPressed(App::KEY_S)) {
        inputDir.y -= 1.0f;  // Down
    }
    if (App::IsKeyPressed(App::KEY_A)) {
        inputDir.x -= 1.0f;  // Left
    }
    if (App::IsKeyPressed(App::KEY_D)) {
        inputDir.x += 1.0f;  // Right
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

    // World position = player position + mouse offset
    float worldMouseX = position.x + mouseOffsetX;
    float worldMouseY = position.y + mouseOffsetY;

    // Calculate aim angle
    float dx = worldMouseX - position.x;
    float dy = worldMouseY - position.y;
    aimAngle = atan2f(dy, dx) * 180.0f / 3.14159f;
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
        1.0f, 1.0f, 1.0f,  // White
        false);
}

Vec3 Player::GetAimDirection() const {
    float angleRad = aimAngle * 3.14159f / 180.0f;
    Vec3 dir;
    dir.x = cosf(angleRad);
    dir.y = sinf(angleRad);
    dir.z = 0.0f;
    return dir;
}
