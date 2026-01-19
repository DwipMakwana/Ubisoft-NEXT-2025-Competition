//-----------------------------------------------------------------------------
// RopeSystem.h - Physics-based rope for asteroid towing
//-----------------------------------------------------------------------------
#ifndef ROPE_SYSTEM_H
#define ROPE_SYSTEM_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Rendering/Renderer3D.h"
#include <vector>

// Verlet integration point for rope simulation
struct RopeNode {
    Vec3 position;
    Vec3 oldPosition;
    float mass;
    bool locked;  // true = fixed in place (anchor points)

    RopeNode() : position(0, 0, 0), oldPosition(0, 0, 0), mass(1.0f), locked(false) {}
};

class RopeSystem {
public:
    std::vector<RopeNode> nodes;
    float segmentLength;      // Rest length between nodes
    float stiffness;          // How rigid the rope is (0-1)
    float damping;            // Energy loss (0-1)
    int solverIterations;     // More = more stable but slower

    Vec3 gravity;

    RopeSystem();

    // Initialize rope with N segments between two points
    void CreateRope(const Vec3& start, const Vec3& end, int numSegments);

    // Update simulation
    void Update(float deltaTime);

    // Set anchor points (player and asteroid positions)
    void SetAnchorPoints(const Vec3& playerPos, const Vec3& asteroidPos);

    // Render the rope
    void Render(const Camera3D& camera, float r = 0.7f, float g = 0.6f, float b = 0.3f);

    // Get total rope length
    float GetCurrentLength() const;

    // Clear rope
    void Clear();

private:
    void ApplyConstraints();
    void SatisfyConstraint(int nodeA, int nodeB);
};

#endif // ROPE_SYSTEM_H
