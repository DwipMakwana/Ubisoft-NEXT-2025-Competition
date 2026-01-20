//------------------------------------------------------------------------
// Player.h - Free movement space shooter
//------------------------------------------------------------------------
#ifndef PLAYER_H
#define PLAYER_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Rendering/Renderer3D.h"
#include "../Rendering/Mesh3D.h"
#include "../Rendering/Particle3D.h"

class Player {
private:
    // Position (free movement)
    Vec3 position;
    Vec3 velocity;

    // Rotation (follows mouse)
    float aimAngle;          // Angle toward mouse cursor

    // Physics
    float acceleration;
    float maxSpeed;
    float drag;

    // Visual
    float size;
    Mesh3D mesh;

	float r, g, b;  // Color

    float fuel = 100.0f;      // 0-100%
    float maxFuel = 100.0f;
    float fuelConsumptionRate = 0.5f;  // Per second at max speed

    // Health system
    float health = 100.0f;
    float maxHealth = 100.0f;
    bool isDead = false;
    float respawnTimer = 0.0f;
    float respawnDelay = 1.0f;  // 3 seconds to respawn

    ParticleSystem3D* thrusterParticles;

public:
    Player();
    ~Player() { delete thrusterParticles; }

    void Init();
    void Update(float deltaTime);
    void DrawFuelBarWorldSpace(Vec3 playerPos, float fuel, float maxFuel, const Camera3D& camera);
    void DrawHealthBarWorldSpace(Vec3 playerPos, float health, float maxHealth, const Camera3D& camera);
    void DrawResourceBar(const Vec3& worldPos, float value, const Camera3D& camera, float r, float g, float b);
    void Render(const Camera3D& camera);

    // Get aim direction for shooting
    Vec3 GetAimDirection() const;

    // Getters
    Vec3 GetPosition() const { return position; }
    float GetAngle() const { return aimAngle; }
    Vec3 GetVelocityVector() const { return velocity; }

    // Setters
    void SetPosition(const Vec3& pos) { position = pos; }

    // Apply knockback force to player
    void ApplyKnockback(const Vec3& force) {
        velocity.x -= force.x;
        velocity.y -= force.y;
        velocity.z -= force.z;
    }

    float GetFuel() const { return fuel; }
    void RefillFuel(float amount);
    void UpdateFuel(bool isThrusting, float dtSeconds);

    // Health methods
    void TakeDamage(float damage);
    void Die();
    void Respawn();
    void RefillHealth(float amount);
    bool IsDead() const { return isDead; }
    float GetHealth() const { return health; }
    void UpdateRespawn(float deltaTime);
};

#endif
