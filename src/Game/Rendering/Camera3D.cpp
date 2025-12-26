//-----------------------------------------------------------------------------
// Camera3D.cpp
//-----------------------------------------------------------------------------
#include "Camera3D.h"
#include "AppSettings.h"
#include <cmath>

#ifndef PI
#define PI 3.14159265359f
#endif

Camera3D::Camera3D()
    : position(0, 2, 5), target(0, 0, 0), up(0, 1, 0),
    fov(60.0f), aspect(float(APP_VIRTUAL_WIDTH) / float(APP_VIRTUAL_HEIGHT)),
    nearPlane(0.5f), farPlane(100.0f) {  // Increased near plane from 0.1 to 0.5
}

Vec4 Camera3D::WorldToNDC(const Vec3& worldPos) const {
    // Transform to camera space
    Vec3 forward = GetForward();
    Vec3 right = GetRight();
    Vec3 camUp = GetUp();

    Vec3 worldToCam = worldPos - position;

    // Camera space coordinates
    float camX = worldToCam.Dot(right);
    float camY = worldToCam.Dot(camUp);
    float camZ = worldToCam.Dot(forward);

    // Check if behind camera (negative Z in camera space)
    if (camZ < nearPlane) {
        return Vec4(0, 0, 0, -1); // Invalid - behind camera or too close
    }

    // Perspective projection
    float fovRad = fov * PI / 180.0f;
    float tanHalfFov = tanf(fovRad / 2.0f);

    // Project to NDC (-1 to 1 range)
    float ndcX = camX / (camZ * tanHalfFov * aspect);
    float ndcY = camY / (camZ * tanHalfFov);

    // Map Z to 0-1 range (near=0, far=1)
    float ndcZ = (camZ - nearPlane) / (farPlane - nearPlane);

    // Clamp Z to valid range
    if (ndcZ < 0.0f) ndcZ = 0.0f;
    if (ndcZ > 1.0f) ndcZ = 1.0f;

    return Vec4(ndcX, ndcY, ndcZ, camZ); // w = distance for clipping
}

bool Camera3D::IsInFrustum(const Vec3& worldPos) const {
    Vec4 ndc = WorldToNDC(worldPos);

    // Behind camera or too close
    if (ndc.w < nearPlane) return false;

    // Outside frustum bounds
    if (ndc.x < -1.5f || ndc.x > 1.5f) return false;
    if (ndc.y < -1.5f || ndc.y > 1.5f) return false;
    if (ndc.z < 0.0f || ndc.z > 1.0f) return false;

    return true;
}

void Camera3D::OrbitAroundTarget(float angleX, float angleY, float distance) {
    float x = distance * cosf(angleY) * cosf(angleX);
    float y = distance * sinf(angleY);
    float z = distance * cosf(angleY) * sinf(angleX);

    position = target + Vec3(x, y, z);
}

void Camera3D::MoveForward(float amount) {
    Vec3 forward = GetForward();
    position += forward * amount;
    target += forward * amount;
}

void Camera3D::MoveRight(float amount) {
    Vec3 right = GetRight();
    position += right * amount;
    target += right * amount;
}

void Camera3D::MoveUp(float amount) {
    position += up * amount;
    target += up * amount;
}

//Ray3D Camera3D::ScreenToWorldRay(float screenX, float screenY) const {
//    // screenX, screenY are in screen space (Y=0 at TOP)
//
//    // Step 1: Convert to NDC (-1 to +1)
//    float ndcX = (screenX / APP_VIRTUAL_WIDTH) * 2.0f - 1.0f;
//    float ndcY = 1.0f - (screenY / APP_VIRTUAL_HEIGHT) * 2.0f;  // Flip Y
//
//    // Step 2: Calculate ray direction using FOV and aspect ratio
//    float fovRad = fov * PI / 180.0f;
//    float tanHalfFov = tanf(fovRad / 2.0f);
//
//    // Step 3: Get camera basis vectors (MUST be orthonormal)
//    Vec3 forward = GetForward();
//    Vec3 right = GetRight();
//
//    // CRITICAL: Use cross product to get true orthogonal up vector
//    // DO NOT use the stored 'up' vector directly!
//    Vec3 camUp = right.Cross(forward).Normalized();
//
//    // Step 4: Build ray direction
//    // The ray starts at camera position and shoots through the screen point
//    Vec3 rayDir = forward +
//        right * (ndcX * aspect * tanHalfFov) +
//        camUp * (ndcY * tanHalfFov);
//
//    return Ray3D(position, rayDir.Normalized());
//}

Vec3 Camera3D::ScreenToWorldDirection(const Vec2& screenNDC) const {
    // Convert NDC to world direction
    // This is a simplified version - adjust based on your projection

    Vec3 forward = (target - position).Normalized();
    Vec3 right = forward.Cross(up).Normalized();
    Vec3 upDir = right.Cross(forward).Normalized();

    // Apply screen offset to direction
    float fovRadians = fov * PI / 180.0f;
    float aspectRatio = (float)APP_VIRTUAL_WIDTH / (float)APP_VIRTUAL_HEIGHT;

    float tanFov = tanf(fovRadians * 0.5f);

    Vec3 direction = forward +
        right * (screenNDC.x * tanFov * aspectRatio) +
        upDir * (screenNDC.y * tanFov);

    return direction.Normalized();
}

Vec3 Camera3D::WorldToScreen(const Vec3& worldPos) const {
    // View matrix transformation
    Vec3 forward = (target - position).Normalized();
    Vec3 right = forward.Cross(up).Normalized();
    Vec3 upDir = right.Cross(forward).Normalized();

    // Transform to camera space
    Vec3 toPoint = worldPos - position;
    float x = toPoint.Dot(right);
    float y = toPoint.Dot(upDir);
    float z = toPoint.Dot(forward);

    // Simple perspective projection
    if (z <= 0.1f) z = 0.1f; // Prevent divide by zero

    float fovRadians = fov * PI / 180.0f;
    float aspect = (float)APP_VIRTUAL_WIDTH / (float)APP_VIRTUAL_HEIGHT;

    float projX = x / (z * tanf(fovRadians * 0.5f) * aspect);
    float projY = y / (z * tanf(fovRadians * 0.5f));

    // Convert to screen coordinates
#if APP_USE_VIRTUAL_RES
    float screenX = (projX * 0.5f + 0.5f) * APP_VIRTUAL_WIDTH;
    float screenY = (0.5f - projY * 0.5f) * APP_VIRTUAL_HEIGHT;
#else
    float screenX = projX;
    float screenY = projY;
#endif

    // Return with depth for z-testing
    return Vec3(screenX, screenY, z);
}

void Camera3D::Pan(float deltaX, float deltaY) {
    // Move camera and target together (sideways and up/down)
    Vec3 right = GetRight();
    Vec3 upDir = up;

    float panSpeed = 0.01f;
    Vec3 movement = right * (-deltaX * panSpeed) + upDir * (deltaY * panSpeed);

    position = position + movement;
    target = target + movement;
}

void Camera3D::Orbit(float deltaX, float deltaY, const Vec3& pivotPoint) {
    // Calculate current orbit parameters
    Vec3 offset = position - pivotPoint;
    float distance = offset.Length();

    // Calculate yaw and pitch from mouse delta
    float sensitivity = 0.2f;
    orbitYaw -= deltaX * sensitivity;
    orbitPitch += deltaY * sensitivity;

    // Clamp pitch to prevent flipping
    if (orbitPitch > 89.0f) orbitPitch = 89.0f;
    if (orbitPitch < -89.0f) orbitPitch = -89.0f;

    // Convert to radians
    float yawRad = orbitYaw * PI / 180.0f;
    float pitchRad = orbitPitch * PI / 180.0f;

    // Calculate new position
    float x = distance * cosf(pitchRad) * sinf(yawRad);
    float y = distance * sinf(pitchRad);
    float z = distance * cosf(pitchRad) * cosf(yawRad);

    position = pivotPoint + Vec3(x, y, z);
    target = pivotPoint;
}

void Camera3D::Zoom(float delta) {
    // Move camera toward/away from target
    Vec3 forward = GetForward();
    float zoomSpeed = 0.5f;

    position = position + forward * (delta * zoomSpeed);

    // Don't get too close
    float minDistance = 1.0f;
    if ((target - position).Length() < minDistance) {
        position = target - forward * minDistance;
    }
}

void Camera3D::Flythrough(float forward, float right, float up, float yaw, float pitch) {
    Vec3 forwardDir = GetForward();
    Vec3 rightDir = GetRight();
    Vec3 upDir = this->up;

    float moveSpeed = 0.1f;
    Vec3 movement =
        forwardDir * (forward * moveSpeed) +
        rightDir * (right * moveSpeed) +
        upDir * (up * moveSpeed);
    position = position + movement;

    // ---- NEW: static yaw/pitch with one-time initialization ----
    static bool initialized = false;
    static float currentYaw = 0.0f;
    static float currentPitch = 0.0f;

    if (!initialized) {
        Vec3 dir = (target - position).Normalized();
        // From direction vector to yaw/pitch (Y up, Z forward)
        currentPitch = asinf(dir.y) * 180.0f / PI;
        currentYaw = atan2f(dir.x, dir.z) * 180.0f / PI;
        initialized = true;
    }
    // ------------------------------------------------------------

    float sensitivity = 0.2f;
    currentYaw += yaw * sensitivity;
    currentPitch -= pitch * sensitivity;

    if (currentPitch > 89.0f)  currentPitch = 89.0f;
    if (currentPitch < -89.0f) currentPitch = -89.0f;

    float yawRad = currentYaw * PI / 180.0f;
    float pitchRad = currentPitch * PI / 180.0f;

    float x = cosf(pitchRad) * sinf(yawRad);
    float y = sinf(pitchRad);
    float z = cosf(pitchRad) * cosf(yawRad);

    Vec3 newDir = Vec3(x, y, z).Normalized();
    float distance = (target - position).Length();
    target = position + newDir * distance;
}

