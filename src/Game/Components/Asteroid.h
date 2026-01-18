//------------------------------------------------------------------------
// Asteroid.h - AI-driven asteroid types
//------------------------------------------------------------------------
#ifndef ASTEROID_H
#define ASTEROID_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Rendering/Renderer3D.h"
#include "../Rendering/Mesh3D.h"
#include <vector>
#include <map>

// NEW: Asteroid types
enum AsteroidType {
    WANDERER,   // Passive drifters (gray)
    ORBITER,    // Orbit large asteroids (blue-gray)
    FLOCKING    // Travel in groups (green-gray)
};

struct Asteroid {
    Vec3 position;
    Vec3 velocity;
    float size;
    float mass;
    float rotationSpeed;
    float currentRotation;
    bool active;

    // Fragmentation
    bool isFragment;
    float lifetime;
    float maxLifetime;
    float alpha;

    // Color
    float r, g, b;

    // Sector tracking
    int sectorX;
    int sectorY;
    bool isPersistent;

    // AI properties
    AsteroidType type;

    // Orbiter AI
    int orbitTargetIndex;       // CHANGED: Now indexes into PlanetSystem, not asteroids
    bool orbitingPlanet;        // NEW: True if orbiting planet, false if orbiting asteroid
    float orbitAngle;
    float orbitRadius;
    float orbitSpeed;
    bool orbitClockwise;

    // Flocking AI
    Vec3 flockingForce;

    // NEW: Wanderer patrol AI
    Vec3 patrolTarget;      // Destination position
    float patrolWaitTimer;  // Time to wait at destination
    float patrolWaitTime;   // How long to wait (random)
    bool patrolWaiting;     // Currently waiting?
};

struct SectorKey {
    int x, y;

    bool operator<(const SectorKey& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

class AsteroidSystem {
private:
    static const int MAX_ASTEROIDS = 500;
    static const int GRID_SIZE = 10;
    static const int ASTEROIDS_PER_SECTOR = 20;
    static const int SECTOR_SIZE = 100;

    Asteroid asteroids[MAX_ASTEROIDS];
    Mesh3D asteroidMesh;

    std::map<SectorKey, bool> loadedSectors;

    // Spawn settings
    float spawnDistance;
    float cullDistance;

    // Asteroid properties
    float minSize;
    float maxSize;
    float minSpeed;
    float maxSpeed;

    // Physics
    float restitution;

    // Fragmentation
    float fragmentMinSize;
    int fragmentCount;
    float fragmentSpeed;

    // Spatial partitioning
    float cellSize;
    std::vector<int> spatialGrid[10][10];

    // Helper functions
    void UpdateSpatialGrid();
    void GetGridCell(const Vec3& pos, int& cellX, int& cellY);
    void GetNearbyAsteroids(int cellX, int cellY, std::vector<int>& nearby);

    Vec3 lastPlayerPos;

    // Sector management
    void GetSectorCoords(const Vec3& pos, int& sectorX, int& sectorY);
    void LoadSectorsAroundPlayer(const Vec3& playerPos);
    void UnloadDistantSectors(const Vec3& playerPos);
    void GenerateSectorAsteroids(int sectorX, int sectorY);

    // AI functions
    void UpdateWandererAI(int index, float deltaTime);
    void UpdateOrbiterAI(int index, float deltaTime, class PlanetSystem* planetSystem);
    void UpdateFlockingAI(int index, float deltaTime);

    void FindOrbitTarget(int index, class PlanetSystem* planetSystem);

    int FindNearestPlanet(const Vec3& pos, float maxDistance, class PlanetSystem* planetSystem);
    void GetFlockingNeighbors(int index, std::vector<int>& neighbors, float radius);

public:
    AsteroidSystem();

    void Init();
    void Update(float deltaTime, const Vec3& playerPos, class PlanetSystem* planetSystem);
    void Render(const Camera3D& camera);

    int GetActiveAsteroidCount() const;
    int CheckBulletCollision(const Vec3& bulletPos, const Vec3& bulletVel, float bulletRadius);
    Vec3 CheckPlayerCollision(const Vec3& playerPos, float playerRadius, const Vec3& playerVel);
    void DestroyAsteroid(int index);
    void Clear();

    Asteroid* GetAsteroids() { return asteroids; }
    int GetMaxAsteroids() const { return MAX_ASTEROIDS; }

private:
    void ResolveCollision(int index1, int index2);
    void BreakAsteroidIntoFragments(int index, const Vec3& impactPoint, const Vec3& impactVel);
    void UpdateFragment(int index, float deltaTime);
};

#endif // ASTEROID_H
