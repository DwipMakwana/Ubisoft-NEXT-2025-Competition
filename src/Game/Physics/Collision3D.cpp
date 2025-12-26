//-----------------------------------------------------------------------------
// Collision3D.cpp
//-----------------------------------------------------------------------------

#include "Collision3D.h"
#include <cmath>
#include <algorithm>

void OBB3D::SetRotation(const Vec3& eulerAngles) {
    // Build rotation matrix from Euler angles (XYZ order)
    float cx = cosf(eulerAngles.x), sx = sinf(eulerAngles.x);
    float cy = cosf(eulerAngles.y), sy = sinf(eulerAngles.y);
    float cz = cosf(eulerAngles.z), sz = sinf(eulerAngles.z);

    // Right axis
    axis[0].x = cy * cz;
    axis[0].y = sx * sy * cz + cx * sz;
    axis[0].z = -cx * sy * cz + sx * sz;

    // Up axis
    axis[1].x = -cy * sz;
    axis[1].y = -sx * sy * sz + cx * cz;
    axis[1].z = cx * sy * sz + sx * cz;

    // Forward axis
    axis[2].x = sy;
    axis[2].y = -sx * cy;
    axis[2].z = cx * cy;
}

//-----------------------------------------------------------------------------
// Sphere vs Sphere
//-----------------------------------------------------------------------------

bool Collision3D::TestSphereSphere(const Sphere3D& a, const Sphere3D& b) {
    float radiusSum = a.radius + b.radius;
    float distSq = (b.center - a.center).LengthSquared();
    return distSq <= radiusSum * radiusSum;
}

bool Collision3D::SphereSphere(const Sphere3D& a, const Sphere3D& b, ContactManifold& manifold) {
    manifold.Clear();

    Vec3 delta = b.center - a.center;
    float distSq = delta.LengthSquared();
    float radiusSum = a.radius + b.radius;

    if (distSq >= radiusSum * radiusSum) {
        return false;
    }

    float dist = sqrtf(distSq);

    // Handle overlapping spheres
    if (dist < 0.0001f) {
        manifold.AddContact(a.center, Vec3(0, 1, 0), radiusSum);
        return true;
    }

    Vec3 normal = delta / dist;
    float penetration = radiusSum - dist;
    Vec3 contactPoint = a.center + normal * a.radius;

    manifold.AddContact(contactPoint, normal, penetration);
    return true;
}

//-----------------------------------------------------------------------------
// OBB vs OBB using SAT
//-----------------------------------------------------------------------------

bool Collision3D::TestOBBOBB(const OBB3D& a, const OBB3D& b) {
    // Separating Axis Theorem test (15 axes)
    Vec3 axes[15];

    // Face normals of A (3)
    axes[0] = a.axis[0];
    axes[1] = a.axis[1];
    axes[2] = a.axis[2];

    // Face normals of B (3)
    axes[3] = b.axis[0];
    axes[4] = b.axis[1];
    axes[5] = b.axis[2];

    // Cross products (9)
    int idx = 6;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            axes[idx++] = a.axis[i].Cross(b.axis[j]);
        }
    }

    Vec3 T = b.center - a.center;

    for (int i = 0; i < 15; i++) {
        Vec3 axis = axes[i];
        float axisLen = axis.Length();

        if (axisLen < 0.0001f) continue; // Skip near-parallel axes
        axis = axis / axisLen;

        // Project boxes onto axis
        float ra = 0.0f, rb = 0.0f;
        for (int j = 0; j < 3; j++) {
            ra += fabsf(a.halfExtents.x * axis.Dot(a.axis[j]));
            rb += fabsf(b.halfExtents.x * axis.Dot(b.axis[j]));
        }

        float distance = fabsf(T.Dot(axis));

        if (distance > ra + rb) {
            return false; // Separating axis found
        }
    }

    return true; // No separating axis
}

bool Collision3D::OBBOBB(const OBB3D& a, const OBB3D& b, ContactManifold& manifold) {
    manifold.Clear();

    if (!TestOBBOBB(a, b)) {
        return false;
    }

    // Find the minimum penetration axis
    Vec3 axes[15];
    float penetrations[15];

    // Face normals of A (3)
    axes[0] = a.axis[0]; axes[1] = a.axis[1]; axes[2] = a.axis[2];

    // Face normals of B (3)
    axes[3] = b.axis[0]; axes[4] = b.axis[1]; axes[5] = b.axis[2];

    // Cross products (9)
    int idx = 6;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            axes[idx++] = a.axis[i].Cross(b.axis[j]);
        }
    }

    Vec3 T = b.center - a.center;

    float minPenetration = 999999.0f;
    int minAxis = -1;

    for (int i = 0; i < 15; i++) {
        Vec3 axis = axes[i];
        float axisLen = axis.Length();

        if (axisLen < 0.0001f) continue;
        axis = axis / axisLen;

        // Project boxes onto axis
        float ra = 0.0f, rb = 0.0f;

        // For box A
        ra = fabsf(a.halfExtents.x * axis.Dot(a.axis[0])) +
            fabsf(a.halfExtents.y * axis.Dot(a.axis[1])) +
            fabsf(a.halfExtents.z * axis.Dot(a.axis[2]));

        // For box B
        rb = fabsf(b.halfExtents.x * axis.Dot(b.axis[0])) +
            fabsf(b.halfExtents.y * axis.Dot(b.axis[1])) +
            fabsf(b.halfExtents.z * axis.Dot(b.axis[2]));

        float distance = fabsf(T.Dot(axis));
        float penetration = ra + rb - distance;

        if (penetration < minPenetration) {
            minPenetration = penetration;
            minAxis = i;
            axes[i] = axis; // Store normalized
        }
    }

    if (minAxis == -1) return false;

    // Use the minimum penetration axis as the normal
    Vec3 normal = axes[minAxis];

    // Make sure normal points from A to B
    if (T.Dot(normal) < 0.0f) {
        normal = normal * -1.0f;
    }

    // Generate contact point (simplified - use average of centers for now)
    Vec3 contactPoint = (a.center + b.center) * 0.5f;

    manifold.AddContact(contactPoint, normal, minPenetration);
    return true;
}

//-----------------------------------------------------------------------------
// Sphere vs OBB
//-----------------------------------------------------------------------------

Vec3 Collision3D::ClosestPointOnOBB(const Vec3& point, const OBB3D& box) {
    Vec3 d = point - box.center;
    Vec3 closest = box.center;

    // Project onto each axis and clamp
    for (int i = 0; i < 3; i++) {
        float extent = (i == 0) ? box.halfExtents.x :
            (i == 1) ? box.halfExtents.y : box.halfExtents.z;

        float dist = d.Dot(box.axis[i]);

        // Clamp to box extent
        if (dist > extent) dist = extent;
        if (dist < -extent) dist = -extent;

        closest = closest + box.axis[i] * dist;
    }

    return closest;
}

bool Collision3D::TestSphereOBB(const Sphere3D& sphere, const OBB3D& box) {
    Vec3 closest = ClosestPointOnOBB(sphere.center, box);
    float distSq = (closest - sphere.center).LengthSquared();
    return distSq <= sphere.radius * sphere.radius;
}

bool Collision3D::SphereOBB(const Sphere3D& sphere, const OBB3D& box, ContactManifold& manifold) {
    manifold.Clear();

    // Find closest point on box to sphere center
    Vec3 closest = ClosestPointOnOBB(sphere.center, box);

    // Vector from closest point TO sphere center
    Vec3 delta = sphere.center - closest;
    float distSq = delta.LengthSquared();
    float radiusSq = sphere.radius * sphere.radius;

    // No collision
    if (distSq > radiusSq) {
        return false;
    }

    float dist = sqrtf(distSq);

    Vec3 normal;
    float penetration;
    Vec3 contactPoint;

    // Case 1: Sphere center is inside or very close to box
    if (dist < 0.001f) {
        // Find shortest exit direction
        Vec3 localPos = sphere.center - box.center;

        float minDist = 999999.0f;
        Vec3 exitNormal = Vec3(0, 1, 0);

        for (int i = 0; i < 3; i++) {
            float extent = (i == 0) ? box.halfExtents.x :
                (i == 1) ? box.halfExtents.y : box.halfExtents.z;

            float proj = localPos.Dot(box.axis[i]);

            // Distance to positive face
            float distPos = extent - proj;
            if (distPos < minDist) {
                minDist = distPos;
                exitNormal = box.axis[i];
            }

            // Distance to negative face
            float distNeg = extent + proj;
            if (distNeg < minDist) {
                minDist = distNeg;
                exitNormal = box.axis[i] * -1.0f;
            }
        }

        normal = exitNormal;
        penetration = sphere.radius + minDist;
        contactPoint = sphere.center - normal * sphere.radius;
    }
    // Case 2: Normal collision
    else {
        // Normal points FROM box TO sphere (push sphere away from box)
        normal = delta / dist;
        penetration = sphere.radius - dist;
        contactPoint = closest;
    }

    manifold.AddContact(contactPoint, normal, penetration);
    return true;
}

//-----------------------------------------------------------------------------
// Plane collisions
//-----------------------------------------------------------------------------

bool Collision3D::SpherePlane(const Sphere3D& sphere, const Plane3D& plane, ContactManifold& manifold) {
    manifold.Clear();

    float dist = sphere.center.Dot(plane.normal) - plane.distance;

    if (dist >= sphere.radius) {
        return false;
    }

    Vec3 contactPoint = sphere.center - plane.normal * sphere.radius;
    float penetration = sphere.radius - dist;

    manifold.AddContact(contactPoint, plane.normal, penetration);
    return true;
}

void Collision3D::GetOBBCorners(const OBB3D& box, Vec3 corners[8]) {
    Vec3 extents[3] = {
        box.axis[0] * box.halfExtents.x,
        box.axis[1] * box.halfExtents.y,
        box.axis[2] * box.halfExtents.z
    };

    corners[0] = box.center - extents[0] - extents[1] - extents[2];
    corners[1] = box.center + extents[0] - extents[1] - extents[2];
    corners[2] = box.center - extents[0] + extents[1] - extents[2];
    corners[3] = box.center + extents[0] + extents[1] - extents[2];
    corners[4] = box.center - extents[0] - extents[1] + extents[2];
    corners[5] = box.center + extents[0] - extents[1] + extents[2];
    corners[6] = box.center - extents[0] + extents[1] + extents[2];
    corners[7] = box.center + extents[0] + extents[1] + extents[2];
}

bool Collision3D::OBBPlane(const OBB3D& box, const Plane3D& plane, ContactManifold& manifold, float planeHalfSizeX, float planeHalfSizeZ) {
    manifold.Clear();

    Vec3 corners[8];
    GetOBBCorners(box, corners);

    int belowCount = 0;
    for (int i = 0; i < 8; i++) {
        float dist = corners[i].Dot(plane.normal) - plane.distance;

        if (planeHalfSizeX > 0.0f && planeHalfSizeZ > 0.0f) {
            if (fabsf(corners[i].x) > planeHalfSizeX ||
                fabsf(corners[i].z) > planeHalfSizeZ) {
                continue;
            }
        }

        if (dist < 0.0f) {
            belowCount++;
            Vec3 contactPoint = corners[i] - plane.normal * dist;
            manifold.AddContact(contactPoint, plane.normal, -dist);
        }
    }

    return belowCount > 0;
}
