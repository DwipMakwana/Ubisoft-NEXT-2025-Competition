//-----------------------------------------------------------------------------
// InputManager.h
// Unified input handling for keyboard and mouse
//-----------------------------------------------------------------------------
#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "../Utilities/MathUtils3D.h"
#include "Main.h"
#include "App.h"

class InputManager {
public:
    // Initialize input tracking
    static void Init();

    // Update input state (call once per frame)
    static void Update();

    // Mouse position
    static Vec2 GetMousePosition3D();
    static Vec2 GetMouseDelta();

    // Mouse buttons (0=left, 1=right, 2=middle)
    static bool GetMouseButton(int button);
    static bool GetMouseButtonDown(int button);
    static bool GetMouseButtonUp(int button);

    // Keyboard
    static bool GetKey(int key);
    static bool GetKeyDown(int key);
    static bool GetKeyUp(int key);

    // Special keys
    static bool IsAltPressed();
    static bool IsShiftPressed();
    static bool IsCtrlPressed();

private:
    static Vec2 mousePos;
    static Vec2 lastMousePos;
    static Vec2 mouseDelta;

    static bool mouseButtons[3];
    static bool lastMouseButtons[3];

    static bool keys[256];
    static bool lastKeys[256];
};

#endif
