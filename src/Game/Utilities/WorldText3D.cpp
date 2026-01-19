//-----------------------------------------------------------------------------
// WorldText3D.cpp - Simple 3D text rendering utility
//-----------------------------------------------------------------------------
#include "WorldText3D.h"
#include "App.h"

namespace WorldText3D {

    bool Print(const Vec3& worldPos,
        const char* text,
        const Camera3D& camera,
        float r, float g, float b,
        void* font) {

        // Use GLUT_BITMAP_HELVETICA_12 as default if no font specified
        if (font == nullptr) {
            font = GLUT_BITMAP_HELVETICA_12;
        }

        // 1. Transform world position to NDC (Normalized Device Coordinates)
        Vec4 ndc = camera.WorldToNDC(worldPos);

        // 2. Check if behind camera
        if (ndc.w < camera.GetNearPlane()) {
            return false;  // Behind camera
        }

        // 3. Check if outside screen bounds (with some margin)
        if (ndc.x < -1.5f || ndc.x > 1.5f ||
            ndc.y < -1.5f || ndc.y > 1.5f) {
            return false;  // Off screen
        }

        // 4. Convert NDC (-1 to +1) to screen coordinates
        float screenX = (ndc.x + 1.0f) * (APP_VIRTUAL_WIDTH * 0.5f);
        float screenY = (ndc.y + 1.0f) * (APP_VIRTUAL_HEIGHT * 0.5f);

        // 5. Render text at screen position
        App::Print(screenX, screenY, text, r, g, b, font);

        return true;
    }

    bool PrintWithOffset(const Vec3& worldPos,
        float offsetY,
        const char* text,
        const Camera3D& camera,
        float r, float g, float b,
        void* font) {

        // Apply offset in world space (Y-up)
        Vec3 offsetPos = worldPos;
        offsetPos.y += offsetY;

        return Print(offsetPos, text, camera, r, g, b, font);
    }

} // namespace WorldText3D
