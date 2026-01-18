//------------------------------------------------------------------------
// Planet.h - Persistent planet system
//------------------------------------------------------------------------
#ifndef PLANET_H
#define PLANET_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Rendering/Renderer3D.h"
#include "../Rendering/Mesh3D.h"
#include <vector>
#include <map>

struct Planet {
    Vec3 position;
    float size;           // Radius (6.0 - 15.0, always 3x+ bigger than asteroids)
    float rotationSpeed;
    float currentRotation;
    bool active;

    // Sector tracking
    int sectorX;
    int sectorY;
};

struct PlanetSectorKey {
    int x, y;

    bool operator<(const PlanetSectorKey& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

class PlanetSystem {
private:
    static const int MAX_PLANETS = 20;           // Pool size
    static const int SECTOR_SIZE = 100;          // Same as asteroids
    static const int PLANETS_PER_SECTOR = 1;    // 1 planet per sector (8 total nearby)

    Planet planets[MAX_PLANETS];
    Mesh3D planetMesh;

    std::map<PlanetSectorKey, bool> loadedSectors;

    Vec3 lastPlayerPos;

    // Helper functions
    void GetSectorCoords(const Vec3& pos, int& sectorX, int& sectorY);
    void LoadSectorsAroundPlayer(const Vec3& playerPos);
    void UnloadDistantSectors(const Vec3& playerPos);
    void GenerateSectorPlanet(int sectorX, int sectorY);

public:
    PlanetSystem();

    void Init();
    void Update(float deltaTime, const Vec3& playerPos);
    void Render(const Camera3D& camera);
    void Clear();

    int GetMaxPlanets() const { return MAX_PLANETS; }
    Planet* GetPlanets() { return planets; }
    const Planet* GetPlanets() const { return planets; }

    // Collision queries
    bool CheckAsteroidCollision(const Vec3& asteroidPos, float asteroidRadius, Vec3& pushDirection);
    bool CheckPlayerCollision(const Vec3& playerPos, float playerRadius, Vec3& pushDirection);

    int GetActivePlanetCount() const;
};

#endif // PLANET_H
