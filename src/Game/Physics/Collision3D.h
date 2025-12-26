//-----------------------------------------------------------------------------
// Collision3D.h
// 3D collision detection with contact manifolds
//-----------------------------------------------------------------------------

#ifndef COLLISION_3D_H
#define COLLISION_3D_H

#include "../Utilities/MathUtils3D.h"

// Contact information for physics resolution
struct Contact3D {
    Vec3 point;          // Contact point in world space
    Vec3 normal;         // Contact normal (from A to B)
    float penetration;   // Penetration depth

    Contact3D() : point(0, 0, 0), normal(0, 1, 0), penetration(0) {}
    Contact3D(const Vec3& p, const Vec3& n, float pen)
        : point(p), normal(n), penetration(pen) {
    }
};

// Contact manifold - can hold multiple contact points
struct ContactManifold {
    Contact3D contacts[4];
    int numContacts;

    ContactManifold() : numContacts(0) {}

    void AddContact(const Vec3& point, const Vec3& normal, float penetration) {
        if (numContacts < 4) {
            contacts[numContacts++] = Contact3D(point, normal, penetration);
        }
    }

    void Clear() {
        numContacts = 0;
    }
};

//-----------------------------------------------------------------------------
// Basic shapes
//-----------------------------------------------------------------------------

struct Sphere3D {
    Vec3 center;
    float radius;

    Sphere3D() : center(0, 0, 0), radius(1.0f) {}
    Sphere3D(const Vec3& c, float r) : center(c), radius(r) {}
};

struct AABB3D {
    Vec3 min;
    Vec3 max;

    AABB3D() : min(-1, -1, -1), max(1, 1, 1) {}
    AABB3D(const Vec3& minP, const Vec3& maxP) : min(minP), max(maxP) {}

    Vec3 GetCenter() const { return (min + max) * 0.5f; }
    Vec3 GetExtents() const { return (max - min) * 0.5f; }
};

struct OBB3D {
    Vec3 center;
    Vec3 halfExtents;
    Vec3 axis[3];  // Local axes (right, up, forward)

    OBB3D() : center(0, 0, 0), halfExtents(0.5f, 0.5f, 0.5f) {
        axis[0] = Vec3(1, 0, 0);
        axis[1] = Vec3(0, 1, 0);
        axis[2] = Vec3(0, 0, 1);
    }

    void SetRotation(const Vec3& eulerAngles); // Radians
};

struct Plane3D {
    Vec3 normal;
    float distance;

    Plane3D() : normal(0, 1, 0), distance(0) {}
    Plane3D(const Vec3& n, float d) : normal(n.Normalized()), distance(d) {}
};

//-----------------------------------------------------------------------------
// Collision detection class
//-----------------------------------------------------------------------------

class Collision3D {
public:
    // Sphere collisions
    static bool TestSphereSphere(const Sphere3D& a, const Sphere3D& b);
    static bool SphereSphere(const Sphere3D& a, const Sphere3D& b, ContactManifold& manifold);

    // Box collisions  
    static bool TestOBBOBB(const OBB3D& a, const OBB3D& b);
    static bool OBBOBB(const OBB3D& a, const OBB3D& b, ContactManifold& manifold);

    // Sphere vs Box
    static bool TestSphereOBB(const Sphere3D& sphere, const OBB3D& box);
    static bool SphereOBB(const Sphere3D& sphere, const OBB3D& box, ContactManifold& manifold);

    // Plane collisions
    static bool SpherePlane(const Sphere3D& sphere, const Plane3D& plane, ContactManifold& manifold);
    static bool OBBPlane(const OBB3D& box, const Plane3D& plane, ContactManifold& manifold, float planeHalfSizeX = -1.0f, float planeHalfSizeZ = -1.0f);
    // Utility functions
    static Vec3 ClosestPointOnOBB(const Vec3& point, const OBB3D& box);
    static void GetOBBCorners(const OBB3D& box, Vec3 corners[8]);
};

#endif // COLLISION_3D_H
