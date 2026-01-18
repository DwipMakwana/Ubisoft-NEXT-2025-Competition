//------------------------------------------------------------------------
// StarField.h - Simple infinite star generation (no sectors)
//------------------------------------------------------------------------
#ifndef STARFIELD_H
#define STARFIELD_H

#include "../Utilities/MathUtils3D.h"
#include "../Rendering/Camera3D.h"
#include "../Rendering/Renderer3D.h"
#include "../Rendering/Mesh3D.h"
#include <vector>

struct Star {
    Vec3 position;       // World position
    float brightness;    // 0.0 to 1.0
    float size;          // Render size
};

class StarField {
private:
    std::vector<Star> stars;
    Mesh3D starMesh;

    // Settings
    int maxStars;              // Maximum stars to keep active
    float spawnDistance;       // Distance from player to spawn stars
    float cullDistance;        // Distance from player to remove stars

    Vec3 lastPlayerPos;        // Track player movement

    // Helper functions
    void SpawnStarsAroundPlayer(const Vec3& playerPos, int count);
    void SpawnStarsInDirection(const Vec3& playerPos, float dirX, float dirY, int count);
    void CullDistantStars(const Vec3& playerPos);
    float GetRandomFloat(float min, float max);

public:
    StarField();
    void Init(const Vec3& initialPos);
    void Update(const Vec3& playerPos);
    void Render(const Camera3D& camera);
    int GetActiveStarCount() const { return static_cast<int>(stars.size()); }
};

#endif // STARFIELD_H
