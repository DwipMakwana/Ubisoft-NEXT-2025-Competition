//-----------------------------------------------------------------------------
// PhysicsSystem.cpp
// Impulse-based physics with realistic rotation
//-----------------------------------------------------------------------------

#include "PhysicsSystem.h"
#include "Collision3D.h"
#include "../Rendering/Renderer3D.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

PhysicsSystem::PhysicsSystem()
    : gravity(0, -9.8f, 0)
    , groundPlane(nullptr)
    , velocityIterations(4)
    , positionIterations(3)
    , baumgarte(0.3f)
{
    // Create ground plane
    groundPlane = new Plane3D(Vec3(0, 1, 0), 0.0f);
}

PhysicsSystem::~PhysicsSystem() {
    delete groundPlane;
}

PhysicsObject& PhysicsSystem::SpawnBody(int type) {
    PhysicsObject obj;

    // Random spawn position
    float x = ((rand() % 100) / 100.0f - 0.5f) * 4.0f;
    float z = ((rand() % 100) / 100.0f - 0.5f) * 4.0f;
    obj.position = Vec3(x, 5.0f, z);

    // Random initial rotation
    obj.rotation = Vec3(
        ((rand() % 100) / 100.0f) * 6.28f,
        ((rand() % 100) / 100.0f) * 6.28f,
        ((rand() % 100) / 100.0f) * 6.28f
    );

    // Random initial angular velocity
    obj.angularVelocity = Vec3(
        ((rand() % 100) / 100.0f - 0.5f) * 4.0f,
        ((rand() % 100) / 100.0f - 0.5f) * 4.0f,
        ((rand() % 100) / 100.0f - 0.5f) * 4.0f
    );

    obj.isStatic = false;
    obj.useGravity = true;
    obj.mass = 1.0f;
    obj.lifetime = 0.0f;
    obj.maxLifetime = 5.0f;

    if (type == 0) {
        // Sphere
        obj.colliderType = ColliderType::SPHERE;
        obj.SetSphere(0.5f);
        obj.SetMass(1.0f);
        obj.material.restitution = 0.3f;
        obj.material.staticFriction = 0.6f;
        obj.material.dynamicFriction = 0.4f;
    }
    else {
        // Box
        obj.colliderType = ColliderType::BOX;
        Vec3 size(0.5f, 0.5f, 0.5f);
        obj.SetBox(size);
        obj.SetMass(1.0f);
        obj.material.restitution = 0.2f;
        obj.material.staticFriction = 0.7f;
        obj.material.dynamicFriction = 0.5f;
    }

    bodies.push_back(obj);
    return bodies.back();
}

//-----------------------------------------------------------------------------
// Integration
//-----------------------------------------------------------------------------

void PhysicsSystem::IntegrateForces(PhysicsObject& obj, float dt) {
    if (obj.isStatic || obj.isSleeping) return;

    // Apply gravity
    if (obj.useGravity) {
        obj.AddForce(gravity * obj.mass);
    }

    // Integrate linear velocity
    Vec3 acceleration = obj.force * obj.invMass;
    obj.velocity = obj.velocity + acceleration * dt;

    // Apply linear damping
    obj.velocity = obj.velocity * (1.0f - obj.linearDamping);

    // Integrate angular velocity
    Vec3 angularAccel(
        obj.torque.x * obj.invInertia.x,
        obj.torque.y * obj.invInertia.y,
        obj.torque.z * obj.invInertia.z
    );
    obj.angularVelocity = obj.angularVelocity + angularAccel * dt;

    // Apply angular damping
    obj.angularVelocity = obj.angularVelocity * (1.0f - obj.angularDamping);

    // Clear forces
    obj.ClearForces();
}

void PhysicsSystem::IntegrateVelocity(PhysicsObject& obj, float dt) {
    if (obj.isStatic || obj.isSleeping) return;

    // Update position
    obj.position = obj.position + obj.velocity * dt;

    // Update rotation (Euler integration - good enough for this)
    obj.rotation = obj.rotation + obj.angularVelocity * dt;

    // Keep rotation in range [0, 2π]
    const float TWO_PI = 6.28318530718f;
    obj.rotation.x = fmodf(obj.rotation.x, TWO_PI);
    obj.rotation.y = fmodf(obj.rotation.y, TWO_PI);
    obj.rotation.z = fmodf(obj.rotation.z, TWO_PI);
}

//-----------------------------------------------------------------------------
// Collision Detection
//-----------------------------------------------------------------------------

void PhysicsSystem::GetSphere(const PhysicsObject& obj, Sphere3D& sphere) {
    sphere.center = obj.position;
    sphere.radius = obj.radius;
}

void PhysicsSystem::GetOBB(const PhysicsObject& obj, OBB3D& obb) {
    obb.center = obj.position;
    obb.halfExtents = obj.halfExtents;
    obb.SetRotation(obj.rotation);
}

bool PhysicsSystem::DetectCollision(PhysicsObject& a, PhysicsObject& b, ContactManifold& manifold) {
    // Broad phase
    float maxRadius = (a.colliderType == ColliderType::SPHERE) ? a.radius : a.halfExtents.Length();
    maxRadius += (b.colliderType == ColliderType::SPHERE) ? b.radius : b.halfExtents.Length();

    if ((b.position - a.position).LengthSquared() > maxRadius * maxRadius * 4.0f) {
        return false;
    }

    // Narrow phase
    if (a.colliderType == ColliderType::SPHERE && b.colliderType == ColliderType::SPHERE) {
        Sphere3D sphereA, sphereB;
        GetSphere(a, sphereA);
        GetSphere(b, sphereB);
        return Collision3D::SphereSphere(sphereA, sphereB, manifold);
    }
    else if (a.colliderType == ColliderType::BOX && b.colliderType == ColliderType::BOX) {
        OBB3D obbA, obbB;
        GetOBB(a, obbA);
        GetOBB(b, obbB);
        return Collision3D::OBBOBB(obbA, obbB, manifold);
    }
    else {
        // Sphere vs Box
        // SphereOBB returns normal pointing from BOX to SPHERE
        // We need normal pointing from A to B

        Sphere3D s;
        OBB3D o;

        if (a.colliderType == ColliderType::SPHERE) {
            // A = sphere, B = box
            GetSphere(a, s);
            GetOBB(b, o);

            bool hit = Collision3D::SphereOBB(s, o, manifold);

            // SphereOBB normal points box->sphere
            // We need A->B which is sphere->box
            // So FLIP the normal
            if (hit) {
                for (int i = 0; i < manifold.numContacts; i++) {
                    manifold.contacts[i].normal = manifold.contacts[i].normal * -1.0f;
                }
            }
            return hit;
        }
        else {
            // A = box, B = sphere
            GetSphere(b, s);
            GetOBB(a, o);

            bool hit = Collision3D::SphereOBB(s, o, manifold);

            // SphereOBB normal points box->sphere which is A->B
            // This is correct, NO flip needed
            return hit;
        }
    }

    return false;
}

bool PhysicsSystem::DetectGroundCollision(PhysicsObject& obj, ContactManifold& manifold) {
    if (obj.colliderType == ColliderType::SPHERE) {
        Sphere3D sphere;
        GetSphere(obj, sphere);

        // Check if sphere center is within plane bounds
        const float PLANE_HALF_SIZE = 10.0f;
        if (fabsf(sphere.center.x) > PLANE_HALF_SIZE ||
            fabsf(sphere.center.z) > PLANE_HALF_SIZE) {
            return false;  // Outside bounds, no collision
        }

        return Collision3D::SpherePlane(sphere, *groundPlane, manifold);
    }
    else if (obj.colliderType == ColliderType::BOX) {
        OBB3D obb;
        GetOBB(obj, obb);

        // Use bounded plane collision (20x20 = ±10 units)
        const float PLANE_HALF_SIZE = 10.0f;
        return Collision3D::OBBPlane(obb, *groundPlane, manifold, PLANE_HALF_SIZE, PLANE_HALF_SIZE);
    }
    return false;
}

//-----------------------------------------------------------------------------
// Collision Resolution (Sequential Impulse)
//-----------------------------------------------------------------------------

void PhysicsSystem::ResolveContact(PhysicsObject& a, PhysicsObject& b, const Contact3D& contact) {

    Vec3 ra = contact.point - a.position;
    Vec3 rb = contact.point - b.position;

    Vec3 va = a.velocity + a.angularVelocity.Cross(ra);
    Vec3 vb = b.velocity + b.angularVelocity.Cross(rb);
    Vec3 relVel = vb - va;

    float velAlongNormal = relVel.Dot(contact.normal);

    // CRITICAL: Skip if separating OR penetration is tiny
    if (velAlongNormal > 0.05f) return;  // Objects moving apart
    if (contact.penetration < 0.005f) return;  // Too small to matter

    // Add velocity bias to prevent sinking
    float velocityBias = 0.1f * contact.penetration / 0.016f; // Assumes 60 FPS

    float e = (a.material.restitution + b.material.restitution) * 0.5f;

    // Calculate impulse with bias
    float angularEffect = 0.0f;
    if (a.invMass > 0.0f) {
        Vec3 raCrossN = ra.Cross(contact.normal);
        angularEffect += raCrossN.x * raCrossN.x * a.invInertia.x +
            raCrossN.y * raCrossN.y * a.invInertia.y +
            raCrossN.z * raCrossN.z * a.invInertia.z;
    }
    if (b.invMass > 0.0f) {
        Vec3 rbCrossN = rb.Cross(contact.normal);
        angularEffect += rbCrossN.x * rbCrossN.x * b.invInertia.x +
            rbCrossN.y * rbCrossN.y * b.invInertia.y +
            rbCrossN.z * rbCrossN.z * b.invInertia.z;
    }

    // Apply bias to push objects apart
    float j = -(1.0f + e) * velAlongNormal + velocityBias;
    j /= (a.invMass + b.invMass + angularEffect);

    // Clamp impulse to prevent explosion
    if (j < 0.0f) return;  // Don't pull objects together!

    Vec3 impulse = contact.normal * j;

    a.velocity = a.velocity - impulse * a.invMass;
    b.velocity = b.velocity + impulse * b.invMass;

    if (a.invMass > 0.0f) {
        Vec3 torqueA = ra.Cross(impulse);
        a.angularVelocity.x -= torqueA.x * a.invInertia.x;
        a.angularVelocity.y -= torqueA.y * a.invInertia.y;
        a.angularVelocity.z -= torqueA.z * a.invInertia.z;
    }

    if (b.invMass > 0.0f) {
        Vec3 torqueB = rb.Cross(impulse);
        b.angularVelocity.x += torqueB.x * b.invInertia.x;
        b.angularVelocity.y += torqueB.y * b.invInertia.y;
        b.angularVelocity.z += torqueB.z * b.invInertia.z;
    }

    // Friction (same as before)
    Vec3 tangent = relVel - contact.normal * velAlongNormal;
    float tangentLength = tangent.Length();

    if (tangentLength > 0.001f) {
        tangent = tangent / tangentLength;
        float friction = (a.material.dynamicFriction + b.material.dynamicFriction) * 0.5f;

        float angularEffectT = 0.0f;
        if (a.invMass > 0.0f) {
            Vec3 raCrossT = ra.Cross(tangent);
            angularEffectT += raCrossT.x * raCrossT.x * a.invInertia.x +
                raCrossT.y * raCrossT.y * a.invInertia.y +
                raCrossT.z * raCrossT.z * a.invInertia.z;
        }
        if (b.invMass > 0.0f) {
            Vec3 rbCrossT = rb.Cross(tangent);
            angularEffectT += rbCrossT.x * rbCrossT.x * b.invInertia.x +
                rbCrossT.y * rbCrossT.y * b.invInertia.y +
                rbCrossT.z * rbCrossT.z * b.invInertia.z;
        }

        float jt = -tangentLength / (a.invMass + b.invMass + angularEffectT);
        float maxFriction = friction * fabsf(j);
        if (jt > maxFriction) jt = maxFriction;
        if (jt < -maxFriction) jt = -maxFriction;

        Vec3 frictionImpulse = tangent * jt;

        a.velocity = a.velocity - frictionImpulse * a.invMass;
        b.velocity = b.velocity + frictionImpulse * b.invMass;

        if (a.invMass > 0.0f) {
            Vec3 torqueA = ra.Cross(frictionImpulse);
            a.angularVelocity.x -= torqueA.x * a.invInertia.x;
            a.angularVelocity.y -= torqueA.y * a.invInertia.y;
            a.angularVelocity.z -= torqueA.z * a.invInertia.z;
        }

        if (b.invMass > 0.0f) {
            Vec3 torqueB = rb.Cross(frictionImpulse);
            b.angularVelocity.x += torqueB.x * b.invInertia.x;
            b.angularVelocity.y += torqueB.y * b.invInertia.y;
            b.angularVelocity.z += torqueB.z * b.invInertia.z;
        }
    }

    // Position correction - very gentle
    const float percent = 0.2f;  // Only correct 20%
    const float slop = 0.02f;     // Larger slop
    float correctionAmount = contact.penetration - slop;
    if (correctionAmount > 0.0f) {
        Vec3 correction = contact.normal * (correctionAmount * percent);
        float totalInvMass = a.invMass + b.invMass;
        if (totalInvMass > 0.0f) {
            a.position = a.position - correction * (a.invMass / totalInvMass);
            b.position = b.position + correction * (b.invMass / totalInvMass);
        }
    }
}

void PhysicsSystem::ResolveGroundContact(PhysicsObject& obj, const Contact3D& contact) {
    Vec3 r = contact.point - obj.position;
    Vec3 v = obj.velocity + obj.angularVelocity.Cross(r);
    float velAlongNormal = v.Dot(contact.normal);

    // Don't resolve if separating
    if (velAlongNormal > 0.1f || contact.penetration < 0.001f) return;

    float e = obj.material.restitution * 0.3f; // Ground dampens more

    Vec3 rCrossN = r.Cross(contact.normal);
    float angularEffect = rCrossN.x * rCrossN.x * obj.invInertia.x +
        rCrossN.y * rCrossN.y * obj.invInertia.y +
        rCrossN.z * rCrossN.z * obj.invInertia.z;

    float j = -(1.0f + e) * velAlongNormal / (obj.invMass + angularEffect);
    Vec3 impulse = contact.normal * j;

    obj.velocity = obj.velocity + impulse * obj.invMass;

    Vec3 torque = r.Cross(impulse);
    obj.angularVelocity.x += torque.x * obj.invInertia.x;
    obj.angularVelocity.y += torque.y * obj.invInertia.y;
    obj.angularVelocity.z += torque.z * obj.invInertia.z;

    // Friction
    Vec3 tangent = v - contact.normal * velAlongNormal;
    float tangentLen = tangent.Length();
    if (tangentLen > 0.001f) {
        tangent = tangent / tangentLen;
        Vec3 rCrossT = r.Cross(tangent);
        float angularEffectT = rCrossT.x * rCrossT.x * obj.invInertia.x +
            rCrossT.y * rCrossT.y * obj.invInertia.y +
            rCrossT.z * rCrossT.z * obj.invInertia.z;

        float jt = -tangentLen / (obj.invMass + angularEffectT);
        float maxFriction = obj.material.dynamicFriction * fabsf(j);
        if (jt > maxFriction) jt = maxFriction;
        if (jt < -maxFriction) jt = -maxFriction;

        Vec3 frictionImpulse = tangent * jt;
        obj.velocity = obj.velocity + frictionImpulse * obj.invMass;

        Vec3 torqueF = r.Cross(frictionImpulse);
        obj.angularVelocity.x += torqueF.x * obj.invInertia.x;
        obj.angularVelocity.y += torqueF.y * obj.invInertia.y;
        obj.angularVelocity.z += torqueF.z * obj.invInertia.z;
    }

    // Position correction
    const float percent = 0.5f;
    const float slop = 0.01f;
    float correctionAmount = contact.penetration - slop;
    if (correctionAmount > 0.0f) {
        obj.position = obj.position + contact.normal * (correctionAmount * percent);
    }
}

//-----------------------------------------------------------------------------
// Main Update Loop
//-----------------------------------------------------------------------------

void PhysicsSystem::Update(float dt) {
    // Cap timestep
    if (dt > 0.033f) dt = 0.033f;

    // Step 1: Apply forces -> velocities
    for (auto& body : bodies) {
        IntegrateForces(body, dt);
        body.lifetime += dt;
    }

    // Step 2: Apply velocities -> positions  
    for (auto& body : bodies) {
        IntegrateVelocity(body, dt);
    }

    // Step 3: Collision detection and response
    // Ground collisions
    for (auto& body : bodies) {
        ContactManifold manifold;
        if (DetectGroundCollision(body, manifold)) {
            for (int i = 0; i < manifold.numContacts; i++) {
                ResolveGroundContact(body, manifold.contacts[i]);
            }
        }
    }

    // Object-object collisions
    for (size_t i = 0; i < bodies.size(); i++) {
        for (size_t j = i + 1; j < bodies.size(); j++) {
            ContactManifold manifold;
            if (DetectCollision(bodies[i], bodies[j], manifold)) {
                for (int k = 0; k < manifold.numContacts; k++) {
                    ResolveContact(bodies[i], bodies[j], manifold.contacts[k]);
                }
            }
        }
    }

    // Step 4: Update sleep
    for (auto& body : bodies) {
        body.UpdateSleep(dt);
    }

    bodies.erase(
        std::remove_if(bodies.begin(), bodies.end(),
            [](const PhysicsObject& obj) {
                // Remove if too old
                /*if (obj.lifetime > obj.maxLifetime) {
                    return true;
                }*/
                // Remove if fallen too far below ground
                if (obj.position.y < -10.0f) {
                    return true;
                }
                return false;
            }
        ),
        bodies.end()
    );
}

//-----------------------------------------------------------------------------
// Rendering
//-----------------------------------------------------------------------------

void PhysicsSystem::Render(Camera3D& camera, Mesh3D& sphereMesh, Mesh3D& cubeMesh, bool wireframe) {
    for (const auto& body : bodies) {
        // Convert rotation from radians to degrees for rendering
        Vec3 rotDegrees = body.rotation * (180.0f / 3.14159265359f);

        if (body.colliderType == ColliderType::SPHERE) {
            float r = body.radius;
            Renderer3D::DrawMesh(
                sphereMesh,
                body.position,
                rotDegrees,
                Vec3(r, r, r),
                camera,
                0.3f, 0.6f, 1.0f,
                wireframe
            );
        }
        else if (body.colliderType == ColliderType::BOX) {
            Vec3 size = body.halfExtents * 2.0f;
            Renderer3D::DrawMesh(
                cubeMesh,
                body.position,
                rotDegrees,
                size,
                camera,
                1.0f, 0.4f, 0.3f,
                wireframe
            );
        }
    }
}
