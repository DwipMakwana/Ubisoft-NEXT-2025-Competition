//-----------------------------------------------------------------------------
// RopeSystem.cpp - Verlet integration rope physics
//-----------------------------------------------------------------------------
#include "RopeSystem.h"
#include "../Utilities/Logger.h"
#include <cmath>

// Forward declaration of helper function
void DrawRopeLine(const Vec3& start, const Vec3& end, const Camera3D& camera,
    float r, float g, float b);

RopeSystem::RopeSystem()
    : segmentLength(2.0f)
    , stiffness(0.8f)
    , damping(0.98f)
    , solverIterations(3)
    , gravity(0, -5.0f, 0)
{
}

void RopeSystem::CreateRope(const Vec3& start, const Vec3& end, int numSegments) {
    nodes.clear();

    if (numSegments < 2) numSegments = 2;

    // Calculate segment length
    Vec3 delta = end - start;
    float totalLength = delta.Length();
    segmentLength = totalLength / (float)(numSegments - 1);

    // Create nodes along the line
    for (int i = 0; i < numSegments; i++) {
        RopeNode node;
        float t = (float)i / (float)(numSegments - 1);
        node.position = start + delta * t;
        node.oldPosition = node.position;
        node.mass = 0.5f;

        // Lock first and last nodes (anchors)
        node.locked = (i == 0 || i == numSegments - 1);

        nodes.push_back(node);
    }

    Logger::LogFormat("Rope created: %d nodes, segment length: %.2f\n",
        numSegments, segmentLength);
}

void RopeSystem::Update(float deltaTime) {
    if (nodes.size() < 2) return;

    float dt = deltaTime / 1000.0f;
    if (dt > 0.033f) dt = 0.033f;  // Cap at 30 FPS to prevent instability

    // Verlet integration
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i].locked) continue;

        // Store current position
        Vec3 temp = nodes[i].position;

        // Verlet: newPos = pos + (pos - oldPos) * damping + acceleration * dt^2
        Vec3 velocity = (nodes[i].position - nodes[i].oldPosition) * damping;
        Vec3 acceleration = gravity;  // Apply gravity

        nodes[i].position = nodes[i].position + velocity + acceleration * (dt * dt);
        nodes[i].oldPosition = temp;
    }

    // Constraint solver - enforce segment lengths
    for (int iteration = 0; iteration < solverIterations; iteration++) {
        ApplyConstraints();
    }
}

void RopeSystem::ApplyConstraints() {
    // Satisfy distance constraints between adjacent nodes
    for (size_t i = 0; i < nodes.size() - 1; i++) {
        SatisfyConstraint(i, i + 1);
    }
}

void RopeSystem::SatisfyConstraint(int idxA, int idxB) {
    RopeNode& nodeA = nodes[idxA];
    RopeNode& nodeB = nodes[idxB];

    Vec3 delta = nodeB.position - nodeA.position;
    float currentLength = delta.Length();

    if (currentLength < 0.001f) return;  // Avoid division by zero

    // Calculate correction
    float difference = (currentLength - segmentLength) / currentLength;
    Vec3 correction = delta * (difference * stiffness * 0.5f);

    // Apply correction based on whether nodes are locked
    if (!nodeA.locked && !nodeB.locked) {
        nodeA.position = nodeA.position + correction;
        nodeB.position = nodeB.position - correction;
    }
    else if (!nodeA.locked) {
        nodeA.position = nodeA.position + correction * 2.0f;
    }
    else if (!nodeB.locked) {
        nodeB.position = nodeB.position - correction * 2.0f;
    }
}

void RopeSystem::SetAnchorPoints(const Vec3& playerPos, const Vec3& asteroidPos) {
    if (nodes.empty()) return;

    // Update anchor positions
    nodes.front().position = playerPos;
    nodes.front().oldPosition = playerPos;

    nodes.back().position = asteroidPos;
    nodes.back().oldPosition = asteroidPos;
}

void RopeSystem::Render(const Camera3D& camera, float r, float g, float b) {
    if (nodes.size() < 2) return;

    // Draw rope as connected line segments with THICKER lines
    for (size_t i = 0; i < nodes.size() - 1; i++) {
        Vec3 start = nodes[i].position;
        Vec3 end = nodes[i + 1].position;

        float length = (end - start).Length();
        if (length < 0.01f) continue;

        // Draw MULTIPLE offset lines for thickness
        DrawRopeLine(start, end, camera, r, g, b);

        // Add offset lines for thickness
        Vec3 camRight = camera.GetRight();
        Vec3 camUp = camera.GetUp();

        Vec3 offset1 = camRight * 0.08f;
        Vec3 offset2 = camUp * 0.08f;

        DrawRopeLine(start + offset1, end + offset1, camera, r * 0.8f, g * 0.8f, b * 0.8f);
        DrawRopeLine(start - offset1, end - offset1, camera, r * 0.8f, g * 0.8f, b * 0.8f);
        DrawRopeLine(start + offset2, end + offset2, camera, r * 0.8f, g * 0.8f, b * 0.8f);
        DrawRopeLine(start - offset2, end - offset2, camera, r * 0.8f, g * 0.8f, b * 0.8f);
    }

    // Draw nodes as LARGER spheres for visibility
    for (const auto& node : nodes) {
        Renderer3D::DrawSphere(node.position, 0.15f, camera, r, g, b, 2, false);
    }
}

// Helper function to draw a line in 3D using thin triangles
void DrawRopeLine(const Vec3& start, const Vec3& end, const Camera3D& camera,
    float r, float g, float b) {
    Vec3 camRight = camera.GetRight();
    Vec3 offset = camRight * 0.05f;

    Vec3 p1 = start + offset;
    Vec3 p2 = start - offset;
    Vec3 p3 = end + offset;
    Vec3 p4 = end - offset;

    Renderer3D::DrawTriangle3D(p1, p2, p3, camera, r, g, b, r, g, b, r, g, b, false);
    Renderer3D::DrawTriangle3D(p2, p4, p3, camera, r, g, b, r, g, b, r, g, b, false);
}

float RopeSystem::GetCurrentLength() const {
    float length = 0.0f;
    for (size_t i = 0; i < nodes.size() - 1; i++) {
        length += (nodes[i + 1].position - nodes[i].position).Length();
    }
    return length;
}

void RopeSystem::Clear() {
    nodes.clear();
}
