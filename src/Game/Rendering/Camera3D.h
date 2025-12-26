//-----------------------------------------------------------------------------
// Camera3D.h
// 3D camera with proper perspective projection
//-----------------------------------------------------------------------------
#ifndef CAMERA_3D_H
#define CAMERA_3D_H

#include "../Utilities/MathUtils3D.h"

class Camera3D {
private:
    Vec3 position;
    Vec3 target;
    Vec3 up;

    float fov;      // Field of view in degrees
    float aspect;   // Aspect ratio
    float nearPlane;
    float farPlane;

    Vec3 orbitPivot;  // Point to orbit around
    float orbitDistance;  // Distance from pivot
    float orbitYaw;  // Horizontal angle
    float orbitPitch;  // Vertical angle

public:
    Camera3D();

    void Pan(float deltaX, float deltaY);
    void Orbit(float deltaX, float deltaY, const Vec3& pivotPoint);
    void Zoom(float delta);
    void Flythrough(float forward, float right, float up, float yaw, float pitch);

    void SetPivot(const Vec3& pivot) { orbitPivot = pivot; }
    Vec3 GetPivot() const { return orbitPivot; }

    void SetPosition(const Vec3& pos) { position = pos; }
    void SetTarget(const Vec3& tgt) { target = tgt; }
    void SetUp(const Vec3& u) { up = u; }

    Vec3 GetPosition() const { return position; }
    Vec3 GetTarget() const { return target; }
    Vec3 GetForward() const { return (target - position).Normalized(); }
    Vec3 GetRight() const { return GetForward().Cross(up).Normalized(); }
    Vec3 GetUp() const { return up; }

    float GetNearPlane() const { return nearPlane; }
    float GetFarPlane() const { return farPlane; }

    // Convert 3D world position to normalized device coordinates (NDC)
    // Returns Vec4 where: xyz = NDC position, w = clip space w (for perspective divide)
    // Returns w < 0 if point is behind camera
    Vec4 WorldToNDC(const Vec3& worldPos) const;

    // Check if point is in view frustum
    bool IsInFrustum(const Vec3& worldPos) const;

    // Simple orbit controls
    void OrbitAroundTarget(float angleX, float angleY, float distance);

    // Move camera
    void MoveForward(float amount);
    void MoveRight(float amount);
    void MoveUp(float amount);

    // Screen to world ray (for mouse picking)
    //Ray3D ScreenToWorldRay(float screenX, float screenY) const;

    Vec3 ScreenToWorldDirection(const Vec2& screenNDC) const;

    Vec3 WorldToScreen(const Vec3& worldPos) const;
};

#endif
