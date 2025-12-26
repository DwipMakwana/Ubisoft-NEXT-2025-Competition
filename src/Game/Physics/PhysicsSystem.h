//-----------------------------------------------------------------------------
// PhysicsSystem.h
// Main physics simulation with proper rotation and friction
//-----------------------------------------------------------------------------

#ifndef PHYSICSSYSTEM_H
#define PHYSICSSYSTEM_H

#include "PhysicsObject.h"
#include "../Rendering/Camera3D.h"
#include "../Rendering/Mesh3D.h"
#include <vector>

// Forward declarations from Collision3D.h
struct Contact3D;
struct ContactManifold;
struct Sphere3D;
struct OBB3D;
struct Plane3D;

class PhysicsSystem {
public:
    std::vector<PhysicsObject> bodies;
    Vec3 gravity;
    Plane3D* groundPlane;  // Changed to pointer

    // Solver settings
    int velocityIterations;
    int positionIterations;
    float baumgarte;  // Position correction factor

    PhysicsSystem();
    ~PhysicsSystem();

    // Main API
    PhysicsObject& SpawnBody(int type);
    void Update(float dt);
    void Render(Camera3D& camera, Mesh3D& sphereMesh, Mesh3D& cubeMesh, bool wireframe);

private:
    // Integration
    void IntegrateForces(PhysicsObject& obj, float dt);
    void IntegrateVelocity(PhysicsObject& obj, float dt);

    // Collision detection
    bool DetectCollision(PhysicsObject& a, PhysicsObject& b, ContactManifold& manifold);
    bool DetectGroundCollision(PhysicsObject& obj, ContactManifold& manifold);

    // Collision resolution
    void ResolveContact(PhysicsObject& a, PhysicsObject& b, const Contact3D& contact);
    void ResolveGroundContact(PhysicsObject& obj, const Contact3D& contact);

    // Helper to convert PhysicsObject to collision shapes
    void GetSphere(const PhysicsObject& obj, Sphere3D& sphere);
    void GetOBB(const PhysicsObject& obj, OBB3D& obb);
};

#endif // PHYSICSSYSTEM_H
