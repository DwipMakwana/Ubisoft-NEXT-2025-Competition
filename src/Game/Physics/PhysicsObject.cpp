//-----------------------------------------------------------------------------
// PhysicsObject.cpp
//-----------------------------------------------------------------------------

#include "PhysicsObject.h"
#include <cmath>

PhysicsObject::PhysicsObject(const Vec3& pos, float r, ColliderType type)
    : position(pos)
    , rotation(0, 0, 0)
    , scale(1, 1, 1)
    , velocity(0, 0, 0)
    , force(0, 0, 0)
    , mass(1.0f)
    , invMass(1.0f)
    , angularVelocity(0, 0, 0)
    , torque(0, 0, 0)
    , inertia(1, 1, 1)
    , invInertia(1, 1, 1)
    , colliderType(type)
    , radius(r)
    , halfExtents(r, r, r)
    , height(r * 2.0f)
    , linearDamping(0.01f)
    , angularDamping(0.05f)
    , isStatic(false)
    , useGravity(true)
    , isSleeping(false)
    , sleepTimer(0.0f)
    , lifetime(0.0f)       
    , maxLifetime(30.0f)
    , convexHull(nullptr)
    , renderMeshRef(nullptr)
    , sleepThreshold(0.5f)
    , isGrounded(false)
    , groundedTime(0.0f)
{
    if (type == ColliderType::BOX) {
        SetBox(Vec3(r, r, r));
    }
    else {
        SetSphere(r);
    }
}

void PhysicsObject::SetMass(float m) {
    if (m <= 0.0f || isStatic) {
        mass = 0.0f;
        invMass = 0.0f;
    }
    else {
        mass = m;
        invMass = 1.0f / m;
    }
    UpdateInertia();
}

void PhysicsObject::SetBox(const Vec3& size) {
    halfExtents = size;
    colliderType = ColliderType::BOX;
    UpdateInertia();
}

void PhysicsObject::SetSphere(float r) {
    radius = r;
    colliderType = ColliderType::SPHERE;
    UpdateInertia();
}

void PhysicsObject::SetCylinder(float r, float h) {
    colliderType = ColliderType::CYLINDER;
    radius = r;
    height = h;

    // Initialize halfExtents for collision purposes (treat as box)
    halfExtents = Vec3(r, h * 0.5f, r);

    // Calculate inertia for cylinder
    float r2 = r * r;
    float h2 = h * h;

    inertia.x = mass * (3.0f * r2 + h2) / 12.0f;
    inertia.y = mass * r2 / 2.0f;
    inertia.z = mass * (3.0f * r2 + h2) / 12.0f;

    invInertia.x = (inertia.x > 0) ? (1.0f / inertia.x) : 0;
    invInertia.y = (inertia.y > 0) ? (1.0f / inertia.y) : 0;
    invInertia.z = (inertia.z > 0) ? (1.0f / inertia.z) : 0;
}

void PhysicsObject::UpdateInertia() {
    if (invMass == 0.0f) {
        inertia = Vec3(0, 0, 0);
        invInertia = Vec3(0, 0, 0);
        return;
    }

    // Calculate moment of inertia based on shape
    if (colliderType == ColliderType::SPHERE) {
        // Solid sphere: I = (2/5) * m * r^2
        float I = 0.4f * mass * radius * radius;
        inertia = Vec3(I, I, I);
    }
    else if (colliderType == ColliderType::BOX) {
        // Box inertia tensor (diagonal)
        float x2 = halfExtents.x * halfExtents.x;
        float y2 = halfExtents.y * halfExtents.y;
        float z2 = halfExtents.z * halfExtents.z;

        inertia.x = (mass / 3.0f) * (y2 + z2);
        inertia.y = (mass / 3.0f) * (x2 + z2);
        inertia.z = (mass / 3.0f) * (x2 + y2);
    }

    // Calculate inverse inertia
    invInertia.x = (inertia.x > 0.0f) ? 1.0f / inertia.x : 0.0f;
    invInertia.y = (inertia.y > 0.0f) ? 1.0f / inertia.y : 0.0f;
    invInertia.z = (inertia.z > 0.0f) ? 1.0f / inertia.z : 0.0f;
}

void PhysicsObject::AddForce(const Vec3& f) {
    force = force + f;
    WakeUp();
}

void PhysicsObject::AddForceAtPoint(const Vec3& f, const Vec3& point) {
    // Add linear force
    force = force + f;

    // Calculate torque = r × F
    Vec3 r = point - position;
    Vec3 t = r.Cross(f);
    torque = torque + t;

    WakeUp();
}

void PhysicsObject::AddTorque(const Vec3& t) {
    torque = torque + t;
    WakeUp();
}

void PhysicsObject::ClearForces() {
    force = Vec3(0, 0, 0);
    torque = Vec3(0, 0, 0);
}

void PhysicsObject::UpdateSleep(float dt) {
    float speed = velocity.Length();
    float angularSpeed = angularVelocity.Length();

    // More strict sleep conditions
    if (speed < 0.02f && angularSpeed < 0.05f && isGrounded) {
        sleepTimer += dt;
        if (sleepTimer > 0.2f) {  // Sleep after only 0.2 seconds
            isSleeping = true;
            velocity = Vec3(0, 0, 0);
            angularVelocity = Vec3(0, 0, 0);

            // Snap boxes to flat orientation
            if (colliderType == ColliderType::BOX) {
                rotation.x = roundf(rotation.x / 1.5708f) * 1.5708f;
                rotation.z = roundf(rotation.z / 1.5708f) * 1.5708f;
            }
        }
    }
    else {
        sleepTimer = 0.0f;
        isSleeping = false;
    }
}

void PhysicsObject::WakeUp() {
    isSleeping = false;
    sleepTimer = 0.0f;
}

void PhysicsObject::SetConvexHull(const Mesh3D& mesh) {
    if (convexHull) delete convexHull;

    convexHull = new ConvexHull3D();
    colliderType = ColliderType::CONVEX_HULL;

    // Extract vertex positions
    convexHull->vertices.clear();
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        convexHull->vertices.push_back(mesh.vertices[i].position);
    }

    // Calculate center of mass
    Vec3 center(0, 0, 0);
    for (const Vec3& v : convexHull->vertices) {
        center = center + v;
    }
    center = center * (1.0f / (float)convexHull->vertices.size());
    convexHull->center = center;

    // Generate face normals using a simple approach
    // We'll use the vertex normals if available, or generate basic faces
    std::vector<Vec3> faceNormals;

    // Try to extract normals from vertex data
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        Vec3 normal = mesh.vertices[i].normal;  // Assuming vertices have normals

        // Check if unique
        bool found = false;
        for (const Vec3& existing : faceNormals) {
            if (fabsf(normal.Dot(existing)) > 0.98f) {
                found = true;
                break;
            }
        }

        if (!found && normal.Length() > 0.01f) {
            faceNormals.push_back(normal.Normalized());
        }

        // Limit number of faces for performance
        if (faceNormals.size() >= 20) break;
    }

    // Fallback: use 6 axis-aligned directions
    if (faceNormals.empty()) {
        faceNormals.push_back(Vec3(1, 0, 0));
        faceNormals.push_back(Vec3(-1, 0, 0));
        faceNormals.push_back(Vec3(0, 1, 0));
        faceNormals.push_back(Vec3(0, -1, 0));
        faceNormals.push_back(Vec3(0, 0, 1));
        faceNormals.push_back(Vec3(0, 0, -1));
    }

    convexHull->faces = faceNormals;

    // Calculate bounding radius
    float maxRadius = 0.0f;
    for (const Vec3& v : convexHull->vertices) {
        float r = (v - center).Length();
        if (r > maxRadius) maxRadius = r;
    }

    this->radius = maxRadius;
    this->halfExtents = Vec3(maxRadius, maxRadius, maxRadius);
    SetMass(mass);
}
