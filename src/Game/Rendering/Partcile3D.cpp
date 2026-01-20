#include "Particle3D.h"
#include "Renderer3D.h"

static unsigned int fastRngState = 123456789;

ParticleSystem3D::ParticleSystem3D(int maxParticles)
    : nextParticleSlot(0), emitAccumulator(0.0f), activeCount(0) {

    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }

    // Thruster-optimized defaults
    startColor = Vec3(1.0f, 0.6f, 0.1f);
    endColor = Vec3(0.8f, 0.2f, 0.05f);
    startSize = 0.8f;
    endSize = 0.1f;
    lifeTime = 0.4f;
    lifeTimeVariance = 0.1f;
    gravity = Vec3(0, -8.0f, 0);
    emitVelocity = Vec3(0, 0, 0);
    emitVelocityVariance = Vec3(3.0f, 3.0f, 1.0f);
    useGravity = true;
}

inline float FastRandom() {
    fastRngState = fastRngState * 1103515245u + 12345u;
    return (float)(fastRngState & 0x7FFFFFFFu) / 2147483647.0f;
}

void ParticleSystem3D::EmitContinuous(const Vec3& position, float particlesPerSecond, float dt) {
    emitAccumulator += particlesPerSecond * dt;

    while (emitAccumulator >= 1.0f && activeCount < MAX_PARTICLES) {
        BillboardParticle& p = particles[nextParticleSlot];

        // Full Vec3 velocity with randomization
        p.position = position;
        p.velocity.x = emitVelocity.x + (FastRandom() * 2.0f - 1.0f) * emitVelocityVariance.x;
        p.velocity.y = emitVelocity.y + (FastRandom() * 2.0f - 1.0f) * emitVelocityVariance.y;
        p.velocity.z = emitVelocity.z + (FastRandom() * 2.0f - 1.0f) * emitVelocityVariance.z;

        p.lifetime = 0.0f;
        p.maxLifetime = lifeTime + FastRandom() * lifeTimeVariance;
        p.active = true;

        nextParticleSlot = (nextParticleSlot + 1) % MAX_PARTICLES;
        activeCount++;
        emitAccumulator -= 1.0f;
    }
}

void ParticleSystem3D::Update(float deltaTime) {
    float dt = deltaTime / 1000.0f;
    int slot = nextParticleSlot;
    int count = activeCount;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[slot].active) {
            slot = (slot + 1) % MAX_PARTICLES;
            continue;
        }

        BillboardParticle& p = particles[slot];
        p.lifetime += dt;

        if (p.lifetime >= p.maxLifetime) {
            p.active = false;
            count--;
        }
        else {
            // Full 3D physics
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.position.z += p.velocity.z * dt;

            // Apply gravity (Vec3)
            if (useGravity) {
                p.velocity.x += gravity.x * dt;
                p.velocity.y += gravity.y * dt;
                p.velocity.z += gravity.z * dt;
            }

            // Branchless interpolation
            float t = p.lifetime / p.maxLifetime;
            p.size = startSize * (1.0f - t) + endSize * t;
            p.color.x = startColor.x * (1.0f - t) + endColor.x * t;
            p.color.y = startColor.y * (1.0f - t) + endColor.y * t;
            p.color.z = startColor.z * (1.0f - t) + endColor.z * t;
        }

        slot = (slot + 1) % MAX_PARTICLES;
    }

    nextParticleSlot = slot;
    activeCount = count;
}

void ParticleSystem3D::Render(const Camera3D& camera) {
    // Cache camera vectors
    Vec3 camRight = camera.GetRight();
    Vec3 camUp = camera.GetUp();
    Vec3 camPos = camera.GetPosition();

    float cullDistSq = 100.0f * 100.0f;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;

        BillboardParticle& p = particles[i];

        // Fast distance culling
        float dx = p.position.x - camPos.x;
        float dy = p.position.y - camPos.y;
        float dz = p.position.z - camPos.z;
        if (dx * dx + dy * dy + dz * dz > cullDistSq) continue;

        float halfSize = p.size * 0.5f;

        // Billboard quad (camera-facing)
        Vec3 v1 = p.position + camRight * halfSize + camUp * halfSize;
        Vec3 v2 = p.position + camRight * halfSize - camUp * halfSize;
        Vec3 v3 = p.position - camRight * halfSize - camUp * halfSize;
        Vec3 v4 = p.position - camRight * halfSize + camUp * halfSize;

        // Two triangles
        Renderer3D::DrawTriangle3D(v1, v2, v3, camera,
            p.color.x, p.color.y, p.color.z,
            p.color.x, p.color.y, p.color.z,
            p.color.x, p.color.y, p.color.z, false);

        Renderer3D::DrawTriangle3D(v1, v3, v4, camera,
            p.color.x, p.color.y, p.color.z,
            p.color.x, p.color.y, p.color.z,
            p.color.x, p.color.y, p.color.z, false);
    }
}
