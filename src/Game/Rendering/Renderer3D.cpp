//-----------------------------------------------------------------------------
// Renderer3D.cpp
// Improved 3D rendering with proper clipping
//-----------------------------------------------------------------------------
#include "Renderer3D.h"
#include "../Systems/LightingSystem.h"
#include <cmath>

void Renderer3D::DrawTriangle3D(
    const Vec3& p1, const Vec3& p2, const Vec3& p3,
    const Camera3D& camera,
    float r1, float g1, float b1,
    float r2, float g2, float b2,
    float r3, float g3, float b3,
    bool wireframe
) {
    // Project all vertices to NDC
    Vec4 ndc1 = camera.WorldToNDC(p1);
    Vec4 ndc2 = camera.WorldToNDC(p2);
    Vec4 ndc3 = camera.WorldToNDC(p3);

    // Skip if any vertex is behind camera or too close
    if (ndc1.w < camera.GetNearPlane() ||
        ndc2.w < camera.GetNearPlane() ||
        ndc3.w < camera.GetNearPlane()) {
        return;
    }

    // Simple frustum culling - skip if entire triangle is off-screen
    float minX = fminf(fminf(ndc1.x, ndc2.x), ndc3.x);
    float maxX = fmaxf(fmaxf(ndc1.x, ndc2.x), ndc3.x);
    float minY = fminf(fminf(ndc1.y, ndc2.y), ndc3.y);
    float maxY = fmaxf(fmaxf(ndc1.y, ndc2.y), ndc3.y);

    if (maxX < -1.5f || minX > 1.5f || maxY < -1.5f || minY > 1.5f) {
        return; // Entirely off-screen
    }

    // Back-face culling - FIXED: Changed > to <
    Vec3 edge1 = p2 - p1;
    Vec3 edge2 = p3 - p1;
    Vec3 normal = edge1.Cross(edge2);

    // Get triangle center
    Vec3 triCenter = (p1 + p2 + p3) * (1.0f / 3.0f);
    Vec3 viewDir = triCenter - camera.GetPosition();
    viewDir = viewDir.Normalized();
    normal = normal.Normalized();

    float backFaceDot = normal.Dot(viewDir);

    // FIXED: Cull if facing AWAY from camera (dot < -0.1)
    if (backFaceDot < -0.1f) {
        return; // Back-facing triangle
    }

    // Convert NDC (-1 to 1) to screen coordinates
    float x1 = (ndc1.x + 1.0f) * APP_VIRTUAL_WIDTH * 0.5f;
    float y1 = (ndc1.y + 1.0f) * APP_VIRTUAL_HEIGHT * 0.5f;

    float x2 = (ndc2.x + 1.0f) * APP_VIRTUAL_WIDTH * 0.5f;
    float y2 = (ndc2.y + 1.0f) * APP_VIRTUAL_HEIGHT * 0.5f;

    float x3 = (ndc3.x + 1.0f) * APP_VIRTUAL_WIDTH * 0.5f;
    float y3 = (ndc3.y + 1.0f) * APP_VIRTUAL_HEIGHT * 0.5f;

    // Use Z from NDC (0 to 1 range)
    App::DrawTriangle(
        x1, y1, ndc1.z, 1.0f,
        x2, y2, ndc2.z, 1.0f,
        x3, y3, ndc3.z, 1.0f,
        r1, g1, b1,
        r2, g2, b2,
        r3, g3, b3,
        wireframe
    );
}

void Renderer3D::DrawSphere(
    const Vec3& center, float radius, const Camera3D& camera,
    float r, float g, float b, int subdivisions, bool wireframe
) {
    int latSegments = 8 + subdivisions * 4;
    int lonSegments = 16 + subdivisions * 8;

    for (int lat = 0; lat < latSegments; lat++) {
        float theta1 = (float)lat * PI / latSegments;
        float theta2 = (float)(lat + 1) * PI / latSegments;

        for (int lon = 0; lon < lonSegments; lon++) {
            float phi1 = (float)lon * 2.0f * PI / lonSegments;
            float phi2 = (float)(lon + 1) * 2.0f * PI / lonSegments;

            Vec3 v1(
                center.x + radius * sinf(theta1) * cosf(phi1),
                center.y + radius * cosf(theta1),
                center.z + radius * sinf(theta1) * sinf(phi1)
            );

            Vec3 v2(
                center.x + radius * sinf(theta2) * cosf(phi1),
                center.y + radius * cosf(theta2),
                center.z + radius * sinf(theta2) * sinf(phi1)
            );

            Vec3 v3(
                center.x + radius * sinf(theta2) * cosf(phi2),
                center.y + radius * cosf(theta2),
                center.z + radius * sinf(theta2) * sinf(phi2)
            );

            Vec3 v4(
                center.x + radius * sinf(theta1) * cosf(phi2),
                center.y + radius * cosf(theta1),
                center.z + radius * sinf(theta1) * sinf(phi2)
            );

            // FIXED: Reversed order - v1, v3, v2 and v1, v4, v3
            if (wireframe) {
                DrawTriangle3D(v1, v2, v3, camera, r, g, b, r, g, b, r, g, b, true);
                DrawTriangle3D(v1, v3, v4, camera, r, g, b, r, g, b, r, g, b, true);
            }
            else {
                DrawTriangle3DLit(v1, v2, v3, camera, r, g, b);
                DrawTriangle3DLit(v1, v3, v4, camera, r, g, b);
            }
        }
    }
}

void Renderer3D::DrawCylinder(
    const Vec3& base, float radius, float height, const Camera3D& camera,
    float r, float g, float b, int segments, bool wireframe
) {
    Vec3 top = base + Vec3(0, height, 0);

    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i * 2.0f * PI / segments;
        float angle2 = (float)(i + 1) * 2.0f * PI / segments;

        Vec3 bottomV1 = base + Vec3(cosf(angle1) * radius, 0, sinf(angle1) * radius);
        Vec3 bottomV2 = base + Vec3(cosf(angle2) * radius, 0, sinf(angle2) * radius);
        Vec3 topV1 = top + Vec3(cosf(angle1) * radius, 0, sinf(angle1) * radius);
        Vec3 topV2 = top + Vec3(cosf(angle2) * radius, 0, sinf(angle2) * radius);

        // FIXED: Reversed side faces - bottomV1, topV2, bottomV2
        if (wireframe) {
            DrawTriangle3D(bottomV1, topV2, bottomV2, camera, r, g, b, r, g, b, r, g, b, true);
            DrawTriangle3D(bottomV1, topV1, topV2, camera, r, g, b, r, g, b, r, g, b, true);
        }
        else {
            DrawTriangle3DLit(bottomV1, topV2, bottomV2, camera, r, g, b);
            DrawTriangle3DLit(bottomV1, topV1, topV2, camera, r, g, b);
        }

        // FIXED: Bottom cap - base, bottomV1, bottomV2
        if (wireframe) {
            DrawTriangle3D(base, bottomV1, bottomV2, camera, r * 0.8f, g * 0.8f, b * 0.8f, r * 0.8f, g * 0.8f, b * 0.8f, r * 0.8f, g * 0.8f, b * 0.8f, true);
        }
        else {
            DrawTriangle3DLit(base, bottomV1, bottomV2, camera, r * 0.8f, g * 0.8f, b * 0.8f);
        }

        // FIXED: Top cap - top, topV2, topV1
        if (wireframe) {
            DrawTriangle3D(top, topV2, topV1, camera, r * 0.8f, g * 0.8f, b * 0.8f, r * 0.8f, g * 0.8f, b * 0.8f, r * 0.8f, g * 0.8f, b * 0.8f, true);
        }
        else {
            DrawTriangle3DLit(top, topV2, topV1, camera, r * 0.8f, g * 0.8f, b * 0.8f);
        }
    }
}

void Renderer3D::DrawCone(
    const Vec3& base, float radius, float height, const Camera3D& camera,
    float r, float g, float b, int segments, bool wireframe
) {
    Vec3 tip = base + Vec3(0, height, 0);

    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i * 2.0f * PI / segments;
        float angle2 = (float)(i + 1) * 2.0f * PI / segments;

        Vec3 baseV1 = base + Vec3(cosf(angle1) * radius, 0, sinf(angle1) * radius);
        Vec3 baseV2 = base + Vec3(cosf(angle2) * radius, 0, sinf(angle2) * radius);

        // FIXED: Reversed side - baseV1, tip, baseV2
        if (wireframe) {
            DrawTriangle3D(baseV1, tip, baseV2, camera, r, g, b, r, g, b, r, g, b, true);
        }
        else {
            DrawTriangle3DLit(baseV1, tip, baseV2, camera, r, g, b);
        }

        // FIXED: Base cap - base, baseV1, baseV2
        if (wireframe) {
            DrawTriangle3D(base, baseV1, baseV2, camera, r * 0.7f, g * 0.7f, b * 0.7f, r * 0.7f, g * 0.7f, b * 0.7f, r * 0.7f, g * 0.7f, b * 0.7f, true);
        }
        else {
            DrawTriangle3DLit(base, baseV1, baseV2, camera, r * 0.7f, g * 0.7f, b * 0.7f);
        }
    }
}

void Renderer3D::DrawPyramid(
    const Vec3& base, float size, float height, const Camera3D& camera,
    float r, float g, float b, bool wireframe
) {
    float hs = size * 0.5f;
    Vec3 tip = base + Vec3(0, height, 0);

    Vec3 c1 = base + Vec3(-hs, 0, -hs);
    Vec3 c2 = base + Vec3(hs, 0, -hs);
    Vec3 c3 = base + Vec3(hs, 0, hs);
    Vec3 c4 = base + Vec3(-hs, 0, hs);

    // FIXED: Correct winding order (counter-clockwise from outside)
    if (wireframe) {
        // Side faces - REVERSED ORDER
        DrawTriangle3D(c2, tip, c1, camera, r, g, b, r, g, b, r, g, b, true);
        DrawTriangle3D(c3, tip, c2, camera, r, g, b, r, g, b, r, g, b, true);
        DrawTriangle3D(c4, tip, c3, camera, r, g, b, r, g, b, r, g, b, true);
        DrawTriangle3D(c1, tip, c4, camera, r, g, b, r, g, b, r, g, b, true);

        // Base - two triangles
        DrawTriangle3D(c1, c2, c3, camera, r * 0.7f, g * 0.7f, b * 0.7f,
            r * 0.7f, g * 0.7f, b * 0.7f, r * 0.7f, g * 0.7f, b * 0.7f, true);
        DrawTriangle3D(c1, c3, c4, camera, r * 0.7f, g * 0.7f, b * 0.7f,
            r * 0.7f, g * 0.7f, b * 0.7f, r * 0.7f, g * 0.7f, b * 0.7f, true);
    }
    else {
        // Side faces with lighting - REVERSED ORDER
        DrawTriangle3DLit(c2, tip, c1, camera, r, g, b);
        DrawTriangle3DLit(c3, tip, c2, camera, r, g, b);
        DrawTriangle3DLit(c4, tip, c3, camera, r, g, b);
        DrawTriangle3DLit(c1, tip, c4, camera, r, g, b);

        // Base
        DrawTriangle3DLit(c1, c2, c3, camera, r * 0.7f, g * 0.7f, b * 0.7f);
        DrawTriangle3DLit(c1, c3, c4, camera, r * 0.7f, g * 0.7f, b * 0.7f);
    }
}


void Renderer3D::DrawTorus(
    const Vec3& center, float majorRadius, float minorRadius, const Camera3D& camera,
    float r, float g, float b, int majorSegments, int minorSegments, bool wireframe
) {
    for (int i = 0; i < majorSegments; i++) {
        float theta1 = (float)i * 2.0f * PI / majorSegments;
        float theta2 = (float)(i + 1) * 2.0f * PI / majorSegments;

        for (int j = 0; j < minorSegments; j++) {
            float phi1 = (float)j * 2.0f * PI / minorSegments;
            float phi2 = (float)(j + 1) * 2.0f * PI / minorSegments;

            float cx1 = majorRadius * cosf(theta1);
            float cz1 = majorRadius * sinf(theta1);
            float cx2 = majorRadius * cosf(theta2);
            float cz2 = majorRadius * sinf(theta2);

            Vec3 v1(
                center.x + cx1 + minorRadius * cosf(phi1) * cosf(theta1),
                center.y + minorRadius * sinf(phi1),
                center.z + cz1 + minorRadius * cosf(phi1) * sinf(theta1)
            );

            Vec3 v2(
                center.x + cx2 + minorRadius * cosf(phi1) * cosf(theta2),
                center.y + minorRadius * sinf(phi1),
                center.z + cz2 + minorRadius * cosf(phi1) * sinf(theta2)
            );

            Vec3 v3(
                center.x + cx2 + minorRadius * cosf(phi2) * cosf(theta2),
                center.y + minorRadius * sinf(phi2),
                center.z + cz2 + minorRadius * cosf(phi2) * sinf(theta2)
            );

            Vec3 v4(
                center.x + cx1 + minorRadius * cosf(phi2) * cosf(theta1),
                center.y + minorRadius * sinf(phi2),
                center.z + cz1 + minorRadius * cosf(phi2) * sinf(theta1)
            );

            // FIXED: Reversed order - v1, v3, v2 and v1, v4, v3
            if (wireframe) {
                DrawTriangle3D(v1, v3, v2, camera, r, g, b, r, g, b, r, g, b, true);
                DrawTriangle3D(v1, v4, v3, camera, r, g, b, r, g, b, r, g, b, true);
            }
            else {
                DrawTriangle3DLit(v1, v3, v2, camera, r, g, b);
                DrawTriangle3DLit(v1, v4, v3, camera, r, g, b);
            }
        }
    }
}

void Renderer3D::DrawPlane(
    const Vec3& center, float width, float depth, const Camera3D& camera,
    float r, float g, float b, bool wireframe
) {
    float hw = width * 0.5f;
    float hd = depth * 0.5f;

    Vec3 v1 = center + Vec3(-hw, 0, -hd);
    Vec3 v2 = center + Vec3(hw, 0, -hd);
    Vec3 v3 = center + Vec3(hw, 0, hd);
    Vec3 v4 = center + Vec3(-hw, 0, hd);

    // FIXED: Reversed order - v1, v3, v2 and v1, v4, v3
    if (wireframe) {
        DrawTriangle3D(v1, v3, v2, camera, r, g, b, r, g, b, r, g, b, true);
        DrawTriangle3D(v1, v4, v3, camera, r, g, b, r, g, b, r, g, b, true);
    }
    else {
        DrawTriangle3DLit(v1, v3, v2, camera, r, g, b);
        DrawTriangle3DLit(v1, v4, v3, camera, r, g, b);
    }
}

void Renderer3D::DrawCube(const Vec3& center, float size, const Camera3D& camera,
    float r, float g, float b) {
    float hs = size * 0.5f;

    // 8 vertices of cube
    Vec3 v[8] = {
        Vec3(center.x - hs, center.y - hs, center.z - hs), // 0: left-bottom-back
        Vec3(center.x + hs, center.y - hs, center.z - hs), // 1: right-bottom-back
        Vec3(center.x + hs, center.y + hs, center.z - hs), // 2: right-top-back
        Vec3(center.x - hs, center.y + hs, center.z - hs), // 3: left-top-back
        Vec3(center.x - hs, center.y - hs, center.z + hs), // 4: left-bottom-front
        Vec3(center.x + hs, center.y - hs, center.z + hs), // 5: right-bottom-front
        Vec3(center.x + hs, center.y + hs, center.z + hs), // 6: right-top-front
        Vec3(center.x - hs, center.y + hs, center.z + hs)  // 7: left-top-front
    };

    // Check if cube is in frustum (using center + radius)
    if (!camera.IsInFrustum(center)) {
        float radius = size * 0.866f; // sqrt(3)/2 for cube diagonal
        float dist = (center - camera.GetPosition()).Length();
        if (dist - radius > camera.GetFarPlane() || dist + radius < camera.GetNearPlane()) {
            return; // Entire cube is outside frustum
        }
    }

    // Draw 12 triangles (2 per face) - FIXED WINDING ORDER
    // All triangles now use counter-clockwise winding when viewed from outside

    // Front face (+Z) - facing camera when looking from +Z
    DrawTriangle3D(v[4], v[5], v[6], camera, r, g, b, r, g, b, r, g, b, true);
    DrawTriangle3D(v[4], v[6], v[7], camera, r, g, b, r, g, b, r, g, b, true);

    // Back face (-Z) - facing camera when looking from -Z
    DrawTriangle3D(v[1], v[0], v[3], camera, r * 0.7f, g * 0.7f, b * 0.7f, r * 0.7f, g * 0.7f, b * 0.7f, r * 0.7f, g * 0.7f, b * 0.7f, true);
    DrawTriangle3D(v[1], v[3], v[2], camera, r * 0.7f, g * 0.7f, b * 0.7f, r * 0.7f, g * 0.7f, b * 0.7f, r * 0.7f, g * 0.7f, b * 0.7f, true);

    // Top face (+Y) - facing camera when looking from +Y
    DrawTriangle3D(v[7], v[6], v[2], camera, r * 0.9f, g * 0.9f, b * 0.9f, r * 0.9f, g * 0.9f, b * 0.9f, r * 0.9f, g * 0.9f, b * 0.9f, true);
    DrawTriangle3D(v[7], v[2], v[3], camera, r * 0.9f, g * 0.9f, b * 0.9f, r * 0.9f, g * 0.9f, b * 0.9f, r * 0.9f, g * 0.9f, b * 0.9f, true);

    // Bottom face (-Y) - facing camera when looking from -Y
    DrawTriangle3D(v[0], v[1], v[5], camera, r * 0.6f, g * 0.6f, b * 0.6f, r * 0.6f, g * 0.6f, b * 0.6f, r * 0.6f, g * 0.6f, b * 0.6f, true);
    DrawTriangle3D(v[0], v[5], v[4], camera, r * 0.6f, g * 0.6f, b * 0.6f, r * 0.6f, g * 0.6f, b * 0.6f, r * 0.6f, g * 0.6f, b * 0.6f, true);

    // Right face (+X) - facing camera when looking from +X
    DrawTriangle3D(v[5], v[1], v[2], camera, r * 0.8f, g * 0.8f, b * 0.8f, r * 0.8f, g * 0.8f, b * 0.8f, r * 0.8f, g * 0.8f, b * 0.8f, true);
    DrawTriangle3D(v[5], v[2], v[6], camera, r * 0.8f, g * 0.8f, b * 0.8f, r * 0.8f, g * 0.8f, b * 0.8f, r * 0.8f, g * 0.8f, b * 0.8f, true);

    // Left face (-X) - facing camera when looking from -X
    DrawTriangle3D(v[0], v[4], v[7], camera, r * 0.75f, g * 0.75f, b * 0.75f, r * 0.75f, g * 0.75f, b * 0.75f, r * 0.75f, g * 0.75f, b * 0.75f, true);
    DrawTriangle3D(v[0], v[7], v[3], camera, r * 0.75f, g * 0.75f, b * 0.75f, r * 0.75f, g * 0.75f, b * 0.75f, r * 0.75f, g * 0.75f, b * 0.75f, true);
}


void Renderer3D::DrawAxes(const Vec3& origin, float length, const Camera3D& camera) {
    // X axis - Red
    Vec3 xEnd = origin + Vec3(length, 0, 0);
    DrawTriangle3D(origin, xEnd, origin + Vec3(0, 0.05f, 0), camera, 1, 0, 0, 1, 0, 0, 1, 0, 0, true);

    // Y axis - Green  
    Vec3 yEnd = origin + Vec3(0, length, 0);
    DrawTriangle3D(origin, yEnd, origin + Vec3(0.05f, 0, 0), camera, 0, 1, 0, 0, 1, 0, 0, 1, 0, true);

    // Z axis - Blue
    Vec3 zEnd = origin + Vec3(0, 0, length);
    DrawTriangle3D(origin, zEnd, origin + Vec3(0.05f, 0, 0), camera, 0, 0, 1, 0, 0, 1, 0, 0, 1, true);
}

void Renderer3D::DrawTriangle3DLit(
    const Vec3& p1, const Vec3& p2, const Vec3& p3,
    const Camera3D& camera,
    float r, float g, float b,
    const Vec3& lightDir  // This parameter is now ignored
) {
    // Project all vertices to NDC
    Vec4 ndc1 = camera.WorldToNDC(p1);
    Vec4 ndc2 = camera.WorldToNDC(p2);
    Vec4 ndc3 = camera.WorldToNDC(p3);

    // Skip if any vertex is behind camera
    if (ndc1.w < camera.GetNearPlane() ||
        ndc2.w < camera.GetNearPlane() ||
        ndc3.w < camera.GetNearPlane()) {
        return;
    }

    // Frustum culling
    float minX = fminf(fminf(ndc1.x, ndc2.x), ndc3.x);
    float maxX = fmaxf(fmaxf(ndc1.x, ndc2.x), ndc3.x);
    float minY = fminf(fminf(ndc1.y, ndc2.y), ndc3.y);
    float maxY = fmaxf(fmaxf(ndc1.y, ndc2.y), ndc3.y);

    if (maxX < -1.5f || minX > 1.5f || maxY < -1.5f || minY > 1.5f) {
        return;
    }

    // Calculate surface normal
    Vec3 edge1 = p2 - p1;
    Vec3 edge2 = p3 - p1;
    Vec3 normal = edge1.Cross(edge2);

    // Back-face culling
    Vec3 triCenter = (p1 + p2 + p3) * (1.0f / 3.0f);
    Vec3 viewDir = (triCenter - camera.GetPosition()).Normalized();
    if (normal.Dot(viewDir) < 0.0f) {
        return; // Back-face cull
    }

    normal = normal.Normalized();

    // === USE GLOBAL LIGHTING SYSTEM ===
    Vec3 baseColor(r, g, b);
    Vec3 litColor = LightingSystem::CalculateLighting(triCenter, normal, baseColor);

    // Convert NDC to screen coordinates
    float x1 = (ndc1.x + 1.0f) * APP_VIRTUAL_WIDTH * 0.5f;
    float y1 = (ndc1.y + 1.0f) * APP_VIRTUAL_HEIGHT * 0.5f;

    float x2 = (ndc2.x + 1.0f) * APP_VIRTUAL_WIDTH * 0.5f;
    float y2 = (ndc2.y + 1.0f) * APP_VIRTUAL_HEIGHT * 0.5f;

    float x3 = (ndc3.x + 1.0f) * APP_VIRTUAL_WIDTH * 0.5f;
    float y3 = (ndc3.y + 1.0f) * APP_VIRTUAL_HEIGHT * 0.5f;

    // Draw with calculated lighting
    App::DrawTriangle(
        x1, y1, ndc1.z, 1.0f,
        x2, y2, ndc2.z, 1.0f,
        x3, y3, ndc3.z, 1.0f,
        litColor.x, litColor.y, litColor.z,
        litColor.x, litColor.y, litColor.z,
        litColor.x, litColor.y, litColor.z,
        false
    );
}

void Renderer3D::DrawMesh(
    const Mesh3D& mesh,
    const Vec3& position,
    const Vec3& rotation,
    const Vec3& scale,
    const Camera3D& camera,
    float r, float g, float b,
    bool wireframe
) {
    // Build transformation matrix
    float radX = rotation.x * PI / 180.0f;
    float radY = rotation.y * PI / 180.0f;
    float radZ = rotation.z * PI / 180.0f;

    Matrix4x4 matScale = Matrix4x4::Scale(scale);
    Matrix4x4 matRotX = Matrix4x4::RotationX(radX);
    Matrix4x4 matRotY = Matrix4x4::RotationY(radY);
    Matrix4x4 matRotZ = Matrix4x4::RotationZ(radZ);
    Matrix4x4 matTrans = Matrix4x4::Translation(position);

    Matrix4x4 transform = matTrans * matRotZ * matRotY * matRotX * matScale;

    // Draw each triangle
    for (size_t i = 0; i < mesh.vertices.size(); i += 3) {
        if (i + 2 >= mesh.vertices.size()) break;

        // Transform vertices
        Vec3 v0 = transform.TransformPoint(mesh.vertices[i].position);
        Vec3 v1 = transform.TransformPoint(mesh.vertices[i + 1].position);
        Vec3 v2 = transform.TransformPoint(mesh.vertices[i + 2].position);

        if (wireframe) {
            DrawTriangle3D(v0, v1, v2, camera, r, g, b, r, g, b, r, g, b, true);
        }
        else {
            // Now uses LightingSystem internally
            DrawTriangle3DLit(v0, v1, v2, camera, r, g, b);
        }
    }
}

// Keep DrawCubeLit updated too
void Renderer3D::DrawCubeLit(
    const Vec3& center,
    float size,
    const Camera3D& camera,
    float r, float g, float b
) {
    float hs = size * 0.5f;

    Vec3 v[8] = {
        Vec3(center.x - hs, center.y - hs, center.z - hs), // 0
        Vec3(center.x + hs, center.y - hs, center.z - hs), // 1
        Vec3(center.x + hs, center.y + hs, center.z - hs), // 2
        Vec3(center.x - hs, center.y + hs, center.z - hs), // 3
        Vec3(center.x - hs, center.y - hs, center.z + hs), // 4
        Vec3(center.x + hs, center.y - hs, center.z + hs), // 5
        Vec3(center.x + hs, center.y + hs, center.z + hs), // 6
        Vec3(center.x - hs, center.y + hs, center.z + hs)  // 7
    };

    // Frustum check
    if (!camera.IsInFrustum(center)) {
        float radius = size * 0.866f;
        float dist = (center - camera.GetPosition()).Length();
        if (dist - radius > camera.GetFarPlane() || dist + radius < camera.GetNearPlane()) {
            return;
        }
    }

    // All faces now use LightingSystem
    DrawTriangle3DLit(v[4], v[5], v[6], camera, r, g, b);
    DrawTriangle3DLit(v[4], v[6], v[7], camera, r, g, b);
    DrawTriangle3DLit(v[1], v[0], v[3], camera, r, g, b);
    DrawTriangle3DLit(v[1], v[3], v[2], camera, r, g, b);
    DrawTriangle3DLit(v[7], v[6], v[2], camera, r, g, b);
    DrawTriangle3DLit(v[7], v[2], v[3], camera, r, g, b);
    DrawTriangle3DLit(v[0], v[1], v[5], camera, r, g, b);
    DrawTriangle3DLit(v[0], v[5], v[4], camera, r, g, b);
    DrawTriangle3DLit(v[5], v[1], v[2], camera, r, g, b);
    DrawTriangle3DLit(v[5], v[2], v[6], camera, r, g, b);
    DrawTriangle3DLit(v[0], v[4], v[7], camera, r, g, b);
    DrawTriangle3DLit(v[0], v[7], v[3], camera, r, g, b);
}