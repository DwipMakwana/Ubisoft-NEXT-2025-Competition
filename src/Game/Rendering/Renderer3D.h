//-----------------------------------------------------------------------------
// Renderer3D.h
// Helper for drawing 3D shapes using App::DrawTriangle
//-----------------------------------------------------------------------------
#ifndef RENDERER_3D_H
#define RENDERER_3D_H

#include "App.h"
#include "AppSettings.h"
#include "../Utilities/MathUtils3D.h"
#include "Camera3D.h"
#include "../Rendering/Mesh3D.h"

class Renderer3D {
public:
    // Draw a 3D triangle
    static void DrawTriangle3D(
        const Vec3& p1, const Vec3& p2, const Vec3& p3,
        const Camera3D& camera,
        float r1 = 1.0f, float g1 = 1.0f, float b1 = 1.0f,
        float r2 = 1.0f, float g2 = 1.0f, float b2 = 1.0f,
        float r3 = 1.0f, float g3 = 1.0f, float b3 = 1.0f,
        bool wireframe = false
    );

    // Sphere (subdivided icosahedron approach)
    static void DrawSphere(
        const Vec3& center, float radius, const Camera3D& camera,
        float r, float g, float b, int subdivisions = 2, bool wireframe = false
    );

    // Cylinder (with top and bottom caps)
    static void DrawCylinder(
        const Vec3& base, float radius, float height, const Camera3D& camera,
        float r, float g, float b, int segments = 16, bool wireframe = false
    );

    // Cone
    static void DrawCone(
        const Vec3& base, float radius, float height, const Camera3D& camera,
        float r, float g, float b, int segments = 16, bool wireframe = false
    );

    // Pyramid (square base)
    static void DrawPyramid(
        const Vec3& base, float size, float height, const Camera3D& camera,
        float r, float g, float b, bool wireframe = false
    );

    // Torus (donut)
    static void DrawTorus(
        const Vec3& center, float majorRadius, float minorRadius, const Camera3D& camera,
        float r, float g, float b, int majorSegments = 16, int minorSegments = 8, bool wireframe = false
    );

    // Plane (flat rectangle)
    static void DrawPlane(
        const Vec3& center, float width, float depth, const Camera3D& camera,
        float r, float g, float b, bool wireframe = false
    );

    // Draw a cube at position with size
    static void DrawCube(const Vec3& center, float size, const Camera3D& camera,
        float r = 1.0f, float g = 1.0f, float b = 1.0f);

    // Draw coordinate axes (RGB = XYZ)
    static void DrawAxes(const Vec3& origin, float length, const Camera3D& camera);

    // Draw filled triangle with lighting
    static void DrawTriangle3DLit(
        const Vec3& p1, const Vec3& p2, const Vec3& p3,
        const Camera3D& camera,
        float r, float g, float b,
        const Vec3& lightDir = Vec3(0, -1, 0) // Default light direction
    );

    // Draw filled cube with lighting
    static void DrawCubeLit(
        const Vec3& center, float size, const Camera3D& camera,
        float r, float g, float b
    );

    static void DrawMesh(const Mesh3D& mesh, const Vec3& position, 
        const Vec3& rotation, const Vec3& scale, const Camera3D& camera, 
        float r = 1.0f, float g = 1.0f, float b = 1.0f, 
        bool wireframe = false);

private:
    
};

#endif
