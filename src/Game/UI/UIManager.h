//-----------------------------------------------------------------------------
// UIManager.h
// Centralized UI rendering and management
//-----------------------------------------------------------------------------
#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <string>
#include <vector>

struct UIText {
    float x, y;
    std::string text;
    float r, g, b;
    void* font;
    bool enabled;

    UIText(float px, float py, const std::string& txt,
        float red = 1.0f, float green = 1.0f, float blue = 1.0f,
        void* fnt = nullptr)
        : x(px), y(py), text(txt), r(red), g(green), b(blue),
        font(fnt), enabled(true) {
    }
};

struct UIBar {
    float x, y;
    float width, height;
    float value;        // 0.0 to 1.0
    float maxValue;
    float r, g, b;
    float bgR, bgG, bgB;
    bool showText;
    std::string label;
    bool enabled;

    UIBar(float px, float py, float w, float h, float val = 1.0f)
        : x(px), y(py), width(w), height(h), value(val), maxValue(1.0f),
        r(0.3f), g(1.0f), b(0.3f),
        bgR(0.2f), bgG(0.2f), bgB(0.2f),
        showText(true), label(""), enabled(true) {
    }
};

class UIManager {
public:
    static void Init();
    static void Shutdown();
    static void Render();
    static void Clear();

    // Text rendering
    static void AddText(const std::string& id, float x, float y, const std::string& text,
        float r = 1.0f, float g = 1.0f, float b = 1.0f, void* font = nullptr);
    static void UpdateText(const std::string& id, const std::string& newText);
    static void SetTextColor(const std::string& id, float r, float g, float b);
    static void SetTextPosition(const std::string& id, float x, float y);
    static void SetTextEnabled(const std::string& id, bool enabled);
    static void RemoveText(const std::string& id);

    // Progress bars
    static void AddBar(const std::string& id, float x, float y, float width, float height, float initialValue = 1.0f);
    static void UpdateBar(const std::string& id, float value);
    static void SetBarColor(const std::string& id, float r, float g, float b);
    static void SetBarLabel(const std::string& id, const std::string& label);
    static void SetBarEnabled(const std::string& id, bool enabled);
    static void RemoveBar(const std::string& id);

    // Quick helpers (for temporary debug text)
    static void DrawDebugText(float x, float y, const std::string& text);
    static void DrawFPS(float deltaTime, float x = 20, float y = 20);

    // Screen dimensions helpers
    static float GetScreenWidth();
    static float GetScreenHeight();

private:
    static std::vector<std::pair<std::string, UIText>> texts;
    static std::vector<std::pair<std::string, UIBar>> bars;

    static float fpsAccumulator;
    static int frameCount;
    static float currentFPS;
};

#endif
