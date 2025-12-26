//-----------------------------------------------------------------------------
// InputManager.cpp
//-----------------------------------------------------------------------------
#include "InputManager.h"
#include "AppSettings.h"

// Static member initialization
Vec2 InputManager::mousePos(0, 0);
Vec2 InputManager::lastMousePos(0, 0);
Vec2 InputManager::mouseDelta(0, 0);
bool InputManager::mouseButtons[3] = { false, false, false };
bool InputManager::lastMouseButtons[3] = { false, false, false };
bool InputManager::keys[256] = { false };
bool InputManager::lastKeys[256] = { false };

void InputManager::Init() {
    mousePos = Vec2(0, 0);
    lastMousePos = Vec2(0, 0);
    mouseDelta = Vec2(0, 0);

    for (int i = 0; i < 3; i++) {
        mouseButtons[i] = false;
        lastMouseButtons[i] = false;
    }

    for (int i = 0; i < 256; i++) {
        keys[i] = false;
        lastKeys[i] = false;
    }
}

void InputManager::Update() {
    lastMousePos = mousePos;

    // Use direct references if GetMousePos uses references
    float mx, my;
    Internal::GetMousePos(mx, my);  // NO & symbols if it uses references
    mousePos = Vec2(mx, my);

    // Calculate delta
    mouseDelta = Vec2(mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y);

    // Update mouse buttons
    for (int i = 0; i < 3; i++) {
        lastMouseButtons[i] = mouseButtons[i];
        mouseButtons[i] = Internal::IsMousePressed(i);
    }

    // Update keyboard state
    for (int i = 0; i < 256; i++) {
        lastKeys[i] = keys[i];
        keys[i] = Internal::IsKeyPressed(i);
    }
}

Vec2 InputManager::GetMousePosition3D() {
    return mousePos;
}

Vec2 InputManager::GetMouseDelta() {
    return mouseDelta;
}

bool InputManager::GetMouseButton(int button) {
    if (button < 0 || button >= 3) return false;
    return mouseButtons[button];
}

bool InputManager::GetMouseButtonDown(int button) {
    if (button < 0 || button >= 3) return false;
    return mouseButtons[button] && !lastMouseButtons[button];
}

bool InputManager::GetMouseButtonUp(int button) {
    if (button < 0 || button >= 3) return false;
    return !mouseButtons[button] && lastMouseButtons[button];
}

bool InputManager::GetKey(int key) {
    if (key < 0 || key >= 256) return false;
    return keys[key];
}

bool InputManager::GetKeyDown(int key) {
    if (key < 0 || key >= 256) return false;
    return keys[key] && !lastKeys[key];
}

bool InputManager::GetKeyUp(int key) {
    if (key < 0 || key >= 256) return false;
    return !keys[key] && lastKeys[key];
}

bool InputManager::IsAltPressed() {
#ifdef _WIN32
    return GetKey(VK_MENU);
#else
    return GetKey(18); // Alt key code
#endif
}

bool InputManager::IsShiftPressed() {
#ifdef _WIN32
    return GetKey(VK_SHIFT);
#else
    return GetKey(16); // Shift key code
#endif
}

bool InputManager::IsCtrlPressed() {
#ifdef _WIN32
    return GetKey(VK_CONTROL);
#else
    return GetKey(17); // Ctrl key code
#endif
}
