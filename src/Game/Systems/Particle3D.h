//-----------------------------------------------------------------------------
// Particle3D.h
//-----------------------------------------------------------------------------
#ifndef PARTICLE3D_H
#define PARTICLE3D_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include <vector>

struct Particle3D {
    Vec3 position;
    Vec3 velocity;
    Vec3 color;
    float size;
    float life;
    float maxLife;
    bool active;

    Particle3D()
        : position(0, 0, 0)
        , velocity(0, 0, 0)
        , color(1, 1, 1)
        , size(0.1f)
        , life(0)
        , maxLife(1.0f)
        , active(false)
    {
    }
};

class ParticleSystem3D {
public:
    // Configuration
    Vec3 emitVelocity;
    Vec3 emitVelocityVariance;
    Vec3 gravity;
    Vec3 startColor;
    Vec3 endColor;
    float startSize;
    float endSize;
    float lifeTime;
    float lifeTimeVariance;
    bool useGravity;

    ParticleSystem3D(int maxParticles);

    void Update(float deltaTime);
    void Render(const Camera3D& camera);

    void Emit(const Vec3& position, int count);
    void EmitExplosion(const Vec3& position, int count);
    void EmitContinuous(const Vec3& position, float rate, float deltaTime);

    int GetActiveCount() const;

private:
    std::vector<Particle3D> particles;
    int maxParticles;
    float emitAccumulator;

    int FindInactiveParticle();
    void ActivateParticle(int index, const Vec3& position);
    void RenderBillboard(const Vec3& position, float size, const Vec3& color, const Camera3D& camera);
};

#endif // PARTICLE3D_H
