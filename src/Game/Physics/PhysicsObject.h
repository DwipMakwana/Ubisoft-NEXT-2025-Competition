//-----------------------------------------------------------------------------
// PhysicsObject.h
// Unified physics object with proper rotation and inertia
//-----------------------------------------------------------------------------

#ifndef PHYSICS_OBJECT_H
#define PHYSICS_OBJECT_H

#include "../Utilities/MathUtils3D.h"
#include "Collision3D.h"
#include "../Rendering/Mesh3D.h"

enum class ColliderType {
    SPHERE,
    BOX,
    PLANE,
    CONVEX_HULL,
    CYLINDER,
};

struct Material {
    float restitution;  // Bounciness (0-1)
    float staticFriction;  // Static friction coefficient
    float dynamicFriction; // Dynamic friction coefficient

    Material() : restitution(0.4f), staticFriction(0.6f), dynamicFriction(0.4f) {}
};

class PhysicsObject {
public:
    ConvexHull3D* convexHull;
    Mesh3D* renderMeshRef;

    // Track if object is resting on ground
    bool isGrounded;
    float groundedTime;

    // Transform
    Vec3 position;
    Vec3 rotation;      // Euler angles in radians (for rendering)
    Vec3 scale;

    // Linear motion
    Vec3 velocity;
    Vec3 force;         // Force accumulator
    float mass;
    float invMass;

    // Angular motion
    Vec3 angularVelocity; // Radians per second
    Vec3 torque;          // Torque accumulator
    Vec3 inertia;         // Diagonal inertia tensor (simplified)
    Vec3 invInertia;

    // Collider
    ColliderType colliderType;
    float radius;       // For sphere/capsule
    Vec3 halfExtents;   // For box (half-sizes)
    float height;       // For capsule

    // Material
    Material material;

    // Damping
    float linearDamping;
    float angularDamping;

    // State
    bool isStatic;
    bool useGravity;
    bool isSleeping;
    float sleepTimer;
    float sleepThreshold;

	// Lifetime
    float lifetime;
    float maxLifetime;

    // Constructor
    PhysicsObject(const Vec3& pos = Vec3(0, 0, 0), float r = 1.0f, ColliderType type = ColliderType::SPHERE);

    // Setters
    void SetMass(float m);
    void SetBox(const Vec3& size);
    void SetSphere(float r);
    void SetCylinder(float r, float h);

    // Force/Torque application
    void AddForce(const Vec3& f);
    void AddForceAtPoint(const Vec3& f, const Vec3& point);
    void AddTorque(const Vec3& t);
    void ClearForces();

    // Inertia calculation
    void UpdateInertia();

    // Sleep system
    void UpdateSleep(float dt);
    void WakeUp();

    void SetConvexHull(const Mesh3D& mesh);
};

#endif // PHYSICS_OBJECT_H
