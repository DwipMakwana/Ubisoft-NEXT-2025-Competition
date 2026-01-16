//-----------------------------------------------------------------------------
// UIManager.h
// Unified UI system for text and sprites
//-----------------------------------------------------------------------------

#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <string>
#include <map>
#include "../ContestAPI/SimpleSprite.h"

// Text element structure
struct UIText {
    std::string text;
    float x, y;
    float r, g, b;
    void* font;
};

// Sprite element structure
struct UISprite {
    CSimpleSprite* sprite;
    float x, y;
    float width, height;
    float rotation;
    float r, g, b, a;
    int layer;
    bool visible;
    int currentFrame;

    UISprite()
        : sprite(nullptr)
        , x(0), y(0)
        , width(100), height(100)
        , rotation(0)
        , r(1), g(1), b(1), a(1)
        , layer(0)
        , visible(true)
        , currentFrame(0)
    {
    }
};

class UIManager {
public:
    static void Init();
    static void Shutdown();
    static void Update(float dt);
    static void Render();

    // Text functions
    static void AddText(const std::string& id, float x, float y, const std::string& text,
        float r, float g, float b, void* font);
    static void UpdateText(const std::string& id, const std::string& newText);
    static void RemoveText(const std::string& id);
    static void SetTextPosition(const std::string& id, float x, float y);
    static void SetTextColor(const std::string& id, float r, float g, float b);

    // Sprite functions
    static bool LoadSprite(const std::string& name, const std::string& filepath,
        int frameColumns = 1, int frameRows = 1);
    static void AddSprite(const std::string& id, const std::string& spriteName,
        float x, float y, float width, float height);
    static void RemoveSprite(const std::string& id);
    static void SetSpritePosition(const std::string& id, float x, float y);
    static void SetSpriteSize(const std::string& id, float width, float height);
    static void SetSpriteRotation(const std::string& id, float degrees);
    static void SetSpriteColor(const std::string& id, float r, float g, float b, float a = 1.0f);
    static void SetSpriteLayer(const std::string& id, int layer);
    static void SetSpriteVisible(const std::string& id, bool visible);
    static void SetSpriteFrame(const std::string& id, int frame);
    static void PlaySpriteAnimation(const std::string& id, int startFrame, int endFrame, float speed);

    // Utility
    static bool IsPointInSprite(const std::string& id, float x, float y);

private:
    static std::map<std::string, UIText> texts;
    static std::map<std::string, UISprite> sprites;
    static std::map<std::string, CSimpleSprite*> loadedSprites;
};

#endif
