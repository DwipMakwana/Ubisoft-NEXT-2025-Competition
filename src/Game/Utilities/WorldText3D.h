//-----------------------------------------------------------------------------
// WorldText3D.h - Simple utility for rendering text at 3D world positions
//-----------------------------------------------------------------------------
#ifndef WORLDTEXT3D_H
#define WORLDTEXT3D_H

#include "../Rendering/Camera3D.h"
#include "../Utilities/MathUtils3D.h"
#include <string>

namespace WorldText3D {

    // Render text at a 3D world position
    // Returns true if text was rendered (on screen), false if behind camera/off-screen
    bool Print(const Vec3& worldPos,
        const char* text,
        const Camera3D& camera,
        float r = 1.0f,
        float g = 1.0f,
        float b = 1.0f,
        void* font = nullptr);

    // Print with Y offset (e.g., above an object)
    bool PrintWithOffset(const Vec3& worldPos,
        float offsetY,
        const char* text,
        const Camera3D& camera,
        float r = 1.0f,
        float g = 1.0f,
        float b = 1.0f,
        void* font = nullptr);
}

#endif
