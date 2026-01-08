//-----------------------------------------------------------------------------
// AudioManager.cpp
// Implementation of enhanced audio management system
//-----------------------------------------------------------------------------
#include "AudioManager.h"
#include <algorithm>
#include <Utilities/Logger.h>

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
AudioManager::AudioManager() : initialized(false) {
    InitializeCategoryVolumes();
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
AudioManager::~AudioManager() {
    if (initialized) {
        Shutdown();
    }
}

//-----------------------------------------------------------------------------
// Initialize the audio manager
//-----------------------------------------------------------------------------
void AudioManager::Initialize() {
    if (initialized) {
        Logger::LogWarning("AudioManager already initialized");
        return;
    }

    InitializeCategoryVolumes();
    initialized = true;
    Logger::LogInfo("AudioManager initialized successfully");
}

//-----------------------------------------------------------------------------
// Shutdown and cleanup all audio resources
//-----------------------------------------------------------------------------
void AudioManager::Shutdown() {
    if (!initialized) {
        return;
    }

    StopAllSounds();
    initialized = false;
    Logger::LogInfo("AudioManager shut down");
}

//-----------------------------------------------------------------------------
// Initialize category volumes to default (1.0)
//-----------------------------------------------------------------------------
void AudioManager::InitializeCategoryVolumes() {
    categoryVolumes[AudioCategory::MUSIC] = 1.0f;
    categoryVolumes[AudioCategory::SFX] = 1.0f;
    categoryVolumes[AudioCategory::AMBIENT] = 1.0f;
    categoryVolumes[AudioCategory::UI] = 1.0f;
}

//-----------------------------------------------------------------------------
// Play a sound file with optional looping and category
//-----------------------------------------------------------------------------
void AudioManager::PlaySound(const char* filename, bool loop, AudioCategory category) {
    if (!initialized) {
        Logger::LogError("AudioManager not initialized - cannot play sound");
        return;
    }

    if (filename == nullptr || strlen(filename) == 0) {
        Logger::LogError("Invalid filename provided to PlaySound");
        return;
    }

    // Use the Contest API to play the audio
    App::PlayAudio(filename, loop);

    // Track the sound in our active sounds map
    activeSounds[filename] = { filename, loop, category, 1.0f };

    Logger::LogFormat("Playing sound: %s (loop: %d) \n", filename, loop);
}

//-----------------------------------------------------------------------------
// Stop a specific sound
//-----------------------------------------------------------------------------
void AudioManager::StopSound(const char* filename) {
    if (!initialized || filename == nullptr) {
        return;
    }

    App::StopAudio(filename);
    activeSounds.erase(filename);

    Logger::LogFormat("Stopped sound: %s \n", filename);
}

//-----------------------------------------------------------------------------
// Stop all active sounds
//-----------------------------------------------------------------------------
void AudioManager::StopAllSounds() {
    for (const auto& pair : activeSounds) {
        App::StopAudio(pair.first.c_str());
    }
    activeSounds.clear();

    Logger::LogInfo("Stopped all sounds");
}

//-----------------------------------------------------------------------------
// Stop all sounds in a specific category
//-----------------------------------------------------------------------------
void AudioManager::StopCategory(AudioCategory category) {
    std::vector<std::string> toRemove;

    for (const auto& pair : activeSounds) {
        if (pair.second.category == category) {
            App::StopAudio(pair.first.c_str());
            toRemove.push_back(pair.first);
        }
    }

    for (const auto& key : toRemove) {
        activeSounds.erase(key);
    }

    Logger::LogFormat("Stopped all sounds in category: %d", static_cast<int>(category));
}

//-----------------------------------------------------------------------------
// Check if a sound is currently playing
//-----------------------------------------------------------------------------
bool AudioManager::IsPlaying(const char* filename) const {
    if (!initialized || filename == nullptr) {
        return false;
    }

    return App::IsSoundPlaying(filename);
}

//-----------------------------------------------------------------------------
// Set volume for a specific audio category (0.0 to 1.0)
//-----------------------------------------------------------------------------
void AudioManager::SetCategoryVolume(AudioCategory category, float volume) {
    // Clamp volume between 0.0 and 1.0
    float clampedVolume = (std::max)(0.0f, (std::min)(1.0f, volume));
    categoryVolumes[category] = clampedVolume;

    Logger::LogFormat("Set category %d volume to: %.2f",
        static_cast<int>(category), clampedVolume);
}

//-----------------------------------------------------------------------------
// Get volume for a specific audio category
//-----------------------------------------------------------------------------
float AudioManager::GetCategoryVolume(AudioCategory category) const {
    auto it = categoryVolumes.find(category);
    return (it != categoryVolumes.end()) ? it->second : 1.0f;
}

//-----------------------------------------------------------------------------
// Set master volume for all categories
//-----------------------------------------------------------------------------
void AudioManager::SetMasterVolume(float volume) {
    float clampedVolume = (std::max)(0.0f, (std::min)(1.0f, volume));

    for (auto& pair : categoryVolumes) {
        pair.second = clampedVolume;
    }

    Logger::LogFormat("Set master volume to: %.2f", clampedVolume);
}

//-----------------------------------------------------------------------------
// Get count of currently active sounds
//-----------------------------------------------------------------------------
int AudioManager::GetActiveSoundCount() const {
    return static_cast<int>(activeSounds.size());
}

//-----------------------------------------------------------------------------
// Pause all currently playing sounds
//-----------------------------------------------------------------------------
void AudioManager::PauseAll() {
    // Note: The Contest API doesn't provide pause functionality
    // This would require direct miniaudio access
    // For now, we stop all sounds as a workaround
    Logger::LogWarning("Pause not supported by Contest API - stopping all sounds");
    StopAllSounds();
}

//-----------------------------------------------------------------------------
// Resume all paused sounds
//-----------------------------------------------------------------------------
void AudioManager::ResumeAll() {
    // Note: The Contest API doesn't provide pause/resume functionality
    // This would require tracking paused sounds and replaying them
    Logger::LogWarning("Resume not supported by Contest API");
}
