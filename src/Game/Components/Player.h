//------------------------------------------------------------------------
// Player.h - Free movement space shooter
//------------------------------------------------------------------------
#ifndef PLAYER_H
#define PLAYER_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Rendering/Renderer3D.h"
#include "../Rendering/Mesh3D.h"

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

public:
    Player();

    void Init();
    void Update(float deltaTime);
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
};

#endif
