//------------------------------------------------------------------------
// Bullet.h - Bullet system for player projectiles
//------------------------------------------------------------------------
#ifndef BULLET_H
#define BULLET_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Rendering/Renderer3D.h"
#include "../Rendering/Mesh3D.h"

struct Bullet {
    Vec3 position;
    Vec3 velocity;
    float size;
    bool active;
    float lifetime;  // Time bullet has been alive (for auto-destroy)
};

class BulletSystem {
private:
    static const int MAX_BULLETS = 100;
    Bullet bullets[MAX_BULLETS];
    Mesh3D bulletMesh;

    // Bullet properties
    float bulletSpeed;
    float bulletSize;
    float maxLifetime;  // Max distance/time before auto-destroy

public:
    BulletSystem();

    void Init();
    void Update(float deltaTime);
    void Render(const Camera3D& camera);

    // Spawn bullet in specific direction (for mouse aiming)
    void SpawnBulletDirectional(const Vec3& startPos, const Vec3& direction);

    bool GetBulletData(int index, Vec3& outPos, Vec3& outVel, float& outSize) const;

    void DestroyBullet(int index);

    // Get bullet count (for debugging)
    int GetActiveBulletCount() const;

    // Clear all bullets
    void Clear();
};

#endif
