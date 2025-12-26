//-----------------------------------------------------------------------------
// Particle3D.cpp - 3D billboarded particles WITHOUT back-face culling
//-----------------------------------------------------------------------------
#include "../Systems/Particle3D.h"
#include "../Rendering/Renderer3D.h"
#include "../Utilities/Logger.h"
#include "App.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

ParticleSystem3D::ParticleSystem3D(int maxPart)
    : maxParticles(maxPart)
    , emitAccumulator(0.0f)
    , emitVelocity(0, 2, 0)
    , emitVelocityVariance(1, 1, 1)
    , gravity(0, -5, 0)
    , startColor(1, 0.8f, 0.3f)
    , endColor(0.5f, 0.1f, 0.1f)
    , startSize(0.05f)
    , endSize(0.05f)
    , lifeTime(2.0f)
    , lifeTimeVariance(0.5f)
    , useGravity(true)
{
    particles.resize(maxParticles);
    Logger::LogFormat("ParticleSystem3D created with %d max particles\n", maxParticles);
}

void ParticleSystem3D::Update(float deltaTime) {
    for (auto& p : particles) {
        if (!p.active) continue;

        p.life -= deltaTime;
        if (p.life <= 0.0f) {
            p.active = false;
            continue;
        }

        if (useGravity) {
            p.velocity.x += gravity.x * deltaTime;
            p.velocity.y += gravity.y * deltaTime;
            p.velocity.z += gravity.z * deltaTime;
        }

        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;
        p.position.z += p.velocity.z * deltaTime;

        float t = 1.0f - (p.life / p.maxLife);
        p.color.x = startColor.x + (endColor.x - startColor.x) * t;
        p.color.y = startColor.y + (endColor.y - startColor.y) * t;
        p.color.z = startColor.z + (endColor.z - startColor.z) * t;
        p.size = startSize + (endSize - startSize) * t;
    }
}

void ParticleSystem3D::RenderBillboard(const Vec3& position, float size, const Vec3& color, const Camera3D& camera) {
    // Get camera basis vectors for billboarding
    Vec3 forward = camera.GetForward();
    Vec3 right = camera.GetRight();
    Vec3 up = right.Cross(forward).Normalized();

    // Create billboard quad facing camera
    float halfSize = size * 0.5f;
    
    Vec3 v1 = position + (right * -halfSize) + (up * -halfSize);
    Vec3 v2 = position + (right * halfSize) + (up * -halfSize);
    Vec3 v3 = position + (right * halfSize) + (up * halfSize);
    Vec3 v4 = position + (right * -halfSize) + (up * halfSize);

    // Project all vertices to NDC manually to bypass back-face culling
    Vec4 ndc1 = camera.WorldToNDC(v1);
    Vec4 ndc2 = camera.WorldToNDC(v2);
    Vec4 ndc3 = camera.WorldToNDC(v3);
    Vec4 ndc4 = camera.WorldToNDC(v4);

    // Skip if behind camera
    if (ndc1.w < camera.GetNearPlane() || 
        ndc2.w < camera.GetNearPlane() ||
        ndc3.w < camera.GetNearPlane() ||
        ndc4.w < camera.GetNearPlane()) {
        return;
    }

    // Convert to screen space
    float x1 = (ndc1.x + 1.0f) * APP_VIRTUAL_WIDTH * 0.5f;
    float y1 = (ndc1.y + 1.0f) * APP_VIRTUAL_HEIGHT * 0.5f;
    float x2 = (ndc2.x + 1.0f) * APP_VIRTUAL_WIDTH * 0.5f;
    float y2 = (ndc2.y + 1.0f) * APP_VIRTUAL_HEIGHT * 0.5f;
    float x3 = (ndc3.x + 1.0f) * APP_VIRTUAL_WIDTH * 0.5f;
    float y3 = (ndc3.y + 1.0f) * APP_VIRTUAL_HEIGHT * 0.5f;
    float x4 = (ndc4.x + 1.0f) * APP_VIRTUAL_WIDTH * 0.5f;
    float y4 = (ndc4.y + 1.0f) * APP_VIRTUAL_HEIGHT * 0.5f;

    // Draw directly using App::DrawTriangle to bypass back-face culling
    // Triangle 1: v1, v2, v3
    App::DrawTriangle(
        x1, y1, ndc1.z, 1.0f,
        x2, y2, ndc2.z, 1.0f,
        x3, y3, ndc3.z, 1.0f,
        color.x, color.y, color.z,
        color.x, color.y, color.z,
        color.x, color.y, color.z,
        false
    );

    // Triangle 2: v1, v3, v4
    App::DrawTriangle(
        x1, y1, ndc1.z, 1.0f,
        x3, y3, ndc3.z, 1.0f,
        x4, y4, ndc4.z, 1.0f,
        color.x, color.y, color.z,
        color.x, color.y, color.z,
        color.x, color.y, color.z,
        false
    );
}

void ParticleSystem3D::Render(const Camera3D& camera) {
    struct ParticleDepth {
        const Particle3D* particle;
        float depth;
    };
    
    std::vector<ParticleDepth> sortedParticles;
    sortedParticles.reserve(maxParticles);

    Vec3 camPos = camera.GetPosition();
    
    for (const auto& p : particles) {
        if (!p.active) continue;

        Vec3 diff = p.position - camPos;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        
        sortedParticles.push_back({ &p, distSq });
    }

    if (sortedParticles.empty()) return;

    // Sort back to front
    std::sort(sortedParticles.begin(), sortedParticles.end(),
        [](const ParticleDepth& a, const ParticleDepth& b) {
            return a.depth > b.depth;
        });

    // Render sorted particles as billboards
    for (const auto& pd : sortedParticles) {
        const Particle3D& p = *pd.particle;
        RenderBillboard(p.position, p.size, p.color, camera);
    }
}

void ParticleSystem3D::Emit(const Vec3& position, int count) {
    for (int i = 0; i < count; i++) {
        int index = FindInactiveParticle();
        if (index >= 0) {
            ActivateParticle(index, position);
        }
    }
}

void ParticleSystem3D::EmitExplosion(const Vec3& position, int count) {
    Logger::LogFormat("EXPLOSION: %d particles at (%.2f, %.2f, %.2f)\n",
        count, position.x, position.y, position.z);

    for (int i = 0; i < count; i++) {
        int index = FindInactiveParticle();
        if (index < 0) {
            Logger::LogLine("WARNING: No inactive particles available!");
            break;
        }

        Particle3D& p = particles[index];
        p.position = position;
        p.active = true;
        p.maxLife = lifeTime + ((rand() % 1000 / 1000.0f) - 0.5f) * lifeTimeVariance;
        p.life = p.maxLife;

        float theta = (rand() % 1000 / 1000.0f) * 2.0f * PI;
        float phi = (rand() % 1000 / 1000.0f) * PI;
        float speed = 3.0f + (rand() % 1000 / 1000.0f) * 2.0f;

        p.velocity.x = sinf(phi) * cosf(theta) * speed;
        p.velocity.y = sinf(phi) * sinf(theta) * speed;
        p.velocity.z = cosf(phi) * speed;

        p.color = startColor;
        p.size = startSize;
    }
    
    Logger::LogFormat("Explosion complete. Active: %d\n", GetActiveCount());
}

void ParticleSystem3D::EmitContinuous(const Vec3& position, float rate, float deltaTime) {
    emitAccumulator += rate * deltaTime;
    int toEmit = (int)emitAccumulator;
    emitAccumulator -= toEmit;
    
    if (toEmit > 0) {
        Emit(position, toEmit);
    }
}

int ParticleSystem3D::GetActiveCount() const {
    int count = 0;
    for (const auto& p : particles) {
        if (p.active) count++;
    }
    return count;
}

int ParticleSystem3D::FindInactiveParticle() {
    for (int i = 0; i < maxParticles; i++) {
        if (!particles[i].active) return i;
    }
    return -1;
}

void ParticleSystem3D::ActivateParticle(int index, const Vec3& position) {
    Particle3D& p = particles[index];
    p.position = position;
    p.active = true;
    p.maxLife = lifeTime + ((rand() % 1000 / 1000.0f) - 0.5f) * lifeTimeVariance;
    p.life = p.maxLife;

    float rx = ((rand() % 1000 / 1000.0f) - 0.5f) * 2.0f;
    float ry = ((rand() % 1000 / 1000.0f) - 0.5f) * 2.0f;
    float rz = ((rand() % 1000 / 1000.0f) - 0.5f) * 2.0f;

    p.velocity.x = emitVelocity.x + rx * emitVelocityVariance.x;
    p.velocity.y = emitVelocity.y + ry * emitVelocityVariance.y;
    p.velocity.z = emitVelocity.z + rz * emitVelocityVariance.z;

    p.color = startColor;
    p.size = startSize;
}
