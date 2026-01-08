//-----------------------------------------------------------------------------
// AudioManager.h
// Enhanced audio management with volume control and categories
//-----------------------------------------------------------------------------
#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "App.h"
#include <string>
#include <vector>
#include <unordered_map>

enum class AudioCategory {
    MUSIC,
    SFX,
    AMBIENT,
    UI
};

class AudioManager {
private:
    struct Sound {
        std::string filename;
        bool isLooping;
        AudioCategory category;
        float volume;
    };

    std::unordered_map<std::string, Sound> activeSounds;
    std::unordered_map<AudioCategory, float> categoryVolumes;
    bool initialized;

public:
    AudioManager();
    ~AudioManager();

    // Initialization and cleanup
    void Initialize();
    void Shutdown();

    // Sound playback control
    void PlaySound(const char* filename, bool loop = false,
        AudioCategory category = AudioCategory::SFX);
    void StopSound(const char* filename);
    void StopAllSounds();
    void StopCategory(AudioCategory category);

    // Query functions
    bool IsPlaying(const char* filename) const;
    bool IsInitialized() const { return initialized; }

    // Volume control
    void SetCategoryVolume(AudioCategory category, float volume);
    float GetCategoryVolume(AudioCategory category) const;
    void SetMasterVolume(float volume);

    // Utility functions
    int GetActiveSoundCount() const;
    void PauseAll();
    void ResumeAll();

private:
    void InitializeCategoryVolumes();
};

#endif // AUDIO_MANAGER_H
