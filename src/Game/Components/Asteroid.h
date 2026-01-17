//------------------------------------------------------------------------
// Asteroid.h - Optimized asteroid system
//------------------------------------------------------------------------
#ifndef ASTEROID_H
#define ASTEROID_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Rendering/Renderer3D.h"
#include "../Rendering/Mesh3D.h"
#include <vector>

struct Asteroid {
    Vec3 position;
    Vec3 velocity;
    float size;
    float mass;
    float rotationSpeed;
    float currentRotation;
    bool active;

    // Fragmentation system
    bool isFragment;
    float lifetime;
    float maxLifetime;
    float alpha;

    // Color
    float r, g, b;
};

class AsteroidSystem {
private:
    static const int MAX_ASTEROIDS = 400;
    static const int GRID_SIZE = 10;

    Asteroid asteroids[MAX_ASTEROIDS];
    Mesh3D asteroidMesh;

    // Spawn settings
    float spawnTimer;
    float spawnInterval;
    float spawnDistance;

    // Asteroid properties
    float minSize;
    float maxSize;
    float minSpeed;
    float maxSpeed;

    // Physics
    float restitution;

    // Fragmentation settings
    float fragmentMinSize;
    int fragmentCount;
    float fragmentSpeed;

    // Spatial partitioning
    float cellSize;
    std::vector<int> spatialGrid[10][10];  // Use literal instead of GRID_SIZE

    // Optimization
    int nextFreeIndex;

    // Helper functions
    void UpdateSpatialGrid();
    void GetGridCell(const Vec3& pos, int& cellX, int& cellY);
    void GetNearbyAsteroids(int cellX, int cellY, std::vector<int>& nearby);
    int FindFreeSlot();

public:
    AsteroidSystem();

    void Init();
    void Update(float deltaTime, const Vec3& earthPos);
    void Render(const Camera3D& camera);

    void SpawnAsteroid(const Vec3& earthPos);

    int GetActiveAsteroidCount() const;

    int CheckBulletCollision(const Vec3& bulletPos, const Vec3& bulletVel, float bulletRadius);

    void CheckPlayerCollision(const Vec3& playerPos, float playerRadius, const Vec3& playerVel);

    void DestroyAsteroid(int index);
    void Clear();

private:
    void ResolveCollision(int index1, int index2);
    void BreakAsteroidIntoFragments(int index, const Vec3& impactPoint, const Vec3& impactVel);
    void UpdateFragment(int index, float deltaTime);
};

#endif
