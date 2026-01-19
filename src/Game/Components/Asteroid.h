//------------------------------------------------------------------------
// Asteroid.h - Stationary / physics-driven asteroid field
//------------------------------------------------------------------------

#ifndef ASTEROID_H
#define ASTEROID_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Rendering/Renderer3D.h"
#include "../Rendering/Mesh3D.h"

#include <map>
#include <vector>

class PlanetSystem;

// Minerals carried by asteroids
enum AsteroidMineralType {
    MINERAL_IRON,   // Red
    MINERAL_WATER,  // Blue
    MINERAL_CARBON, // Green
    MINERAL_ENERGY  // Yellow
};

// Single asteroid instance
struct Asteroid {
    // World transform
    Vec3 position;
    Vec3 velocity;

    int claimedByAIShip = -1;

    float size;            // radius
    float mass;

    float rotationSpeed;   // degrees per second
    float currentRotation;

    bool active;

    // Fragmentation (optional)
    bool  isFragment;
    float lifetime;        // remaining lifetime for timed fragments
    float maxLifetime;
    float alpha;           // 0-1 for fade-out

    // Color
    float r, g, b;

    // Sector tracking
    int sectorX;
    int sectorY;
    bool isPersistent;     // survives sector unloads (e.g. fragments if desired)

    // Resource data
    AsteroidMineralType mineralType;
    float resourceAmount;  // 0.0 to 100.0

    // Towing state
    bool isTowed;
};

// Sector key for map
struct SectorKey {
    int x, y;
    bool operator<(const SectorKey& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

class AsteroidSystem {
private:
    static const int MAX_ASTEROIDS = 5000;
    static const int ASTEROIDS_PER_SECTOR = 30;
    static const int SECTOR_SIZE = 100;

    Asteroid asteroids[MAX_ASTEROIDS];
    Mesh3D   asteroidMesh;

    PlanetSystem* planetSystemRef;

    // Sector management
    std::map<SectorKey, bool> loadedSectors;
    Vec3 lastPlayerPos;

    // Spatial grid for collisions (simple 2D grid in worldspace)
    static const int GRID_SIZE = 10;
    float            cellSize;
    std::vector<int> spatialGrid[GRID_SIZE][GRID_SIZE];

    // Fragment tracking (optional, kept for effects)
    struct FragmentInfo {
        Vec3 position;
        AsteroidMineralType mineralType;
        float lifetime;
        bool  collected;
    };
    static const int MAX_TRACKED_FRAGMENTS = 200;
    FragmentInfo trackedFragments[MAX_TRACKED_FRAGMENTS];
    int          nextFragmentSlot;

    // Spawn / culling distances
    float spawnDistance;
    float cullDistance;

    // Helper functions
    int  FindFreeSlot();
    void LoadSectorsAroundPlayer(const Vec3& playerPos);
    void UnloadDistantSectors(const Vec3& playerPos);
    void GenerateSectorAsteroids(int sectorX, int sectorY);

    // Spatial partitioning
    void UpdateSpatialGrid();
    void ClearSpatialGrid();
    void GetGridCell(const Vec3& pos, int& cellX, int& cellY);
    void GetNearbyAsteroids(int cellX, int cellY, std::vector<int>& nearby);

    // Physics / collision
    void ResolveAsteroidCollisions(const Vec3& playerPos);
    void ResolveCollision(int index1, int index2);

    // Fragments
    void BreakAsteroidIntoFragments(int index, const Vec3& impactPoint, const Vec3& impactVel);
    void UpdateFragment(int index, float deltaTime);
    void UpdateFragmentTracking(float deltaTime);

public:
    AsteroidSystem();

    void Init();
    void Update(float deltaTime, const Vec3& playerPos, PlanetSystem* planetSystem);
    void Render(const Camera3D& camera);

    void SetPlanetSystem(PlanetSystem* planetSys) { planetSystemRef = planetSys; }

    void GetSectorCoords(const Vec3& pos, int& sectorX, int& sectorY);

    int  GetActiveAsteroidCount() const;
    int  GetMaxAsteroids() const { return MAX_ASTEROIDS; }

    // Collision queries
    Vec3  CheckPlayerCollision(const Vec3& playerPos, float playerRadius, const Vec3& playerVel);

    void  DestroyAsteroid(int index);
    void  Clear();

    Asteroid* GetAsteroids() { return asteroids; }
    const Asteroid* GetAsteroids() const { return asteroids; }
};

#endif // ASTEROID_H
