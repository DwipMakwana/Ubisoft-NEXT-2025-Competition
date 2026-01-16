//-----------------------------------------------------------------------------
// UIManager.cpp
//-----------------------------------------------------------------------------

#include "UIManager.h"
#include "../Utilities/Logger.h"
#include "../ContestAPI/App.h"
#include <algorithm>

// Static members
std::map<std::string, UIText> UIManager::texts;
std::map<std::string, UISprite> UIManager::sprites;
std::map<std::string, CSimpleSprite*> UIManager::loadedSprites;

void UIManager::Init() {
    texts.clear();
    sprites.clear();
    loadedSprites.clear();
    Logger::LogInfo("UIManager initialized");
}

void UIManager::Shutdown() {
    texts.clear();
    sprites.clear();

    // Clean up loaded sprite textures
    for (auto& pair : loadedSprites) {
        delete pair.second;
    }
    loadedSprites.clear();

    Logger::LogInfo("UIManager shutdown");
}

void UIManager::Update(float dt) {
    // dt is in SECONDS, but CSimpleSprite expects MILLISECONDS
    float dtMilliseconds = dt * 1000.0f;

    // Update sprite animations
    for (auto& pair : sprites) {
        if (pair.second.sprite && pair.second.visible) {
            pair.second.sprite->Update(dtMilliseconds);  // Pass milliseconds
        }
    }
}

void UIManager::Render() {
    // Sort sprites by layer
    std::vector<std::pair<std::string, UISprite*>> sortedSprites;
    for (auto& pair : sprites) {
        if (pair.second.visible) {
            sortedSprites.push_back({ pair.first, &pair.second });
        }
    }

    std::sort(sortedSprites.begin(), sortedSprites.end(),
        [](const auto& a, const auto& b) {
            return a.second->layer < b.second->layer;
        });

    // Render sprites (bottom to top by layer)
    for (auto& pair : sortedSprites) {
        UISprite* s = pair.second;
        if (s->sprite) {
            s->sprite->SetPosition(s->x, s->y);

            // Calculate scale
            float frameWidth = s->sprite->GetWidth();
            float scale = s->width / frameWidth;

            s->sprite->SetScale(scale);
            s->sprite->SetAngle(s->rotation);
            s->sprite->SetColor(s->r, s->g, s->b);

            // Just draw - let CSimpleSprite manage its own frame
            s->sprite->Draw();
        }
    }

    // Render text on top
    for (const auto& pair : texts) {
        const UIText& text = pair.second;
        App::Print(text.x, text.y, text.text.c_str(), text.r, text.g, text.b, text.font);
    }
}

//-----------------------------------------------------------------------------
// Sprite Functions
//-----------------------------------------------------------------------------

bool UIManager::LoadSprite(const std::string& name, const std::string& filepath,
    int frameColumns, int frameRows) {
    if (loadedSprites.find(name) != loadedSprites.end()) {
        Logger::LogWarning(("Sprite '" + name + "' already loaded").c_str());
        return true;
    }

    Logger::LogFormat("Loading sprite: %s from %s (%dx%d frames = %d total frames)\n",
        name.c_str(), filepath.c_str(), frameColumns, frameRows,
        frameColumns * frameRows);

    CSimpleSprite* sprite = App::CreateSprite(filepath.c_str(), frameColumns, frameRows);

    if (sprite != nullptr && sprite->GetWidth() > 0 && sprite->GetHeight() > 0) {
        loadedSprites[name] = sprite;

        // Calculate individual frame dimensions
        float frameWidth = sprite->GetWidth();
        float frameHeight = sprite->GetHeight();

        Logger::LogFormat("SUCCESS: Loaded sprite '%s'\n", name.c_str());
        Logger::LogFormat("  - Texture size: %.0fx%.0f pixels\n", frameWidth, frameHeight);
        Logger::LogFormat("  - Grid: %dx%d frames\n", frameColumns, frameRows);
        Logger::LogFormat("  - Each frame: %.0fx%.0f pixels\n",
            frameWidth / frameColumns, frameHeight / frameRows);
        Logger::LogFormat("  - Total frames: %d (0-%d)\n",
            frameColumns * frameRows, frameColumns * frameRows - 1);

        return true;
    }
    else {
        Logger::LogFormat("ERROR: Failed to load sprite '%s' from %s\n",
            name.c_str(), filepath.c_str());
        if (sprite) delete sprite;
        return false;
    }
}

void UIManager::AddSprite(const std::string& id, const std::string& spriteName,
    float x, float y, float width, float height) {
    auto it = loadedSprites.find(spriteName);
    if (it == loadedSprites.end()) {
        Logger::LogFormat("ERROR: Cannot add sprite '%s': sprite '%s' not loaded\n",
            id.c_str(), spriteName.c_str());
        return;
    }

    UISprite newSprite;
    newSprite.sprite = it->second;
    newSprite.x = x;
    newSprite.y = y;
    newSprite.width = width;
    newSprite.height = height;

    sprites[id] = newSprite;

    Logger::LogFormat("Added sprite '%s' at (%.1f, %.1f) size %.1fx%.1f\n",
        id.c_str(), x, y, width, height);
}

void UIManager::RemoveSprite(const std::string& id) {
    sprites.erase(id);
}

void UIManager::SetSpritePosition(const std::string& id, float x, float y) {
    auto it = sprites.find(id);
    if (it != sprites.end()) {
        it->second.x = x;
        it->second.y = y;
    }
}

void UIManager::SetSpriteSize(const std::string& id, float width, float height) {
    auto it = sprites.find(id);
    if (it != sprites.end()) {
        it->second.width = width;
        it->second.height = height;
    }
}

void UIManager::SetSpriteRotation(const std::string& id, float degrees) {
    auto it = sprites.find(id);
    if (it != sprites.end()) {
        it->second.rotation = degrees;
    }
}

void UIManager::SetSpriteColor(const std::string& id, float r, float g, float b, float a) {
    auto it = sprites.find(id);
    if (it != sprites.end()) {
        it->second.r = r;
        it->second.g = g;
        it->second.b = b;
        it->second.a = a;
    }
}

void UIManager::SetSpriteLayer(const std::string& id, int layer) {
    auto it = sprites.find(id);
    if (it != sprites.end()) {
        it->second.layer = layer;
    }
}

void UIManager::SetSpriteVisible(const std::string& id, bool visible) {
    auto it = sprites.find(id);
    if (it != sprites.end()) {
        it->second.visible = visible;
    }
}

void UIManager::SetSpriteFrame(const std::string& id, int frame) {
    auto it = sprites.find(id);
    if (it != sprites.end() && it->second.sprite) {
        it->second.sprite->SetFrame(frame);
        it->second.currentFrame = frame;
    }
}

void UIManager::PlaySpriteAnimation(const std::string& id, int startFrame, int endFrame, float speed) {
    auto it = sprites.find(id);
    if (it != sprites.end() && it->second.sprite) {
        // Build frame vector
        std::vector<int> frames;
        for (int i = startFrame; i <= endFrame; i++) {
            frames.push_back(i);
        }

        Logger::LogFormat("Creating animation for '%s': %d frames (", id.c_str(), (int)frames.size());
        for (int f : frames) {
            Logger::LogFormat("%d ", f);
        }
        Logger::LogLine(")");

        // Create animation (ID 0, speed in seconds per frame, frame list)
        it->second.sprite->CreateAnimation(0, speed, frames);
        it->second.sprite->SetAnimation(0, true);  // Play from beginning

        Logger::LogFormat("Animation started for '%s'\n", id.c_str());
    }
}

bool UIManager::IsPointInSprite(const std::string& id, float x, float y) {
    auto it = sprites.find(id);
    if (it != sprites.end() && it->second.visible) {
        const UISprite& s = it->second;
        float halfW = s.width / 2.0f;
        float halfH = s.height / 2.0f;
        return (x >= s.x - halfW && x <= s.x + halfW &&
            y >= s.y - halfH && y <= s.y + halfH);
    }
    return false;
}

//-----------------------------------------------------------------------------
// Text Functions
//-----------------------------------------------------------------------------

void UIManager::AddText(const std::string& id, float x, float y, const std::string& text,
    float r, float g, float b, void* font) {
    UIText newText;
    newText.text = text;
    newText.x = x;
    newText.y = y;
    newText.r = r;
    newText.g = g;
    newText.b = b;
    newText.font = font;
    texts[id] = newText;
}

void UIManager::UpdateText(const std::string& id, const std::string& newText) {
    auto it = texts.find(id);
    if (it != texts.end()) {
        it->second.text = newText;
    }
}

void UIManager::RemoveText(const std::string& id) {
    texts.erase(id);
}

void UIManager::SetTextPosition(const std::string& id, float x, float y) {
    auto it = texts.find(id);
    if (it != texts.end()) {
        it->second.x = x;
        it->second.y = y;
    }
}

void UIManager::SetTextColor(const std::string& id, float r, float g, float b) {
    auto it = texts.find(id);
    if (it != texts.end()) {
        it->second.r = r;
        it->second.g = g;
        it->second.b = b;
    }
}
