//------------------------------------------------------------------------
// Bullet.cpp - Rapid-fire machine gun bullets
//------------------------------------------------------------------------
#include "Bullet.h"
#include <cstdio>

BulletSystem::BulletSystem()
    : bulletSpeed(50.0f)      // INCREASED from 30.0f
    , bulletSize(0.3f)
    , maxLifetime(5.0f)
{
    // Initialize all bullets as inactive
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }
}

void BulletSystem::Init() {
    // Create bullet mesh (small cube)
    bulletMesh = Mesh3D::CreateCube(1.0f);
    printf("BulletSystem initialized: Max bullets = %d\n", MAX_BULLETS);
}

void BulletSystem::SpawnBulletDirectional(const Vec3& startPos, const Vec3& direction) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;

            // Position slightly ahead of player
            bullets[i].position.x = startPos.x + direction.x * 1.5f;
            bullets[i].position.y = startPos.y + direction.y * 1.5f;
            bullets[i].position.z = 0.0f;

            // Velocity in aim direction
            bullets[i].velocity.x = direction.x * bulletSpeed;
            bullets[i].velocity.y = direction.y * bulletSpeed;
            bullets[i].velocity.z = 0.0f;

            bullets[i].size = bulletSize;
            bullets[i].lifetime = 0.0f;

            return;
        }
    }
}

bool BulletSystem::GetBulletData(int index, Vec3& outPos, Vec3& outVel, float& outSize) const {
    if (index >= 0 && index < MAX_BULLETS && bullets[index].active) {
        outPos = bullets[index].position;
        outVel = bullets[index].velocity;
        outSize = bullets[index].size;
        return true;
    }
    return false;
}

void BulletSystem::DestroyBullet(int index) {
    if (index >= 0 && index < MAX_BULLETS) {
        bullets[index].active = false;
    }
}


void BulletSystem::Update(float deltaTime) {
    float dt = deltaTime / 1000.0f;

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;

        // Update position
        bullets[i].position.x += bullets[i].velocity.x * dt;
        bullets[i].position.y += bullets[i].velocity.y * dt;
        bullets[i].position.z += bullets[i].velocity.z * dt;

        // Update lifetime
        bullets[i].lifetime += dt;

        // Destroy if too old (FASTER expiration - 2 seconds)
        if (bullets[i].lifetime > maxLifetime) {
            bullets[i].active = false;
            continue;
        }

        // Destroy if too far from Earth (off-screen)
        float distFromEarth = sqrtf(bullets[i].position.x * bullets[i].position.x +
            bullets[i].position.y * bullets[i].position.y +
            bullets[i].position.z * bullets[i].position.z);

        if (distFromEarth > 100.0f) {  // Far enough to be off-screen
            bullets[i].active = false;
        }
    }
}

void BulletSystem::Render(const Camera3D& camera) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;

        // Render bullet as small WHITE cube
        Renderer3D::DrawMesh(bulletMesh,
            bullets[i].position,
            Vec3(0, 0, 0),  // No rotation
            Vec3(bullets[i].size, bullets[i].size, bullets[i].size),
            camera,
            1.0f, 1.0f, 1.0f,  // WHITE (was yellow)
            false);
    }
}

int BulletSystem::GetActiveBulletCount() const {
    int count = 0;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) count++;
    }
    return count;
}

void BulletSystem::Clear() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }
}
