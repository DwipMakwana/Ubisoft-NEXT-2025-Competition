//-----------------------------------------------------------------------------
// UIManager.cpp
//-----------------------------------------------------------------------------
#include "UIManager.h"
#include "App.h"
#include "AppSettings.h"
#include <cstdio>
#include <algorithm>

// Static member initialization
std::vector<std::pair<std::string, UIText>> UIManager::texts;
std::vector<std::pair<std::string, UIBar>> UIManager::bars;
float UIManager::fpsAccumulator = 0.0f;
int UIManager::frameCount = 0;
float UIManager::currentFPS = 60.0f;

void UIManager::Init() {
    texts.clear();
    bars.clear();
    fpsAccumulator = 0.0f;
    frameCount = 0;
    currentFPS = 60.0f;
}

void UIManager::Shutdown() {
    texts.clear();
    bars.clear();
}

void UIManager::Render() {
    // Render all texts
    for (auto& pair : texts) {
        UIText& text = pair.second;
        if (text.enabled) {
            void* font = text.font ? text.font : GLUT_BITMAP_HELVETICA_12;
            App::Print(text.x, text.y, text.text.c_str(), text.r, text.g, text.b, font);
        }
    }

    // Render all bars
    for (auto& pair : bars) {
        UIBar& bar = pair.second;
        if (!bar.enabled) continue;

        // Background
        App::DrawLine(bar.x, bar.y, bar.x + bar.width, bar.y, bar.bgR, bar.bgG, bar.bgB);
        App::DrawLine(bar.x, bar.y + bar.height, bar.x + bar.width, bar.y + bar.height,
            bar.bgR, bar.bgG, bar.bgB);
        App::DrawLine(bar.x, bar.y, bar.x, bar.y + bar.height, bar.bgR, bar.bgG, bar.bgB);
        App::DrawLine(bar.x + bar.width, bar.y, bar.x + bar.width, bar.y + bar.height,
            bar.bgR, bar.bgG, bar.bgB);

        // Filled portion
        float fillWidth = bar.width * (bar.value / bar.maxValue);
        if (fillWidth > 0) {
            for (float i = 0; i < bar.height; i += 2.0f) {
                App::DrawLine(bar.x, bar.y + i, bar.x + fillWidth, bar.y + i,
                    bar.r, bar.g, bar.b);
            }
        }

        // Label text
        if (bar.showText && !bar.label.empty()) {
            char labelText[128];
            snprintf(labelText, sizeof(labelText), "%s: %.0f%%",
                bar.label.c_str(), (bar.value / bar.maxValue) * 100.0f);
            App::Print(bar.x + 5, bar.y + bar.height / 2 - 5, labelText,
                1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
        }
    }
}

void UIManager::Clear() {
    texts.clear();
    bars.clear();
}

void UIManager::AddText(const std::string& id, float x, float y, const std::string& text,
    float r, float g, float b, void* font) {
    // Remove if exists
    RemoveText(id);

    UIText uiText(x, y, text, r, g, b, font);
    texts.push_back(std::make_pair(id, uiText));
}

void UIManager::UpdateText(const std::string& id, const std::string& newText) {
    for (auto& pair : texts) {
        if (pair.first == id) {
            pair.second.text = newText;
            return;
        }
    }
}

void UIManager::SetTextColor(const std::string& id, float r, float g, float b) {
    for (auto& pair : texts) {
        if (pair.first == id) {
            pair.second.r = r;
            pair.second.g = g;
            pair.second.b = b;
            return;
        }
    }
}

void UIManager::SetTextPosition(const std::string& id, float x, float y) {
    for (auto& pair : texts) {
        if (pair.first == id) {
            pair.second.x = x;
            pair.second.y = y;
            return;
        }
    }
}

void UIManager::SetTextEnabled(const std::string& id, bool enabled) {
    for (auto& pair : texts) {
        if (pair.first == id) {
            pair.second.enabled = enabled;
            return;
        }
    }
}

void UIManager::RemoveText(const std::string& id) {
    texts.erase(
        std::remove_if(texts.begin(), texts.end(),
            [&id](const std::pair<std::string, UIText>& pair) {
                return pair.first == id;
            }),
        texts.end()
    );
}

void UIManager::AddBar(const std::string& id, float x, float y, float width, float height,
    float initialValue) {
    // Remove if exists
    RemoveBar(id);

    UIBar bar(x, y, width, height, initialValue);
    bars.push_back(std::make_pair(id, bar));
}

void UIManager::UpdateBar(const std::string& id, float value) {
    for (auto& pair : bars) {
        if (pair.first == id) {
            pair.second.value = value;
            if (pair.second.value < 0.0f) pair.second.value = 0.0f;
            if (pair.second.value > pair.second.maxValue) {
                pair.second.value = pair.second.maxValue;
            }
            return;
        }
    }
}

void UIManager::SetBarColor(const std::string& id, float r, float g, float b) {
    for (auto& pair : bars) {
        if (pair.first == id) {
            pair.second.r = r;
            pair.second.g = g;
            pair.second.b = b;
            return;
        }
    }
}

void UIManager::SetBarLabel(const std::string& id, const std::string& label) {
    for (auto& pair : bars) {
        if (pair.first == id) {
            pair.second.label = label;
            return;
        }
    }
}

void UIManager::SetBarEnabled(const std::string& id, bool enabled) {
    for (auto& pair : bars) {
        if (pair.first == id) {
            pair.second.enabled = enabled;
            return;
        }
    }
}

void UIManager::RemoveBar(const std::string& id) {
    bars.erase(
        std::remove_if(bars.begin(), bars.end(),
            [&id](const std::pair<std::string, UIBar>& pair) {
                return pair.first == id;
            }),
        bars.end()
    );
}

void UIManager::DrawDebugText(float x, float y, const std::string& text) {
    App::Print(x, y, text.c_str(), 1.0f, 1.0f, 0.0f, GLUT_BITMAP_HELVETICA_10);
}

void UIManager::DrawFPS(float deltaTime, float x, float y) {
    fpsAccumulator += deltaTime;
    frameCount++;

    if (fpsAccumulator >= 1000.0f) {
        currentFPS = frameCount / (fpsAccumulator / 1000.0f);
        fpsAccumulator = 0.0f;
        frameCount = 0;
    }

    char fpsText[64];
    snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", currentFPS);
    App::Print(x, y, fpsText, 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
}

float UIManager::GetScreenWidth() {
    return APP_VIRTUAL_WIDTH;
}

float UIManager::GetScreenHeight() {
    return APP_VIRTUAL_HEIGHT;
}
