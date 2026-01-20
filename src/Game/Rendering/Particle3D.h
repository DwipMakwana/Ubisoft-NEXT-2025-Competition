#ifndef PARTICLESYSTEM3D_H
#define PARTICLESYSTEM3D_H

#include "../Utilities/MathUtils3D.h"
#include "Camera3D.h"

struct BillboardParticle {
    Vec3 position;
    Vec3 velocity;        // Vec3 for full 3D control
    float lifetime;
    float maxLifetime;
    float size;
    Vec3 color;
    bool active;
};

class ParticleSystem3D {
private:
    static const int MAX_PARTICLES = 1000;
    BillboardParticle particles[MAX_PARTICLES];
    int nextParticleSlot;
    float emitAccumulator;
    int activeCount;

public:
    ParticleSystem3D(int maxParticles);

    // Public settings
    Vec3 startColor, endColor;
    float startSize, endSize;
    float lifeTime, lifeTimeVariance;
    Vec3 gravity;              // Full Vec3 gravity
    Vec3 emitVelocity;         // Full Vec3 velocity
    Vec3 emitVelocityVariance; // Full Vec3 variance
    bool useGravity;

    void EmitContinuous(const Vec3& position, float particlesPerSecond, float dt);
    void Update(float deltaTime);
    void Render(const Camera3D& camera);
    int GetActiveParticleCount() const { return activeCount; }
};

#endif
